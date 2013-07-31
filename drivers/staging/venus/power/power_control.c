/* 
 * linux/arch/arm/mach-s5pv210/$(VENUS_PROJECT_DIR)/power_control.c
 * 
 * Copyright (C) 2010, TMSBG L.H. SW-BSP1
 *
 * Copyright (C) 2011, Venus Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <mach/regs-gpio.h>
#include <mach/gpio.h>
//#include <mach/gpio-bank.h>
#include <plat/gpio-cfg.h>
#include <linux/string.h>
#include <linux/gpio.h>
#include <linux/mutex.h>
#include <linux/venus/power_control.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>

static DEFINE_MUTEX(power_control_mutex);
struct sys_total_power_module  *system_power_mod;

struct regulator {
	struct device *dev;
	struct list_head list;
	int uA_load;
	int min_uV;
	int max_uV;
	char *supply_name;
	struct device_attribute dev_attr;
	struct regulator_dev *rdev;
};
/*
 * power control by gpio
 */
static int gpio_power_enable(int level)
{
	int err = 0;

	if (gpio_is_valid(system_power_mod->modules[level].power_contrl_gpio)) {
			err = gpio_request(system_power_mod->modules[level].power_contrl_gpio, "power");

			if (err) {
				printk(KERN_ERR "failed to request power enable for "
					"power control on\n");
				err = -EINVAL;
			}
			gpio_direction_output(system_power_mod->modules[level].power_contrl_gpio, 
								system_power_mod->modules[level].active);
		}
	gpio_free(system_power_mod->modules[level].power_contrl_gpio);
	
	return err;
}	

static int gpio_power_disable(int level)
{
	int err = 0;

	if (gpio_is_valid(system_power_mod->modules[level].power_contrl_gpio)) {
			err = gpio_request(system_power_mod->modules[level].power_contrl_gpio, "power");

			if (err) {
				printk(KERN_ERR "failed to request power disable for "
					"power control off\n");
				err = -EINVAL;
			}
			gpio_direction_output(system_power_mod->modules[level].power_contrl_gpio, 
								!system_power_mod->modules[level].active);
		}
	gpio_free(system_power_mod->modules[level].power_contrl_gpio);
	
	return err;
}	

/*
 * max8698 ldo and buck , control by regulator
 */
static int pmic_power_enable(char *s)
{
	int err = 0;
        struct regulator *module_regulator;
	
	module_regulator = regulator_get(NULL, s);
	if (IS_ERR(module_regulator)) {
		printk(KERN_ERR "failed to get resource %s\n", s);
		return PTR_ERR(module_regulator);
	}
	err = regulator_enable(module_regulator);

	return err;
}

static int pmic_power_disable(char *s)
{
	int err = 0;
	struct regulator *module_regulator;

	module_regulator = regulator_get(NULL, s);
	if (IS_ERR(module_regulator)) {
		printk(KERN_ERR "failed to get resource %s\n", s);
		return PTR_ERR(module_regulator);
	}
	err = regulator_disable(module_regulator);
	
	return err;
}

int request_power(char *power_name)
{
	const char *s;
	int level = 0;
	int err = 0;
	mutex_lock(&power_control_mutex);
	for (; level < system_power_mod->num_power_module; level++)							
	{
		s = system_power_mod->modules[level].power_name;

		if (strcmp(power_name, s) == 0) 
		{
			system_power_mod->modules[level].refer_count++;

			if (system_power_mod->modules[level].type == NORMAL_SUPPLY)
			{
				err = gpio_power_enable(level);
			}
			else if (system_power_mod->modules[level].type == PMIC_SUPPLY)
			{
				err = pmic_power_enable(system_power_mod->modules[level].power_name);
			}
		
			break;						
		}
	
	}
	mutex_unlock(&power_control_mutex);
	return err;
}
EXPORT_SYMBOL_GPL(request_power);

int release_power(char *power_name)
{
	const char *s;
	int level = 0;
	int err = 0;

	mutex_lock(&power_control_mutex);
	for (; level < system_power_mod->num_power_module; level++)
	{
		s = system_power_mod->modules[level].power_name;

		if (strcmp(power_name, s) == 0) 
		{	
			if (system_power_mod->modules[level].type == NORMAL_SUPPLY)
			{
			
				if (system_power_mod->modules[level].always_on == 1)
					printk(" The %s is always on power ,can't be release !\n",s);					
				else 
				{	
					if (system_power_mod->modules[level].refer_count > 0)
					{
						system_power_mod->modules[level].refer_count--;
						// printk("Now power module use count is %d\n",
						//			system_power_mod->modules[level].refer_count);
					}

					if (system_power_mod->modules[level].refer_count == 0)
					{
						err = gpio_power_disable(level);
						if (!err)
						;
						//	printk("The %s is power off !\n",s);
						
					}

				}


			}
			else if (system_power_mod->modules[level].type == PMIC_SUPPLY)
			{
				err = pmic_power_disable(system_power_mod->modules[level].power_name);		
			}

			break;

		}
					
	}

	mutex_unlock(&power_control_mutex);
	return err;
}
EXPORT_SYMBOL_GPL(release_power);

static int __init power_regulator_init_complete(void)
{
	const char *s;
	int level = 0;
	int err = 0;
	int count = 1;
	struct regulator *module_regulator;

	for (; level < system_power_mod->num_power_module; level++)
	{	
	
		s = system_power_mod->modules[level].power_name;
		module_regulator = regulator_get(NULL, s);

		if (!IS_ERR(module_regulator)) 
		{
			count = module_regulator->rdev->use_count;
			system_power_mod->modules[level].refer_count = count;
			printk("The %s power module use count is %d\n",
								s,system_power_mod->modules[level].refer_count);
		}
	
		if (system_power_mod->modules[level].always_on == 0 && 
						system_power_mod->modules[level].refer_count == 0 && 
						system_power_mod->modules[level].type == NORMAL_SUPPLY)
		{
			err = release_power(system_power_mod->modules[level].power_name);
		}
	}
	
	printk("Finished power regulator init !\n");
	return err;
}
//late_initcall_sync(power_regulator_init_complete);

static int __devinit power_control_probe(struct platform_device *pdev)
{
	system_power_mod = pdev->dev.platform_data;

	return 0;
}

static int __devexit power_control_remove(struct platform_device *pdev)
{
//	struct sys_total_power_module *pdata = pdev->dev.platform_data;
	return 0;
}


#ifdef CONFIG_PM
static int power_control_suspend(struct device *dev)
{
//	struct platform_device *pdev = to_platform_device(dev);
	return 0;
}

static int power_control_resume(struct device *dev)
{
//	struct platform_device *pdev = to_platform_device(dev);
	return 0;
}

static const struct dev_pm_ops power_control_pm_ops = {
	.suspend	= power_control_suspend,
	.resume		= power_control_resume,
};
#endif

static struct platform_driver power_control_device_driver = {
	.probe		= power_control_probe,
	.remove		= __devexit_p(power_control_remove),
	.driver		= {
		.name	= "sys-power-control",
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
		.pm	= &power_control_pm_ops,
#endif
	}
};

static int __init power_control_init(void)
{
	printk("system power control interface init\n");
	return platform_driver_register(&power_control_device_driver);
}

static void __exit power_control_exit(void)
{
	platform_driver_unregister(&power_control_device_driver);
}

core_initcall(power_control_init);
module_exit(power_control_exit);

MODULE_DESCRIPTION("Universal power control interface");
MODULE_AUTHOR("jimmyteng");
MODULE_LICENSE("GPL");


