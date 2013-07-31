/* 
 * linux/arch/arm/mach-s5pv210/$(VENUS_PROJECT_DIR)/dev-gpiokeys.c
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
#include <linux/platform_device.h>
#include <linux/venus/gpio_keys.h>
#include <linux/input.h>

#include <mach/map.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <asm/mach/irq.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>

#include "input.h"

#if 1

static struct gpio_keys_button gpio_keys[] = {
	
	{
	     .code   = GPIO_KEY_POWER,
	     .gpio   = S5PV210_GPH0(1),
	     .desc   = "POWER button",
	     .type   = EV_KEY,
	     .active_low     = 1,
	     .debounce_interval      = 10,
	     .timer_interval = 50,
	     .longpress_time = 6000,
	     .key_status = "power_key_status",
	     .wakeup = 1,
	},
	{
		.code	= GPIO_KEY_VOLDN, 
		.gpio	= S5PV210_GPH3(3),
		.desc	= "Volume down button", 
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 10,
		.longpress_time = 2000,
		.key_status = "voldn_key_status",
	},
	{
		.code	= GPIO_KEY_VOLUP, 
		.gpio	= S5PV210_GPH3(4),
		.desc	= "Volume up button", 
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 10,
		.longpress_time = 2000,
		.key_status = "volup_key_status",
	},

	//add by luis
	{	//key map define in framework /system/usr/keylayout/s3c-keypad.kl
		.code = 5,
		.gpio	= S5PV210_GPH1(2),
		.desc	= "Serch button", 
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 1900,
		.key_status = "serch_key_status",
	},

		{
		.code = 34,
		.gpio	= S5PV210_GPH3(1),
		.desc	= "Back button", 
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 1900,
		.key_status = "back_key_status",
	},

		{
		.code = 6,
		.gpio	= S5PV210_GPH3(6),
		.desc	= "Home button", 
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 1900,
		.key_status = "home_key_status",
	},
		{	
		.code = 50,
		.gpio	= S5PV210_GPH3(5),
		.desc	= "Menu button", 
		.type	= EV_KEY,
		.active_low	= 1,
		.debounce_interval	= 10,
		.timer_interval	= 50,
		.longpress_time = 2000,
		.key_status = "menu_key_status",
	},

	//add by luis
#if 0
	{
		.code   = GPIO_SW_USB,
		.gpio   = S5PV210_GPH0(6),
		.desc   = "USB button",
		.type   = EV_SW,
		.active_low     = 0,
		.debounce_interval      = 10,
		.timer_interval = 200,
		.longpress_time = 0,
		.key_status = "usb_key_status",
	},

	{
		.code   = GPIO_SW_TOUCH,
		.gpio   = S5PV210_GPH3(5),
		.desc   = "TOUCH switch button",
		.type   = EV_SW,
		.active_low     = 0,
		.debounce_interval      = 10,
		.timer_interval = 200,
		.longpress_time = 0,
		.key_status = "touch_status",
	},
#endif	
};

static void gpiokeys_init(void)
{
	s3c_gpio_cfgpin(S5PV210_GPH3(4), S3C_GPIO_SFN(0xf));
	set_irq_type(IRQ_EINT(28), IRQ_TYPE_LEVEL_LOW);

	s3c_gpio_cfgpin(S5PV210_GPH3(3), S3C_GPIO_SFN(0xf));
	set_irq_type(IRQ_EINT(27), IRQ_TYPE_LEVEL_LOW);

	
	//add by luis
	s3c_gpio_cfgpin(S5PV210_GPH1(2), S3C_GPIO_SFN(0xf));
	set_irq_type(IRQ_EINT(10), IRQ_TYPE_LEVEL_LOW);

	s3c_gpio_cfgpin(S5PV210_GPH3(1), S3C_GPIO_SFN(0xf));
	set_irq_type(IRQ_EINT(25), IRQ_TYPE_LEVEL_LOW);

	s3c_gpio_cfgpin(S5PV210_GPH3(6), S3C_GPIO_SFN(0xf));
	set_irq_type(IRQ_EINT(30), IRQ_TYPE_LEVEL_LOW);

	s3c_gpio_cfgpin(S5PV210_GPH3(5), S3C_GPIO_SFN(0xf));
	set_irq_type(IRQ_EINT(29), IRQ_TYPE_LEVEL_LOW);

	//add by luis
}
#endif

static struct gpio_keys_platform_data gpiokeys_platdata = 
{
	.buttons	= gpio_keys,
	.nbuttons	= ARRAY_SIZE(gpio_keys),
	.init		= gpiokeys_init,
	.suspend	= NULL,
	.resume		= NULL,
};

struct platform_device gpiokeys_device = 
{
	.name		= "gpio-keys",
	.id		= 0,
	.dev		= 
	{
		.platform_data	= &gpiokeys_platdata,
	},
};

