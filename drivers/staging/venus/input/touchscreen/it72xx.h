/*
 * Touch Screen driver for Renesas MIGO-R Platform
 *
 * Copyright (c) 2008 Magnus Damm
 * Copyright (c) 2007 Ujjwal Pande <ujjwal@kenati.com>,
 *  Kenati Technologies Pvt Ltd.
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
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>

//command index
#define QUERYBUF_INDEX   0x80  //read only
#define CMDBUF_INDEX 	 0x20  //write only
#define CMDRES_INDEX     0xa0  //read only
#define POINTBUF_INDEX   0xe0  //read only
#define IT72XX_MAX_DATA 100
#define WAIT_TIMEOUT 200

//#define DEBUG_IT72XX
#define ENABLE_INTERRUPT_MODE
//#undef ENABLE_INTERRUPT_MODE
#define  TOUCH_RESET

struct it72xx_ts_priv {
	struct i2c_client *client;
	struct input_dev *input;
	struct delayed_work work;
	struct delayed_work powersupply_work;
#ifdef DEBUG_IT72XX
	struct delayed_work work_cdc;
#endif
	int irq;
};
static int it72xx_i2c_read(char reg,int bytes, void *dest);

static int it72xx_enter_update_mode(void);
//static int it72xx_i2c_write(struct it72xx_ts_priv *priv, char reg,int bytes, void *src);
//static int it72xx_i2c_read(unsigned char BufferIndex,unsigned int Length, unsigned char* Data);
//static int it72xx_i2c_write(unsigned char BufferIndex,unsigned int Length, unsigned char* Data);
static int it72xx_i2c_write(char reg,int bytes, void *src);

static void ts_reset_gpio(void);
static int it72xx_reset(void);
static void it72xx_ts_poscheck(struct work_struct *work);
static void it72xx_set_powersupply(struct work_struct *work);
#ifdef ENABLE_INTERRUPT_MODE
static irqreturn_t it72xx_ts_isr(int irq, void *dev_id);
#else
static void touch_it72xx_timer(unsigned long data);
#endif
int it72xx_firmware_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos);
static int it72xx_wait_for_idle(unsigned ms, unsigned char *buf);
//static int it72xx_setinterruptmode(void);
//static int it72xx_read_point(void);
static int it72xx_setinterruptmode(struct input_dev *dev);
static int it72xx_read_point(struct input_dev *dev);
#ifdef DEBUG_IT72XX
static int it72xx_identifycapsensor(void);
static int it72xx_get_cdcinfo(void);
#endif
