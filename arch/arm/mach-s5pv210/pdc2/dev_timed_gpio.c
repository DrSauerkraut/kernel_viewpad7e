/*
 * linux/arch/arm/mach-s5pv210/ch2/dev_timed_gpio.c
 * 
 * Copyright (C) 2010, TMSBG L.H. SW-BSP1
 *
 * Copyright (C) 2011, Venus Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <mach/gpio.h>

#include <linux/venus/timed_gpio.h>

#include <linux/platform_device.h>

	
static struct timed_gpio timed_leds_gpio[] = 
{
	{
		.name = "touch_led",
		.gpio = S5PV210_GPD0(0),
		.max_timeout = 60000,
		.active_low = 0,
	},
//	{
//		.name = "vibrator",
//		.gpio = S5PV210_GPJ3(1),
//		.max_timeout = 60000,
//		.active_low = 0,
//	},
};

struct timed_gpio_platform_data s5pv210_timed_gpio_platdata =
{
	.num_gpios = ARRAY_SIZE(timed_leds_gpio),
	.gpios = timed_leds_gpio,
};


struct platform_device s5pv210_device_gpio_leds = 
{
	.name		= TIMED_GPIO_NAME,
	.id		    = -1,
	.dev		= 
	{
		.platform_data	= &s5pv210_timed_gpio_platdata,
	},
};

