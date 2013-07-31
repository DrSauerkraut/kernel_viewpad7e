/*
 *Max17044  battery driver
 *
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#include <linux/module.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/idr.h>
#include <linux/i2c.h>
#include <asm/unaligned.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#include <plat/gpio-cfg.h>
#include <linux/gpio.h>
#include <linux/io.h>

//#include <plat/regs-gpio.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <linux/venus/battery_core.h>

#define DRIVER_VERSION			"0.0.0"
/* MAX17044 register address */
#define MAX17044_REG_VOLT		0x02 //report 12bit A/D measurement of battery voltage
#define MAX17044_REG_RSOC		0x04 //Relative State-of-Charge 
#define MAX17044_REG_MODE		0x06 //sent special commands to the IC
#define MAX17044_VERSION		0x08 //return IC version
#define MAX17044_REG_CONFIG		0x0C //battery compensation
#define MAX17044_REG_COMMAND		0xFE //sent special commands to the IC
#define MAX17040_DELAY		1000
/*CONFIG register command*/
#define SLEEP  	0x9794  // sleep bit set to 1 ,IC get into sleep
#define WAKE	0x9714  //sleep bit set to 0 ,IC wake up
//#define ATHD	0x9714 // set  10% percent of battery as low battery alert

/*COMMAND register command */
#define POR 	0x5400 // IC power on reset
#define STR	0x4000 // IC quick start

extern int smb137b_lbr_mode;	// jimmy add for smb137b_lbr mode set
extern int smb137b_check_dc_status(void);
/* If the system has several batteries we need a different name for each
 * of them...
 */
static DEFINE_MUTEX(battery_mutex);

struct max17044_device_info *battery;
struct max17044_access_methods {
	int (*read)(u8 reg, int *rt_value, 
		struct max17044_device_info *di);

	int (*write)(u8 reg, int wt_value,
		struct i2c_client *client);
};

struct max17044_device_info {
	struct device 		*dev;
	int			id;
	int			voltage_uV;
	int			charge_rsoc;
	struct max17044_access_methods	*bus;
	struct power_supply	bat;
	struct i2c_client	*client;

//	struct workqueue_struct *monitor_wqueue;
	struct delayed_work monitor_work;
};

static int max17044_write_code(u8 reg,int wt_value,
			 struct i2c_client *client);

static int max17044_read_code_burn(u8 reg, char *rt_value,
			struct i2c_client *client);

static int max17044_write_code_burn(u8 reg,char *wt_value,
			 struct i2c_client *client);

static int max17044_write_code_restore(u8 reg,char *wt_value,
			 struct i2c_client *client);


static int max17044_read(u8 reg, int *rt_value,
			struct max17044_device_info *di)
{
	int ret;
	ret = di->bus->read(reg, rt_value, di);
//	*rt_value = be16_to_cpu(*rt_value);
	return ret;
}

static int max17044_write(u8 reg, int wt_value, 
			 	struct i2c_client *client)
{	
	int ret;
	ret = max17044_write_code(reg,wt_value,client);
	return ret;
}
	
static int capacity_report(int capacity)
{	
	if(capacity <= 1)
		capacity = 0;
	else if(capacity <= 5)
	 	capacity = 5;
	else if(capacity <= 10)
		capacity = 5;
	else if(capacity <= 15)
		capacity = 10;
	else if(capacity <= 20)
		capacity = 15;
	else if(capacity <= 25)
		capacity = 20;
	else if(capacity <= 30)
		capacity = 25;
	else if(capacity <= 35)
		capacity = 30;
	else if(capacity <= 40)
		capacity = 35;
	else if(capacity <= 45)
		capacity = 40;
	else if(capacity <= 50)
		capacity = 45;
	else if(capacity <= 55)
		capacity = 50;
	else if(capacity <= 60)
	 	capacity = 55;
	else if(capacity <= 65)
		capacity = 60;
	else if(capacity <= 70)
		capacity = 65;
	else if(capacity <= 75)
		capacity = 70;
	else if(capacity <= 80)
		capacity = 75;
	else if(capacity <= 85)
		capacity = 80;
	else if(capacity <= 90)
		capacity = 85;
	else if(capacity <= 95)
		capacity = 90;
	else if(capacity <= 100)
		capacity = 100;
	else if(capacity > 100)
		capacity = 100;
	return capacity;
}


 int max17044_present(void)
{

	return 0;
}

EXPORT_SYMBOL(max17044_present);
/*
 * Return the battery Voltage in milivolts
 * Or < 0 if something fails.
 */
static int max17044_battery_voltage(struct max17044_device_info *di)
{
	int ret;
	int volt = 0;

	ret = max17044_read(MAX17044_REG_VOLT, &volt, di);
	if (ret) {
		printk( "error reading voltage and ret is %d\n",ret);
		return ret;
	}
	volt = ((volt >> 4) & 0xFFF)*125;
	return volt;
}

int get_battery_voltage(void)
{
	int vol = 0;
	vol = max17044_battery_voltage(battery);

	return vol;
}

/*
 * Return the battery Relative State-of-Charge
 * Or < 0 if something fails.
 */
static int max17044_battery_rsoc(struct max17044_device_info *di)
{
	int ret;
	int rsoc = 0;

	ret = max17044_read(MAX17044_REG_RSOC, &rsoc, di);
	if (ret) {
		printk( "error reading relative State-of-Charge\n");
	}
	rsoc = ((rsoc >> 8) & 0xff);
//	rsoc = ((rsoc >> 8) & 0xff)/2;
//rsoc = capacity_report(rsoc);
	return rsoc;
}

 int get_battery_level(void)
{
	int cap = 0;
	cap = max17044_battery_rsoc(battery);

	return cap;
}

int clear_batf(void)
{
	int ret;
	int reg;

	ret = max17044_read(0x0C, &reg, battery);
	if (ret) {
		printk( "error reading alter bit of 0xD register\n");
	}
	printk("1.reg=%x\n",reg);
	reg &=~(0x1<<5);
	ret = max17044_write(0x0C, reg, battery->client);
	ret = max17044_read(0x0C,&reg, battery);
	printk("2.reg=%x\n",reg);

	return 0;
}


#if 1
/*
 ************************ Max17044 Loading a Custom Model********************************
 */

/*1.Unlock Model Access*/
static int max17044_ulock_model(struct i2c_client *client)
{
	int ret ;
	ret = max17044_write(0x3E,0x4A57,client);
	if (ret) {
		printk( "writing unlock  Register,and ret is %d\n",ret);
		return ret;
	}
	return ret;
}

/*2.Read Original RCOMP and OCV Register*/
static int max17044_read_rccmp_ocv(struct i2c_client *client,char *rt_value)
{
	int ret;
	char rsoc[4];
	int i;
	ret = max17044_read_code_burn(0x0C,rsoc,client);
	if (ret) {
		printk( "reading rccmp and ocv  Register\n");
		return ret;
	}
	for(i=0;i<4;i++)
	{
		rt_value[i] = rsoc[i];
	}
	printk("rt_value[0] is %x and rt_value[1] is %x,and rt_value[2] is %x, and rt_value[3] is %x, \n",\
							    rt_value[0],rt_value[1],rt_value[2],rt_value[3]);
	return ret;
}

/*3.Write OCV Register*/
static int max17044_write_ocv(struct i2c_client *client)
{	
	int ret ;
	ret = max17044_write(0x0E,0xDA30,client);
	if (ret) {
		printk( "writing OCV Register,and ret is %d\n",ret);
		return ret;
	}

	return ret;
	
}

/*4.Write RCCOMP Register to a Maximum value of 0xFF00h*/
static int max17044_write_rccomp(struct i2c_client *client, int data)
{	
	int ret;
	ret = max17044_write(0x0C,data,client);
	if (ret) {
		printk( "writing rccomp Register,and ret is %d\n",ret);
		return ret;
	}
	return ret;
}

/*5.Write the Model,once the model is unlocked,
 *the host software must write the 64 byte model to the max17044.
 *the model is located between memory locations 0x40h and 0x7Fh.
 */
static int max17044_write_model(struct i2c_client *client)
{	
	int ret;
//	int i;

/*****************set 64 byte mode ******************************/ 
	char data0[] = {0x9A,0x50,0xAB,0x40,0xAC,0x20,0xB0,0x90,
			  0xB1,0x00,0xB4,0xB0,0xB4,0xF0,0xB5,0x30};

	char data1[] = {0xB5,0x50,0xB5,0x70,0xBA,0x60,0xBA,0xA0,
			  0xBB,0x10,0xC1,0xB0,0xC4,0x50,0xD0,0x10};

	char data2[] = {0x01,0x00,0x1F,0x00,0x00,0x20,0x70,0x00,
			  0x00,0xF0,0x6E,0x00,0x80,0x00,0x80,0x00};

	char data3[] = {0x80,0x00,0x01,0xF0,0x1D,0x00,0x73,0x00,
			  0x05,0xD0,0x0E,0x00,0x05,0x90,0x05,0x90};


#if 0
	for(i=0;i<16;i++)
	{	
		printk( "*********get into max17044 write model*************");
		printk( "data0[%d] is %x\n",i,data0[i]);
	}
#endif			
	ret = max17044_write_code_burn(0x40,data0,client);
	if (ret) {
		printk( "data0 writing mode,and ret is %d\n",ret);

	}

	ret = max17044_write_code_burn(0x50,data1,client);
	if (ret) {
		printk( "data1 writing mode,and ret is %d\n",ret);

	}

	ret = max17044_write_code_burn(0x60,data2,client);
	if (ret) {
		printk( "data2 writing mode,and ret is %d\n",ret);

	}

	ret = max17044_write_code_burn(0x70,data3,client);
	if (ret) {
		printk( "data3 writing mode,and ret is %d\n",ret);
	}

	return ret;
}

/*10.Restore RCOMP and OCV */
static int max17044_restore(struct i2c_client *client, char *val)
{	
	int ret;
	int i;
	char data[4] ;
	for(i=0;i<4;i++)
	{
		data[i] = val[i];
		printk( "restore data[%d] is %x\n",i,data[i]);
	}
	data[0] = 0x93;	 	// set temperture register to 0x93H 	
	data[1] = 0x00;
	ret = max17044_write_code_restore(0x0C,data,client);
	if (ret) {
//		printk( "restore writing mode,and ret is %d\n",ret);
		return ret;
	}

	return ret;
}

/*lock Model Access*/
static int max17044_lock_model(struct i2c_client *client)
{
	int ret ;
	ret = max17044_write(0x3E,0x0000,client);
	if (ret) {
//		printk( "writing lock  Register,and ret is %d\n",ret);
		return ret;
	}
	return ret;
}

/*burn mode sequence */
static void max17044_burn(struct i2c_client *client)
{	
	char rt_value[4];
	max17044_ulock_model(client);			//1.unlock model access
	max17044_read_rccmp_ocv(client, rt_value);	//2.read rcomp and ocv register
	max17044_write_ocv(client);			//3.write OCV register
	max17044_write_rccomp(client, 0xFF00);			//4.write rcomp to a maximum value of 0xff00
	max17044_write_model(client);			//5.write the model
	mdelay(200);					//6.delay at least 200ms
	max17044_write_ocv(client);			//7.write ocv register
	mdelay(200);					//8. this delay must between 150ms and 600ms
	/*9.read soc register and compare to expected result
	 *if SOC1 >= 0xF9 and SOC1 <= 0xFB then 
	 *mode was loaded successful else was not loaded successful
	 */
	max17044_restore(client, rt_value);		//10.restore rcomp and ocv
	max17044_lock_model(client);			//11.lock model access
	max17044_write_rccomp(client, 0x930c);			//4.write rcomp to a  value of 0x930c, as define lower battery thread is 20%
}


#endif

/*
 * max17044 specific code
 */
/*For normal data reading*/
static int max17044_read_code(u8 reg, int *rt_value,
			struct max17044_device_info *di)
{
	struct i2c_client *client = di->client;
	struct i2c_msg msg[1];
	unsigned char data[2];
	int err;

	if (!client->adapter)
		return -ENODEV;

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 1;
	msg->buf = data;

	data[0] = reg;
	err = i2c_transfer(client->adapter, msg, 1);

	if (err >= 0) {
		msg->len = 2;
		msg->flags = I2C_M_RD;
		err = i2c_transfer(client->adapter, msg, 1);
//		printk("data[0] is %x and data[1] is %x \n",data[0],data[1]);
		if (err >= 0) {		
			*rt_value = get_unaligned_be16(data);
//		printk(" get_unaligned_be16(data) is %x \n",*rt_value);
			return 0;
		}
	}
	return err;
}


/*For mode burn reading*/
static int max17044_read_code_burn(u8 reg, char *rt_value,
			struct i2c_client *client)
{
//	struct i2c_client *client = di->client;
	struct i2c_msg msg[1];
	unsigned char data[4];
	int err;
	int i;
	if (!client->adapter)
		return -ENODEV;

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 1;
	msg->buf = data;

	data[0] = reg;
	err = i2c_transfer(client->adapter, msg, 1);

	if (err >= 0) {
		msg->len = 4;
		msg->flags = I2C_M_RD;
		err = i2c_transfer(client->adapter, msg, 1);
		printk("data[0] is %x and data[1] is %x,and data[2] is %x, and data[3] is %x, \n",data[0],data[1],data[2],data[3]);
		if (err >= 0) {		
		//	*rt_value = get_unaligned_be16(data);
		//printk(" get_unaligned_be16(data) is %x \n",*rt_value);
			for(i = 0;i<4;i++)
			{
				rt_value[i]=data[i];
			}
		
			return 0;
		}
	}
	return err;
}

/*For normal data writing*/
static int max17044_write_code(u8 reg,int wt_value,
			 struct i2c_client *client)
{
	struct i2c_msg msg[1];
	unsigned char data[3];
	int err;
	
	if (!client->adapter)
		return -ENODEV;

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 3;
	msg->buf = data;

	data[0] = reg;
	data[1] = wt_value >> 8;
	data[2] = wt_value & 0xFF;
	err = i2c_transfer(client->adapter, msg, 1);
	return err;
}

/*For mode burn writing*/
static int max17044_write_code_burn(u8 reg,char *wt_value,
			 struct i2c_client *client)
{
	struct i2c_msg msg[1];
	unsigned char data[17];
	int err;
	int i;
	
	if (!client->adapter)
		return -ENODEV;

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 17;
	msg->buf = data;

	data[0] = reg;
	for(i=1;i<17;i++)
	{
		data[i]=wt_value[i-1];
	}
#if 0	
	for(i=0;i<17;i++)
	{	
		printk( "*********get into max17044 write code burn*************");
		printk( "data[%d] is %x\n",i,data[i]);
	}
#endif
	err = i2c_transfer(client->adapter, msg, 1);
	return err;
}

/*For mode burn restore*/
static int max17044_write_code_restore(u8 reg,char *wt_value,
			 struct i2c_client *client)
{
	struct i2c_msg msg[1];
	unsigned char data[5];
	int err;
	int i;
	
	if (!client->adapter)
		return -ENODEV;

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 5;
	msg->buf = data;

	data[0] = reg;
	for(i=1;i<5;i++)
	{
		data[i]=wt_value[i-1];
		printk( "*****get into write code restore and data[%d] is %x\n",i,data[i]);
	}	
	err = i2c_transfer(client->adapter, msg, 1);
	return err;
}

static int max17044_suspend(void)
{
	clear_batf();

	return 0;
}

static int max17044_resume(void)
{
	clear_batf();
	return 0;
}

static int max17044_battery_remove(void)
{
	kfree(battery);
	return 0;
}

static irqreturn_t max1704x_irq(int irq, void *handle)  
{

	return IRQ_HANDLED;
}

static int max17044_battery_probe(struct i2c_client *client,
				 const struct i2c_device_id *id)
{	
	struct max17044_device_info *di;
	struct max17044_access_methods *bus;
	struct battery_platform_data  platdata;  // jimmy add for battery register
	struct battery_platform_data  *bat_platdata;
	int err = 0;
	int retval = 0;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di) {
		dev_err(&client->dev, "failed to allocate device info data\n");
		retval = -ENOMEM;
		goto batt_failed_2;
	}

	bus = kzalloc(sizeof(*bus), GFP_KERNEL);
	if (!bus) {
		dev_err(&client->dev, "failed to allocate access method "
					"data\n");
		retval = -ENOMEM;
		goto batt_failed_3;
	}

	i2c_set_clientdata(client, di);
	di->dev = &client->dev;
	bus->read = &max17044_read_code;
	bus->write = &max17044_write_code;
	di->bus = bus;
	di->client = client;
	
	printk("max1704x low battery irq is %d \n", client->irq);
	err = request_irq(client->irq, max1704x_irq, IRQ_TYPE_EDGE_FALLING, client->dev.driver->name, di);
	if (err) 
	{
		printk( "irq %d busy?\n", client->irq);
	}

	bat_platdata = kzalloc(sizeof(platdata), GFP_KERNEL);
	if (!bat_platdata) {
		dev_err(&client->dev, "failed to allocate battery platform  "
					"data\n");
		retval = -ENOMEM;
		goto batt_failed_4;
	}
	bat_platdata->battery_level 	= &get_battery_level;
	bat_platdata->battery_voltage 	= &get_battery_voltage;
	bat_platdata->bat_suspend	= &max17044_suspend;
	bat_platdata->bat_resume	= &max17044_resume;
	bat_platdata->bat_remove	= &max17044_battery_remove;

	max17044_burn(client); // burn a new customer mode

	battery = di;
	battery_register(bat_platdata);


	return 0;

batt_failed_4:
	kfree(bat_platdata);
batt_failed_3:
	kfree(bus);
batt_failed_2:
	kfree(di);

	return retval;
}

/*
 * Module stuff
 */

static const struct i2c_device_id max17044_id[] = {
	{ "max17044", 0 },
	{},
};

static struct i2c_driver max17044_battery_driver = {
	.driver = {
		.name = "max17044-battery",
	},
	.probe = max17044_battery_probe,
//	.remove = max17044_battery_remove,
//	.suspend= max17044_suspend,
//      .resume = max17044_resume,
	.id_table = max17044_id,
};

static int __init max17044_battery_init(void)
{
	int ret;
	int present = 0;
 	present = max17044_present();
	if(present == 1) {
		return 0;
	}

	ret = i2c_add_driver(&max17044_battery_driver);
	if (ret)
		printk(KERN_ERR "Unable to register max17044 driver\n");

	return ret;
}
module_init(max17044_battery_init);

static void __exit max17044_battery_exit(void)
{
	int present = 0;
 	present = max17044_present();
	if(present == 1) {
		return;
	}
	i2c_del_driver(&max17044_battery_driver);
}
module_exit(max17044_battery_exit);
MODULE_AUTHOR("jimmy teng");
MODULE_DESCRIPTION("MAX17044 battery monitor driver");
MODULE_LICENSE("GPL");
