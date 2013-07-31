/*
 * Driver for keys on GPIO lines capable of generating interrupts.
 *
 * Copyright 2005 Phil Blundell
 *
 * Copyright 2009, 2010, L.H. SW-BSP01
 *
 * Copyright 2011, Venus Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/venus/gpio_keys.h>
#include <linux/workqueue.h>

#include <asm/gpio.h>

#define GPIOKEY_STATE_IDLE		0
#define GPIOKEY_STATE_DEBOUNCE		1
#define GPIOKEY_STATE_PRESSED		2

struct gpio_button_data {
	struct gpio_keys_button *button;
	struct input_dev *input;
	struct timer_list timer;
	struct work_struct work;
	int pressed_time;
	int gk_state;
	struct device_attribute key_status;
};

struct gpio_keys_drvdata {
	struct input_dev *input;
	struct gpio_button_data data[0];
};

static void gpio_keys_sw_report_event(struct work_struct *work)
{
	struct gpio_button_data *bdata =
		container_of(work, struct gpio_button_data, work);
	struct gpio_keys_button *button = bdata->button;
	struct input_dev *input = bdata->input;
	unsigned int type = button->type ? : EV_SW;
	int state = (gpio_get_value(button->gpio) ? 1 : 0) ^ button->active_low;

	input_event(input, type, button->code, !!state);
	input_sync(input);
	mod_timer(&bdata->timer, jiffies + msecs_to_jiffies(button->timer_interval));
}

static void gpio_keys_key_report_event(struct work_struct *work)
{
	struct gpio_button_data *bdata =
		container_of(work, struct gpio_button_data, work);
	struct gpio_keys_button *button = bdata->button;
	struct input_dev *input = bdata->input;
	unsigned int type = button->type ?: EV_KEY;
	int state = (gpio_get_value(button->gpio) ? 1 : 0) ^ button->active_low;

	switch (bdata->gk_state)
	{
	case GPIOKEY_STATE_IDLE:
		break;

	case GPIOKEY_STATE_DEBOUNCE:
		if (state)
		{
			input_event(input, type, button->code, !!state);
			input_sync(input);
			bdata->pressed_time = 0;
			bdata->gk_state = GPIOKEY_STATE_PRESSED;
			mod_timer(&bdata->timer, jiffies + msecs_to_jiffies(button->timer_interval));
		}
		else
		{
//			enable_irq(gpio_to_irq(button->gpio));
			bdata->gk_state = GPIOKEY_STATE_IDLE;
			
		}
		break;

	case GPIOKEY_STATE_PRESSED:
		if (state)
		{
			if (bdata->pressed_time >= button->longpress_time)
			{
				input_event(input, type, button->code, 2);
				input_sync(input);
				bdata->pressed_time = 0;
			}
			else
			{
				bdata->pressed_time += button->timer_interval;
			}
			mod_timer(&bdata->timer, jiffies + msecs_to_jiffies(button->timer_interval));
		}
		else
		{
			input_event(input, type, button->code, !!state);
			input_sync(input);
			
			bdata->pressed_time = 0;
//			enable_irq(gpio_to_irq(button->gpio));
			bdata->gk_state = GPIOKEY_STATE_IDLE;
		}
		break;

	default:
		break;
	}
}

static void gpio_keys_timer(unsigned long _data)
{
	struct gpio_button_data *data = (struct gpio_button_data *)_data;

	schedule_work(&data->work);
}

static irqreturn_t gpio_keys_isr(int irq, void *dev_id)
{
	struct gpio_button_data *bdata = dev_id;
	struct gpio_keys_button *button = bdata->button;

	BUG_ON(irq != gpio_to_irq(button->gpio));

	if (button->debounce_interval)
	{
		if (gpio_get_value(button->gpio))
		{
			return IRQ_HANDLED;
		}

		bdata->gk_state = GPIOKEY_STATE_DEBOUNCE;
//		disable_irq(irq);
		mod_timer(&bdata->timer,
			jiffies + msecs_to_jiffies(button->debounce_interval));
	}
	else
		schedule_work(&bdata->work);

	return IRQ_HANDLED;
}

static ssize_t gpio_keys_show_status(struct device *dev, struct device_attribute *attr, char *buffer)
{
	struct gpio_keys_button *button = container_of(attr->attr.name, struct gpio_keys_button, key_status[0]);
	return snprintf(buffer, PAGE_SIZE, "%d\n", !!((gpio_get_value(button->gpio) ? 1 : 0) ^ button->active_low));
}

static int __devinit gpio_keys_probe(struct platform_device *pdev)
{
	struct gpio_keys_platform_data *pdata = pdev->dev.platform_data;
	struct gpio_keys_drvdata *ddata;
	struct input_dev *input;
	int i, error;
	int wakeup = 0;
	int ret = 0;
	
	if (pdata->init)
	{
		pdata->init();
	}

	ddata = kzalloc(sizeof(struct gpio_keys_drvdata) +
			pdata->nbuttons * sizeof(struct gpio_button_data),
			GFP_KERNEL);
	input = input_allocate_device();
	if (!ddata || !input) {
		error = -ENOMEM;
		goto fail1;
	}

	platform_set_drvdata(pdev, ddata);

	input->name = pdev->name;
	input->phys = "gpio-keys/input0";
	input->dev.parent = &pdev->dev;

	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0100;

	/* Enable auto repeat feature of Linux input subsystem */
	if (pdata->rep)
		__set_bit(EV_REP, input->evbit);

	ddata->input = input;

	for (i = 0; i < pdata->nbuttons; i++) {
		struct gpio_keys_button *button = &pdata->buttons[i];
		struct gpio_button_data *bdata = &ddata->data[i];
		int irq;
		unsigned int type = button->type ?: EV_KEY;

		bdata->input = input;
		bdata->button = button;
		bdata->gk_state = GPIOKEY_STATE_IDLE;

		if (button->key_status != NULL)
		{
			bdata->key_status.attr.name = button->key_status;
			bdata->key_status.attr.owner = THIS_MODULE;
			bdata->key_status.attr.mode = 0444;
			bdata->key_status.show = gpio_keys_show_status;
			bdata->key_status.store = NULL;
		}
		ret = device_create_file(&(pdev->dev), &(bdata->key_status));
		if (ret < 0)
			printk(KERN_WARNING "gpio-keys: failed to add entries for %s\n", 
				button->key_status);

		setup_timer(&bdata->timer,
			    gpio_keys_timer, (unsigned long)bdata);
		if (button->type == EV_SW)
		{
			INIT_WORK(&bdata->work, gpio_keys_sw_report_event);
			mod_timer(&bdata->timer, jiffies + msecs_to_jiffies(200));
		}else{
			INIT_WORK(&bdata->work, gpio_keys_key_report_event);
		}

		error = gpio_request(button->gpio, button->desc ?: "gpio_keys");
		if (error < 0) {
			pr_err("gpio-keys: failed to request GPIO %d,"
				" error %d\n", button->gpio, error);
			goto fail2;
		}

		error = gpio_direction_input(button->gpio);
		if (error < 0) {
			pr_err("gpio-keys: failed to configure input"
				" direction for GPIO %d, error %d\n",
				button->gpio, error);
			gpio_free(button->gpio);
			goto fail2;
		}

		if (button->type != EV_SW)
		{
			irq = gpio_to_irq(button->gpio);
			if (irq < 0) {
				error = irq;
				pr_err("gpio-keys: Unable to get irq number"
					" for GPIO %d, error %d\n",
					button->gpio, error);
				gpio_free(button->gpio);
				goto fail2;
			}

			error = request_irq(irq, gpio_keys_isr,
					    IRQF_SHARED |
					    IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
					//	IRQ_DISABLED,
					    button->desc ? button->desc : "gpio_keys",
					    bdata);
			if (error) {
				pr_err("gpio-keys: Unable to claim irq %d; error %d\n",
					irq, error);
				gpio_free(button->gpio);
				goto fail2;
			}
		}

		if (button->wakeup)
			wakeup = 1;

		input_set_capability(input, type, button->code);
	}

	error = input_register_device(input);
	if (error) {
		pr_err("gpio-keys: Unable to register input device, "
			"error: %d\n", error);
		goto fail2;
	}

	device_init_wakeup(&pdev->dev, wakeup);

	return 0;

 fail2:
	while (--i >= 0) {
		free_irq(gpio_to_irq(pdata->buttons[i].gpio), &ddata->data[i]);
		if (pdata->buttons[i].debounce_interval)
			del_timer_sync(&ddata->data[i].timer);
		cancel_work_sync(&ddata->data[i].work);
		gpio_free(pdata->buttons[i].gpio);
	}

	platform_set_drvdata(pdev, NULL);
 fail1:
	input_free_device(input);
	kfree(ddata);

	return error;
}

static int __devexit gpio_keys_remove(struct platform_device *pdev)
{
	struct gpio_keys_platform_data *pdata = pdev->dev.platform_data;
	struct gpio_keys_drvdata *ddata = platform_get_drvdata(pdev);
	struct input_dev *input = ddata->input;
	int i;

	device_init_wakeup(&pdev->dev, 0);

	for (i = 0; i < pdata->nbuttons; i++) {
		int irq = gpio_to_irq(pdata->buttons[i].gpio);
		free_irq(irq, &ddata->data[i]);
		if (pdata->buttons[i].debounce_interval)
			del_timer_sync(&ddata->data[i].timer);
		cancel_work_sync(&ddata->data[i].work);
		gpio_free(pdata->buttons[i].gpio);
	}

	input_unregister_device(input);

	return 0;
}


#ifdef CONFIG_PM
static int gpio_keys_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct gpio_keys_platform_data *pdata = pdev->dev.platform_data;
	int i;

	if (device_may_wakeup(&pdev->dev)) {
		for (i = 0; i < pdata->nbuttons; i++) {
			struct gpio_keys_button *button = &pdata->buttons[i];
			if (button->wakeup) {
				int irq = gpio_to_irq(button->gpio);
				enable_irq_wake(irq);
				disable_irq(irq);
			}
		}
	}

	return 0;
}

static int gpio_keys_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct gpio_keys_platform_data *pdata = pdev->dev.platform_data;
	int i;

	if (device_may_wakeup(&pdev->dev)) {
		for (i = 0; i < pdata->nbuttons; i++) {
			struct gpio_keys_button *button = &pdata->buttons[i];
			if (button->wakeup) {
				int irq = gpio_to_irq(button->gpio);
				disable_irq_wake(irq);
				enable_irq(irq);
			}
		}
	}

	/* FIXME: For more portable, following code should be modified */
	{
		int wakeup_state;
		int eintpend_0,eintpend_1;

		wakeup_state = __raw_readl(S5P_WAKEUP_STAT);
		//eintpend_0 = __raw_readl(S5P_EINTPEND(0));
		//eintpend_1 = __raw_readl(S5P_EINTPEND(1));

		if ((wakeup_state & 0x1) || (wakeup_state & 0x2))
		{
			input_event(input, EV_KEY, 0x1, 1);
			input_event(input, EV_KEY, 0x1, 0);
		}
	}

	return 0;
}

static const struct dev_pm_ops gpio_keys_pm_ops = {
	.suspend	= gpio_keys_suspend,
	.resume		= gpio_keys_resume,
};
#endif

static struct platform_driver gpio_keys_device_driver = {
	.probe		= gpio_keys_probe,
	.remove		= __devexit_p(gpio_keys_remove),
	.driver		= {
		.name	= "gpio-keys",
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
		.pm	= &gpio_keys_pm_ops,
#endif
	}
};

static int __init gpio_keys_init(void)
{
	return platform_driver_register(&gpio_keys_device_driver);
}

static void __exit gpio_keys_exit(void)
{
	platform_driver_unregister(&gpio_keys_device_driver);
}

module_init(gpio_keys_init);
module_exit(gpio_keys_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Phil Blundell <pb@handhelds.org>");
MODULE_DESCRIPTION("Keyboard driver for CPU GPIOs");
MODULE_ALIAS("platform:gpio-keys");
