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

#include "s3cfb.h"

static struct s3cfb_lcd a080sn03 = {
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

	a080sn03.init_ldi = NULL;
	ctrl->lcd = &a080sn03;
}



