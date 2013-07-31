/*
 *  smb137b.c 
 *
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/signal.h>
#include <linux/slab.h> //mem malloc
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/major.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/workqueue.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/unaligned.h>
#include <asm/gpio.h>

//#include <plat/regs-gpio.h>
#include <plat/irqs.h>


#include <mach/map.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <asm/mach/irq.h>
#include <plat/gpio-cfg.h>
#include <mach/gpio.h>		// jimmy add 
#include <linux/gpio.h>
#include <linux/venus/charge.h>
#include "smb137b.h"
#include <linux/venus/power_control.h>
#include <linux/earlysuspend.h>


int smb137b_get_dc_status;
int set_current_limmit_float = 0;
int smb137b_lbr_mode = 0;	// 0: not set, 1: set
int smb137b_lbr_state = 0;	// 0: not in, 1: in
int smb137b_get_charge_status(void);
int smb137b_check_dc_or_usb_status(void);
int smb137b_check_dc_status(void);
int smb137b_check_usb_status(void);
int smb137b_check_usb_or_dc(void);
int smb137b_get_battery_present(void);
struct smb137b_data *gp_smb;
extern int bat_level;

struct early_suspend e_s;
struct smb137b_data *priv;

#ifdef CONFIG_VENUS_TOUCHSCREEN_MG8698S 
int charge_status_touch = 0;//tommy: about touch ac_dc_mode
#endif

static int smb137b_i2c_read(struct i2c_client *client, unsigned char reg, unsigned char * buf)
{
	int ret;
	struct i2c_msg send_msg = { client->addr, 0, 1, &reg };
	struct i2c_msg recv_msg = { client->addr, I2C_M_RD, 1, buf };
 
	ret = i2c_transfer(client->adapter, &send_msg, 1);
	if (ret < 0) 
	{
		printk(KERN_ALERT "%s transfer sendmsg Error.\n",__func__);
		return -EIO;
	}

	ret = i2c_transfer(client->adapter, &recv_msg, 1);
	if (ret < 0)
	{
		printk(KERN_ALERT "%s receive reg_data error.\n",__func__);	
		return -EIO;
	}

	return 0;     	
}


static int smb137b_i2c_write(struct i2c_client *client, unsigned char reg, unsigned char val)
{
	int ret;
	unsigned char buf[2];
	struct i2c_msg msg = { client->addr, 0, 2, buf };

	buf[0] = reg;
	buf[1] = val;

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0)
	{
		printk(KERN_ALERT "%s write reg Error.\n",__func__);
		return -EIO;
	}

	return 0;
}

static int smb137b_i2c_read_block_data(struct i2c_client *client, unsigned char reg, int count, unsigned char *buf)
{
	int ret;
	struct i2c_msg send_msg = { client->addr, 0, 1, &reg };
	struct i2c_msg recv_msg = { client->addr, I2C_M_RD, count, buf };
  
	ret = i2c_transfer(client->adapter, &send_msg, 1);
	if (ret < 0) 
	{
		printk(KERN_ALERT "%s transfer sendmsg Error.\n",__func__);
		return -EIO;
	}

	ret = i2c_transfer(client->adapter, &recv_msg, 1);
	if (ret < 0)
	{
		printk(KERN_ALERT "%s receive reg_data error.\n",__func__);	
		return -EIO;
	}

	return 0;     	
}

void smb137b_charge_enable(void)
{
	int err = 0;

	if (gpio_is_valid((gp_smb->pdata).gpios->gpio_charge_en)) {
			err = gpio_request((gp_smb->pdata).gpios->gpio_charge_en, "charge_enable");

			if (err) {
				printk(KERN_ERR "failed to request charge_enable for "
					"charge enable control on\n");
			}
			gpio_direction_output((gp_smb->pdata).gpios->gpio_charge_en,(gp_smb->pdata).gpios->gpio_charge_en_active);
		}
	gpio_free((gp_smb->pdata).gpios->gpio_charge_en);	

}

void smb137b_charge_disable(void)
{
	int err = 0;
	
	if (gpio_is_valid((gp_smb->pdata).gpios->gpio_charge_en)) {
			err = gpio_request((gp_smb->pdata).gpios->gpio_charge_en, "charge_enable");

			if (err) {
				printk(KERN_ERR "failed to request charge_enable for "
					"charge enable control on\n");
			}
			gpio_direction_output((gp_smb->pdata).gpios->gpio_charge_en, !(gp_smb->pdata).gpios->gpio_charge_en_active);
		}
	gpio_free((gp_smb->pdata).gpios->gpio_charge_en);
}

#if 0
void smb137b_current_HC_enable(void)
{
	int err = 0;
	
	if (gpio_is_valid((gp_smb->pdata).gpios->gpio_current_limint)) {
			err = gpio_request((gp_smb->pdata).gpios->gpio_current_limint, "current HC mode");

			if (err) {
				printk(KERN_ERR "failed to request current limint for "
					"current HC mode\n");
			}
			gpio_direction_output((gp_smb->pdata).gpios->gpio_current_limint, (gp_smb->pdata).gpios->gpio_current_limint_active);
		}
	gpio_free((gp_smb->pdata).gpios->gpio_current_limint);

	set_current_limmit_float = 1;

}

void smb137b_current_USB500_enable(void)
{
	int err = 0;

	if (gpio_is_valid((gp_smb->pdata).gpios->gpio_current_limint)) {
			err = gpio_request((gp_smb->pdata).gpios->gpio_current_limint, "current USB500 mode");

			if (err) {
				printk(KERN_ERR "failed to request current limint for "
					"current USB500 mode\n");
			}
			gpio_direction_output((gp_smb->pdata).gpios->gpio_current_limint, !(gp_smb->pdata).gpios->gpio_current_limint_active);
		}
	gpio_free((gp_smb->pdata).gpios->gpio_current_limint);
	
	set_current_limmit_float = 0;
}
#endif

void smb137b_otg_lbr_enable(void)
{
	int err = 0;
	
	if (gpio_is_valid((gp_smb->pdata).gpios->gpio_otg_en)) {
			err = gpio_request((gp_smb->pdata).gpios->gpio_otg_en, "otg_enable");

			if (err) {
				printk(KERN_ERR "failed to request otg_enable for "
					"otg enable control on\n");
			}
			gpio_direction_output((gp_smb->pdata).gpios->gpio_otg_en, (gp_smb->pdata).gpios->gpio_otg_en_active);
		}
	gpio_free((gp_smb->pdata).gpios->gpio_otg_en);
}

void smb137b_otg_lbr_disable(void)
{
	int err = 0;

	if (gpio_is_valid((gp_smb->pdata).gpios->gpio_otg_en)) {
			err = gpio_request((gp_smb->pdata).gpios->gpio_otg_en, "otg_enable");

			if (err) {
				printk(KERN_ERR "failed to request otg_enable for "
					"otg enable control on\n");
			}
			gpio_direction_output((gp_smb->pdata).gpios->gpio_otg_en, !(gp_smb->pdata).gpios->gpio_otg_en_active);
		}
	gpio_free((gp_smb->pdata).gpios->gpio_otg_en);
}

void enable_smb137b_lbr_mode(void)
{	
	int len = 0;
	
	smb137b_otg_lbr_disable();
#if 1
	len = smb137b_i2c_write(gp_smb->client, 0x31,0x80);	// allow write 0x0 ~ 0x09
	if (len < 0)
		printk(" write 0x31 register err ! \n");

	len = smb137b_i2c_write(gp_smb->client, 0x04,0x21);	// disable LBR timer
	if (len < 0)
		printk(" write 0x04 register err ! \n");

	len = smb137b_i2c_write(gp_smb->client, 0x06,0x58);	// set battery UVLO as 2.64V
	if (len < 0)
		printk(" write 0x06 register err ! \n");

	len = smb137b_i2c_write(gp_smb->client, 0x05,0x3B);	// enable LBR mode
	if (len < 0)
		printk(" write 0x05 register err ! \n");
	
#endif
	smb137b_charge_disable();
	smb137b_otg_lbr_enable();
}

void disable_smb137b_lbr_mode(void)
{
	smb137b_charge_enable();
	smb137b_otg_lbr_disable();
}

static void turn_on_green_led(void)
{
	int err = 0;
	
	//printk("debug: %s, %d, %s>>>>>>>>>\n", __FILE__, __LINE__, __FUNCTION__);
	if (gpio_is_valid((gp_smb->pdata).gpios->gpio_green_led)) {
			err = gpio_request((gp_smb->pdata).gpios->gpio_green_led, "green_led");

			if (err) {
				printk(KERN_ERR "failed to request green_led for "
					"green led control on\n");
			}
			gpio_direction_output((gp_smb->pdata).gpios->gpio_green_led, (gp_smb->pdata).gpios->gpio_green_active);
		}
	gpio_free((gp_smb->pdata).gpios->gpio_green_led);
}

static void turn_off_green_led(void)
{	
	int err = 0;
	
	
	//printk("debug: %s, %d, %s>>>>>>>>>\n", __FILE__, __LINE__, __FUNCTION__);
	if (gpio_is_valid((gp_smb->pdata).gpios->gpio_green_led)) {
			err = gpio_request((gp_smb->pdata).gpios->gpio_green_led, "green_led");

			if (err) {
				printk(KERN_ERR "failed to request green_led for "
					"green led control off\n");
			}
			gpio_direction_output((gp_smb->pdata).gpios->gpio_green_led, !(gp_smb->pdata).gpios->gpio_green_active);
		}
	gpio_free((gp_smb->pdata).gpios->gpio_green_led);
}

static void turn_on_red_led(void)
{
	int err = 0;
	
	if (gpio_is_valid((gp_smb->pdata).gpios->gpio_red_led)) {
			err = gpio_request((gp_smb->pdata).gpios->gpio_red_led, "red led gpio");

			if (err) {
				printk(KERN_ERR "failed to request red_led for "
					"red led control on\n");
			}
			gpio_direction_output((gp_smb->pdata).gpios->gpio_red_led, (gp_smb->pdata).gpios->gpio_red_active);
		}
	gpio_free((gp_smb->pdata).gpios->gpio_red_led);
}

static void turn_off_red_led(void)
{	
	int err = 0;

	
	//printk("debug: %s, %d, %s>>>>>>>>>\n", __FILE__, __LINE__, __FUNCTION__);
	if (gpio_is_valid((gp_smb->pdata).gpios->gpio_red_led)) {
			err = gpio_request((gp_smb->pdata).gpios->gpio_red_led, "red led gpio");

			if (err) {
				printk(KERN_ERR "failed to request red_led for "
					"red led control off\n");
			}
			gpio_direction_output((gp_smb->pdata).gpios->gpio_red_led, !(gp_smb->pdata).gpios->gpio_red_active);
		}
	gpio_free((gp_smb->pdata).gpios->gpio_red_led);
}

static void charging_led_indicate(void)
{
	turn_on_red_led();
//	turn_on_green_led();
}

void bat_full_led_indicate(void)
{
	
	//printk("debug: %s, %d, %s>>>>>>>>>\n", __FILE__, __LINE__, __FUNCTION__);
	turn_on_green_led();
	turn_off_red_led();
}
//EXPORT_SYMBOL_GPL(bat_full_led_indicate);

static void stop_charge_led_indicate(void)
{
	
	//printk("debug: %s, %d, %s>>>>>>>>>\n", __FILE__, __LINE__, __FUNCTION__);
	turn_off_green_led();
	turn_off_red_led();
}

	
static void smb137b_irq_clear(void)
{
	int len = 0;
	unsigned char val = 0xFF;

	len = smb137b_i2c_write(gp_smb->client, 0x30, val);
	if (len < 0)
	{
		printk("write 0x30 regiter err!!\n");
	}

}

/*
 * when battery under voltage lower than 3.08V ,return 1, else return 0;
 */
static int smb137b_check_battery_UVLO(void)
{
	int len = 0;
	unsigned char val = 0;
	
	len = smb137b_i2c_read(gp_smb->client, 0x35, &val);
	if (len < 0)
	{
		printk("read 0x35 err!!\n");
	}
	val = val & 0x1;
	if (val == 1) 
	{
		return 1;
	} else 
		return 0;


}

extern void s3c_cable_check_status(int flag);
/*
 * do charge irq treat, show LED as charge status;
 */


int early_suspend_stat = 0;
EXPORT_SYMBOL(early_suspend_stat);
extern int sd_early_suspend;
extern void SendPowerbuttonEvent(void);
//static struct wake_lock smb137b_lock;
static struct timer_list powerbutton_event_timer;
volatile int usb_otg_level = 0; 
#define USB_OTG_GPIO		        S5PV210_GPH0(6)	
int old_usb_stat = 0;
int old_dc_stat = 0;
int new_usb_stat = 0;
int new_dc_stat = 0;

static void powerbutton_event_function(unsigned long data)
{
		//printk("debug: %s, %d, %s>>>>>>>>>\n", __FILE__, __LINE__, __FUNCTION__);
		SendPowerbuttonEvent();
		early_suspend_stat = 0;
}

extern unsigned int wakeup_state;
static void smb137b_work(struct work_struct *work)
{
	int charge_status = 0;
	int dc_status = 0;
	
	int value = gpio_get_value(S5PV210_GPH3(2));
	if(!value && (wakeup_state == 2))
		goto clear_irq;

		new_usb_stat = smb137b_check_usb_status();
		new_dc_stat = smb137b_check_dc_status();
	

	
#if 1	
		if(early_suspend_stat && !old_usb_stat && old_dc_stat && new_usb_stat && new_usb_stat)
		{
			//printk("debug: %s, %d, %s>>>>>>>>>\n", __FILE__, __LINE__, __FUNCTION__);
			early_suspend_stat = 0;
			mod_timer(&powerbutton_event_timer, jiffies + msecs_to_jiffies(1000));
			//enable_irq(gp_smb->client->irq);
			//return IRQ_HANDLED;
		}
		else if(early_suspend_stat)
		{
			//printk("debug: %s, %d, %s>>>>>>>>>\n", __FILE__, __LINE__, __FUNCTION__);
			SendPowerbuttonEvent();
			early_suspend_stat = 0;
			//enable_irq(gp_smb->client->irq);
			//return IRQ_HANDLED;
		}
	
#endif
		//printk("debug: %s, %d, %s>>>>>>>>>\n", __FILE__, __LINE__, __FUNCTION__);




	dc_status = smb137b_check_dc_or_usb_status();

	charge_status = smb137b_get_charge_status();
#if 0	
	if (smb137b_check_usb_or_dc() == 0 && set_current_limmit_float == 1)
	{
		smb137b_current_USB500_enable(); 	// set current limmit pin to high
	}
#endif

	if (smb137b_check_dc_status() == 1)
	{
		smb137b_i2c_write(gp_smb->client, 0x31, 0xAC);
		smb137b_i2c_write(gp_smb->client, 0x05, 0x8B);
		smb137b_i2c_write(gp_smb->client, 0x00, 0xFA);
		smb137b_i2c_write(gp_smb->client, 0x01, 0xE7);
	}
	else
	{
		smb137b_i2c_write(gp_smb->client, 0x31, 0xA8);
		smb137b_i2c_write(gp_smb->client, 0x05, 0x8B);
	}

	if (smb137b_check_battery_UVLO())
	{
		if (dc_status != 0)
			charge_status = 0x2;
		else if (dc_status == 0) 
		{	
			charge_status = 0x4;
			smb137b_i2c_write(gp_smb->client, 0x31, 0x80);	// allow write config register
			smb137b_i2c_write(gp_smb->client, 0x07, 0x21);	// disable UVLO battery irq
		}
		
	}
#if defined CONFIG_PRODUCT_V7E || CONFIG_PRODUCT_KS8
	if(dc_status==0){
		stop_charge_led_indicate();
	}else{	
#endif
	if (dc_status != 0 && charge_status != 0x1)
		charge_status = 0x2;

	//printk("Tony:dc_ststus:%d,charge_status:%d\n",dc_status,charge_status);
#if 1
	switch (charge_status){
		
		case 0x4:
			break;
		case 0x3:
			stop_charge_led_indicate();
			printk("battery is not charging\n");
			break;
		case 0x2:
			stop_charge_led_indicate();
			#if defined CONFIG_PRODUCT_V7E || CONFIG_PRODUCT_KS8
			charging_led_indicate();
			#endif
			printk("battery is charging\n");
			break;
		case 0x1:
			stop_charge_led_indicate();
			#if defined CONFIG_PRODUCT_V7E || CONFIG_PRODUCT_KS8 
			bat_full_led_indicate();
			#endif
			printk("battery is full\n");
			break;
	}
#endif
#if defined CONFIG_PRODUCT_V7E || CONFIG_PRODUCT_KS8
	}
#endif
clear_irq:	smb137b_irq_clear(); // clear irq
	enable_irq(gp_smb->client->irq);
}
EXPORT_SYMBOL_GPL(smb137b_work);


static irqreturn_t smb137b_irq(int irq, void *handle)  
{
	struct smb137b_data *ac = handle;
	disable_irq_nosync(irq);

	
	schedule_work(&ac->work);

	return IRQ_HANDLED;
}

//Johnson add start for battery
int set_battery_capacity(int k)
	{
		int i=0;
		int charge_status;
        charge_status=smb137b_get_charge_status();
		if(k){
			if(charge_status==0x1)
				i=100;			             			
			else
				i=99;
		}
		return i;
	}
EXPORT_SYMBOL_GPL(set_battery_capacity);  
//Johnson add end for battery
/*
 * get charge status , 0x1: full , 0x2: charging , 3: not charge 
 */
int smb137b_get_charge_status(void)
{
	int len = 0;
	unsigned char val = 0;
	int charge_status = 0;
	len = smb137b_i2c_read(gp_smb->client, 0x36, &val);
	if (len < 0)
	{
		printk("read 0x36 err!!\n");
	}
	len = (val >> 1) & 0x3;

	switch (len)
	{
	case 0:
		charge_status = 0x3;
		break;
	case 1:
		charge_status = 0x2;
		break;
	case 2:
		charge_status = 0x2;
		break;
	case 3:
		charge_status = 0x2;
		break;
	}
#ifdef CONFIG_VENUS_TOUCHSCREEN_MG8698S 
	charge_status_touch = charge_status;
#endif

	val = (val >> 6) & 0x1;
	if (val == 1 || bat_level == 100)
		charge_status = 0x1;
#if 1	
	if (smb137b_get_battery_present() == 0)
	{
		charge_status = 0x3;
	}
#endif
	return charge_status;
}
EXPORT_SYMBOL_GPL(smb137b_get_charge_status);


/*
 * get battery status , 1: present , 0: not present
 */
int smb137b_get_battery_present(void)
{
	int len = 0;
	unsigned char val = 0;
	len = smb137b_i2c_read(gp_smb->client, 0x37, &val);
	if (len < 0)
	{
		printk("read 0x37 err!!\n");
	}
	val = (val >> 0) & 0x1;

	return !val;
}
EXPORT_SYMBOL_GPL(smb137b_get_battery_present);


int smb137b_check_dc_or_usb_status(void)
{
	int len = 0;
	if (smb137b_check_dc_status())
		len = 2;
	else if (smb137b_check_usb_status())
		len = 1;
	else
		len = 0;
	smb137b_get_dc_status = len;
#if 0										// LBR mode to support low battery status			
	if (smb137b_get_dc_status == 0)
	{
		if (smb137b_lbr_mode == 1 && smb137b_lbr_state <= 10)
		{
		//	printk(" ***get into LBR mode !***\n");
			enable_smb137b_lbr_mode();
			smb137b_lbr_state += 1;
		}

	} else if (smb137b_lbr_state > 1) {
		//printk(" ***get out LBR mode !***\n");
		disable_smb137b_lbr_mode();
		smb137b_lbr_state = 0;
	}
#endif
	s3c_cable_check_status(len);				// update /sys dc status

	return len;
}
   
EXPORT_SYMBOL_GPL(smb137b_check_dc_or_usb_status);

/*
 * get DC status , 1: present , 0: not present
 */
int smb137b_check_dc_status(void)
{
	int len = 0;
	unsigned char val = 0;
	len = smb137b_i2c_read(gp_smb->client, 0x33, &val);
	if (len < 0)
	{
		printk("read 0x33 err!!\n");
	}
	val = (val >> 2)& 0x1;

	return !val;
}

/*
 * get USB status , 1: present , 0: not present
 */
int smb137b_check_usb_status(void)
{
	int len = 0;
	unsigned char val = 0;
	len = smb137b_i2c_read(gp_smb->client, 0x33, &val);
	if (len < 0)
	{
		printk("read 0x33 err!!\n");
	}
	val = (val >> 0)& 0x1;

	return !val;
}

/* sysfs support */
// show smb137b device id

static ssize_t smb137b_show_smb137b_id(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int len = 0;
	unsigned char val = 0;

	len = smb137b_i2c_read(gp_smb->client, 0x3B, &val);
	if (len < 0)
	{
		printk("read 0x3B err!!\n");
	}
	
	return sprintf(buf, "%x\n", val);
}

static DEVICE_ATTR(smb137b_id, S_IRUGO,
		    smb137b_show_smb137b_id , NULL);


static ssize_t smb137b_show_smb137b_00(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int len = 0;
	unsigned char val = 0;

	len = smb137b_i2c_read(gp_smb->client, 0x00, &val);

	if (len < 0)
	{
		printk("read 0x00 err!!\n");
	}
	return sprintf(buf, "%x\n", val);
}

static DEVICE_ATTR(smb137b_00, S_IRUGO,
		    smb137b_show_smb137b_00 , NULL);


static ssize_t smb137b_show_smb137b_01(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int len = 0;
	unsigned char val = 0;

	len = smb137b_i2c_read(gp_smb->client, 0x01, &val);
	if (len < 0)
	{
		printk("read 0x01 err!!\n");
	}

	return sprintf(buf, "%x\n", val);
}

static DEVICE_ATTR(smb137b_01, S_IRUGO,
		    smb137b_show_smb137b_01 , NULL);



static ssize_t smb137b_store_smb137b_register(struct device *dev,
                struct device_attribute *attr, const char *buf, size_t count)
{	
	int len = 0;
	unsigned char val = 0;
        unsigned int reg = simple_strtoul(buf, NULL, 16);
        len = smb137b_i2c_read(gp_smb->client, reg, &val);
    	if (len < 0)
	{
		printk("read 0x%x\n err!!\n",reg);
	}

	printk("register 0x%x value is %x\n", reg, val);

        return count;
}



static DEVICE_ATTR(smb137b_register, S_IWUSR | S_IRUGO,
		    NULL, smb137b_store_smb137b_register);

static ssize_t smb137b_store_red_led(struct device *dev,
                struct device_attribute *attr, const char *buf, size_t count)
{	
        unsigned int val = simple_strtoul(buf, NULL, 16);
	if (val == 1)
		turn_on_red_led();
	else if(val == 0)
		turn_off_red_led();
       
        return count;
}

static DEVICE_ATTR(red_led, S_IWUSR | S_IRUGO,
		    NULL, smb137b_store_red_led);


static ssize_t smb137b_store_green_led(struct device *dev,
                struct device_attribute *attr, const char *buf, size_t count)
{	
	//printk("debug: %s, %d, %s>>>>>>>>>\n", __FILE__, __LINE__, __FUNCTION__);

        unsigned int val = simple_strtoul(buf, NULL, 16);
	if (val == 1)
		turn_on_green_led();
	else if(val == 0)
		turn_off_green_led();
       
        return count;
}

static DEVICE_ATTR(green_led, S_IWUSR | S_IRUGO,
		    NULL, smb137b_store_green_led);


static struct attribute *smb137b_attributes[] = {
	&dev_attr_smb137b_id.attr,
	&dev_attr_smb137b_00.attr,
	&dev_attr_smb137b_01.attr,
	&dev_attr_smb137b_register.attr,
	&dev_attr_red_led.attr,
	&dev_attr_green_led.attr,
	NULL
};


static const struct attribute_group smb137b_attr_group = {
	.attrs = smb137b_attributes,
};

/*
 * check usb or DC ,bit5, 0:USB, 1:DC
 */
int smb137b_check_usb_or_dc(void)
{	
	int len;
	unsigned char val = 0;

	len = smb137b_i2c_read(gp_smb->client, 0x33, &val);
	if (len < 0)
	{
		printk("read 0x33 err!!\n");
		return -1;
	}
	val = (val >> 5) & 0x1;

	return val;
	
}
EXPORT_SYMBOL_GPL(smb137b_check_usb_or_dc);



/*
 * Initialization function
 */

static int smb137b_init_client(struct i2c_client *client,struct smb137b_data *ac)
{

	struct charge_platform_data *devpd = client->dev.platform_data;
	struct charge_platform_data *pdata;
	int err;
 	
 	printk(KERN_ALERT "smb137b_initialize\n");
	
	if (!client->irq) 
	{
		printk(KERN_ALERT "%s no IRQ?\n",__func__);
		return -ENODEV;
	}

	if (!devpd) 
	{
		printk(KERN_ALERT "%s No platfrom data: Using default initialization\n",__func__);
	}

	memcpy(&ac->pdata, devpd, sizeof(ac->pdata));
	pdata = &ac->pdata;
	
	if (pdata->init)
	{
		pdata->init();
	}

	INIT_WORK(&ac->work, smb137b_work);
	mutex_init(&ac->mutex);

	printk("smb137b irq is %d \n", client->irq);
	err = request_irq(client->irq, smb137b_irq, IRQ_TYPE_EDGE_FALLING, client->dev.driver->name, ac);
	if (err) 
	{
		printk( "irq %d busy?\n", client->irq);
		goto err_free_irq;
	}

	err = sysfs_create_group(&client->dev.kobj, &smb137b_attr_group);
	if (err)
		goto err_remove_attr;

	/* for charging timeout issue and charging temperature over issue, do some configure change */
	if (smb137b_check_usb_or_dc())
	{
	//	printk("***HC Mode change current limmit pin to float *** \n");
	//	smb137b_current_HC_enable();	// set current limmit pin to float
		AC_WRITE(ac, 0x31, 0x80); // allow write 0x0 ~ 0x09
		AC_WRITE(ac, 0x08, 0x25); // change high temperature inhibit from 40 to 55
		AC_WRITE(ac, 0x09, 0x2C); // change complete charge timeout from 382min to disable 

	} else {
	//	printk("***USB Mode no need control current limmit pin *** \n");
		AC_WRITE(ac, 0x31, 0x80); // allow write 0x0 ~ 0x09
		AC_WRITE(ac, 0x08, 0x25); // change high temperature inhibit from 40 to 55
		AC_WRITE(ac, 0x09, 0x2C); // change complete charge timeout from 382min to disable 
	}
	
	return 0;

err_remove_attr:
	sysfs_remove_group(&client->dev.kobj, &smb137b_attr_group);
err_free_irq:
	free_irq(client->irq, ac);

	return err;
}

/*
 * I2C init/probing/exit functions
 */
#if 1
static void smb137b_early_suspend(struct early_suspend *h)
{
	int charge_status;
	early_suspend_stat = 1;
	sd_early_suspend=1;
	old_usb_stat = smb137b_check_usb_status();
	old_dc_stat = smb137b_check_dc_status();

	if (smb137b_check_dc_or_usb_status() != 0) {

		charge_status = smb137b_get_charge_status();
		switch (charge_status){

	 		case 0x3:
			stop_charge_led_indicate();
			printk("battery is not charging\n");
			break;
			case 0x2:
			charging_led_indicate();
			printk("battery is charging\n");
			break;
			case 0x1:
			// printk("%s, %d, %s>>>>>>>>>>>>>>>\n", __FILE__, __LINE__, __FUNCTION__);	
			bat_full_led_indicate();
			printk("battery is full\n");
			break;
		}
	}
	return ;
}

static void smb137b_late_resume(struct early_suspend *h)
{
	
	 early_suspend_stat = 0;
	sd_early_suspend=0;
	#if !(defined(CONFIG_PRODUCT_V7E) || defined(CONFIG_PRODUCT_KS8)) 
	stop_charge_led_indicate();
	#endif
	return ;
}
#endif
//static struct i2c_driver smb137b_driver;


static int __devexit smb137b_remove(struct i2c_client *client)
{
	sysfs_remove_group(&client->dev.kobj, &smb137b_attr_group);

	/* Power down the device */
	
	kfree(i2c_get_clientdata(client));

//release_power(HDMI_POWER);
//release_power(CAM_2_8V);
	return 0;
}

#ifdef CONFIG_PM
#if 1
static int smb137b_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int charge_status = 0;
	// printk("debug: %s, %d, %s>>>>>>>>>\n", __FILE__, __LINE__, __FUNCTION__);
	disable_irq(client->irq);
	cancel_work_sync(&gp_smb->work);

/* the same code in smb137b_early_suspend(), but must reserve this code here, because when battery
    is  full in suspend state, smb137b_suspend() was invoked, but smb137b_early_suspend() won't be */
#if 1 
		if (smb137b_check_dc_or_usb_status() != 0) {

		charge_status = smb137b_get_charge_status();

		switch (charge_status){

			case 0x3:
			stop_charge_led_indicate();
			printk("battery is not charging\n");
			break;
			case 0x2:
			charging_led_indicate();
			printk("battery is charging\n");
			break;
			case 0x1:
			bat_full_led_indicate();
			printk("battery is full\n");
			break;
		}
	}
#endif
//release_power(HDMI_POWER);
//release_power(CAM_2_8V);
	return 0;
}

static int smb137b_resume(struct i2c_client *client)
{
	//int charge_status = 0;
	// printk("debug: %s, %d, %s>>>>>>>>>\n", __FILE__, __LINE__, __FUNCTION__);

//request_power(CAM_2_8V);
//request_power(HDMI_POWER);

	enable_irq(client->irq);	//  add enable gpn8 irq
	smb137b_check_dc_or_usb_status();
	#if !(defined(CONFIG_PRODUCT_V7E) || defined(CONFIG_PRODUCT_KS8)) 
	stop_charge_led_indicate();
	#endif
#if 0
	if (smb137b_check_dc_or_usb_status() != 0) {

		charge_status = smb137b_get_charge_status();

		switch (charge_status){

			case 0x3:
			stop_charge_led_indicate();
			printk("battery is not charging\n");
			break;
			case 0x2:
			charging_led_indicate();
			printk("battery is charging\n");
			break;
			case 0x1:
			bat_full_led_indicate();
			printk("battery is full\n");
			break;
		}
	}
#endif
	smb137b_irq_clear(); // clear irq

	return 0;
}
#endif

#else

#define smb137b_suspend		NULL
#define smb137b_resume		NULL

#endif /* CONFIG_PM */

static int __devinit smb137b_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct smb137b_data *data;

	int  err = 0;
//request_power(CAM_2_8V);
//request_power(HDMI_POWER);
	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE)) {
		err = -EIO;
		goto exit;
	}

	data = kzalloc(sizeof(struct smb137b_data), GFP_KERNEL);
	if (!data) {
		err = -ENOMEM;
		goto exit;
	}
	priv = data;
	data->client = client;
	data->read_block = smb137b_i2c_read_block_data;
	data->read = smb137b_i2c_read;
	data->write = smb137b_i2c_write;
	i2c_set_clientdata(client, data); //client->dev.driver_data = ac;

	gp_smb = data;
	/* Initialize the smb137b chip */
	err = smb137b_init_client(client,data);
	smb137b_check_dc_or_usb_status();
	if (err)
		goto exit_kfree;
#if 1
#ifdef CONFIG_HAS_EARLYSUSPEND
	e_s.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	e_s.suspend = smb137b_early_suspend;
	e_s.resume = smb137b_late_resume;
	register_early_suspend(&e_s);
#endif
#endif
	#if !(defined(CONFIG_PRODUCT_V7E) || defined(CONFIG_PRODUCT_KS8)) 
	stop_charge_led_indicate();
	#endif
	setup_timer(&powerbutton_event_timer, powerbutton_event_function, 0);
	return 0;

exit_kfree:
	kfree(data);
exit:
	return err;
}


static const struct i2c_device_id smb137b_id[] = {
	{ "smb137b", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, smb137b_id);

static struct i2c_driver smb137b_driver = {
	.driver = {
		.name	= SMB137B_DRV_NAME,
		.owner	= THIS_MODULE,
	},
	.suspend = smb137b_suspend,
	.resume	= smb137b_resume,
	.probe	= smb137b_probe,
	.remove	= __devexit_p(smb137b_remove),
	.id_table = smb137b_id,
};

static int __init smb137b_init(void)
{
	return i2c_add_driver(&smb137b_driver);
}

static void __exit smb137b_exit(void)
{
	i2c_del_driver(&smb137b_driver);
}

MODULE_AUTHOR("jimmy teng");
MODULE_DESCRIPTION("SMB137B Charge IC driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);

rootfs_initcall(smb137b_init);
//subsys_initcall(smb137b_init);
//module_init(smb137b_init);
module_exit(smb137b_exit); 

