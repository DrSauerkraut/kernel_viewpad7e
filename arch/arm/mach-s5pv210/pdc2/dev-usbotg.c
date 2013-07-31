/* linux/arch/arm/mach-s5pv210/$(CONFIG_PROJECT_NAME)/dev-usbotg.c
 *
 * Copyright (c) 2009 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Copyright (c) 2011 Venus Project
 *
 * Base S5P resource and device definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/platform_device.h>

//#include <plat/devs.h>

/* Android Gadget */
#include <linux/usb/android_composite.h>
#if defined(CONFIG_PRODUCT_V7E)
#define S3C_VENDOR_ID		0x0bb0   //0x18d1       //
#define S3C_PRODUCT_ID		0x2a2b
#define S3C_ADB_PRODUCT_ID	0x0005
#elif defined(CONFIG_PRODUCT_KS8)
#define S3C_VENDOR_ID		0x04fc   //0x18d1       //
#define S3C_PRODUCT_ID		0xe780
#define S3C_ADB_PRODUCT_ID	0x0005
#else
#define S3C_VENDOR_ID		0x0bb4   //0x18d1       //
#define S3C_PRODUCT_ID		0x0c01
#define S3C_ADB_PRODUCT_ID	0x0005
#endif

#define MAX_USB_SERIAL_NUM	17

static char *usb_functions_ums[] = {
	"usb_mass_storage",
};

static char *usb_functions_rndis[] = {
	"rndis",
};

static char *usb_functions_rndis_adb[] = {
	"rndis",
	"adb",
};
static char *usb_functions_ums_adb[] = {
	"usb_mass_storage",
	"adb",
};

static char *usb_functions_all[] = {
#ifdef CONFIG_USB_ANDROID_RNDIS
	"rndis",
#endif
	"usb_mass_storage",
	"adb",
#ifdef CONFIG_USB_ANDROID_ACM
	"acm",
#endif
};
static struct android_usb_product usb_products[] = {
	{
		.product_id	= S3C_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums),
		.functions	= usb_functions_ums,
	},
	{
		.product_id	= S3C_ADB_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums_adb),
		.functions	= usb_functions_ums_adb,
	},
	/*
	{
		.product_id	= S3C_RNDIS_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis),
		.functions	= usb_functions_rndis,
	},
	{
		.product_id	= S3C_RNDIS_ADB_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis_adb),
		.functions	= usb_functions_rndis_adb,
	},
	*/
};
// serial number should be changed as real device for commercial release
#if defined(CONFIG_PRODUCT_V7E)
static char device_serial[MAX_USB_SERIAL_NUM]="ViewSonic";
#elif defined(CONFIG_PRODUCT_KS8)
static char device_serial[MAX_USB_SERIAL_NUM]="S-book";
#else
static char device_serial[MAX_USB_SERIAL_NUM]="PD_Universe";
#endif
/* standard android USB platform data */

// Information should be changed as real product for commercial release
static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id		= S3C_VENDOR_ID,
	.product_id		= S3C_PRODUCT_ID,
#if defined(CONFIG_PRODUCT_V7E)
	.manufacturer_name	= "VS",//"Samsung",
	.product_name		= "ViewPad 7e",//"Samsung SMDKV210",
#elif defined(CONFIG_PRODUCT_KS8)
	.manufacturer_name	= "KS",//"Samsung",
	.product_name		= "S-book",//"Samsung SMDKV210",
#else
	.manufacturer_name	= "Pan Digital ",//"Samsung",
	.product_name		= "PD Universe",//"Samsung SMDKV210",
#endif
	.serial_number		= device_serial,
	.num_products 		= ARRAY_SIZE(usb_products),
	.products 		= usb_products,
	.num_functions 		= ARRAY_SIZE(usb_functions_all),
	.functions 		= usb_functions_all,
};

struct platform_device s3c_device_android_usb = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data	= &android_usb_pdata,
	},
};
EXPORT_SYMBOL(s3c_device_android_usb);

static struct usb_mass_storage_platform_data ums_pdata = {
#if defined(CONFIG_PRODUCT_V7E)
	.vendor			= "VS",//"Samsung",
	.product		= "ViewPad 7e",//"SMDKV210",
#elif defined(CONFIG_PRODUCT_KS8)
	.vendor	        = "KS",//"Samsung",
	.product        = "S-book",//"Samsung SMDKV210",
#else
	.vendor			= "Pan Digital ",//"Samsung",
	.product		= "PD Universe",//"SMDKV210",
#endif
	.release		= 1,
#ifdef CONFIG_USB_GADGET_ANDROID_MAX_LUNS
	.nluns          = CONFIG_USB_GADGET_ANDROID_MAX_LUNS,
#endif
};
struct platform_device s3c_device_usb_mass_storage= {
	.name	= "usb_mass_storage",
	.id	= -1,
	.dev	= {
		.platform_data = &ums_pdata,
	},
};
EXPORT_SYMBOL(s3c_device_usb_mass_storage);

