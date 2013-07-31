/* drivers/input/touchscreen/pixcir_i2c_ts.c
 *
 * Copyright (C) 2010 Pixcir, Inc.
 *
 * pixcir_i2c_ts.c V1.0  support multi touch
 * pixcir_i2c_ts.c V2.0  add tuning function including follows function:
 *
 * CALIBRATION_FLAG	1
 * NORMAL_MODE		2
 * DEBUG_MODE		3
 * INTERNAL_MODE	4
 * RASTER_MODE		5
 * VERSION_FLAG		6
 * BOOTLOADER_MODE	7
 * EE_SLAVE_READ
 * RAM_SLAVE_READ
 * POWER
 * WRITE_EE2SLAVE
 * SET_OFFSET_FLAG
 * CLEAR_SPECOP
 * CRC_FLAG
 * RASTER_MODE_1
 * RASTER_MODE_2
 * READ_XN_YN_FLAG
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <asm/uaccess.h>
#include <linux/smp_lock.h>
#include <linux/delay.h>
#include <linux/slab.h> 
#include <linux/proc_fs.h>
#include <linux/gpio.h>
#include <linux/earlysuspend.h>
#include <linux/venus/power_control.h>
//#include <linux/wakelock.h>

#include "pixcir_i2c_ts.h"

#define  DRIVER_VERSION "v2.0"
#define  DRIVER_AUTHOR "Bee<http://www.pixcir.com.cn>"
#define  DRIVER_DESC "Pixcir I2C Touchscreen Driver with tune fuction"
#define  DRIVER_LICENSE "GPL"

#define  PIXCIR_DEBUG   0

#define  PROC_CMD_LEN   20

#define  TWO_POINTS

/*********************************V2.0-Bee-0928-TOP****************************************/

#define  SLAVE_ADDR		0x5c
#define  BOOTLOADER_ADDR		0x5d

#ifndef  I2C_MAJOR
#define  I2C_MAJOR 		125
#endif

#define  I2C_MINORS 		256

#define  CALIBRATION_FLAG	1
#define  NORMAL_MODE		8
#define  PIXCIR_DEBUG_MODE		3
#define  INTERNAL_MODE		4
#define  RASTER_MODE		5
#define  VERSION_FLAG		6
#define  BOOTLOADER_MODE	7

#define  ENABLE_IRQ         10
#define  DISABLE_IRQ        11

#define  INT_PER		0x08
#define  INT_MOV		0x09
#define  INT_FIN		0x0a

#define IDLE_PERIOD		0x50

#define ACTIVE_MODE     0x00
#define SLEEP_MODE 		0x01
#define DEEP_SLEEP_MODE 0x02
#define FREEZE_MODE		0x03

#define AUTO_ACTIVE_MODE    	(IDLE_PERIOD | 0x04)
#define AUTO_SLEEP_MODE 		(IDLE_PERIOD | 0x05)
#define AUTO_DEEP_SLEEP_MODE 	(IDLE_PERIOD | 0x06)
#define AUTO_FREEZE_MODE		(IDLE_PERIOD | 0x07)

#ifdef  R8C_AUO_I2C	
  #define SPECOP		0x78
#else
  #define SPECOP		0x37
#endif

#define reset

#define PIXCIR_OFF   0
#define PIXCIR_ON    1

#define POWER_OFF    0
#define POWER_ON     1

#define NO_SUSPEND_CMD     0
#define HAVE_SUSPEND_CMD   1

static unsigned char pixcir_state = PIXCIR_ON;
static unsigned char power_state = POWER_ON;
static unsigned char suspend_cmd_state = NO_SUSPEND_CMD;
static unsigned char suspend_cmd_store;

int global_irq;

static unsigned char status_reg = 0;
unsigned char read_XN_YN_flag = 0;

unsigned char global_touching, global_oldtouching;
unsigned char global_posx1_low, global_posx1_high, global_posy1_low,
		global_posy1_high, global_posx2_low, global_posx2_high,
		global_posy2_low, global_posy2_high;

unsigned char Tango_number;

unsigned char interrupt_flag = 0;

unsigned char x_nb_electrodes = 0;
unsigned char y_nb_electrodes = 0;
unsigned char x2_nb_electrodes = 0;
unsigned char x1_x2_nb_electrodes = 0;

signed char xy_raw1[67];
signed char xy_raw2[64];
signed char xy_raw12[131];

#ifdef CONFIG_HAS_EARLYSUSPEND
struct early_suspend es;
static void pixcir_early_suspend(struct early_suspend *h);
static void pixcir_late_resume(struct early_suspend *h);
#endif	

void pixcir_ts_reset(void);

struct i2c_dev
{
	struct list_head list;
	struct i2c_adapter *adap;
	struct device *dev;
};

struct pixcir_i2c_ts_data
{
	struct i2c_client *client;
	struct input_dev *input;
	struct delayed_work work;
	struct timer_list timer;
	//struct wake_lock pixcir_wake_lock;
	int irq;
};

static struct workqueue_struct *pixcir_wq;
static struct i2c_driver pixcir_i2c_ts_driver;
static struct class *i2c_dev_class;
static struct pixcir_i2c_ts_data *tsdata;
static LIST_HEAD(i2c_dev_list);
static DEFINE_SPINLOCK(i2c_dev_list_lock);


/*static int i2cdev_check(struct device *dev, void *addrp)
 {
 struct i2c_client *client = i2c_verify_client(dev);

 if (!client || client->addr != *(unsigned int *)addrp)
 return 0;

 return dev->driver ? -EBUSY : 0;
 }

 static int i2cdev_check_addr(struct i2c_adapter *adapter,unsigned int addr)
 {
 return device_for_each_child(&adapter->dev,&addr,i2cdev_check);
 }*/

static void return_i2c_dev(struct i2c_dev *i2c_dev)
{
	spin_lock(&i2c_dev_list_lock);
	list_del(&i2c_dev->list);
	spin_unlock(&i2c_dev_list_lock);
	kfree(i2c_dev);
}

static struct i2c_dev *i2c_dev_get_by_minor(unsigned index)
{
	struct i2c_dev *i2c_dev;
	i2c_dev = NULL;

	spin_lock(&i2c_dev_list_lock);
	list_for_each_entry(i2c_dev, &i2c_dev_list, list)
	{
		if (i2c_dev->adap->nr == index)
		goto found;
	}
	i2c_dev = NULL;
	found: spin_unlock(&i2c_dev_list_lock);
	return i2c_dev;
}

static struct i2c_dev *get_free_i2c_dev(struct i2c_adapter *adap)
{
	struct i2c_dev *i2c_dev;

	if (adap->nr >= I2C_MINORS)
	{
		printk(KERN_ERR "i2c-dev: Out of device minors (%d)\n",
				adap->nr);
		return ERR_PTR(-ENODEV);
	}

	i2c_dev = kzalloc(sizeof(*i2c_dev), GFP_KERNEL);
	if (!i2c_dev)
		return ERR_PTR(-ENOMEM);
	i2c_dev->adap = adap;

	spin_lock(&i2c_dev_list_lock);
	list_add_tail(&i2c_dev->list, &i2c_dev_list);
	spin_unlock(&i2c_dev_list_lock);
	return i2c_dev;
}

void read_XN_YN_value(struct i2c_client *client)
{
	char Wrbuf[4], Rdbuf[2];
	int ret;

	memset(Wrbuf, 0, sizeof(Wrbuf));
	memset(Rdbuf, 0, sizeof(Rdbuf));

	Wrbuf[0] = SPECOP; //specop addr
	Wrbuf[1] = 1; //write 1 to read eeprom
	Wrbuf[2] = 64;
	Wrbuf[3] = 0;
	ret = i2c_master_send(client, Wrbuf, 4);
	if (ret != 4)
		printk("send ret = %d\n", ret);
	mdelay(8);
	ret = i2c_master_recv(client, Rdbuf, 2);
	if (ret != 1)
		printk("send ret = %d\n", ret);
	x_nb_electrodes = Rdbuf[0];
	printk("x_nb_electrodes = %d\n", x_nb_electrodes);

	if (Tango_number == 1)
	{
		x2_nb_electrodes = 0;

		memset(Wrbuf, 0, sizeof(Wrbuf));
		memset(Rdbuf, 0, sizeof(Rdbuf));

		Wrbuf[0] = SPECOP; //specop addr
		Wrbuf[1] = 1; //write to eeprom
		Wrbuf[2] = 203;
		Wrbuf[3] = 0;
		ret = i2c_master_send(client, Wrbuf, 4);
		mdelay(4);

		ret = i2c_master_recv(client, Rdbuf, 2);
		y_nb_electrodes = Rdbuf[0];
		printk("y_nb_electrodes = %d\n", y_nb_electrodes);
	}
	else if (Tango_number == 2)
	{
		memset(Wrbuf, 0, sizeof(Wrbuf));
		memset(Rdbuf, 0, sizeof(Rdbuf));

		Wrbuf[0] = SPECOP; //specop addr
		Wrbuf[1] = 1; //write to eeprom
	#ifdef R8C_3GA_2TG
		Wrbuf[2] = 151;
		Wrbuf[3] = 0;
	#else
		Wrbuf[2] = 211;
		Wrbuf[3] = 0;
	#endif
		ret = i2c_master_send(client, Wrbuf, 4);
		mdelay(4);
		ret = i2c_master_recv(client, Rdbuf, 2);
		x2_nb_electrodes = Rdbuf[0];
		printk("x2_nb_electrodes = %d\n", x2_nb_electrodes);

		memset(Wrbuf, 0, sizeof(Wrbuf));
		memset(Rdbuf, 0, sizeof(Rdbuf));

		Wrbuf[0] = SPECOP; //specop addr
		Wrbuf[1] = 1; //write to eeprom
	#ifdef R8C_3GA_2TG
		Wrbuf[2] = 238;
		Wrbuf[3] = 0;
	#else
		Wrbuf[2] = 151;
		Wrbuf[3] = 0;
	#endif
		ret = i2c_master_send(client, Wrbuf, 4);
		mdelay(4);

		ret = i2c_master_recv(client, Rdbuf, 2);
		y_nb_electrodes = Rdbuf[0];
		printk("y_nb_electrodes = %d\n", y_nb_electrodes);
	}

	if (x2_nb_electrodes)
	{
		x1_x2_nb_electrodes = x_nb_electrodes + x2_nb_electrodes - 1;
	}
	else
	{
		x1_x2_nb_electrodes = x_nb_electrodes;
	}
	read_XN_YN_flag = 1;
}

void read_XY_tables(struct i2c_client *client, signed char *xy_raw1_buf,
		signed char *xy_raw2_buf)
{
	u_int8_t Wrbuf[1];
	int ret;

	memset(Wrbuf, 0, sizeof(Wrbuf));
	#ifdef R8C_AUO_I2C
	   Wrbuf[0] = 128; //xy_raw1[0] rawdata X register address for AUO
	#else
	   Wrbuf[0] = 61; //xy_raw1[0] rawdata X register address for PIXCIR
	#endif
	ret = i2c_master_send(client, Wrbuf, 1);
	if (ret != 1)
	{
		printk("send xy_raw1[0] register address error in read_XY_tables function ret = %d\n",ret);
	}
	ret = i2c_master_recv(client, xy_raw1_buf, 62);
	if (ret != 62)
	{
		printk("receive xy_raw1 error in read_XY_tables function ret = %d\n",ret);
	}
	#ifdef R8C_AUO_I2C
	   Wrbuf[0] = 43;  //xy_raw2[0] rawdata Y register address for AUO
	#else
	   Wrbuf[0] = 125; //xy_raw2[0] rawdata Y register address for PIXCIR
	#endif
	ret = i2c_master_send(client, Wrbuf, 1);
	if (ret != 1)
	{
		printk("send xy_raw2[0] register address error in read_XY_tables function ret = %d\n",ret);
	}
	ret = i2c_master_recv(client, xy_raw2_buf, 62);
	if (ret != 62)
	{
		printk("receive xy_raw2 error in read_XY_tables function ret = %d\n",ret);
	}
}

/////////////////////////////////////////////////////////////

/*********************************Porc-filesystem-TOP**************************************/

static int pixcir_fwinfo(void)
{
	unsigned char Wrbuf[1] = {0};
	unsigned char Rdbuf[5] = {0};
	int ret = 0;

	Wrbuf[0] = 0x30;  //version info register address

	ret = i2c_master_send(tsdata->client, Wrbuf, sizeof(Wrbuf));
	if (ret != sizeof(Wrbuf))
	{
		dev_err(&tsdata->client->dev, "Unable to write to i2c touchscreen!\n");
		goto out;
	}

	//printk("%s\n", __FUNCTION__);

	ret = i2c_master_recv(tsdata->client, Rdbuf, sizeof(Rdbuf));
	if (ret != sizeof(Rdbuf))
	{
		dev_err(&tsdata->client->dev, "Unable to read touch ic buffer!\n");
		goto out;
	}
	
	printk("Pixcir Tango_A8 Firmware version info.\n");
	printk("Main version: %.2x%.2x-%.2x-%.2x\n", Rdbuf[0], Rdbuf[1], Rdbuf[2], Rdbuf[3]);
	printk("Sub version: %.2x\n", Rdbuf[4]);
	
	return 0;
out:
	return -1;
}

unsigned int pixcir_chip_enable_disable(unsigned int cmd)
{
	unsigned char Wrbuf[2];
	int ret = 0;

	//printk("%s\n", __FUNCTION__);
	
	if (POWER_OFF == power_state)
	{
		if (PIXCIR_ON == cmd) 
		{
			printk("Cap touch panel enable suspend.\n");
		}
	
		if (PIXCIR_OFF == cmd)
		{
			printk("Cap touch panel disable suspend.\n");
		}

		suspend_cmd_state = HAVE_SUSPEND_CMD;
		suspend_cmd_store = cmd;
		
		return 0;
	}
	else
	{
		suspend_cmd_state = NO_SUSPEND_CMD;
	}

	memset(Wrbuf, 0, sizeof(Wrbuf));

	Wrbuf[0] = 36; //enable/disable register address
 
	if (PIXCIR_ON == cmd) 
	{
		Wrbuf[1] = 0;
		pixcir_state = PIXCIR_ON;
		printk("Cap touch panel enable.\n");
	}
	
	if (PIXCIR_OFF == cmd)
	{
		Wrbuf[1] = 1;
		pixcir_state = PIXCIR_OFF;
		printk("Cap touch panel disable.\n");
		mdelay(25);  //delay for switch not at the some time
	}	
	
	ret = i2c_master_send(tsdata->client, Wrbuf, sizeof(Wrbuf));
	if (ret != sizeof(Wrbuf))
	{
		printk("%s: ret = %d\n", __FUNCTION__, ret);
		dev_err(&tsdata->client->dev, "Unable to write to i2c touchscreen!!\n");
		goto out;
	}

	return 0;
out:
	return -1;
}

unsigned int pixcir_chip_status(void)
{
	unsigned char Wrbuf[1];
	unsigned char Rdbuf[1];
	int ret = 0;
	
	memset(Wrbuf, 0, sizeof(Wrbuf));
	memset(Rdbuf, 0, sizeof(Rdbuf));

	Wrbuf[0] = 36;	 //enable/disable register address

	ret = i2c_master_send(tsdata->client, Wrbuf, sizeof(Wrbuf));
	if (ret != sizeof(Wrbuf))
	{
		dev_err(&tsdata->client->dev, "Unable to write to i2c touchscreen!\n");
		goto out;
	}

	ret = i2c_master_recv(tsdata->client, Rdbuf, sizeof(Rdbuf));
	if (ret != sizeof(Rdbuf))
	{
		dev_err(&tsdata->client->dev, "Unable to read touch ic buffer!\n");
		goto out;
	}

	if (1 == Rdbuf[0])
		printk("Cap touch panel has been disabled.\n");
	else if (0 == Rdbuf[0])
		printk("Cap touch panel has been enabled.\n");
	else
		printk("Error: Cap touch panel undefined status.\n");
		
	return 0;
out:
	return -1;
}

unsigned int pixcir_do_calibration(unsigned int cmd)
{
	unsigned char Wrbuf[2] = {0};
    int ret = 0;

	if (0 == cmd)
		goto out;

	Wrbuf[0] = SPECOP;
	Wrbuf[1] = 0x3;

	ret = i2c_master_send(tsdata->client, Wrbuf, sizeof(Wrbuf));
	if (ret != sizeof(Wrbuf))
	{
		printk("%s\n", __func__);
		dev_err(&tsdata->client->dev, "Unable to write to i2c touchscreen!\n");
		goto out;
	}

	printk("Cap Touch screen calibration...\n");
	printk("DO NOT touch the screen in 5 seconds!!!\n");

	return 0;
out:
	return -1;
}

unsigned int pixcir_get_chip(void)
{
	pixcir_chip_status();
	return 0;
}

unsigned int pixcir_set_chip(unsigned int cmd)
{
	pixcir_chip_enable_disable(cmd);
	return 0;
}

unsigned int pixcir_get_version(void)
{
	pixcir_fwinfo();
	return 0;
}

unsigned int pixcir_set_version(unsigned int version)
{
	//printk("%s:Blank function\n", __FUNCTION__);

	/*
	note: add for debugs...

	*/
	enable_irq(tsdata->irq);

	return 0;
}

unsigned int pixcir_get_calibration(void)
{
	printk("%s:Blank function\n", __FUNCTION__);
	return 0;
}

unsigned int pixcir_set_calibration(unsigned int cmd)
{
	pixcir_do_calibration(cmd);
	return 0;
}

struct pixcir_proc {
	char *module_name;
	unsigned int (*pixcir_get) (void);
	unsigned int (*pixcir_set) (unsigned int);
};

static const struct pixcir_proc pixcir_modules[] = {
	{"pixcir_version", pixcir_get_version, pixcir_set_version},
	{"pixcir_enable", pixcir_get_chip, pixcir_set_chip},
	{"pixcir_calibration", pixcir_get_calibration, pixcir_set_calibration},
};

ssize_t pixcir_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	char *out = page;
	unsigned int pixcir;
	const struct pixcir_proc *proc = data;
	ssize_t len;

	pixcir = proc->pixcir_get();

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

size_t pixcir_proc_write(struct file * filp, const char __user * buf, unsigned long len, void *data)
{
	char command[PROC_CMD_LEN];
	const struct pixcir_proc *proc = data;

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
		proc->pixcir_set(1);
	} else if (strnicmp(command, "off", 3) == 0 || strnicmp(command, "0", 1) == 0) {
		proc->pixcir_set(0);
	} else {
		return -EINVAL;
	}

	return len;
}

/*
static int pixcir_upgrade_firmware(void)
{
	int i, ret = 0;
	unsigned char bootloader_cmd[] = {0x37, 0x05};
	unsigned char info_buf[4] = {0};
	unsigned char *pinfo = info_buf;
	
	//printk("%s\n", __FUNCTION__);

	printk("addr is 0x%x\n", tsdata->client->addr);

	ret = i2c_master_send(tsdata->client, bootloader_cmd, sizeof(bootloader_cmd));
	if (ret != sizeof(bootloader_cmd))
	{
		dev_err(&tsdata->client->dev, "Unable to write to i2c touchscreen!\n");
		goto out;
	}

	mdelay(4);

	tsdata->client->addr = 0x5d;  //change to bootloader mode i2c address

	printk("addr is 0x%x\n", tsdata->client->addr);
	ret = i2c_master_recv(tsdata->client, info_buf, sizeof(info_buf));
	printk("ret is %d\n", ret);
	printk("addr is 0x%x\n", tsdata->client->addr);
	if (ret != sizeof(info_buf))
	{
		dev_err(&tsdata->client->dev, "Unable to read touch ic buffer!\n");
		goto out;
	}
	
	tsdata->client->addr = 0x5c;  //change to normal mode i2c address

	printk("Bootloader information: ");

	for (i = 0; i < 4; i++)
	{
		printk("%d ",*pinfo++);
	}

	printk("\n");

	return 0;
out:
	return -1;
}

static int pixcir_firmware_write(struct file *file, const char __user * buffer, size_t count, loff_t * ppos)
{
	
	struct update_data *ud = kzalloc(sizeof(struct update_data), GFP_KERNEL);
	if (count != sizeof(struct update_data)) {
		printk("upgrade firmware buffer is not correct!!!\n");
		return -EINVAL;
	}

	if (copy_from_user(ud, buffer, count)) {
		printk("copy_from_user error!\n");
		return -EFAULT;
	}

	if (-1 == pixcir_upgrade_firmware()) {
		printk("upgrade firmware error!!!\n");
		return -EINVAL;
	}
	
//	kfree(ud);

	return count;
}
*/

//proc file_operations
struct file_operations pixcir_fops = {
	.owner = THIS_MODULE,
	//.write = pixcir_firmware_write,
};

static int proc_node_num = (sizeof(pixcir_modules) / sizeof(*pixcir_modules));
static struct proc_dir_entry *update_proc_root;
static struct proc_dir_entry *proc[5];

static int pixcir_proc_init(void)
{
	unsigned int i;
	//struct proc_dir_entry *proc;

	update_proc_root = proc_mkdir("pixcir_touch", NULL);

	if (!update_proc_root) 
	{
		printk("create pixcir_touch directory failed\n");
		return -1;
	}

	/*
	proc = create_proc_entry(pixcir_firmware_upgrade, S_IWUGO, update_proc_root);
	if (!proc) {
		printk("proc_register error!!!\n");
		return -1;
	}

	proc->proc_fops = &pixcir_fops;
	*/

	for (i = 0; i < proc_node_num; i++) 
	{
		mode_t mode;

		mode = 0;
		if (pixcir_modules[i].pixcir_get)
		{
			mode |= S_IRUGO;
		}
		if (pixcir_modules[i].pixcir_set)
		{
			mode |= S_IWUGO;
		}

		proc[i] = create_proc_entry(pixcir_modules[i].module_name, mode, update_proc_root);
		if (proc[i]) 
		{
			proc[i]->data = (void *)(&pixcir_modules[i]);
			proc[i]->read_proc = pixcir_proc_read;
			proc[i]->write_proc = pixcir_proc_write;
		}
	}

	return 0;
}

static void pixcir_proc_remove(void)
{
//	printk("pixcir_proc_remove\n");
	remove_proc_entry("pixcir_enable", update_proc_root);
	remove_proc_entry("pixcir_version", update_proc_root);
	remove_proc_entry("pixcir_calibration", update_proc_root);
	remove_proc_entry("pixcir_touch", NULL);
}

/*********************************Porc-filesystem-BOTTOM**************************************/

static int pixcir_power_int_mode(unsigned char power_mode, unsigned char int_mode)
{
	int ret = 0;
	unsigned char Wrbuf[3] = {0};

	Wrbuf[0] = 0x14; //POWER_mode register address

	Wrbuf[1] = power_mode;
	Wrbuf[2] = int_mode;

	ret = i2c_master_send(tsdata->client, Wrbuf, sizeof(Wrbuf));
	if (ret != sizeof(Wrbuf))
	{
		printk("%s\n", __func__);
		dev_err(&tsdata->client->dev, "Unable to write to i2c touchscreen!\n");
		goto out;
	}

	return 0;
out: 
	return -1;
}

/*********************************V2.0-Bee-0928-BOTTOM****************************************/

static unsigned char btn_status = 0;
/*
static void pixcir_btn_timer(unsigned long arg)
{
	//printk("arg is %ld\n", arg);

	schedule_work(&tsdata->btn_work);
}

static void pixcir_btn_report(void)
{
	int ret = 0;
	unsigned char Rdbuf[1], Wrbuf[1];
	
	memset(Wrbuf, 0, sizeof(Wrbuf));
	memset(Rdbuf, 0, sizeof(Rdbuf));

	Wrbuf[0] = 0x29;

	ret = i2c_master_send(tsdata->client, Wrbuf, sizeof(Wrbuf));
	if (ret != sizeof(Wrbuf))
	{
		printk("%s\n", __func__);
		dev_err(&tsdata->client->dev, "Unable to write to i2c touchscreen!\n");
		goto out;
	}

	ret = i2c_master_recv(tsdata->client, Rdbuf, sizeof(Rdbuf));
	if (ret != sizeof(Rdbuf))
	{
		printk("%s\n", __func__);
		dev_err(&tsdata->client->dev, "Unable to read i2c page!\n");
		goto out;
	}
	
	printk("Rdbuf[0] is 0x%x\n", Rdbuf[0]);

	if ((Rdbuf[0] & BIT_HOME) != (btn_status & BIT_HOME))
	{
		input_report_key(tsdata->input, TOUCH_KEY_HOME , !!(Rdbuf[0] & BIT_HOME));
	}

	if ((Rdbuf[0] & BIT_BACK) != (btn_status & BIT_BACK))
	{	
		input_report_key(tsdata->input, TOUCH_KEY_BACK, !!(Rdbuf[0] & BIT_BACK));
	}

	if ((Rdbuf[0] & BIT_SEARCH) != (btn_status & BIT_SEARCH))
	{
		input_report_key(tsdata->input, TOUCH_KEY_SEARCH, !!(Rdbuf[0] & BIT_SEARCH));
	}

	if ((Rdbuf[0] & BIT_MENU) != (btn_status & BIT_MENU))
	{
		input_report_key(tsdata->input, TOUCH_KEY_MENU, !!(Rdbuf[0] & BIT_MENU));
	}

	btn_status = Rdbuf[0];
	
out:
	return ;
}
*/
static void pixcir_btn_check(struct pixcir_i2c_ts_data *pdata)
{
	unsigned char Rdbuf[1], Wrbuf[1];
	int ret;

//	printk("%s\n", __func__);

	memset(Wrbuf, 0, sizeof(Wrbuf));
	memset(Rdbuf, 0, sizeof(Rdbuf));

	Wrbuf[0] = 0x29;

	ret = i2c_master_send(pdata->client, Wrbuf, sizeof(Wrbuf));
	if (ret != sizeof(Wrbuf))
	{
		printk("%s\n", __func__);
		dev_err(&pdata->client->dev, "Unable to write to i2c touchscreen!\n");
		goto out;
	}

	ret = i2c_master_recv(pdata->client, Rdbuf, sizeof(Rdbuf));
	if (ret != sizeof(Rdbuf))
	{
		printk("%s\n", __func__);
		dev_err(&pdata->client->dev, "Unable to read i2c page!\n");
		goto out;
	}

	if (Rdbuf[0] == btn_status)
	{
		goto out;
	}
	
	//mod_timer(&pdata->timer, jiffies + msecs_to_jiffies(14));  //20ms debounce
	
	if ((Rdbuf[0] & BIT_HOME) != (btn_status & BIT_HOME))
	{
		input_report_key(tsdata->input, TOUCH_KEY_HOME , !!(Rdbuf[0] & BIT_HOME));
	}

	if ((Rdbuf[0] & BIT_BACK) != (btn_status & BIT_BACK))
	{	
		input_report_key(tsdata->input, TOUCH_KEY_BACK, !!(Rdbuf[0] & BIT_BACK));
	}

	if ((Rdbuf[0] & BIT_SEARCH) != (btn_status & BIT_SEARCH))
	{
		input_report_key(tsdata->input, TOUCH_KEY_SEARCH, !!(Rdbuf[0] & BIT_SEARCH));
	}

	if ((Rdbuf[0] & BIT_MENU) != (btn_status & BIT_MENU))
	{
		input_report_key(tsdata->input, TOUCH_KEY_MENU, !!(Rdbuf[0] & BIT_MENU));
	}

	btn_status = Rdbuf[0];	
	
out:
	return;
}

static void pixcir_ts_poscheck(struct work_struct *work)
{
	struct pixcir_i2c_ts_data *tsdata = container_of(work,
			struct pixcir_i2c_ts_data,
			work.work);
	unsigned char touching = 0;
	unsigned char oldtouching = 0;
	int posx1, posy1, posx2, posy2;
	unsigned char Rdbuf[18],Wrbuf[1];
	int ret;
	int z1 = 0, z2 = 0;
	int w1 = 0, w2 = 0;

	//printk("%s\n", __func__);

	interrupt_flag = 1;

	memset(Wrbuf, 0, sizeof(Wrbuf));
	memset(Rdbuf, 0, sizeof(Rdbuf));

	Wrbuf[0] = 0;

	ret = i2c_master_send(tsdata->client, Wrbuf, 1);

#if PIXCIR_DEBUG
	printk("master send ret:%d\n",ret);
#endif

	if (ret != 1)
	{
#if PIXCIR_DEBUG
		printk("%s\n", __func__);
		dev_err(&tsdata->client->dev, "Unable to write to i2c touchscreen!!!\n");
#endif
		goto out;
	}

#ifdef R8C_AUO_I2C
	ret = i2c_master_recv(tsdata->client, Rdbuf, 8);

	if (ret != 8)
	{
		printk("%s\n", __func__);
		dev_err(&tsdata->client->dev, "Unable to read i2c page!\n");
		goto out;
	}

	posx1 = ((Rdbuf[1] << 8) | Rdbuf[0]);
	posy1 = ((Rdbuf[3] << 8) | Rdbuf[2]);
	posx2 = ((Rdbuf[5] << 8) | Rdbuf[4]);
	posy2 = ((Rdbuf[7] << 8) | Rdbuf[6]);
#else
	ret = i2c_master_recv(tsdata->client, Rdbuf, sizeof(Rdbuf));

	if (ret != sizeof(Rdbuf))
	{
		dev_err(&tsdata->client->dev, "Unable to read i2c page!\n");
		goto out;
	}

	posx1 = ((Rdbuf[3] << 8) | Rdbuf[2]);
	posy1 = ((Rdbuf[5] << 8) | Rdbuf[4]);
	posx2 = ((Rdbuf[7] << 8) | Rdbuf[6]);
	posy2 = ((Rdbuf[9] << 8) | Rdbuf[8]);
#endif

	w1 = (Rdbuf[10] + Rdbuf[11]) / 5;
	w2 = (Rdbuf[12] + Rdbuf[13]) / 5;
	z1 = (Rdbuf[14] + Rdbuf[15]) / 5;
	z2 = (Rdbuf[16] + Rdbuf[17]) / 5;

#if PIXCIR_DEBUG
	printk("master recv ret:%d\n",ret);
#endif


#ifdef R8C_AUO_I2C
	if((posx1 != 0) || (posy1 != 0) || (posx2 != 0) || (posy2 != 0)){
		Wrbuf[0] = 113;
		ret = i2c_master_send(tsdata->client, Wrbuf, 1);
		ret = i2c_master_recv(tsdata->client, auotpnum, 1);
		touching = auotpnum[0]>>5;
		oldtouching = 0;
	}
#else
	touching = Rdbuf[0];
	oldtouching = Rdbuf[1];
	
#endif

#if PIXCIR_DEBUG
	printk("touching:%-3d,oldtouching:%-3d,x1:%-6d,y1:%-6d,x2:%-6d,y2:%-6d\n",touching, oldtouching, posx1, posy1, posx2, posy2);
#endif
/*
	if (touching)
	{
		//printk("\n**touching = %d***\n", touching);
		input_report_abs(tsdata->input, ABS_X, posx1);
		input_report_abs(tsdata->input, ABS_Y, posy1);
		input_report_key(tsdata->input, BTN_TOUCH, 1);
		input_report_abs(tsdata->input, ABS_PRESSURE, 1);
	}
	else
	{
		input_report_key(tsdata->input, BTN_TOUCH, 0);
		input_report_abs(tsdata->input, ABS_PRESSURE, 0);
	}
*/

	pixcir_btn_check(tsdata); 
	
	if (pixcir_state == PIXCIR_OFF)
	{
		goto out;
	}
	
#ifdef TWO_POINTS
	if (touching)
	{

		//printk("touching is %d\n", touching);
		input_report_key(tsdata->input, BTN_TOUCH, 1);
		input_report_abs(tsdata->input, ABS_MT_TOUCH_MAJOR, z1);
		input_report_abs(tsdata->input, ABS_MT_WIDTH_MAJOR, w1);
		input_report_abs(tsdata->input, ABS_MT_POSITION_X, posx1);
		input_report_abs(tsdata->input, ABS_MT_POSITION_Y, posy1);
		input_mt_sync(tsdata->input);
		if (touching == 2)
		{
			input_report_abs(tsdata->input, ABS_MT_TOUCH_MAJOR, z2);
			input_report_abs(tsdata->input, ABS_MT_WIDTH_MAJOR, w2);
			input_report_abs(tsdata->input, ABS_MT_POSITION_X, posx2);
			input_report_abs(tsdata->input, ABS_MT_POSITION_Y, posy2);
			input_mt_sync(tsdata->input);
		}
	}
	else
	{
		//printk("key up\n");
		input_report_key(tsdata->input, BTN_TOUCH, 0);
		input_report_abs(tsdata->input, ABS_MT_TOUCH_MAJOR, 0);
		input_report_abs(tsdata->input, ABS_MT_WIDTH_MAJOR, 0);
		input_report_abs(tsdata->input, ABS_MT_POSITION_X, 0);
		input_report_abs(tsdata->input, ABS_MT_POSITION_Y, 0);
		input_mt_sync(tsdata->input);
	}
	
#endif
	input_sync(tsdata->input);

	out: enable_irq(tsdata->irq);

	if(status_reg == NORMAL_MODE)
	{
	#ifdef R8C_AUO_I2C
		global_touching = touching;
	    	global_oldtouching = 0;
	    	global_posx1_low = Rdbuf[0];
		global_posx1_high = Rdbuf[1];
		global_posy1_low = Rdbuf[2];
		global_posy1_high = Rdbuf[3];
		global_posx2_low = Rdbuf[4];
		global_posx2_high = Rdbuf[5];
		global_posy2_low = Rdbuf[6];
		global_posy2_high = Rdbuf[7];

	#else
		global_touching = touching;
		global_oldtouching = oldtouching;
		global_posx1_low = Rdbuf[2];
		global_posx1_high = Rdbuf[3];
		global_posy1_low = Rdbuf[4];
		global_posy1_high = Rdbuf[5];
		global_posx2_low = Rdbuf[6];
		global_posx2_high = Rdbuf[7];
		global_posy2_low = Rdbuf[8];
		global_posy2_high = Rdbuf[9];
	#endif
	}
}

static irqreturn_t pixcir_ts_isr(int irq, void *dev_id)
{
	struct pixcir_i2c_ts_data *tsdata = dev_id;
	
	//printk("%s\n", __func__);

	if ((status_reg == 0) || (status_reg == NORMAL_MODE))
	{
		disable_irq_nosync(irq);
		
		queue_work(pixcir_wq, &tsdata->work.work);
		//queue_delayed_work(pixcir_wq, &tsdata->work, HZ / 500);	
	}
	
	return IRQ_HANDLED;
}

static int pixcir_eeprom_write(unsigned int addr, unsigned char *pdata,
							   unsigned int num)
{
	unsigned char Wrbuf[3];
	unsigned char Rdbuf[1];
	unsigned char specop[2] = {0x37, 0x02};
	int ret = 0;
	int i = 0;

	printk("%s\n", __func__);

	memset(Wrbuf, 0, sizeof(Wrbuf));
	memset(Rdbuf, 0, sizeof(Rdbuf));
	
	ret = i2c_master_send(tsdata->client, specop, sizeof(specop));
	if (ret != sizeof(specop))
	{
		dev_err(&tsdata->client->dev, "Unable to write to i2c touchscreen!\n");
		goto out;
	}
	
	for (i = 0; i < num; i++)
	{
		Wrbuf[0] = (addr % 0xff - 1);  //eeprom address
		Wrbuf[1] = addr / 0xff;
		Wrbuf[2] = *pdata++;
		
		ret = i2c_master_send(tsdata->client, Wrbuf, sizeof(Wrbuf));
		if (ret != sizeof(Wrbuf))
		{
			dev_err(&tsdata->client->dev, "Unable to write to i2c touchscreen!\n");
			goto out;
		}
		
		addr += 1;
		
		mdelay(4);
	}

	ret = i2c_master_recv(tsdata->client, Rdbuf, sizeof(Rdbuf));
	if (ret != sizeof(Rdbuf))
	{
		dev_err(&tsdata->client->dev, "Unable to read to i2c touchscreen!\n");
		goto out;
	}

	return 0;
out:
	return -1;
}

static int pixcir_eeprom_read(unsigned int addr, unsigned char num)
{
	unsigned char Wrbuf[4] = {0};
	unsigned char Rdbuf[255] = {0};
	int ret = 0;
	int i = 0;

	/*
	Wrbuf[0] = 56;

	ret = i2c_master_send(tsdata->client, Wrbuf, sizeof(Wrbuf));
	if (ret != sizeof(Wrbuf))
	{
		dev_err(&tsdata->client->dev, "Unable to write to i2c touchscreen!\n");
		goto out;
	}*/

	printk("%s\n", __func__);

	Wrbuf[0] = SPECOP;
	Wrbuf[1] = 1;
	Wrbuf[2] = (addr % 0xff - 1);
	Wrbuf[3] = addr / 0xff;

	ret = i2c_master_send(tsdata->client, Wrbuf, sizeof(Wrbuf));
	if (ret != sizeof(Wrbuf))
	{
		dev_err(&tsdata->client->dev, "Unable to write to i2c touchscreen!\n");
		goto out;
	}

	mdelay(4);

	ret = i2c_master_recv(tsdata->client, Rdbuf, num);
	if (ret != num)
	{
		dev_err(&tsdata->client->dev, "Unable to read touch ic buffer!\n");
		goto out;
	}

	printk("eeprom read : ");
	for (i = 0; i< num; i++)
	{
		printk("%d ", Rdbuf[i]);
	}
	printk("\n");

	return 0;
out:
	return -1;
}

static int pixcir_ts_open(struct input_dev *dev)
{
	return 0;
}

static void pixcir_ts_close(struct input_dev *dev)
{
}

void pixcir_ts_reset(void)
{
	printk("%s\n", __func__);

	RESETPIN_SET0;  //reset enable (hold)	
	POWERPIN_SET1;  //VCC power disable
	
	mdelay(2);
	
	POWERPIN_SET0;  //VCC power enable
	
	mdelay(1);
	
	RESETPIN_SET1; //reset disable

	return ;
}

static int pixcir_i2c_ts_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct input_dev *input;
	struct device *dev;
	struct i2c_dev *i2c_dev;
	int error;

	printk("pixcir_i2c_ts_probe\n");
	
	request_power(VDD_TOUCH_POWER);
	
	power_state = POWER_ON;
	
	GPIO_RESET_REQUEST;
	GPIO_POWER_REQUEST;
	
	RESETPIN_CFG;
	POWERPIN_CFG;
	
	pixcir_ts_reset();

	tsdata = kzalloc(sizeof(*tsdata), GFP_KERNEL);
	if (!tsdata)
	{
		dev_err(&client->dev, "failed to allocate driver data!\n");
		error = -ENOMEM;
		dev_set_drvdata(&client->dev, NULL);
		return error;
	}

	dev_set_drvdata(&client->dev, tsdata);

	input = input_allocate_device();
	if (!input)
	{
		dev_err(&client->dev, "failed to allocate input device!\n");
		error = -ENOMEM;
		input_free_device(input);
		kfree(tsdata);
	}

	set_bit(EV_SYN, input->evbit);
	set_bit(EV_KEY, input->evbit);
	set_bit(EV_ABS, input->evbit);
	set_bit(BTN_TOUCH, input->keybit);
	set_bit(TOUCH_KEY_MENU, input->keybit);
	set_bit(TOUCH_KEY_SEARCH, input->keybit);
	set_bit(TOUCH_KEY_BACK, input->keybit);
	set_bit(TOUCH_KEY_HOME , input->keybit);
	//input_set_abs_params(input, ABS_X, TOUCHSCREEN_MINX, TOUCHSCREEN_MAXX, 0, 0);
	//input_set_abs_params(input, ABS_Y, TOUCHSCREEN_MINY, TOUCHSCREEN_MAXY, 0, 0);
#ifdef TWO_POINTS
	input_set_abs_params(input, ABS_MT_POSITION_X, TOUCHSCREEN_MINX, TOUCHSCREEN_MAXX, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_Y, TOUCHSCREEN_MINY, TOUCHSCREEN_MAXY, 0, 0);
	input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
#endif

	input->name = client->name;
	input->id.bustype = BUS_I2C;
	input->dev.parent = &client->dev;
	input->open = pixcir_ts_open;
	input->close = pixcir_ts_close;

	input_set_drvdata(input, tsdata);

	tsdata->client = client;
	tsdata->input = input;
	
	pixcir_wq = create_singlethread_workqueue("pixcir_wq");
	if(!pixcir_wq)
	{
		dev_err(&client->dev, "failed to create_singlethread_workqueue!\n");
		kfree(tsdata);
		dev_set_drvdata(&client->dev, NULL);
		input_free_device(input);	
		return -ENOMEM;
	}

	//setup_timer(&tsdata->timer, pixcir_btn_timer, 0);
	//INIT_WORK(&tsdata->btn_work, pixcir_btn_report);
	
	INIT_DELAYED_WORK(&tsdata->work, pixcir_ts_poscheck);

	//wake_lock_init(&tsdata->pixcir_wake_lock, WAKE_LOCK_SUSPEND, "pixcir_wake_work");

	tsdata->irq = client->irq;
	global_irq = client->irq;

	if (input_register_device(input))
	{
		input_free_device(input);
		kfree(tsdata);
	}

	if (request_irq(tsdata->irq, pixcir_ts_isr, IRQF_TRIGGER_LOW, client->name, tsdata))
  //if (request_irq(tsdata->irq, pixcir_ts_isr, IRQF_TRIGGER_FALLING, client->name, tsdata))
	{
		dev_err(&client->dev, "Unable to request touchscreen IRQ.\n");
		input_unregister_device(input);
		input = NULL;
	}

	device_init_wakeup(&client->dev, 1);

	pixcir_proc_init();
	
#ifdef CONFIG_HAS_EARLYSUSPEND
	es.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	es.suspend = pixcir_early_suspend;
	es.resume = pixcir_late_resume;
	register_early_suspend(&es);
#endif

	/*********************************V2.0-Bee-0928-TOP****************************************/
	i2c_dev = get_free_i2c_dev(client->adapter);
	if (IS_ERR(i2c_dev))
	{
		error = PTR_ERR(i2c_dev);
		return error;
	}

	dev = device_create(i2c_dev_class, &client->adapter->dev, MKDEV(I2C_MAJOR,
			client->adapter->nr), NULL, "pixcir_i2c_ts%d", client->adapter->nr);
	if (IS_ERR(dev))
	{
		error = PTR_ERR(dev);
		return error;
	}
	/*********************************V2.0-Bee-0928-BOTTOM****************************************/
	
	mdelay(150);

	//pixcir_power_int_mode(AUTO_ACTIVE_MODE, INT_FIN);	
	//pixcir_power_int_mode(AUTO_ACTIVE_MODE, INT_MOV);
	//pixcir_power_int_mode(ACTIVE_MODE, INT_MOV);
	
	//unsigned char Wrbuf[1] = {0};
	//pixcir_eeprom_write(281, Wrbuf, sizeof(Wrbuf));
	//pixcir_eeprom_read(281, 8);

	return 0;
}

static int pixcir_i2c_ts_remove(struct i2c_client *client)
{
	int error;
	struct i2c_dev *i2c_dev;
	struct pixcir_i2c_ts_data *tsdata = dev_get_drvdata(&client->dev);
	
	printk("pixcir_i2c_ts_remove\n");

	free_irq(tsdata->irq, tsdata);
	
	device_init_wakeup(&client->dev, 0);

	/*********************************V2.0-Bee-0928-TOP****************************************/
	i2c_dev = get_free_i2c_dev(client->adapter);
	if (IS_ERR(i2c_dev))
	{

		error = PTR_ERR(i2c_dev);
		return error;
	}
	return_i2c_dev(i2c_dev);
	device_destroy(i2c_dev_class, MKDEV(I2C_MAJOR, client->adapter->nr));
	/*********************************V2.0-Bee-0928-BOTTOM****************************************/
	input_unregister_device(tsdata->input);
	input_free_device(tsdata->input);
	flush_workqueue(pixcir_wq);
	destroy_workqueue(pixcir_wq);
	//wake_lock_destroy(&tsdata->pixcir_wake_lock);
	kfree(tsdata);
	dev_set_drvdata(&client->dev, NULL);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&es);
#endif

	pixcir_proc_remove();
	
	GPIO_RRSET_FREE;
	GPIO_POWER_FREE;
	
	release_power(VDD_TOUCH_POWER);
	
	power_state = POWER_OFF;
	
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void pixcir_early_suspend(struct early_suspend *h)
{
	int ret = 0;
	printk("%s\n", __func__);
	
	disable_irq(tsdata->irq);
	
	ret = cancel_delayed_work_sync(&tsdata->work);
	printk("ret = %d\n", ret);
	if (ret == 1)
	{
		enable_irq(tsdata->irq);
	}
	
	release_power(VDD_TOUCH_POWER);
	
	power_state = POWER_OFF;
}

static void pixcir_late_resume(struct early_suspend *h)
{
	printk("%s\n", __func__);

	request_power(VDD_TOUCH_POWER);
	
	power_state = POWER_ON;
		
	pixcir_ts_reset();
	
	mdelay(150);
	
	//pixcir_power_int_mode(AUTO_ACTIVE_MODE, INT_FIN);
	//pixcir_power_int_mode(AUTO_ACTIVE_MODE, INT_MOV);
	//pixcir_power_int_mode(ACTIVE_MODE, INT_MOV);
	
	if (HAVE_SUSPEND_CMD == suspend_cmd_state)
	{
		pixcir_chip_enable_disable(suspend_cmd_store);
		printk("Handled CAP suspend command.\n");
		goto _out;
	}

	if (PIXCIR_OFF == pixcir_state)
	{
		pixcir_chip_enable_disable(PIXCIR_OFF);
	}
	
_out:
	enable_irq(tsdata->irq);
}
#endif

#if 1
static int pixcir_i2c_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct pixcir_i2c_ts_data *tsdata = dev_get_drvdata(&client->dev);
	
	printk("%s\n", __func__);

#if 0
	if (device_may_wakeup(&client->dev))
		enable_irq_wake(tsdata->irq);
#endif

	return 0;
}

static int pixcir_i2c_ts_resume(struct i2c_client *client)
{
	struct pixcir_i2c_ts_data *tsdata = dev_get_drvdata(&client->dev);
	
	printk("%s\n", __func__);
	
#if 0
	if (device_may_wakeup(&client->dev))
		disable_irq_wake(tsdata->irq);
#endif

	return 0;
}
#endif

/*********************************V2.0-Bee-0928****************************************/
/*                        	  pixcir_open                                         */
/*********************************V2.0-Bee-0928****************************************/
static int pixcir_open(struct inode *inode, struct file *file)
{
	int subminor;
	struct i2c_client *client;
	struct i2c_adapter *adapter;
	struct i2c_dev *i2c_dev;
	int ret = 0;
#if PIXCIR_DEBUG
	printk("enter pixcir_open function\n");
#endif
	subminor = iminor(inode);
#if PIXCIR_DEBUG
	printk("subminor=%d\n",subminor);
#endif
	lock_kernel();
	i2c_dev = i2c_dev_get_by_minor(subminor);
	if (!i2c_dev)
	{
		printk("error i2c_dev\n");
		return -ENODEV;
	}

	adapter = i2c_get_adapter(i2c_dev->adap->nr);
	if (!adapter)
	{

		return -ENODEV;
	}
	//printk("after i2c_dev_get_by_minor\n");

	client = kzalloc(sizeof(*client), GFP_KERNEL);
	if (!client)
	{
		i2c_put_adapter(adapter);
		ret = -ENOMEM;
	}
	snprintf(client->name, I2C_NAME_SIZE, "pixcir_i2c_ts%d", adapter->nr);
	client->driver = &pixcir_i2c_ts_driver;
	client->adapter = adapter;
	//if(i2cdev_check_addr(client->adapter,0x5c))
	//	return -EBUSY;
	file->private_data = client;

	return 0;
}

/*********************************V2.0-Bee-0928****************************************/
/*                        	  pixcir_ioctl                                        */
/*********************************V2.0-Bee-0928****************************************/
static int pixcir_ioctl(struct file *file, unsigned int cmd, unsigned int arg)
{
	//printk("ioctl function\n");
	//unsigned char tmp[2] = {0};
	//int ret = 0;
	struct i2c_client *client = (struct i2c_client *) file->private_data;
#if PIXCIR_DEBUG
	printk("cmd = %d,arg = %d\n", cmd, arg);
#endif

	switch (cmd)
	{
	case CALIBRATION_FLAG: //CALIBRATION_FLAG = 1
#if PIXCIR_DEBUG
		printk("CALIBRATION\n");
#endif
		client->addr = SLAVE_ADDR;
		status_reg = 0;
		status_reg = CALIBRATION_FLAG;
		break;

	case NORMAL_MODE:
		client->addr = SLAVE_ADDR;
#if PIXCIR_DEBUG
		printk("NORMAL_MODE\n");
#endif		
		status_reg = 0;
		status_reg = NORMAL_MODE;
		break;

	case PIXCIR_DEBUG_MODE:
		client->addr = SLAVE_ADDR;
#if PIXCIR_DEBUG		
		printk("PIXCIR_DEBUG_MODE\n");
#endif		
		status_reg = 0;
		status_reg = PIXCIR_DEBUG_MODE;
		
		Tango_number = arg;
    	#ifdef R8C_3GA_2TG
        	Tango_number = 2;
    	#endif
		break;

	case BOOTLOADER_MODE: //BOOTLOADER_MODE = 7

#if PIXCIR_DEBUG
		printk("BOOTLOADER_MODE\n");
#endif
		status_reg = 0;
		status_reg = BOOTLOADER_MODE;
		disable_irq_nosync(global_irq);

#ifdef reset
		client->addr = BOOTLOADER_ADDR;

		RESETPIN_CFG;
		RESETPIN_SET0;
		mdelay(20);
		RESETPIN_SET1;

    	#ifdef R8C_3GA_2TG
        	mdelay(50);
    	#else 
			mdelay(30);
    	#endif
    
#else		//normal
		client->addr = SLAVE_ADDR;
		tmp[0] = SPECOP; //specop addr
		tmp[1] = 5; //change to bootloader
		ret = i2c_master_send(client,tmp,2);
	#if PIXCIR_DEBUG
		printk("BOOTLOADER_MODE,change to bootloader i2c_master_send ret = %d\n",ret);
	#endif
    
		client->addr = BOOTLOADER_ADDR;
		
	#if PIXCIR_DEBUG
		printk("client->addr = %x\n",client->addr);
	#endif
		
		mdelay(4);  // mdelay time for change to bootloader mode
#endif

		break;
		
		case ENABLE_IRQ:
		    enable_irq(global_irq);
		break;
		
		case DISABLE_IRQ:
			disable_irq_nosync(global_irq);
		break;


	default:
		break;//return -ENOTTY;
	}
	return 0;
}

/*********************************V2.0-Bee-0928****************************************/
/*                        	  pixcir_read                                        */
/*********************************V2.0-Bee-0928****************************************/
static ssize_t pixcir_read (struct file *file, unsigned char __user *buf, size_t count,loff_t *offset)
{
	struct i2c_client *client = (struct i2c_client *)file->private_data;
	int ret=0;
	unsigned char normal_tmp[10];
	unsigned char Rdverbuf[4] = {0};
	
	switch(status_reg)
	{
		case NORMAL_MODE:
		memset(normal_tmp, 0, sizeof(normal_tmp));
		if(interrupt_flag)
		{
			normal_tmp[0] = global_touching;
			normal_tmp[1] = global_oldtouching;
			normal_tmp[2] = global_posx1_low;
			normal_tmp[3] = global_posx1_high;
			normal_tmp[4] = global_posy1_low;
			normal_tmp[5] = global_posy1_high;
			normal_tmp[6] = global_posx2_low;
			normal_tmp[7] = global_posx2_high;
			normal_tmp[8] = global_posy2_low;
			normal_tmp[9] = global_posy2_high;
			
			//printk("global_touching:%-3d,global_oldtouching:%-3d\n",normal_tmp[0],normal_tmp[1]);
			
			ret = copy_to_user(buf,normal_tmp,10);
			/*if(ret!=10)
			{
				printk("interrupt_flag=%d,NORMAL_MODE,copy_to_user error, ret = %d\n",interrupt_flag,ret);
			}*/
		}
		interrupt_flag = 0;
		break;

		case PIXCIR_DEBUG_MODE:
		if(read_XN_YN_flag == 0)
		{
			unsigned char buf[2];
			memset(buf, 0, sizeof(buf));
			read_XN_YN_value(client);
			#ifdef R8C_AUO_I2C
			#else
			buf[0]=194;
			buf[1]=0;
			ret = i2c_master_send(client, buf, 2); //write internal_enable = 0
			if(ret!=2)
			{
				printk("PIXCIR_DEBUG_MODE,master send %d,%d error, ret = %d\n",buf[0],buf[1],ret);
			}
			#endif
		}
		else
		{
			memset(xy_raw1, 0, sizeof(xy_raw1));
			memset(xy_raw2, 0, sizeof(xy_raw2));
			read_XY_tables(client,xy_raw1,xy_raw2);
		}
		
		if(Tango_number ==1 )
		{
			xy_raw1[64] = x_nb_electrodes;
			xy_raw1[65] = y_nb_electrodes;
			ret = copy_to_user(buf,xy_raw1,66);
			/*if(ret!=66)
			 {
			 printk("PIXCIR_DEBUG_MODE,Tango_number = 1 copy_to_user error, ret = %d\n",ret);
			 }*/
		}
		else if(Tango_number == 2)
		{
			xy_raw1[64] = x_nb_electrodes;
			xy_raw1[65] = y_nb_electrodes;
			xy_raw1[66] = x2_nb_electrodes;
			for(ret=0;ret<67;ret++)
				xy_raw12[ret] = xy_raw1[ret];
			for(ret=0;ret<63;ret++)
				xy_raw12[67+ret] = xy_raw2[ret];
			ret = copy_to_user(buf,xy_raw12,131);
			/*if(ret!=131)
			{
				printk("PIXCIR_DEBUG_MODE,Tango_number = 2 copy_to_user error, ret = %d\n",ret);
			}*/
		}
		break;
		
		case BOOTLOADER_MODE:
		
			ret = i2c_master_recv(client, Rdverbuf, sizeof(Rdverbuf));
			if((ret != sizeof(Rdverbuf)) || (Rdverbuf[1] != 0xA5))
			{
			    printk("i2c_master_recv boot status error ret = %d, bootloader status = 0x%x\n", ret, Rdverbuf[1]);
			    ret = -1;
			    break;
			}
			
			ret = copy_to_user(buf, Rdverbuf, sizeof(Rdverbuf));
			if (0 != ret)
			{
				printk("copy_to_user failed!\n");
				ret = -EFAULT;
				break;
			}
			//printk("Bootloader Status: %x %x\n",Rdverbuf[0],Rdverbuf[1]);
		break;
		
		default:
		break;
	}

	return ret;
}

/*********************************V2.0-Bee-0928****************************************/
/*                        	  pixcir_write                                        */
/*********************************V2.0-Bee-0928****************************************/
static ssize_t pixcir_write(struct file *file, char __user *buf,size_t count, loff_t *ppos)
{
	struct i2c_client *client;
	unsigned char Wrbuf[2],bootload_data[143],Rdbuf[1],Rdverbuf[2];
	static int ret=0,ret2=0,stu;
	static int re_value=0;

	client = file->private_data;

	//printk("pixcir_write function\n");

	//wake_lock(&tsdata->pixcir_wake_lock);
	
	switch(status_reg)
	{
		case CALIBRATION_FLAG: //CALIBRATION_FLAG=1
		
		if (1 == *buf)
		{
		//	printk("Calibration. \n");
			Wrbuf[0] = SPECOP;
			Wrbuf[1] = 0x03;

			ret = i2c_master_send(client, Wrbuf, sizeof(Wrbuf));
			if (ret != sizeof(Wrbuf))
			{
				dev_err(&tsdata->client->dev, "Unable to write to i2c touchscreen!\n");
				ret = -1;
				break;
			}

		#if PIXCIR_DEBUG
			printk("CALIBRATION_FLAG,i2c_master_send ret = %d\n",ret);
		#endif
		}

		status_reg = 0;
		break;

		case BOOTLOADER_MODE: //BOOTLOADER_MODE=7
#if PIXCIR_DEBUG
		printk("BOOT. \n");
#endif
		memset(bootload_data, 0, sizeof(bootload_data));
		memset(Rdbuf, 0, sizeof(Rdbuf));

		if (copy_from_user(bootload_data,buf,count))
		{
			printk("COPY FAIL ");
			return -EFAULT;
		}
		
#if PIXCIR_DEBUG
		static int i;
		for(i=0;i<143;i++)
		{
			if(bootload_data[i]<0x10)
			printk("0%x",bootload_data[i]);
			else
			printk("%x",bootload_data[i]);
		}
#endif

		stu = bootload_data[0];
		#ifdef R8C_3GA_2TG
		if(stu == 0x01){
			ret2 = i2c_master_recv(client,Rdverbuf,2);
			if((ret2 != 2) || (Rdverbuf[1] != 0xA5)){
			    printk("i2c_master_recv boot status error ret2=%d,bootloader status=%x",ret2,Rdverbuf[1]);
			    ret = 0;
			    break;
			}
		//	printk("\n");
		//	printk("Bootloader Status:%x%x\n",Rdverbuf[0],Rdverbuf[1]);
		}
		#endif
		ret = i2c_master_send(client,bootload_data,count);
		if(ret!=count)
		{
			printk("bootload 143 bytes error,ret = %d\n",ret);
			break;
		}
		
		if(stu!=0x01)
		{
			mdelay(1);
			while(get_attb_value(ATTB));
			mdelay(1);

		#ifdef R8C_3GA_2TG
			if(stu == 0x03){
				ret2 = i2c_master_recv(client,Rdbuf,1);
				//printk("ret2=%d re_value=%d\n",ret2,Rdbuf[0]);
				if(ret2!=1)
				{
			  	  	ret = 0;
					printk("read IIC slave status error,ret = %d\n",ret2);
					break;
				}
			}
		#else
			ret2 = i2c_master_recv(client,Rdbuf,1);
			if(ret2!=1)
			{
			    	ret = 0;
				printk("read IIC slave status error,ret = %d\n",ret2);
				break;

			}
			re_value = Rdbuf[0];
		    #if PIXCIR_DEBUG
			printk("re_value = %d\n",re_value);
		    #endif
		#endif
		}
		else
		{
			mdelay(100);
			status_reg = 0;
			enable_irq(global_irq);
		}

		if((re_value&0x80)&&(stu!=0x01))
		{
			printk("Failed:(re_value&0x80)&&(stu!=0x01)=1\n");
			ret = 0;
		}
		break;

		default:
		break;
	}
	
	//wake_unlock(&tsdata->pixcir_wake_lock);
	
	return ret;
}

/*********************************V2.0-Bee-0928****************************************/
/*                        	  pixcir_release                                         */
/*********************************V2.0-Bee-0928****************************************/
static int pixcir_release(struct inode *inode, struct file *file)
{
	struct i2c_client *client = file->private_data;
   #if PIXCIR_DEBUG
	printk("enter pixcir_release funtion\n");
   #endif
	i2c_put_adapter(client->adapter);
	kfree(client);
	file->private_data = NULL;

	return 0;
}

/*********************************V2.0-Bee-0928-TOP****************************************/
static const struct file_operations pixcir_i2c_ts_fops =
{	.owner = THIS_MODULE, 
	.read = pixcir_read, 
	.write = pixcir_write,
	.open = pixcir_open, 
	.unlocked_ioctl = pixcir_ioctl,
	.release = pixcir_release, 
};
/*********************************V2.0-Bee-0928-BOTTOM****************************************/

static const struct i2c_device_id pixcir_i2c_ts_id[] =
{
	{ "pixcir168", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, pixcir_i2c_ts_id);

static struct i2c_driver pixcir_i2c_ts_driver =
{ 	.driver =
		{
			.owner = THIS_MODULE,
			.name = "pixcir_i2c_ts_driver_v2.0",
		}, 
	.probe = pixcir_i2c_ts_probe, 
	.remove = pixcir_i2c_ts_remove,
	.suspend = pixcir_i2c_ts_suspend, 
	.resume = pixcir_i2c_ts_resume,
	.id_table = pixcir_i2c_ts_id, 
};

static int __init pixcir_i2c_ts_init(void)
{
	int ret;
	printk("pixcir_i2c_init\n");
	
	/*********************************V2.0-Bee-0928-TOP****************************************/
	ret = register_chrdev(I2C_MAJOR,"pixcir_i2c_ts",&pixcir_i2c_ts_fops);
	if(ret)
	{
		printk(KERN_ERR "can't register major number, register chrdev failed!\n");
		return ret;
	}

	i2c_dev_class = class_create(THIS_MODULE, "pixcir_i2c_dev");
	if (IS_ERR(i2c_dev_class))
	{
		ret = PTR_ERR(i2c_dev_class);
		class_destroy(i2c_dev_class);
	}
	/********************************V2.0-Bee-0928-BOTTOM******************************************/
	ret = i2c_add_driver(&pixcir_i2c_ts_driver);
	//printk("ret = %d\n",ret);
	return ret;
}

static void __exit pixcir_i2c_ts_exit(void)
{
	printk("pixcir_i2c_exit\n");
	i2c_del_driver(&pixcir_i2c_ts_driver);
	/********************************V2.0-Bee-0928-TOP******************************************/
	class_destroy(i2c_dev_class);
	unregister_chrdev(I2C_MAJOR,"pixcir_i2c_ts");
	/********************************V2.0-Bee-0928-BOTTOM******************************************/
}

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE(DRIVER_LICENSE);

module_init( pixcir_i2c_ts_init);
module_exit( pixcir_i2c_ts_exit);

