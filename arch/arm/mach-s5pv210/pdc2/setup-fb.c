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
#include <linux/venus/power_control.h>

#define LCD_POWER1 S5PV210_GPJ2(4)
#define AVDD_CTRL S5PV210_GPJ3(1)
#define VGH_CTRL S5PV210_GPJ2(2)
#define LCD_BL_ADJ S5PV210_GPD0(1)
#define LCD_BL_EN S5PV210_GPJ3(0)
#define S5PV210_GPD_0_1_TOUT_1  (0x2 << 4)

#define kEY_LCD   S5PV210_GPD0(0)

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
	// printk("init LCD interface.\n");
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

	/* drive strength to 1x */
//#if defined(CONFIG_PRODUCT_PDC2)
	writel(0x00000000, S5PV210_GPF0_BASE + 0xc);
	writel(0x00000000, S5PV210_GPF1_BASE + 0xc);
	writel(0x00000000, S5PV210_GPF2_BASE + 0xc);
	writel(0x00000000, S5PV210_GPF3_BASE + 0xc);
//#endif
#if 0//defined(CONFIG_PRODUCT_PDC3)
	writel(0xffffffff, S5PV210_GPF0_BASE + 0xc);
	writel(0xffffffff, S5PV210_GPF1_BASE + 0xc);
	writel(0xffffffff, S5PV210_GPF2_BASE + 0xc);
	writel(0x000000ff, S5PV210_GPF3_BASE + 0xc);
#endif
}

int s3cfb_backlight_onoff(struct platform_device *pdev, int onoff)
{

	int err;
	err = gpio_request(LCD_BL_ADJ, "LCD_BL_ADJ");
	if (err) {
		printk(KERN_ERR "failed to request LCD_BL_ADJ for "
			"lcd backlight control\n");
		return err;
	}
	if (onoff) {
		gpio_direction_output(LCD_BL_ADJ, 1);
	}
	else {
		gpio_direction_output(LCD_BL_ADJ, 0);
	}
	gpio_free(LCD_BL_ADJ);

	return 0;
}

int s3cfb_backlight_on(struct platform_device *pdev)
{
	int err;

	msleep(150);

   	// printk("%s\n",__FUNCTION__);
	
	err = gpio_request(LCD_BL_ADJ, "LCD_BL_ADJ");
	if (err) {
		printk(KERN_ERR "failed to request LCD_BL_ADJ for "
				"lcd backlight control\n");
		return err;
	}
	gpio_direction_output(LCD_BL_ADJ, 0);
	gpio_free(LCD_BL_ADJ);
	err = gpio_request(LCD_BL_EN, "LCD_BL_EN");
	if (err) {
		printk(KERN_ERR "failed to request LCD_BL_EN for "
				"lcd backlight control\n");
		return err;
	}
	gpio_direction_output(LCD_BL_EN, 1);
	gpio_free(LCD_BL_EN);
	
	request_power(BACKLIGHT_POWER);
	s3c_gpio_cfgpin(LCD_BL_ADJ, S5PV210_GPD_0_1_TOUT_1);
	
	// Add for kEY_LCD, 2011.07.01.
	err = gpio_request(kEY_LCD,  "GPD0");
	if (err) {
		printk(KERN_ERR "failed to request kEY_LCD for "
				"key led control\n");
		return err;
	}
	
	gpio_direction_output(kEY_LCD, 1);
	gpio_free(kEY_LCD);
  msleep(20);

	return 0;
}

int s3cfb_backlight_off(struct platform_device *pdev)
{
	int err;

	// printk("%s\n",__FUNCTION__);   
	err = gpio_request(LCD_BL_ADJ, "LCD_BL_ADJ");

	if (err) {
		printk(KERN_ERR "failed to request LCD_BL_ADJ for "
				"lcd backlight control\n");
		return err;
	}

	gpio_direction_output(LCD_BL_ADJ, 1);
	gpio_free(LCD_BL_ADJ);

	release_power(BACKLIGHT_POWER);
	
	// Add for kEY_LCD, 2011.07.01.
	err = gpio_request(kEY_LCD, "GPD0");
	if (err) {
		printk(KERN_ERR "failed to request kEY_LCD for "
				"key led control\n");
		return err;
	}

	gpio_direction_output(kEY_LCD,0);
	gpio_free( kEY_LCD );
	msleep(20);
	
	return 0;
}

int s3cfb_shut_down(struct platform_device *pdev)
{
	int err;

	 //printk("luis: %s, %s\n", __FILE__, __FUNCTION__); 
#ifdef CONFIG_PRDOUCT_PDC3_FOG
	msleep(20);
#endif

#ifndef CONFIG_PRDOUCT_PDC3
	msleep(20);
#endif
	
#ifdef CONFIG_PRODUCT_PDC3
#ifndef CONFIG_PRDOUCT_PDC3_FOG
	s3c_gpio_cfgpin(AVDD_CTRL, S3C_GPIO_SFN(1));
	gpio_set_value(AVDD_CTRL, 0);
	msleep(40);
#endif
#endif 


	err = gpio_request(LCD_BL_ADJ, "LCD_BL_ADJ");
	if (err) {
		printk(KERN_ERR "failed to request LCD_BL_ADJ for "
				"lcd backlight control\n");
		return err;
	}
	gpio_direction_output(LCD_BL_ADJ, 1);
	gpio_free(LCD_BL_ADJ);


#ifdef CONFIG_PRODUCT_PDC3_FOG
  s3c_gpio_cfgpin(LCD_BL_EN, S3C_GPIO_SFN(1));
	gpio_set_value(LCD_BL_EN, 0);
#endif

#ifndef CONFIG_PRODUCT_PDC3
  s3c_gpio_cfgpin(LCD_BL_EN, S3C_GPIO_SFN(1));
	gpio_set_value(LCD_BL_EN, 0);
#endif
	
 
 	s3c_gpio_cfgpin(VGH_CTRL, S3C_GPIO_SFN(1));
	gpio_set_value(VGH_CTRL, 0);

	s3c_gpio_cfgpin(LCD_POWER1, S3C_GPIO_SFN(1));
	gpio_set_value(LCD_POWER1, 0);
	
	// Add for kEY_LCD, 2011.07.01.
	err = gpio_request(kEY_LCD, "GPD0");
	if (err) {
		printk(KERN_ERR "failed to request kEY_LCD for "
				"key led control\n");
		return err;
	}

	gpio_direction_output(kEY_LCD,0);
	gpio_free( kEY_LCD );
	msleep(20);

	return 0;
}

int s3cfb_lcd_on(struct platform_device *pdev)
{

		request_power(VDD_LCD);                                      
		request_power(LCD_POWER);                                                
		// printk("%s\n",__FUNCTION__);                                             
		msleep(30);                                                              
  /*                                                                         
    //turn off at first                                              
    s3c_gpio_cfgpin(VGH_CTRL, S3C_GPIO_SFN(1));                        
		gpio_set_value(VGH_CTRL, 0);                                             
		msleep(10);                                                              
                                                                           
    #ifdef CONFIG_PRODUCT_PDC3                                             
	    	s3c_gpio_cfgpin(AVDD_CTRL, S3C_GPIO_SFN(1));                         
	    	gpio_set_value(AVDD_CTRL, 0);                                        
	    	msleep(10);                                                          
    #endif                                                                 
                                                                           
		s3c_gpio_cfgpin(LCD_POWER1, S3C_GPIO_SFN(1));                            
		gpio_set_value(LCD_POWER1, 0);                                           
	  msleep(10);                                                              
          
   */                                                             
      // turn on       
		s3c_gpio_cfgpin(LCD_POWER1, S3C_GPIO_SFN(1));                            
		gpio_set_value(LCD_POWER1, 1); 
		msleep(30);                                                              
                                                                           
    #ifdef CONFIG_PRODUCT_PDC3                                            
	    	s3c_gpio_cfgpin(AVDD_CTRL, S3C_GPIO_SFN(1));                        
	    	gpio_set_value(AVDD_CTRL, 1);                                       
	    	msleep(45);                                                         
    #endif
                                                                                                                                           
		s3c_gpio_cfgpin(VGH_CTRL, S3C_GPIO_SFN(1));                              
		gpio_set_value(VGH_CTRL, 1);                                            
		msleep(35);    
		
#ifdef CONFIG_PRODUCT_PDC3_FOG
		extern void init_a080sn01_date(void);//TOMMY
		init_a080sn01_date();//TOMMY
#endif
                                                                           
	return 0; 			
			
}
int s3cfb_lcd_off(struct platform_device *pdev)
{
	// printk("%s\n",__FUNCTION__);
	msleep(10);
	
	s3c_gpio_cfgpin(VGH_CTRL, S3C_GPIO_SFN(1));
	gpio_set_value(VGH_CTRL, 0);
	msleep(20);

#ifdef CONFIG_PRODUCT_PDC3
	s3c_gpio_cfgpin(AVDD_CTRL, S3C_GPIO_SFN(1));
	gpio_set_value(AVDD_CTRL, 0);
	msleep(20);
#endif





	s3c_gpio_cfgpin(LCD_POWER1, S3C_GPIO_SFN(1));
	gpio_set_value(LCD_POWER1, 0);
	msleep(300);

	release_power(LCD_POWER);

	release_power(VDD_LCD);

	return 0;
}
