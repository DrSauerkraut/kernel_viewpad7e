/* linux/arch/arm/mach-s5pv210/$(VENUS_PROJECT_DIR)/dev-camera.c
 *
 * Copyright (c) 2011 PDC2 Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/venus/power_control.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/setup.h>
#include <asm/mach-types.h>

#include <mach/regs-clock.h>
#include <mach/regs-mem.h>
#include <mach/regs-gpio.h>

#include <media/gc0309_platform.h>
#include <media/s5k5ca_platform.h>

#include <plat/fimc.h>

#include <plat/gpio-cfg.h>

#define GPIO_CAM0M3_RESET		S5PV210_GPE1(4)
#define GPIO_CAM0M3_PWDN		S5PV210_GPJ4(3)

#define GPIO_CAM3M_RESET		S5PV210_GPJ1(3)
#define GPIO_CAM3M_PWDN			S5PV210_GPJ3(2)

static int cam0_power_status = 0;
static int cam1_power_status = 0;

static int gpio_output(int gpio, int value, char *label)
{
	int err;

	err = gpio_request(gpio, label);
	if (err)
	{
		printk("Fail to request gpio(%d) for %s\n", gpio, label);
	}

	s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	gpio_direction_output(gpio, value);
	gpio_free(gpio);

	return 0;
}

static int cam0_power(int onoff)
{
	//printk("%s(%d)\n", __func__, onoff);
	if (cam0_power_status == onoff)
	{
		return 0;
	}

	if (onoff)
	{
	//	request_power(CAM_2_8V);
	//	request_power(CAM_1_8V);
		//24Mhz clock on
		s3c_gpio_cfgpin(S5PV210_GPE1(3), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV210_GPE1(3), S3C_GPIO_PULL_NONE);
		mdelay(100);
		gpio_output(GPIO_CAM0M3_PWDN, 1, "CAM0M3 PWDN");
		mdelay(50);
		gpio_output(GPIO_CAM0M3_RESET, 1, "CAM0M3 RESET");
		mdelay(50);
		gpio_output(GPIO_CAM0M3_PWDN, 0, "CAM0M3 PWDN");
	}
	else
	{
		gpio_output(GPIO_CAM0M3_PWDN, 1, "CAM0M3 PWDN");
		mdelay(50);
		s3c_gpio_cfgpin(S5PV210_GPE1(3), S3C_GPIO_SFN(0));
		mdelay(50);
		gpio_output(GPIO_CAM0M3_RESET, 0, "CAM0M3 RESET");
		mdelay(100);

	//	release_power(CAM_1_8V);
	//	release_power(CAM_2_8V);
	}

	cam0_power_status = onoff;
	return 0;
}

static int cam1_power(int onoff)
{
	if (cam1_power_status == onoff)
	{
		return 0;
	}

	if (onoff)
	{
		//24Mhz clock on
		s3c_gpio_cfgpin(S5PV210_GPJ1(4), S3C_GPIO_SFN(3));
		s3c_gpio_setpull(S5PV210_GPJ1(4), S3C_GPIO_PULL_NONE);
		mdelay(100);
	//	request_power(CAM_2_8V);
	//	request_power(CAM_1_8V);

		gpio_output(GPIO_CAM3M_PWDN, 1, "CAM3M PWDN");
		mdelay(20);
		gpio_output(GPIO_CAM3M_RESET, 0, "CAM3M RESET");
		mdelay(50);
		gpio_output(GPIO_CAM3M_PWDN, 0, "CAM3M PWDN");
		udelay(100);
		gpio_output(GPIO_CAM3M_RESET, 1, "CAM3M RESET");
	}
	else
	{
		gpio_output(GPIO_CAM3M_PWDN, 1, "CAM3M PWDN");
		
		mdelay(50);
		//24Mhz clock off
		s3c_gpio_cfgpin(S5PV210_GPJ1(4), S3C_GPIO_SFN(0));
		gpio_output(GPIO_CAM3M_RESET, 0, "CAM3M RESET");
		mdelay(50);

	//	release_power(CAM_1_8V);
	//	release_power(CAM_2_8V);
	}

	cam1_power_status = onoff;

	return 0;
}

int cam_init(void)
{
	gpio_output(GPIO_CAM0M3_RESET, 0, "CAM0M3 RESET");	
	gpio_output(GPIO_CAM0M3_PWDN, 0, "CAM0M3 PWDN");

	gpio_output(GPIO_CAM3M_PWDN, 0, "CAM3M PWDN");
	gpio_output(GPIO_CAM3M_RESET, 0, "CAM3M RESET");

	return 0;
}

// RC0408
static struct gc0309_platform_data gc0309_plat = {
	.default_width = 640,
	.default_height = 480,
	.pixelformat = V4L2_PIX_FMT_VYUY,
	.freq = 24000000,
	.is_mipi = 0,
};

static struct i2c_board_info  gc0309_i2c_info = {
	I2C_BOARD_INFO("gc0309", 0x21),
	.platform_data = &gc0309_plat,
};

static struct s3c_platform_camera gc0309 = {
	.id		= CAMERA_PAR_A,
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CRYCBY,
	.i2c_busnum	= 4,
	.info		= &gc0309_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_VYUY,
	.srclk_name	= "mout_mpll",
	.clk_name	= "sclk_cam1",
	.clk_rate	= 24000000,
	.line_length	= 1920,
	.width		= 640,
	.height		= 480,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 640,
		.height	= 480,
	},

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,

	.initialized	= 0,
	.cam_power	= cam0_power,
};

// RC0506
static struct s5k5ca_platform_data s5k5ca_plat = {
	.default_width = 800,
	.default_height = 600,
	.pixelformat = V4L2_PIX_FMT_VYUY,
	.freq = 24000000,
	.is_mipi = 0,
};

static struct i2c_board_info s5k5ca_i2c_info = {
	I2C_BOARD_INFO("s5k5ca", 0x2d),
	.platform_data = &s5k5ca_plat,
};

static struct s3c_platform_camera s5k5ca = {
	.id             = CAMERA_PAR_B,
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.i2c_busnum	= 4,
	.info		= &s5k5ca_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_VYUY,
	.srclk_name	= "mout_mpll",
	.clk_name	= "sclk_cam1",
	.clk_rate	= 24000000,
	.line_length	= 1920,
	.width		= 800,
	.height		= 600,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 800,
		.height	= 600,
	},

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,

	.initialized	= 0,
	.cam_power	= cam1_power,
};

/* Interface setting */
struct s3c_platform_fimc fimc_plat = {
	.srclk_name	= "mout_mpll",
	.clk_name	= "sclk_fimc",
	.lclk_name	= "sclk_fimc_lclk",
	.clk_rate	= 166750000,
	
#ifdef CONFIG_PRODUCT_USING_V7E_COST_DOWN
   .default_cam    = CAMERA_PAR_A,        
#else 
	 .default_cam    = CAMERA_PAR_B,
#endif
	
	.camera		= {
#ifdef CONFIG_PRODUCT_USING_V7E_COST_DOWN
	   &gc0309,
#else 
		 &s5k5ca,
		 &gc0309,
#endif	
	
	},
	.hw_ver		= 0x43,
};

