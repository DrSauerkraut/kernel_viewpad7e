/* 
 * linux/arch/arm/mach-s5pv210/$(VENUS_PROJECT_DIR)/power_control.c
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
#include <linux/device.h>
#include <linux/err.h>
#include <mach/map.h>
#include <plat/cpu.h>
#include <asm/io.h>
#include <mach/regs-gpio.h>
#include <mach/gpio.h>
//#include <mach/gpio-bank.h>
#include <plat/gpio-cfg.h>
#include <linux/string.h>
#include <linux/gpio.h>
#include <linux/venus/power_control.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/platform_device.h>
	
struct power_module init_power_module[] = {
	{
		.power_name 		= WIFI_POWER,
		.power_contrl_gpio	= S5PV210_GPH3(0),
		.active			= 1,
		.refer_count		= 0,
		.type			= NORMAL_SUPPLY,
		.always_on		= 0,
		.boot_status		= 1,
	},
	{
		.power_name 		= MMC0_CAED_POWER,
		.power_contrl_gpio	= 0,
		.active			= 0,
		.refer_count		= 0,
		.type			= NORMAL_SUPPLY,
		.always_on		= 1,
		.boot_status		= 1,
	},
	{
		.power_name 		= MMC1_CAED_POWER,
		.power_contrl_gpio	= 0,
		.active			= 0,
		.refer_count		= 0,
		.type			= NORMAL_SUPPLY,
		.always_on		= 0,
		.boot_status		= 1,
	},
	{
		.power_name 		= MMC2_CAED_POWER,
		.power_contrl_gpio	= S5PV210_GPJ4(4),
		.active			= 0,
		.refer_count		= 0,
		.type			= NORMAL_SUPPLY,
		.always_on		= 0,
		.boot_status		= 1,
	},
	{
		.power_name 		= LCD_POWER,
		.power_contrl_gpio	= S5PV210_GPJ2(4),
		.active			= 0,
		.refer_count		= 0,
		.type			= NORMAL_SUPPLY,
		.always_on		= 0,
		.boot_status		= 1,
	},
	{
		.power_name 		= BACKLIGHT_POWER,
		.power_contrl_gpio	= S5PV210_GPJ3(0),
		.active			= 1,
		.refer_count		= 0,
		.type			= NORMAL_SUPPLY,
		.always_on		= 0,
		.boot_status		= 1,
	},
	{
		.power_name 		= HDMI_POWER,
		.power_contrl_gpio	= S5PV210_GPJ2(0),
		.active			= 1,
		.refer_count		= 0,
		.type			= NORMAL_SUPPLY,
		.always_on		= 0,
		.boot_status		= 1,
	},
	{
		.power_name 		= IOC_POWER,
		.power_contrl_gpio	= S5PV210_GPJ3(6),
		.active			= 1,
		.refer_count		= 0,
		.type			= NORMAL_SUPPLY,
		.always_on		= 0,
		.boot_status		= 1,
	},
	{
		.power_name 		= CAM_1_8V,
		.power_contrl_gpio	= S5PV210_GPJ1(5),
		.active			= 0,
		.refer_count		= 0,
		.type			= NORMAL_SUPPLY,
		.always_on		= 0,
		.boot_status		= 1,
	},
	{
		.power_name 		= CAM_1_5V,
		.power_contrl_gpio	= S5PV210_GPB(3),
		.active			= 1,
		.refer_count		= 0,
		.type			= NORMAL_SUPPLY,
		.always_on		= 0,
		.boot_status		= 1,
	},
	{
		.power_name 		= GPS_POWER,
		.power_contrl_gpio	= S5PV210_GPJ3(3),
		.active			= 1,
		.refer_count		= 0,
		.type			= NORMAL_SUPPLY,
		.always_on		= 0,
		.boot_status		= 1,
	},
	{
		.power_name 		= SET_3G_POWER,
		.power_contrl_gpio	= S5PV210_GPB(5),
		.active			= 1,
		.refer_count		= 0,
		.type			= NORMAL_SUPPLY,
		.always_on		= 0,
		.boot_status		= 1,
	},
	{
		.power_name 		= ALIVE_1_1V,
		.refer_count		= 0,
		.type			= PMIC_SUPPLY,
		.supply_source		= P_MAX8698_LDO2,
		.always_on		= 1,
		.boot_status		= 1,
	},
	{
		.power_name 		= OTG_1_1V,
		.refer_count		= 0,
		.type			= PMIC_SUPPLY,
		.supply_source		= P_MAX8698_LDO3,
		.always_on		= 0,
		.boot_status		= 1,
	},
	{
		.power_name 		= CAM_2_8V,		
		.refer_count		= 0,
		.type			= PMIC_SUPPLY,
		.supply_source		= P_MAX8698_LDO4,
		.always_on		= 0,
		.boot_status		= 1,
	},
	{
		.power_name 		= VDD_MMC_3_3V,		
		.refer_count		= 0,
		.type			= PMIC_SUPPLY,
		.supply_source		= P_MAX8698_LDO5,
		.always_on		= 1,
		.boot_status		= 1,
	},
	{
		.power_name 		= VDD_LCD,
		.refer_count		= 0,
		.type			= PMIC_SUPPLY,
		.supply_source		= P_MAX8698_LDO6,
		.always_on		= 0,
		.boot_status		= 1,
	},
	{
		.power_name 		= VDD_TOUCH_POWER,		
		.refer_count		= 0,
		.type			= PMIC_SUPPLY,
		.supply_source		= P_MAX8698_LDO7,
		.always_on		= 0,
		.boot_status		= 1,
	},
	{
		.power_name 		= OTG_3_3V,		
		.refer_count		= 0,
		.type			= PMIC_SUPPLY,
		.supply_source		= P_MAX8698_LDO8,
		.always_on		= 0,
		.boot_status		= 1,
	},
	{
		.power_name 		= SYS_3_3V,		
		.refer_count		= 0,
		.type			= PMIC_SUPPLY,
		.supply_source		= P_MAX8698_LDO9,
		.always_on		= 1,
		.boot_status		= 1,
	},
	{
		.power_name 		= VCC_ARM_1_2V,		
		.refer_count		= 0,
		.type			= PMIC_SUPPLY,
		.supply_source		= P_MAX8698_BUCK1,
		.always_on		= 1,
		.boot_status		= 1,
	},
	{
		.power_name 		= VCC_1_2V,	
		.refer_count		= 0,
		.type			= PMIC_SUPPLY,
		.supply_source		= P_MAX8698_BUCK2,
		.always_on		= 1,
		.boot_status		= 1,
	},
	{
		.power_name 		= VCC_1_8V,		
		.refer_count		= 0,
		.type			= PMIC_SUPPLY,
		.supply_source		= P_MAX8698_BUCK3,
		.always_on		= 1,
		.boot_status		= 1,
	},

};

struct sys_total_power_module  sys_power_module = {
	.modules = init_power_module,
	.num_power_module = ARRAY_SIZE(init_power_module),
};


struct platform_device power_control_device = {
	.name             = "sys-power-control",
	.id		= 0,
	.dev = {
		.platform_data    = &sys_power_module,
	}
};

