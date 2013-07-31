/*
 * at88x.c  --  AT88SC0104C CryptoMemory driver
 *
 * Copyright 2011 FOXCONN TMSBG
 *
 * Author: Amos Chen <xd-ch@163.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/ioctl.h>
#include <linux/proc_fs.h>
#include <asm/gpio.h>
#include <asm/uaccess.h>
#include <asm/mach/irq.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/miscdevice.h>


#define AT88X_RDWR 0xa
#define AT88X_RD 1
#define AT88X_WR 0

#define AT88X_MINOR MISC_DYNAMIC_MINOR

/*
struct at88x_dev {
	struct cdev cdev; 
};
struct at88x_dev *at88x_devp;
*/

struct at88x_algo {
	unsigned int	dat_pin;
	unsigned int	clk_pin;
	unsigned int	udelay;
	unsigned int	timeout;
	unsigned int	retries;
};

static struct at88x_algo at88x_algo = {
     .dat_pin        = S5PV210_GPC0(4),
     .clk_pin        = S5PV210_GPJ0(6),
     .udelay         = 5,
//     .timeout        = HZ / 10,
	 .retries        = 4,
};

struct at88x_msg {
//  __u16 addr;		/* slave address			*/
	__u16 flags;	/* indicates read or write	*/
	__u8  reg[3];	/* regs relevant parameters */
	__u16 len;		/* msg length				*/
	__u8  *buf;		/* pointer to msg data		*/
};

/* Toggle SDA by changing the direction of the pin */
static void at88x_gpio_setdat_dir(struct at88x_algo *adap, int state)
{
	if (state)
		gpio_direction_input(adap->dat_pin);
	else
		gpio_direction_output(adap->dat_pin, 0);
}

/*
 * Toggle SDA by changing the output value of the pin. This is only
 * valid for pins configured as open drain (i.e. setting the value
 * high effectively turns off the output driver.)
 */
static void at88x_gpio_setdat_val(struct at88x_algo *adap, int state)
{
	gpio_set_value(adap->dat_pin, state);
}

/* Toggle SCL by changing the direction of the pin. */
static void at88x_gpio_setclk_dir(struct at88x_algo *adap, int state)
{
	if (state)
		gpio_direction_input(adap->clk_pin);
	else
		gpio_direction_output(adap->clk_pin, 0);
}

/*
 * Toggle SCL by changing the output value of the pin. This is used
 * for pins that are configured as open drain and for output-only
 * pins. The latter case will break the at88x protocol, but it will
 * often work in practice.
 */
static void at88x_gpio_setclk_val(struct at88x_algo *adap, int state)
{
	gpio_set_value(adap->clk_pin, state);
}

static int at88x_gpio_getdat(struct at88x_algo *adap)
{
	return gpio_get_value(adap->dat_pin);
}

static int at88x_gpio_getclk(struct at88x_algo *adap)
{
	return gpio_get_value(adap->clk_pin);
}

static inline void datlo(struct at88x_algo *adap)
{
	at88x_gpio_setdat_dir(adap, 0);
	udelay((adap->udelay + 1) / 2);
}

static inline void dathi(struct at88x_algo *adap)
{
	at88x_gpio_setdat_dir(adap, 1);
	udelay((adap->udelay + 1) / 2);
}

static inline void clklo(struct at88x_algo *adap)
{
	at88x_gpio_setclk_dir(adap, 0);
	udelay(adap->udelay / 2);
}

static inline void clkhi(struct at88x_algo *adap)
{
	at88x_gpio_setclk_dir(adap, 1);
	udelay(adap->udelay);
}

/* --- other auxiliary functions --------------------------------------	*/
static void at88x_start(struct at88x_algo *adap)
{
	/* assert: clk, dat are high */
	at88x_gpio_setdat_dir(adap, 0);
	udelay(adap->udelay);
	clklo(adap);
}

static void at88x_repstart(struct at88x_algo *adap)
{
	/* assert: clk is low */
	dathi(adap);
	clkhi(adap);
	at88x_gpio_setdat_dir(adap, 0);
	udelay(adap->udelay);
	clklo(adap);
}


static void at88x_stop(struct at88x_algo *adap)
{
	/* assert: clk is low */
	datlo(adap);
	clkhi(adap);
	at88x_gpio_setdat_dir(adap, 1);
	udelay(adap->udelay);
}

/* send a byte without start cond., check ackn. from slave 
 * returns:
 * 1 if the device acknowledged
 * 0 if the device did not ack
 */
static int at88x_outb(struct at88x_algo *adap, unsigned char c)
{
	int i;
	int sb;
	int ack;

	/* assert: clk is low */
	for (i = 7; i >= 0; i--) {
		sb = (c >> i) & 1;
		at88x_gpio_setdat_dir(adap, sb);
		udelay((adap->udelay + 1) / 2);
		clkhi(adap);
		clklo(adap);
	}
	dathi(adap);
	clkhi(adap);
	
	/* read ack: SDA should be pulled down by slave, or it may
	 * NAK (usually to report problems with the data we wrote).
	 */
	ack = !at88x_gpio_getdat(adap);    /* ack: dat is pulled low -> success */
	clklo(adap);
	
	return ack;
	/* assert: clk is low (dat undef) */
}

static int at88x_inb(struct at88x_algo *adap)
{
	/* read byte via at88x port, without start/stop sequence	*/
	/* acknowledge is sent in at88x_read.			*/
	int i;
	unsigned char indata = 0;
	
	/* assert: clk is low */
	dathi(adap);
	for (i = 0; i < 8; i++) {
		clkhi(adap);
		indata *= 2;
		if (at88x_gpio_getdat(adap))
			indata |= 0x01;
		at88x_gpio_setclk_dir(adap, 0);
		udelay(i == 7 ? adap->udelay / 2 : adap->udelay);
	}
	/* assert: clk is low */
	return indata;
}

static int sendbytes(struct at88x_algo *adap, struct at88x_msg *msg)
{
	const unsigned char *temp = msg->buf;
	int count = msg->len;
	int retval;
	int wrcount = 0;

	while (count > 0) {
		retval = at88x_outb(adap, *temp);

		/* OK/ACK; or ignored NAK */
		if (retval > 0) {
			count--;
			temp++;
			wrcount++;
		} else if (retval == 0) {
			return -EIO;
		} else {
			return retval;
		}
	}
	return wrcount;
}

static int is_ack(struct at88x_algo *adap, int is_ack)
{
	/* send ack */
	if (is_ack)
		at88x_gpio_setdat_dir(adap, 0);
	udelay((adap->udelay + 1) / 2);
	clkhi(adap);
	clklo(adap);
	return 0;
}

static int readbytes(struct at88x_algo *adap, struct at88x_msg *msg)
{
	int inval;
	int rdcount = 0;	/* counts bytes read */
	unsigned char *temp = msg->buf;
	int count = msg->len;

	while (count > 0) {
		inval = at88x_inb(adap);
		if (inval >= 0) {
			*temp = inval;
			rdcount++;
		} else { 
			break;
		}
		temp++;
		count--;
//		printk("read %d, 0x%x\n", rdcount, inval);
		is_ack(adap, count);
		}
	
	return rdcount;
}

/* doAddress initiates the transfer by generating the start condition 
 * and transmits the address in the necessary format to handle
 * reads, writes.
 * returns:
 *  0 everything went okay, the chip ack'ed
 * -x an error occurred (like: -EREMOTEIO if the device did not answer, or
 *	-ETIMEDOUT, for example if the lines are stuck...)
 */
static int doAddress(struct at88x_algo *adap, unsigned char addr)
{
	int retries = adap->retries;
	int i, ret = 0;

	for (i = 0; i <= retries; i++) {
		ret = at88x_outb(adap, (addr << 1));
		if (ret == 1 || i == retries)
			break;
		at88x_stop(adap);
		udelay(adap->udelay);
		at88x_start(adap);
	}
	if (ret != 1)
		return -ENXIO;

	return 0;
}

static int send_para(struct at88x_algo *adap, struct at88x_msg *msg)
{
	int count = 3;
	int retval;
	int wrcount = 0;

	while (count > 0) {
//		printk("send_para msg->reg[%d] = 0x%x\n", wrcount, msg->reg[wrcount]);
		retval = at88x_outb(adap, msg->reg[wrcount]);
		/* OK/ACK */
		if (retval > 0) {
			count--;
			wrcount++;
		} else if (retval == 0) {
			return -EIO;
		} else {
			return retval;
		}
	}
	return wrcount;
}
static int at88x_transfer(struct at88x_algo *adap, struct at88x_msg *msg)
{
	struct at88x_msg *pmsg = msg;
	unsigned char addr;
	int ret;

	if (pmsg->flags & AT88X_RD)
		addr = 0x5b;
	else
		addr = 0x5a;

	at88x_start(adap);
	ret = doAddress(adap, addr);
	if (ret != 0)  {
				goto bailout;
	}

	/* write location releavant data to at88x */
	ret = send_para(adap, msg);
	if (ret < 3) {
		printk("Send parameters error...!\n");
		ret = -EREMOTEIO;
		goto bailout;
	}
		
	if (pmsg->flags & AT88X_RD) {
		/* read bytes into buffer*/
		ret = readbytes(adap, pmsg);
		if (ret >= 1)
//			printk("read %d byte%s\n",
//				ret, ret == 1 ? "" : "s");
		if (ret < pmsg->len) {
			if (ret >= 0)
				ret = -EREMOTEIO;
			goto bailout;
		}
	} else {
		/* write bytes from buffer */
		ret = sendbytes(adap, pmsg);
		if (ret >= 1)
//			printk("wrote %d byte%s\n",
//				ret, ret == 1 ? "" : "s");
		if (ret < pmsg->len) {
			if (ret >= 0)
				ret = -EREMOTEIO;
			goto bailout;
		}
	}

bailout:
	at88x_stop(adap);
	return ret;
}

static int at88x_open(struct inode *inode, struct file *file)
{                                                                                                                                             
	int i_return = -ENODEV;                                                                                                                   
                                                                                                                                            
    if(!try_module_get(THIS_MODULE))                                                                                                          
		goto OUT;                                                                                                                             
	i_return = 0;                                                                                                                             
    printk("open iicx driver for at88x\n");                                                                                                 

OUT:                                                                                                                                   
    return i_return;                                                                                                                          
}                                                                                                                                   

static int at88x_read()
{
	return 0;
}

static int at88x_write()
{
	return 0;
}

static int at88x_release(struct inode *inode, struct file *file)                                                                             
{                                                                                                                                             
	module_put(THIS_MODULE);                                                                                                                  
//	printk("release iicx driver for at88x\n");                                                                                              
	return 0;                                                                                                                                 
}                                                                                                                                             

static int at88x_ioctl_rdwr(unsigned long arg)
{
	struct at88x_msg rdwr_msg;
	struct at88x_algo adap = at88x_algo;
	u8 __user *data_ptrs;
	int res;

	if (copy_from_user(&rdwr_msg, (struct at88x_msg __user *)arg, sizeof(rdwr_msg)))
		return -EFAULT;

	if ((rdwr_msg.len > 8192))
		return -EINVAL;

	data_ptrs = (u8 __user *)rdwr_msg.buf;
	rdwr_msg.buf = kmalloc(rdwr_msg.len, GFP_KERNEL);
	if (rdwr_msg.buf == NULL) { 
		kfree(rdwr_msg.buf);
		return -ENOMEM;
	}
	
	if (copy_from_user(rdwr_msg.buf, data_ptrs, rdwr_msg.len)){
		kfree(rdwr_msg.buf);
//		kfree(data_ptrs);
		return -EFAULT;
	}

	res = at88x_transfer(&adap, &rdwr_msg);
	if (res >= 0 && (rdwr_msg.flags & AT88X_RD)) {
		if (copy_to_user(data_ptrs, rdwr_msg.buf, rdwr_msg.len))
			res = -EFAULT;
	}
	kfree(rdwr_msg.buf);
//	kfree(data_ptrs);
	return res;
}

static int at88x_ioctl(struct inode *inode, struct file *filp,
                 unsigned int cmd, unsigned long arg)
{
	switch(cmd) {
		case AT88X_RDWR:
			return at88x_ioctl_rdwr(arg);

//		case AT88X_RD:
//			return at88x_rd(arg);

		default:
			return -ENOTTY;
	}
	return 0;
}

int at88x_read_twobyte(char *buf, char **start, off_t offset,
                   int count, int *eof, void *data)
{
	//  printk("%s\n",__FUNCTION__);
	int ret;
	ssize_t len;
	unsigned int res;
	char *out = buf;
	struct at88x_algo adap = at88x_algo;
	struct at88x_msg at88x_test_msg = {
//	    addr = 0x5b,    
		.flags = 1,  
        .reg = {0x0, 0xa, 0x2},
        .len = 2,  
        .buf = (unsigned char *)kmalloc(2, GFP_KERNEL),
	};
	ret = at88x_transfer(&adap, &at88x_test_msg);
	
	if( ret > 0)
		res = 1; // pass
	else
		res = 0; // fail

    out += sprintf(out, "%d\n", res);
 
    len = out - buf - offset;
    if (len < count)
    {
	    *eof = 1;
        if (len <= 0)
        {
	        return 0;
        }
     }
     else
     {
	     len = count;
     }
	 *start = buf + offset;

     return len;
	//return 0;
}

struct file_operations at88x_fops = {
	.owner =    THIS_MODULE,
	.read =     at88x_read,
	.write =    at88x_write,
	.ioctl =    at88x_ioctl,
	.open =     at88x_open,
	.release =  at88x_release,
};

static struct miscdevice at88x_dev={
    .minor  = AT88X_MINOR,
    .name   = "at88x",
    .fops   = &at88x_fops,
};

int __init at88x_init(void)
{
    int result;
    result = misc_register(&at88x_dev);
    if(result)
    {
		printk("fail to regist dev\n");
	    return result;
	}

	create_proc_read_entry("at88x_test", 0, NULL, at88x_read_twobyte, NULL);
	return 0;
}	
 
void __exit at88x_exit(void)
{
	remove_proc_entry("at88x_test", NULL);
    misc_deregister(&at88x_dev);
}


#if 0
void at88x_exit(void)
{
	dev_t devno = MKDEV(at88x_major, 0);

	/* Get rid of our at88x dev entries */
	if (at88x_devp) {
		cdev_del(&at88x_devp->cdev);
		kfree(at88x_devp);
	}

#ifdef at88x_PROC
	at88x_remove_proc();
#endif

	/* cleanup_module is never called if registering failed */
	unregister_chrdev_region(devno, 1);
}
#endif

#if 0
int at88x_init(void)
{
	printk(KERN_ALERT "at88x_init for at88x access \n");
	int result, err;
	dev_t dev = 0;

	/*
	 * Get a range of minor numbers to work with, asking for a dynamic
	 * major unless directed otherwise at load time.
	 */
	if (at88x_major) {
		dev = MKDEV(at88x_major, 0);
		result = register_chrdev_region(dev, 1, "at88x");
	} else {
		result = alloc_chrdev_region(&dev, 0, 1, "at88x");
		at88x_major = MAJOR(dev);
	}
	if (result < 0) {
		printk(KERN_WARNING "at88x: can't get major %d\n", at88x_major);
		return result;
	}

	/* 
	 * allocate the devices -- we can't have them static, as the number
	 * can be specified at load time
	 */
	at88x_devp = kmalloc(sizeof(struct at88x_dev), GFP_KERNEL);
	if (!at88x_devp) {
	result = -ENOMEM;
	goto fail; 
	}
	memset(at88x_devp, 0, sizeof(struct at88x_dev));

    /* Initialize at88x device. */
	int devno = MKDEV(at88x_major, 0);
    
	cdev_init(&at88x_devp->cdev, &at88x_fops);
	at88x_devp->cdev.owner = THIS_MODULE;
	at88x_devp->cdev.ops = &at88x_fops;
	err = cdev_add (&at88x_devp->cdev, devno, 1);

	if (err)
		printk(KERN_NOTICE "Error %d adding at88x\n", err);

#ifdef at88x_PROC
	at88x_create_proc();
#endif

	return 0; /* succeed */

	fail:
	at88x_exit();
	return result;
}
#endif

MODULE_AUTHOR("Amos Chen");
MODULE_LICENSE("Dual BSD/GPL");

module_init(at88x_init);
module_exit(at88x_exit);
