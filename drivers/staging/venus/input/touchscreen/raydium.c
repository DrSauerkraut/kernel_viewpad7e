/* Touch Screen driver for Renesas Raydium Platform
 *
 * Copyright (C) 2011 Raydium, Inc.
 *
 * Based on migor_ts.c and synaptics_i2c_rmi.c
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU  General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */
 

#include <linux/input.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/timer.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/poll.h>
#include <asm/uaccess.h>
// cherry add
#include <linux/venus/raydium.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
//CHERRY ADD
#include <linux/delay.h>
#include <asm/io.h>
#include <mach/map.h>
#include <linux/venus/power_control.h>
#include <linux/earlysuspend.h>

/*
#define RM310XX_I2C_TS_NAME "raydium"
#define DRIVER_VERSION "v1.0"
#define DEVICE_NAME		"raydiumflash"


#define TS_MIN_X 0
#define TS_MAX_X 800
#define TS_MIN_Y 0
#define TS_MAX_Y 600
#define MAX_BUFFER_SIZE	144
*/
#define FLASH_ENABLE  1
#define DEVICE_NAME		"raydiumflash"
#define PROC_CMD_LEN 20

static int raydium_flash_major = 121;
static int raydium_flash_minor = 0;
static struct cdev raydium_flash_cdev;
static struct class *raydium_flash_class = NULL;
static dev_t raydium_flash_dev;

static int flag=0;

static struct workqueue_struct *raydium_wq;
static char fwversion;
static int calibration_state = -1;


#ifdef CONFIG_HAS_EARLYSUSPEND
struct early_suspend es;
static void raydium_early_suspend(struct early_suspend *h);
static void raydium_late_resume(struct early_suspend *h);
#endif	


struct raydium_ts_priv{
	uint16_t addr;
	struct i2c_client *client;
	struct input_dev *input_dev;
	int use_irq;
	struct hrtimer timer;
	struct work_struct work;
	int irq;
};
static struct raydium_ts_priv *priv;

//Johnson add start
struct raydium_ts_priv *proc_priv;
//Johnson add end



#if FLASH_ENABLE
struct raydium_flash_data {
	rwlock_t lock;
	unsigned char bufferIndex;
	unsigned int length;
	unsigned int buffer[MAX_BUFFER_SIZE];
	struct i2c_client *client;
};
struct raydium_flash_data *flash_priv;
#endif

/*static int 
RAYDIUM_i2c_transfer(
	struct i2c_client *client, struct i2c_msg *msgs, int cnt)
{
	int ret, count=5;
	while(count >= 0){
		count-= 1;
		ret = i2c_transfer(client->adapter, msgs, cnt);
                
                if(ret < 0){
                        udelay(500);
			continue;
                }
		break;
	}
	return ret;
}
*/
/*

static int 
RAYDIUM_i2c_read(
	struct i2c_client *client,
	uint8_t cmd, 
	uint8_t *data, 
	int length)
{
	int ret;
        struct i2c_msg msgs[] = {
		{.addr = client->addr, .flags = 0, .len = 1, .buf = &cmd,},
		{.addr = client->addr, .flags = I2C_M_RD, .len = length, .buf = data,}
        };

        ret = RAYDIUM_i2c_transfer(client, msgs, 2);

	return ret;
}
*/

#if FLASH_ENABLE
int raydium_flash_write(struct file *file, const char __user *buff, size_t count, loff_t *offp)
{
 struct i2c_msg msg[2];	
 char *str;
 uint8_t Wrbuf[2];	
 //struct raydium_flash_data *dev = file->private_data;
 int ret/*, i, ii*/;
 file->private_data = (uint8_t *)kmalloc(64, GFP_KERNEL);
 str = file->private_data;
 ret=copy_from_user(str, buff, count);

 if(str[0]==0x11&&str[1]==0x22)
	{
	 flag=1;
	 udelay(5*100);
	 printk("Start!\n");
	 return 1;
	}
 else if(str[0]==0x33&&str[1]==0x44)
	{
	 flag=0;
	 udelay(5*10);
	mdelay(5000);

	//calibration
	//write 0x03 to reg 0x78 start

	Wrbuf[0]=0x78;
	Wrbuf[1]=0x03;
	i2c_master_send(flash_priv->client,Wrbuf,2);
	mdelay(3000);
	//write 0x03 to reg 0x78 end

	//set mode
	//change mode  to 00 start
	Wrbuf[0]=0x71;
	Wrbuf[1]=0x08;
	i2c_master_send(flash_priv->client,Wrbuf,2);
	//change mode  to 00 end
	mdelay(50);
	
	printk("End!\n");
	return 1;
	}
 
//for(i=0;i<5;i++)
	{
	udelay(100);
	}
 
	msg[0].addr = flash_priv->client->addr;
	msg[0].flags = 0;
	msg[0].len = str[0];
	msg[0].buf = &str[1];
	
	i2c_transfer(flash_priv->client->adapter, msg, 1);

// for(i=0;i<5;i++)
	{
	udelay(str[str[0]+1]);
	}

/* printk("Write:");
 printk("lenth=%d, ", str[0]);
 for(ii=0;ii<str[0];ii++)
	 printk("data=%d, ", str[ii+1]);
 printk("\n");
*/
 return 1;
}

int raydium_flash_read(struct file *file, char __user *buff, size_t count, loff_t *offp)
{
 struct i2c_msg msg[2];	 
 char *str;
 //struct raydium_flash_data *dev = file->private_data;
 int i, ret/*, ii*/;
 file->private_data = (uint8_t *)kmalloc(64, GFP_KERNEL);
 str = file->private_data;
 if(copy_from_user(str, buff, count))
	return -EFAULT;

 for(i=0;i<5;i++)
	{
	udelay(100);
	}
/*
	for(ii=0;ii<32;ii++)
		printk("%d, ", str[ii]);
	printk("\n");	
*/

	msg[0].addr = flash_priv->client->addr;
	msg[0].flags = 0;
	msg[0].len = str[0];
	msg[0].buf = &str[1];
//	start_reg = 0x10;

	msg[1].addr = flash_priv->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = str[str[0]+1];
	msg[1].buf = &str[str[0]+2];

	i2c_transfer(flash_priv->client->adapter, msg, 2);

 	ret=copy_to_user(buff, str, count);
/*	for(ii=0;ii<str[0]+2+str[str[0]+1];ii++)
		printk("%c, ", str[ii]);
	printk("\n");	*/
// for(i=0;i<5;i++)
	{
	udelay(str[str[0]+str[2]+2]);
	}

/* printk("Read:");
 printk("length=%d, ", str[0]+str[2]+2);
 for(ii=0;ii<str[0]+str[2]+2;ii++)
	 printk("data=%d, ", buff[ii]);
 printk("\n");
*/

 return 1;
}


int raydium_flash_open(struct inode *inode, struct file *file)
{
	int i;
	struct raydium_flash_data *dev;

	dev = kmalloc(sizeof(struct raydium_flash_data), GFP_KERNEL);
	if (dev == NULL) {
		return -ENOMEM;
	}

	/* initialize members */
	rwlock_init(&dev->lock);
	for(i = 0; i < MAX_BUFFER_SIZE; i++) {
		dev->buffer[i] = 0xFF;
	}
//	dev->client = flash_priv->client;
//	printk("%d\n", dev->client->addr);
	file->private_data = dev;


	return 0;   /* success */
}

int raydium_flash_close(struct inode *inode, struct file *file)
{
	struct raydium_flash_data *dev = file->private_data;

	if (dev) {
		kfree(dev);
	}
	return 0;   /* success */
}

struct file_operations raydium_flash_fops = {
	.owner = THIS_MODULE,
	.open = raydium_flash_open,
	.release = raydium_flash_close,
//	.ioctl = raydium_flash_ioctl,
	.write = raydium_flash_write,
	.read = raydium_flash_read,
};

static int raydium_flash_init(struct raydium_ts_priv *priv)
{		
	dev_t dev = MKDEV(raydium_flash_major, 0);
	int alloc_ret = 0;
	int major;
	int cdev_err = 0;
	int input_err = 0;
	struct device *class_dev = NULL;

	// dynamic allocate driver handle
	alloc_ret = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
	if(alloc_ret) {
		goto error;
	}
	major = MAJOR(dev);
	raydium_flash_major = major;

	cdev_init(&raydium_flash_cdev, &raydium_flash_fops);
	raydium_flash_cdev.owner = THIS_MODULE;
	raydium_flash_cdev.ops = &raydium_flash_fops;
	cdev_err = cdev_add(&raydium_flash_cdev, MKDEV(raydium_flash_major, raydium_flash_minor), 1);
	if(cdev_err) {
		goto error;
	}

	// register class
	raydium_flash_class = class_create(THIS_MODULE, DEVICE_NAME);

	if(IS_ERR(raydium_flash_class)) {
		goto error;
	}
	raydium_flash_dev = MKDEV(raydium_flash_major, raydium_flash_minor);
	class_dev = device_create(raydium_flash_class, NULL, raydium_flash_dev, NULL, DEVICE_NAME);


/*
	input_dev = input_allocate_device();

	if (!input_dev) {
		input_err = -ENOMEM;
		goto error;
	}
*/
//	input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);

//	input_dev->name = DEVICE_NAME;

//	input_err = input_register_device(input_dev);



	printk("============================================================\n");
	printk("raydium_flash driver loaded\n");
	printk("============================================================\n");	
	flash_priv=kzalloc(sizeof(*flash_priv),GFP_KERNEL);	
	if (priv == NULL) {
		//dev_set_drvdata(&client->dev, NULL);
                input_err = -ENOMEM;
                goto error;
	}
	flash_priv->client = priv->client;
	return 0;

error:
	if(cdev_err == 0) {
		cdev_del(&raydium_flash_cdev);
	}

	if(alloc_ret == 0) {
		unregister_chrdev_region(dev, 1);
	}

	if(input_err != 0)
	{
	printk("flash_priv error!\n");
	}

	return -1;
}

static void raydium_flash_exit(void)
{
	dev_t dev = MKDEV(raydium_flash_major, raydium_flash_minor);

	// unregister class
	device_destroy(raydium_flash_class, raydium_flash_dev);
	class_destroy(raydium_flash_class);

	// unregister driver handle
	cdev_del(&raydium_flash_cdev);
	unregister_chrdev_region(dev, 1);

	printk("============================================================\n");
	printk("raydium_flash driver unloaded\n");
	printk("============================================================\n");
}
#endif


/*
static enum hrtimer_restart raydium_ts_timer_func(struct hrtimer *timer)
{
	

	struct raydium_ts_priv *priv = container_of(timer, struct raydium_ts_priv, timer);
		queue_work(raydium_wq, &priv->work);
		hrtimer_start(&priv->timer, ktime_set(0, 15000000), HRTIMER_MODE_REL);
		return HRTIMER_NORESTART;

}
*/

static void raydium_ts_poscheck(/*void* unused*/struct work_struct *work)
{
	struct raydium_ts_priv *priv = container_of(work,struct raydium_ts_priv,
							work);
  
	unsigned char touching;
	struct i2c_msg msg[2];  
	  
	int posx[10];
	int posy[10];
	uint8_t start_reg;
	uint8_t Rdbuf[50];
	int ret;
	//int z=5;
	//int w=10;
	int i;
  
	if(flag==1)
		return;

//	mdelay(30);
/*
	msg[0].addr = priv->client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &start_reg;
	start_reg = 0x10;
	msg[1].addr = priv->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = Wrbuf;
	//memset(Wrbuf, 0, sizeof(Wrbuf));
	//memset(Rdbuf, 0, sizeof(Rdbuf));  
	  
  
	ret = i2c_transfer(priv->client->adapter, msg, 2);
  //printk(KERN_INFO "raydium_ts_poscheck is working2\n");
	touching=Wrbuf[0]&0x0F;
*/
  //printk(KERN_INFO "raydium_ts_poscheck is working2\n");

	msg[0].addr = priv->client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &start_reg;
	start_reg = 0x00;
	msg[1].addr = priv->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 8;
	msg[1].buf = Rdbuf;
	ret = i2c_transfer(priv->client->adapter, msg, 2);

	//ret = GPIO_Read(0x11, touching*5, Rdbuf);

	touching=0;
	for(i=0;i<2;i++)
	{
		posx[i] = (((Rdbuf[1+4*i]&0x03) << 8) | Rdbuf[0+4*i]);
		posy[i] = (((Rdbuf[3+4*i]&0x03) << 8) | Rdbuf[2+4*i]);
		//printk("======pos[%d](%d,%d)\n",i,posx[i],posy[i]);
		if(posx[i]!=0||posy[i]!=0)
			touching++;
			
			posx[i] = TS_MAX_X - posx[i];
			posy[i] = TS_MAX_Y - posy[i];		
		//printk("======pos[%d](%d,%d)\n",i,posx[i],posy[i]);
	}

	if(!(touching)){
//		printk("### touching %d Wrbuf[0] %d \n", touching, Wrbuf[0]);
		//input_report_key(priv->input_dev, BTN_TOUCH, 0);
		//input_report_key(priv->input_dev, BTN_2, 0);
		//input_report_key(priv->input_dev, ABS_MT_TRACKING_ID, 0);
		input_report_abs(priv->input_dev, ABS_MT_TOUCH_MAJOR, 0);
		//input_report_abs(priv->input_dev, ABS_MT_WIDTH_MAJOR, 0);
		//input_report_abs(priv->input_dev, ABS_PRESSURE, 0);
		//input_report_abs(priv->input_dev, ABS_MT_POSITION_X, 0);
		//input_report_abs(priv->input_dev, ABS_MT_POSITION_Y, 0);
		//input_report_key(priv->input_dev, ABS_MT_TRACKING_ID, 1);
		//input_report_abs(priv->input_dev, ABS_MT_TOUCH_MAJOR, 0);
		//input_report_abs(priv->input_dev, ABS_MT_WIDTH_MAJOR, 0);
		//input_report_abs(priv->input_dev, ABS_MT_POSITION_X, 0);
		//input_report_abs(priv->input_dev, ABS_MT_POSITION_Y, 0);
		  
		input_mt_sync(priv->input_dev);
		input_sync(priv->input_dev);
		goto out;
	}
/*  
	udelay(1);
	msg[0].addr = priv->client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &start_reg;
	start_reg = 0x11;
	msg[1].addr = priv->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = touching*5;
	msg[1].buf = Rdbuf;
	ret = i2c_transfer(priv->client->adapter, msg, 2);
	for(i=0;i<touching;i++)
	{
		//cherry todo
		//posx[i] = (((Rdbuf[0+5*i]&0x07) << 8) | Rdbuf[1+5*i]);
		posx[i] = ((Rdbuf[0+5*i] << 8) | Rdbuf[1+5*i]);
		posy[i] = ((Rdbuf[2+5*i] << 8) | Rdbuf[3+5*i]);
		//pid[i]= (Rdbuf[0+5*i]&0xF0)>>3;
		printk("raydium_touch touching %d posx[%d]:%d,posy[%d]:%d \n",touching,i,posx[i],i,posy[i]);	//cherry add 2011-06-08
	}
*/	
	for(i=0;i<touching;i++)
	{
		input_report_abs(priv->input_dev, ABS_MT_TOUCH_MAJOR, 128);
		input_report_abs(priv->input_dev, ABS_MT_POSITION_X, posx[i]);
		input_report_abs(priv->input_dev, ABS_MT_POSITION_Y, posy[i]);
		//input_report_abs(priv->input_dev, ABS_MT_TOUCH_MAJOR, z);
		//input_report_abs(priv->input_dev, ABS_MT_TOUCH_MAJOR, z*(i+1));
		//input_report_abs(priv->input_dev, ABS_MT_WIDTH_MAJOR, w);
		//input_report_abs(priv->input_dev, ABS_MT_WIDTH_MAJOR, w*(i+1));
		input_mt_sync(priv->input_dev);
	}   
	input_sync(priv->input_dev);
	//udelay((touching-1)*5); 

    out:
		enable_irq(priv->client->irq);
}

static irqreturn_t raydium_ts_isr(int irq,void *dev_id)
{
	//printk("=========%s========\n",__FUNCTION__);
	struct raydium_ts_priv *priv=dev_id;
	disable_irq_nosync(irq);
    //disable_irq(priv->client->irq);
    //hrtimer_start(&priv->timer, ktime_set(0, 0), HRTIMER_MODE_REL);
	queue_work(raydium_wq, &priv->work);
	return IRQ_HANDLED;
}


static int raydium_ts_open(struct input_dev *dev)
{
	// printk(KERN_INFO"raydium_touch %s,%d\n",__func__,__LINE__);
	return 0;
}

static void raydium_ts_close(struct input_dev *dev)
{
	// printk(KERN_INFO"raydium_touch %s,%d\n",__func__,__LINE__);
}

static int Raydium_proc_init(void);
//static int Raydium_fwinfo(void);
void raydium_gpio_init(void);

static int raydium_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{	 
    
//	struct raydium_ts_priv *priv;
	struct i2c_msg msg[2];	//tmp for i2c read
	uint8_t start_reg;
	uint8_t Rdbuf[50],Wrbuf[2];	
    	int ret = 0;
	printk(KERN_INFO"raydium_touch %s,%d\n",__func__,__LINE__);	
	raydium_gpio_init();
	request_power(VDD_TOUCH_POWER);
	mdelay(100);
	gpio_direction_output(S5PV210_GPJ3(4),1);

	priv=kzalloc(sizeof(*priv),GFP_KERNEL);
	if (priv == NULL) {
		dev_set_drvdata(&client->dev, NULL);
                ret = -ENOMEM;
                goto err_alloc_data_failed;
    }
    INIT_WORK(&priv->work,raydium_ts_poscheck);
	priv->client = client;
	i2c_set_clientdata(client, priv);
	//Johnson add start
    proc_priv=priv;
	//Johnson add end
    //cherry todo	
    //priv->use_irq=0;
	dev_set_drvdata(&client->dev, priv);
	priv->input_dev = input_allocate_device();
	if(priv->input_dev == NULL){
                ret = -ENOMEM;
                printk(KERN_ERR "raydium_ts_probe: Failed to allocate input device\n");
                goto err_input_dev_alloc_failed;
	}

	set_bit(EV_SYN, priv->input_dev->evbit);
	set_bit(EV_KEY, priv->input_dev->evbit);
	set_bit(BTN_TOUCH, priv->input_dev->keybit);
	set_bit(BTN_2, priv->input_dev->keybit);
//	set_bit(ABS_MT_TRACKING_ID, priv->input_dev->keybit);
	set_bit(EV_ABS, priv->input_dev->evbit);
	input_set_abs_params(priv->input_dev, ABS_X, TS_MIN_X, TS_MAX_X, 0, 0);
	input_set_abs_params(priv->input_dev, ABS_Y, TS_MIN_Y, TS_MAX_Y, 0, 0);
	input_set_abs_params(priv->input_dev, ABS_HAT0X, TS_MIN_X, TS_MAX_X, 0, 0);
	input_set_abs_params(priv->input_dev, ABS_HAT0Y, TS_MIN_Y, TS_MAX_Y, 0, 0);
	input_set_abs_params(priv->input_dev, ABS_MT_POSITION_X, TS_MIN_X, TS_MAX_X, 0, 0);
	input_set_abs_params(priv->input_dev, ABS_MT_POSITION_Y, TS_MIN_Y, TS_MAX_Y, 0, 0);
	input_set_abs_params(priv->input_dev, ABS_MT_TOUCH_MAJOR, 0, 250, 0, 0);
	input_set_abs_params(priv->input_dev, ABS_MT_WIDTH_MAJOR, 0, 50, 0, 0);
	//input_set_abs_params(input_dev, ABS_PRESSURE, 0, 10, 0, 0);

	priv->input_dev->name = "raydium-touchscreen";
	priv->input_dev->id.bustype = BUS_I2C;
	priv->input_dev->dev.parent = &client->dev;

	priv->input_dev->open = raydium_ts_open;
	priv->input_dev->close = raydium_ts_close;

	
	/*setup_timer(&timer, raydium_ts_poscheck, NULL);
	timer.expires = jiffies +HZ * 30;
	add_timer(&timer);*/
	ret = input_register_device(priv->input_dev);
	priv->irq = client->irq;

    if (ret) {
    	printk(KERN_ERR "raydium_ts_probe: Unable to register %s input device\n", priv->input_dev->name);
        goto err_input_register_device_failed;
    } 
    if (client->irq) {
//		ret = request_irq(client->irq, raydium_ts_isr, IRQF_TRIGGER_LOW, client->name, priv);
		ret = request_irq(client->irq, raydium_ts_isr, IRQF_TRIGGER_FALLING, client->name, priv);
        //cherry add 0609
		/*
		if (ret != 0) {
				free_irq(client->irq, priv);
		}
		if (ret == 0)
			priv->use_irq = 1;
		else
			dev_err(&client->dev, "request_irq failed\n");
		*/
	}
	

	//device_init_wakeup(&client->dev, 1);
	printk(KERN_INFO "raydium_ts_probe: Start touchscreen %s is success\n", priv->input_dev->name);

	//cherry todo
//	if (!priv->use_irq) 
//	{
//		hrtimer_init(&priv->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
//		priv->timer.function = raydium_ts_timer_func;
//		if(!client->irq)
//			hrtimer_start(&priv->timer, ktime_set(10, 0), HRTIMER_MODE_REL);
//	}
#ifdef CONFIG_HAS_EARLYSUSPEND
	es.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	es.suspend = raydium_early_suspend;
	es.resume = raydium_late_resume;
	register_early_suspend(&es);
#endif

#if FLASH_ENABLE
	raydium_flash_init(priv);
#endif
/*

	Wrbuf[0]=0x71;
	Wrbuf[1]=0x0a;
	
	i2c_master_send(priv->client,Wrbuf,2);
*/
//change mode  to 00 start

	Wrbuf[0]=0x71;
	Wrbuf[1]=0x08;
	i2c_master_send(priv->client,Wrbuf,2);
//change mode  to 00 end

//write 0x03 to reg 0x78 start
//	Wrbuf[0]=0x78;
//	Wrbuf[1]=0x03;
//	i2c_master_send(priv->client,Wrbuf,2);
//	mdelay(3000);
//write 0x03 to reg 0x78 end	

//read reg 0x78 start
	msg[0].addr = priv->client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &start_reg;
	start_reg = 0x78;
	msg[1].addr = priv->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = Rdbuf;
	ret = i2c_transfer(priv->client->adapter, msg, 2);
	printk("raydium_touch reg 0x78=%x\n",Rdbuf[0]);
//read reg 0x78 end		
	
//read reg 0x71 start
	msg[0].addr = priv->client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &start_reg;
	start_reg = 0x71;
	msg[1].addr = priv->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = Rdbuf;
	ret = i2c_transfer(priv->client->adapter, msg, 2);
	printk("raydium_touch reg 0x71=%x\n",Rdbuf[0]);
//read reg 0x71 end 

//read FW ver. start
	msg[0].addr = priv->client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &start_reg;
	start_reg = 0x77;
	msg[1].addr = priv->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = Rdbuf;
	i2c_transfer(priv->client->adapter, msg, 2);
	fwversion = Rdbuf[0] & 0x3f;
	printk("raydium_touch FW version = 0x%x\n",fwversion);
//read FW ver. end
	Raydium_proc_init();
	return 0;
	
err_input_register_device_failed:
        input_free_device(priv->input_dev);	
err_input_dev_alloc_failed:        
        kfree(priv);
err_alloc_data_failed:
return ret;
}

static int raydium_ts_remove(struct i2c_client *client)
{
	
	struct raydium_ts_priv *priv = i2c_get_clientdata(client);
	free_irq(priv->irq, priv);
	
	printk(KERN_INFO"raydium_touch %s,%d\n",__func__,__LINE__);
//	hrtimer_cancel(&priv->timer);
	input_unregister_device(priv->input_dev);
#ifdef CONFIG_HAS_EARLYSUSPEND
		unregister_early_suspend(&es);
#endif
#if FLASH_ENABLE
	raydium_flash_exit();
#endif
	kfree(priv);
	//dev_set_drvdata(&client->dev, NULL);
	gpio_direction_output(S5PV210_GPJ3(4),0);
	mdelay(2);
	release_power(VDD_TOUCH_POWER);
 	return 0;
}
/*********************************Proc-filesystem-TOP**************************************/

unsigned int Raydium_get_version(char *buf)
{
	int ret;	
	ret = sprintf(buf, "0x%x\n", fwversion);
	return ret;
}


unsigned int Raydium_set_version(unsigned int version)
{
//	printk("%s:Blank function\n", __FUNCTION__);
	return 0;
}

/***************************Johnson add start*******************/
unsigned int raydium_do_calibration(unsigned int cmd)
{
	unsigned char Wrbuf[2] = {0};
    int ret = 0;

	if (0 == cmd)
		goto out;

	Wrbuf[0] = 0x78;
	Wrbuf[1] = 0x03;

	ret = i2c_master_send(proc_priv->client, Wrbuf, sizeof(Wrbuf));
//    printk("Johnson:ret=%d\n",ret);
	if (ret != sizeof(Wrbuf))
	{
		printk("%s\n", __func__);
		dev_err(&proc_priv->client->dev, "Unable to write to i2c touchscreen!\n");
		goto out;
	}
	printk("Cap Touch screen calibration...\n");
	printk("DO NOT touch the screen in 3 seconds!!!\n");
	mdelay(3000);
	
	return 0;
out:
	return -1;
}


unsigned int raydium_get_calibration(char *buf)
{
	int ret;
	if(calibration_state==0)
		ret = sprintf(buf, "Calibration OK!\n");
	else 
		ret = sprintf(buf, "Calibration ERROR!\n");
	return ret;
}

unsigned int raydium_set_calibration(unsigned int cmd)
{
	int err=0;
	err=raydium_do_calibration(cmd);
	calibration_state = err;
	return err;
}


/***************************Johnson add end*******************/



struct Raydium_proc {
	char *module_name;
	unsigned int (*Raydium_get) (char *);
	unsigned int (*Raydium_set) (unsigned int);
};

static const struct Raydium_proc Raydium_modules[] = {
	{"version", Raydium_get_version, Raydium_set_version},
	{"Raydium_calibration", raydium_get_calibration, raydium_set_calibration},
};


ssize_t Raydium_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
//	printk("============%s,%d\n",__FUNCTION__,__LINE__);
	ssize_t len;
	char *p = page;
	const struct Raydium_proc *proc = data;

	p += proc->Raydium_get(p);

	len = p - page;
	
	*eof = 1;
	
	//printk("len = %d\n", len);
	
	return len;
}

size_t Raydium_proc_write(struct file * filp, const char __user * buf, unsigned long len, void *data)
{
//	printk("============%s,%d\n",__FUNCTION__,__LINE__);
	char command[PROC_CMD_LEN];
	const struct Raydium_proc *proc = data;

	if (!buf || len > PAGE_SIZE - 1)
		return -EINVAL;

	if (len > PROC_CMD_LEN) {
		printk("Command to long\n");
		return -ENOSPC;
	}
	if (copy_from_user(command, buf, len)) {
		return -EFAULT;
	}

	if (len < 1)
		return -EINVAL;

	if (strnicmp(command, "on", 2) == 0 || strnicmp(command, "1", 1) == 0) {
		proc->Raydium_set(1);
	} else if (strnicmp(command, "off", 3) == 0 || strnicmp(command, "0", 1) == 0) {
		proc->Raydium_set(0);
	} else {
		return -EINVAL;
	}

	return len;
}

//proc file_operations
struct file_operations Raydium_fops = {
	.owner = THIS_MODULE,
};

static int proc_node_num = (sizeof(Raydium_modules) / sizeof(*Raydium_modules));
static struct proc_dir_entry *update_proc_root;
static struct proc_dir_entry *proc[5];

static int Raydium_proc_init(void)
{
//	printk("============%s,%d\n",__FUNCTION__,__LINE__);
	unsigned int i;
	//struct proc_dir_entry *proc;

	update_proc_root = proc_mkdir("touch", NULL);

	if (!update_proc_root) 
	{
		printk("create Raydium_touch directory failed\n");
		return -1;
	}

	for (i = 0; i < proc_node_num; i++) 
	{
		mode_t mode;

		mode = 0;
		if (Raydium_modules[i].Raydium_get)
		{
			mode |= S_IRUGO;
		}
		if (Raydium_modules[i].Raydium_set)
		{
			mode |= S_IWUGO;
		}

		proc[i] = create_proc_entry(Raydium_modules[i].module_name, mode, update_proc_root);
		if (proc[i]) 
		{
			proc[i]->data = (void *)(&Raydium_modules[i]);
			proc[i]->read_proc = (read_proc_t *)Raydium_proc_read;
			proc[i]->write_proc =(write_proc_t *)Raydium_proc_write;
		}
	}

	return 0;
}

/*static void Raydium_proc_remove(void)
{
//	printk("============%s,%d\n",__FUNCTION__,__LINE__);
	remove_proc_entry("version", update_proc_root);
	remove_proc_entry("raydium_calibration", update_proc_root);

	remove_proc_entry("touch", NULL);
}
*/
/*********************************Proc-filesystem-BOTTOM**************************************/
#ifdef CONFIG_HAS_EARLYSUSPEND
void raydium_gpio_init(void)
{
	int ret;
	ret = gpio_request(S5PV210_GPJ3(4),"TOUCH_RESET");
//	gpio_direction_output(S5PV210_GPJ3(4),1);
 //	mdelay(15);
//	gpio_direction_output(S5PV210_GPJ3(4),0);
}

static void raydium_early_suspend(struct early_suspend *h)
{	
	int ret=0;
	// printk(KERN_INFO"raydium_touch %s,%d\n",__func__,__LINE__);
	//struct raydium_ts_priv *priv = i2c_get_clientdata(client);
	disable_irq(priv->irq);
	ret = cancel_work_sync(&priv->work);
	if (ret == 1)
	{
		enable_irq(priv->irq);
	}
	gpio_direction_output(S5PV210_GPJ3(4),0);
	mdelay(2);
	release_power(VDD_TOUCH_POWER);
	// printk("Raydium touch early suspend done!\n");
}


static void raydium_late_resume(struct early_suspend *h)
{
   	// printk(KERN_INFO"raydium_touch %s,%d\n",__func__,__LINE__);
	uint8_t Wrbuf[2];
	int ret;

        request_power(VDD_TOUCH_POWER);
	mdelay(100);
//	raydium_reset();
	gpio_direction_output(S5PV210_GPJ3(4),1);
	//change mode  to 00 start
	Wrbuf[0]=0x71;
	Wrbuf[1]=0x08;
	ret = i2c_master_send(priv->client,Wrbuf,2);
    //change mode  to 00 end
   	enable_irq(priv->irq);
	// printk("Raydium touch late resume done!\n");
}
#endif


#if 0
static int raydium_ts_suspend(struct i2c_client *client, pm_message_t mesg)

{
	printk(KERN_INFO"raydium_touch %s,%d\n",__func__,__LINE__);

	struct raydium_ts_priv *priv = i2c_get_clientdata(client);


		//hrtimer_cancel(&priv->timer);
	cancel_work_sync(&priv->work);
   
	disable_irq(client->irq);
	release_power(VDD_TOUCH_POWER);
	return 0;
}

static int raydium_ts_resume(struct i2c_client *client)
{
	printk(KERN_INFO"raydium_touch %s,%d\n",__func__,__LINE__);
	struct raydium_ts_priv *priv = i2c_get_clientdata(client);
    request_power(VDD_TOUCH_POWER);
	mdelay(100);
	uint8_t Rdbuf[50],Wrbuf[2];
	//change mode  to 00 start
	Wrbuf[0]=0x71;
	Wrbuf[1]=0x08;
	i2c_master_send(priv->client,Wrbuf,2);
   //change mode  to 00 end
	enable_irq(client->irq);

	//hrtimer_start(&priv->timer, ktime_set(1, 0), HRTIMER_MODE_REL);

	return 0;
}
#endif

static const struct i2c_device_id raydium_ts_id[] = {
	{ RM310XX_I2C_TS_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, raydium_ts_id);

static struct i2c_driver raydium_ts_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "raydium",
	 },
	.probe = raydium_ts_probe,
	.remove = raydium_ts_remove,
//	.suspend = raydium_ts_suspend,
//	.resume = raydium_ts_resume,
	.id_table = raydium_ts_id,
};



static int __init raydium_ts_init(void)
{        	
//	u_int8_t Rdbuf[1];
	raydium_wq = create_singlethread_workqueue("raydium_wq");
//	Rdbuf[0]=0;
	if(!raydium_wq)
		return -ENOMEM;
	return i2c_add_driver(&raydium_ts_driver);
}

static void __exit raydium_ts_exit(void)
{
	i2c_del_driver(&raydium_ts_driver);
	if (raydium_wq)
		destroy_workqueue(raydium_wq);
}

MODULE_DESCRIPTION("Raydium Touchscreen Driver");
MODULE_AUTHOR("Cherry<http://www.rad-ic.com>");
MODULE_LICENSE("GPL");

module_init(raydium_ts_init);
module_exit(raydium_ts_exit);
