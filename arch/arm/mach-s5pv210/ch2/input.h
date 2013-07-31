/*
 * linux/arch/arm/mach-s5pv210/$(VENUS_PROJECT_DIR)/input.h
 *
 * Copyright (c) 2010 TMSBG L.H. SW-BSP1
 *
 * Copyright (C) 2011, Venus Project
 *
 * Key code definitions
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#ifndef _MACH_VENUS_INPUT_H
#define _MACH_VENUS_INPUT_H

/*
 * GPIO keys
 */
#define GPIO_START			0x01
#define GPIO_KEY_POWER			0x01
#define GPIO_KEY_VOLUP			0x02
#define GPIO_KEY_VOLDN			0x03
#define GPIO_KEY_HOME			0x04
#define GPIO_KEY_MENU			0x05
#define GPIO_KEY_BACK			0x06
#define GPIO_KEY_GSW			0x07
#define GPIO_SW_USB			0x08
#define GPIO_SW_HEADPHONE		0x09
#define GPIO_SW_SEARCH			0x0a
#define GPIO_SW_TOUCH			0x0b

#define GPIO_KEY_MAX			0x40

#endif

