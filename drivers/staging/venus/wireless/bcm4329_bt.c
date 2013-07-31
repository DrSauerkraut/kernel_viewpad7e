/*
 * Bluetooth built-in chip control
 *
 * Copyright (c) 2008 Dmitry Baryshkov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/rfkill.h>
#include <linux/venus/bcm4329_bt.h>
#include <plat/gpio-cfg.h>
//#include <mach/gpio-bank.h>
#include <mach/regs-gpio.h>
#include <mach/gpio.h>

void require_module(void);
void release_module(void);

static void bcm4329_bt_on(struct bcm4329_bt_data *data)
{
	     unsigned int gpio;
		for (gpio = S5PV210_GPA0(0); gpio <= S5PV210_GPA0(3); gpio++) {
//			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
		}
	      require_module();
#if 1
	gpio_set_value(data->gpio_reset, 1);
//	gpio_set_value(data->gpio_pwr, 1);

	gpio_set_value(data->gpio_reset, 0);
	mdelay(20);
	gpio_set_value(data->gpio_reset, 1);
       // pr_info("BCM4329_BT: power  ON>>>>>>>>>>allen\n");
#endif
}

static void bcm4329_bt_off(struct bcm4329_bt_data *data)
{
#if 0
	gpio_set_value(data->gpio_reset, 0);
	mdelay(10);
//	gpio_set_value(data->gpio_pwr, 0);
	gpio_set_value(data->gpio_reset, 1);
	gpio_set_value(data->gpio_reset, 0);
        pr_info("BCM4329_BT: power  OFF\n");;
#endif
   	      release_module();
}

static int bcm4329_bt_set_block(void *data, bool blocked)
{
	// pr_info("BT_RADIO going: %s\n", blocked ? "off" : "on");

	if (!blocked) {
		
		bcm4329_bt_on(data);
	} else {
		
		bcm4329_bt_off(data);
	}

	return 0;
}

static const struct rfkill_ops bcm4329_bt_rfkill_ops = {
	.set_block = bcm4329_bt_set_block,
};

static int bcm4329_bt_probe(struct platform_device *dev)
{
	int rc;
	struct rfkill *rfk;

	struct bcm4329_bt_data *data = dev->dev.platform_data;

	rc = gpio_request(data->gpio_reset, "Bluetooth reset");
	if (rc)
		goto err_reset;
	rc = gpio_direction_output(data->gpio_reset, 1);
	if (rc)
		goto err_reset_dir;
	rc = gpio_request(data->gpio_pwr, "Bluetooth power");
	if (rc)
		goto err_pwr;
	rc = gpio_direction_output(data->gpio_pwr, 0);
	if (rc)
		goto err_pwr_dir;

	rfk = rfkill_alloc("bcm4329-bt", &dev->dev, RFKILL_TYPE_BLUETOOTH,
			   &bcm4329_bt_rfkill_ops, data);
	if (!rfk) {
		rc = -ENOMEM;
		goto err_rfk_alloc;
	}

	rfkill_set_led_trigger_name(rfk, "bcm4329-bt");

	rc = rfkill_register(rfk);
	if (rc)
		goto err_rfkill;

	platform_set_drvdata(dev, rfk);

	return 0;

err_rfkill:
	rfkill_destroy(rfk);
err_rfk_alloc:
	bcm4329_bt_off(data);
err_pwr_dir:
	gpio_free(data->gpio_pwr);
err_pwr:
err_reset_dir:
	gpio_free(data->gpio_reset);
err_reset:
	return rc;
}

static int __devexit bcm4329_bt_remove(struct platform_device *dev)
{
	struct bcm4329_bt_data *data = dev->dev.platform_data;
	struct rfkill *rfk = platform_get_drvdata(dev);

	platform_set_drvdata(dev, NULL);

	if (rfk) {
		rfkill_unregister(rfk);
		rfkill_destroy(rfk);
	}
	rfk = NULL;

	bcm4329_bt_off(data);

	gpio_free(data->gpio_pwr);
	gpio_free(data->gpio_reset);

	return 0;
}

static struct platform_driver bcm4329_bt_driver = {
	.probe = bcm4329_bt_probe,
	.remove = __devexit_p(bcm4329_bt_remove),

	.driver = {
		.name = "bcm4329-bt",
		.owner = THIS_MODULE,
	},
};


static int __init bcm4329_bt_init(void)
{
	return platform_driver_register(&bcm4329_bt_driver);
}

static void __exit bcm4329_bt_exit(void)
{
	platform_driver_unregister(&bcm4329_bt_driver);
}

module_init(bcm4329_bt_init);
module_exit(bcm4329_bt_exit);
