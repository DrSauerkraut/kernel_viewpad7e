/* linux/drivers/media/video/hi253.h
 *
 *
 * Driver for hi253 (SXGA camera) from Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef __HI253_H__
#define __HI253_H__

#define log_camera
#ifdef log_camera
	#define logg(x, args...)	printk(x, ##args)
#else
	#define logg(x, args...)	NULL
#endif

struct hi253_reg {
	unsigned short addr;
	unsigned short val;
};

struct hi253_regset_type {
	unsigned char *regset;
	int len;
};

/*
 * Macro
 */
#define REGSET_LENGTH(x)	(sizeof(x)/sizeof(hi253_reg))

#define REGSET_END		(0xFFFF)
#define REGSET_MAX		1000

/*
 * Host S/W Register interface (0x70000000-0x70002000)
 */
/* Initialization section */

/*
 * User defined commands
 */
/* S/W defined features for tune */
//#define REG_DELAY	0xFF00	/* in ms */
#define REG_CMD		0xFFFF	/* Followed by command */


/* Following order should not be changed */
enum image_size_hi253 {
	/* This SoC supports upto SXGA (1280*1024) */
	QQVGA,	/* 160*120*/
	QCIF,	/* 176*144 */
	QVGA,	/* 320*240 */
	CIF,	/* 352*288 */
	VGA,	/* 640*480 */
	SVGA,	/* 800*600 */
#if 0
	HD720P,	/* 1280*720 */
	SXGA,	/* 1280*1024 */
#endif
};

/*
 * Following values describe controls of camera
 * in user aspect and must be match with index of hi253_regset[]
 * These values indicates each controls and should be used
 * to control each control
 */
enum hi253_control {
	HI253_INIT,
	HI253_EV,
	HI253_AWB,
	HI253_MWB,
	HI253_EFFECT,
	HI253_CONTRAST,
	HI253_SATURATION,
	HI253_SHARPNESS,
};

#if 1
#define HI253_REGSET(x)	{	\
	.regset = x,			\
	.len = sizeof(x)/sizeof(hi253_reg),}
#endif

#include "hi253_reg.h"


#define HI253_INIT_REGS	(sizeof(hi253_init_reg) / sizeof(hi253_init_reg[0]))

/*
 * Sleep and Wakeup setting
 */
unsigned short hi253_sleep_reg[][2] = {
	{REGSET_END, 0},
};

#define HI253_SLEEP_REGS	\
	(sizeof(hi253_sleep_reg) / sizeof(hi253_sleep_reg[0]))

unsigned short hi253_wakeup_reg[][2] = {
	{REGSET_END, 0},
};

#define HI253_WAKEUP_REGS	\
	(sizeof(hi253_wakeup_reg) / sizeof(hi253_wakeup_reg[0]))

/*
 * V4L2_CID_CAMERA_ANTI_BANDING (V4L2_CID_PRIVATE_BASE + 105)
 */
static const struct hi253_reg hi253_auto_banding[] = {
        {0x03,0x20},  //select page
        {0x10,0xdc},
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_50hz_banding[] = {
        {0x03,0x20},  //select page
        {0x10,0x9c},
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_60hz_banding[] = {
        {0x03,0x20},  //select page
        {0x10,0x8c},
	{REGSET_END, 0},
};
	
static const struct hi253_reg hi253_off_banding[] = {
	{REGSET_END, 0},
};

/* Order of this array should be following the querymenu data */
static const struct hi253_reg *hi253_regs_anti_banding[] = {
        [ANTI_BANDING_AUTO]     = hi253_auto_banding,
        [ANTI_BANDING_50HZ]     = hi253_50hz_banding,
        [ANTI_BANDING_60HZ]     = hi253_60hz_banding,
        [ANTI_BANDING_OFF]      = hi253_off_banding,
};

/*
 * Manual White Balance (presets)
 */
static const struct hi253_reg hi253_wb_auto[] = {
	{0x03, 0x22},
	{0x11, 0x2e},
	{0x83, 0x5e},	
	{0x84, 0x1e},
	{0x85, 0x5e},
	{0x86, 0x22},
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_wb_tungsten[] = {
	{0x03, 0x22},
	{0x11, 0x24},
	{0x80, 0x20},	
	{0x82, 0x58},	
	{0x83, 0x27},	
	{0x84, 0x22},
	{0x85, 0x58},
	{0x86, 0x52},
	{REGSET_END, 0},

};

static const struct hi253_reg hi253_wb_fluorescent[] = {
	{0x03, 0x22},
	{0x11, 0x28},
	{0x80, 0x41},	
	{0x82, 0x42},	
	{0x83, 0x44},	
	{0x84, 0x34},
	{0x85, 0x46},
	{0x86, 0x3a},
        {REGSET_END, 0},
};

static const struct hi253_reg hi253_wb_sunny[] = {
	{0x03, 0x22},
	{0x11, 0x28},
	{0x80, 0x59},	
	{0x82, 0x29},	
	{0x83, 0x60},	
	{0x84, 0x50},
	{0x85, 0x2f},
	{0x86, 0x23},
        {REGSET_END, 0},
};

static const struct hi253_reg hi253_wb_cloudy[] = {
	{0x03, 0x22},
	{0x11, 0x28},
	{0x80, 0x71},	
	{0x82, 0x2b},	
	{0x83, 0x72},	
	{0x84, 0x70},
	{0x85, 0x2b},
	{0x86, 0x28},
	{REGSET_END, 0},
};

/* Order of this array should be following the querymenu data */
static const struct hi253_reg *hi253_regs_wb_preset[] = {
        [WHITE_BALANCE_AUTO]            = hi253_wb_auto,
        [WHITE_BALANCE_SUNNY]           = hi253_wb_sunny,
        [WHITE_BALANCE_CLOUDY]          = hi253_wb_cloudy,
        [WHITE_BALANCE_TUNGSTEN]        = hi253_wb_tungsten,
        [WHITE_BALANCE_FLUORESCENT]     = hi253_wb_fluorescent,
};

/*
 * Color Effect (COLORFX)
 *  */
static const struct hi253_reg hi253_color_normal[] = {
	{0x03,0x10},
	{0x11,0x03},
	{0x12,0x30},
	{0x13,0x02},
	{0x44,0x80},
	{0x45,0x80},
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_color_emboss[] = {
	{0x03,0x10},
	{0x11,0x03},
	{0x12,0x30},
	{0x13,0x02},
	{0x44,0x80},
	{0x45,0x80},
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_color_sepia[] = {
	{0x03,0x10},
	{0x11,0x03},
	{0x12,0x33},
	{0x13,0x02},
	{0x44,0x70},
	{0x45,0x78},
        {REGSET_END, 0},
};

static const struct hi253_reg hi253_color_aqua[] = {
	{0x03,0x10},
	{0x11,0x03},
	{0x12,0x23},
	{0x13,0x00},
	{0x44,0x80},
	{0x45,0x80},
	{0x47,0x7f},
	{0x03,0x13},
	{0x20,0x07},
	{0x21,0x07},
        {REGSET_END, 0},
};

static const struct hi253_reg hi253_color_sketch[] = {
	{0x03,0x10},
	{0x11,0x13},
	{0x12,0x30},
	{0x13,0x02},
	{0x40,0x20},
	{0x44,0x80},
	{0x45,0x80},
	{0x47,0x7f},
	{0x03,0x13},
	{0x20,0x07},
	{0x21,0x07},
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_color_negative[] = {
	{0x03, 0x10},
	{0x11, 0x03},
	{0x12, 0x28},
	{0x13, 0x00},
	{0x40, 0x00},
	{0x44, 0x80},
	{0x45, 0x80},
	{0x47, 0x7f},
	{0x03, 0x13},
	{0x20, 0x07},
	{0x21, 0x07},
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_color_solarize[] = {
	{0x03,0x10},
	{0x11,0x03},
	{0x12,0x30},
	{0x13,0x02},
	{0x44,0x80},
	{0x45,0x80},
	{REGSET_END, 0},
};

/* Order of this array should be following the querymenu data */
static const struct hi253_reg *hi253_regs_color_effect[] = {
        [IMAGE_EFFECT_NONE]             = hi253_color_normal,
        [IMAGE_EFFECT_BNW]              = hi253_color_emboss,
        [IMAGE_EFFECT_SEPIA]            = hi253_color_sepia,
        [IMAGE_EFFECT_AQUA]             = hi253_color_aqua,
	[IMAGE_EFFECT_ANTIQUE]		= hi253_color_sketch,
        [IMAGE_EFFECT_NEGATIVE]         = hi253_color_negative,
	[IMAGE_EFFECT_SHARPEN]		= hi253_color_solarize,
};

/*
 * EV bias
 */
#define FLAG 0x07
static const struct hi253_reg hi253_ev_m4[] = {
	{0x03,0x10},
	{0x12,0x10|FLAG},
	{0x40,0xc0},
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_ev_m3[] = {
	{0x03,0x10},
	{0x12,0x10|FLAG},
	{0x40,0xb0},
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_ev_m2[] = {
	{0x03,0x10},
	{0x12,0x10|FLAG},
	{0x40,0xa0},
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_ev_m1[] = {
	{0x03,0x10},
	{0x12,0x10|FLAG},
	{0x40,0x90},
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_ev_default[] = {
	{0x03,0x10},
	{0x12,0x10|FLAG},
	{0x40,0x80},
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_ev_p1[] = {
	{0x03,0x10},
	{0x12,0x10|FLAG},
	{0x40,0x10},
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_ev_p2[] = {
	{0x03,0x10},
	{0x12,0x10|FLAG},
	{0x40,0x20},
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_ev_p3[] = {
	{0x03,0x10},
	{0x12,0x10|FLAG},
	{0x40,0x30},
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_ev_p4[] = {
	{0x03,0x10},
	{0x12,0x10|FLAG},
	{0x40,0x40},
	{REGSET_END, 0},
};

/* Order of this array should be following the querymenu data */
static const struct hi253_reg *hi253_regs_ev_bias[] = {
	[EV_MINUS_4]		= hi253_ev_m4,		
	[EV_MINUS_3]		= hi253_ev_m3,		
	[EV_MINUS_2]		= hi253_ev_m2,		
	[EV_MINUS_1]		= hi253_ev_m1,		
	[EV_DEFAULT]		= hi253_ev_default,
	[EV_PLUS_1]		= hi253_ev_p1,
	[EV_PLUS_2]		= hi253_ev_p2,
	[EV_PLUS_3]		= hi253_ev_p3,
	[EV_PLUS_4]		= hi253_ev_p4,
};
 
/*
 * Contrast bias
 */
static const struct hi253_reg hi253_contrast_m2[] = {
	{0x03,0x10},
	{0x13,0x02},
	{0x48,0x6c},
	{REGSET_END, 0},
};

#define temp 0xbb
static const struct hi253_reg hi253_contrast_m1[] = {
	{0x03,0x10},
	{0x13,0x02|temp},
	{0x48,0x76},
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_contrast_default[] = {
	{0x03,0x10},
	{0x13,0x02|temp},
	{0x48,0x80},
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_contrast_p1[] = {
	{0x03,0x10},
	{0x13,0x02|temp},
	{0x48,0x8a},
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_contrast_p2[] = {
	{0x03,0x10},
	{0x13,0x02|temp},
	{0x48,0x94},
	{REGSET_END, 0},
};

/* Order of this array should be following the querymenu data */
static const struct hi253_reg *hi253_regs_contrast_bias[] = {
	[CONTRAST_MINUS_2]		= hi253_contrast_m2,		
	[CONTRAST_MINUS_1]		= hi253_contrast_m1,		
	[CONTRAST_DEFAULT]		= hi253_contrast_default,
	[CONTRAST_PLUS_1]		= hi253_contrast_p1,
	[CONTRAST_PLUS_2]		= hi253_contrast_p2,
};

/*
 * Saturation bias
 */
#define CON 0x55
static const struct hi253_reg hi253_saturation_m2[] = {
	{0x03,0x10},
	{0x60,0x02|CON},
	{0x61,0x60},
	{0x62,0x60},
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_saturation_m1[] = {
	{0x03,0x10},
	{0x60,0x02|CON},
	{0x61,0x70},
	{0x62,0x70},
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_saturation_default[] = {
	{0x03,0x10},
	{0x60,0x02|CON},
	{0x61,0x80},
	{0x62,0x80},
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_saturation_p1[] = {
	{0x03,0x10},
	{0x60,0x02|CON},
	{0x61,0x90},
	{0x62,0x90},
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_saturation_p2[] = {
	{0x03,0x10},
	{0x60,0x02|CON},
	{0x61,0xa0},
	{0x62,0xa0},
	{REGSET_END, 0},
};

/* Order of this array should be following the querymenu data */
static const struct hi253_reg *hi253_regs_saturation_bias[] = {
	[SATURATION_MINUS_2]		= hi253_saturation_m2,		
	[SATURATION_MINUS_1]		= hi253_saturation_m1,		
	[SATURATION_DEFAULT]		= hi253_saturation_default,
	[SATURATION_PLUS_1]		= hi253_saturation_p1,
	[SATURATION_PLUS_2]		= hi253_saturation_p2,
};

/*
 * Sharpness bias
 */
static const struct hi253_reg hi253_sharpness_m2[] = {
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_sharpness_m1[] = {
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_sharpness_default[] = {
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_sharpness_p1[] = {
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_sharpness_p2[] = {
	{REGSET_END, 0},
};

/* Order of this array should be following the querymenu data */
static const struct hi253_reg *hi253_regs_sharpness_bias[] = {
	[SHARPNESS_MINUS_2]		= hi253_sharpness_m2,		
	[SHARPNESS_MINUS_1]		= hi253_sharpness_m1,		
	[SHARPNESS_DEFAULT]		= hi253_sharpness_default,
	[SHARPNESS_PLUS_1]		= hi253_sharpness_p1,
	[SHARPNESS_PLUS_2]		= hi253_sharpness_p2,
};

/* preview mode */
static const struct hi253_reg hi253_regs_configure_vga_preview[] = {
        {REGSET_END, 0},
};

static const struct hi253_reg hi253_regs_configure_d1_preview[] = {
        {REGSET_END, 0},
};
static const struct hi253_reg hi253_regs_configure_1280x1024_preview[] = {
	{REGSET_END, 0},
};

/* capture mode */
static const struct hi253_reg hi253_regs_configure_1280x1024_capture[] = {
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_regs_configure_1024x768_capture[] = {
	{REGSET_END, 0},
};

static const struct hi253_reg hi253_regs_configure_vga_capture[] = {
        {REGSET_END, 0},
};

static const struct hi253_reg hi253_regs_configure_1mp_capture[] = {
        {REGSET_END, 0},
};

static const struct hi253_reg hi253_regs_configure_full_capture[] = {
        {REGSET_END, 0},
};


/* Camera mode */
enum
{
        CAMERA_PREVIEW,
        CAMERA_CAPTURE,
        CAMERA_RECORD,
        CAMERA_CAPTURE_JPEG,
};

enum hi253_preview_frame_size {
	HI253_PREVIEW_QCIF = 0,	/* 176x144 */
	HI253_PREVIEW_CIF,		/* 352x288 */
	HI253_PREVIEW_VGA,		/* 640x480 */
	HI253_PREVIEW_D1,		/* 720x480 */
//	HI253_PREVIEW_FULL,		/* 1280x1024 */
};

enum hi253_capture_frame_size {
	HI253_CAPTURE_VGA = 0,		/* 640x480 */
	HI253_CAPTURE_WVGA,		/* 800x480 */
	HI253_CAPTURE_SVGA,		/* 800x600 */
//	HI253_CAPTURE_WSVGA,		/* 1024x600 */
//	HI253_CAPTURE_1MP,		/* 1280x960 */
//	HI253_CAPTURE_FULL,		/* 1280x960 */
};

struct hi253_framesize {
        u32 index;
        u32 width;
        u32 height;
};

static const struct hi253_framesize hi253_preview_framesize_list[] = {
//	{ HI253_PREVIEW_QCIF,        176,  144 },
//	{ HI253_PREVIEW_CIF,         352,  288 },
	{ HI253_PREVIEW_VGA,         640,  480 },
	{ HI253_PREVIEW_D1,          720,  480 },
//	{ HI253_PREVIEW_FULL,        1280, 1024},
};

static const struct hi253_framesize hi253_capture_framesize_list[] = {
	{ HI253_CAPTURE_VGA,         640,  480 },
//	{ HI253_CAPTURE_1MP,         1280, 960 },
//	{ HI253_CAPTURE_FULL,        1280, 1024},
};

static const struct hi253_framesize hi253_record_framesize_list[] = {
	{ HI253_PREVIEW_QCIF,        176,  144 },
	{ HI253_PREVIEW_CIF,         352,  288 },
	{ HI253_PREVIEW_VGA,         640,  480 },
	{ HI253_PREVIEW_D1,          720,  480 },
};

static const struct hi253_reg *hi253_regs_preview[] = {
	[HI253_PREVIEW_VGA]	= hi253_regs_configure_vga_preview,
	[HI253_PREVIEW_D1]	= hi253_regs_configure_d1_preview,
//	[HI253_PREVIEW_FULL]	= hi253_regs_configure_1280x1024_preview,
};

static const struct hi253_reg *hi253_regs_capture[] = {
	[HI253_CAPTURE_VGA]	= hi253_regs_configure_vga_capture,
//	[HI253_CAPTURE_1MP]	= hi253_regs_configure_1mp_capture,
//	[HI253_CAPTURE_FULL]	= hi253_regs_configure_full_capture,
//	[HI253_CAPTURE_FULL]	= hi253_regs_configure_1280x1024_capture,
};

static const struct hi253_reg *hi253_regs_record[] = {

};

#endif

