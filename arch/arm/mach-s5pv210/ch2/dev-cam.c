/* linux/arch/arm/mach-s5pv210/$(VENUS_PROJECT_DIR)/dev-cam.c
 *
 * Copyright (c) 2011 Venus Project
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
//#include <mach/gpio-bank.h>

#include <media/s5k6aa_platform.h>
#include <media/mt9p111_platform.h>

#include <plat/fimc.h>
#include <plat/csis.h>

#include <plat/gpio-cfg.h>

#define GPIO_LEVEL_EN		S5PV210_GPJ0(4)
#define GPIO_CAM0_RESET		S5PV210_GPJ1(3)
#define GPIO_CAM0_PWDN		S5PV210_GPJ3(2)
#define GPIO_MIPI_RESET		S5PV210_GPC0(3)
#define GPIO_MIPI_PWDN		S5PV210_GPJ0(5)

static int cam0_power_status = 0;
static int mipi_power_status = 0;

static int cam0_power(int onoff)
{
	int err;

	if (cam0_power_status == onoff)
	{
		return 0;
	}

	if (onoff)
	{
		printk("cam0 power on\n");
		request_power(CAM_2_8V);
		request_power(CAM_1_8V);
		request_power(CAM_1_5V);

		/* Level EN */
		err = gpio_request(S5PV210_GPJ0(4), "GPJ0");
		s3c_gpio_setpull(S5PV210_GPJ0(4), S3C_GPIO_PULL_NONE);
		gpio_direction_output(S5PV210_GPJ0(4), 0);
		gpio_free(S5PV210_GPJ0(4));

		/* PWDN */
		err = gpio_request(S5PV210_GPJ3(2), "GPJ3");
		s3c_gpio_setpull(S5PV210_GPJ3(2), S3C_GPIO_PULL_NONE);
		gpio_direction_output(S5PV210_GPJ3(2), 1);
		gpio_free(S5PV210_GPJ3(2));

		/* MIPI Reset to low */
		err = gpio_request(S5PV210_GPC0(3), "GPC0");
		s3c_gpio_setpull(S5PV210_GPC0(3), S3C_GPIO_PULL_NONE);
		gpio_direction_output(S5PV210_GPC0(3), 0);
		gpio_free(S5PV210_GPC0(3));

		/* MIPI PWDN to high*/
		err = gpio_request(S5PV210_GPJ0(5), "GPJ0(5)");
		s3c_gpio_setpull(S5PV210_GPJ0(5), S3C_GPIO_PULL_NONE);
		gpio_direction_output(S5PV210_GPJ0(5), 1);
		gpio_free(S5PV210_GPJ0(5));

		udelay(50);

		/* Reset */
		err = gpio_request(S5PV210_GPJ1(3), "GPJ1");
		s3c_gpio_setpull(S5PV210_GPJ1(3), S3C_GPIO_PULL_NONE);
		gpio_direction_output(S5PV210_GPJ1(3), 0);

		mdelay(100);

		gpio_direction_output(S5PV210_GPJ1(3), 1);
		gpio_free(S5PV210_GPJ1(3));

	}
	else
	{
		printk("cam0 power off\n");
		/* Level EN */
		err = gpio_request(S5PV210_GPJ0(4), "GPJ0");
		s3c_gpio_setpull(S5PV210_GPJ0(4), S3C_GPIO_PULL_NONE);
		gpio_direction_output(S5PV210_GPJ0(4), 1);
		gpio_free(S5PV210_GPJ0(4));

		/* MIPI PWDN */
		err = gpio_request(S5PV210_GPC0(3), "GPC0(3)");
		s3c_gpio_setpull(S5PV210_GPC0(3), S3C_GPIO_PULL_NONE);
		gpio_direction_output(S5PV210_GPC0(3), 0);
		gpio_free(S5PV210_GPC0(3));

		/* Reset */
		err = gpio_request(S5PV210_GPJ1(3), "GPJ1");
		s3c_gpio_setpull(S5PV210_GPJ1(3), S3C_GPIO_PULL_NONE);
		gpio_direction_output(S5PV210_GPJ1(3), 0);
		gpio_free(S5PV210_GPJ1(3));

		release_power(CAM_1_8V);
		release_power(CAM_1_5V);
		release_power(CAM_2_8V);
	}

	cam0_power_status = onoff;

	return 0;
}

static int mipi_power(int onoff)
{
	int err;

	if (mipi_power_status == onoff)
	{
		return 0;
	}

	if (onoff)
	{
		printk("mipi power on\n");
		request_power(CAM_1_8V);
		request_power(CAM_2_8V);
		request_power(CAM_1_5V);
	
		/* Level EN */
		err = gpio_request(S5PV210_GPJ0(4), "GPJ0");
		s3c_gpio_setpull(S5PV210_GPJ0(4), S3C_GPIO_PULL_NONE);
		gpio_direction_output(S5PV210_GPJ0(4), 0);
		gpio_free(S5PV210_GPJ0(4));

		/* PWDN */
		err = gpio_request(S5PV210_GPJ0(5), "GPJ0");
		s3c_gpio_setpull(S5PV210_GPJ0(5), S3C_GPIO_PULL_NONE);
		gpio_direction_output(S5PV210_GPJ0(5), 0);
		gpio_free(S5PV210_GPJ0(5));

		/* Cam0 Reset */
		err = gpio_request(S5PV210_GPJ1(3), "GPJ1");
		s3c_gpio_setpull(S5PV210_GPJ1(3), S3C_GPIO_PULL_NONE);
		gpio_direction_output(S5PV210_GPJ1(3), 0);
		gpio_free(S5PV210_GPJ1(3));

		/* Cam0 PWDN */
		err = gpio_request(S5PV210_GPJ3(2), "GPJ3");
		s3c_gpio_setpull(S5PV210_GPJ3(2), S3C_GPIO_PULL_NONE);
		gpio_direction_output(S5PV210_GPJ3(2), 1);
		gpio_free(S5PV210_GPJ3(2));

		/* Reset */
		err = gpio_request(S5PV210_GPC0(3), "GPC0");
		s3c_gpio_setpull(S5PV210_GPC0(3), S3C_GPIO_PULL_NONE);
		gpio_direction_output(S5PV210_GPC0(3), 0);
		gpio_free(S5PV210_GPC0(3));

		mdelay(1);
	}
	else
	{
		printk("mipi power off\n");
		/* Level EN */
		err = gpio_request(S5PV210_GPJ0(4), "GPJ0");
		s3c_gpio_setpull(S5PV210_GPJ0(4), S3C_GPIO_PULL_NONE);
		gpio_direction_output(S5PV210_GPJ0(4), 1);
		gpio_free(S5PV210_GPJ0(4));

		/* Cam0 PWDN */
		err = gpio_request(S5PV210_GPJ3(2), "GPJ3");
		s3c_gpio_setpull(S5PV210_GPJ3(2), S3C_GPIO_PULL_NONE);
		gpio_direction_output(S5PV210_GPJ3(2), 0);
		gpio_free(S5PV210_GPJ3(2));

		/* Reset */
		err = gpio_request(S5PV210_GPC0(3), "GPC0");
		s3c_gpio_setpull(S5PV210_GPC0(3), S3C_GPIO_PULL_NONE);
		gpio_direction_output(S5PV210_GPC0(3), 0);
		gpio_free(S5PV210_GPC0(3));

		release_power(CAM_1_5V);
		release_power(CAM_1_8V);
		release_power(CAM_2_8V);
	}

	mipi_power_status = onoff;

	return 0;
}

static int mipi_cam_reset(void)
{
	int err;

	/* Reset */
	err = gpio_request(S5PV210_GPC0(3), "GPC0");
	s3c_gpio_setpull(S5PV210_GPC0(3), S3C_GPIO_PULL_NONE);
	gpio_direction_output(S5PV210_GPC0(3), 0);

	mdelay(100);

	gpio_direction_output(S5PV210_GPC0(3), 1);
	gpio_free(S5PV210_GPC0(3));

	mdelay(200);

	return 0;
}

static int mipi_cam_power(int onoff)
{
	int err;

	return 0;
}

int cam_init(void)
{
	int err;

	/* Level EN */
	err = gpio_request(S5PV210_GPJ0(4), "GPJ0");
	s3c_gpio_setpull(S5PV210_GPJ0(4), S3C_GPIO_PULL_NONE);
	gpio_direction_output(S5PV210_GPJ0(4), 1);
	gpio_free(S5PV210_GPJ0(4));

	/* Reset */
	err = gpio_request(S5PV210_GPJ1(3), "GPJ1");
	s3c_gpio_setpull(S5PV210_GPJ1(3), S3C_GPIO_PULL_NONE);
	gpio_direction_output(S5PV210_GPJ1(3), 0);
	gpio_free(S5PV210_GPJ1(3));

	/* Reset */
	err = gpio_request(S5PV210_GPC0(3), "GPC0");
	s3c_gpio_setpull(S5PV210_GPC0(3), S3C_GPIO_PULL_NONE);
	gpio_direction_output(S5PV210_GPC0(3), 0);
	gpio_free(S5PV210_GPC0(3));

	/* Cam0 PWDN */
	err = gpio_request(S5PV210_GPJ3(2), "GPJ3");
	s3c_gpio_setpull(S5PV210_GPJ3(2), S3C_GPIO_PULL_NONE);
	gpio_direction_output(S5PV210_GPJ3(2), 0);
	gpio_free(S5PV210_GPJ3(2));

	/* MIPI PWDN */
	err = gpio_request(S5PV210_GPC0(3), "GPC0(3)");
	s3c_gpio_setpull(S5PV210_GPC0(3), S3C_GPIO_PULL_NONE);
	gpio_direction_output(S5PV210_GPC0(3), 0);
	gpio_free(S5PV210_GPC0(3));

	return 0;
}

static struct s5k6aa_platform_data s5k6aa_plat = {
	.default_width = 1280,
	.default_height = 1024,
	.pixelformat = V4L2_PIX_FMT_VYUY,
	.freq = 24000000,
	.is_mipi = 0,
};

static struct i2c_board_info  s5k6aa_i2c_info = {
	I2C_BOARD_INFO("S5K6AA", 0x3c),
	.platform_data = &s5k6aa_plat,
};

static struct s3c_platform_camera s5k6aa = {
	.id		= CAMERA_PAR_A,
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.i2c_busnum	= 7,
	.info		= &s5k6aa_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_VYUY,
	.srclk_name	= "mout_mpll",
	.clk_name	= "sclk_cam1",
	.clk_rate	= 24000000,
	.line_length	= 1920,
	.width		= 1280,
	.height		= 1024,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 1280,
		.height	= 1024,
	},

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,

	.initialized	= 0,
	.cam_power	= cam0_power,
};

static struct mt9p111_platform_data mt9p111_plat = {
	.default_width = 640,
	.default_height = 480,
	.pixelformat = V4L2_PIX_FMT_VYUY,
	.freq = 24000000,
	.is_mipi = 1,
};

static struct i2c_board_info mt9p111_i2c_info = {
	I2C_BOARD_INFO("MT9P111", 0x3c),
	.platform_data = &mt9p111_plat,
};

static struct s3c_platform_camera mt9p111 = {
	.id		= CAMERA_CSI_C,
	.type		= CAM_TYPE_MIPI,
	.fmt		= MIPI_CSI_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.i2c_busnum	= 4,
	.info		= &mt9p111_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_VYUY,
//	.srclk_name	= "mout_mpll",
	.srclk_name	= "xusbxti",
//	.clk_name	= "sclk_csis",
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

	.mipi_lanes     = 1,
	.mipi_settle    = 12, // Reserved
	.mipi_align     = 32,

	.initialized	= 0,
	.cam_power	= mipi_power,
	.cam_reset	= mipi_cam_reset,
};

/* Interface setting */
struct s3c_platform_fimc fimc_plat = {
	.srclk_name	= "mout_mpll",
	.clk_name	= "fimc",
	.clk_rate	= 166000000,
//	.default_cam	= CAMERA_PAR_A,
	.default_cam	= CAMERA_CSI_C,
	.camera		= {
		&mt9p111,
		&s5k6aa,
	},
	.hw_ver		= 0x43,
};

