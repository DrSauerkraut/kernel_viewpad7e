/* linux/drivers/media/video/gc0309.h
 *
 *
 * Driver for gc0309 (SXGA camera) from Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef __GC0309_H__
#define __GC0309_H__

#define log_camera
#ifdef log_camera
	#define logg(x, args...)	printk(x, ##args)
#else
	#define logg(x, args...)	NULL
#endif

struct gc0309_reg {
	unsigned short addr;
	unsigned short val;
};

struct gc0309_regset_type {
	unsigned char *regset;
	int len;
};

/*
 * Macro
 */
#define REGSET_LENGTH(x)	(sizeof(x)/sizeof(gc0309_reg))

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
#define REG_DELAY	0xFF00	/* in ms */
#define REG_CMD		0xFFFF	/* Followed by command */


/* Following order should not be changed */
/* This RC0408 SoC supports up to VGA */
enum image_size_gc0309 {
	VGA,	/* 640*480 */
};

/*
 * Following values describe controls of camera
 * in user aspect and must be match with index of gc0309_regset[]
 * These values indicates each controls and should be used
 * to control each control
 */
enum gc0309_control {
	GC0309_INIT,
	GC0309_EV,
	GC0309_AWB,
	GC0309_MWB,
	GC0309_EFFECT,
	GC0309_CONTRAST,
	GC0309_SATURATION,
	GC0309_SHARPNESS,
};

#define GC0309_REGSET(x)	{	\
	.regset = x,			\
	.len = sizeof(x)/sizeof(gc0309_reg),}

//#include "gc0309_reg.h"
#include "gc0309_new_regs.h"


#define GC0309_INIT_REGS	(sizeof(gc0309_init_reg) / sizeof(gc0309_init_reg[0]))

/*
 * Sleep and Wakeup setting
 */
unsigned short gc0309_sleep_reg[][2] = {
	{0x1a,0x27},
	{0x25,0x00},
	{REGSET_END, 0},
};

#define GC0309_SLEEP_REGS	\
	(sizeof(gc0309_sleep_reg) / sizeof(gc0309_sleep_reg[0]))

unsigned short gc0309_wakeup_reg[][2] = {
	{0x1a,0x26},
	{0x25,0xff},
	{REGSET_END, 0},
};

#define GC0309_WAKEUP_REGS	\
	(sizeof(gc0309_wakeup_reg) / sizeof(gc0309_wakeup_reg[0]))

/*
 * V4L2_CID_CAMERA_ANTI_BANDING (V4L2_CID_PRIVATE_BASE + 105) 
 */

// below is HB\VB setting MCLK@24MHz; 50Hz Banding
static const struct gc0309_reg gc0309_50hz_banding[] = {
	{0xfe,0x00},  //select page
	{0x01,0x2c},
	{0x02,0xd8},
	{0x0f,0x12},
	
	{0xe2,0x00},
	{0xe3,0x60},
	{0xe4,0x03},
	{0xe5,0x00},
	{0xe6,0x04},
	{0xe7,0x20},
	{0xe8,0x05},
	{0xe9,0xa0},
	//{0xea,0x09},
	//{0xeb,0xc4},
	{REGSET_END, 0},
};

// below is HB\VB setting MCLK@24MHz; 60Hz Banding
static const struct gc0309_reg gc0309_60hz_banding[] = {
	{0xfe,0x00},  //select page
	{0x01,0x32},
	{0x02,0x00},
	{0x0f,0x21},
	
	{0xe2,0x00},
	{0xe3,0x64},
	{0xe4,0x03},
	{0xe5,0x20},
	{0xe6,0x04},
	{0xe7,0x4c},
	{0xe8,0x05},
	{0xe9,0xdc},
	//{0xea,0x09},
	//{0xeb,0xc4},
	{REGSET_END, 0},
};

static const struct gc0309_reg gc0309_auto_banding[] = {
	{REGSET_END, 0},
};
	
static const struct gc0309_reg gc0309_off_banding[] = {
	{REGSET_END, 0},
};

/* Order of this array should be following the querymenu data */
static const struct gc0309_reg *gc0309_regs_anti_banding[] = {
        [ANTI_BANDING_AUTO]	= gc0309_auto_banding,
        [ANTI_BANDING_50HZ]	= gc0309_50hz_banding,
        [ANTI_BANDING_60HZ]	= gc0309_60hz_banding,
        [ANTI_BANDING_OFF]	= gc0309_off_banding,
};


/*
 *  V4L2_CID_CAMERA_WHITE_BALANCE (V4L2_CID_PRIVATE_BASE + 73)
 */

#define TEMP_REG 0x57

// Auto white balance 
static const struct gc0309_reg gc0309_wb_auto[] = {
	{0xfe,0x00}, //select Page
	{0x5a,0x56}, //for AWB can adjust back
	{0x5b,0x40},
	{0x5c,0x4a},
	{0x22,TEMP_REG|0x02},     // Enable AWB
	{REGSET_END, 0},
};

// Incandescent 
static const struct gc0309_reg gc0309_wb_tungsten[] = {
	{0xfe,0x00}, //select Page
	{0x22,TEMP_REG&0xfd},
	{0x5a,0x48},
	{0x5b,0x40},
	{0x5c,0x5c},
	{REGSET_END, 0},
};

// ri guang deng
static const struct gc0309_reg gc0309_wb_fluorescent[] = {
	{0xfe,0x00}, //select Page
	{0x22,TEMP_REG&0xfd},
	{0x5a,0x40},
	{0x5b,0x42},
	{0x5c,0x50},
        {REGSET_END, 0},
};

// Sunny | Daylight
static const struct gc0309_reg gc0309_wb_sunny[] = {
	{0xfe,0x00}, //select Page
	{0x22,TEMP_REG&0xfd},
	{0x5a,0x74},
	{0x5b,0x52},
	{0x5c,0x40},
        {REGSET_END, 0},
};

// Cloudy
static const struct gc0309_reg gc0309_wb_cloudy[] = {
	{0xfe,0x00}, //select Page
	{0x22,TEMP_REG&0xfd},
	{0x5a,0x8c},
	{0x5b,0x50},
	{0x5c,0x40},
	{REGSET_END, 0},
};

/* Order of this array should be following the querymenu data */
static const struct gc0309_reg *gc0309_regs_wb_preset[] = {
        [WHITE_BALANCE_AUTO]            = gc0309_wb_auto,
        [WHITE_BALANCE_SUNNY]           = gc0309_wb_sunny,
        [WHITE_BALANCE_CLOUDY]          = gc0309_wb_cloudy,
        [WHITE_BALANCE_TUNGSTEN]        = gc0309_wb_tungsten,
        [WHITE_BALANCE_FLUORESCENT]     = gc0309_wb_fluorescent,
};

/*
 * V4L2_CID_CAMERA_EFFECT (V4L2_CID_PRIVATE_BASE + 74)
 */

static const struct gc0309_reg gc0309_color_normal[] = {
	{0xfe,0x00}, //select Page
	{0x23,0x00},
	{0xba,0x00},
	{0xbb,0x00},
        {REGSET_END, 0},
};

static const struct gc0309_reg gc0309_color_sepia[] = {
	{0xfe,0x00},
	{0x23,0x02},
	{0xba,0x00},
	{0xbb,0x11},
        {REGSET_END, 0},
};

static const struct gc0309_reg gc0309_color_aqua[] = {
	{0xfe,0x00},
	{0x23,0x02},
	{0xba,0x11},
	{0xbb,0x00},
        {REGSET_END, 0},
};

static const struct gc0309_reg gc0309_color_monochrome[] = {
	{0xfe,0x00},
	{0x23,0x02},
	{0xba,0x00},
	{0xbb,0x00},
	{REGSET_END, 0},
};

static const struct gc0309_reg gc0309_color_negative[] = {
	{0xfe,0x00},
	{0x23,0x01},
	{REGSET_END, 0},
};

/* Order of this array should be following the querymenu data */
static const struct gc0309_reg *gc0309_regs_color_effect[] = {
	[IMAGE_EFFECT_NONE]		= gc0309_color_normal,
	[IMAGE_EFFECT_BNW]		= gc0309_color_monochrome,
	[IMAGE_EFFECT_NEGATIVE]		= gc0309_color_negative,
	[IMAGE_EFFECT_SEPIA]		= gc0309_color_sepia,
	[IMAGE_EFFECT_AQUA]		= gc0309_color_aqua,
};

/*
 * EV bias (Exposure & Brightness)
 * V4L2_CID_CAMERA_BRIGHTNESS (V4L2_CID_PRIVATE_BASE + 72)
 */

static const struct gc0309_reg gc0309_ev_m4[] = {
	{0xfe,0x00},	//select page
	{0xd3,0x70},
	{REGSET_END, 0},
};

static const struct gc0309_reg gc0309_ev_m3[] = {
	{0xfe,0x00},	//select page
	{0xd3,0x75},
	{REGSET_END, 0},
};

static const struct gc0309_reg gc0309_ev_m2[] = {
	{0xfe,0x00},	//select page
	{0xd3,0x7a},
	{REGSET_END, 0},
};

static const struct gc0309_reg gc0309_ev_m1[] = {
	{0xfe,0x00},	//select page
	{0xd3,0x80},
	{REGSET_END, 0},
};

static const struct gc0309_reg gc0309_ev_default[] = {
	{0xfe,0x00},	//select page
	{0xd3,0x88},
	{REGSET_END, 0},
};

static const struct gc0309_reg gc0309_ev_p1[] = {
	{0xfe,0x00},	//select page
	{0xd3,0x90},
	{REGSET_END, 0},
};

static const struct gc0309_reg gc0309_ev_p2[] = {
	{0xfe,0x00},	//select page
	{0xd3,0x98},
	{REGSET_END, 0},
};

static const struct gc0309_reg gc0309_ev_p3[] = {
	{0xfe,0x00},	//select page
	{0xd3,0xa0},
	{REGSET_END, 0},
};

static const struct gc0309_reg gc0309_ev_p4[] = {
	{0xfe,0x00},	//select page
	{0xd3,0xa8},
	{REGSET_END, 0},
};

/* Order of this array should be following the querymenu data */
static const struct gc0309_reg *gc0309_regs_ev_bias[] = {
	[EV_MINUS_4]		= gc0309_ev_m4,		
	[EV_MINUS_3]		= gc0309_ev_m3,		
	[EV_MINUS_2]		= gc0309_ev_m2,		
	[EV_MINUS_1]		= gc0309_ev_m1,		
	[EV_DEFAULT]		= gc0309_ev_default,
	[EV_PLUS_1]		= gc0309_ev_p1,
	[EV_PLUS_2]		= gc0309_ev_p2,
	[EV_PLUS_3]		= gc0309_ev_p3,
	[EV_PLUS_4]		= gc0309_ev_p4,
};
 
/*
 * Contrast bias
 * V4L2_CID_CAMERA_CONTRAST     (V4L2_CID_PRIVATE_BASE + 77)
 *
 */
static const struct gc0309_reg gc0309_contrast_m2[] = {
	{0xfe,0x00},	//select page
	{0xb3,0x30},
	{0xb4,0x80},
	{REGSET_END, 0},
};

static const struct gc0309_reg gc0309_contrast_m1[] = {
	{0xfe,0x00},	//select page
	{0xb3,0x38},
	{0xb4,0x80},
	{REGSET_END, 0},
};

static const struct gc0309_reg gc0309_contrast_default[] = {
	{0xfe,0x00},	//select page
	{0xb3,0x40},
	{0xb4,0x80},
	{REGSET_END, 0},
};

static const struct gc0309_reg gc0309_contrast_p1[] = {
	{0xfe,0x00},	//select page
	{0xb3,0x48},
	{0xb4,0x80},
	{REGSET_END, 0},
};

static const struct gc0309_reg gc0309_contrast_p2[] = {
	{0xfe,0x00},	//select page
	{0xb3,0x50},
	{0xb4,0x80},
	{REGSET_END, 0},
};

/* Order of this array should be following the querymenu data */
static const struct gc0309_reg *gc0309_regs_contrast_bias[] = {
	[CONTRAST_MINUS_2]		= gc0309_contrast_m2,		
	[CONTRAST_MINUS_1]		= gc0309_contrast_m1,		
	[CONTRAST_DEFAULT]		= gc0309_contrast_default,
	[CONTRAST_PLUS_1]		= gc0309_contrast_p1,
	[CONTRAST_PLUS_2]		= gc0309_contrast_p2,
};

/*
 * Saturation bias
 * V4L2_CID_CAMERA_SATURATION     (V4L2_CID_PRIVATE_BASE + 78)
 *
 */
static const struct gc0309_reg gc0309_saturation_m2[] = {
	{0xfe,0x00},	//select page
	{0xb1,0x30},
	{0xb2,0x30},
	{REGSET_END, 0},
};

static const struct gc0309_reg gc0309_saturation_m1[] = {
	{0xfe,0x00},	//select page
	{0xb1,0x38},
	{0xb2,0x38},
	{REGSET_END, 0},
};

static const struct gc0309_reg gc0309_saturation_default[] = {
	{0xfe,0x00},	//select page
	{0xb1,0x4b},
	{0xb2,0x4b},
	{REGSET_END, 0},
};

static const struct gc0309_reg gc0309_saturation_p1[] = {
	{0xfe,0x00},	//select page
	{0xb1,0x4f},
	{0xb2,0x4f},
	{REGSET_END, 0},
};

static const struct gc0309_reg gc0309_saturation_p2[] = {
	{0xfe,0x00},	//select page
	{0xb1,0x54},
	{0xb2,0x54},
	{REGSET_END, 0},
};

/* Order of this array should be following the querymenu data */
static const struct gc0309_reg *gc0309_regs_saturation_bias[] = {
	[SATURATION_MINUS_2]		= gc0309_saturation_m2,		
	[SATURATION_MINUS_1]		= gc0309_saturation_m1,		
	[SATURATION_DEFAULT]		= gc0309_saturation_default,
	[SATURATION_PLUS_1]		= gc0309_saturation_p1,
	[SATURATION_PLUS_2]		= gc0309_saturation_p2,
};

/*
 * Sharpness bias
 * V4L2_CID_CAMERA_SHARPNESS   (V4L2_CID_PRIVATE_BASE + 79)
 */
static const struct gc0309_reg gc0309_sharpness_m2[] = {
	{0xfe,0x00},	//select page
	{0x77,0x33},
	{REGSET_END, 0},
};

static const struct gc0309_reg gc0309_sharpness_m1[] = {
	{0xfe,0x00},	//select page
	{0x77,0x44},
	{REGSET_END, 0},
};

static const struct gc0309_reg gc0309_sharpness_default[] = {
	{0xfe,0x00},	//select page
	{0x77,0x55},
	{REGSET_END, 0},
};

static const struct gc0309_reg gc0309_sharpness_p1[] = {
	{0xfe,0x00},	//select page
	{0x77,0x66},
	{REGSET_END, 0},
};

static const struct gc0309_reg gc0309_sharpness_p2[] = {
	{0xfe,0x00},	//select page
	{0x77,0x77},
	{REGSET_END, 0},
};

/* Order of this array should be following the querymenu data */
static const struct gc0309_reg *gc0309_regs_sharpness_bias[] = {
	[SHARPNESS_MINUS_2]		= gc0309_sharpness_m2,		
	[SHARPNESS_MINUS_1]		= gc0309_sharpness_m1,		
	[SHARPNESS_DEFAULT]		= gc0309_sharpness_default,
	[SHARPNESS_PLUS_1]		= gc0309_sharpness_p1,
	[SHARPNESS_PLUS_2]		= gc0309_sharpness_p2,
};

/*
 * V4L2_CID_CAMERA_ISO                (V4L2_CID_PRIVATE_BASE + 75)
 */
static const struct gc0309_reg gc0309_iso_auto[] = {
        {REGSET_END, 0},
};

static const struct gc0309_reg gc0309_iso_50[] = {
        {REGSET_END, 0},
};

static const struct gc0309_reg gc0309_iso_100[] = {
        {REGSET_END, 0},
};

static const struct gc0309_reg gc0309_iso_200[] = {
        {REGSET_END, 0},
};

static const struct gc0309_reg gc0309_iso_400[] = {
        {REGSET_END, 0},
};

static const struct gc0309_reg gc0309_iso_800[] = {
        {REGSET_END, 0},
};

/* Order of this array should be following the querymenu data */
static const struct gc0309_reg *gc0309_regs_iso[] = {
	[ISO_AUTO]	= gc0309_iso_auto,
	[ISO_50]	= gc0309_iso_50,
	[ISO_100]	= gc0309_iso_100,
	[ISO_200]	= gc0309_iso_200,
	[ISO_400]	= gc0309_iso_400,
	[ISO_800]	= gc0309_iso_800,
};

/* ********  preview mode  ****** */
static const struct gc0309_reg gc0309_regs_configure_vga_preview[] = {
	{0x05, 0x00},
	{0x06, 0x00},
	{0x07, 0x00},
	{0x08, 0x00},
	{0x09, 0x01},
	{0x0a, 0xe8},
	{0x0b, 0x02},
	{0x0c, 0x88},
        {REGSET_END, 0},
};

/* ******** capture mode ******* */
static const struct gc0309_reg gc0309_regs_configure_vga_capture[] = {
	{REGSET_END, 0},
};

/* ******* record mode ****** */
static const struct gc0309_reg gc0309_regs_configure_vga_record[] = {
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

enum gc0309_preview_frame_size {
	GC0309_PREVIEW_VGA,		/* 640x480 */
};

enum gc0309_capture_frame_size {
	GC0309_CAPTURE_VGA = 0,		/* 640x480 */
};

enum gc0309_record_frame_size {
	GC0309_RECORD_VGA = 0,		/* 640x480 */
};

struct gc0309_framesize {
        u32 index;
        u32 width;
        u32 height;
};

static const struct gc0309_framesize gc0309_preview_framesize_list[] = {
	{ GC0309_PREVIEW_VGA,         640,  480 },
};

static const struct gc0309_framesize gc0309_capture_framesize_list[] = {
	{ GC0309_CAPTURE_VGA,         640,  480 },
};

static const struct gc0309_framesize gc0309_record_framesize_list[] = {
	{ GC0309_RECORD_VGA,         640,  480 },
};

static const struct gc0309_reg *gc0309_regs_preview[] = {
	[GC0309_PREVIEW_VGA]	= gc0309_regs_configure_vga_preview,
};

static const struct gc0309_reg *gc0309_regs_capture[] = {
	[GC0309_CAPTURE_VGA]	= gc0309_regs_configure_vga_capture,
};

static const struct gc0309_reg *gc0309_regs_record[] = {
	[GC0309_RECORD_VGA]	= gc0309_regs_configure_vga_record,
};

#endif

