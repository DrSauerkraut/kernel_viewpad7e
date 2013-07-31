/* linux/include/media/hi253_platform.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * 		http://www.samsung.com/
 *
 * Driver for hi253 (SXGA camera) from Samsung Electronics
 * 1/5" 2Mp CMOS Image Sensor SoC with an Embedded Image Processor
 * supporting MIPI CSI-2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

struct hi253_platform_data {
	unsigned int default_width;
	unsigned int default_height;
	unsigned int pixelformat;
	int freq;	/* MCLK in KHz */

	/* This SoC supports Parallel & CSI-2 */
	int is_mipi;
};

