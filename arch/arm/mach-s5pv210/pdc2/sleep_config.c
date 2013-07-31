/* 
 * linux/arch/arm/mach-s5pv210/$(VENUS_PROJECT_DIR)/dev-sleep_config.c
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
#include <mach/map.h>
#include <plat/cpu.h>
#include <asm/io.h>
#include <mach/regs-gpio.h>
#include <mach/gpio.h>
#include <mach/gpio-bank.h>
#include <plat/gpio-cfg.h>
#include <linux/venus/power_control.h>

/*
 *
 * GP**CONPDN :   [2n + 1 : 2n];
 *	
 *		   0 0 = Output 0; 
 *		   0 1 = Output 1; 
 *		   1 0 = Input; 
 *		   1 1 = Previous state 
 * 
 * GP**PUDPDN :   [2n + 1 : 2n];
 *		 
 *		   0 0 = Pull-up / down disable; 
 *        	   0 1 = Pull-down enable; 
 *		   1 0 = Pull-up enable; 
 *		   1 1 = Reserved ;
 *
 */
 
#ifdef CONFIG_PRODUCT_PDC2
void sleep_gpio_GPA0_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;
	/* 
	 *  GPA0:
	 *
  	 *  GPIO Description:
	 *  GPA0[7]: "None", GPA0[6]: "None", GPA0[5]: "None", GPA0[4]: "None";
	 *  GPA0[3]: "BT_RTS", GPA0[2]: "BT_CTS", GPA0[1]: "BT_TX", GPA0[0]: "BT_RX";
        *
	 *  GPIO sleep configuration:
	 *
	 *  GPA0[7] = "10" Input, GPA0[6] = "10" Input, GPA0[5] = "10" Input, GPA0[4] = "10" Input;
	 *  GPA0[3] = "10" Input, GPA0[2] = "10" Input, GPA0[1] = "10" Input, GPA0[0] = "10" Input;
	 *  
	 *  Register Bit [15 ~ 0] = 1010 1010 1010 1010
	 */
	//sleep_config = readl(S5PV210_GPA0CONPDN);
	//sleep_config |= (1 << 15 | 1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	//sleep_config &= ~( 1 << 14 | 1 << 12 | 1<< 10 | 1 << 8  | 1 << 6  | 1<< 4  | 1 << 2   | 1 << 0 );		
	writel(sleep_config, S5PV210_GPA0CONPDN);

	/* 
	 *  GPA0:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPA0[7] = "00"  Pull-up / down disable, 	GPA0[6] = "00"  Pull-up / down disable;
	 *  GPA0[5] = "00"  Pull-up / down disable, 	GPA0[4] = "00"  Pull-up / down disable;
	 *  GPA0[3] = "00"  Pull-up / down disable,     GPA0[2] = "00"  Pull-up / down disable;
	 *  GPA0[1] = "00"  Pull-up / down disable,     GPA0[0] = "00"  Pull-up / down disable;;
	 *  
	 *  Register Bit [15 ~ 0] = 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPA0PUDPDN);
	sleep_pud &= ~( 1 << 15 | 1 << 14 | 1 << 13 | 1 << 12 | 1 << 11 | 1 << 10 | 1 << 9 | 1 << 8 );
	sleep_pud &= ~( 1 << 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0);
	writel(sleep_pud, S5PV210_GPA0PUDPDN);

}

void sleep_gpio_GPA1_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;
	/* 
	 *  GPA1:
	 *
  	 *  GPIO Description:
	 *  GPA1[3]: "INAND_EN", GPA1[2]: "LEVEL_EN", GPA1[1]: "Debug_TX", GPA1[0]: "Debug_RX";
         *
	 *  GPIO sleep configuration:
	 *
	 *  GPA1[3] = "10" Input, GPA1[2] = "10" Input, GPA1[1] = "10" Intput, GPA1[0] = "10" Input;
	 *  
	 *  Register Bit [7 ~ 0] = 1010 1010 
	 */
	//sleep_config = readl(S5PV210_GPA1CONPDN);
	//sleep_config |= (1 << 7 | 1 << 5  | 1 << 3 | 1 << 1);
	//sleep_config &= ~(1 << 6 | 1 << 4  | 1 << 2  | 1 << 0);	
	writel(sleep_config, S5PV210_GPA1CONPDN);

	/* 
	 *  GPA1:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPA1[3] = "00"  Pull-up / down disable,     GPA1[2] = "00"  Pull-up / down disable;
	 *  GPA1[1] = "00"  Pull-up / down disable,     GPA1[0] = "00"  Pull-up / down disable;;
	 *  
	 *  Register Bit [7 ~ 0] = 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPA1PUDPDN);
	sleep_pud &= ~(1 << 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0);
	writel(sleep_pud, S5PV210_GPA1PUDPDN);

}

void sleep_gpio_GPB_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;
	/* 
	 *  GPB:
	 *
  	 *  GPIO Description:
	 *  GPB[7]: "None", GPB[6]: "None", GPB[5]: "None", GPB[4]: "None";
	 *  GPB[3]: "None", GPB[2]: "INAND_RST", GPB[1]: "Sub_SDATA", GPB[0]: "Sub_SCLK";
         *
	 *  GPIO sleep configuration:
	 *
	 *  GPB[7] = "10" Input, GPB[6] = "10" Input,  GPB[5] = "10" Input, GPB[4] = "10" Input, 
	 *  GPB[3] = "10" Input, GPB[2] = "10" Input,  GPB[1] = "01" Output 1, GPB[0] = "01" Output 1;
	 *  
	 *  Register Bit [15 ~ 0] = 1010 1010 1010 0101
	 */
	sleep_config = readl(S5PV210_GPBCONPDN);
	sleep_config |= (1 << 15 |  1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1<< 5 | 1 << 2 | 1 << 0);
	sleep_config &= ~( 1 << 14 | 1 << 12 | 1 << 10 | 1 << 8 | 1 << 6  | 1 << 4 | 1 << 3 | 1<< 1 );		
	writel(sleep_config, S5PV210_GPBCONPDN);

	/* 
	 *  GPB:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPB[7] = "00"  Pull-up / down disable, 	GPB[6] = "00"  Pull-up / down disable;
	 *  GPB[5] = "00"  Pull-up / down disable, 	GPB[4] = "00"  Pull-up / down disable;
	 *  GPB[3] = "00"  Pull-up / down disable,      GPB[2] = "00"  Pull-up / down disable;
	 *  GPB[1] = "00"  Pull-up / down disable,      GPB[0] = "00"  Pull-up / down disable;;
	 *  
	 *  Register Bit [15 ~ 0] = 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPBPUDPDN);
	sleep_pud &= ~( 1 << 15 | 1 << 14 | 1 << 13 | 1 << 12 | 1 << 11 | 1 << 10 | 1 << 9 | 1 << 8 );
	sleep_pud &= ~( 1 << 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0);
	writel(sleep_pud, S5PV210_GPBPUDPDN);

}

void sleep_gpio_GPC0_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;
	/* 
	 *  GPC0:
	 *
  	 *  GPIO Description:
	 *  GPC0[4]: "MUTE";
	 *  GPC0[3]: "LCD_STBYB", GPC0[2]: "LCD_RESET", GPC0[1]: "ROW_6", GPC0[0]: "ROW_5";
        *
	 *  GPIO sleep configuration:
	 *
	 *  GPC0[4] = "00" Output 0;
	 *  GPC0[3] = "10" Input, GPC0[2] = "10" Input, GPC0[1] = "10" Input, GPC0[0] = "10" Input;
	 *  
	 *  Register Bit [9 ~ 0] = 00 1010 1010
	 */
	//sleep_config = readl(S5PV210_GPC0CONPDN);
	//sleep_config |= ( 1 << 7 | 1 << 5 | 1 << 3 | 1<< 1 );
	//sleep_config &= ~( 1 << 9 | 1 << 8 | 1 << 6 | 1<< 4 |  1 << 2 | 1 << 0);
	writel(sleep_config, S5PV210_GPC0CONPDN);

	/* 
	 *  GPC0:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPC0[4] = "00"  Pull-up / down disable;
	 *  GPC0[3] = "00"  Pull-up / down disable,      GPC0[2] = "00"  Pull-up / down disable;
	 *  GPC0[1] = "00"  Pull-up / down disable,      GPC0[0] = "00"  Pull-up / down disable;;
	 *  
	 *  Register Bit [9 ~ 0] = 00 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPC0PUDPDN);
	sleep_pud &= ~( 1 << 9 | 1 << 8 );
	sleep_pud &= ~( 1 << 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0);
	writel(sleep_pud, S5PV210_GPC0PUDPDN);

}

void sleep_gpio_GPC1_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;
	/* 
	 *  GPC1: DO NOT USE
	 *
  	 * 
	 *  Register Bit [9 ~ 0] = 10 1010 1010
	 */
	//sleep_config = readl(S5PV210_GPC1CONPDN);
	//sleep_config |= ( 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 <<1 );
	//sleep_config &= ~( 1 << 8 | 1 << 6 | 1<< 4 | 1 << 2 | 1<< 0 );		
	writel(sleep_config, S5PV210_GPC1CONPDN);

	/* 
	 *  GPC1:
	 *
	 *  GPIO sleep PUD configuration:
	 *  
	 *  Register Bit [9 ~ 0] = 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPC1PUDPDN);
	sleep_pud &= ~( 1 << 9 | 1 << 8 );
	sleep_pud &= ~( 1 << 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0);
	writel(sleep_pud, S5PV210_GPC1PUDPDN);

}

void sleep_gpio_GPD0_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;

	/* 
	 *  GPD0:
	 *
	 *  GPIO Description:
	 *
	 *  GPD0[3] : "CAMERA_I2C_CLK",  		GPD0[2] : "CAMERA_I2C_DATA"; 
	 *  GPD0[1] : "LCD_BL_ADJ",			GPD0[0] : " KEY_LED ";
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPD0[3] = "10"  Input,  		GPD0[2] = "10"  Input;
	 *  GPD0[1] = "10"  Input,  		GPD0[0] = "00"  Output 0;
	 *  
	 *  Register Bit [7 ~ 0] = 10 10 10 00
	 */
	//sleep_config = readl(S5PV210_GPD0CONPDN);		
	//sleep_config |= (1 << 7 | 1 << 5 | 1 << 3 );
	//sleep_config &= ~( 1<< 6 | 1 << 4 | 1 << 2  | 1 << 1 | 1 << 0 );
	writel(sleep_config, S5PV210_GPD0CONPDN);
	
	/*
	 *  GPD0:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPD0[3] = "00"  Pull-up / down disable, 	GPD0[2] = "00"  Pull-up / down disable;
	 *  GPD0[1] = "10"  Pull-up enable, 		GPD0[0] = "00"  Pull-up / down disable;
	 *
	 *  Register Bit [7 ~ 0] = 0000 1000
	 */
	sleep_pud = readl(S5PV210_GPD0PUDPDN);
	sleep_pud |= ( 1 << 3 );
	sleep_pud &= ~( 1<< 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 2 | 1 << 1 | 1 << 0 );
	writel(sleep_pud, S5PV210_GPD0PUDPDN);

}

void sleep_gpio_GPD1_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;

	/* 
	 *  GPD1:
	 *
	 *  GPIO Description:
	 *  
	 *  GPD1[5] : "XI2C2SCL",  		GPD1[4] : "XI2C2SDA"; 	
	 *  GPD1[3] : "XI2C1SCL",  		GPD1[2] : "XI2C1SDA"; 
	 *  GPD1[1] : "XI2C0SCL",		GPD1[0] : "XI2C0SDA";
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPD1[5] = "10"  Input,  		GPD1[4] = "10"  Input;
	 *  GPD1[3] = "01"  Output 1,  		GPD1[2] = "01"  Output 1;
	 *  GPD1[1] = "01"  Output 1,  		GPD1[0] = "01"  Output 1;
	 *  
	 *  Register Bit [11 ~ 0] = 1010 0101 0101
	 */
	sleep_config = readl(S5PV210_GPD1CONPDN);		
	sleep_config |= (1<< 11 | 1 << 9 | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	sleep_config &= ~(  1 << 10 | 1 << 8 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	writel(sleep_config, S5PV210_GPD1CONPDN);
	
	/*
	 *  GPD1:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPD1[5] = "00"  Pull-up / down disable, 	GPD1[4] = "00"  Pull-up / down disable;
	 *  GPD1[3] = "00"  Pull-up / down disable, 	GPD1[2] = "00"  Pull-up / down disable;
	 *  GPD1[1] = "00"  Pull-up / down disable, 	GPD1[0] = "00"  Pull-up / down disable;
	 *
	 *  Register Bit [11 ~ 0] = 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPD1PUDPDN);
	sleep_pud &= ~( 1<< 11 | 1 << 10 | 1 << 9| 1 << 8 | 1<< 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0 );
	writel(sleep_pud, S5PV210_GPD1PUDPDN);

}

void sleep_gpio_GPE0_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;

	/* 
	 *  GPE0:
	 *
	 *  GPIO Description:
	 *  
	 *  GPE0[7] : "0M3CAM_D4",  		GPE0[6] : "0M3CAM_D3"; 
	 *  GPE0[5] : "0M3CAM_D2",  		GPE0[4] : "0M3CAM_D1"; 	
	 *  GPE0[3] : "0M3CAM_D0",  		GPE0[2] : "0M3CAM_HSYNC"; 
	 *  GPE0[1] : "0M3CAM_VSYNC",		GPE0[0] : "0M3CAM_PCLK";
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPE0[7] : "10"  Input,  		GPE0[6] : "10"  Input; 
	 *  GPE0[5] = "10"  Input,  		GPE0[4] = "10"  Input;
	 *  GPE0[3] = "10"  Input,  		GPE0[2] = "10"  Input;
	 *  GPE0[1] = "10"  Input,  		GPE0[0] = "10"  Input;
	 *  
	 *  Register Bit [15 ~ 0] = 1010 1010 1010 1010
	 */
	sleep_config = readl(S5PV210_GPE0CONPDN);		
	sleep_config |= (1 << 15 | 1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_config &= ~( 1<< 14 | 1 << 12 | 1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_config, S5PV210_GPE0CONPDN);
	
	/*
	 *  GPE0:
	 *
	 *  GPIO sleep PUD configuration:
	 *
   	 *  GPE0[7] = "00"  Pull-up / down disable, 	GPE0[6] = "00"  Pull-up / down disable;
	 *  GPE0[5] = "00"  Pull-up / down disable, 	GPE0[4] = "00"  Pull-up / down disable;
	 *  GPE0[3] = "00"  Pull-up / down disable, 	GPE0[2] = "00"  Pull-up / down disable;
	 *  GPE0[1] = "00"  Pull-up / down disable, 	GPE0[0] = "00"  Pull-up / down disable;
	 *
	 *  Register Bit [15 ~ 0] = 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPE0PUDPDN);
	sleep_pud &= ~( 1<< 15 | 1 << 14 | 1 << 13 | 1 << 12 );
	sleep_pud &= ~( 1<< 11 | 1 << 10 | 1 << 9 | 1 << 8 | 1<< 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0 );
	writel(sleep_pud, S5PV210_GPE0PUDPDN);

}

void sleep_gpio_GPE1_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;

	/* 
	 *  GPE1:
	 *
	 *  GPIO Description:
	 *  
	 *  GPE1[4] : "CAM_0M3_Reset"; 	
	 *  GPE1[3] : "0M3CAM_MCLK",  		GPE1[2] : "0M3CAM_D7"; 
	 *  GPE1[1] : "0M3CAM_D6",			GPE1[0] : "0M3CAM_D5";
	 *
	 *  GPIO sleep configuration:
  	 *
	 *  GPE1[4] = "01"  Output 1;
	 *  GPE1[3] = "10"  Input,  		GPE1[2] = "10"  Input;
	 *  GPE1[1] = "10"  Input,  		GPE1[0] = "10"  Input;
	 *  
	 *  Register Bit [9 ~ 0] = 01 1010 1010
	 */
	sleep_config = readl(S5PV210_GPE1CONPDN);		
	sleep_config |= (  1 << 8  | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_config &= ~( 1 << 9 | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_config, S5PV210_GPE1CONPDN);
	
	/*
	 *  GPE1:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPE1[4] = "00"  Pull-up / down disable;
	 *  GPE1[3] = "00"  Pull-up / down disable, 	GPE1[2] = "00"  Pull-up / down disable;
	 *  GPE1[1] = "00"  Pull-up / down disable, 	GPE1[0] = "00"  Pull-up / down disable;
	 *
	 *  Register Bit [9 ~ 0] = 00 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPE1PUDPDN);
	sleep_pud &= ~( 1 << 9 | 1 << 8 | 1<< 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0 );
	writel(sleep_pud, S5PV210_GPE1PUDPDN);

}

void sleep_gpio_GPF0_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;

	/* 
	 *  GPF0:
	 *
	 *  GPIO Description:
	 *  
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPF0[7] : "10"  Input,  		GPF0[6] : "10"  Input; 
	 *  GPF0[5] = "10"  Input,  		GPF0[4] = "10"  Input;
	 *  GPF0[3] = "10"  Input,  		GPF0[2] = "10"  Input;
	 *  GPF0[1] = "10"  Input,  		GPF0[0] = "10"  Input;
	 *  
	 *  Register Bit [15 ~ 0] = 1010 1010 1010 1010
	 */
	sleep_config = readl(S5PV210_GPF0CONPDN);		
	sleep_config |= (1 << 15 | 1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_config &= ~( 1<< 14 | 1 << 12 | 1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_config, S5PV210_GPF0CONPDN);
	
	/*
	 *  GPF0:
	 *
	 *  GPIO sleep PUD configuration:
	 *
   	 *  GPF0[7] = "00"  Pull-up / down disable, 	GPF0[6] = "00"  Pull-up / down disable;
	 *  GPF0[5] = "00"  Pull-up / down disable, 	GPF0[4] = "00"  Pull-up / down disable;
	 *  GPF0[3] = "00"  Pull-up / down disable, 	GPF0[2] = "00"  Pull-up / down disable;
	 *  GPF0[1] = "00"  Pull-up / down disable, 	GPF0[0] = "00"  Pull-up / down disable;
	 *
	 *  Register Bit [15 ~ 0] = 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPF0PUDPDN);
	sleep_pud &= ~( 1<< 15 | 1 << 14 | 1 << 13 | 1 << 12 );
	sleep_pud &= ~( 1<< 11 | 1 << 10 | 1 << 9 | 1 << 8 | 1<< 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0 );
	writel(sleep_pud, S5PV210_GPF0PUDPDN);

}

void sleep_gpio_GPF1_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;

	/* 
	 *  GPF1:
	 *
	 *  GPIO Description:
	 *  
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPF1[7] : "10"  Input,  		GPF1[6] : "10"  Input; 
	 *  GPF1[5] = "10"  Input,  		GPF1[4] = "10"  Input;
	 *  GPF1[3] = "10"  Input,  		GPF1[2] = "10"  Input;
	 *  GPF1[1] = "10"  Input,  		GPF1[0] = "10"  Input;
	 *  
	 *  Register Bit [15 ~ 0] = 1010 1010 1010 1010
	 */
	sleep_config = readl(S5PV210_GPF1CONPDN);		
	sleep_config |= (1 << 15 | 1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_config &= ~( 1<< 14 | 1 << 12 | 1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_config, S5PV210_GPF1CONPDN);
	
	/*
	 *  GPF1:
	 *
	 *  GPIO sleep PUD configuration:
	 *
   	 *  GPF1[7] = "00"  Pull-up / down disable, 	GPF1[6] = "00"  Pull-up / down disable;
	 *  GPF1[5] = "00"  Pull-up / down disable, 	GPF1[4] = "00"  Pull-up / down disable;
	 *  GPF1[3] = "00"  Pull-up / down disable, 	GPF1[2] = "00"  Pull-up / down disable;
	 *  GPF1[1] = "00"  Pull-up / down disable, 	GPF1[0] = "00"  Pull-up / down disable;
	 *
	 *  Register Bit [15 ~ 0] = 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPF1PUDPDN);
	sleep_pud &= ~( 1<< 15 | 1 << 14 | 1 << 13 | 1 << 12 );
	sleep_pud &= ~( 1<< 11 | 1 << 10 | 1 << 9 | 1 << 8 | 1<< 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0 );
	writel(sleep_pud, S5PV210_GPF1PUDPDN);

}

void sleep_gpio_GPF2_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;

	/* 
	 *  GPF2:
	 *
	 *  GPIO Description:
	 *  
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPF2[7] : "10"  Input,  		GPF2[6] : "10"  Input; 
	 *  GPF2[5] = "10"  Input,  		GPF2[4] = "10"  Input;
	 *  GPF2[3] = "10"  Input,  		GPF2[2] = "10"  Input;
	 *  GPF2[1] = "10"  Input,  		GPF2[0] = "10"  Input;
	 *  
	 *  Register Bit [15 ~ 0] = 1010 1010 1010 1010
	 */
	sleep_config = readl(S5PV210_GPF2CONPDN);		
	sleep_config |= (1 << 15 | 1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_config &= ~( 1<< 14 | 1 << 12 | 1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_config, S5PV210_GPF2CONPDN);
	
	/*
	 *  GPF1:
	 *
	 *  GPIO sleep PUD configuration:
	 *
   	 *  GPF2[7] = "00"  Pull-up / down disable, 	GPF2[6] = "00"  Pull-up / down disable;
	 *  GPF2[5] = "00"  Pull-up / down disable, 	GPF2[4] = "00"  Pull-up / down disable;
	 *  GPF2[3] = "00"  Pull-up / down disable, 	GPF2[2] = "00"  Pull-up / down disable;
	 *  GPF2[1] = "00"  Pull-up / down disable, 	GPF2[0] = "00"  Pull-up / down disable;
	 *
	 *  Register Bit [15 ~ 0] = 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPF2PUDPDN);
	sleep_pud &= ~( 1<< 15 | 1 << 14 | 1 << 13 | 1 << 12 );
	sleep_pud &= ~( 1<< 11 | 1 << 10 | 1 << 9 | 1 << 8 | 1<< 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0 );
	writel(sleep_pud, S5PV210_GPF2PUDPDN);

}

void sleep_gpio_GPF3_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;

	/* 
	 *  GPF3:
	 *
	 *  GPIO Description:
	 *  
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPF3[5] = "10"  Input,  		GPF3[4] = "10"  Input;
	 *  GPF3[3] = "10"  Input,  		GPF3[2] = "10"  Input;
	 *  GPF3[1] = "10"  Input,  		GPF3[0] = "10"  Input;
	 *  
	 *  Register Bit [11 ~ 0] = 1010 1010 1010
	 */
	sleep_config = readl(S5PV210_GPF3CONPDN);		
	sleep_config |= ( 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_config &= ~(  1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_config, S5PV210_GPF3CONPDN);
	
	/*
	 *  GPF3:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPF3[5] = "00"  Pull-up / down disable, 	GPF3[4] = "00"  Pull-up / down disable;
	 *  GPF3[3] = "00"  Pull-up / down disable, 	GPF3[2] = "00"  Pull-up / down disable;
	 *  GPF3[1] = "00"  Pull-up / down disable, 	GPF3[0] = "00"  Pull-up / down disable;
	 *
	 *  Register Bit [11 ~ 0] =  0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPF3PUDPDN);
	sleep_pud &= ~( 1<< 11 | 1 << 10 | 1 << 9 | 1 << 8 | 1<< 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0 );
	writel(sleep_pud, S5PV210_GPF3PUDPDN);

}

void sleep_gpio_GPG0_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;

	/* 
	 *  GPG0:
	 *
	 *  GPIO Description:
	 *  
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPG0[6] : "10"  Input; 
	 *  GPG0[5] = "10"  Input,  		GPG0[4] = "10"  Input;
	 *  GPG0[3] = "10"  Input,  		GPG0[2] = "10"  Input;
	 *  GPG0[1] = "10"  Input,  		GPG0[0] = "10"  Input;
	 *  
	 *  Register Bit [13 ~ 0] = 10 1010 1010 1010
	 */
	sleep_config = readl(S5PV210_GPG0CONPDN);		
	sleep_config |= (1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_config &= ~(1 << 12 | 1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_config, S5PV210_GPG0CONPDN);
	
	/*
	 *  GPG0:
	 *
	 *  GPIO sleep PUD configuration:
	 *
   	 *  GPG0[6] = "00"  Pull-up / down disable;
	 *  GPG0[5] = "00"  Pull-up / down disable, 	GPG0[4] = "00"  Pull-up / down disable;
	 *  GPG0[3] = "00"  Pull-up / down disable, 	GPG0[2] = "00"  Pull-up / down disable;
	 *  GPG0[1] = "00"  Pull-up / down disable, 	GPG0[0] = "00"  Pull-up / down disable;
	 *
	 *  Register Bit [13 ~ 0] = 00 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPG0PUDPDN);
	sleep_pud &= ~(1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_pud &= ~(1 << 12 | 1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_pud, S5PV210_GPG0PUDPDN);

}

void sleep_gpio_GPG1_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;

	/* 
	 *  GPG1:
	 *
	 *  GPIO Description:
	 *  
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPG1[6] : "10"  Input; 
	 *  GPG1[5] = "10"  Input,  		GPG1[4] = "10"  Input;
	 *  GPG1[3] = "10"  Input,  		GPG1[2] = "10"  Input;
	 *  GPG1[1] = "10"  Input,  		GPG1[0] = "10"  Input;
	 *  
	 *  Register Bit [13 ~ 0] = 10 1010 1010 1010
	 */
	sleep_config = readl(S5PV210_GPG1CONPDN);		
	sleep_config |= (1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_config &= ~(1 << 12 | 1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_config, S5PV210_GPG1CONPDN);
	
	/*
	 *  GPG1:
	 *
	 *  GPIO sleep PUD configuration:
	 *
   	 *  GPG1[6] = "00"  Pull-up / down disable;
	 *  GPG1[5] = "00"  Pull-up / down disable, 	GPG1[4] = "00"  Pull-up / down disable;
	 *  GPG1[3] = "00"  Pull-up / down disable, 	GPG1[2] = "00"  Pull-up / down disable;
	 *  GPG1[1] = "00"  Pull-up / down disable, 	GPG1[0] = "00"  Pull-up / down disable;
	 *
	 *  Register Bit [13 ~ 0] = 00 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPG1PUDPDN);
	sleep_pud &= ~(1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_pud &= ~(1 << 12 | 1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_pud, S5PV210_GPG1PUDPDN);

}

void sleep_gpio_GPG2_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;

	/* 
	 *  GPG2:
	 *
	 *  GPIO Description:
	 *  
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPG2[6] : "10"  Input; 
	 *  GPG2[5] = "10"  Input,  		GPG2[4] = "10"  Input;
	 *  GPG2[3] = "10"  Input,  		GPG2[2] = "10"  Input;
	 *  GPG2[1] = "10"  Input,  		GPG2[0] = "10"  Input;
	 *  
	 *  Register Bit [13 ~ 0] = 10 1010 1010 1010
	 */
	sleep_config = readl(S5PV210_GPG2CONPDN);		
	sleep_config |= (1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_config &= ~(1 << 12 | 1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_config, S5PV210_GPG2CONPDN);
	
	/*
	 *  GPG2:
	 *
	 *  GPIO sleep PUD configuration:
	 *
   	 *  GPG2[6] = "00"  Pull-up / down disable;
	 *  GPG2[5] = "00"  Pull-up / down disable, 	GPG2[4] = "00"  Pull-up / down disable;
	 *  GPG2[3] = "00"  Pull-up / down disable, 	GPG2[2] = "00"  Pull-up / down disable;
	 *  GPG2[1] = "00"  Pull-up / down disable, 	GPG2[0] = "00"  Pull-up / down disable;
	 *
	 *  Register Bit [13 ~ 0] = 00 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPG2PUDPDN);
	sleep_pud &= ~(1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_pud &= ~(1 << 12 | 1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_pud, S5PV210_GPG2PUDPDN);

}

void sleep_gpio_GPG3_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;

	/* 
	 *  GPG3:
	 *
	 *  GPIO Description:
	 *  
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPG3[6] : "10"  Input; 
	 *  GPG3[5] = "10"  Input,  		GPG3[4] = "10"  Input;
	 *  GPG3[3] = "10"  Input,  		GPG3[2] = "10"  Input;
	 *  GPG3[1] = "10"  Input,  		GPG3[0] = "10"  Input;
	 *  
	 *  Register Bit [13 ~ 0] = 10 1010 1010 1010
	 */
	sleep_config = readl(S5PV210_GPG3CONPDN);		
	sleep_config |= (1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_config &= ~(1 << 12 | 1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_config, S5PV210_GPG3CONPDN);
	
	/*
	 *  GPG3:
	 *
	 *  GPIO sleep PUD configuration:
	 *
   	 *  GPG3[6] = "00"  Pull-up / down disable;
	 *  GPG3[5] = "00"  Pull-up / down disable, 	GPG3[4] = "00"  Pull-up / down disable;
	 *  GPG3[3] = "00"  Pull-up / down disable, 	GPG3[2] = "00"  Pull-up / down disable;
	 *  GPG3[1] = "00"  Pull-up / down disable, 	GPG3[0] = "00"  Pull-up / down disable;
	 *
	 *  Register Bit [13 ~ 0] = 00 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPG3PUDPDN);
	sleep_pud &= ~(1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_pud &= ~(1 << 12 | 1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_pud, S5PV210_GPG3PUDPDN);

}

void sleep_gpio_GPI_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;

	/* 
	 *  GPI:
	 *
	 *  GPIO Description:
	 *  
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPI[6] : "10"  Input; 
	 *  GPI[5] = "10"  Input,  		GPI[4] = "10"  Input;
	 *  GPI[3] = "10"  Input,  		GPI[2] = "10"  Input;
	 *  GPI[1] = "10"  Input,  		GPI[0] = "10"  Input;
	 *  
	 *  Register Bit [13 ~ 0] = 10 1010 1010 1010
	 */
	sleep_config = readl(S5PV210_GPICONPDN);		
	sleep_config |= (1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_config &= ~(1 << 12 | 1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_config, S5PV210_GPICONPDN);
	
	/*
	 *  GPI:
	 *
	 *  GPIO sleep PUD configuration:
	 *
   	 *  GPI[6] = "00"  Pull-up / down disable;
	 *  GPI[5] = "00"  Pull-up / down disable, 	GPI[4] = "00"  Pull-up / down disable;
	 *  GPI[3] = "00"  Pull-up / down disable, 	GPI[2] = "00"  Pull-up / down disable;
	 *  GPI[1] = "00"  Pull-up / down disable, 	GPI[0] = "00"  Pull-up / down disable;
	 *
	 *  Register Bit [13 ~ 0] = 00 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPIPUDPDN);
	sleep_pud &= ~( 1 << 13 | 1 << 12 );
	sleep_pud &= ~( 1<< 11 | 1 << 10 | 1 << 9 | 1 << 8 | 1<< 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0 );
	writel(sleep_pud, S5PV210_GPIPUDPDN);

}

void sleep_gpio_GPJ0_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;
	
	/* 
	 *  GPJ0:
	 *
  	 *  GPIO Description:
	 *  GPJ0[7]: "CAM_D7", GPJ0[6]: "CAM_D6", GPJ0[5]: "CAM_D5", GPJ0[4]: "CAM_D4";
	 *  GPJ0[3]: "CAM_D3", GPJ0[2]: "CAM_D2", GPJ0[1]: "CAM_D1", GPJ0[0]: "CAM_D0";
        *
	 *  GPIO sleep configuration:
	 *
	 *  GPJ0[7] = "10" Input, GPJ0[6] = "10" Input, GPJ0[5] = "10" Input, GPJ0[4] = "10" Input;
	 *  GPJ0[3] = "10" Input, GPJ0[2] = "10" Input, GPJ0[1] = "10" Input, GPJ0[0] = "10" Input;
	 *  
	 *  Register Bit [15 ~ 0] = 1010 1010 1010 1010
	 */
	sleep_config = readl(S5PV210_GPJ0CONPDN);
	sleep_config |= (1 << 15 | 1 << 13 | 1 << 11 | 1 << 9 | 1<< 7 | 1 << 5 | 1<< 3 | 1 << 1);
	sleep_config &= ~( 1 << 14 |  1 << 12 | 1 << 10 | 1 << 8 | 1 << 6 |  1 << 4 | 1 << 2 | 1 << 0 );		
	writel(sleep_config, S5PV210_GPJ0CONPDN);
	
	/* 
	 *  GPJ0:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPJ0[7] = "00"  Pull-up / down disable, 	GPJ0[6] = "00"  Pull-up / down disable;
	 *  GPJ0[5] = "00"  Pull-up / down disable, 	GPJ0[4] = "00"  Pull-up / down disable;
	 *  GPJ0[3] = "00"  Pull-up / down disable, GPJ0[2] = "00"  Pull-up / down disable;
	 *  GPJ0[1] = "00"  Pull-up / down disable, GPJ0[0] = "00"  Pull-up / down disable;;
	 *  
	 *  Register Bit [15 ~ 0] = 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPJ0PUDPDN);
	sleep_pud &= ~( 1 << 15 | 1 << 14 | 1 << 13 | 1 << 12 | 1 << 11 | 1 << 10 | 1 << 9 | 1 << 8 );
	sleep_pud &= ~( 1 << 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0);
	writel(sleep_pud, S5PV210_GPJ0PUDPDN);
	
}

void sleep_gpio_GPJ1_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;
	
	/* 
	 *  GPJ1:
	 *
  	 *  GPIO Description:
	 *  GPJ1[5]: "VDD_CAM_EN", GPJ1[4]: "CAM_MCLK";
	 *  GPJ1[3]: "CAM_2M_Reset", GPJ1[2]: "CAP_HSYNC", GPJ1[1]: "CAM_VSYNC", GPJ1[0]: "CAM_PCLK";
        *
	 *  GPIO sleep configuration:
	 *
	 *  GPJ1[5] = "00" Output 0, GPJ1[4] = "10" Input,;
	 *  GPJ1[3] = "01" Output 1, GPJ1[2] = "10" Input, GPJ1[1] = "10" Input, GPJ1[0] = "10" Input;
	 *  
	 *  Register Bit [11 ~ 0] = 0010 0110 1010
	 */
	sleep_config = readl(S5PV210_GPJ1CONPDN);
	sleep_config |= (  1 << 9 | 1 << 6  | 1 << 5 | 1 << 3 | 1 << 1 );
	sleep_config &= ~(  1 << 11 | 1 << 10 | 1 << 8 | 1 << 7 |  1 << 4 | 1 << 2 | 1 << 0 );		
	writel(sleep_config, S5PV210_GPJ1CONPDN);
	
	/* 
	 *  GPJ1:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPJ1[5] = "00"  Pull-up / down disable, 	GPJ1[4] = "00"  Pull-up / down disable;
	 *  GPJ1[3] = "00"  Pull-up / down disable,     GPJ1[2] = "00"  Pull-up / down disable;
	 *  GPJ1[1] = "00"  Pull-up / down disable,     GPJ1[0] = "00"  Pull-up / down disable;;
	 *  
	 *  Register Bit [11 ~ 0] = 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPJ1PUDPDN);
	sleep_pud &= ~( 1 << 11 | 1 << 10 | 1 << 9 | 1 << 8 );
	sleep_pud &= ~( 1 << 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0);
	writel(sleep_pud, S5PV210_GPJ1PUDPDN);
	
}

void sleep_gpio_GPJ2_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;
	
	/* 
	 *  GPJ2:
	 *
  	 *  GPIO Description:
	 *  GPJ2[7]: "Current Limmit", GPJ2[6]: "OTG_EN", GPJ2[5]: "Charge_EN", GPJ2[4]: "LCD_POWER1";
	 *  GPJ2[3]: "None", GPJ2[2]: "VGH_CTRL", GPJ2[1]: "WF_RSTn", GPJ2[0]: "HDMI_EN";
        *
	 *  GPIO sleep configuration:
	 *
	 *  GPJ2[7] = "11" Previous, GPJ2[6] = "11" Previous, GPJ2[5] = "11" Previous, GPJ2[4] = "00" Output 0;
	 *  GPJ2[3] = "10" input, GPJ2[2] = "00" Output 0, GPJ2[1] = "10" Input, GPJ2[0] = "00" Output 0;
	 *  
	 *  Register Bit [15 ~ 0] = 1111 1100 1000 1000
	 */
	sleep_config = readl(S5PV210_GPJ2CONPDN);
	sleep_config |= (1 << 15 | 1 << 14 | 1 << 13 | 1 << 12 | 1 << 11 | 1 << 10 | 1 << 7 | 1 << 3 | 1 << 1 );
	sleep_config &= ~( 1 << 9 | 1 << 8 |  1 << 6 | 1<< 5 | 1 << 4 | 1 << 2 /*| 1 << 1*/ | 1 << 0 );		
	writel(sleep_config, S5PV210_GPJ2CONPDN);
	
	/* 
	 *  GPJ2:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPJ2[7] = "00"  Pull-up / down disable, 	GPJ2[6] = "00"  Pull-up / down disable;
	 *  GPJ2[5] = "00"  Pull-up / down disable, 	GPJ2[4] = "00"  Pull-up / down disable;
	 *  GPJ2[3] = "00"  Pull-up / down disable,     GPJ2[2] = "00"  Pull-up / down disable;
	 *  GPJ2[1] = "00"  Pull-up / down disable,     GPJ2[0] = "00"  Pull-up / down disable;;
	 *  
	 *  Register Bit [15 ~ 0] = 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPJ2PUDPDN);
	sleep_pud &= ~( 1 << 15 | 1 << 14 | 1 << 13 | 1 << 12 | 1 << 11 | 1 << 10 | 1 << 9 | 1 << 8 );
	sleep_pud &= ~( 1 << 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0);
	writel(sleep_pud, S5PV210_GPJ2PUDPDN);
	
}

void sleep_gpio_GPJ3_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;
	
	/* 
	 *  GPJ3:
	 *
  	 *  GPIO Description:
	 *  GPJ3[7]: "USB_CONTROL", GPJ3[6]: "None", GPJ3[5]: "BT_RSTn", GPJ3[4]: "Touch_RES";
	 *  GPJ3[3]: "None", GPJ3[2]: "CAM_2M_PWDN", GPJ3[1]: "AVDD_CTRL", GPJ3[0]: "LCD_BL_EN";
         *
	 *  GPIO sleep configuration:
	 *
	 *  GPJ3[7] = "11" Previous, GPJ3[6] = "10" Input, GPJ3[5] = "10" input, GPJ3[4] = "10" input;
	 *  GPJ3[3] = "10" Input, GPJ3[2] = "01" Output 1, GPJ3[1] = "00" Output 0, GPJ3[0] = "00" Output 0;
	 *  
	 *  Register Bit [15 ~ 0] = 1110 1010 1001 0000
	 */
	sleep_config = readl(S5PV210_GPJ3CONPDN);
	sleep_config |= (1 << 15 | 1 << 14 | 1 << 13  | 1 << 11| 1 << 9 | 1 << 7  | 1 << 4);
	sleep_config &= ~( 1 << 12  | 1 << 10 | 1 << 8 | 1 << 6 | 1 << 5 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0 );		
	writel(sleep_config, S5PV210_GPJ3CONPDN);
	
	/* 
	 *  GPJ3:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPJ3[7] = "00"  Pull-up / down disable, 	GPJ3[6] = "00"  Pull-up / down disable;
	 *  GPJ3[5] = "00"  Pull-up / down disable, 	GPJ3[4] = "00"  Pull-up / down disable;
	 *  GPJ3[3] = "00"  Pull-up / down disable,     GPJ3[2] = "00"  Pull-up / down disable;
	 *  GPJ3[1] = "00"  Pull-up / down disable,     GPJ3[0] = "00"  Pull-up / down disable;;
	 *  
	 *  Register Bit [15 ~ 0] = 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPJ3PUDPDN);
	sleep_pud &= ~( 1 << 15 | 1 << 14 | 1 << 13 | 1 << 12 | 1 << 11 | 1 << 10 | 1 << 9 | 1 << 8 );
	sleep_pud &= ~( 1 << 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0);
	writel(sleep_pud, S5PV210_GPJ3PUDPDN);
	
}

void sleep_gpio_GPJ4_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;
	
	/* 
	 *  GPJ4:
	 *
  	 *  GPIO Description:
	 *  GPJ4[4]: "EXT_CARD_EN";
	 *  GPJ4[3]: "CAM_0M3_PWDN", GPJ4[2]: "None", GPJ4[1]: "None", GPJ4[0]: "BAT_DET_EN";
        *
	 *  GPIO sleep configuration:
	 *
	 *  GPJ4[4] = "01" Output 1;
	 *  GPJ4[3] = "01" Output 1, GPJ4[2] = "10" Input 0, GPJ4[1] = "10" Input, GPJ4[0] = "10" Input;
	 *  
	 *  Register Bit [9 ~ 0] = 01 0110 1010
	 */
	sleep_config = readl(S5PV210_GPJ4CONPDN);
	sleep_config |= ( 1 << 8 | 1 << 6 | 1 << 5 | 1 << 3 | 1 << 1 );
	sleep_config &= ~( 1 << 9 |  1 << 7  | 1 << 4 | 1 << 2 | 1 << 0 );		
	writel(sleep_config, S5PV210_GPJ4CONPDN);
	
	/* 
	 *  GPJ4:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPJ4[4] = "00"  Pull-up / down disable;
	 *  GPJ4[3] = "00"  Pull-up / down disable,     GPJ4[2] = "00"  Pull-up / down disable;
	 *  GPJ4[1] = "00"  Pull-up / down disable,     GPJ4[0] = "00"  Pull-up / down disable;;
	 *  
	 *  Register Bit [9 ~ 0] = 00 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPJ3PUDPDN);
	sleep_pud &= ~( 1 << 9 | 1 << 8 );
	sleep_pud &= ~( 1 << 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0);
	writel(sleep_pud, S5PV210_GPJ3PUDPDN);
	
}



unsigned long sleep_gph0_pud = 0;
unsigned long sleep_gph1_pud = 0;
unsigned long sleep_gph2_pud = 0;
unsigned long sleep_gph3_pud = 0;

void sleep_gpio_GPH_PUD_store(void)
{
	sleep_gph0_pud = readl(S5PV210_GPH0PUD);
	sleep_gph1_pud = readl(S5PV210_GPH1PUD);
	sleep_gph2_pud = readl(S5PV210_GPH2PUD);
	sleep_gph3_pud = readl(S5PV210_GPH3PUD);
}

void resume_gpio_GPH_PUD_restore(void)
{
	writel(sleep_gph0_pud, S5PV210_GPH0PUD);
	writel(sleep_gph1_pud, S5PV210_GPH1PUD);
	writel(sleep_gph2_pud, S5PV210_GPH2PUD);
	writel(sleep_gph3_pud, S5PV210_GPH3PUD);
}

void sleep_gpio_GPH_PUD_config(void)
{
	
	unsigned long sleep_pud = 0;
	
	sleep_gpio_GPH_PUD_store();

	/* 
	 *  GPH0:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  "00"  Pull-up / down disable;
	 *  register bit [15 ~ 0]: 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPH0PUD);
	sleep_pud &= ~0xFF;
	writel(sleep_pud, S5PV210_GPH0PUD);
	

	/* 
	 *  GPH1:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  "00"  Pull-up / down disable;
	 *  register bit [15 ~ 0]: 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPH1PUD);
	sleep_pud &= ~0xFF;
	writel(sleep_pud, S5PV210_GPH1PUD);

	/* 
	 *  GPH2:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  "00"  Pull-up / down disable;
	 *  register bit [15 ~ 0]: 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPH2PUD);
	sleep_pud &= ~0xFF;
	writel(sleep_pud, S5PV210_GPH2PUD);

	/* 
	 *  GPH3:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPH3 PUD = "00"  Pull-up / down disable;
	 *  register bit [15 ~ 0]: 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPH3PUD);
	sleep_pud &= ~0xFF;
	writel(sleep_pud, S5PV210_GPH3PUD);
	
}

void sleep_gpio_GPH_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_dat = 0;
	/* 
     *  GPH0: 
     *  
     *  GPIO Description: 
     *  
     *  GPIO sleep configuration: 
     * 
     */
	sleep_config = readl(S5PV210_GPH0CON);    
	sleep_config &= ~( 0xF << 3*4 | 0xF << 2*4 );
	sleep_config |= ( 0x0 << 3*4 | 0x0 << 2*4 );
	writel(sleep_config, S5PV210_GPH0CON);   
	
	/*
     *  GPH2:
     *  
     *  GPIO Description: 
     *  
     *  GPIO sleep configuration:
     *  
     */
	sleep_config = readl(S5PV210_GPH2CON);    
	sleep_config &= ~( 0xF << 2*4 );
	sleep_config |= ( 0x0 << 2*4 );
	writel(sleep_config, S5PV210_GPH2CON);    
                                	
	/* 
	 *  GPH3:
	 *
  	 *  GPIO Description:
	 *  GPH3[0]: "WIFI_MAC_WAKE";
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPH3[0] CON = "0001" Output ;
	 *  GPH3[0] DAT	= 0;
	 *  
	 */
	sleep_config = readl(S5PV210_GPH3CON);
	sleep_config &= ~(0xF << 0);
	sleep_config |= 0x1;		
	writel(sleep_config, S5PV210_GPH3CON);


	sleep_dat = readl(S5PV210_GPH3DAT);
	sleep_dat &= ~(0x1 << 0);		
	writel(sleep_dat, S5PV210_GPH3DAT);

	/* GPH PUD config */
	sleep_gpio_GPH_PUD_config();
	
}

void resume_gpio_GPH_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_dat = 0;
	unsigned long sleep_pud = 0;
	
	/* 
	 *  GPH3:
	 *
  	 *  GPIO Description:
	 *  GPH3[0]: "WIFI_MAC_WAKE";
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPH3[0] CON = "0001" Output ;
	 *  GPH3[0] DAT	= 0;
	 *  
	 */
	sleep_config = readl(S5PV210_GPH3CON);
	sleep_config &= ~(0xF << 0);
	sleep_config |= 0x1;		
	writel(sleep_config, S5PV210_GPH3CON);


	sleep_dat = readl(S5PV210_GPH3DAT);
	sleep_dat &= ~(0x1 << 0);		
	writel(sleep_dat, S5PV210_GPH3DAT);
	
	/* 
	 * 
	 *
	 *  GPIO sleep GPH PUD restore configuration:
	 *
	 *  
	 */
	resume_gpio_GPH_PUD_restore();
	
}





void sleep_gpio_config(void)
{
	sleep_gpio_GPA0_config();
	sleep_gpio_GPA1_config();
	sleep_gpio_GPB_config();
	sleep_gpio_GPC0_config();
	sleep_gpio_GPC1_config();
	sleep_gpio_GPD0_config();
	sleep_gpio_GPD1_config();
	sleep_gpio_GPE0_config();
	sleep_gpio_GPE1_config();
	sleep_gpio_GPF0_config();
	sleep_gpio_GPF1_config();
	sleep_gpio_GPF2_config();
	sleep_gpio_GPF3_config();
	sleep_gpio_GPG0_config();
	sleep_gpio_GPG1_config();
	sleep_gpio_GPG2_config();
	sleep_gpio_GPG3_config();	
	sleep_gpio_GPI_config();
	sleep_gpio_GPJ0_config();
	sleep_gpio_GPJ1_config();
	sleep_gpio_GPJ2_config();
	sleep_gpio_GPJ3_config();
	sleep_gpio_GPJ4_config();
	sleep_gpio_GPH_config();
}

void resume_gpio_config(void)
{
	resume_gpio_GPH_config();
}
#endif

#ifdef CONFIG_PRODUCT_PDC3
void sleep_gpio_GPA0_config(void)
{
	//unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;
	/* 
	 *  GPA0:
	 *
  	 *  GPIO Description:
	 *  GPA0[7]: "None", GPA0[6]: "None", GPA0[5]: "None", GPA0[4]: "None";
	 *  GPA0[3]: "BT_RTS", GPA0[2]: "BT_CTS", GPA0[1]: "BT_TX", GPA0[0]: "BT_RX";
        *
	 *  GPIO sleep configuration:
	 *
	 *  GPA0[7] = "10" Input, GPA0[6] = "10" Input, GPA0[5] = "10" Input, GPA0[4] = "10" Input;
	 *  GPA0[3] = "10" Input, GPA0[2] = "10" Input, GPA0[1] = "10" Input, GPA0[0] = "10" Input;
	 *  
	 *  Register Bit [15 ~ 0] = 1010 1010 1010 1010
	 */
	//sleep_config = readl(S5PV210_GPA0CONPDN);
	//sleep_config |= (1 << 15 | 1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	//sleep_config &= ~( 1 << 14 | 1 << 12 | 1<< 10 | 1 << 8  | 1 << 6  | 1<< 4  | 1 << 2   | 1 << 0 );		
	//writel(sleep_config, S5PV210_GPA0CONPDN);
	writel(0,S5PV210_GPA0CONPDN);
	/* 
	 *  GPA0:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPA0[7] = "00"  Pull-up / down disable, 	GPA0[6] = "00"  Pull-up / down disable;
	 *  GPA0[5] = "00"  Pull-up / down disable, 	GPA0[4] = "00"  Pull-up / down disable;
	 *  GPA0[3] = "00"  Pull-up / down disable,     GPA0[2] = "00"  Pull-up / down disable;
	 *  GPA0[1] = "00"  Pull-up / down disable,     GPA0[0] = "00"  Pull-up / down disable;;
	 *  
	 *  Register Bit [15 ~ 0] = 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPA0PUDPDN);
	sleep_pud &= ~( 1 << 15 | 1 << 14 | 1 << 13 | 1 << 12 | 1 << 11 | 1 << 10 | 1 << 9 | 1 << 8 );
	sleep_pud &= ~( 1 << 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0);
	writel(sleep_pud, S5PV210_GPA0PUDPDN);

}

void sleep_gpio_GPA1_config(void)
{
	//unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;
	/* 
	 *  GPA1:
	 *
  	 *  GPIO Description:
	 *  GPA1[3]: "INAND_EN", GPA1[2]: "LEVEL_EN", GPA1[1]: "Debug_TX", GPA1[0]: "Debug_RX";
         *
	 *  GPIO sleep configuration:
	 *
	 *  GPA1[3] = "10" Input, GPA1[2] = "10" Input, GPA1[1] = "10" Intput, GPA1[0] = "10" Input;
	 *  
	 *  Register Bit [7 ~ 0] = 1010 1010 
	 */
	//sleep_config = readl(S5PV210_GPA1CONPDN);
	//sleep_config |= (1 << 7 | 1 << 5  | 1 << 3 | 1 << 1);
	//sleep_config &= ~(1 << 6 | 1 << 4  | 1 << 2  | 1 << 0);	
	//writel(sleep_config, S5PV210_GPA1CONPDN);
	writel(0,S5PV210_GPA1CONPDN);
	/* 
	 *  GPA1:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPA1[3] = "00"  Pull-up / down disable,     GPA1[2] = "00"  Pull-up / down disable;
	 *  GPA1[1] = "00"  Pull-up / down disable,     GPA1[0] = "00"  Pull-up / down disable;;
	 *  
	 *  Register Bit [7 ~ 0] = 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPA1PUDPDN);
	sleep_pud &= ~(1 << 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0);
	writel(sleep_pud, S5PV210_GPA1PUDPDN);

}

void sleep_gpio_GPB_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;
	/* 
	 *  GPB:
	 *
  	 *  GPIO Description:
	 *  GPB[7]: "None", GPB[6]: "None", GPB[5]: "None", GPB[4]: "None";
	 *  GPB[3]: "LCD_CS", GPB[2]: "INAND_RST", GPB[1]: "Sub_SDATA", GPB[0]: "Sub_SCLK";
         *
	 *  GPIO sleep configuration:
	 *
	 *  GPB[7] = "10" Input, GPB[6] = "10" Input,  GPB[5] = "10" Input, GPB[4] = "10" Input, 
	 *  GPB[3] = "10" Input, GPB[2] = "10" Input,  GPB[1] = "01" Output 1, GPB[0] = "01" Output 1;
	 *  
	 *  Register Bit [15 ~ 0] = 1010 1010 1010 0101
	 */
	sleep_config = readl(S5PV210_GPBCONPDN);
	sleep_config |= (1 << 15 |  1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1<< 5 | 1 << 2 | 1 << 0);
	sleep_config &= ~( 1 << 14 | 1 << 12 | 1 << 10 | 1 << 8 | 1 << 6  | 1 << 4 | 1 << 3 | 1<< 1 );		
	writel(sleep_config, S5PV210_GPBCONPDN);

	/* 
	 *  GPB:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPB[7] = "00"  Pull-up / down disable, 	GPB[6] = "00"  Pull-up / down disable;
	 *  GPB[5] = "00"  Pull-up / down disable, 	GPB[4] = "00"  Pull-up / down disable;
	 *  GPB[3] = "00"  Pull-up / down disable,      GPB[2] = "00"  Pull-up / down disable;
	 *  GPB[1] = "00"  Pull-up / down disable,      GPB[0] = "00"  Pull-up / down disable;;
	 *  
	 *  Register Bit [15 ~ 0] = 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPBPUDPDN);
	sleep_pud &= ~( 1 << 15 | 1 << 14 | 1 << 13 | 1 << 12 | 1 << 11 | 1 << 10 | 1 << 9 | 1 << 8 );
	sleep_pud &= ~( 1 << 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0);
	writel(sleep_pud, S5PV210_GPBPUDPDN);

}

void sleep_gpio_GPC0_config(void)
{
	//unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;
	/* 
	 *  GPC0:
	 *
  	 *  GPIO Description:
	 *  GPC0[4]: "MUTE";
	 *  GPC0[3]: "LCD_STBYB", GPC0[2]: "LCD_RESET", GPC0[1]: "RSDn", GPC0[0]: "DAC_MUTE";
         *
	 *  GPIO sleep configuration:
	 *
	 *  GPC0[4] = "10" Input;
	 *  GPC0[3] = "10" Input, GPC0[2] = "10" Input, GPC0[1] = "10" Input, GPC0[0] = "10" Input;
	 *  
	 *  Register Bit [9 ~ 0] = 10 1010 1010
	 */
	//sleep_config = readl(S5PV210_GPC0CONPDN);
	//sleep_config |= ( 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1<< 1 );
	//sleep_config &= ~(  1 << 8 | 1 << 6 | 1<< 4 | 1 << 2 | 1 << 0);		
	//writel(sleep_config, S5PV210_GPC0CONPDN);
	writel(0,S5PV210_GPC0CONPDN);

	/* 
	 *  GPC0:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPC0[4] = "00"  Pull-up / down disable;
	 *  GPC0[3] = "00"  Pull-up / down disable,      GPC0[2] = "00"  Pull-up / down disable;
	 *  GPC0[1] = "00"  Pull-up / down disable,      GPC0[0] = "00"  Pull-up / down disable;;
	 *  
	 *  Register Bit [9 ~ 0] = 00 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPC0PUDPDN);
	sleep_pud &= ~( 1 << 9 | 1 << 8 );
	sleep_pud &= ~( 1 << 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0);
	writel(sleep_pud, S5PV210_GPC0PUDPDN);

}

void sleep_gpio_GPC1_config(void)
{
	//unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;
	/* 
	 *  GPC1: DO NOT USE
	 *
  	 * 
	 *  Register Bit [9 ~ 0] = 10 1010 1010
	 */
	//sleep_config = readl(S5PV210_GPC1CONPDN);
	//sleep_config |= ( 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 <<1 );
	//sleep_config &= ~( 1 << 8 | 1 << 6 | 1<< 4 | 1 << 2 | 1<< 0 );		
	//writel(sleep_config, S5PV210_GPC1CONPDN);
	writel(0,S5PV210_GPC1CONPDN);
	/* 
	 *  GPC1:
	 *
	 *  GPIO sleep PUD configuration:
	 *  
	 *  Register Bit [9 ~ 0] = 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPC1PUDPDN);
	sleep_pud &= ~( 1 << 9 | 1 << 8 );
	sleep_pud &= ~( 1 << 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0);
	writel(sleep_pud, S5PV210_GPC1PUDPDN);

}

void sleep_gpio_GPD0_config(void)
{
	//unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;

	/* 
	 *  GPD0:
	 *
	 *  GPIO Description:
	 *
	 *  GPD0[3] : "CAMERA_I2C_CLK",  		GPD0[2] : "CAMERA_I2C_DATA"; 
	 *  GPD0[1] : "LCD_BL_ADJ",			GPD0[0] : " KEY_LED ";
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPD0[3] = "10"  Input,  		GPD0[2] = "10"  Input;
	 *  GPD0[1] = "10"  Input,  		GPD0[0] = "10"  Input;
	 *  
	 *  Register Bit [7 ~ 0] = 10 10 10 10
	 */
	//sleep_config = readl(S5PV210_GPD0CONPDN);		
	//sleep_config |= (1 << 7 | 1 << 5 | 1 << 3 | 1 << 1 );
	//sleep_config &= ~( 1<< 6 | 1 << 4 | 1 << 2  | 1 << 0 );
	//writel(sleep_config, S5PV210_GPD0CONPDN);
	writel(0,S5PV210_GPD0CONPDN);
	/*
	 *  GPD0:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPD0[3] = "00"  Pull-up / down disable, 	GPD0[2] = "00"  Pull-up / down disable;
	 *  GPD0[1] = "00"  Pull-up / down disable,	GPD0[0] = "00"  Pull-up / down disable;
	 *
	 *  Register Bit [7 ~ 0] = 0000 1000
	 */
	sleep_pud = readl(S5PV210_GPD0PUDPDN);
	sleep_pud &= ~( 1<< 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3  | 1 << 2 | 1 << 1 | 1 << 0 );
	writel(sleep_pud, S5PV210_GPD0PUDPDN);

}

void sleep_gpio_GPD1_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;

	/* 
	 *  GPD1:
	 *
	 *  GPIO Description:
	 *  
	 *  GPD1[5] : "XI2C2SCL",  		GPD1[4] : "XI2C2SDA"; 	
	 *  GPD1[3] : "XI2C1SCL",  		GPD1[2] : "XI2C1SDA"; 
	 *  GPD1[1] : "XI2C0SCL",		GPD1[0] : "XI2C0SDA";
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPD1[5] = "10"  Input,  		GPD1[4] = "10"  Input;
	 *  GPD1[3] = "01"  Output 1,  		GPD1[2] = "01"  Output 1;
	 *  GPD1[1] = "01"  Output 1,  		GPD1[0] = "01"  Output 1;
	 *  
	 *  Register Bit [11 ~ 0] = 1010 0101 0101
	 */
	

	sleep_config = readl(S5PV210_GPD1CONPDN);		
	sleep_config |= (1<< 11 | 1 << 9 | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	sleep_config &= ~(  1 << 10 | 1 << 8 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	writel(sleep_config, S5PV210_GPD1CONPDN);
	
	/*
	 *  GPD1:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPD1[5] = "00"  Pull-up / down disable, 	GPD1[4] = "00"  Pull-up / down disable;
	 *  GPD1[3] = "00"  Pull-up / down disable, 	GPD1[2] = "00"  Pull-up / down disable;
	 *  GPD1[1] = "00"  Pull-up / down disable, 	GPD1[0] = "00"  Pull-up / down disable;
	 *
	 *  Register Bit [11 ~ 0] = 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPD1PUDPDN);
	sleep_pud &= ~( 1<< 11 | 1 << 10 | 1 << 9| 1 << 8 | 1<< 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0 );
	writel(sleep_pud, S5PV210_GPD1PUDPDN);

}

void sleep_gpio_GPE0_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;

	/* 
	 *  GPE0:
	 *
	 *  GPIO Description:
	 *  
	 *  GPE0[7] : "0M3CAM_D4",  		GPE0[6] : "0M3CAM_D3"; 
	 *  GPE0[5] : "0M3CAM_D2",  		GPE0[4] : "0M3CAM_D1"; 	
	 *  GPE0[3] : "0M3CAM_D0",  		GPE0[2] : "0M3CAM_HSYNC"; 
	 *  GPE0[1] : "0M3CAM_VSYNC",		GPE0[0] : "0M3CAM_PCLK";
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPE0[7] : "10"  Input,  		GPE0[6] : "10"  Input; 
	 *  GPE0[5] = "10"  Input,  		GPE0[4] = "10"  Input;
	 *  GPE0[3] = "10"  Input,  		GPE0[2] = "10"  Input;
	 *  GPE0[1] = "10"  Input,  		GPE0[0] = "10"  Input;
	 *  
	 *  Register Bit [15 ~ 0] = 1010 1010 1010 1010
	 */
	sleep_config = readl(S5PV210_GPE0CONPDN);		
	sleep_config |= (1 << 15 | 1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_config &= ~( 1<< 14 | 1 << 12 | 1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_config, S5PV210_GPE0CONPDN);
	
	/*
	 *  GPE0:
	 *
	 *  GPIO sleep PUD configuration:
	 *
   	 *  GPE0[7] = "00"  Pull-up / down disable, 	GPE0[6] = "00"  Pull-up / down disable;
	 *  GPE0[5] = "00"  Pull-up / down disable, 	GPE0[4] = "00"  Pull-up / down disable;
	 *  GPE0[3] = "00"  Pull-up / down disable, 	GPE0[2] = "00"  Pull-up / down disable;
	 *  GPE0[1] = "00"  Pull-up / down disable, 	GPE0[0] = "00"  Pull-up / down disable;
	 *
	 *  Register Bit [15 ~ 0] = 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPE0PUDPDN);
	sleep_pud &= ~( 1<< 15 | 1 << 14 | 1 << 13 | 1 << 12 );
	sleep_pud &= ~( 1<< 11 | 1 << 10 | 1 << 9 | 1 << 8 | 1<< 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0 );
	writel(sleep_pud, S5PV210_GPE0PUDPDN);

}

void sleep_gpio_GPE1_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;

	/* 
	 *  GPE1:
	 *
	 *  GPIO Description:
	 *  
	 *  GPE1[4] : "CAM_0M3_Reset"; 	
	 *  GPE1[3] : "0M3CAM_MCLK",  		GPE1[2] : "0M3CAM_D7"; 
	 *  GPE1[1] : "0M3CAM_D6",			GPE1[0] : "0M3CAM_D5";
	 *
	 *  GPIO sleep configuration:
  	 *
	 *  GPE1[4] = "01"  Output 1;
	 *  GPE1[3] = "10"  Input,  		GPE1[2] = "10"  Input;
	 *  GPE1[1] = "10"  Input,  		GPE1[0] = "10"  Input;
	 *  
	 *  Register Bit [9 ~ 0] = 01 1010 1010
	 */
	sleep_config = readl(S5PV210_GPE1CONPDN);		
	sleep_config |= (  1 << 8  | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_config &= ~( 1 << 9 | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_config, S5PV210_GPE1CONPDN);
	
	/*
	 *  GPE1:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPE1[4] = "00"  Pull-up / down disable;
	 *  GPE1[3] = "00"  Pull-up / down disable, 	GPE1[2] = "00"  Pull-up / down disable;
	 *  GPE1[1] = "00"  Pull-up / down disable, 	GPE1[0] = "00"  Pull-up / down disable;
	 *
	 *  Register Bit [9 ~ 0] = 00 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPE1PUDPDN);
	sleep_pud &= ~( 1 << 9 | 1 << 8 | 1<< 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0 );
	writel(sleep_pud, S5PV210_GPE1PUDPDN);

}

void sleep_gpio_GPF0_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;

	/* 
	 *  GPF0:
	 *
	 *  GPIO Description:
	 *  
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPF0[7] : "10"  Input,  		GPF0[6] : "10"  Input; 
	 *  GPF0[5] = "10"  Input,  		GPF0[4] = "10"  Input;
	 *  GPF0[3] = "10"  Input,  		GPF0[2] = "10"  Input;
	 *  GPF0[1] = "10"  Input,  		GPF0[0] = "10"  Input;
	 *  
	 *  Register Bit [15 ~ 0] = 1010 1010 1010 1010
	 */
	sleep_config = readl(S5PV210_GPF0CONPDN);		
	sleep_config |= (1 << 15 | 1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_config &= ~( 1<< 14 | 1 << 12 | 1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_config, S5PV210_GPF0CONPDN);
	
	/*
	 *  GPF0:
	 *
	 *  GPIO sleep PUD configuration:
	 *
   	 *  GPF0[7] = "00"  Pull-up / down disable, 	GPF0[6] = "00"  Pull-up / down disable;
	 *  GPF0[5] = "00"  Pull-up / down disable, 	GPF0[4] = "00"  Pull-up / down disable;
	 *  GPF0[3] = "00"  Pull-up / down disable, 	GPF0[2] = "00"  Pull-up / down disable;
	 *  GPF0[1] = "00"  Pull-up / down disable, 	GPF0[0] = "00"  Pull-up / down disable;
	 *
	 *  Register Bit [15 ~ 0] = 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPF0PUDPDN);
	sleep_pud &= ~( 1<< 15 | 1 << 14 | 1 << 13 | 1 << 12 );
	sleep_pud &= ~( 1<< 11 | 1 << 10 | 1 << 9 | 1 << 8 | 1<< 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0 );
	writel(sleep_pud, S5PV210_GPF0PUDPDN);

}

void sleep_gpio_GPF1_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;

	/* 
	 *  GPF1:
	 *
	 *  GPIO Description:
	 *  
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPF1[7] : "10"  Input,  		GPF1[6] : "10"  Input; 
	 *  GPF1[5] = "10"  Input,  		GPF1[4] = "10"  Input;
	 *  GPF1[3] = "10"  Input,  		GPF1[2] = "10"  Input;
	 *  GPF1[1] = "10"  Input,  		GPF1[0] = "10"  Input;
	 *  
	 *  Register Bit [15 ~ 0] = 1010 1010 1010 1010
	 */
	sleep_config = readl(S5PV210_GPF1CONPDN);		
	sleep_config |= (1 << 15 | 1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_config &= ~( 1<< 14 | 1 << 12 | 1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_config, S5PV210_GPF1CONPDN);
	
	/*
	 *  GPF1:
	 *
	 *  GPIO sleep PUD configuration:
	 *
   	 *  GPF1[7] = "00"  Pull-up / down disable, 	GPF1[6] = "00"  Pull-up / down disable;
	 *  GPF1[5] = "00"  Pull-up / down disable, 	GPF1[4] = "00"  Pull-up / down disable;
	 *  GPF1[3] = "00"  Pull-up / down disable, 	GPF1[2] = "00"  Pull-up / down disable;
	 *  GPF1[1] = "00"  Pull-up / down disable, 	GPF1[0] = "00"  Pull-up / down disable;
	 *
	 *  Register Bit [15 ~ 0] = 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPF1PUDPDN);
	sleep_pud &= ~( 1<< 15 | 1 << 14 | 1 << 13 | 1 << 12 );
	sleep_pud &= ~( 1<< 11 | 1 << 10 | 1 << 9 | 1 << 8 | 1<< 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0 );
	writel(sleep_pud, S5PV210_GPF1PUDPDN);

}

void sleep_gpio_GPF2_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;

	/* 
	 *  GPF2:
	 *
	 *  GPIO Description:
	 *  
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPF2[7] : "10"  Input,  		GPF2[6] : "10"  Input; 
	 *  GPF2[5] = "10"  Input,  		GPF2[4] = "10"  Input;
	 *  GPF2[3] = "10"  Input,  		GPF2[2] = "10"  Input;
	 *  GPF2[1] = "10"  Input,  		GPF2[0] = "10"  Input;
	 *  
	 *  Register Bit [15 ~ 0] = 1010 1010 1010 1010
	 */
	sleep_config = readl(S5PV210_GPF2CONPDN);		
	sleep_config |= (1 << 15 | 1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_config &= ~( 1<< 14 | 1 << 12 | 1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_config, S5PV210_GPF2CONPDN);
	
	/*
	 *  GPF1:
	 *
	 *  GPIO sleep PUD configuration:
	 *
   	 *  GPF2[7] = "00"  Pull-up / down disable, 	GPF2[6] = "00"  Pull-up / down disable;
	 *  GPF2[5] = "00"  Pull-up / down disable, 	GPF2[4] = "00"  Pull-up / down disable;
	 *  GPF2[3] = "00"  Pull-up / down disable, 	GPF2[2] = "00"  Pull-up / down disable;
	 *  GPF2[1] = "00"  Pull-up / down disable, 	GPF2[0] = "00"  Pull-up / down disable;
	 *
	 *  Register Bit [15 ~ 0] = 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPF2PUDPDN);
	sleep_pud &= ~( 1<< 15 | 1 << 14 | 1 << 13 | 1 << 12 );
	sleep_pud &= ~( 1<< 11 | 1 << 10 | 1 << 9 | 1 << 8 | 1<< 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0 );
	writel(sleep_pud, S5PV210_GPF2PUDPDN);

}

void sleep_gpio_GPF3_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;

	/* 
	 *  GPF3:
	 *
	 *  GPIO Description:
	 *  
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPF3[5] = "10"  Input,  		GPF3[4] = "10"  Input;
	 *  GPF3[3] = "10"  Input,  		GPF3[2] = "10"  Input;
	 *  GPF3[1] = "10"  Input,  		GPF3[0] = "10"  Input;
	 *  
	 *  Register Bit [11 ~ 0] = 1010 1010 1010
	 */
	sleep_config = readl(S5PV210_GPF3CONPDN);		
	sleep_config |= ( 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_config &= ~(  1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_config, S5PV210_GPF3CONPDN);
	
	/*
	 *  GPF3:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPF3[5] = "00"  Pull-up / down disable, 	GPF3[4] = "00"  Pull-up / down disable;
	 *  GPF3[3] = "00"  Pull-up / down disable, 	GPF3[2] = "00"  Pull-up / down disable;
	 *  GPF3[1] = "00"  Pull-up / down disable, 	GPF3[0] = "00"  Pull-up / down disable;
	 *
	 *  Register Bit [11 ~ 0] =  0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPF3PUDPDN);
	sleep_pud &= ~( 1<< 11 | 1 << 10 | 1 << 9 | 1 << 8 | 1<< 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0 );
	writel(sleep_pud, S5PV210_GPF3PUDPDN);

}

void sleep_gpio_GPG0_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;

	/* 
	 *  GPG0:
	 *
	 *  GPIO Description:
	 *  
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPG0[6] : "10"  Input; 
	 *  GPG0[5] = "10"  Input,  		GPG0[4] = "10"  Input;
	 *  GPG0[3] = "10"  Input,  		GPG0[2] = "10"  Input;
	 *  GPG0[1] = "10"  Input,  		GPG0[0] = "10"  Input;
	 *  
	 *  Register Bit [13 ~ 0] = 10 1010 1010 1010
	 */
	sleep_config = readl(S5PV210_GPG0CONPDN);		
	sleep_config |= (1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_config &= ~(1 << 12 | 1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_config, S5PV210_GPG0CONPDN);
	
	/*
	 *  GPG0:
	 *
	 *  GPIO sleep PUD configuration:
	 *
   	 *  GPG0[6] = "00"  Pull-up / down disable;
	 *  GPG0[5] = "00"  Pull-up / down disable, 	GPG0[4] = "00"  Pull-up / down disable;
	 *  GPG0[3] = "00"  Pull-up / down disable, 	GPG0[2] = "00"  Pull-up / down disable;
	 *  GPG0[1] = "00"  Pull-up / down disable, 	GPG0[0] = "00"  Pull-up / down disable;
	 *
	 *  Register Bit [13 ~ 0] = 00 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPG0PUDPDN);
	sleep_pud &= ~(1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_pud &= ~(1 << 12 | 1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_pud, S5PV210_GPG0PUDPDN);

}

void sleep_gpio_GPG1_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;

	/* 
	 *  GPG1:
	 *
	 *  GPIO Description:
	 *  
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPG1[6] : "10"  Input; 
	 *  GPG1[5] = "10"  Input,  		GPG1[4] = "10"  Input;
	 *  GPG1[3] = "10"  Input,  		GPG1[2] = "10"  Input;
	 *  GPG1[1] = "10"  Input,  		GPG1[0] = "10"  Input;
	 *  
	 *  Register Bit [13 ~ 0] = 10 1010 1010 1010
	 */
	sleep_config = readl(S5PV210_GPG1CONPDN);		
	sleep_config |= (1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_config &= ~(1 << 12 | 1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_config, S5PV210_GPG1CONPDN);
	
	/*
	 *  GPG1:
	 *
	 *  GPIO sleep PUD configuration:
	 *
   	 *  GPG1[6] = "00"  Pull-up / down disable;
	 *  GPG1[5] = "00"  Pull-up / down disable, 	GPG1[4] = "00"  Pull-up / down disable;
	 *  GPG1[3] = "00"  Pull-up / down disable, 	GPG1[2] = "00"  Pull-up / down disable;
	 *  GPG1[1] = "00"  Pull-up / down disable, 	GPG1[0] = "00"  Pull-up / down disable;
	 *
	 *  Register Bit [13 ~ 0] = 00 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPG1PUDPDN);
	sleep_pud &= ~(1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_pud &= ~(1 << 12 | 1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_pud, S5PV210_GPG1PUDPDN);

}

void sleep_gpio_GPG2_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;

	/* 
	 *  GPG2:
	 *
	 *  GPIO Description:
	 *  
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPG2[6] : "10"  Input; 
	 *  GPG2[5] = "10"  Input,  		GPG2[4] = "10"  Input;
	 *  GPG2[3] = "10"  Input,  		GPG2[2] = "10"  Input;
	 *  GPG2[1] = "10"  Input,  		GPG2[0] = "10"  Input;
	 *  
	 *  Register Bit [13 ~ 0] = 10 1010 1010 1010
	 */
	sleep_config = readl(S5PV210_GPG2CONPDN);		
	sleep_config |= (1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_config &= ~(1 << 12 | 1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_config, S5PV210_GPG2CONPDN);
	
	/*
	 *  GPG2:
	 *
	 *  GPIO sleep PUD configuration:
	 *
   	 *  GPG2[6] = "00"  Pull-up / down disable;
	 *  GPG2[5] = "00"  Pull-up / down disable, 	GPG2[4] = "00"  Pull-up / down disable;
	 *  GPG2[3] = "00"  Pull-up / down disable, 	GPG2[2] = "00"  Pull-up / down disable;
	 *  GPG2[1] = "00"  Pull-up / down disable, 	GPG2[0] = "00"  Pull-up / down disable;
	 *
	 *  Register Bit [13 ~ 0] = 00 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPG2PUDPDN);
	sleep_pud &= ~(1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_pud &= ~(1 << 12 | 1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_pud, S5PV210_GPG2PUDPDN);

}

void sleep_gpio_GPG3_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;

	/* 
	 *  GPG3:
	 *
	 *  GPIO Description:
	 *  
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPG3[6] : "10"  Input; 
	 *  GPG3[5] = "10"  Input,  		GPG3[4] = "10"  Input;
	 *  GPG3[3] = "10"  Input,  		GPG3[2] = "10"  Input;
	 *  GPG3[1] = "10"  Input,  		GPG3[0] = "10"  Input;
	 *  
	 *  Register Bit [13 ~ 0] = 10 1010 1010 1010
	 */
	sleep_config = readl(S5PV210_GPG3CONPDN);		
	sleep_config |= (1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_config &= ~(1 << 12 | 1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_config, S5PV210_GPG3CONPDN);
	
	/*
	 *  GPG3:
	 *
	 *  GPIO sleep PUD configuration:
	 *
   	 *  GPG3[6] = "00"  Pull-up / down disable;
	 *  GPG3[5] = "00"  Pull-up / down disable, 	GPG3[4] = "00"  Pull-up / down disable;
	 *  GPG3[3] = "00"  Pull-up / down disable, 	GPG3[2] = "00"  Pull-up / down disable;
	 *  GPG3[1] = "00"  Pull-up / down disable, 	GPG3[0] = "00"  Pull-up / down disable;
	 *
	 *  Register Bit [13 ~ 0] = 00 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPG3PUDPDN);
	sleep_pud &= ~(1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_pud &= ~(1 << 12 | 1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_pud, S5PV210_GPG3PUDPDN);

}

void sleep_gpio_GPI_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;

	/* 
	 *  GPI:
	 *
	 *  GPIO Description:
	 *  
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPI[6] : "10"  Input; 
	 *  GPI[5] = "10"  Input,  		GPI[4] = "10"  Input;
	 *  GPI[3] = "10"  Input,  		GPI[2] = "10"  Input;
	 *  GPI[1] = "10"  Input,  		GPI[0] = "10"  Input;
	 *  
	 *  Register Bit [13 ~ 0] = 10 1010 1010 1010
	 */
	sleep_config = readl(S5PV210_GPICONPDN);		
	sleep_config |= (1 << 13 | 1 << 11 | 1 << 9 | 1 << 7 | 1 << 5 | 1 << 3 | 1 << 1);
	sleep_config &= ~(1 << 12 | 1 << 10 | 1 << 8  | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0);
	writel(sleep_config, S5PV210_GPICONPDN);
	
	/*
	 *  GPI:
	 *
	 *  GPIO sleep PUD configuration:
	 *
   	 *  GPI[6] = "00"  Pull-up / down disable;
	 *  GPI[5] = "00"  Pull-up / down disable, 	GPI[4] = "00"  Pull-up / down disable;
	 *  GPI[3] = "00"  Pull-up / down disable, 	GPI[2] = "00"  Pull-up / down disable;
	 *  GPI[1] = "00"  Pull-up / down disable, 	GPI[0] = "00"  Pull-up / down disable;
	 *
	 *  Register Bit [13 ~ 0] = 00 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPIPUDPDN);
	sleep_pud &= ~( 1 << 13 | 1 << 12 );
	sleep_pud &= ~( 1<< 11 | 1 << 10 | 1 << 9 | 1 << 8 | 1<< 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0 );
	writel(sleep_pud, S5PV210_GPIPUDPDN);

}

void sleep_gpio_GPJ0_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;
	
	/* 
	 *  GPJ0:
	 *
  	 *  GPIO Description:
	 *  GPJ0[7]: "CAM_D7", GPJ0[6]: "CAM_D6", GPJ0[5]: "CAM_D5", GPJ0[4]: "CAM_D4";
	 *  GPJ0[3]: "CAM_D3", GPJ0[2]: "CAM_D2", GPJ0[1]: "CAM_D1", GPJ0[0]: "CAM_D0";
        *
	 *  GPIO sleep configuration:
	 *
	 *  GPJ0[7] = "10" Input, GPJ0[6] = "10" Input, GPJ0[5] = "10" Input, GPJ0[4] = "10" Input;
	 *  GPJ0[3] = "10" Input, GPJ0[2] = "10" Input, GPJ0[1] = "10" Input, GPJ0[0] = "10" Input;
	 *  
	 *  Register Bit [15 ~ 0] = 1010 1010 1010 1010
	 */
	sleep_config = readl(S5PV210_GPJ0CONPDN);
	sleep_config |= (1 << 15 | 1 << 13 | 1 << 11 | 1 << 9 | 1<< 7 | 1 << 5 | 1<< 3 | 1 << 1);
	sleep_config &= ~( 1 << 14 |  1 << 12 | 1 << 10 | 1 << 8 | 1 << 6 |  1 << 4 | 1 << 2 | 1 << 0 );		
	writel(sleep_config, S5PV210_GPJ0CONPDN);
	
	/* 
	 *  GPJ0:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPJ0[7] = "00"  Pull-up / down disable, 	GPJ0[6] = "00"  Pull-up / down disable;
	 *  GPJ0[5] = "00"  Pull-up / down disable, 	GPJ0[4] = "00"  Pull-up / down disable;
	 *  GPJ0[3] = "00"  Pull-up / down disable, GPJ0[2] = "00"  Pull-up / down disable;
	 *  GPJ0[1] = "00"  Pull-up / down disable, GPJ0[0] = "00"  Pull-up / down disable;;
	 *  
	 *  Register Bit [15 ~ 0] = 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPJ0PUDPDN);
	sleep_pud &= ~( 1 << 15 | 1 << 14 | 1 << 13 | 1 << 12 | 1 << 11 | 1 << 10 | 1 << 9 | 1 << 8 );
	sleep_pud &= ~( 1 << 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0);
	writel(sleep_pud, S5PV210_GPJ0PUDPDN);
	
}

void sleep_gpio_GPJ1_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;
	
	/* 
	 *  GPJ1:
	 *
  	 *  GPIO Description:
	 *  GPJ1[5]: "VDD_CAM_EN", GPJ1[4]: "CAM_MCLK";
	 *  GPJ1[3]: "CAM_2M_Reset", GPJ1[2]: "CAP_HSYNC", GPJ1[1]: "CAM_VSYNC", GPJ1[0]: "CAM_PCLK";
        *
	 *  GPIO sleep configuration:
	 *
	 *  GPJ1[5] = "00" Output 0, GPJ1[4] = "10" Input,;
	 *  GPJ1[3] = "01" Output 1, GPJ1[2] = "10" Input, GPJ1[1] = "10" Input, GPJ1[0] = "10" Input;
	 *  
	 *  Register Bit [11 ~ 0] = 0010 0110 1010
	 */
	sleep_config = readl(S5PV210_GPJ1CONPDN);
	sleep_config |= (  1 << 9 | 1 << 6  | 1 << 5 | 1 << 3 | 1 << 1 );
	sleep_config &= ~(  1 << 11 | 1 << 10 | 1 << 8 | 1 << 7 |  1 << 4 | 1 << 2 | 1 << 0 );		
	writel(sleep_config, S5PV210_GPJ1CONPDN);
	
	/* 
	 *  GPJ1:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPJ1[5] = "00"  Pull-up / down disable, 	GPJ1[4] = "00"  Pull-up / down disable;
	 *  GPJ1[3] = "00"  Pull-up / down disable,     GPJ1[2] = "00"  Pull-up / down disable;
	 *  GPJ1[1] = "00"  Pull-up / down disable,     GPJ1[0] = "00"  Pull-up / down disable;;
	 *  
	 *  Register Bit [11 ~ 0] = 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPJ1PUDPDN);
	sleep_pud &= ~( 1 << 11 | 1 << 10 | 1 << 9 | 1 << 8 );
	sleep_pud &= ~( 1 << 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0);
	writel(sleep_pud, S5PV210_GPJ1PUDPDN);
	
}

void sleep_gpio_GPJ2_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;
	
	/* 
	 *  GPJ2:
	 *
  	 *  GPIO Description:
	 *  GPJ2[7]: "Current Limmit", GPJ2[6]: "OTG_EN", GPJ2[5]: "Charge_EN", GPJ2[4]: "LCD_POWER1";
	 *  GPJ2[3]: "None", GPJ2[2]: "VGH_CTRL", GPJ2[1]: "WF_RSTn", GPJ2[0]: "HDMI_EN";
        *
	 *  GPIO sleep configuration:
	 *
	 *  GPJ2[7] = "11" Previous, GPJ2[6] = "11" Previous, GPJ2[5] = "11" Previous, GPJ2[4] = "00" Output 0;
	 *  GPJ2[3] = "10" input, GPJ2[2] = "00" Output 0, GPJ2[1] = "10" Input, GPJ2[0] = "00" Output 0;
	 *  
	 *  Register Bit [15 ~ 0] = 1111 1100 1000 1000
	 */
	sleep_config = readl(S5PV210_GPJ2CONPDN);
	sleep_config |= (1 << 15 | 1 << 14 | 1 << 13 | 1 << 12 | 1 << 11 | 1 << 10 | 1 << 7 | 1 << 3 | 1<<1 );
	sleep_config &= ~( 1 << 9 | 1 << 8 |  1 << 6 | 1<< 5 | 1 << 4 | 1 << 2 /*| 1 << 1*/ | 1 << 0 );		
	writel(sleep_config, S5PV210_GPJ2CONPDN);
	
	/* 
	 *  GPJ2:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPJ2[7] = "00"  Pull-up / down disable, 	GPJ2[6] = "00"  Pull-up / down disable;
	 *  GPJ2[5] = "00"  Pull-up / down disable, 	GPJ2[4] = "00"  Pull-up / down disable;
	 *  GPJ2[3] = "00"  Pull-up / down disable,     GPJ2[2] = "00"  Pull-up / down disable;
	 *  GPJ2[1] = "00"  Pull-up / down disable,     GPJ2[0] = "00"  Pull-up / down disable;;
	 *  
	 *  Register Bit [15 ~ 0] = 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPJ2PUDPDN);
	sleep_pud &= ~( 1 << 15 | 1 << 14 | 1 << 13 | 1 << 12 | 1 << 11 | 1 << 10 | 1 << 9 | 1 << 8 );
	sleep_pud &= ~( 1 << 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0);
	writel(sleep_pud, S5PV210_GPJ2PUDPDN);
	
}

void sleep_gpio_GPJ3_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;
	
	/* 
	 *  GPJ3:
	 *
  	 *  GPIO Description:
	 *  GPJ3[7]: "USB_CONTROL", GPJ3[6]: "None", GPJ3[5]: "BT_RSTn", GPJ3[4]: "Touch_RES";
	 *  GPJ3[3]: "None", GPJ3[2]: "CAM_2M_PWDN", GPJ3[1]: "AVDD_CTRL", GPJ3[0]: "LCD_BL_EN";
         *
	 *  GPIO sleep configuration:
	 *
	 *  GPJ3[7] = "11" Previous, GPJ3[6] = "10" Input, GPJ3[5] = "10" input, GPJ3[4] = "10" input;
	 *  GPJ3[3] = "10" Input, GPJ3[2] = "01" Output 1, GPJ3[1] = "00" Output 0, GPJ3[0] = "00" Output 0;
	 *  
	 *  Register Bit [15 ~ 0] = 1110 1010 1001 0000
	 */
	sleep_config = readl(S5PV210_GPJ3CONPDN);
	sleep_config |= (1 << 15 | 1 << 14 | 1 << 13  | 1 << 11| 1 << 9 | 1 << 7  | 1 << 4);
	sleep_config &= ~( 1 << 12  | 1 << 10 | 1 << 8 | 1 << 6 | 1 << 5 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0 );		
	writel(sleep_config, S5PV210_GPJ3CONPDN);
	
	/* 
	 *  GPJ3:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPJ3[7] = "00"  Pull-up / down disable, 	GPJ3[6] = "00"  Pull-up / down disable;
	 *  GPJ3[5] = "00"  Pull-up / down disable, 	GPJ3[4] = "00"  Pull-up / down disable;
	 *  GPJ3[3] = "00"  Pull-up / down disable,     GPJ3[2] = "00"  Pull-up / down disable;
	 *  GPJ3[1] = "00"  Pull-up / down disable,     GPJ3[0] = "00"  Pull-up / down disable;;
	 *  
	 *  Register Bit [15 ~ 0] = 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPJ3PUDPDN);
	sleep_pud &= ~( 1 << 15 | 1 << 14 | 1 << 13 | 1 << 12 | 1 << 11 | 1 << 10 | 1 << 9 | 1 << 8 );
	sleep_pud &= ~( 1 << 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0);
	writel(sleep_pud, S5PV210_GPJ3PUDPDN);
	
}

void sleep_gpio_GPJ4_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_pud = 0;
	
	/* 
	 *  GPJ4:
	 *
  	 *  GPIO Description:
	 *  GPJ4[4]: "EXT_CARD_EN";
	 *  GPJ4[3]: "CAM_0M3_PWDN", GPJ4[2]: "None", GPJ4[1]: "None", GPJ4[0]: "BAT_DET_EN";
        *
	 *  GPIO sleep configuration:
	 *
	 *  GPJ4[4] = "01" Output 1;
	 *  GPJ4[3] = "01" Output 1, GPJ4[2] = "10" Input 0, GPJ4[1] = "10" Input, GPJ4[0] = "10" Input;
	 *  
	 *  Register Bit [9 ~ 0] = 01 0110 1010
	 */
	sleep_config = readl(S5PV210_GPJ4CONPDN);
	sleep_config |= ( 1 << 8 | 1 << 6 | 1 << 5 | 1 << 3 | 1 << 1 );
	sleep_config &= ~( 1 << 9 |  1 << 7  | 1 << 4 | 1 << 2 | 1 << 0 );		
	writel(sleep_config, S5PV210_GPJ4CONPDN);
	
	/* 
	 *  GPJ4:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPJ4[4] = "00"  Pull-up / down disable;
	 *  GPJ4[3] = "00"  Pull-up / down disable,     GPJ4[2] = "00"  Pull-up / down disable;
	 *  GPJ4[1] = "00"  Pull-up / down disable,     GPJ4[0] = "00"  Pull-up / down disable;;
	 *  
	 *  Register Bit [9 ~ 0] = 00 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPJ3PUDPDN);
	sleep_pud &= ~( 1 << 9 | 1 << 8 );
	sleep_pud &= ~( 1 << 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3 | 1 << 2 | 1 << 1 | 1 << 0);
	writel(sleep_pud, S5PV210_GPJ3PUDPDN);
	
}



unsigned long sleep_gph0_pud = 0;
unsigned long sleep_gph1_pud = 0;
unsigned long sleep_gph2_pud = 0;
unsigned long sleep_gph3_pud = 0;

void sleep_gpio_GPH_PUD_store(void)
{
	sleep_gph0_pud = readl(S5PV210_GPH0PUD);
	sleep_gph1_pud = readl(S5PV210_GPH1PUD);
	sleep_gph2_pud = readl(S5PV210_GPH2PUD);
	sleep_gph3_pud = readl(S5PV210_GPH3PUD);
}

void resume_gpio_GPH_PUD_restore(void)
{
	writel(sleep_gph0_pud, S5PV210_GPH0PUD);
	writel(sleep_gph1_pud, S5PV210_GPH1PUD);
	writel(sleep_gph2_pud, S5PV210_GPH2PUD);
	writel(sleep_gph3_pud, S5PV210_GPH3PUD);
}

void sleep_gpio_GPH_PUD_config(void)
{
	
	unsigned long sleep_pud = 0;
	
	sleep_gpio_GPH_PUD_store();

	/* 
	 *  GPH0:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  "00"  Pull-up / down disable;
	 *  register bit [15 ~ 0]: 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPH0PUD);
	sleep_pud &= ~0xFF;
	writel(sleep_pud, S5PV210_GPH0PUD);
	

	/* 
	 *  GPH1:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  "00"  Pull-up / down disable;
	 *  register bit [15 ~ 0]: 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPH1PUD);
	sleep_pud &= ~0xFF;
	writel(sleep_pud, S5PV210_GPH1PUD);

	/* 
	 *  GPH2:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  "00"  Pull-up / down disable;
	 *  register bit [15 ~ 0]: 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPH2PUD);
	sleep_pud &= ~0xFF;
	writel(sleep_pud, S5PV210_GPH2PUD);

	/* 
	 *  GPH3:
	 *
	 *  GPIO sleep PUD configuration:
	 *
	 *  GPH3 PUD = "00"  Pull-up / down disable;
	 *  register bit [15 ~ 0]: 0000 0000 0000 0000
	 */
	sleep_pud = readl(S5PV210_GPH3PUD);
	sleep_pud &= ~0xFF;
	writel(sleep_pud, S5PV210_GPH3PUD);
	
}

void sleep_gpio_GPH_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_dat = 0;
	/* 
     *  GPH0:
     *  
     *  GPIO Description: 
     * 
     *  GPIO sleep configuration:
     *  
     */
	sleep_config = readl(S5PV210_GPH0CON);    
	sleep_config &= ~( 0xF << 3*4 | 0xF << 2*4 );
	sleep_config |= ( 0x0 << 3*4 | 0x0 << 2*4 );
	writel(sleep_config, S5PV210_GPH0CON);   
	
	/*
     *  GPH2:
     *  
     *  GPIO Description: 
     *  
     *  GPIO sleep configuration: 
     * 
     */
	sleep_config = readl(S5PV210_GPH2CON);    
	sleep_config &= ~( 0xF << 7*4 | 0xF << 6*4 | 0xF << 5*4 | 0xF << 4*4 | 0xF << 2*4);
	sleep_config |= ( 0x0 << 7*4 | 0x0 << 6*4 | 0x0 << 5*4 | 0x0 << 4*4 | 0x0 << 2*4 );
	writel(sleep_config, S5PV210_GPH2CON);    
                                	
	/* 
	 *  GPH3:
	 *
  	 *  GPIO Description:
	 *  GPH3[0]: "WIFI_MAC_WAKE";
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPH3[0] CON = "0001" Output ;
	 *  GPH3[0] DAT	= 0;
	 *  
	 */
	sleep_config = readl(S5PV210_GPH3CON);
	sleep_config &= ~(0xF << 0);
	sleep_config |= 0x1;		
	writel(sleep_config, S5PV210_GPH3CON);


	sleep_dat = readl(S5PV210_GPH3DAT);
	sleep_dat &= ~(0x1 << 0);		
	writel(sleep_dat, S5PV210_GPH3DAT);

	/* GPH PUD config */
	sleep_gpio_GPH_PUD_config();
	
}

void resume_gpio_GPH_config(void)
{
	unsigned long sleep_config = 0;
	unsigned long sleep_dat = 0;
	unsigned long sleep_pud = 0;
	
	/* 
	 *  GPH3:
	 *
  	 *  GPIO Description:
	 *  GPH3[0]: "WIFI_MAC_WAKE";
	 *
	 *  GPIO sleep configuration:
	 *
	 *  GPH3[0] CON = "0001" Output ;
	 *  GPH3[0] DAT	= 0;
	 *  
	 */
	sleep_config = readl(S5PV210_GPH3CON);
	sleep_config &= ~(0xF << 0);
	sleep_config |= 0x1;		
	writel(sleep_config, S5PV210_GPH3CON);


	sleep_dat = readl(S5PV210_GPH3DAT);
	sleep_dat &= ~(0x1 << 0);		
	writel(sleep_dat, S5PV210_GPH3DAT);
	
	/* 
	 * 
	 *
	 *  GPIO sleep GPH PUD restore configuration:
	 *
	 *  
	 */
	resume_gpio_GPH_PUD_restore();
	
}





void sleep_gpio_config(void)
{
	sleep_gpio_GPA0_config();
	sleep_gpio_GPA1_config();
	sleep_gpio_GPB_config();
	sleep_gpio_GPC0_config();
	sleep_gpio_GPC1_config();
	sleep_gpio_GPD0_config();
	sleep_gpio_GPD1_config();
	sleep_gpio_GPE0_config();
	sleep_gpio_GPE1_config();
	sleep_gpio_GPF0_config();
	sleep_gpio_GPF1_config();
	sleep_gpio_GPF2_config();
	sleep_gpio_GPF3_config();
	sleep_gpio_GPG0_config();
	sleep_gpio_GPG1_config();
	sleep_gpio_GPG2_config();
	sleep_gpio_GPG3_config();	
	sleep_gpio_GPI_config();
	sleep_gpio_GPJ0_config();
	sleep_gpio_GPJ1_config();
	sleep_gpio_GPJ2_config();
	sleep_gpio_GPJ3_config();
	sleep_gpio_GPJ4_config();
	sleep_gpio_GPH_config();
}

void resume_gpio_config(void)
{
	
	resume_gpio_GPH_config();
}
#endif
