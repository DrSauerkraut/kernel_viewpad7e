/*
 * Touch Screen driver for ITE CAP S3C6410POP Platform
 *
 * Copyright (c) 2010 FOXCONN TMSBG
 * Copyright (c) 2010 TMSBG  <tmsbg-bsp1@foxconn.com>,
 * TMSBG CDPG LH Team.
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU  General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <linux/i2c.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/earlysuspend.h>
#include <linux/math64.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-adc.h>
#include "it72xx.h"
#define EVENT_PENDOWN 1
#define EVENT_REPEAT  2
#define EVENT_PENUP   3
#define PROC_CMD_LEN            20

#ifdef DEBUG_IT72XX
spinlock_t it72xx_lock;
struct timer_list cdc_touch_timer;
struct delayed_work cdc_work;
#endif

#ifndef ENABLE_INTERRUPT_MODE
static struct timer_list touch_timer = TIMER_INITIALIZER(touch_it72xx_timer, 0, 0);
struct timer_list touch_timer;
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
struct early_suspend es;

void it72xx_early_suspend(struct early_suspend *h);
void it72xx_late_resume(struct early_suspend *h);
enum {
	ACTIVE = 0,
	IGNORE
};
#endif

struct timer_list powersupply_timer;

static int ts_xres;
static int ts_yres;
static int powersupply = -1;	// 0: battery, 1: ac
static int powersupply_change = 0;	// 0: battery, 1: ac

unsigned int flash_size = 0;
unsigned int MEM_SIZE = 128;

unsigned char cmd_it72xx_enable_interrupt[4] = { 0x02, 0x04, 0x01, 0x00 };
unsigned char cmd_it72xx_disable_interrupt[4] = { 0x02, 0x04, 0x00, 0x00 };
unsigned char cmd_it72xx_get_fwinfo[2] = { 0x01, 0x00 };
unsigned char cmd_it72xx_get_deviceinfo[1] = { 0x00 };
unsigned char cmd_it72xx_get_2dresolution[3] = { 0x01, 0x02, 0x00 };
unsigned char cmd_it72xx_reset[1] = { 0x6F };

unsigned char cmd_it72xx_enter_update_mode[10] = { 0x60, 0x00, 0x49, 0x54, 0x37, 0x32, 0x36, 0x30, 0x55, 0xAA };
unsigned char cmd_it72xx_exit_update_mode[10] = { 0x60, 0x80, 0x49, 0x54, 0x37, 0x32, 0x36, 0x30, 0xAA, 0x55 };
unsigned char cmd_it72xx_get_flash_size[2] = { 0x01, 0x03 };
unsigned char cmd_it72xx_set_flash_off[4] = { 0x61, 0x00, 0x00, 0x00 };
unsigned char cmd_it72xx_write_mem_to_flash[1] = { 0xF1 };
unsigned char cmd_it72xx_write_fw_to_mem[6] = { 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00 };
unsigned char cmd_it72xx_do_calibration[12] =
	{ 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
unsigned char cmd_it72xx_powersupply_bt[2] = { 0x20, 0x01 };
unsigned char cmd_it72xx_powersupply_ac[2] = { 0x20, 0x00 };

struct it72xx_ts_priv *priv;

#ifdef DEBUG_IT72XX
static const unsigned char cmd_it72xx_cdc_debug[7] = { 0xE1, 0x2D, 0x02, 0x30, 0x68, 0x00, 0x00 };
#endif

static int it72xx_i2c_read(char reg, int bytes, void *dest)
{
	int err;
	struct i2c_msg msg[2];
	unsigned char data[1];
	data[0] = reg;
	msg[0].addr = priv->client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = data;

	msg[1].addr = priv->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = bytes;
	msg[1].buf = dest;

	err = i2c_transfer(priv->client->adapter, msg, 2);

	if (err >= 0)
		return true;
	else
		return false;
}

static int it72xx_i2c_write(char reg, int bytes, void *src)
{
	int ret;
	u8 msg[(IT72XX_MAX_DATA << 1) + 1];
	if (bytes > ((IT72XX_MAX_DATA << 1) + 1))
		return false;
	msg[0] = reg;
	memcpy(&msg[1], src, bytes);
	ret = i2c_master_send(priv->client, msg, bytes + 1);
	if (ret >= 0)
		return true;
	else
		return false;
}

static int it72xx_do_command(unsigned char *cmd, unsigned int cmd_len, unsigned char *read_buff, unsigned int buff_len)
{
	unsigned char buf[IT72XX_MAX_DATA];
	int ret;
	ret = it72xx_wait_for_idle(WAIT_TIMEOUT, buf);
	if (ret == false) {
		printk("%s:wait for device idle timeout\n", __FUNCTION__);
		goto command_err;
	}

	ret = it72xx_i2c_write(CMDBUF_INDEX, cmd_len, (unsigned char *)cmd);
	if (ret == false) {
		printk("%s --> %s failed\n", __FUNCTION__, cmd);
		goto command_err;
	}

	ret = it72xx_wait_for_idle(WAIT_TIMEOUT, buf);
	if (ret == false) {
		printk("%s:wait for device idle timeout\n", __FUNCTION__);
		goto command_err;
	}

	ret = it72xx_i2c_read(CMDRES_INDEX, buff_len, read_buff);
	if (ret == false) {
		printk("%s --> %s failed\n", __FUNCTION__, cmd);
		goto command_err;
	}
	return true;

  command_err:
	return ret;
}

static int it72xx_do_calibration_command(unsigned char *cmd, unsigned int cmd_len, unsigned char *read_buff,
										 unsigned int buff_len)
{
	unsigned char buf[IT72XX_MAX_DATA];
	int ret;
	ret = it72xx_wait_for_idle(WAIT_TIMEOUT, buf);
	if (ret == false) {
		printk("%s:wait for device idle timeout\n", __FUNCTION__);
		goto command_err;
	}

	ret = it72xx_i2c_write(CMDBUF_INDEX, cmd_len, (unsigned char *)cmd);
	if (ret == false) {
		printk("%s --> %s failed\n", __FUNCTION__, cmd);
		goto command_err;
	}
	mdelay(3500);
	ret = it72xx_wait_for_idle(WAIT_TIMEOUT, buf);
	if (ret == false) {
		printk("%s:wait for device idle timeout\n", __FUNCTION__);
		goto command_err;
	}

	ret = it72xx_i2c_read(CMDRES_INDEX, buff_len, read_buff);
	if (ret == false) {
		printk("%s --> %s failed\n", __FUNCTION__, cmd);
		goto command_err;
	}
	return 0;

  command_err:
	return ret;
}

static int it72xx_read_point(struct input_dev *dev)
{
	unsigned int ret;
	unsigned char buf[IT72XX_MAX_DATA];

	ret = it72xx_wait_for_idle(WAIT_TIMEOUT, buf);
	if (ret == false) {
		printk("1 read query buffer failed!\n");
		goto out;
	}
	if (buf[0] & 0x80) {		//New point information available.
//      printk("buf[0] %04x, ", buf[0]);
		ret = it72xx_i2c_read(POINTBUF_INDEX, 5, buf);
		if (ret == false) {
			printk("read point information buffer failed!\n");
			goto out;
		}
		if (buf[0] & 0xF0)		//the format tag of point is 0000b
			goto out;

		if (buf[1] & 0x01)		//The finger is touched the screen
			goto out;

		if (buf[0] & 0x01)		//Point 0 information is available
		{
			int xraw, yraw, xtmp, ytmp;
			xraw = ((buf[3] & 0x0F) << 8) + buf[2];
			yraw = ((buf[3] & 0xF0) << 4) + buf[4];
			xtmp = ts_xres - xraw;
			ytmp = ts_yres - yraw;

			if (xtmp < 0)
				xtmp = 0;
			else if (xtmp > ts_xres)
				xtmp = ts_xres;

			if (ytmp < 0)
				ytmp = 0;
			else if (ytmp > ts_yres)
				ytmp = ts_yres;

			//  printk("x = %d, y = %d\n", xtmp, ytmp);

			input_report_abs(priv->input, ABS_X, xtmp);
			input_report_abs(priv->input, ABS_Y, ytmp);

			input_report_key(priv->input, BTN_TOUCH, 1);
			input_report_abs(priv->input, ABS_PRESSURE, 1);
			
			input_report_key(priv->input, 0x34, 1);

			input_sync(priv->input);
		} else {
			input_report_key(priv->input, BTN_TOUCH, 0);
			input_report_abs(priv->input, ABS_PRESSURE, 0);
			input_sync(priv->input);
		}
	} else {
		/*
		   printk("buf[0] %04x, ", buf[0]);
		   ret = it72xx_i2c_read(POINTBUF_INDEX, 5, buf);
		   int xraw, yraw, xtmp, ytmp;
		   xraw = ((buf[3] & 0x0F) << 8) + buf[2];
		   yraw = ((buf[3] & 0xF0) << 4) + buf[4];
		   xtmp = ts_xres - xraw;
		   ytmp = ts_yres - yraw;

		   printk("x = %d, y = %d\n", xtmp, ytmp);
		 */
	}
	return 0;
  out:
	return 0;
}

static void it72xx_ts_poscheck(struct work_struct *work)
{
	it72xx_read_point(priv->input);

#ifdef ENABLE_INTERRUPT_MODE
	enable_irq(priv->irq);
#endif
}

#ifndef ENABLE_INTERRUPT_MODE
static void touch_it72xx_timer(unsigned long data)
{
	schedule_delayed_work(&priv->work, HZ / 1000);
	mod_timer(&touch_timer, jiffies + 1);
}
#endif

#ifdef DEBUG_IT72XX
static void cdc_touch_it72xx_timer(unsigned long data)
{
	schedule_delayed_work(&priv->work_cdc, HZ / 0.5);
	mod_timer(&cdc_touch_timer, jiffies + 1);
}

static void cdc_work_func(struct work_struct *work)
{
	it72xx_get_cdcinfo(priv->input);

}

static int it72xx_get_cdcinfo(struct input_dev *dev)
{
	unsigned char buf[90];
	unsigned int ret;
	unsigned int i;

	ret = it72xx_do_command(cmd_it72xx_cdc_debug, sizeof(cmd_it72xx_cdc_debug), buf, sizeof(buf));

	printk("cdc buf=:\n");
	for (i = 0; i < 90; i += 2) {
		if (i != 0 && !(i % 18))
			printk("\n");
		printk("0x%02x%02x, ", buf[i + 1], buf[i]);
	}
	printk("\n");

	return ret;
}
#endif

#if 0
static void it72xx_set_powersupply(struct work_struct *work)
{
	int ret;
	unsigned long val;
	unsigned char buf[2];
	val = (readl(S3C64XX_GPLDAT) & (0x1 << 7)) >> 7;
	if (val != powersupply)
		powersupply_change = 1;
	powersupply = val;

	if (powersupply == 0 && powersupply_change == 1)
		ret = it72xx_do_command(cmd_it72xx_powersupply_bt, sizeof(cmd_it72xx_powersupply_bt), buf, sizeof(buf));
	else if (powersupply == 1 && powersupply_change == 1) {
		ret = it72xx_do_command(cmd_it72xx_powersupply_ac, sizeof(cmd_it72xx_powersupply_ac), buf, sizeof(buf));
	}

	powersupply_change = 0;
	//return ret;
}
#endif

static void powersupply_it72xx_timer(unsigned long data)
{
	schedule_delayed_work(&priv->powersupply_work, HZ / 1);
	mod_timer(&powersupply_timer, jiffies + 1);
}

#ifdef ENABLE_INTERRUPT_MODE
static irqreturn_t it72xx_ts_isr(int irq, void *dev_id)
{
	disable_irq_nosync(irq);

	schedule_delayed_work(&priv->work, HZ / 1000);

	return IRQ_HANDLED;
}
#endif

static int it72xx_wait_for_idle(unsigned int ms, unsigned char *buf)
{
	unsigned int timeout = 0;
	unsigned int ret = false;
	do {
		buf[0] &= 0x0;
		ret = it72xx_i2c_read(QUERYBUF_INDEX, 1, buf);
		//printk("query buffer value=%x\n",buf[0]);
		if (ret == false) {
			buf[0] = 0xff;
			//printk("2 read query buffer failed!\n");
		}
		mdelay(1);
		timeout++;
	} while ((buf[0] & 0x01) && timeout < ms);
	return ret;
}

static int it72xx_fwinfo(void)
{
	unsigned char buf[9];
	int ret;
	int i;
	printk("%s\n", __FUNCTION__);

	ret = it72xx_do_command(cmd_it72xx_get_fwinfo, sizeof(cmd_it72xx_get_fwinfo), buf, sizeof(buf));
	for (i = 0; i < 9; i++)
		printk("buf[%d]=%x\n", i, buf[i]);

	return ret;
}

static int it72xx_calibration(void)
{
	unsigned char buf[2];
	int ret;
	printk("%s\n", __FUNCTION__);
	ret = it72xx_do_calibration_command(cmd_it72xx_do_calibration, sizeof(cmd_it72xx_do_calibration), buf, sizeof(buf));

	return ret;
}

struct update_data {
	unsigned char fw[32 * 1024];
	unsigned long fw_size;
	unsigned char cfg[16 * 1024];
	unsigned long cfg_size;
	unsigned char reversed[1016];
};

unsigned int it72xx_set_version(unsigned int version)
{
	printk("%s:blank function\n", __FUNCTION__);
	return 0;
}

unsigned int it72xx_get_version(void)
{
	it72xx_fwinfo();
	return 0;
}

unsigned int it72xx_do_calibration(unsigned int calibration)
{

	disable_irq(priv->irq);
	it72xx_calibration();
	mdelay(400);
	enable_irq(priv->irq);
	return 0;
}

unsigned int it72xx_set_calibration(void)
{
	printk("%s:blank function\n", __FUNCTION__);
	return 0;
}

static const struct it72xx_proc {
	char *module_name;
	unsigned int (*it72xx_get) (void);
	unsigned int (*it72xx_set) (unsigned int);
} it72xx_modules[] = {
	{
	"it72xx_version", it72xx_get_version, it72xx_set_version}, {
"it72xx_calibration", it72xx_do_calibration, it72xx_set_calibration},};

ssize_t it72xx_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	char *out = page;
	unsigned int it72xx;
	const struct it72xx_proc *proc = data;
	ssize_t len;

	it72xx = proc->it72xx_get();

	out += sprintf(out, "%d\n", it72xx);

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

size_t it72xx_proc_write(struct file * filp, const char __user * buf, unsigned long len, void *data)
{
	char command[PROC_CMD_LEN];
	const struct it72xx_proc *proc = data;

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
		proc->it72xx_set(1);
	} else if (strnicmp(command, "off", 3) == 0 || strnicmp(command, "0", 1) == 0) {
		proc->it72xx_set(0);
	} else {
		return -EINVAL;
	}

	return len;
}

struct file_operations it72xx_fops = {
	.owner = THIS_MODULE,
	.write = it72xx_firmware_write,
};

static int it72xx_proc_init()
{

	unsigned int i;
	struct proc_dir_entry *update_proc_root = proc_mkdir("it72xx_update", 0);
	struct proc_dir_entry *proc;
	if (!update_proc_root) {
		return -1;
	}

	//update_proc_root->owner = THIS_MODULE;
	proc = create_proc_entry("buff", S_IWUGO, update_proc_root);
	if (!proc) {
		printk("proc_register error!!!\n");
		return -1;
	}

	proc->proc_fops = &it72xx_fops;
	for (i = 0; i < (sizeof(it72xx_modules) / sizeof(*it72xx_modules)); i++) {
		struct proc_dir_entry *proc;
		mode_t mode;

		mode = 0;
		if (it72xx_modules[i].it72xx_get) {
			mode |= S_IRUGO;
		}
		if (it72xx_modules[i].it72xx_set) {
			mode |= S_IWUGO;
		}

		proc = create_proc_entry(it72xx_modules[i].module_name, mode, update_proc_root);
		if (proc) {
			proc->data = (void *)(&it72xx_modules[i]);
			proc->read_proc = it72xx_proc_read;
			proc->write_proc = it72xx_proc_write;
		}
	}

	return 0;
}

static int it72xx_enter_update_mode(void)
{
	unsigned char buf[2];
	unsigned int ret;

	printk("enter %s\n", __FUNCTION__);
	ret = it72xx_do_command(cmd_it72xx_enter_update_mode, sizeof(cmd_it72xx_enter_update_mode), buf, sizeof(buf));

	return ret;
}

static int it72xx_exit_update_mode(void)
{
	int ret;
	unsigned char buf[2];

	printk("enter %s\n", __FUNCTION__);
	ret = it72xx_do_command(cmd_it72xx_exit_update_mode, sizeof(cmd_it72xx_exit_update_mode), buf, sizeof(buf));

	return 0;
}

static int it72xx_update_fw(unsigned char *fw, int fw_size)
{
	//int ret, i, j, k;
	int ret = 0, j;
	unsigned char buf[2];
	unsigned char cmd_it72xx_write_flash[130] = { 0x62, 128 };
	printk("enter %s\n", __FUNCTION__);

	for (j = 0; j < fw_size; j += MEM_SIZE) {
		memcpy(cmd_it72xx_write_flash + 2, fw + j, 128);
		ret = it72xx_do_command(cmd_it72xx_write_flash, sizeof(cmd_it72xx_write_flash), buf, sizeof(buf));
		if (ret < 0) {
			printk("cmd_it72xx_write_fw_to_mem error!!!\n");
			goto err;
		}
	}

  err:
	return ret;
}

static int it72xx_update_cfg(unsigned char *cfg, int cfg_size)
{
	int ret = 0, j;
	unsigned char buf[2];
	unsigned char cmd_it72xx_write_flash[130] = { 0x62, 128 };
	printk("%s\n", __FUNCTION__);
	for (j = 0; j < cfg_size; j += MEM_SIZE) {
		printk("0x%08x:\n", j);
		memcpy(cmd_it72xx_write_flash + 2, cfg + j, 128);
		ret = it72xx_do_command(cmd_it72xx_write_flash, sizeof(cmd_it72xx_write_flash), buf, sizeof(buf));
		if (ret < 0) {
			printk("cmd_it72xx_write_fw_to_mem error!!!\n");
			goto err;
		}
	}

  err:
	return ret;

}

static int it72xx_update_firmware(struct update_data *ud)
{
	int ret;
	unsigned char buf[3];
	unsigned char fw_signal[8] = "IT72XXFW";
	unsigned char cfg_signal[6] = "CONFIG";
	unsigned int cfg_off;
	printk("enter %s\n", __FUNCTION__);

	if (strncmp(fw_signal, ud->fw, sizeof(fw_signal) != 0)
		|| (strncmp(cfg_signal, ud->cfg + ud->cfg_size - 16, sizeof(cfg_signal) != 0))
		|| (ud->fw_size % 128 != 0)
		|| (ud->cfg_size % 128 != 0)) {
		printk("firmware file format is error!!\n");
		return -1;
	}
#ifdef ENABLE_INTERRUPT_MODE
	disable_irq(priv->irq);
#endif

	printk("1\n");
	ret = it72xx_enter_update_mode();
	if (ret < 0) {
		printk("it72xx_enter_update_mode error!!!\n");
		goto err;
	}

	printk("2\n");
	ret = it72xx_do_command(cmd_it72xx_get_flash_size, sizeof(cmd_it72xx_get_flash_size), buf, sizeof(buf));
	if (ret < 0) {
		printk("it72xx_get_flash_size error!!!\n");
		goto err;
	}
	flash_size = (buf[2] & 0xFF) << 8 + buf[1];
	printk("flash_size:%x\n", flash_size);

	printk("3\n");
	printk("set flash offset of firmware:\n");
	ret = it72xx_do_command(cmd_it72xx_set_flash_off, sizeof(cmd_it72xx_set_flash_off), buf, sizeof(buf));
	if (ret < 0) {
		printk("set flash offset of firmware error!!!\n");
		goto err;
	}

	ret = it72xx_update_fw(ud->fw, ud->fw_size);
	if (ret < 0) {
		printk("it72xx_update_fw error!!!\n");
		goto err;
	}
	mdelay(50);

	cfg_off = flash_size - ud->cfg_size;
	printk("set flash offset of config=0x%x\n", cfg_off);
	cmd_it72xx_set_flash_off[2] = cfg_off & 0x00ff;
	cmd_it72xx_set_flash_off[3] = (cfg_off & 0xff00) >> 8;
	ret = it72xx_do_command(cmd_it72xx_set_flash_off, sizeof(cmd_it72xx_set_flash_off), buf, sizeof(buf));

	if (ret < 0) {
		printk("set flash offset of config error!!!\n");
		goto err;
	}
	ret = it72xx_update_cfg(ud->cfg, ud->cfg_size);
	if (ret < 0) {
		printk("it72xx_update_cfg error!!!\n");
		goto err;
	}
	mdelay(50);

	ret = it72xx_exit_update_mode();
	if (ret < 0) {
		printk("it72xx_exit_update_mode error!!!\n");
		goto err;
	}
	printk("do it72xx_reset\n");
	ret = it72xx_reset();
	if (ret < 0) {
		printk("it72xx_reset error!!!\n");
		goto err;
	}

	printk("after  it72xx_reset\n");
#ifdef ENABLE_INTERRUPT_MODE
	enable_irq(priv->irq);
#endif

	printk("after enable_irq\n");
	return 0;

  err:
	return ret;
}

int it72xx_firmware_write(struct file *file, const char __user * buffer, size_t count, loff_t * ppos)
{
	struct update_data *ud = kzalloc(sizeof(struct update_data), GFP_KERNEL);
	if (count != sizeof(struct update_data)) {
		printk("update firmware buffer is not correct!!!\n");
		return -EINVAL;
	}

	if (copy_from_user(ud, buffer, count)) {
		printk("copy_from_user error!\n");
		return -EFAULT;
	}

#ifdef DEBUG_IT72XX
	printk("fw_size: %d, cfg_size: %d\n", ud->fw_size, ud->cfg_size);
	int i;
	for (i = 0; i < ud->fw_size; i += 16) {
		printk("%08x: %02x%02x %02x%02x	%02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x\n",
			   i, *(ud->fw + (i + 0)), *(ud->fw + (i + 1)), *(ud->fw + (i + 2)), *(ud->fw + (i + 3)),
			   *(ud->fw + (i + 4)), *(ud->fw + (i + 5)), *(ud->fw + (i + 6)), *(ud->fw + (i + 7)), *(ud->fw + (i + 8)),
			   *(ud->fw + (i + 9)), *(ud->fw + (i + 10)), *(ud->fw + (i + 11)), *(ud->fw + (i + 12)),
			   *(ud->fw + (i + 13)), *(ud->fw + (i + 14)), *(ud->fw + (i + 15)));
	}

	for (i = 0; i < ud->cfg_size; i += 16) {
		printk("%08x: %02x%02x %02x%02x	%02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x\n",
			   i, *(ud->cfg + (i)), *(ud->cfg + (i + 1)), *(ud->cfg + (i + 2)), *(ud->cfg + (i + 3)),
			   *(ud->cfg + (i + 4)), *(ud->cfg + (i + 5)), *(ud->cfg + (i + 6)), *(ud->cfg + (i + 7)),
			   *(ud->cfg + (i + 8)), *(ud->cfg + (i + 9)), *(ud->cfg + (i + 10)), *(ud->cfg + (i + 11)),
			   *(ud->cfg + (i + 12)), *(ud->cfg + (i + 13)), *(ud->cfg + (i + 14)), *(ud->cfg + (i + 15)));
	}
#endif

	if (it72xx_update_firmware(ud)) {
		printk("updatefirmware error!!!\n");
		return -EINVAL;
	}
	kfree(ud);

	return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int it72xx_get_2d_resolution(struct input_dev *dev)
{
	unsigned char buf[7];
	unsigned int ret;
	ret = it72xx_do_command(cmd_it72xx_get_2dresolution, sizeof(cmd_it72xx_get_2dresolution), buf, sizeof(buf));
	ts_xres = ((buf[3] & 0xFF) << 8) + buf[2];
	ts_yres = ((buf[5] & 0xFF) << 8) + buf[4];
	printk("2D resolution x:%d, y:%d\n", ts_xres, ts_yres);

	return ret;
}

static int it72xx_reset(void)
{
	unsigned char buf[2];
	unsigned int ret, i;

	printk("enter %s\n", __FUNCTION__);
	ret = it72xx_do_command(cmd_it72xx_reset, sizeof(cmd_it72xx_reset), buf, sizeof(buf));
	//for(i=0;i<2;i++)         
	//  printk("buf[%d]=%x\n",i,buf[i]);
	mdelay(200);
	return ret;
}

static int it72xx_setinterruptmode(struct input_dev *dev)
{
	unsigned int ret;
	unsigned char buf[2];
#ifdef ENABLE_INTERRUPT_MODE
	ret = it72xx_do_command(cmd_it72xx_enable_interrupt, sizeof(cmd_it72xx_enable_interrupt), buf, sizeof(buf));
#else
	ret = it72xx_do_command(cmd_it72xx_disable_interrupt, sizeof(cmd_it72xx_disable_interrupt), buf, sizeof(buf));

#endif

	return ret;
}

static void ts_reset_gpio()
{
	/*
	   unsigned long val;
	   val = readl(S5PV210_GPJ3CON);
	   val &= ~(0x3 << 9 * 2);
	   writel(val | (0x1 << 9 * 2), S3C64XX_GPFCON);

	   //set TOUCH_RES to high
	   val = readl(S3C64XX_GPFDAT);
	   val &= ~(0x1 << 0x9);
	   writel(val | (0x1 << 0x9), S3C64XX_GPFDAT);
	   mdelay(100);

	   #ifdef TOUCH_RESET
	   //set TOUCH_RES to low
	   val = readl(S3C64XX_GPFDAT);
	   val &= ~(0x1 << 9);
	   writel(val, S3C64XX_GPFDAT);
	   mdelay(200);
	   //set TOUCH_RES to high
	   val = readl(S3C64XX_GPFDAT);
	   val &= ~(0x1 << 9);
	   writel(val | (0x1 << 0x9), S3C64XX_GPFDAT);
	   #endif

	   s3c_gpio_cfgpin(S3C64XX_GPN(9), S3C_GPIO_SFN(2));
	   s3c_gpio_setpull(S3C64XX_GPN(9), S3C_GPIO_PULL_UP);

	   mdelay(200);
	 */

	int err = 0;
	err = gpio_request(S5PV210_GPH0(7), "GPH0");
	if (!err) {
		s3c_gpio_cfgpin(S5PV210_GPH0(7), (0xf << 28));
		s3c_gpio_setpull(S5PV210_GPH0(7), S3C_GPIO_PULL_NONE);
	}
}

static int it72xx_suspend()
{
	disable_irq(priv->irq);
	return 0;
}

static int it72xx_resume()
{

	enable_irq(priv->irq);
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
void it72xx_early_suspend(struct early_suspend *h)
{
	printk("it72xx enter early suspend\n");
#ifdef ENABLE_INTERRUPT_MODE
	//disable_irq(priv->irq);
#endif
}

void it72xx_late_resume(struct early_suspend *h)
{
	printk("it72xx enter late resume\n");
	//ts_reset_gpio();

#ifdef ENABLE_INTERRUPT_MODE
	//enable_irq(priv->irq);
#endif
}
#endif

static int it72xx_ts_open(struct input_dev *dev)
{
	return 0;
}

static void it72xx_ts_close(struct input_dev *dev)
{

#ifdef ENABLE_INTERRUPT_MODE
	disable_irq(priv->irq);
#endif
	if (cancel_delayed_work_sync(&priv->work)) {
#ifdef ENABLE_INTERRUPT_MODE
		enable_irq(priv->irq);
#endif
	}
#ifdef ENABLE_INTERRUPT_MODE
	enable_irq(priv->irq);
#endif
}

static int __init it72xx_ts_probe(struct i2c_client *client, const struct i2c_device_id *idp)
{
	struct input_dev *input_dev;
	int error;
	int ret = false;
	ts_reset_gpio();
	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		error = -ENOMEM;
		goto fail;
	}

	i2c_set_clientdata(client, priv);
	input_dev = input_allocate_device();

	if (!input_dev) {
		ret = -ENOMEM;
		goto fail;
	}

	input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
	set_bit(0x34, input_dev->keybit);

	input_set_abs_params(input_dev, ABS_X, 0, 800, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, 600, 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE, 0, 1, 0, 0);

	input_dev->name = client->name;
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;
	input_dev->open = it72xx_ts_open;
	input_dev->close = it72xx_ts_close;
	input_set_drvdata(input_dev, priv);
	priv->client = client;
	priv->input = input_dev;

	ret = input_register_device(input_dev);

	if (ret) {
		ret = -EIO;
		goto fail;
	}
	dev_set_drvdata(&client->dev, priv);
	it72xx_proc_init();
	INIT_DELAYED_WORK(&priv->work, it72xx_ts_poscheck);
	//INIT_DELAYED_WORK(&priv->powersupply_work, it72xx_set_powersupply);

#ifdef DEBUG_IT72XX
	INIT_DELAYED_WORK(&priv->work_cdc, cdc_work_func);
	spin_lock_init(&it72xx_lock);
	it72xx_identifycapsensor();
	setup_timer(&cdc_touch_timer, cdc_touch_it72xx_timer, (unsigned long)priv);
	mod_timer(&cdc_touch_timer, jiffies + 1);
#endif
	ret = it72xx_get_2d_resolution(priv->input);
	if (ret == false) {
		ret = -EIO;
		goto fail;
	}
	ret = it72xx_setinterruptmode(priv->input);
	if (ret == false) {
		ret = -EIO;
		goto fail;
	}

	it72xx_reset();

	udelay(1000);

#ifdef ENABLE_INTERRUPT_MODE
	priv->irq = client->irq;
	//error = request_irq(priv->irq, it72xx_ts_isr, IRQF_TRIGGER_FALLING, "it72xx-action", priv);
	error = request_irq(priv->irq, it72xx_ts_isr, IRQF_TRIGGER_LOW, "it72xx-action", priv);
	if (error) {
		goto fail;
	}
#else
	setup_timer(&touch_timer, touch_it72xx_timer, (unsigned long)priv);
	mod_timer(&touch_timer, jiffies + 1);
#endif

	//setup_timer(&powersupply_timer, powersupply_it72xx_timer, (unsigned long) priv);
	//mod_timer(&powersupply_timer, jiffies + 1);
#ifdef CONFIG_HAS_EARLYSUSPEND
	es.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	es.suspend = it72xx_early_suspend;
	es.resume = it72xx_late_resume;
	register_early_suspend(&es);
#endif

	return 0;

  fail:
	printk("ITE touch driver unregister\n");
	kfree(priv);
	input_free_device(input_dev);
	return ret;
}

static int it72xx_ts_remove(struct i2c_client *client)
{

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&es);
#endif

	disable_irq(priv->irq);
	free_irq(priv->irq, priv);
	input_unregister_device(priv->input);
	kfree(priv);

	return 0;
}

static const struct i2c_device_id it72xx_ts_id[] = {
	{"it72xx", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, it72xx_ts_id);

static struct i2c_driver it72xx_ts_driver = {
	.driver = {
			   .owner = THIS_MODULE,
			   .name = "it72xx",
			   },
	.probe = it72xx_ts_probe,
	.remove = it72xx_ts_remove,
	.id_table = it72xx_ts_id,
	.suspend = it72xx_suspend,
	.resume = it72xx_resume,

};

static char banner[] __initdata = KERN_INFO "I2C IT72XX Touch Screen driver\n";

static int __init it72xx_ts_init(void)
{
	printk(banner);
	return i2c_add_driver(&it72xx_ts_driver);
}

static void __exit it72xx_ts_exit(void)
{
	i2c_del_driver(&it72xx_ts_driver);
}

MODULE_DESCRIPTION("ITE Touchscreen driver");
MODULE_AUTHOR("Sno <tmsbg-bsp1@foxconn.com>");
MODULE_LICENSE("GPL");

module_init(it72xx_ts_init);
module_exit(it72xx_ts_exit);
