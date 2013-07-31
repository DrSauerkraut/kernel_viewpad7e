/* linux/arch/arm/mach-s5pv210/setup-fb.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Base FIMD controller configuration
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <plat/clock.h>
#include <plat/gpio-cfg.h>
#include <plat/fb.h>
#include <mach/regs-clock.h>
#include <mach/regs-gpio.h>
#include <linux/io.h>
#include <mach/map.h>

struct platform_device; /* don't need the contents */

int s3cfb_clk_on(struct platform_device *pdev, struct clk **s3cfb_clk)
{
	struct clk *sclk = NULL;
	struct clk *mout_mpll = NULL;
	u32 rate = 0;

	sclk = clk_get(&pdev->dev, "sclk_fimd");
	if (IS_ERR(sclk)) {
		dev_err(&pdev->dev, "failed to get sclk for fimd\n");
		goto err_clk1;
	}

	mout_mpll = clk_get(&pdev->dev, "mout_mpll");
	if (IS_ERR(mout_mpll)) {
		dev_err(&pdev->dev, "failed to get mout_mpll\n");
		goto err_clk2;
	}

	clk_set_parent(sclk, mout_mpll);

	if (!rate)
		rate = 166750000;

	clk_set_rate(sclk, rate);
	dev_dbg(&pdev->dev, "set fimd sclk rate to %d\n", rate);

	clk_put(mout_mpll);

	clk_enable(sclk);

	*s3cfb_clk = sclk;

	return 0;

err_clk2:
	clk_put(sclk);

err_clk1:
	return -EINVAL;
}

int s3cfb_clk_off(struct platform_device *pdev, struct clk **clk)
{
	clk_disable(*clk);
	clk_put(*clk);

	*clk = NULL;

	return 0;
}

void s3cfb_get_clk_name(char *clk_name)
{
	strcpy(clk_name, "sclk_fimd");
}

void s3cfb_cfg_gpio(struct platform_device *pdev)
{
	int i;
	printk("init LCD interface\n");
	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PV210_GPF0(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV210_GPF0(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PV210_GPF1(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV210_GPF1(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PV210_GPF2(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV210_GPF2(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 4; i++) {
		s3c_gpio_cfgpin(S5PV210_GPF3(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV210_GPF3(i), S3C_GPIO_PULL_NONE);
	}

	/* mDNIe SEL: why we shall write 0x2 ? */
	writel(0x2, S5P_MDNIE_SEL);

	/* drive strength to max */

	writel(0xffffffff, S5PV210_GPF0_BASE + 0xc);
	writel(0xffffffff, S5PV210_GPF1_BASE + 0xc);
	writel(0xffffffff, S5PV210_GPF2_BASE + 0xc);
	writel(0x000000ff, S5PV210_GPF3_BASE + 0xc);

	/* enable LVDS , LCD RESET, LCD STB*/

	s3c_gpio_cfgpin(S5PV210_GPJ1(0), S3C_GPIO_SFN(1));  
	gpio_set_value(S5PV210_GPJ1(0), 1);

	s3c_gpio_cfgpin(S5PV210_GPJ1(1), S3C_GPIO_SFN(1));
	gpio_set_value(S5PV210_GPJ1(1), 1);

	s3c_gpio_cfgpin(S5PV210_GPJ4(3), S3C_GPIO_SFN(1));
	gpio_set_value(S5PV210_GPJ4(3), 1);

}

int s3cfb_backlight_onoff(struct platform_device *pdev, int onoff)
{

	int err;

	err = gpio_request(S5PV210_GPD0(1), "GPD0");

	if (err) {
		printk(KERN_ERR "failed to request GPD0 for "
			"lcd backlight control\n");
		return err;
	}

	if (onoff) {
		gpio_direction_output(S5PV210_GPD0(1), 1);
		/* 2009.12.28 by icarus : added for PWM backlight */
//		s3c_gpio_cfgpin(S5PV210_GPD0(1), S5PV210_GPD_0_1_TOUT_1);

	}
	else {
		gpio_direction_output(S5PV210_GPD0(1), 0);
	}
	gpio_free(S5PV210_GPD0(1));

	return 0;
}

#define S5PV210_GPD_0_1_TOUT_1  (0x2 << 4)

int s3cfb_backlight_on(struct platform_device *pdev)
{
	int err;

	 mdelay(150);
	//turn on backlight 
   	printk("%s\n",__FUNCTION__);
	
	err = gpio_request(S5PV210_GPD0(1), "GPD0");

	if (err) {
		printk(KERN_ERR "failed to request GPD0 for "
				"lcd backlight control\n");
		return err;
	}

	gpio_direction_output(S5PV210_GPD0(1), 0);
	gpio_free(S5PV210_GPD0(1));


	err = gpio_request(S5PV210_GPJ3(0), "GPJ3");

	if (err) {
		printk(KERN_ERR "failed to request GPJ3 for "
				"lcd backlight control\n");
		return err;
	}

	gpio_direction_output(S5PV210_GPJ3(0), 1);
	gpio_free(S5PV210_GPJ3(0));



//	request_power(BACKLIGHT_POWER);
	s3c_gpio_cfgpin(S5PV210_GPD0(1), S5PV210_GPD_0_1_TOUT_1);

	return 0;
}

int s3cfb_backlight_off(struct platform_device *pdev)
{
	int err;

	printk("%s\n",__FUNCTION__);   
	err = gpio_request(S5PV210_GPD0(1), "GPD0");

	if (err) {
		printk(KERN_ERR "failed to request GPD0 for "
				"lcd backlight control\n");
		return err;
	}

	gpio_direction_output(S5PV210_GPD0(1), 1);
	gpio_free(S5PV210_GPD0(1));

//	release_power(BACKLIGHT_POWER);
	return 0;
}

int s3cfb_shut_down(struct platform_device *pdev)
{
	int err;

	printk("%s\n",__FUNCTION__); 

	mdelay(20);

	err = gpio_request(S5PV210_GPD0(1), "GPD0");

	if (err) {
		printk(KERN_ERR "failed to request GPD0 for "
				"lcd backlight control\n");
		return err;
	}

	gpio_direction_output(S5PV210_GPD0(1), 1);
	gpio_free(S5PV210_GPD0(1));

	//turn off backlight               
	printk("%s\n",__FUNCTION__);
        s3c_gpio_cfgpin(S5PV210_GPJ3(0), S3C_GPIO_SFN(1));
	gpio_set_value(S5PV210_GPJ3(0), 0);

#if 1                                                                                                                                         
    //turn off VGH                                                                                                                            
    s3c_gpio_cfgpin(S5PV210_GPJ2(2), S3C_GPIO_SFN(1));                                                                                        
    gpio_set_value(S5PV210_GPJ2(2), 0);                                                                                                       
                                                                                                                                           
    //turn off LCD_5V                                                                                                                         
    s3c_gpio_cfgpin(S5PV210_GPJ2(4), S3C_GPIO_SFN(1));                                                                                        
    gpio_set_value(S5PV210_GPJ2(4), 0);                                                                                                       
                                                                                                                                              
    //release_power(LCD_POWER);                                                                                                               
#endif     

	mdelay(1000);

	return 0;
}

int s3cfb_lcd_on(struct platform_device *pdev)
{

//	request_power(LCD_POWER);
	printk("%s\n",__FUNCTION__);
	mdelay(20);
//turn on LCD_3.3V
        mdelay(30);
//turn on LCD_5V
        s3c_gpio_cfgpin(S5PV210_GPJ2(4), S3C_GPIO_SFN(1));
	gpio_set_value(S5PV210_GPJ2(4), 1);
//turn on AVDD&VGL
//	s3c_gpio_cfgpin(S5PV210_GPJ3(1), S3C_GPIO_SFN(1));
//	gpio_set_value(S5PV210_GPJ3(1), 1);
        mdelay(20);
//turn on VGH
        s3c_gpio_cfgpin(S5PV210_GPJ2(2), S3C_GPIO_SFN(1));
	gpio_set_value(S5PV210_GPJ2(2), 1);
 
	return 0;
}

int s3cfb_lcd_off(struct platform_device *pdev)
{
	printk("%s\n",__FUNCTION__);
	/* Amos add for power lcd off sequence */
	mdelay(10);

//turn off VGH
        s3c_gpio_cfgpin(S5PV210_GPJ2(2), S3C_GPIO_SFN(1));
	gpio_set_value(S5PV210_GPJ2(2), 0);
//        mdelay(20);
//turn off AVDD&VGL
//	s3c_gpio_cfgpin(S5PV210_GPJ3(1), S3C_GPIO_SFN(1));
//	gpio_set_value(S5PV210_GPJ3(1), 0);
//turn off LCD_5V
        s3c_gpio_cfgpin(S5PV210_GPJ2(4), S3C_GPIO_SFN(1));
	gpio_set_value(S5PV210_GPJ2(4), 0);
	mdelay(300);

//	release_power(LCD_POWER);
	return 0;
}
