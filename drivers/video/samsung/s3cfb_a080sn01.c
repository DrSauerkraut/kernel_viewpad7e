/* linux/drivers/video/samsung/s3cfb_lte480wv.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * LTE480 4.8" WVGA Landscape LCD module driver for the SMDK
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/timer.h>
#include <asm/delay.h>
#include <linux/delay.h>
#include <linux/gpio.h>

#include "s3cfb.h"

static struct s3cfb_lcd a080sn01 = {
	.width	= 800,
	.height	= 600,
	.bpp	= 24,
	.freq	= 60,

#if 1
	.timing = {	// precise 800*600 44MHz c3
		.h_fp	= 150,
		.h_bp	= 120,
		.h_sw	= 50,
		.v_fp	= 10,
		.v_fpe	= 1,
		.v_bp	= 10,
		.v_bpe	= 1,
		.v_sw	= 8,
	},
#endif

#if 0
	.timing = {	// precise 1400*800 66MHz c2 
		.h_fp	= 160,
		.h_bp	= 130,
		.h_sw	= 60,
		.v_fp	= 10,
		.v_fpe	= 1,
		.v_bp	= 10,
		.v_bpe	= 1,
		.v_sw	= 10,
	},
#endif

#if 1
	.polarity = {
		.rise_vclk	= 1,
		.inv_hsync	= 1,
		.inv_vsync	= 1,
		.inv_vden	= 0,
	},
#endif

};


/* name should be fixed as 's3cfb_set_lcd_info' */
void s3cfb_set_lcd_info(struct s3cfb_global *ctrl)
{

	a080sn01.init_ldi = NULL;
	ctrl->lcd = &a080sn01;
}


/////////////////////////start of function by tommy////////////////////////
#ifdef CONFIG_PRODUCT_PDC3_FOG

#define A080_CLK	S5PV210_GPB(4)	 
#define A080_DATA	S5PV210_GPB(6)
#define A080_CS 	S5PV210_GPB(5)

int A080_R[6]={0x2db,0x116f,0x2080,0x3008,0x411f,0x61ce};

//int A080_R[6]={0x2ff,0x11ff,0x20ff,0x30ff,0x41ff,0x61ff};

/*=====================================================
function:a080sn01_init_gpio
date:2011-08-25
======================================================*/
static void a080sn01_init_gpio(void)
{
	//gpio_direction_output(A080_CLK, 1);
	//gpio_direction_output(A080_DATA, 1);
	//gpio_direction_output(A080_CS, 1);
	s3c_gpio_cfgpin(A080_CLK, S3C_GPIO_SFN(1));
	s3c_gpio_cfgpin(A080_DATA, S3C_GPIO_SFN(1));
	s3c_gpio_cfgpin(A080_CS, S3C_GPIO_SFN(1));
}


/*=====================================================
function:write_i2c_command(int commant)
date:2011-08-25
======================================================*/
static void a080sn01_write_i2c_command(int command)
{
	//printk("TTTTTTTTTTT %d, %s, %s\n",__LINE__, __FUNCTION__, __FILE__);
	int i;
	int k = 1<< 15;
	gpio_set_value(A080_CS, 0);
	ndelay(200);
	s3c_gpio_cfgpin(A080_DATA,S3C_GPIO_SFN(1));
	for(i=0;i<16;i++)
	{
		gpio_set_value(A080_CLK, 0);
		
		ndelay(200);
		
		if(command & k)
		{
			gpio_set_value(A080_DATA, 1);
			//printk("gpio_set_value write 1 \n");
			//printk("gpio_set_value read %d\n",gpio_get_value(A080_DATA));
		}
		else
		{
			gpio_set_value(A080_DATA, 0);
			//printk("gpio_set_value write 0 \n");
			//printk("gpio_set_value read %d\n",gpio_get_value(A080_DATA));
		}
		k = k >> 1;
		
		gpio_set_value(A080_CLK, 1);
		ndelay(200);
	}
	gpio_set_value(A080_CS, 1);
	ndelay(200);
}

/*=====================================================
function:read_i2c_command(int commant)
date:2011-08-25
======================================================*/
static void a080sn01_read_i2c_data(int command)
{
	//printk("TTTTTTTTTTT %d, %s, %s\n",__LINE__, __FUNCTION__, __FILE__);
	int i;
	int read_data;
	int k = 1<< 15;
	command = command | (1<< 11);
	gpio_set_value(A080_CS, 0);
	ndelay(200);
	for(i=0;i<5;i++)
	{
		s3c_gpio_cfgpin(A080_DATA,S3C_GPIO_SFN(1));
		gpio_set_value(A080_CLK, 0);
		ndelay(200);
		
		if(command & k)
		{
			gpio_set_value(A080_DATA, 1);
			//printk("gpio_set_value write 1 \n");
			//printk("gpio_set_value read %d\n",gpio_get_value(A080_DATA));
		}
		else
		{
			gpio_set_value(A080_DATA, 0);
			//printk("gpio_set_value write 0 \n");
			//printk("gpio_set_value read %d\n",gpio_get_value(A080_DATA));
		}
		k = k >> 1;
		
		gpio_set_value(A080_CLK, 1);
		ndelay(200);
	}

	s3c_gpio_cfgpin(A080_DATA,S3C_GPIO_SFN(0));
	read_data = 0;

	for(i=0;i<11;i++)
	{
		gpio_set_value(A080_CLK, 0);
		ndelay(200);
		
		read_data |= (gpio_get_value(A080_DATA) & 0x001);
		read_data  = read_data << 1;
		
		//printk("%d",gpio_get_value(A080_DATA));	
		gpio_set_value(A080_CLK, 1);
		ndelay(200);
	}
	//printk("\n");
	gpio_set_value(A080_CS, 1);
	udelay(200);
	read_data = read_data >>1;
	printk("TTTTTTTTTTT  %x ,read data = %x\n",read_data,(command & 0xf800) | (read_data & 0x7ff));
}

/*=====================================================
function:init_a080sn01_date
date:2011-08-25
======================================================*/
void init_a080sn01_date(void)
{
//while(1)
//{
//	printk("TTTTTTTTTTT %d, %s, %s\n",__LINE__, __FUNCTION__, __FILE__);

	a080sn01_init_gpio();
	int i;
#if 0
	printk("a080sn01: read r0 ~ r5 first!\n");
	for(i=0;i<6;i++)
	{
		a080sn01_read_i2c_data(A080_R[i]);
	}
#endif

	printk("a080sn01_write_i2c_command:");
	for(i=0;i<6;i++)
	{	
		printk("%x ",A080_R[i]);
		a080sn01_write_i2c_command(A080_R[i]);
	}
	printk("\n");
	
#if 0
	printk("a080sn01: read r0 ~ r5 again!\n");
	for(i=0;i<6;i++)
	{
		a080sn01_read_i2c_data(A080_R[i]);
	}
#endif
//}
	
}
#endif
	
//////////////////////////end of function////////////////////////////////



