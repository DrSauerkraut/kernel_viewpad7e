/* 
 * linux/arch/arm/mach-s5pv210/$(VENUS_PROJECT_DIR)/dev-smb137b.c
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
#include <linux/venus/charge.h>
#include <linux/input.h>

#include <mach/map.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <asm/mach/irq.h>
#include <asm/io.h>
#include <mach/regs-gpio.h>
#include <mach/gpio.h>
//#include <mach/gpio-bank.h>
#include <plat/gpio-cfg.h>

/*
 * when you change board using now GPIO, just modify  ".gpio", do not change sequence of struct .
 */
static struct charge_gpio gpio_table = {
	.gpio_red_led			= S5PV210_GPH3(7),
	.gpio_green_led			= S5PV210_GPH1(1),
	.gpio_current_limint		= S5PV210_GPJ2(7),
	.gpio_otg_en			= S5PV210_GPJ2(6),
	.gpio_charge_en			= S5PV210_GPJ2(5),
	.gpio_charge_sta		= S5PV210_GPH0(5),
	
	.gpio_red_active		= 1,
	.gpio_green_active		= 1,
	.gpio_current_limint_active	= 1,
	.gpio_otg_en_active		= 1,
	.gpio_charge_en_active		= 0,
	
};

static void gpio_init(void)
{
	s3c_gpio_cfgpin(gpio_table.gpio_charge_sta, S3C_GPIO_SFN(0xF)); //G_INT5
	s3c_gpio_setpull(gpio_table.gpio_charge_sta, S3C_GPIO_PULL_UP);
}

struct charge_platform_data smb137b_platdata = 
{
	.gpios		= &gpio_table,
	.init		= gpio_init,
	.suspend	= NULL,
	.resume		= NULL,
};


