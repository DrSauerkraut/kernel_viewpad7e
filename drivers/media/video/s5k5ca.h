/* linux/drivers/media/video/s5k5ca.h
 *
 *
 * Driver for s5k5ca (SXGA camera) from Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef __S5K5CA_H__
#define __S5K5CA_H__

#define log_camera
#ifdef log_camera
	#define logg(x, args...)	printk(x, ##args)
#else
	#define logg(x, args...)	NULL
#endif

struct s5k5ca_reg {
	unsigned short addr;
	unsigned short val;
};

struct s5k5ca_regset_type {
	unsigned char *regset;
	int len;
};

/*
 * Macro
 */
#define REGSET_LENGTH(x)	(sizeof(x)/sizeof(s5k5ca_reg))

#define REGSET_END		(0xFFFF)
#define REGSET_MAX		10000

/*
 * Host S/W Register interface (0x70000000-0x70002000)
 */
/* Initialization section */

/*
 * User defined commands
 */
/* S/W defined features for tune */
#define REG_DELAY	0xFFFF	/* in ms */
#define REG_CMD		0xFFFF	/* Followed by command */

/* Following order should not be changed */
enum image_size_s5k5ca {
	/* This SoC supports upto QXGA (2048*1536) */
	VGA,	/* 640*480 */
	SVGA,	/* 800*600 */
	XGA,	/* 1024*768 */
	UXGA,	/* 1600*1200 */	
	QXGA,	/* 2048*1536 */
};

/*
  preview resolution list
            { 2592, 1936},
            { 2560, 1920},
            { 2048, 1536},
            { 1600, 1200},
            { 1280,  720},
            { 1024,  768},
            {  800,  600},
            {  720,  480},
            {  640,  480},
            {  480,  320},
            {  352,  288},
            {  320,  240},
            {  176,  144}
*/

/*
 * Following values describe controls of camera
 * in user aspect and must be match with index of s5k5ca_regset[]
 * These values indicates each controls and should be used
 * to control each control
 */
enum s5k5ca_control {
	S5K5CA_INIT,
	S5K5CA_EV,
	S5K5CA_AWB,
	S5K5CA_MWB,
	S5K5CA_EFFECT,
	S5K5CA_CONTRAST,
	S5K5CA_SATURATION,
	S5K5CA_SHARPNESS,
};

#define S5K5CA_REGSET(x)	{	\
	.regset = x,			\
	.len = sizeof(x)/sizeof(s5k5ca_reg),}

#include "s5k5ca_regs.h"

#define S5K5CA_INIT_REGS	(sizeof(s5k5ca_init_reg) / sizeof(s5k5ca_init_reg[0]))

#if 0
/*
 * Standby and Wakeup setting
 */
unsigned short s5k5ca_standby_reg[][2] = {
	{0xFCFC, 0xD000},
	{0x107E, 0x0001},
	{REGSET_END, 0},
};

#define S5K5CA_STANDBY_REGS	\
	(sizeof(s5k5ca_standby_reg) / sizeof(s5k5ca_standby_reg[0]))

unsigned short s5k5ca_active_reg[][2] = {
	{0xFCFC, 0xD000},
	{0x107E, 0x0000},
	{REGSET_END, 0},
};

#define S5K5CA_ACTIVE_REGS	\
	(sizeof(s5k5ca_active_reg) / sizeof(s5k5ca_active_reg[0]))

#endif

/*
 * Auto focus #define V4L2_CID_CAMERA_SET_AUTO_FOCUS         (V4L2_CID_PRIVATE_BASE + 93)
 */
static const struct s5k5ca_reg s5k5ca_af_off[] = {
	{REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_af_on[] = {
	{REGSET_END, 0},
};

static const struct s5k5ca_reg *s5k5ca_regs_auto_focus[] = {
	[AUTO_FOCUS_OFF]        = s5k5ca_af_off,
	[AUTO_FOCUS_ON]         = s5k5ca_af_on,
};

/*
 * V4L2_CID_CAMERA_ANTI_BANDING (V4L2_CID_PRIVATE_BASE + 105) 
 */
//Auto
static const struct s5k5ca_reg s5k5ca_auto_banding[] = {
	{REGSET_END, 0},
};

// below is HB\VB setting MCLK@24MHz; 50Hz Banding
static const struct s5k5ca_reg s5k5ca_50hz_banding[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x04BA},
	{0x0F12, 0x0001},
	{0x0F12, 0x0001},
	{REGSET_END, 0},
};

// below is HB\VB setting MCLK@24MHz; 60Hz Banding
static const struct s5k5ca_reg s5k5ca_60hz_banding[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x04BA},
	{0x0F12, 0x0002},
	{0x0F12, 0x0001},
	{REGSET_END, 0},
};

//Off
static const struct s5k5ca_reg s5k5ca_off_banding[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x04BA},
	{0x0F12, 0x0000},
	{0x0F12, 0x0001},
	{REGSET_END, 0},
};

/* Order of this array should be following the querymenu data */
static const struct s5k5ca_reg *s5k5ca_regs_anti_banding[] = {
	[ANTI_BANDING_AUTO]		= s5k5ca_auto_banding,
	[ANTI_BANDING_50HZ]		= s5k5ca_50hz_banding,
	[ANTI_BANDING_60HZ]		= s5k5ca_60hz_banding,
	[ANTI_BANDING_OFF]		= s5k5ca_off_banding,
};


/*
 *  V4L2_CID_CAMERA_WHITE_BALANCE (V4L2_CID_PRIVATE_BASE + 73)
 */

// Auto white balance 
static const struct s5k5ca_reg s5k5ca_wb_auto[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x246E},
	{0x0F12, 0x0001},
	{REGSET_END, 0},
};

// Incandescent 
static const struct s5k5ca_reg s5k5ca_wb_tungsten[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x246E},
	{0x0F12, 0x0000},
	{0x002A, 0x04A0},
	{0x0F12, 0x0400},
	{0x0F12, 0x0001},
	{0x0F12, 0x0400},
	{0x0F12, 0x0001},
	{0x0F12, 0x0940},
	{0x0F12, 0x0001},
	{REGSET_END, 0},
};

// ri guang deng
static const struct s5k5ca_reg s5k5ca_wb_fluorescent[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x246E},
	{0x0F12, 0x0000},
	{0x002A, 0x04A0},
	{0x0F12, 0x0575},
	{0x0F12, 0x0001},
	{0x0F12, 0x0400},
	{0x0F12, 0x0001},
	{0x0F12, 0x0800},
	{0x0F12, 0x0001},
	{REGSET_END, 0},
};

// Sunny | Daylight
static const struct s5k5ca_reg s5k5ca_wb_sunny[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x246E},
	{0x0F12, 0x0000},
	{0x002A, 0x04A0},
	{0x0F12, 0x05E0},
	{0x0F12, 0x0001},
	{0x0F12, 0x0400},
	{0x0F12, 0x0001},
	{0x0F12, 0x0530},
	{0x0F12, 0x0001},
	{REGSET_END, 0},
};

// Cloudy
static const struct s5k5ca_reg s5k5ca_wb_cloudy[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x246E},
	{0x0F12, 0x0000},
	{0x002A, 0x04A0},
	{0x0F12, 0x0740},
	{0x0F12, 0x0001},
	{0x0F12, 0x0400},
	{0x0F12, 0x0001},
	{0x0F12, 0x0460},
	{0x0F12, 0x0001},
	{REGSET_END, 0},
};

/* Order of this array should be following the querymenu data */
static const struct s5k5ca_reg *s5k5ca_regs_wb_preset[] = {
	[WHITE_BALANCE_AUTO]            = s5k5ca_wb_auto,
	[WHITE_BALANCE_SUNNY]           = s5k5ca_wb_sunny,
	[WHITE_BALANCE_CLOUDY]          = s5k5ca_wb_cloudy,
	[WHITE_BALANCE_TUNGSTEN]        = s5k5ca_wb_tungsten,
	[WHITE_BALANCE_FLUORESCENT]     = s5k5ca_wb_fluorescent,
};

/*
 * V4L2_CID_CAMERA_EFFECT (V4L2_CID_PRIVATE_BASE + 74)
 * V4L2_CID_COLORFX	(V4L2_CID_BASE+31)
 */
 
//None
static const struct s5k5ca_reg s5k5ca_color_normal[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x021E},
	{0x0F12, 0x0000},
	{REGSET_END, 0},
};

//BW
static const struct s5k5ca_reg s5k5ca_color_monochrome[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x021E},
	{0x0F12, 0x0001},
	{REGSET_END, 0},
};

//Sepia
static const struct s5k5ca_reg s5k5ca_color_sepia[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x021E},
	{0x0F12, 0x0004},
	{REGSET_END, 0},
};

//Negative
static const struct s5k5ca_reg s5k5ca_color_negative[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x021E},
	{0x0F12, 0x0003},
	{REGSET_END, 0},
};

//Emboss
static const struct s5k5ca_reg s5k5ca_color_emboss[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x021E},
	{0x0F12, 0x0008},
	{REGSET_END, 0},
};

//Sketch
static const struct s5k5ca_reg s5k5ca_color_sketch[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x021E},
	{0x0F12, 0x0006},
	{REGSET_END, 0},
};

//Sky blue
static const struct s5k5ca_reg s5k5ca_color_blue[] = {
	{REGSET_END, 0},
};

//Grass green
static const struct s5k5ca_reg s5k5ca_color_aqua[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x021E},
	{0x0F12, 0x0005},
	{REGSET_END, 0},
};

//Skin whiten
static const struct s5k5ca_reg s5k5ca_color_skin[] = {
	{REGSET_END, 0},
};

/* Order of this array should be following the querymenu data */
static const struct s5k5ca_reg *s5k5ca_regs_color_effect[] = {
        [IMAGE_EFFECT_NONE]             = s5k5ca_color_normal,
        [IMAGE_EFFECT_BNW]              = s5k5ca_color_monochrome,
        [IMAGE_EFFECT_NEGATIVE]         = s5k5ca_color_negative,
        [IMAGE_EFFECT_SEPIA]            = s5k5ca_color_sepia,
        [IMAGE_EFFECT_AQUA]             = s5k5ca_color_aqua,

//	[V4L2_COLORFX_NONE]		= s5k5ca_color_normal,
//	[V4L2_COLORFX_BW]		= s5k5ca_color_bw,
//	[V4L2_COLORFX_NEGATIVE]		= s5k5ca_color_negative,
//	[V4L2_COLORFX_SEPIA]		= s5k5ca_color_sepia,	
//	[V4L2_COLORFX_GRASS_GREEN]	= s5k5ca_color_aqua,
//	[V4L2_COLORFX_EMBOSS]		= s5k5ca_color_emboss,
//	[V4L2_COLORFX_SKETCH]		= s5k5ca_color_sketch,
//	[V4L2_COLORFX_SKY_BLUE]		= s5k5ca_color_blue,
//	[V4L2_COLORFX_SKIN_WHITEN]	= s5k5ca_color_skin,
};

/*
 * EV bias (Exposure & Brightness) AE Target
 * V4L2_CID_CAMERA_BRIGHTNESS (V4L2_CID_PRIVATE_BASE + 72)
 */

static const struct s5k5ca_reg s5k5ca_ev_m4[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x0F70},
	{0x0F12, 0x0028},
	{REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_ev_m3[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x0F70},
	{0x0F12, 0x002F},
	{REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_ev_m2[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x0F70},
	{0x0F12, 0x0030},
	{REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_ev_m1[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x0F70},
	{0x0F12, 0x0035},
	{REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_ev_default[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x0F70},
	{0x0F12, 0x0045},
	{REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_ev_p1[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x0F70},
	{0x0F12, 0x0050},
	{REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_ev_p2[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x0F70},
	{0x0F12, 0x0055},
	{REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_ev_p3[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x0F70},
	{0x0F12, 0x0060},
	{REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_ev_p4[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x0F70},
	{0x0F12, 0x0065},
	{REGSET_END, 0},
};

/* Order of this array should be following the querymenu data */
static const struct s5k5ca_reg *s5k5ca_regs_ev_bias[] = {
	[EV_MINUS_4]		= s5k5ca_ev_m4,		
	[EV_MINUS_3]		= s5k5ca_ev_m3,		
	[EV_MINUS_2]		= s5k5ca_ev_m2,		
	[EV_MINUS_1]		= s5k5ca_ev_m1,		
	[EV_DEFAULT]		= s5k5ca_ev_default,
	[EV_PLUS_1]			= s5k5ca_ev_p1,
	[EV_PLUS_2]			= s5k5ca_ev_p2,
	[EV_PLUS_3]			= s5k5ca_ev_p3,
	[EV_PLUS_4]			= s5k5ca_ev_p4,
};
 
/*
 * Contrast bias
 * V4L2_CID_CAMERA_CONTRAST     (V4L2_CID_PRIVATE_BASE + 77)
 *
 */
static const struct s5k5ca_reg s5k5ca_contrast_m2[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x020E},
	{0x0F12, 0x0000},
	{REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_contrast_m1[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x020E},
	{0x0F12, 0x0010},
	{REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_contrast_default[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x020E},
	{0x0F12, 0x0020},
	{REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_contrast_p1[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x020E},
	{0x0F12, 0x0030},
	{REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_contrast_p2[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x020E},
	{0x0F12, 0x0040},
	{REGSET_END, 0},
};

/* Order of this array should be following the querymenu data */
static const struct s5k5ca_reg *s5k5ca_regs_contrast_bias[] = {
	[CONTRAST_MINUS_2]		= s5k5ca_contrast_m2,		
	[CONTRAST_MINUS_1]		= s5k5ca_contrast_m1,		
	[CONTRAST_DEFAULT]		= s5k5ca_contrast_default,
	[CONTRAST_PLUS_1]		= s5k5ca_contrast_p1,
	[CONTRAST_PLUS_2]		= s5k5ca_contrast_p2,
};

/*
 * Saturation bias
 * V4L2_CID_CAMERA_SATURATION     (V4L2_CID_PRIVATE_BASE + 78)
 *
 */
static const struct s5k5ca_reg s5k5ca_saturation_m2[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x0210},
	{0x0F12, 0x0000},
	{REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_saturation_m1[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x0210},
	{0x0F12, 0x0010},
	{REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_saturation_default[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x0210},
	{0x0F12, 0x0020},
	{REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_saturation_p1[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x0210},
	{0x0F12, 0x0030},
	{REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_saturation_p2[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x0210},
	{0x0F12, 0x0040},
	{REGSET_END, 0},
};

/* Order of this array should be following the querymenu data */
static const struct s5k5ca_reg *s5k5ca_regs_saturation_bias[] = {
	[SATURATION_MINUS_2]		= s5k5ca_saturation_m2,		
	[SATURATION_MINUS_1]		= s5k5ca_saturation_m1,		
	[SATURATION_DEFAULT]		= s5k5ca_saturation_default,
	[SATURATION_PLUS_1]			= s5k5ca_saturation_p1,
	[SATURATION_PLUS_2]			= s5k5ca_saturation_p2,
};

/*
 * Sharpness bias
 * V4L2_CID_CAMERA_SHARPNESS   (V4L2_CID_PRIVATE_BASE + 79)
 */
static const struct s5k5ca_reg s5k5ca_sharpness_m2[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x0212},
	{0x0F12, 0x0000},
	{REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_sharpness_m1[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x0212},
	{0x0F12, 0x0010},
	{REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_sharpness_default[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x0212},
	{0x0F12, 0x0020},
	{REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_sharpness_p1[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x0212},
	{0x0F12, 0x0030},
	{REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_sharpness_p2[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x0212},
	{0x0F12, 0x0040},
	{REGSET_END, 0},
};

/* Order of this array should be following the querymenu data */
static const struct s5k5ca_reg *s5k5ca_regs_sharpness_bias[] = {
	[SHARPNESS_MINUS_2]		= s5k5ca_sharpness_m2,		
	[SHARPNESS_MINUS_1]		= s5k5ca_sharpness_m1,		
	[SHARPNESS_DEFAULT]		= s5k5ca_sharpness_default,
	[SHARPNESS_PLUS_1]		= s5k5ca_sharpness_p1,
	[SHARPNESS_PLUS_2]		= s5k5ca_sharpness_p2,
};

/*
 *  * V4L2_CID_CAMERA_ISO                (V4L2_CID_PRIVATE_BASE + 75)
 *   */
static const struct s5k5ca_reg s5k5ca_iso_auto[] = {
        {REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_iso_50[] = {
        {REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_iso_100[] = {
        {REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_iso_200[] = {
        {REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_iso_400[] = {
        {REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_iso_800[] = {
        {REGSET_END, 0},
};

/* Order of this array should be following the querymenu data */
static const struct s5k5ca_reg *s5k5ca_regs_iso[] = {
        [ISO_AUTO]      = s5k5ca_iso_auto,
        [ISO_50]        = s5k5ca_iso_50,
        [ISO_100]       = s5k5ca_iso_100,
        [ISO_200]       = s5k5ca_iso_200,
        [ISO_400]       = s5k5ca_iso_400,
        [ISO_800]       = s5k5ca_iso_800,
};

/*
 *  V4L2_CID_CAMERA_ZOOM   (V4L2_CID_PRIVATE_BASE + 90)
 */
static const struct s5k5ca_reg s5k5ca_zoom_level_0[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_zoom_level_1[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_zoom_level_2[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{REGSET_END, 0},
};


static const struct s5k5ca_reg s5k5ca_zoom_level_3[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_zoom_level_4[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{REGSET_END, 0},
};

static const struct s5k5ca_reg s5k5ca_zoom_level_5[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{REGSET_END, 0},
};

static const struct s5k5ca_reg *s5k5ca_regs_zoom_level[] = {
	[ZOOM_LEVEL_0]  = s5k5ca_zoom_level_0,
    [ZOOM_LEVEL_1]  = s5k5ca_zoom_level_1,
    [ZOOM_LEVEL_2]  = s5k5ca_zoom_level_2,
    [ZOOM_LEVEL_3]  = s5k5ca_zoom_level_3,
    [ZOOM_LEVEL_4]  = s5k5ca_zoom_level_4,
    [ZOOM_LEVEL_5]  = s5k5ca_zoom_level_5,
};

/*
 * ********  preview mode  ******
 */

//VGA:640*480
static const struct s5k5ca_reg s5k5ca_regs_configure_640x480_preview[] = {
	{REGSET_END, 0},
};

//SVGA:800*600
static const struct s5k5ca_reg s5k5ca_regs_configure_800x600_preview[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},	//Select page

	{0x002A, 0x023C},
	{0x0F12, 0x0000}, // #REG_TC_GP_ActivePrevConfig // Select preview configuration_0
	{0x002A, 0x0240},
	{0x0F12, 0x0001}, // #REG_TC_GP_PrevOpenAfterChange
	{0x002A, 0x0230},
	{0x0F12, 0x0001}, // #REG_TC_GP_NewConfigSync // Update preview configuration
	{0x002A, 0x023E},
	{0x0F12, 0x0001}, // #REG_TC_GP_PrevConfigChanged
	{0x002A, 0x0220},
	{0x0F12, 0x0001}, // #REG_TC_GP_EnablePreview // Start preview
	{0x0F12, 0x0001}, // #REG_TC_GP_EnablePreviewChanged
	{REGSET_END, 0},
};

//XGA:1024*768
static const struct s5k5ca_reg s5k5ca_regs_configure_1024x768_preview[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},	//Select page

	{0x002A, 0x023C},
	{0x0F12, 0x0001}, // #REG_TC_GP_ActivePrevConfig // Select preview configuration_1
	{0x002A, 0x0240},
	{0x0F12, 0x0001}, // #REG_TC_GP_PrevOpenAfterChange
	{0x002A, 0x0230},
	{0x0F12, 0x0001}, // #REG_TC_GP_NewConfigSync // Update preview configuration
	{0x002A, 0x023E},
	{0x0F12, 0x0001}, // #REG_TC_GP_PrevConfigChanged
	{0x002A, 0x0220},
	{0x0F12, 0x0001}, // #REG_TC_GP_EnablePreview // Start preview
	{0x0F12, 0x0001}, // #REG_TC_GP_EnablePreviewChanged

	{REGSET_END, 0},
};

//UXGA:1600*1200
static const struct s5k5ca_reg s5k5ca_regs_configure_1600x1200_preview[] = {
	{REGSET_END, 0},
};

//QXGA:2048*1536
static const struct s5k5ca_reg s5k5ca_regs_configure_2048x1536_preview[] = {
	{REGSET_END, 0},
};


/*
 * ******** capture mode *******
 */
//VGA:640*480
static const struct s5k5ca_reg s5k5ca_regs_configure_640x480_capture[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},	//Select page

	//================================================================================================
	// SET CAPTURE CONFIGURATION_2
	// # Foramt :JPEG
	// # Size: VGA: 640X480
	// # FPS : 30 ~ 7.5fps
	//================================================================================================
	{0x002A, 0x035C}, //#REG_2TC_CCFG_uCaptureMode
	{0x0F12, 0x0000}, //#REG_2TC_CCFG_uCaptureModeJpEG
	{0x0F12, 0x0280}, //#REG_2TC_CCFG_usWidth  640
	{0x0F12, 0x01E0}, //#REG_2TC_CCFG_usHeight 480
	{0x0F12, 0x0005}, //#REG_2TC_CCFG_Format//5:YUV9:JPEG 
	{0x0F12, 0x3AA8}, //#REG_2TC_CCFG_usMaxOut4KHzRate
	{0x0F12, 0x3A88}, //#REG_2TC_CCFG_usMinOut4KHzRate
	{0x0F12, 0x0100}, //#REG_2TC_CCFG_OutClkPerPix88
	{0x0F12, 0x0800}, //#REG_2TC_CCFG_uMaxBpp88 
	{0x0F12, 0x0052}, //#REG_2TC_CCFG_PVIMask 
	{0x0F12, 0x0050}, //#REG_2TC_CCFG_OIFMask   edison
	{0x0F12, 0x01E0}, //#REG_2TC_CCFG_usJpegPacketSize
	{0x0F12, 0x08fc}, //#REG_2TC_CCFG_usJpegTotalPackets
	{0x0F12, 0x0000}, //#REG_2TC_CCFG_uClockInd 
	{0x0F12, 0x0000}, //#REG_2TC_CCFG_usFrTimeType
	{0x0F12, 0x0002}, //#REG_2TC_CCFG_FrRateQualityType 
	{0x0F12, 0x0535}, //#REG_2TC_CCFG_usMaxFrTimeMsecMult10 //7.5fps
	{0x0F12, 0x014D}, //#REG_2TC_CCFG_usMinFrTimeMsecMult10 //30fps 
	{0x0F12, 0x0000}, //#REG_2TC_CCFG_bSmearOutput
	{0x0F12, 0x0000}, //#REG_2TC_CCFG_sSaturation 
	{0x0F12, 0x0000}, //#REG_2TC_CCFG_sSharpBlur
	{0x0F12, 0x0000}, //#REG_2TC_CCFG_sColorTemp
	{0x0F12, 0x0000}, //#REG_2TC_CCFG_uDeviceGammaIndex 	


	{0x002A, 0x0244}, 	//Normal capture// 
	{0x0F12, 0x0000}, 	//config 2 // REG_TC_GP_ActiveCapConfig
	{0x002A, 0x0240}, 
	{0x0F12, 0x0001},	//#REG_TC_GP_PrevOpenAfterChange
	{0x002A, 0x0230}, 
	{0x0F12, 0x0001},	//REG_TC_GP_NewConfigSync
	{0x002A, 0x0246},                    
	{0x0F12, 0x0001},   //REG_TC_GP_CapConfigChanged (Synchronize FW with new capture configuration)
	{0x002A, 0x0224},                    
	{0x0F12, 0x0001},	// REG_TC_GP_EnableCapture         
	{0x0F12, 0x0001},    // REG_TC_GP_EnableCaptureChanged

	{REGSET_END, 0},
};

//SVGA:800*600
static const struct s5k5ca_reg s5k5ca_regs_configure_800x600_capture[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},	//Select page


	//================================================================================================
	// SET CAPTURE CONFIGURATION_1
	// # Foramt :JPEG
	// # Size: SVGA: 800X600
	// # FPS : 30 ~ 7.5fps
	//================================================================================================
	{0x002A, 0x035C}, //#REG_1TC_CCFG_uCaptureMode
	{0x0F12, 0x0000}, //#REG_1TC_CCFG_uCaptureModeJpEG
	{0x0F12, 0x0320}, //#REG_1TC_CCFG_usWidth  800
	{0x0F12, 0x0258}, //#REG_1TC_CCFG_usHeight 600
	{0x0F12, 0x0005}, //#REG_1TC_CCFG_Format//5:YUV9:JPEG 
	{0x0F12, 0x3AA8}, //#REG_1TC_CCFG_usMaxOut4KHzRate
	{0x0F12, 0x3A88}, //#REG_1TC_CCFG_usMinOut4KHzRate
	{0x0F12, 0x0100}, //#REG_1TC_CCFG_OutClkPerPix88
	{0x0F12, 0x0800}, //#REG_1TC_CCFG_uMaxBpp88 
	{0x0F12, 0x0052}, //#REG_1TC_CCFG_PVIMask 
	{0x0F12, 0x0050}, //#REG_1TC_CCFG_OIFMask   edison
	{0x0F12, 0x01E0}, //#REG_1TC_CCFG_usJpegPacketSize
	{0x0F12, 0x08fc}, //#REG_1TC_CCFG_usJpegTotalPackets
	{0x0F12, 0x0000}, //#REG_1TC_CCFG_uClockInd 
	{0x0F12, 0x0000}, //#REG_1TC_CCFG_usFrTimeType
	{0x0F12, 0x0002}, //#REG_1TC_CCFG_FrRateQualityType 
	{0x0F12, 0x0535}, //#REG_1TC_CCFG_usMaxFrTimeMsecMult10 //7.5fps
	{0x0F12, 0x014D}, //#REG_1TC_CCFG_usMinFrTimeMsecMult10 //30fps 
	{0x0F12, 0x0000}, //#REG_1TC_CCFG_bSmearOutput
	{0x0F12, 0x0000}, //#REG_1TC_CCFG_sSaturation 
	{0x0F12, 0x0000}, //#REG_1TC_CCFG_sSharpBlur
	{0x0F12, 0x0000}, //#REG_1TC_CCFG_sColorTemp
	{0x0F12, 0x0000}, //#REG_1TC_CCFG_uDeviceGammaIndex 

	{0x002A, 0x0244}, 	//Normal capture// 
	{0x0F12, 0x0000}, 	//config 1 // REG_TC_GP_ActiveCapConfig 
	{0x002A, 0x0240}, 
	{0x0F12, 0x0001},	//#REG_TC_GP_PrevOpenAfterChange
	{0x002A, 0x0230},                    
	{0x0F12, 0x0001},	//REG_TC_GP_NewConfigSync          
	{0x002A, 0x0246},                    
	{0x0F12, 0x0001},   //Synchronize FW with new capture configuration          
	{0x002A, 0x0224},                    
	{0x0F12, 0x0001},	// REG_TC_GP_EnableCapture         
	{0x0F12, 0x0001},    // REG_TC_GP_EnableCaptureChanged
	
	{REGSET_END, 0},
};

//XGA:1024*768
static const struct s5k5ca_reg s5k5ca_regs_configure_1024x768_capture[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},	//Select page
	
	//================================================================================================
	// SET CAPTURE CONFIGURATION_0
	// # Foramt :JPEG
	// # Size: XGA: 1024x768
	// # FPS : 30 ~ 7.5fps
	//================================================================================================
	{0x002A, 0x035C}, //#REG_0TC_CCFG_uCaptureMode
	{0x0F12, 0x0000}, //#REG_0TC_CCFG_uCaptureModeJpEG
	{0x0F12, 0x0400}, //#REG_0TC_CCFG_usWidth  1024
	{0x0F12, 0x0300}, //#REG_0TC_CCFG_usHeight 768
	{0x0F12, 0x0005}, //#REG_0TC_CCFG_Format//5:YUV 9:JPEG 
	{0x0F12, 0x3AA8}, //#REG_0TC_CCFG_usMaxOut4KHzRate
	{0x0F12, 0x3A88}, //#REG_0TC_CCFG_usMinOut4KHzRate
	{0x0F12, 0x0100}, //#REG_0TC_CCFG_OutClkPerPix88
	{0x0F12, 0x0800}, //#REG_0TC_CCFG_uMaxBpp88 
	{0x0F12, 0x0052}, //#REG_0TC_CCFG_PVIMask 
	{0x0F12, 0x0050}, //#REG_0TC_CCFG_OIFMask   edison
	{0x0F12, 0x01E0}, //#REG_0TC_CCFG_usJpegPacketSize
	{0x0F12, 0x08fc}, //#REG_0TC_CCFG_usJpegTotalPackets
	{0x0F12, 0x0000}, //#REG_0TC_CCFG_uClockInd 
	{0x0F12, 0x0000}, //#REG_0TC_CCFG_usFrTimeType
	{0x0F12, 0x0002}, //#REG_0TC_CCFG_FrRateQualityType 
	{0x0F12, 0x0535}, //#REG_0TC_CCFG_usMaxFrTimeMsecMult10 //7.5fps
	{0x0F12, 0x014D}, //#REG_0TC_CCFG_usMinFrTimeMsecMult10 //30fps 
	{0x0F12, 0x0000}, //#REG_0TC_CCFG_bSmearOutput
	{0x0F12, 0x0000}, //#REG_0TC_CCFG_sSaturation 
	{0x0F12, 0x0000}, //#REG_0TC_CCFG_sSharpBlur
	{0x0F12, 0x0000}, //#REG_0TC_CCFG_sColorTemp
	{0x0F12, 0x0000}, //#REG_0TC_CCFG_uDeviceGammaIndex 


	{0x002A, 0x0244}, 	//Normal capture// 
	{0x0F12, 0x0000}, 	//config 0 // REG_TC_GP_ActiveCapConfig    
	{0x002A, 0x0240}, 
	{0x0F12, 0x0001},	//#REG_TC_GP_PrevOpenAfterChange
	{0x002A, 0x0230},                    
	{0x0F12, 0x0001},	//REG_TC_GP_NewConfigSync          
	{0x002A, 0x0246},                    
	{0x0F12, 0x0001},   //Synchronize FW with new capture configuration          
	{0x002A, 0x0224},                    
	{0x0F12, 0x0001},	// REG_TC_GP_EnableCapture         
	{0x0F12, 0x0001},    // REG_TC_GP_EnableCaptureChanged
	
	{REGSET_END, 0},
};

//UXGA:1600*1200
static const struct s5k5ca_reg s5k5ca_regs_configure_1600x1200_capture[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},	//Select page

	//================================================================================================
	// SET CAPTURE CONFIGURATION_3
	// # Foramt :JPEG
	// # Size: UXGA: 1600X1200
	// # FPS : 22 ~ 7.5fps
	//================================================================================================
	{0x002A, 0x035C}, //#REG_3TC_CCFG_uCaptureMode
	{0x0F12, 0x0000}, //#REG_3TC_CCFG_uCaptureModeJpEG
	{0x0F12, 0x0640}, //#REG_3TC_CCFG_usWidth  1600
	{0x0F12, 0x04B0}, //#REG_3TC_CCFG_usHeight 1200
	{0x0F12, 0x0005}, //#REG_3TC_CCFG_Format//5:YUV9:JPEG 
	{0x0F12, 0x3AA8}, //#REG_3TC_CCFG_usMaxOut4KHzRate
	{0x0F12, 0x3A88}, //#REG_3TC_CCFG_usMinOut4KHzRate
	{0x0F12, 0x0100}, //#REG_3TC_CCFG_OutClkPerPix88
	{0x0F12, 0x0800}, //#REG_3TC_CCFG_uMaxBpp88 
	{0x0F12, 0x0052}, //#REG_3TC_CCFG_PVIMask 
	{0x0F12, 0x0050}, //#REG_3TC_CCFG_OIFMask   edison
	{0x0F12, 0x01E0}, //#REG_3TC_CCFG_usJpegPacketSize
	{0x0F12, 0x08fc}, //#REG_3TC_CCFG_usJpegTotalPackets
	{0x0F12, 0x0000}, //#REG_3TC_CCFG_uClockInd 
	{0x0F12, 0x0000}, //#REG_3TC_CCFG_usFrTimeType
	{0x0F12, 0x0002}, //#REG_3TC_CCFG_FrRateQualityType 
	{0x0F12, 0x0535}, //#REG_3TC_CCFG_usMaxFrTimeMsecMult10 //7.5fps
	{0x0F12, 0x01C6}, //#REG_3TC_CCFG_usMinFrTimeMsecMult10 //22fps 
	{0x0F12, 0x0000}, //#REG_3TC_CCFG_bSmearOutput
	{0x0F12, 0x0000}, //#REG_3TC_CCFG_sSaturation 
	{0x0F12, 0x0000}, //#REG_3TC_CCFG_sSharpBlur
	{0x0F12, 0x0000}, //#REG_3TC_CCFG_sColorTemp
	{0x0F12, 0x0000}, //#REG_3TC_CCFG_uDeviceGammaIndex 	

	{0x002A, 0x0244}, 	//Normal capture// 
	{0x0F12, 0x0000}, 	//config 3 // REG_TC_GP_ActiveCapConfig 
	{0x002A, 0x0240}, 
	{0x0F12, 0x0001},	//#REG_TC_GP_PrevOpenAfterChange
	{0x002A, 0x0230},                    
	{0x0F12, 0x0001},	//REG_TC_GP_NewConfigSync          
	{0x002A, 0x0246},                    
	{0x0F12, 0x0001},   //Synchronize FW with new capture configuration          
	{0x002A, 0x0224},                    
	{0x0F12, 0x0001},	// REG_TC_GP_EnableCapture         
	{0x0F12, 0x0001},    // REG_TC_GP_EnableCaptureChanged

	{REGSET_END, 0},
};

//QXGA:2048*1536
static const struct s5k5ca_reg s5k5ca_regs_configure_2048x1536_capture[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},	//Select page
	
	//================================================================================================
	// SET CAPTURE CONFIGURATION_4
	// # Foramt :JPEG
	// # Size: QXGA: 2048X1536
	// # FPS : 22 ~ 7.5fps
	//================================================================================================
	{0x002A, 0x035C}, //#REG_4TC_CCFG_uCaptureMode
	{0x0F12, 0x0000}, //#REG_4TC_CCFG_uCaptureModeJpEG
	{0x0F12, 0x0800}, //#REG_4TC_CCFG_usWidth  2048
	{0x0F12, 0x0600}, //#REG_4TC_CCFG_usHeight 1536
	{0x0F12, 0x0005}, //#REG_4TC_CCFG_Format//5:YUV9:JPEG 
	{0x0F12, 0x3AA8}, //#REG_4TC_CCFG_usMaxOut4KHzRate
	{0x0F12, 0x3A88}, //#REG_4TC_CCFG_usMinOut4KHzRate
	{0x0F12, 0x0100}, //#REG_4TC_CCFG_OutClkPerPix88
	{0x0F12, 0x0800}, //#REG_4TC_CCFG_uMaxBpp88 
	{0x0F12, 0x0052}, //#REG_4TC_CCFG_PVIMask 
	{0x0F12, 0x0050}, //#REG_4TC_CCFG_OIFMask   edison
	{0x0F12, 0x01E0}, //#REG_4TC_CCFG_usJpegPacketSize
	{0x0F12, 0x08fc}, //#REG_4TC_CCFG_usJpegTotalPackets
	{0x0F12, 0x0000}, //#REG_4TC_CCFG_uClockInd 
	{0x0F12, 0x0000}, //#REG_4TC_CCFG_usFrTimeType
	{0x0F12, 0x0002}, //#REG_4TC_CCFG_FrRateQualityType 
	{0x0F12, 0x0535}, //#REG_4TC_CCFG_usMaxFrTimeMsecMult10 //7.5fps
	{0x0F12, 0x01C6}, //#REG_4TC_CCFG_usMinFrTimeMsecMult10 //22fps 
	{0x0F12, 0x0000}, //#REG_4TC_CCFG_bSmearOutput
	{0x0F12, 0x0000}, //#REG_4TC_CCFG_sSaturation 
	{0x0F12, 0x0000}, //#REG_4TC_CCFG_sSharpBlur
	{0x0F12, 0x0000}, //#REG_4TC_CCFG_sColorTemp
	{0x0F12, 0x0000}, //#REG_4TC_CCFG_uDeviceGammaIndex

	{0x002A, 0x0244}, 	//Normal capture// 
	{0x0F12, 0x0000}, 	//config 4 // REG_TC_GP_ActiveCapConfig 
	{0x002A, 0x0240}, 
	{0x0F12, 0x0001},	//#REG_TC_GP_PrevOpenAfterChange
	{0x002A, 0x0230},                    
	{0x0F12, 0x0001},	//REG_TC_GP_NewConfigSync          
	{0x002A, 0x0246},                    
	{0x0F12, 0x0001},   //Synchronize FW with new capture configuration          
	{0x002A, 0x0224},                    
	{0x0F12, 0x0001},	// REG_TC_GP_EnableCapture         
	{0x0F12, 0x0001},    // REG_TC_GP_EnableCaptureChanged

	{REGSET_END, 0},
};

/*
 * ******** record mode *******
 */
//VGA:640*480
static const struct s5k5ca_reg s5k5ca_regs_configure_640x480_record[] = {
	{REGSET_END, 0},
};

//SVGA:800*600
static const struct s5k5ca_reg s5k5ca_regs_configure_800x600_record[] = {
	{REGSET_END, 0},
};

//XGA:1024*768 Fixed frame rate 30fps 
static const struct s5k5ca_reg s5k5ca_regs_configure_1024x768_record[] = {
	{REGSET_END, 0},
};

//UXGA:1600*1200
static const struct s5k5ca_reg s5k5ca_regs_configure_1600x1200_record[] = {
	{REGSET_END, 0},
};

//QXGA:2048*1536
static const struct s5k5ca_reg s5k5ca_regs_configure_2048x1536_record[] = {
	{REGSET_END, 0},
};

/* Camera mode */
enum
{
	CAMERA_PREVIEW = 0,
	CAMERA_CAPTURE,
	CAMERA_RECORD,
	CAMERA_CAPTURE_JPEG,
};

enum s5k5ca_preview_frame_size {
	S5K5CA_ePREVIEW_VGA = 0,	/* 640x480 */
	S5K5CA_PREVIEW_SVGA,		/* 800x600 */
	S5K5CA_PREVIEW_XGA,			/* 1024x768 */
	S5K5CA_PREVIEW_UXGA,		/* 1600x1200 */
	S5K5CA_PREVIEW_QXGA,		/* 2048x1536 */
};

enum s5k5ca_capture_frame_size {
	S5K5CA_CAPTURE_VGA = 0,		/* 640x480 */
	S5K5CA_CAPTURE_SVGA,		/* 800x600 */
	S5K5CA_CAPTURE_XGA,			/* 1024x768 */
	S5K5CA_CAPTURE_UXGA,		/* 1600x1200 */
	S5K5CA_CAPTURE_QXGA,		/* 2048x1536 */
};

enum s5k5ca_record_frame_size {
	S5K5CA_RECORD_XGA = 0,		/* 1024x768 */
};

struct s5k5ca_framesize {
        u32 index;
        u32 width;
        u32 height;
};

static const struct s5k5ca_framesize s5k5ca_preview_framesize_list[] = {
//	{ S5K5CA_PREVIEW_VGA,	640,  480 },
	{ S5K5CA_PREVIEW_SVGA,	800,  600 },
	{ S5K5CA_PREVIEW_XGA,	1024, 768},
//	{ S5K5CA_PREVIEW_UXGA,	1600, 1200},
//	{ S5K5CA_PREVIEW_QXGA,	2048, 1536},
};

static const struct s5k5ca_framesize s5k5ca_capture_framesize_list[] = {
	{ S5K5CA_CAPTURE_VGA,	640, 480 },
	{ S5K5CA_CAPTURE_SVGA,	800, 600 },
	{ S5K5CA_CAPTURE_XGA,   1024, 768},
	{ S5K5CA_CAPTURE_UXGA,  1600, 1200},
	{ S5K5CA_CAPTURE_QXGA,	2048, 1536},
};

static const struct s5k5ca_framesize s5k5ca_record_framesize_list[] = {
	{ S5K5CA_RECORD_XGA,   1024, 768},
};

static const struct s5k5ca_reg *s5k5ca_regs_preview[] = {
	[S5K5CA_PREVIEW_SVGA]	= s5k5ca_regs_configure_800x600_preview,
	[S5K5CA_PREVIEW_XGA]	= s5k5ca_regs_configure_1024x768_preview,
};

static const struct s5k5ca_reg *s5k5ca_regs_capture[] = {
	[S5K5CA_CAPTURE_VGA]	= s5k5ca_regs_configure_640x480_capture,
	[S5K5CA_CAPTURE_SVGA]	= s5k5ca_regs_configure_800x600_capture,
	[S5K5CA_CAPTURE_XGA]	= s5k5ca_regs_configure_1024x768_capture,
	[S5K5CA_CAPTURE_UXGA]	= s5k5ca_regs_configure_1600x1200_capture,
	[S5K5CA_CAPTURE_QXGA]	= s5k5ca_regs_configure_2048x1536_capture,
};

static const struct s5k5ca_reg *s5k5ca_regs_record[] = {
	[S5K5CA_RECORD_XGA]	= s5k5ca_regs_configure_1024x768_record,
};

#endif

