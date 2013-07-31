/* Morgan capacivite multi-touch device driver.
 *
 * Copyright(c) 2010 MorganTouch Inc.
 *
 * Author: Matt Hsu <matt@0xlab.org>
 *
 */

//#define DEBUG
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/timer.h>
//#include <plat/gpio.h>
#include <linux/gpio.h>
//#include <plat/mux.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/jiffies.h>
#include <linux/miscdevice.h>
// for linux 2.6.36.3
#include <linux/cdev.h>
#include <linux/uaccess.h>

#include <linux/proc_fs.h>	//tommy

#include "mg-i2c-ts.h"
#include <linux/earlysuspend.h>
#include <linux/wakelock.h>

#include <linux/venus/power_control.h>//tommy

#define IOCTL_ENDING    		0xD0
#define IOCTL_I2C_SLAVE			0xD1
#define IOCTL_READ_ID		  	0xD2
#define IOCTL_READ_VERSION  		0xD3
#define IOCTL_RESET  			0xD4
#define IOCTL_IAP_MODE			0xD5
#define IOCTL_CALIBRATION		0xD6
#define IOCTL_ACTION2			0xD7
#define IOCTL_DEFAULT			0x88

static int command_flag= 0;
#define PROC_CMD_LEN 20

extern int charge_status_touch;//tommy

#define MG_DRIVER_NAME 	"mg-i2c-mtouch"
#define BUF_SIZE 		15
//#define RANDY_DEBUG
//static int home_flag = 0;
//static int menu_flag = 0;
//static int back_flag = 0;
u8 ver_buf[5]={0};
int ver_flag = 0;
int  id_flag = 0;
int cal_flag = 0;
#define COORD_INTERPRET(MSB_BYTE, LSB_BYTE) \
		(MSB_BYTE << 8 | LSB_BYTE)

struct mg_data{
	__u16 	x, y, w, p, id;
	struct i2c_client *client;
	/* capacivite device*/
	struct input_dev *dev;
	/* digitizer */

	struct timer_list timer;

	struct input_dev *dig_dev;
	struct mutex lock;
	int irq;
	struct early_suspend early_suspend;
	struct work_struct work;
	struct workqueue_struct *mg_wq;
	int (*power)(int on);
	int intr_gpio;
	int fw_ver;
    struct miscdevice firmware;
	#define IAP_MODE_ENABLE		1	/* TS is in IAP mode already */
	int iap_mode;		/* Firmware update mode or 0 for normal */

};


#ifdef CONFIG_HAS_EARLYSUSPEND
static void mg_early_suspend(struct early_suspend *h);
static void mg_late_resume(struct early_suspend *h);
#endif

static int ac_sw = 1;
static int dc_sw = 1;
static int ack_flag = 0;
static u8 read_buf[BUF_SIZE]={0};

static struct mg_data *private_ts;
int work_finish_yet=1;
//int no_suspend_resume=0;
//int irq_enabled_flag=0;
//int work_well_flag=0;
//int work_well_done=1;
int  set_tg_irq_status(unsigned int irq, bool enable);

int mg_iap_open(struct inode *inode, struct file *filp)
{ 
	struct mg_data *dev;

	#ifdef RANDY_DEBUG	
	printk("[Driver]into mg_iap_open\n");
	#endif
	dev = kmalloc(sizeof(struct mg_data), GFP_KERNEL);
	if (dev == NULL) {
		return -ENOMEM;
	}

	filp->private_data = dev;
	return 0;
}

int mg_iap_release(struct inode *inode, struct file *filp)
{    
	struct mg_data *dev = filp->private_data;
	printk("[Driver]into mg_iap_release\n");
	if (dev)
	{
		kfree(dev);
	}
	printk("[Driver]into mg_iap_release OVER\n");

	return 0;
}
ssize_t mg_iap_write(struct file *filp, const char *buff,    size_t count, loff_t *offp)
{  

    int ret;
    char *tmp;
#ifdef RANDY_DEBUG	
	printk("[Driver]into mg_iap_write\n");
	#endif

 	if (count > 128)
       	count = 128;

    tmp = kmalloc(count, GFP_KERNEL);
    
    if (tmp == NULL)
        return -ENOMEM;

    if (copy_from_user(tmp, buff, count)) {
        return -EFAULT;
    }
	#ifdef RANDY_DEBUG	
	int i = 0;
	printk("[Driver]Writing : ");
	for(i = 0; i < count; i++)
	{
		printk("%4x", tmp[i]);
	}
	printk("\n");
	#endif

	ret = i2c_master_send(private_ts->client, tmp, count);
    	if (!ret) 
		printk("[Driver] i2c_master_send fail, ret=%d \n", ret);
    	kfree(tmp);
#ifdef RANDY_DEBUG	
	printk("[Driver]into mg_iap_write OVER\n");
	#endif
    return ret;

}

ssize_t mg_iap_read(struct file *filp, char __user *buff,    size_t count, loff_t *offp){    

    char *tmp;
    int ret;  
    
	#ifdef RANDY_DEBUG	
	printk("[Driver]into mg_iap_read\n");
	#endif
    	if (count > 128)
        	count = 128;
	/**********  randy delete for return 816 **************/
	if(command_flag == 1)
	{
		printk("<<Waiting>>");
		return -1;
	}
	else
	{
		if(ack_flag == 1)
		{
			copy_to_user(buff, read_buf, 5);

			#ifdef RANDY_DEBUG	
			int i = 0;
			printk("[Driver]Reading : ");
			for(i = 0; i < 5; i++)
			{
				printk("%4x", read_buf[i]);
			}
			printk("\n");
			#endif
			
			ack_flag = 0;
			return 5;
		}
	}
	/**********  randy delete for return 816 **************/
    tmp = kmalloc(count, GFP_KERNEL);

    if (tmp == NULL)
        return -ENOMEM;

	ret = i2c_master_recv(private_ts->client, tmp, count);
   	if (ret >= 0)
        	copy_to_user(buff, tmp, count);
    
	#ifdef RANDY_DEBUG	
	int i = 0;
	printk("[Driver]Reading : ");
	for(i = 0; i < count; i++)
	{
		printk("%4x", tmp[i]);
	}
	printk("\n");
	#endif
    kfree(tmp);
#ifdef RANDY_DEBUG	
	printk("[Driver]into mg_iap_read OVER\n");
	#endif

    return ret;
}
int mg_iap_ioctl(struct inode *inode, struct file *filp,    unsigned int cmd, unsigned long arg)
{
	u_int8_t ret = 0;
//	int __user *argp = (int __user *) arg;
/*<-- LH_SWRD_CL1_richard@20110810 judge the calibration ACK correct or not 
	int cal_backup = 0;
	if((cal_flag == 2) ||(cal_flag == 3))
	{
	
	printk("[Driver ]inside cal_flag = %d\n",cal_flag);
		cal_backup = cal_flag;
		cal_flag = 0;
		return cal_backup;
	}
LH_SWRD_CL1_richard@20110810 judge the calibration ACK correct or not-->*/
	command_flag = 1;

	#ifdef RANDY_DEBUG	
	printk("[Driver ]mg_iap_ioctl  Cmd =%x\n", cmd);

	#endif

	switch (cmd) {        
	  
		case IOCTL_DEFAULT:
			#ifdef RANDY_DEBUG	
			printk("[Driver ] IOCTL_DEFAULT  \n");
			#endif
			break;   
			
		case IOCTL_I2C_SLAVE: 
			#ifdef RANDY_DEBUG	
			printk("[Driver ] IOCTL_I2C_SLAVE  \n");
			#endif
			
			gpio_set_value(TOUCH_RESET,1);
		printk("  the TOUCH_RESET pin value is %d\n", gpio_get_value(TOUCH_RESET));
			command_flag = 0;
			break;   

		case IOCTL_RESET:
			#ifdef RANDY_DEBUG	
			printk("[Driver ] IOCTL_RESET  \n");
				#endif
			
		gpio_set_value(TOUCH_RESET,0);
		printk("  the TOUCH_RESET pin value is %d\n", gpio_get_value(TOUCH_RESET));
			command_flag = 0;
			break;
		case IOCTL_READ_ID:        
		#ifdef RANDY_DEBUG
			printk("[Driver ] IOCTL_READ_ID\n");
			#endif
			ret = i2c_master_send(private_ts->client, 
								command_list[3], COMMAND_BIT);

			if(ret < 0)
			{
				printk("[Driver ]IOCTL_READ_ID error!!!!!\n");
				return -1;
			}

			break;    
		case IOCTL_READ_VERSION:    
			#ifdef RANDY_DEBUG    
			printk("[Driver ] IOCTL_READ_VERSION\n");
			#endif
			ret = i2c_master_send(private_ts->client, 
								command_list[4], COMMAND_BIT);

			if(ret < 0)
			{
				printk("[Driver ]IOCTL_READ_VERSION error!!!!!\n");
				return -1;
			}

			break;    
			
		case IOCTL_ENDING:        
			#ifdef RANDY_DEBUG	
			printk("[Driver ] IOCTL_ENDING\n");
			#endif
			

			ret = i2c_master_send(private_ts->client, 
								command_list[13], COMMAND_BIT);
			if(ret < 0)
			{
				printk("[Driver ]IOCTL_ENDING error!!!!!\n");
				return -1;
			}
			command_flag = 0;
			break;        
		case IOCTL_ACTION2:        
			#ifdef RANDY_DEBUG	
			printk("[Driver ] IOCTL_ACTION2\n");
			#endif

			ret = i2c_master_send(private_ts->client, 
								command_list[12], COMMAND_BIT);

			if(ret < 0)
			{
				printk("[Driver ]IOCTL_ACTION2 error!!!!!\n");
				return -1;
			}

			break;        
		case IOCTL_IAP_MODE:
			#ifdef RANDY_DEBUG	
			printk("[Driver ] IOCTL_IAP_MODE\n");
			#endif
			
			ret = i2c_smbus_write_i2c_block_data(private_ts->client,
									0, COMMAND_BIT, command_list[10]);
			if(ret < 0)
			{
				printk("[Driver ]IOCTL_IAP_MODE error!!!!!\n");
				return -1;
			}

			break;
		case IOCTL_CALIBRATION:
			#ifdef RANDY_DEBUG	
			printk("[Driver ] IOCTL_CALIBRATION1\n");
			#endif
			ret = i2c_smbus_write_i2c_block_data(private_ts->client,
									0, COMMAND_BIT, command_list[0]);
			if(ret < 0)
			{
				printk("[Driver ]Calibrate  Write error!!!!!\n");
				return -1;
			}

			break;
		default:            
			#ifdef RANDY_DEBUG	
			printk("[Driver ] default  \n");
			#endif
			break;   
	}     

	return 0;
}


struct file_operations mg_touch_fops = {    
        open:       mg_iap_open,    
        write:      mg_iap_write,    
        read:	    mg_iap_read,    
        release:    mg_iap_release,    
        ioctl:      mg_iap_ioctl,  
 };

 
/*********************************Proc-filesystem-TOP**************************************/
unsigned int mg_get_version(char *buf)
{
	int err;
	#ifdef RANDY_DEBUG    
	printk("[Driver ] IOCTL_READ_VERSION\n");
	#endif
	err = i2c_master_send(private_ts->client, 
			command_list[4], COMMAND_BIT);
	if(err < 0)
	{
	printk("[Driver ]IOCTL_READ_VERSION error!!!!!\n");
	return -1;
	//goto exit_input;
	}
	ver_flag = 1;
	command_flag = 1;
	//*/
	//mdelay(30);
	mdelay(100);

	ack_flag = 0;
	if (ver_flag == 0)
		return sprintf(buf,"%x.0%x%c \n",  ver_buf[3], ver_buf[4], ver_buf[2]);
	else
	{
		command_flag = 0;
		return sprintf(buf,"couldn't read firmware version \n");
	}
}

unsigned int mg_set_version(unsigned int version)
{
	//  printk("%s:Blank function\n", __FUNCTION__);
	return 0;
}

struct mg_proc {
	char *module_name;
	unsigned int (*mg_get) (char *);
	unsigned int (*mg_set) (unsigned int);
};

static const struct mg_proc mg_modules[] = {
	{"version", mg_get_version, mg_set_version},
	/*{"mg_calibration", mg_get_calibration, mg_set_calibration},*/
};


ssize_t mg_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	//  printk("============%s,%d\n",__FUNCTION__,__LINE__);
	ssize_t len;
	char *p = page;
	const struct mg_proc *proc = data;

	p += proc->mg_get(p);

	len = p - page;

	*eof = 1;

	//printk("len = %d\n", len);
	return len;
}

size_t mg_proc_write(struct file * filp, const char __user * buf, unsigned long len, void *data)
{
	//  printk("============%s,%d\n",__FUNCTION__,__LINE__);
	char command[PROC_CMD_LEN];
	const struct mg_proc *proc = data;

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
		proc->mg_set(1);
	} else if (strnicmp(command, "off", 3) == 0 || strnicmp(command, "0", 1) == 0) {
		proc->mg_set(0);
	} else {
		return -EINVAL;
	}

	return len;
}

//proc file_operations
struct file_operations mg_fops = {
	.owner = THIS_MODULE,
};

static int proc_node_num = (sizeof(mg_modules) / sizeof(*mg_modules));
static struct proc_dir_entry *update_proc_root;
static struct proc_dir_entry *proc[5];

static int mg_proc_init(void)
{
	//  printk("============%s,%d\n",__FUNCTION__,__LINE__);
	unsigned int i;
	//struct proc_dir_entry *proc;

	update_proc_root = proc_mkdir("touch", NULL);

	if (!update_proc_root) 
	{
		printk("create mg_touch directory failed\n");
		return -1;
	}

	for (i = 0; i < proc_node_num; i++) 
	{
		mode_t mode;

		mode = 0;
		if (mg_modules[i].mg_get)
		{
			mode |= S_IRUGO;
		}
		if (mg_modules[i].mg_set)
		{
			mode |= S_IWUGO;
		}

		proc[i] = create_proc_entry(mg_modules[i].module_name, mode, update_proc_root);
		if (proc[i]) 
		{
			proc[i]->data = (void *)(&mg_modules[i]);
			proc[i]->read_proc = mg_proc_read;
			proc[i]->write_proc = mg_proc_write;
		}
	}

	return 0;
}

#if 0
static void mg_proc_remove(void)
{
	//  printk("============%s,%d\n",__FUNCTION__,__LINE__);
	remove_proc_entry("version", update_proc_root);
	/*/
	  remove_proc_entry("mg_calibration", update_proc_root);
	//*/ 
	remove_proc_entry("touch", NULL);
}
#endif 

/*********************************Proc-filesystem-BOTTOM**************************************/
 
static ssize_t mg_fw_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
	int err;

	#ifdef RANDY_DEBUG    
	printk("[Driver ] IOCTL_READ_VERSION\n");
	#endif
	printk("[Driver ]DISABLE !!!!!!!\n");
	err = i2c_master_send(private_ts->client, 
						command_list[2], COMMAND_BIT);
	if(err < 0)
	{
		printk("[Driver ]DISABLE  Error !!!!!!!\n");
		return -1;
	}
	printk("[Driver ]ENABLE !!!!!!!\n");
	err = i2c_master_send(private_ts->client, 
						command_list[1], COMMAND_BIT);
	if(err < 0)
	{
		printk("[Driver ]ENABLE  Error !!!!!!!\n");
		return -1;
	}
	mdelay(50);
	err = i2c_master_send(private_ts->client, 
						command_list[4], COMMAND_BIT);
	if(err < 0)
	{
		printk("[Driver ]IOCTL_READ_VERSION error!!!!!\n");
		return -1;
	}
	ver_flag = 1;
	command_flag = 1;
	
	//++++++add for nomal mode
	mdelay(30);
	mdelay(100);
	//+++++modify 4x => x
	return sprintf(buf,"%x,%x,%x,%x,%x\n",  ver_buf[0], ver_buf[1],ver_buf[2],ver_buf[3],ver_buf[4]);
}


static irqreturn_t mg_irq(int irq, void *_mg);
#if 0
static ssize_t mg_work_well(struct i2c_client *client)
{	

	int err;

	err = i2c_master_send(private_ts->client, 
					command_list[4], COMMAND_BIT);

	if(err < 0)
		{
			printk("[Driver ]IOCTL_READ_VERSION error!!!!!\n");
			return -1;
		}
	ver_flag = 1;
	
	command_flag = 1;
	int counter=0;
       while (ver_flag == 1)
       	{       		
		msleep(10);
		counter++;
		if (counter > 30)
			break;
       	}

	if (ver_flag == 0)
		{
		if ((ver_buf[0][0] == 0xde) && (ver_buf[0][1] == 0xee))
			{
			return 1;
			}
		else
			{
			free_irq(private_ts->irq, private_ts);
			request_irq(private_ts->irq, mg_irq,IRQF_TRIGGER_FALLING | IRQF_DISABLED, MG_DRIVER_NAME, private_ts);					
			return -1;			
			}
		}
	else
		{
		free_irq(private_ts->irq, private_ts);
		request_irq(private_ts->irq, mg_irq,IRQF_TRIGGER_FALLING | IRQF_DISABLED, MG_DRIVER_NAME, private_ts);					
		ver_flag = 0;
		command_flag = 0;
		printk("couldn't read firmware version\n");
		return -1;
		
}	
#endif


#if 0
static ssize_t mg_id_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
	
  printk("[Touch]:(%s) touch_ID_version = %s \n",__FUNCTION__, ver_buf[1]);
	
	return sprintf(buf, "%x, %x, %x, %x, %x\n", ver_buf[1][0], ver_buf[1][1], ver_buf[1][2], ver_buf[1][3], ver_buf[1][4]);
}
#endif
static DEVICE_ATTR(firmware_version, S_IRUGO|S_IWUSR|S_IWGRP, mg_fw_show, NULL);
//static DEVICE_ATTR(id_version, S_IRUGO|S_IWUSR|S_IWGRP, mg_id_show, NULL);

static struct attribute *mg_attributes[] = {
	&dev_attr_firmware_version.attr,
//	&dev_attr_id_version.attr,
	NULL
};

static const struct attribute_group mg_attr_group = {
	.attrs = mg_attributes,
};

/*/useless program
static void time_led_off(unsigned long _data)
{
	printk("=******MORGAN_TOUCH******* touch_led  off \n");
	light_touch_LED(TOUCH_LED, LED_OFF);
}
//*/
static struct i2c_driver mg_driver;

/*
if charge_status_touch == 3 then dc
else ac
*/
static void mg_ac_dc_sw(struct mg_data *mg)
{
	int ret=0;
	//int switch_ad = 1;
	//switch_ad = gpio_get_value(51);
	//if( ( touch_ac_dc == 3 ) || (touch_ac_dc == 1) || (touch_ac_dc == 2))
	//if( !switch_ad )
	if(charge_status_touch != 3)
	{

		dc_sw = 1;
		if(ac_sw == 1)
		{
			command_flag = 1;
			//mdelay(10);
			//disable_irq_nosync(mg->client->irq);
			ret = i2c_smbus_write_i2c_block_data(mg->client, 0, COMMAND_BIT, touch_list[1]);
			if (ret < 0)
			{
				 printk("mg get the AC mode fail!\n"); 
			}
			else
			{
				 printk("mg get the AC mode success!\n"); 
			}
		
			msleep(30);
			/**********  randy delete for return 816 **************/
			ack_flag = 0;
			/**********  randy delete for return 816 **************/
			ret = i2c_smbus_write_i2c_block_data(mg->client, 0, COMMAND_BIT, touch_list[2]);
			if (ret < 0)
			{
				 printk("mg report message again fail!\n"); 
			}
			else
			{
				printk("mg report message again success!\n");
			}
		
			ac_sw = 0;
			//set_tg_irq_status(mg->client->irq, true);
		
		}
	}
	else
	{
	
		ac_sw = 1;
		if(dc_sw == 1)
		{
			command_flag = 1;
			//mdelay(10);
			//disable_irq_nosync(mg->client->irq);
			ret = i2c_smbus_write_i2c_block_data(mg->client, 0, COMMAND_BIT, touch_list[0]);
			if (ret < 0)
			{
				 printk("mg get the DC mode fail!\n"); 
			}
			else
			{
				printk("mg get the DC mode success!\n");
			}
		
			msleep(30);
			ack_flag = 0;
			/**********  randy delete for return 816 **************/
			ret = i2c_smbus_write_i2c_block_data(mg->client, 0, COMMAND_BIT, touch_list[2]);
			if (ret < 0)
			{
				 printk("mg report message again fail!\n"); 
			}
			else
			{
				printk("mg report message again success!\n");
			}
		
			dc_sw = 0;
			//set_tg_irq_status(mg->client->irq, true);
		
		}
	}
		
}

static irqreturn_t mg_irq(int irq, void *_mg)
{
	struct mg_data *mg = _mg;
	if (work_finish_yet == 0)
		return IRQ_HANDLED;
	work_finish_yet=0;
	//if (work_well_done == 0)
	//	return IRQ_HANDLED;	
	disable_irq_nosync(irq);
    //irq_enabled_flag = 0;
	schedule_work(&mg->work);
	return IRQ_HANDLED;
}

static inline void mg_mtreport(struct mg_data *mg)
{
	//printk("Touch: x = %x,y = %x",mg->x,mg->y);
	if((mg->x > CAP_MAX_X)||(mg->y > CAP_MAX_Y))	
	{
		#ifdef RANDY_DEBUG	
		printk("Reporting  the X and Y Value is error!" ); 
		#endif
		input_report_abs(mg->dev, ABS_MT_TRACKING_ID, 0);
		input_report_abs(mg->dev, ABS_MT_TOUCH_MAJOR, 0);
		input_report_abs(mg->dev, ABS_MT_WIDTH_MAJOR, 0);
		input_report_abs(mg->dev, ABS_MT_POSITION_X, 0 );
		input_report_abs(mg->dev, ABS_MT_POSITION_Y, 0 );
		input_mt_sync(mg->dev);
	}
	else
	{	
		input_report_abs(mg->dev, ABS_MT_TRACKING_ID, mg->id);
		input_report_abs(mg->dev, ABS_MT_TOUCH_MAJOR, mg->w);
		input_report_abs(mg->dev, ABS_MT_WIDTH_MAJOR, 0);
		input_report_abs(mg->dev, ABS_MT_POSITION_X, (mg->x ) );
		input_report_abs(mg->dev, ABS_MT_POSITION_Y, (mg->y ) );
		input_mt_sync(mg->dev);
	}
}

static void mg_i2c_work(struct work_struct *work)
{
	int i = 0;
	struct mg_data *mg =
			container_of(work, struct mg_data, work);
	u_int8_t ret = 0;

	struct i2c_client *client=mg->client;
	//struct mg_data *ts = i2c_get_clientdata(client);
	for(i = 0; i < BUF_SIZE; i ++)	
			read_buf[i] = 0;
	mutex_lock(&mg->lock);
	if(command_flag == 1)
	{
		ret = i2c_master_recv(mg->client, read_buf, 5);
		if(ret < 0)
		{
			printk("Read error!!!!!\n");
			goto out;
		}
		#ifdef RANDY_DEBUG	
		printk("\nRecieve Command ACK : "); 
		for(i = 0; i < 5; i ++) 	
			printk("%4x", read_buf[i]);
		printk("\n");
		#endif
/*<-- LH_SWRD_CL1_richard@20110810  judge the calibration ACK value
		if(cal_flag == 1)
		{
			if( read_buf[4] == 0x55)
			{
				//intk("<><><><><>%4x \n", cal_buf[0]);
				printk("Cal IRQ\n");
			
				cal_flag = 2;
		
			}
			else
			{
				printk("Cal IRQ Error \n");
				cal_flag = 3;
			}
				cal_flag = 3;
		}
LH_SWRD_CL1_richard@20110810  judge the calibration ACK value -->*/
		if(ver_flag == 1 || id_flag == 1)
		{
			for(i = 0; i < 5; i ++)
				ver_buf[i] = read_buf[i];
			mdelay(20);
			ret =i2c_smbus_write_i2c_block_data(mg->client, 0, COMMAND_BIT, touch_list[2]);
			if(ret<0)
			{
				printk("Wistron==>show f/w!!\n");
				return;
			}

		ver_flag = 0;
		}
		ack_flag = 1;
		command_flag = 0;
		goto out;
	}

	ret = i2c_smbus_read_i2c_block_data(mg->client,
							0x0, BUF_SIZE, read_buf);
	if(ret < 0)
	{
		printk("Read error!!!!!\n");
		goto out;
	}

			#ifdef RANDY_DEBUG	
				printk("\nRead: "); 
				for(i = 0; i < BUF_SIZE; i ++) 	
					printk("%4x", read_buf[i]);
			#endif

	if(read_buf[MG_MODE]  == 2)
	{
		return ;
		/*/
		switch(read_buf[TOUCH_KEY_CODE])
		{
		case 1:

//				printk("=******MORGAN_TOUCH******* touch_led  on \n");
				light_touch_LED(TOUCH_LED, LED_ON);

			home_flag = 1;
			input_event(mg->dev, EV_KEY, KEY_HOME, 1);
			#ifdef RANDY_DEBUG	
				printk("Home\n"); 
			#endif
                     
			break;
		case 2:

//				printk("=******MORGAN_TOUCH******* touch_led  on \n");
				light_touch_LED(TOUCH_LED, LED_ON);

			back_flag = 1;
			input_event(mg->dev, EV_KEY, KEY_BACK, 1);
   	                  #ifdef RANDY_DEBUG	
			printk("back\n"); 
			#endif
			break;
		case 3:

//				printk("=******MORGAN_TOUCH******* touch_led  on \n");
				light_touch_LED(TOUCH_LED, LED_ON);

			menu_flag = 1;
			input_event(mg->dev, EV_KEY, KEY_MENU, 1);
                     	#ifdef RANDY_DEBUG	
			printk("menu\n"); 
			#endif
			break;
		case 0:
			if(home_flag == 1)
			{
				home_flag = 0;
				//input_report_key(mg->dig_dev, BTN_TOUCH, 0);
				input_event(mg->dev, EV_KEY, KEY_HOME, 0);
			#ifdef RANDY_DEBUG	
				printk("home0\n"); 
			#endif
			}
			else if(back_flag == 1)
			{
				back_flag = 0;
				input_event(mg->dev, EV_KEY, KEY_BACK, 0);
			#ifdef RANDY_DEBUG	
				printk("back0\n"); 
			#endif
			}
			else
			{
				menu_flag = 0;
				input_event(mg->dev, EV_KEY, KEY_MENU, 0);
			#ifdef RANDY_DEBUG	
				printk("menu0\n"); 
			#endif
			}

	mod_timer(&mg->timer, jiffies + msecs_to_jiffies(2000));
			
			#ifdef RANDY_DEBUG	
				printk("BTN_TOUCH00\n"); 
			#endif
		input_report_key(mg->dev, BTN_TOUCH, 0);
			#ifdef RANDY_DEBUG	
				printk("BTN_TOUCH11\n"); 
			#endif
			input_sync(mg->dev);
			#ifdef RANDY_DEBUG	
				printk("sync\n"); 
			#endif
			break;
		default:
			break;
		}
		//*/
	}
	else
	{
		if(read_buf[ACTUAL_TOUCH_POINTS] >= 1)
		{
			mg->x = COORD_INTERPRET(read_buf[MG_POS_X_HI], read_buf[MG_POS_X_LOW]);
			mg->y = CAP_MAX_Y - COORD_INTERPRET(read_buf[MG_POS_Y_HI], read_buf[MG_POS_Y_LOW]);
			mg->w = read_buf[MG_STATUS];
			mg->id = read_buf[MG_CONTACT_ID];
			mg_mtreport(mg);
		}
		if(read_buf[ACTUAL_TOUCH_POINTS] == 2) 
		{
			mg->x = COORD_INTERPRET(read_buf[MG_POS_X_HI2], read_buf[MG_POS_X_LOW2]);
			mg->y = CAP_MAX_Y - COORD_INTERPRET(read_buf[MG_POS_Y_HI2], read_buf[MG_POS_Y_LOW2]);
			mg->w = read_buf[MG_STATUS2];
			mg->id = read_buf[MG_CONTACT_ID2];
			mg_mtreport(mg);
		}
	input_sync(mg->dev);
	}
	mg_ac_dc_sw(mg);

out:
	mutex_unlock(&mg->lock);			
	enable_irq(client->irq);
	work_finish_yet=1;
}

static int
mg_probe(struct i2c_client *client, const struct i2c_device_id *ids)
{
	struct mg_data *mg;
	struct input_dev *input_dev;
	int err = 0;

	/* allocate mg data */
	mg = kzalloc(sizeof(struct mg_data), GFP_KERNEL);
	if (!mg)
		return -ENOMEM;

	mg->client = client;
	dev_info(&mg->client->dev, "device probing\n");
	i2c_set_clientdata(client, mg);
	mutex_init(&mg->lock);


      mg->firmware.minor = MISC_DYNAMIC_MINOR;
      mg->firmware.name = "MG_Update";
      mg->firmware.fops = &mg_touch_fops;
      mg->firmware.mode = S_IRWXUGO; 
      if (misc_register(&mg->firmware) < 0)
         printk("[mg]misc_register failed!!");
      else
         printk("[mg]misc_register finished!!"); 

	/* allocate input device for capacitive */
	input_dev = input_allocate_device();

	if (!input_dev) {
		dev_err(&mg->client->dev, "failed to allocate input device \n");
		goto exit_kfree;
	}

	input_dev->name = "mg-capacitive";
	input_dev->phys = "I2C";
	input_dev->id.bustype = BUS_I2C;
	input_dev->id.vendor = 0x0EAF;
	input_dev->id.product = 0x1020;

	mg->dev = input_dev;
	input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) |
					BIT_MASK(EV_ABS);
	input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
//	set_bit(KEY_BACK, input_dev->keybit);
//	set_bit(KEY_MENU, input_dev->keybit);
//    set_bit(KEY_HOME, input_dev->keybit);

	set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit);
	set_bit(ABS_MT_POSITION_X, input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, input_dev->absbit);
	
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 8, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, 8, 0, 0);
//	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, 4, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, CAP_MAX_X, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, CAP_MAX_Y, 0, 0);
	
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);



//&*&*&*JJ2
	err = input_register_device(input_dev);
	if (err)
		goto exit_input;
	
	s3c_gpio_cfgpin(TOUCH_RESET, S3C_GPIO_SFN(1));
	//gpio_direction_output(TOUCH_RESET,1);
	gpio_set_value(TOUCH_RESET,1);
//	printk("&*&*&*&*&*&*&*&*&**&*&*&*&*&*&**&*&*&*&*&*&*&*  the TOUCH_RESET pin value is %d\n", gpio_get_value(TOUCH_RESET));

	err = sysfs_create_group(&client->dev.kobj, &mg_attr_group);
	if (err)
		printk("[Touch](%s)failed to create flash sysfs files\n",__FUNCTION__);

//	setup_timer(&mg->timer, time_led_off, (unsigned long)mg);//about led 

	INIT_WORK(&mg->work, mg_i2c_work);

	/* request IRQ resouce */
	if (client->irq < 0) {
		dev_err(&mg->client->dev,
			"No irq allocated in client resources!\n");
		goto exit_input;
	}

	mg->irq = client->irq;
	err = request_irq(mg->irq, mg_irq,
		IRQF_TRIGGER_FALLING /*| IRQF_DISABLED*/, MG_DRIVER_NAME, mg);
	disable_irq(client->irq);

#ifdef CONFIG_HAS_EARLYSUSPEND
		mg->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
		mg->early_suspend.suspend = mg_early_suspend;
		mg->early_suspend.resume = mg_late_resume;
		register_early_suspend(&mg->early_suspend);
#endif
#ifdef RANDY_DEBUG	
		printk("\nregister_early_suspend  OK !!!!\n"); 
#endif

	private_ts = mg;
	//mg_work_well(client);
	//work_well_done = 1;
		//*/read fw ver.start
	work_finish_yet = 1;
	enable_irq(client->irq);
	mg_proc_init();
	//*/read fw ver. end 
//	enable_irq(client->irq);	

	return 0;

exit_input:
	input_unregister_device(mg->dev);
exit_kfree:
	kfree(mg);
	return err;
}

static int __devexit mg_remove(struct i2c_client *client)
{
	struct mg_data *mg = i2c_get_clientdata(client);

	free_irq(mg->irq, mg);
	input_unregister_device(mg->dev);
	del_timer_sync(&mg->timer);//addby richard 20110704
	//work_finish_yet = 1;
	cancel_work_sync(&mg->work);
	kfree(mg);
	return 0;
}
/*/
static int mg_firstpart_suspend(struct i2c_client *client, pm_message_t state)
{
	struct mg_data *ts = i2c_get_clientdata(client);
//	time_led_off((unsigned long)ts);
	return 0;
}
//*/
static int mg_suspend(struct i2c_client *client, pm_message_t state)
{
	int ret;
	//int counter=0;
	struct mg_data *ts = i2c_get_clientdata(client);
	disable_irq(client->irq);
	ret = cancel_work_sync(&ts->work);
	ret = i2c_smbus_write_i2c_block_data(ts->client, 0, COMMAND_BIT, command_list[5]);
	if (ret < 0)
		printk(KERN_ERR "mg_suspend: i2c_smbus_write_i2c_block_data failed\n");
//	time_led_off((unsigned long)ts);
	del_timer_sync(&ts->timer);
	return 0;
}

static int mg_resume(struct i2c_client *client)
{
	int ret;
	struct mg_data *ts = i2c_get_clientdata(client);
	int i;
	for (i=0; i < 3; i++)
	{
		ret = i2c_smbus_write_i2c_block_data(ts->client, 0, 5, command_list[7]);
		if (ret < 0)
		{
			if (i == 2)
			{
				printk(KERN_ERR "mg_resume: i2c_smbus_write_i2c_block_data   failed\n");

			}
		}
		else 
		{
		printk("mg_resume: i2c_smbus_write_i2c_block_data  success\n");
		//work_well_flag = 1;
		break;
		}
	}		
	work_finish_yet = 1;
	enable_irq(client->irq);	
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mg_early_suspend(struct early_suspend *h)
{
	
	struct mg_data *ts;
	printk("mg debug %d, %s, %s\n",__LINE__,__FUNCTION__,__FILE__);	
	ts = container_of(h, struct mg_data, early_suspend);
	mg_suspend(ts->client, PMSG_SUSPEND);
	//mg_firstpart_suspend(ts->client, PMSG_SUSPEND);
	//release_power(VDD_TOUCH_POWER);
}

static void mg_late_resume(struct early_suspend *h)
{
	struct mg_data *ts;
	//request_power(VDD_TOUCH_POWER);
	//msleep(100)
	printk("mg debug%d, %s, %s\n",__LINE__,__FUNCTION__,__FILE__);	
	ts = container_of(h, struct mg_data, early_suspend);
	mg_resume(ts->client);
}

#else
#define mg_suspend NULL
#define mg_resume NULL
#endif

static struct i2c_device_id mg_id_table[] = {
    /* the slave address is passed by i2c_boardinfo */
    {MG_DRIVER_NAME, },
    {/* end of list */}
};

static struct i2c_driver mg_driver = {
	.driver = {
		.name	 = MG_DRIVER_NAME,

	},
	.id_table 	= mg_id_table,
	.probe 		= mg_probe,
	.remove 	= mg_remove,
	//.suspend = mg_suspend,	
};

static int __init mg_init(void)
{
	return i2c_add_driver(&mg_driver);
}

static void mg_exit(void)
{
	i2c_del_driver(&mg_driver);
}
module_init(mg_init);
module_exit(mg_exit);

MODULE_AUTHOR("Randy pan <panhuangduan@morgan-touch.com>");
