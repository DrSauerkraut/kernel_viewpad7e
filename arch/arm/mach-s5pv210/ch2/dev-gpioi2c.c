/* 
 * linux/arch/arm/mach-s5pv210/$(VENUS_PROJECT_DIR)/dev-gpioi2c.c
 * 
 * Copyright (C) 2011, Venus Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#include <linux/i2c-gpio.h>

#include <mach/map.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <asm/mach/irq.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>

/* For HDMI */
static struct i2c_gpio_platform_data gpioi2c3_platdata = {
	.sda_pin		= S5PV210_GPB(1),
	.scl_pin		= S5PV210_GPB(0),
	.udelay			= 0,
	.timeout		= 0,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

struct platform_device gpioi2c3_device = 
{
	.name		= "i2c-gpio",
	.id		= 3,
	.dev		= 
	{
		.platform_data	= &gpioi2c3_platdata,
	},
};

/* For Camera */
static struct i2c_gpio_platform_data gpioi2c4_platdata = {
	.sda_pin		= S5PV210_GPD0(2),
	.scl_pin		= S5PV210_GPD0(3),
	.udelay			= 0,
	.timeout		= 0,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

struct platform_device gpioi2c4_device = 
{
	.name		= "i2c-gpio",
	.id		= 4,
	.dev		= 
	{
		.platform_data	= &gpioi2c4_platdata,
	},
};

/* For SF */
static struct i2c_gpio_platform_data gpioi2c5_platdata = {
	.sda_pin		= S5PV210_GPC0(4),
	.scl_pin		= S5PV210_GPJ0(6),
	.udelay			= 0,
	.timeout		= 0,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

struct platform_device gpioi2c5_device = 
{
	.name		= "i2c-gpio",
	.id		= 5,
	.dev		= 
	{
		.platform_data	= &gpioi2c5_platdata,
	},
};

/* For NC023 */
static struct i2c_gpio_platform_data gpioi2c6_platdata = {
	.sda_pin		= S5PV210_GPF3(5),
	.scl_pin		= S5PV210_GPF3(4),
	.udelay			= 0,
	.timeout		= 0,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

struct platform_device gpioi2c6_device = 
{
	.name		= "i2c-gpio",
	.id		= 6,
	.dev		= 
	{
		.platform_data	= &gpioi2c6_platdata,
	},
};

/* For Camera */
static struct i2c_gpio_platform_data gpioi2c7_platdata = {
	.sda_pin		= S5PV210_GPD0(2),
	.scl_pin		= S5PV210_GPD0(3),
	.udelay			= 0,
	.timeout		= 0,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

struct platform_device gpioi2c7_device = 
{
	.name		= "i2c-gpio",
	.id		= 7,
	.dev		= 
	{
		.platform_data	= &gpioi2c7_platdata,
	},
};

