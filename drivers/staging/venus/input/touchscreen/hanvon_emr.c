/*
 * Wacom serial touchscreen driver
 *
 * Copyright (c) Dmitry Torokhov <dtor@mail.ru>
 */

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */
 
#include <asm/uaccess.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/serio.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <plat/gpio-cfg.h>

#include <linux/delay.h>
#include <mach/gpio.h>

#include <linux/io.h>
#include <linux/irq.h>
#include <linux/venus/power_control.h>
#include <linux/earlysuspend.h>
static int stop_flag = 0;    //for stop mode touch enter idle

#define SERIO_WACOM	0x3a
#define DRIVER_DESC	"wacom serial touchscreen driver"

MODULE_AUTHOR("Victor Liu <liuduogc@hotmail.com>");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

#define WACOM_LENGTH  7 
//#define FIRMWARE_DOWNLOAD

#define PROC_CMD_LEN  20

#define EMR_OFF 0
#define EMR_ON  1

#define POWER_OFF    0
#define POWER_ON     1

#define NO_SUSPEND_CMD     0
#define HAVE_SUSPEND_CMD   1

static unsigned char power_state = POWER_ON;
static unsigned char emr_state = EMR_ON;
static unsigned char suspend_cmd_state = NO_SUSPEND_CMD;
static unsigned char suspend_cmd_store;

static int ts_lock = 0;                 //for touch lock
static int version_flag = 1;            //for firmware version
static unsigned char version[7]={0};    //for firmware version
struct wacom *serio_dev;

struct wacom {
	struct input_dev *dev;
	struct serio *serio;
	int idx;
	unsigned char data[WACOM_LENGTH];
	char phys[32];
};
enum touch_state{
	touch_EFFECTIVE=0x80,
	touch_PRESS_DOWN=0xb0,
	touch_PRESS_UP=0xe0,
	touch_ACTIVE=0xa0
};

struct input_dev *input_dev;

#define epd_wide 	828
#define epd_heigh 1200
#define ts_wide 	6144
//#define ts_wide 	6000
#define tw_heigh 	8192 * 2

void hanwang_touch_power_off(void);
void hanwang_touch_reset(void);
unsigned int hanwang_chip_enable_disable(unsigned int cmd);

static ssize_t ts_lock_show(struct device_driver *ddp, char *buf)
{	
//	printk("[Hanwang Trace] Enter %s.\n", __FUNCTION__);	

	return sprintf(buf, "%d\n", ts_lock);
}

static ssize_t ts_lock_store(struct device_driver *ddp, const char * buf, size_t count)
{	
	int v;
//	printk("[Hanwang Trace] store %s\n", __FUNCTION__);

	v = (*buf) - '0';
	
	if (v == 0)
	{
		ts_lock = v;
	}else if (v == 1)
	{
		ts_lock = v;
	}

	return count;
}
static DRIVER_ATTR(tslock, /*S_IRUGO | S_IWUSR*/0666, ts_lock_show, ts_lock_store);

static ssize_t ts_version_show(struct device_driver *ddp, char *buf)
{	
    u32 count = 0;

    if (version[0] == 0)
    {
        version_flag = -1;
        serio_write(serio_dev->serio, 0x5F);
        while(version[0] == 0)
        {
	        msleep(1);
	        count++;
            if (count % 100 == 0)
                serio_write(serio_dev->serio, 0x5F);
            if (500 == count)
		        break;	  
	    }
    }
    
	return sprintf(buf, "%s\n", version);
}
static DRIVER_ATTR(version, S_IRUGO, ts_version_show, NULL);


#ifdef FIRMWARE_DOWNLOAD
static ssize_t han_download_show(struct device_driver *ddp, char *buf)
{
	printk("[Hanwang Trace] Enter %s.\n", __FUNCTION__);
	return 0;
}

static ssize_t han_download_store(struct device_driver *ddp, const char * buf, size_t count)
{
	int value = (*buf) - '0';
	printk("[Hanwang Trace] Enter %s value is %d\n", __FUNCTION__, value);

	if (value == 1)
	{
		//Touch_EN GPM0 -> OUTPUT low
   	 	gpio_set_value(S3C64XX_GPK(15),  0);
		s3c_gpio_cfgpin(S3C64XX_GPK(15),  S3C64XX_GPM_OUTPUT(15));
		
		//Touch_Pro GPN9 -> OUTPUT  	
		//Touch_Pro GPN9 -> HIGH
		gpio_set_value(S3C64XX_GPO(6), 1);
		s3c_gpio_cfgpin(S3C64XX_GPO(6), S3C64XX_GPO_OUTPUT(6));
		mdelay(1);
        
		//PLT_RST GPI1 -> OUTPUT high
		gpio_set_value(S3C64XX_GPI(1),	1);
		s3c_gpio_cfgpin(S3C64XX_GPI(1),  S3C64XX_GPI_OUTPUT(1));	//GPI1
		mdelay(1);

		//PLT_RST GPI1 -> OUTPUT low
		gpio_set_value(S3C64XX_GPI(1),	0);
		s3c_gpio_cfgpin(S3C64XX_GPI(1),  S3C64XX_GPI_OUTPUT(1));	//GPI1
		mdelay(1);

		//PLT_RST GPI1 -> OUTPUT high
		gpio_set_value(S3C64XX_GPI(1),	1);
		s3c_gpio_cfgpin(S3C64XX_GPI(1),  S3C64XX_GPI_OUTPUT(1));	
	}

	return count;
}
static DRIVER_ATTR(download, /*S_IRUGO | S_IWUSR*/0666, han_download_show, han_download_store);
#endif

static void emr_touch_early_suspend(struct early_suspend *h)
{
	printk("%s\n", __func__);
	
/*	if (serio_dev)
	{
    	struct serio *serio = serio_dev->serio;
        unsigned int count = 0;

    	printk("[Hanwang Trace] Enter %s\n", __FUNCTION__);
        //if (gpio_get_value(S3C64XX_GPN(4)) == 0)
        if (gpio_get_value(S5PV210_GPJ0(1)) == 0)
        {
            stop_flag = 1;
        	serio_write(serio, 0x4D);
            while (1 == stop_flag)
            {
                mdelay(10);
                count++;
                if (count%5 == 0)
                    serio_write(serio, 0x4D);
                if (50 == count)
                    break;
            }
        }
	}*/
	
	release_power(VDD_TOUCH_POWER);
	
	power_state = POWER_OFF;
	
	return ;
}

static void emr_touch_late_resume(struct early_suspend *h)
{	
	printk("%s\n", __func__);
	
	request_power(VDD_TOUCH_POWER);
	
	power_state = POWER_ON;	
	
	if (HAVE_SUSPEND_CMD == suspend_cmd_state)
	{
		hanwang_chip_enable_disable(suspend_cmd_store);
		printk("Handled EMR suspend command.\n");
		goto _out;
	}

	if (emr_state == EMR_ON)
	{
		hanwang_touch_reset();
	}
	
_out:
	return;
}

static struct early_suspend emr_touch_early_suspend_desc = {
        .level = EARLY_SUSPEND_LEVEL_STOP_DRAWING,
        .suspend = emr_touch_early_suspend,
        .resume = emr_touch_late_resume,
};

static irqreturn_t wacom_interrupt(struct serio *serio,
				     unsigned char data, unsigned int flags)
{
	struct wacom *wacom = serio_get_drvdata(serio);
	struct input_dev *dev = wacom->dev;
	unsigned int abs_x,abs_y,abs_pressure;
	unsigned int touch_button;
	static int pen_up_flag = 0;

#ifdef TOUCH_LOCK		
	if (ts_lock == 1)
		return IRQ_HANDLED;
#endif

    if (1 == stop_flag)
    {
        if (0x4D == data){
            printk("touch Enter idle mode, data=%d\n", data);
            stop_flag=0;
            
            return IRQ_HANDLED;
        }
    }

	if (wacom->idx == 0) 
	{
		/* resync skip until start of frame */
		if ((data & 0x80) != touch_EFFECTIVE)
		{	
			return IRQ_HANDLED;
		}
	} 
	else 
	{
		/* resync skip garbage */
		if ((data & 0x80)==touch_EFFECTIVE) 
		{
			wacom->idx = 0;
		}
	}

	wacom->data[wacom->idx++] = data;

	if (wacom->idx == WACOM_LENGTH) {	
        if (version_flag == -1 && wacom->data[0] == 128)    //for touch vesion number
        {
            version_flag = 1;
            sprintf(version, "%02x%02x%02x%02x%02x%02x%02x", wacom->data[0], wacom->data[1],
                wacom->data[2], wacom->data[3], wacom->data[4], wacom->data[5],
                wacom->data[6]);

            return IRQ_HANDLED;
        }
		if(wacom->data[0] & 0x01)
			touch_button=1;
		else
			touch_button=0;
		//&*&*&*AL2_20100226, pen down detect
		abs_x=(wacom->data[1]<<9|wacom->data[2]<<2|wacom->data[6]>>5);
		abs_y=(wacom->data[3]<<9|wacom->data[4]<<2|((wacom->data[6]>>3)&0x3));
		abs_pressure=((wacom->data[5]|((wacom->data[6]& 0x7) << 7)));
//			printk("\nabs=%d, abs_y=%d\n", 6144-abs_x, 8192-abs_y);
		if ((abs_x < 0) || (abs_y < 0) || (abs_x > 8192 * 2) || (abs_y > 8192))
			return IRQ_HANDLED;
		if(touch_button == 1 && abs_pressure > 0)
		{
				input_report_key(dev, BTN_TOUCH,touch_button);
//				input_report_abs(dev, ABS_X,(ts_wide-abs_x));
				//input_report_abs(dev, ABS_X, (abs_x * 6670) / 65536);
				input_report_abs(dev, ABS_X, (1024-(abs_x * 6557) / 65536));
		 		//printk("[hanwa trace - down]ABS_X11111=%d,",(1024-(abs_x * 6557) / 65536));
		 		//printk("[hanwa trace - down]ABS_X=%d,",abs_x);
//				input_report_abs(dev, ABS_Y,(8192-abs_y));
				//input_report_abs(dev, ABS_Y, (abs_y * 5353) / 65536);
				input_report_abs(dev, ABS_Y, (600-(abs_y * 5298) / 65536));
				//printk("[hanwa trace - down]ABS_Y11111=%d\n",(600-(abs_y * 5298) / 65536));
				//printk("[hanwa trace - down]ABS_Y=%d\n",abs_y);
				input_report_abs(dev, ABS_PRESSURE,abs_pressure);
				//printk("[hanwa trace - down]ABS_PRESSURE=%d\n",abs_pressure);
				input_sync(dev);
				wacom->idx = 0;
				
				pen_up_flag = 0;
		}
		else if(abs_pressure ==0 && touch_button==0)
		{
				input_report_key(dev, BTN_TOUCH,touch_button);
//				input_report_abs(dev, ABS_X,(ts_wide-abs_x));
				input_report_abs(dev, ABS_X, (1024-(abs_x * 6557) / 65536));
		 		//printk("[hanwa trace - up]ABS_X11111=%d,",(1024-(abs_x * 5137) / 65536));
		 		//printk("[hanwa trace - up]ABS_X=%d,",abs_x);
//				input_report_abs(dev, ABS_Y,(8192-abs_y));
				input_report_abs(dev, ABS_Y, (600-(abs_y * 5298) / 65536));
				//printk("[hanwa trace - up]ABS_Y11111=%d\n",(600-(abs_y * 5298) / 65536));
				//printk("[hanwa trace - up]ABS_Y=%d\n",abs_y);
				input_report_abs(dev, ABS_PRESSURE,abs_pressure);
				//printk("hanwa trace - up]ABS_PRESSURE=%d\n",abs_pressure);
				input_sync(dev);
				wacom->idx = 0;
		}
		else if(abs_pressure ==0 && touch_button==1 && pen_up_flag == 0)
		{
				input_report_key(dev, BTN_TOUCH,0);
//				input_report_abs(dev, ABS_X,(ts_wide-abs_x));
				input_report_abs(dev, ABS_X, (1024-(abs_x * 6557) / 65536));
		 		//printk("[hanwa trace - up]ABS_X11111=%d,",(1024-(abs_x * 5137) / 65536));
		 		//printk("!!!!![hanwa trace - up]ABS_X=%d,",abs_x);
//				input_report_abs(dev, ABS_Y,(8192-abs_y));
				input_report_abs(dev, ABS_Y, (600-(abs_y * 5298) / 65536));
				//printk("[hanwa trace - up]ABS_Y11111=%d\n",(600-(abs_y * 5298) / 65536));
				//printk("!!!![hanwa trace - up]ABS_Y=%d\n",abs_y);
				input_report_abs(dev, ABS_PRESSURE, abs_pressure);
				//printk("!!!!!hanwa trace - up]ABS_PRESSURE=%d\n",abs_pressure);
				input_sync(dev);
				wacom->idx = 0;
				
				pen_up_flag = 1;				
		}
		wacom->idx = 0;
	}

	return IRQ_HANDLED;

}

/*********************************Porc-filesystem-TOP**************************************/

unsigned int hanwang_chip_enable_disable(unsigned int cmd)
{
	//printk("%s\n", __func__);
	
	if (POWER_OFF == power_state)
	{
		if (EMR_ON == cmd) 
		{
			printk("EMR touch panel enable suspend.\n");
		}
	
		if (EMR_OFF == cmd)
		{
			printk("EMR touch panel disable suspend.\n");
		}

		suspend_cmd_state = HAVE_SUSPEND_CMD;
		suspend_cmd_store = cmd;
		
		return 0;
	}
	else
	{
		suspend_cmd_state = NO_SUSPEND_CMD;
	}
	
	if (1 == cmd) 
	{
		printk("EMR touch panel enable.\n");
		
		if (EMR_OFF == emr_state)
		{
			gpio_request(S5PV210_GPJ0(3), "GPJ0"); 
			
			gpio_request(S5PV210_GPA1(3), "GPA1"); 
			
			gpio_request(S5PV210_GPJ0(2), "GPJ0");
			
			s3c_gpio_cfgpin(S5PV210_GPA1(3), S3C_GPIO_PULL_UP);
			
			s3c_gpio_cfgpin(S5PV210_GPJ0(2), S3C_GPIO_PULL_UP);
		
			s3c_gpio_cfgpin(S5PV210_GPJ0(3),S3C_GPIO_OUTPUT);
	
			gpio_direction_output(S5PV210_GPJ0(3), 0);  //EMR_EN = 0
		  
	   		gpio_free(S5PV210_GPJ0(3));
	   		
	   		gpio_free(S5PV210_GPJ0(2));
	   		
	   		gpio_free(S5PV210_GPA1(3));
	   		
	   		hanwang_touch_reset();
	   		
			emr_state = EMR_ON;
		}
	}
	
	if (0 == cmd)
	{
		printk("EMR touch panel disable.\n");
		
		if (EMR_ON == emr_state)
		{
			gpio_request(S5PV210_GPA1(3), "GPA1"); 
			
			gpio_request(S5PV210_GPJ0(3), "GPJ0"); 
			
			gpio_request(S5PV210_GPJ0(2), "GPJ0"); 
			
			s3c_gpio_cfgpin(S5PV210_GPA1(3), S3C_GPIO_PULL_NONE);
			
			s3c_gpio_cfgpin(S5PV210_GPJ0(2), S3C_GPIO_PULL_NONE);
			
			s3c_gpio_cfgpin(S5PV210_GPJ0(3), S3C_GPIO_OUTPUT);
	
			gpio_direction_output(S5PV210_GPJ0(3), 1);  //EMR_EN = 1
		  
	   		gpio_free(S5PV210_GPJ0(3));
	   		
	   		gpio_free(S5PV210_GPJ0(2));
	   		
	   		gpio_free(S5PV210_GPA1(3));

			emr_state = EMR_OFF;			
		}
	}
	
	return 0;
}

unsigned int hanwang_chip_status(void)
{
	//printk("%s\n", __func__);
	
	if (emr_state == EMR_OFF)
	{
		printk("EMR touch panel has been disabled.\n");		
	}
	else if (emr_state == EMR_ON)
	{
		printk("EMR touch panel has been enabled.\n");
	}
	else
	{
		printk("Error: EMR touch panel undefined status.\n");
	}
	
	return 0;
}

unsigned int emr_set_chip(unsigned int cmd)
{
	hanwang_chip_enable_disable(cmd);
	return 0;
}


unsigned int emr_get_chip(void)
{
	hanwang_chip_status();
	return 0;
}

struct emr_proc {
	char *module_name;
	unsigned int (*emr_get) (void);
	unsigned int (*emr_set) (unsigned int);
};

static const struct emr_proc emr_modules[] = {
	{"hanwang_enable", emr_get_chip, emr_set_chip},
	{0}
};

ssize_t emr_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	char *out = page;
	unsigned int emr;
	const struct emr_proc *proc = data;
	ssize_t len;

	emr = proc->emr_get();

	len = out - page - off;
	if (len < count) {
		
		*eof = 1;
		
		if (len <= 0) {
			return 0;
		}
	} else {

		len = count;
	}
	*start = page + off;

	return len;
}

ssize_t emr_proc_write(struct file * filp, const char __user * buf, unsigned long len, void *data)
{
	char command[PROC_CMD_LEN];
	const struct emr_proc *proc = data;

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
		proc->emr_set(1);
	} else if (strnicmp(command, "off", 3) == 0 || strnicmp(command, "0", 1) == 0) {
		proc->emr_set(0);
	} else {
		return -EINVAL;
	}

	return len;
}

//proc file_operations
struct file_operations emr_fops = {
	.owner = THIS_MODULE,
};

static int proc_node_num = (sizeof(emr_modules) / sizeof(*emr_modules));
static struct proc_dir_entry *emr_proc_root;
static struct proc_dir_entry *proc[5];

static int emr_proc_init(void)
{
	unsigned int i;
	
	//printk("%s\n", __func__);

	emr_proc_root = proc_mkdir("hanwang_touch", NULL);

	if (!emr_proc_root) 
	{
		printk("create hanwang_touch directory failed\n");
		return -1;
	}

	for (i = 0; i < proc_node_num; i++) 
	{
		mode_t mode;

		mode = 0;
		if (emr_modules[i].emr_get)
		{
			mode |= S_IRUGO;
		}
		if (emr_modules[i].emr_set)
		{
			mode |= S_IWUGO;
		}

		proc[i] = create_proc_entry(emr_modules[i].module_name, mode, emr_proc_root);
		if (proc[i]) 
		{
			proc[i]->data = (void *)(&emr_modules[i]);
			proc[i]->read_proc = emr_proc_read;
			proc[i]->write_proc = emr_proc_write;
		}
	}

	return 0;
}

static void emr_proc_remove(void)
{
	//printk("%s\n", __func__);
	remove_proc_entry("hanwang_enable", emr_proc_root);
	remove_proc_entry("hanwang_touch", NULL);
}

/*********************************Porc-filesystem-BOTTOM**************************************/

void hanwang_touch_power_off(void)
{
#if 0
    unsigned int cfg;
    
    s3c_gpio_cfgpin(S3C64XX_GPM(4), 0);
	s3c_gpio_setpull(S3C64XX_GPM(4), S3C_GPIO_PULL_NONE);

	//set TX RX to input
	s3c_gpio_cfgpin(S3C64XX_GPA(0), S3C64XX_GPA_INPUT(0));	
	s3c_gpio_setpull(S3C64XX_GPA(0), S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(S3C64XX_GPA(1), S3C64XX_GPA_INPUT(1)); 
	s3c_gpio_setpull(S3C64XX_GPA(0), S3C_GPIO_PULL_NONE);

    //set pen_rst to low
    gpio_set_value(S3C64XX_GPC(2), 0);
    s3c_gpio_cfgpin(S3C64XX_GPC(2), S3C64XX_GPC_OUTPUT(2));
    
	//set touch_en pin output high
	cfg=__raw_readl(S3C64XX_GPKCON1);	   
	cfg &= ~(0x0f<<28);	   
	cfg |= (0x01<<28); 	   
	__raw_writel(cfg,S3C64XX_GPKCON1);
	cfg=__raw_readl(S3C64XX_GPKDAT);	   
	cfg |= (0x01<<15); 	   
	__raw_writel(cfg,S3C64XX_GPKDAT);
#endif
}

void hanwang_touch_reset(void)
{
	gpio_request(S5PV210_GPJ0(3), "GPJ0");   //EMR_EN
	gpio_request(S5PV210_GPJ0(2), "GPJ0");   //EMR_RST
	gpio_request(S5PV210_GPA1(3), "GPA1"); 
/*	
	// Touch_EN output low
    gpio_set_value(S5PV210_GPJ0(3), 0);
    s3c_gpio_cfgpin(S5PV210_GPJ0(3),  S3C_GPIO_SFN(1));
    mdelay(60);
#if defined(CONFIG_HW_EP1_DVT) || defined(CONFIG_HW_EP3_DVT)
    //PEN_RST   GPC2->output high
    gpio_set_value(S3C64XX_GPC(2), 0);
    s3c_gpio_cfgpin(S3C64XX_GPC(2), S3C64XX_GPC_OUTPUT(2));
    mdelay(60);
    gpio_set_value(S3C64XX_GPC(2), 1);
    s3c_gpio_cfgpin(S3C64XX_GPC(2), S3C64XX_GPC_OUTPUT(2));
#endif  
    //Touch_RX GPA0 -> RX
    s3c_gpio_cfgpin(S5PV210_GPA1(2), (0x02 << 8));     

    //Touch_TX GPA1 -> TX
    s3c_gpio_cfgpin(S5PV210_GPA1(3), (0x02 << 12));    
    
    //PEN_DET GPM4 ->pen detect
    //s3c_gpio_cfgpin(S5PV210_GPH2(2), S3C_GPIO_SFN(1));       
    //s3c_gpio_cfgpin(S5PV210_GPH2(2), S3C_GPIO_SFN(3));       
    //s3c_gpio_setpull(S5PV210_GPH2(2), S3C_GPIO_PULL_UP);
*/
	s3c_gpio_cfgpin(S5PV210_GPJ0(3),S3C_GPIO_OUTPUT);
	s3c_gpio_cfgpin(S5PV210_GPJ0(2),S3C_GPIO_OUTPUT);
	
	s3c_gpio_cfgpin(S5PV210_GPA1(3), S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(S5PV210_GPJ0(2), S3C_GPIO_PULL_NONE);
	
	gpio_direction_output(S5PV210_GPJ0(3), 1);  //EMR_EN = 1
	gpio_direction_output(S5PV210_GPJ0(2), 0);  //EMR_RST = 0
	
	mdelay(60);
	
	gpio_direction_output(S5PV210_GPJ0(3), 0);  //EMR_EN = 0
	  
	mdelay(60); 
	
	gpio_direction_output(S5PV210_GPJ0(2), 1);  //EMR_RST = 1
	
	s3c_gpio_cfgpin(S5PV210_GPA1(3), S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(S5PV210_GPJ0(2), S3C_GPIO_PULL_UP);
	
    gpio_free(S5PV210_GPJ0(3));
    gpio_free(S5PV210_GPJ0(2));
    gpio_free(S5PV210_GPA1(3));
}


/*
 * wacom_disconnect() is the opposite of wacom_connect()
 */
static void wacom_disconnect(struct serio *serio)
{
	struct wacom *wacom = serio_get_drvdata(serio);
	printk("[hanwa trace]Enter %s.\n", __FUNCTION__);

	//Don't unregister input device(event2) when suspend, It will make input2(event2) lose.
	//input_get_device(wacom->dev);
	//input_unregister_device(wacom->dev);

	serio_close(serio);
	serio_set_drvdata(serio, NULL);
	//input_put_device(wacom->dev);
	kfree(wacom);

	release_power(VDD_TOUCH_POWER);
	
	power_state = POWER_OFF;
}

/*
 * wacom_connect() is the routine that is called when someone adds a
 * new serio device that supports the Wacom protocol and registers it
 * as input device.
 */
static int wacom_connect(struct serio *serio, struct serio_driver *drv)
{
	struct wacom *wacom;
	int err;
        
	printk("[hanwang trace]Enter %s.\n", __FUNCTION__);

	request_power(VDD_TOUCH_POWER);
	
	power_state = POWER_ON;
	
    hanwang_touch_reset();

    
#if 0
#ifdef FIRMWARE_DOWNLOAD
	//Touch_Pro GPO6 -> OUTPUT		
	//Touch_Pro GPO6 -> HIGH
	gpio_set_value(S3C64XX_GPO(6), 1);
	s3c_gpio_cfgpin(S3C64XX_GPO(6), S3C64XX_GPO_OUTPUT(6));
	mdelay(150);
#endif
#endif

	wacom = kzalloc(sizeof(struct wacom), GFP_KERNEL);
	wacom->serio = serio;
	wacom->dev = input_dev;
//	INIT_DELAYED_WORK(&wacom->delay_work, do_softint);
	snprintf(wacom->phys, sizeof(wacom->phys),
		 "%s/input0", serio->phys);

	input_dev->phys = wacom->phys;

	serio_set_drvdata(serio, wacom);
	err = serio_open(serio, drv);
	if (err)
		goto fail2;

	printk(KERN_ALERT "********write 0 to tty device*********\n");
	serio_write(wacom->serio,'0');
	mdelay(100);
    
	serio_dev = wacom;

#if 0
	if (gpio_get_value(S3C64XX_GPN(4)) == 1)
        	hanwang_touch_power_off();
#endif

	return 0;
 fail2:
 	serio_close(serio);
	serio_set_drvdata(serio, NULL);
	kfree(wacom);
	return err;
}


/*
 * The serio driver structure.
 */
static struct serio_device_id wacom_serio_ids[] = {
	{
		.type	= SERIO_RS232,
		.proto	= SERIO_ANY,//0x3a,//SERIO_WACOM,
		.id	= SERIO_ANY,//0x0,//SERIO_ANY,
		.extra	= SERIO_ANY,//0x0,//SERIO_ANY,
	},
	{ 0 }
};

MODULE_DEVICE_TABLE(serio, wacom_serio_ids);

static struct serio_driver wacom_drv = {
	.driver		= {
		.name	= "wacom_ts",
	},
	.description	= DRIVER_DESC,
	.id_table	= wacom_serio_ids,
	.interrupt	= wacom_interrupt,
	.connect	= wacom_connect,
	.disconnect	= wacom_disconnect,
};

static int __init wacom_init(void)
{
	int err;

	printk(KERN_ALERT "****this is wacom init function****\n");

	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		goto fail1;
	}

	input_dev->name = "Wacom Serial Touchscreen";
	input_dev->id.bustype = BUS_RS232;
	input_dev->id.vendor = SERIO_WACOM;
	input_dev->id.product = 0;
	input_dev->id.version = 0x0100;
	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

	input_set_abs_params(input_dev, ABS_X, 0, 1024, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, 600, 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE, 0,1024, 0, 0);
	
	err = input_register_device(input_dev);
	if (err)
		goto fail3;
	err = serio_register_driver(&wacom_drv);

	register_early_suspend(&emr_touch_early_suspend_desc);

	emr_proc_init();
	
	err = driver_create_file(&wacom_drv.driver, &driver_attr_version);
	err = driver_create_file(&wacom_drv.driver, &driver_attr_tslock);
#ifdef FIRMWARE_DOWNLOAD
	return driver_create_file(&wacom_drv.driver, &driver_attr_download);
#else
	return err;
#endif

fail3:
	input_free_device(input_dev);
fail1:
	printk("[hanwa trace]Enter %s, input_allocate_device error!\n", __FUNCTION__);
	return err;
}

static void __exit wacom_exit(void)
{
	driver_remove_file(&wacom_drv.driver, &driver_attr_version);
	driver_remove_file(&wacom_drv.driver, &driver_attr_tslock);
#ifdef FIRMWARE_DOWNLOAD
	driver_remove_file(&wacom_drv.driver, &driver_attr_download);
#endif
	serio_unregister_driver(&wacom_drv);

	unregister_early_suspend(&emr_touch_early_suspend_desc);

	emr_proc_remove();
}

module_init(wacom_init);
//late_initcall(wacom_init);
module_exit(wacom_exit);
