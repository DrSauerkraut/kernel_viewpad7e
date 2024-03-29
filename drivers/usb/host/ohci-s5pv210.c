/* ohci-s5pv210.c - Driver for USB HOST on Samsung S5PV210 processor
 *
 * Bus Glue for SAMSUNG S5PV210 USB HOST OHCI Controller
 *
 * (C) Copyright 1999 Roman Weissgaerber <weissg@vienna.at>
 * (C) Copyright 2000-2002 David Brownell <dbrownell@users.sourceforge.net>
 * (C) Copyright 2002 Hewlett-Packard Company
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * Author: Jingoo Han <jg1.han@samsung.com>
 *
 * Based on "ohci-au1xxx.c" by Matt Porter <mporter@kernel.crashing.org>
 * Modified for SAMSUNG s5pv210 OHCI by Jingoo Han <jg1.han@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/clk.h>
#include <linux/platform_device.h>

static struct clk *usb_clk;

extern int usb_disabled(void);

extern void usb_host_phy_init(void);
extern void usb_host_phy_off(void);

static void s5pv210_start_ohc(void);
static void s5pv210_stop_ohc(void);
static int ohci_hcd_s5pv210_drv_probe(struct platform_device *pdev);
static int ohci_hcd_s5pv210_drv_remove(struct platform_device *pdev);


#include <plat/gpio-cfg.h>
#include <mach/gpio.h>	

#define USB_ID_GPIO		        S5PV210_GPH3(2)	
#define USB_CONTROL1_GPIO		S5PV210_GPJ3(7)
#define USB_CONTROL2_GPIO		S5PV210_GPJ2(3)


#define  ohci_usb_regu
#ifdef  ohci_usb_regu 
#include <linux/regulator/consumer.h>
static  struct regulator *usb_anlg_regulator, *usb_dig_regulator;
//static  struct workqueue_struct *my_work_queue;
//static  struct work_struct work;


static void ohci_start_usb_host(void)
{
	if (!regulator_is_enabled(usb_anlg_regulator))
	{	/* ldo3 regulator on */		
		regulator_enable(usb_dig_regulator);
		/* ldo8 regulator on */
		regulator_enable(usb_anlg_regulator);
	}
/*
	int  io_host_level= -1;
	io_host_level = __gpio_get_value(USB_ID_GPIO);
	if (io_host_level==0)
 	{
	gpio_direction_output(USB_CONTROL1_GPIO,  1 );   //close charge
 	gpio_direction_output(USB_CONTROL2_GPIO,  1 );   //open vbus
	}
*/
}

static void ohci_stop_usb_host(void)
{

	//gpio_direction_output(USB_CONTROL2_GPIO,  0 );  //close vbus
	//gpio_direction_output(USB_CONTROL1_GPIO,  0 );  //open charge

	if (regulator_is_enabled(usb_anlg_regulator))
	{	/* ldo8 regulator off */		
		regulator_disable(usb_anlg_regulator);
		/* ldo3 regulator off */
		regulator_disable(usb_dig_regulator);	
	}
}

#endif 


#ifdef CONFIG_PM
static int ohci_hcd_s5pv210_drv_suspend(
	struct platform_device *pdev,
	pm_message_t message
)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct ohci_hcd	*ohci = hcd_to_ohci(hcd);
	unsigned long flags;
	int rc = 0;

	//printk("111111%s111111",__func__);

	/* Root hub was already suspended. Disable irq emission and
	 * mark HW unaccessible, bail out if RH has been resumed. Use
	 * the spinlock to properly synchronize with possible pending
	 * RH suspend or resume activity.
	 *
	 * This is still racy as hcd->state is manipulated outside of
	 * any locks =P But that will be a different fix.
	 */
	spin_lock_irqsave(&ohci->lock, flags);
	if (hcd->state != HC_STATE_SUSPENDED) {
		rc = -EINVAL;
		goto bail;
	}

	/* make sure snapshot being resumed re-enumerates everything */
	if (message.event == PM_EVENT_PRETHAW)
		ohci_usb_reset(ohci);

	clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	s5pv210_stop_ohc();
bail:
	spin_unlock_irqrestore(&ohci->lock, flags);

	return rc;
}
static int ohci_hcd_s5pv210_drv_resume(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	int rc = 0;

	//printk("111111%s111111",__func__);

	s5pv210_start_ohc();

	/* Mark hardware accessible again as we are out of D3 state by now */
	set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	ohci_finish_controller_resume(hcd);

	return rc;
}
#else
#define ohci_hcd_s5pv210_drv_suspend NULL
#define ohci_hcd_s5pv210_drv_resume NULL
#endif


static void s5pv210_start_ohc(void)
{
	#ifdef	ohci_usb_regu
	ohci_start_usb_host();
	#endif
	clk_enable(usb_clk);
	usb_host_phy_init();
}

static void s5pv210_stop_ohc(void)
{
	usb_host_phy_off();
	clk_disable(usb_clk);
#ifdef	ohci_usb_regu
	ohci_stop_usb_host();
	//INIT_WORK(&work, ohci_stop_usb_host);
	//queue_work(my_work_queue, &work);
#endif
}

static int __devinit ohci_s5pv210_start(struct usb_hcd *hcd)
{
	struct ohci_hcd	*ohci = hcd_to_ohci(hcd);
	int ret;

	ohci_dbg(ohci, "ohci_s5pv210_start, ohci:%p", ohci);

	ret = ohci_init(ohci);
	if (ret < 0)
		return ret;

	ret = ohci_run(ohci);
	if (ret < 0) {
		err("can't start %s", hcd->self.bus_name);
		ohci_stop(hcd);
		return ret;
	}

	return 0;
}

static const struct hc_driver ohci_s5pv210_hc_driver = {
	.description		= hcd_name,
	.product_desc		= "s5pv210 OHCI",
	.hcd_priv_size		= sizeof(struct ohci_hcd),

	.irq			= ohci_irq,
	.flags			= HCD_MEMORY|HCD_USB11,

	.start			= ohci_s5pv210_start,
	.stop			= ohci_stop,
	.shutdown		= ohci_shutdown,

	.get_frame_number	= ohci_get_frame,

	.urb_enqueue		= ohci_urb_enqueue,
	.urb_dequeue		= ohci_urb_dequeue,
	.endpoint_disable	= ohci_endpoint_disable,

	.hub_status_data	= ohci_hub_status_data,
	.hub_control		= ohci_hub_control,
#ifdef	CONFIG_PM
	.bus_suspend		= ohci_bus_suspend,
	.bus_resume		= ohci_bus_resume,
#endif
	.start_port_reset	= ohci_start_port_reset,
};

static int ohci_hcd_s5pv210_drv_probe(struct platform_device *pdev)
{
	struct usb_hcd  *hcd = NULL;
	int retval = 0;

	int err;

#ifdef	CONFIG_PM
	#if 0
	my_work_queue = create_singlethread_workqueue("my_work_queue");
	if (!my_work_queue) {
		printk(KERN_WARNING "can't create work-queue");
		return -EBUSY;
	}
	#endif 

	/* ldo3 regulator on */
	usb_dig_regulator = regulator_get(NULL, "pd_io");
	if (IS_ERR(usb_dig_regulator)) {
		printk(KERN_ERR "failed to get resource %s\n", "pd_io");
		return PTR_ERR(usb_dig_regulator);
	}

	/* ldo8 regulator on */
	usb_anlg_regulator = regulator_get(NULL, "pd_core");
	if (IS_ERR(usb_anlg_regulator)) {
		printk(KERN_ERR "failed to get resource %s\n", "pd_core");
		return PTR_ERR(usb_anlg_regulator);
	}
	
#endif



#if  0	
	err = gpio_request(USB_CONTROL1_GPIO,"USB_CONTROL1");
	if (err)
			goto err_reset;
	err = gpio_request(USB_CONTROL2_GPIO,"USB_CONTROL2");
	if (err)
			goto err_reset;
	s3c_gpio_cfgpin(USB_CONTROL1_GPIO, S3C_GPIO_OUTPUT);
	s3c_gpio_cfgpin(USB_CONTROL2_GPIO, S3C_GPIO_OUTPUT);


	s3c_gpio_cfgpin(USB_ID_GPIO, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(USB_ID_GPIO, 2);

#endif

	if (usb_disabled())
		return -ENODEV;

	if (pdev->resource[1].flags != IORESOURCE_IRQ) {
		dev_err(&pdev->dev, "resource[1] is not IORESOURCE_IRQ.\n");
		return -ENODEV;
	}

	hcd = usb_create_hcd(&ohci_s5pv210_hc_driver, &pdev->dev, "s5pv210");
	if (!hcd) {
		dev_err(&pdev->dev, "usb_create_hcd failed!\n");
		return -ENODEV;
	}

	hcd->rsrc_start = pdev->resource[0].start;
	hcd->rsrc_len = pdev->resource[0].end - pdev->resource[0].start + 1;

	if (!request_mem_region(hcd->rsrc_start, hcd->rsrc_len, hcd_name)) {
		dev_err(&pdev->dev, "request_mem_region failed!\n");
		retval = -EBUSY;
		goto err1;
	}

	usb_clk = clk_get(&pdev->dev, "usb-host");
	if (IS_ERR(usb_clk)) {
		dev_err(&pdev->dev, "cannot get usb-host clock\n");
		retval = -ENODEV;
		goto err2;
	}

	s5pv210_start_ohc();

	hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);
	if (!hcd->regs) {
		dev_err(&pdev->dev, "ioremap failed!\n");
		retval = -ENOMEM;
		goto err2;
	}

	ohci_hcd_init(hcd_to_ohci(hcd));

	retval = usb_add_hcd(hcd, pdev->resource[1].start,
				IRQF_DISABLED | IRQF_SHARED);

	if (retval == 0) {
		platform_set_drvdata(pdev, hcd);
		return retval;
	}

	s5pv210_stop_ohc();
	iounmap(hcd->regs);
err2:
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
err1:
	clk_put(usb_clk);
	usb_put_hcd(hcd);
	return retval;

//err_reset:
//	 return err;
}

static int ohci_hcd_s5pv210_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	usb_remove_hcd(hcd);
	iounmap(hcd->regs);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	usb_put_hcd(hcd);
	s5pv210_stop_ohc();
	clk_put(usb_clk);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct platform_driver  ohci_hcd_s5pv210_driver = {
	.probe		= ohci_hcd_s5pv210_drv_probe,
	.remove		= ohci_hcd_s5pv210_drv_remove,
	.shutdown	= usb_hcd_platform_shutdown,
	.suspend	= ohci_hcd_s5pv210_drv_suspend,
	.resume		= ohci_hcd_s5pv210_drv_resume,
	.driver = {
		.name = "s5p-ohci",
		.owner = THIS_MODULE,
	}
};

MODULE_ALIAS("platform:s5p-ohci");
