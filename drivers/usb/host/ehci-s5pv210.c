/* ehci-s5pv210.c - Driver for USB HOST on Samsung S5PV210 processor
 *
 * Bus Glue for SAMSUNG S5PV210 USB HOST EHCI Controller
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * Author: Jingoo Han <jg1.han@samsung.com>
 *
 * Based on "ehci-au1xxx.c" by by K.Boge <karsten.boge@amd.com>
 * Modified for SAMSUNG s5pv210 EHCI by Jingoo Han <jg1.han@samsung.com>
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

static void s5pv210_start_ehc(void);
static void s5pv210_stop_ehc(void);
static int ehci_hcd_s5pv210_drv_probe(struct platform_device *pdev);
static int ehci_hcd_s5pv210_drv_remove(struct platform_device *pdev);


#include <linux/venus/power_control.h>


#if 1	// for USB DYNAMIC close

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/workqueue.h>
#include <plat/gpio-cfg.h>
#include <mach/gpio.h>	
#include <plat/irqs.h>
#include <asm/mach/irq.h>

#define USB_HOST_DYNAMIC	1


static struct work_struct		usb_monitor_host_work;

static const char host_driver_name[] = "s3c-hcd";

#define USB_ID_GPIO		        S5PV210_GPH3(2)	
#define USB_CONTROL1_GPIO		S5PV210_GPJ3(7)

//#if defined(CONFIG_PRODUCT_PDC3_DVT2)||defined(CONFIG_PRODUCT_PDC2)
#define USB_CONTROL2_GPIO		S5PV210_GPJ2(3)
//#endif

//#if defined(CONFIG_PRODUCT_PDC3_EVT2)||defined(CONFIG_PRODUCT_PDC2)
//#define USB_CONTROL2_GPIO		S5PV210_GPJ2(0)
//#endif

#endif


#define ehci_usb_regu
#ifdef  ehci_usb_regu 
#include <linux/regulator/consumer.h>
static  struct regulator *usb_anlg_regulator, *usb_dig_regulator;
//static  struct workqueue_struct *my_work_queue;
//static  struct work_struct work;


static void ehci_start_usb_host(void)
{

//request_power(CAM_2_8V);
//request_power(HDMI_POWER);

	if (!regulator_is_enabled(usb_anlg_regulator))
	{	/* ldo3 regulator on */
		//int usb_value=-1;		
		regulator_enable(usb_dig_regulator);
		/* ldo8 regulator on */
		regulator_enable(usb_anlg_regulator);
	}





	int  io_host_level= -1;
	gpio_request(USB_ID_GPIO,"USB_IDPIN");
	io_host_level = __gpio_get_value(USB_ID_GPIO);
	gpio_free(USB_ID_GPIO);


	if (io_host_level==0)
 	{
		gpio_direction_output(USB_CONTROL1_GPIO,  1 );   //close charge
 		gpio_direction_output(USB_CONTROL2_GPIO,  1 );   //open vbus
	}

}

static void ehci_stop_usb_host(void)
{

	gpio_direction_output(USB_CONTROL2_GPIO,  0 );  //close vbus
	gpio_direction_output(USB_CONTROL1_GPIO,  0 );  //open charge



	if (regulator_is_enabled(usb_anlg_regulator))
	{	/* ldo8 regulator off */
		int usb_value=8;		
		regulator_disable(usb_anlg_regulator);
		/* ldo3 regulator off */
		regulator_disable(usb_dig_regulator);	
	}

//release_power(HDMI_POWER);
//release_power(CAM_2_8V);
}

#endif 




#ifdef CONFIG_PM
static int ehci_hcd_s5pv210_drv_suspend(
		struct platform_device *pdev,
		pm_message_t message)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	unsigned long flags;
	int rc = 0;

	//printk("111111%s111111\n",__func__);

	if (time_before(jiffies, ehci->next_statechange))
		msleep(10);

	/* Root hub was already suspended. Disable irq emission and
	 * mark HW unaccessible, bail out if RH has been resumed. Use
	 * the spinlock to properly synchronize with possible pending
	 * RH suspend or resume activity.
	 *
	 * This is still racy as hcd->state is manipulated outside of
	 * any locks =P But that will be a different fix.
	 */

	spin_lock_irqsave(&ehci->lock, flags);
	if (hcd->state != HC_STATE_SUSPENDED) {
		rc = -EINVAL;
		goto bail;
	}
	ehci_writel(ehci, 0, &ehci->regs->intr_enable);
	(void)ehci_readl(ehci, &ehci->regs->intr_enable);

	/* make sure snapshot being resumed re-enumerates everything */
	if (message.event == PM_EVENT_PRETHAW) {
		ehci_halt(ehci);
		ehci_reset(ehci);
	}

	clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	s5pv210_stop_ehc();
//#if defined(CONFIG_PRODUCT_PDC3_DVT2)||defined(CONFIG_PRODUCT_PDC2)
release_power(HDMI_POWER);
//#endif
release_power(CAM_2_8V);

#if 0 //#ifdef ehci_usb_regu
	INIT_WORK(&work, ehci_stop_usb_host);
	queue_work(my_work_queue, &work);
#endif

bail:
	spin_unlock_irqrestore(&ehci->lock, flags);

	return rc;
}
static int ehci_hcd_s5pv210_drv_resume(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);

	//printk("111111%s111111\n",__func__);

request_power(CAM_2_8V);
//#if defined(CONFIG_PRODUCT_PDC3_DVT2)||defined(CONFIG_PRODUCT_PDC2)
request_power(HDMI_POWER);
//#endif
	s5pv210_start_ehc();

	if (time_before(jiffies, ehci->next_statechange))
		msleep(10);

	/* Mark hardware accessible again as we are out of D3 state by now */
	set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	if (ehci_readl(ehci, &ehci->regs->configured_flag) == FLAG_CF) {
		int	mask = INTR_MASK;

		if (!hcd->self.root_hub->do_remote_wakeup)
			mask &= ~STS_PCD;
		ehci_writel(ehci, mask, &ehci->regs->intr_enable);
		ehci_readl(ehci, &ehci->regs->intr_enable);
		return 0;
	}

	ehci_dbg(ehci, "lost power, restarting\n");
	usb_root_hub_lost_power(hcd->self.root_hub);

	(void) ehci_halt(ehci);
	(void) ehci_reset(ehci);

	/* emptying the schedule aborts any urbs */
	spin_lock_irq(&ehci->lock);
	if (ehci->reclaim)
		end_unlink_async(ehci);
	ehci_work(ehci);
	spin_unlock_irq(&ehci->lock);

	ehci_writel(ehci, ehci->command, &ehci->regs->command);
	ehci_writel(ehci, FLAG_CF, &ehci->regs->configured_flag);
	ehci_readl(ehci, &ehci->regs->command);	/* unblock posted writes */

	/* here we "know" root ports should always stay powered */
	ehci_port_power(ehci, 1);

	hcd->state = HC_STATE_SUSPENDED;

	return 0;
}

#else
#define ehci_hcd_s5pv210_drv_suspend NULL
#define ehci_hcd_s5pv210_drv_resume NULL
#endif

static void s5pv210_start_ehc(void)
{
	
	#ifdef	ehci_usb_regu
	ehci_start_usb_host();
	#endif
	clk_enable(usb_clk);
	usb_host_phy_init();
}

static void s5pv210_stop_ehc(void)
{
	usb_host_phy_off();
	clk_disable(usb_clk);
#ifdef	ehci_usb_regu
	ehci_stop_usb_host();
	//INIT_WORK(&work, ehci_stop_usb_host);
	//queue_work(my_work_queue, &work);
#endif
}

static const struct hc_driver ehci_s5pv210_hc_driver = {
	.description		= hcd_name,
	.product_desc		= "s5pv210 EHCI",
	.hcd_priv_size		= sizeof(struct ehci_hcd),

	.irq			= ehci_irq,
	.flags			= HCD_MEMORY|HCD_USB2,

	.reset			= ehci_init,
	.start			= ehci_run,
	.stop			= ehci_stop,
	.shutdown		= ehci_shutdown,

	.get_frame_number	= ehci_get_frame,

	.urb_enqueue		= ehci_urb_enqueue,
	.urb_dequeue		= ehci_urb_dequeue,
	.endpoint_disable	= ehci_endpoint_disable,
	.endpoint_reset		= ehci_endpoint_reset,

	.hub_status_data	= ehci_hub_status_data,
	.hub_control		= ehci_hub_control,
	.bus_suspend		= ehci_bus_suspend,
	.bus_resume		= ehci_bus_resume,
	.relinquish_port	= ehci_relinquish_port,
	.port_handed_over	= ehci_port_handed_over,

	.clear_tt_buffer_complete	= ehci_clear_tt_buffer_complete,
};


#if   USB_HOST_DYNAMIC	// for USB  host  module control the power of 5v
static int  usb_host_work(struct work_struct *work)
{

	int  io_level;
	int err = 0;
	gpio_request(USB_ID_GPIO,"USB_IDPIN");
	io_level = __gpio_get_value(USB_ID_GPIO);
	gpio_free(USB_ID_GPIO);
		
	printk("****get into usb host work function****,and io_level is %d\n", io_level);
#if 1
	//gpio_request(USB_CONTROL1_GPIO,"USB_CONTROL1");
	//gpio_request(USB_CONTROL2_GPIO,"USB_CONTROL2");

	if (io_level == 0)	// USB insert
	{
	
		err = gpio_direction_output(USB_CONTROL1_GPIO,  1 );  	 //close usb charge
		if (err)
			goto err_reset;

		err = gpio_direction_output(USB_CONTROL2_GPIO,  1 );	//open  for outputting the 5v voltage
		if (err)
			goto err_reset;
	//usb_host_flag=1;


	} 
	else 			// USB out
	{	

		err = gpio_direction_output(USB_CONTROL2_GPIO,  0 );	//close the outputting of 5v
		//err = gpio_direction_output(USB_CONTROL2_GPIO,  1 );	//close the outputting of 5v
		if (err)
			goto err_reset;

		err = gpio_direction_output(USB_CONTROL1_GPIO,  0 );	//open usb charge
		//err = gpio_direction_output(USB_CONTROL1_GPIO,  1 );	//open usb charge
		if (err)
			goto err_reset;

	//usb_host_flag=0;

	}
	//gpio_free(USB_CONTROL1_GPIO);
	//gpio_free(USB_CONTROL2_GPIO);
#endif
	enable_irq(__gpio_to_irq(USB_ID_GPIO));

	err_reset:
	    return err;
}

static irqreturn_t s3c_hcd_power_irq(int irq, void *handle)  
{
	
	disable_irq_nosync(__gpio_to_irq(USB_ID_GPIO));
	schedule_work(&usb_monitor_host_work);

	return IRQ_HANDLED;
}
#endif


static int ehci_hcd_s5pv210_drv_probe(struct platform_device *pdev)
{
	struct usb_hcd  *hcd = NULL;
	struct ehci_hcd *ehci = NULL;
	int retval = 0;

#if USB_HOST_DYNAMIC
	int err=0; 
#endif

#ifdef	CONFIG_PM

request_power(CAM_2_8V);
//#if defined(CONFIG_PRODUCT_PDC3_DVT2)||defined(CONFIG_PRODUCT_PDC2)
request_power(HDMI_POWER);
//#endif
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

	if (usb_disabled())
		return -ENODEV;

	usb_host_phy_off();

	if (pdev->resource[1].flags != IORESOURCE_IRQ) {
		dev_err(&pdev->dev, "resource[1] is not IORESOURCE_IRQ.\n");
		return -ENODEV;
	}

	hcd = usb_create_hcd(&ehci_s5pv210_hc_driver, &pdev->dev, "s5pv210");
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



#if  USB_HOST_DYNAMIC	
	err = gpio_request(USB_CONTROL1_GPIO,"USB_CONTROL1");
	if (err)
			goto err_reset;
	err = gpio_request(USB_CONTROL2_GPIO,"USB_CONTROL2");
	if (err)
			goto err_reset;

	s3c_gpio_cfgpin(USB_CONTROL1_GPIO, S3C_GPIO_OUTPUT);
	s3c_gpio_cfgpin(USB_CONTROL2_GPIO, S3C_GPIO_OUTPUT);
#endif

#if  USB_HOST_DYNAMIC	
	INIT_WORK(&usb_monitor_host_work, usb_host_work);
	s3c_gpio_cfgpin(USB_ID_GPIO, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(USB_ID_GPIO, 2);
	//s3c_gpio_setpull(USB_ID_GPIO, 1);


		//s3c_gpio_setpull(USB_CONTROL2_GPIO, 0);
		err = gpio_direction_output(USB_CONTROL2_GPIO,  0 );	//close vbus
	//	err = gpio_direction_output(USB_CONTROL2_GPIO,  1 );	//open  vbus
		if (err)
			goto err_reset;

		//s3c_gpio_setpull(USB_CONTROL1_GPIO, 0);
		err = gpio_direction_output(USB_CONTROL1_GPIO,  0 );   //open usb charge when opening
	//	err = gpio_direction_output(USB_CONTROL1_GPIO,  1 );	//close charge 
		if (err)
			goto err_reset;            

	//gpio_free(USB_CONTROL2_GPIO);
	//gpio_free(USB_CONTROL1_GPIO);

	retval = 
		request_irq(__gpio_to_irq(USB_ID_GPIO), s3c_hcd_power_irq, IRQ_TYPE_EDGE_BOTH, host_driver_name, NULL);

	if (retval != 0) {
		printk(KERN_ERR "%s: can't get irq %i, err %d\n", host_driver_name,
		      __gpio_to_irq(USB_ID_GPIO), retval);
	}
	printk(KERN_ERR "%s:  get irq %i, err %d\n", host_driver_name,
		      __gpio_to_irq(USB_ID_GPIO), retval);

#endif

	s5pv210_start_ehc();

	hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);
	if (!hcd->regs) {
		dev_err(&pdev->dev, "ioremap failed!\n");
		retval = -ENOMEM;
		goto err2;
	}

	ehci = hcd_to_ehci(hcd);
	ehci->caps = hcd->regs;
	ehci->regs = hcd->regs + HC_LENGTH(readl(&ehci->caps->hc_capbase));
	/* cache this readonly data; minimize chip reads */
	ehci->hcs_params = readl(&ehci->caps->hcs_params);

#if defined(CONFIG_ARCH_S5PV210) || defined(CONFIG_ARCH_S5P6450)
	writel(0x000F0000, hcd->regs + 0x90);
#endif

#if defined(CONFIG_ARCH_S5PV310)
	writel(0x03C00000, hcd->regs + 0x90);
#endif

	retval = usb_add_hcd(hcd, pdev->resource[1].start,
				IRQF_DISABLED | IRQF_SHARED);

	if (retval == 0) {
		platform_set_drvdata(pdev, hcd);
		return retval;
	}

	s5pv210_stop_ehc();
	iounmap(hcd->regs);
err2:
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
err1:
	clk_put(usb_clk);
	usb_put_hcd(hcd);
	return retval;

#if (USB_HOST_DYNAMIC)
	err_reset:
	    return err;
#endif


}

static int ehci_hcd_s5pv210_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	usb_remove_hcd(hcd);
	iounmap(hcd->regs);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	usb_put_hcd(hcd);
	s5pv210_stop_ehc();

release_power(HDMI_POWER);

release_power(CAM_2_8V);

	clk_put(usb_clk);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct platform_driver  ehci_hcd_s5pv210_driver = {
	.probe		= ehci_hcd_s5pv210_drv_probe,
	.remove		= ehci_hcd_s5pv210_drv_remove,
	.shutdown	= usb_hcd_platform_shutdown,
	.suspend	= ehci_hcd_s5pv210_drv_suspend,
	.resume		= ehci_hcd_s5pv210_drv_resume,
	.driver = {
		.name = "s5p-ehci",
		.owner = THIS_MODULE,
	}
};

MODULE_ALIAS("platform:s5p-ehci");
