/* linux/arch/arm/mach-s5pv210/adc.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Copyright (c) 2004 Arnaud Patard <arnaud.patard@rtp-net.org>
 * iPAQ H1940 touchscreen support
 *
 * ChangeLog
 *
 * 2004-09-05: Herbert Pötzl <herbert@13thfloor.at>
 *	- added clock (de-)allocation code
 *
 * 2005-03-06: Arnaud Patard <arnaud.patard@rtp-net.org>
 *      - h1940_ -> s3c24xx (this driver is now also used on the n30
 *        machines :P)
 *      - Debug messages are now enabled with the config option
 *        TOUCHSCREEN_S3C_DEBUG
 *      - Changed the way the value are read
 *      - Input subsystem should now work
 *      - Use ioremap and readl/writel
 *
 * 2005-03-23: Arnaud Patard <arnaud.patard@rtp-net.org>
 *      - Make use of some undocumented features of the touchscreen
 *        controller
 *
 */


#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/serio.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/io.h>
#include <linux/uaccess.h>

#include <asm/irq.h>

#include <mach/hardware.h>
#include <mach/regs-adc.h>
#include <mach/adc.h>
#include <mach/irqs.h>
#include <linux/venus/venus.h>
#include <mach/map.h>

#define ADC_MINOR 	131
#define ADC_INPUT_PIN   _IOW('S', 0x0c, unsigned long)

//#define ADC_WITH_TOUCHSCREEN		// no need ADC TOUCHSCREEN

#if 1			// add for multi convert
#define CONVERTTIMES 200
static int buff_size[CONVERTTIMES];
#endif
static struct clk	*adc_clock;

static void __iomem 	*base_addr;
static int adc_port = 3;		// channel 3 as default
struct s3c_adc_mach_info *plat_data;


#ifdef ADC_WITH_TOUCHSCREEN
static DEFINE_MUTEX(adc_mutex);

static unsigned long data_for_ADCCON;
static unsigned long data_for_ADCTSC;

static void s3c_adc_save_SFR_on_ADC(void)
{
	data_for_ADCCON = readl(base_addr + S3C_ADCCON);
	data_for_ADCTSC = readl(base_addr + S3C_ADCTSC);
}

static void s3c_adc_restore_SFR_on_ADC(void)
{
	writel(data_for_ADCCON, base_addr + S3C_ADCCON);
	writel(data_for_ADCTSC, base_addr + S3C_ADCTSC);
}
#else
static struct resource	*adc_mem;
#endif

static int s3c_adc_open(struct inode *inode, struct file *file)
{
	return 0;
}

unsigned int s3c_adc_convert(void)
{
	unsigned int adc_return = 0;
	unsigned long data0;
	unsigned long data1;

	writel((adc_port & 0x7), base_addr + S3C_ADCMUX);

	udelay(10);

	writel(readl(base_addr + S3C_ADCCON) | S3C_ADCCON_ENABLE_START, base_addr + S3C_ADCCON);

	do {
		data0 = readl(base_addr + S3C_ADCCON);
	} while (!(data0 & S3C_ADCCON_ECFLG));

	data1 = readl(base_addr + S3C_ADCDAT0);

//	if (plat_data->resolution == 12)
	adc_return = data1 & S3C_ADCDAT0_XPDATA_MASK_12BIT;
//	else
//		adc_return = data1 & S3C_ADCDAT0_XPDATA_MASK;

	return adc_return;
}

#if 1 // add a 100 detect method 
void s3c_adc_get_data(void)
{		
		int i= 0;	
		while(i < CONVERTTIMES ){
		buff_size[i]= s3c_adc_convert();
		i++;
		}
}

int s3c_adc_data_select(void)
{	
	int i =0;
	int t =0;
	int adc_data = 0;
	int sum = 0;
	
	i = 0;
	while(t < CONVERTTIMES){
		while(i < (CONVERTTIMES - 1)){
			if(buff_size[i] < buff_size[i+1]){
				adc_data = buff_size[i];
				buff_size[i] = buff_size[i+1];
				buff_size[i+1] = adc_data;
			}
			
			i++;
		}
		i = 0;
		t++;
	}

	i = 80;

	while(i < (CONVERTTIMES -20)){
		sum += buff_size[i] ;
		i++;
	}
	sum = sum/(CONVERTTIMES - 100);
//	printk(KERN_INFO "s3c_adc_select() num is%d\n",sum);
	return sum;

}

int adc_vol_calculator(int adc_convert_data)
{
	int adc_vol = 0;
	adc_vol = (adc_convert_data * 3312) / 4095 ;
	return adc_vol;
}

int battery_vol_calculator(int adc_vol)
{
	int battery_vol = 0;
	int r1,r2;
//	r1 = 100;
	r1 = 0;
	r2 = 47;
	battery_vol = (adc_vol * (r1 + r2) ) / r2 ;
	return battery_vol;
}

int s3c_adc_convert_multi(void)
{
	int adc_data = 0;
	int battery_voltage = 0;
	int adc_convert_vol = 0;
	int battery_convert_vol = 0;
	s3c_adc_get_data();
	adc_data= s3c_adc_data_select();
	adc_convert_vol= adc_vol_calculator(adc_data);
	battery_convert_vol= battery_vol_calculator(adc_convert_vol);
	battery_voltage = battery_convert_vol ;
//	printk("voltage is %d\n",battery_voltage);
	return battery_voltage;
}

int get_hw_version(void)
{
		int board_version = 0;
		int battery_voltage_v = 0;
		
		battery_voltage_v = s3c_adc_convert_multi();
		
		if (battery_voltage_v >= 3000)
		{
			board_version = EVT2_BOARD;
		}
		else if ( battery_voltage_v <= 1000)
		{
       		 	board_version = EVT3_BOARD;
    		}
	 
#ifdef CONFIG_PRODUCT_PDC3_EVT2
        board_version = EVT2_BOARD;
        printk("board_version is EVT2_BOARD.\n");
#endif

#ifdef CONFIG_PRODUCT_PDC3_DVT2
        board_version = DVT2_BOARD;
        printk("board_version is DVT2_BOARD.\n");
#endif	 

#ifdef CONFIG_PRODUCT_V7E
        board_version = MP_BOARD;
        printk("board_version is MP_BOARD.\n");
#endif	
		return board_version;
	
}
EXPORT_SYMBOL(get_hw_version);

int get_hw_version_before_init(void)
{
	int size;

	size = SZ_4K;
	base_addr = ioremap(S3C_PA_ADC, size);
	if (base_addr ==  NULL) {
		printk("fail to ioremap() region\n");
	}

	adc_clock = clk_get(NULL, "adc");

	if (IS_ERR(adc_clock)) {
		printk("failed to fine ADC clock source\n");
	}

	clk_enable(adc_clock);

	writel(S3C_ADCCON_PRSCEN | S3C_ADCCON_PRSCVL(49 & 0xff), base_addr + S3C_ADCCON);  		// set presce to 49;
	writel(readl(base_addr + S3C_ADCCON) | S3C_ADCCON_RESSEL_12BIT, base_addr + S3C_ADCCON);	// set reslution to 12;
	
	return get_hw_version();
}
EXPORT_SYMBOL(get_hw_version_before_init);

#endif

int s3c_adc_get(struct s3c_adc_request *req)
{
	unsigned adc_channel = req->channel;
	int adc_value_ret = 0;

	adc_value_ret = s3c_adc_convert();

	req->callback(adc_channel, req->param, adc_value_ret);

	return 0;
}
EXPORT_SYMBOL(s3c_adc_get);

static ssize_t
s3c_adc_read(struct file *file, char __user *buffer,
		size_t size, loff_t *pos)
{
//	int  adc_value = 0;
	int adc_vol = 0;	
	
#ifdef ADC_WITH_TOUCHSCREEN
	mutex_lock(&adc_mutex);
	s3c_adc_save_SFR_on_ADC();
#endif
//	printk(KERN_INFO "## delay: %d\n", readl(base_addr + S3C_ADCDLY));
//	adc_value = s3c_adc_convert();

#ifdef ADC_WITH_TOUCHSCREEN
	s3c_adc_restore_SFR_on_ADC();
	mutex_unlock(&adc_mutex);
#endif
	
	adc_vol = s3c_adc_convert_multi();
	
	printk( "adc voltage is %d\n", adc_vol);

	if (copy_to_user(buffer, &adc_vol, sizeof(unsigned int)))
		return -EFAULT;


	return 0;
}


static int s3c_adc_ioctl(struct inode *inode, struct file *file,
	unsigned int cmd, unsigned long arg)
{

	switch (cmd) {
	case ADC_INPUT_PIN:
		adc_port = (unsigned int) arg;

		if (adc_port >= 4)
			printk(KERN_WARNING
				" %d is already reserved for TouchScreen\n",
				adc_port);
		return 0;

	default:
		return -ENOIOCTLCMD;
	}
}

static const struct file_operations s3c_adc_fops = {
	.owner		= THIS_MODULE,
	.read		= s3c_adc_read,
	.open		= s3c_adc_open,
	.ioctl		= s3c_adc_ioctl,
};

static struct miscdevice s3c_adc_miscdev = {
	.minor		= ADC_MINOR,
	.name		= "adc",
	.fops		= &s3c_adc_fops,
};

static struct s3c_adc_mach_info *s3c_adc_get_platdata(struct device *dev)
{
	if (dev->platform_data != NULL)
		return (struct s3c_adc_mach_info *) dev->platform_data;
	else
		return 0;
}

/*
 * The functions for inserting/removing us as a module.
 */

static int __init s3c_adc_probe(struct platform_device *pdev)
{
	struct resource	*res;
	struct device *dev;
	int ret;
	int size;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dev = &pdev->dev;

	if (res == NULL) {
		dev_err(dev, "no memory resource specified\n");
		return -ENOENT;
	}

	size = (res->end - res->start) + 1;

#if !defined(ADC_WITH_TOUCHSCREEN)
	adc_mem = request_mem_region(res->start, size, pdev->name);
	if (adc_mem == NULL) {
		dev_err(dev, "failed to get memory region\n");
		ret = -ENOENT;
		goto err_req;
	}
#endif

	base_addr = ioremap(res->start, size);
	if (base_addr ==  NULL) {
		dev_err(dev, "fail to ioremap() region\n");
		ret = -ENOENT;
		goto err_map;
	}

	adc_clock = clk_get(&pdev->dev, "adc");

	if (IS_ERR(adc_clock)) {
		dev_err(dev, "failed to fine ADC clock source\n");
		ret = PTR_ERR(adc_clock);
		goto err_clk;
	}

	clk_enable(adc_clock);

	/* read platform data from device struct */
	plat_data = s3c_adc_get_platdata(&pdev->dev);

	if ((plat_data->presc & 0xff) > 0)
		writel(S3C_ADCCON_PRSCEN | S3C_ADCCON_PRSCVL(plat_data->presc & 0xff), base_addr + S3C_ADCCON);
	else
		writel(0, base_addr + S3C_ADCCON);

	/* Initialise registers */
	if ((plat_data->delay & 0xffff) > 0)
		writel(plat_data->delay & 0xffff, base_addr + S3C_ADCDLY);

	if (plat_data->resolution == 12)
		writel(readl(base_addr + S3C_ADCCON) | S3C_ADCCON_RESSEL_12BIT, base_addr + S3C_ADCCON);

	ret = misc_register(&s3c_adc_miscdev);
	if (ret) {
		printk(KERN_ERR "cannot register miscdev on minor=%d (%d)\n",
			ADC_MINOR, ret);
		goto err_clk;
	}

	return 0;

err_clk:
	clk_disable(adc_clock);
	clk_put(adc_clock);

err_map:
	iounmap(base_addr);

#if !defined(ADC_WITH_TOUCHSCREEN)
err_req:
	release_resource(adc_mem);
	kfree(adc_mem);
#endif

	return ret;
}


static int s3c_adc_remove(struct platform_device *dev)
{
	return 0;
}

#ifdef CONFIG_PM
static unsigned int adccon, adctsc, adcdly;

static int s3c_adc_suspend(struct platform_device *dev, pm_message_t state)
{
	adccon = readl(base_addr + S3C_ADCCON);
	adctsc = readl(base_addr + S3C_ADCTSC);
	adcdly = readl(base_addr + S3C_ADCDLY);

	clk_disable(adc_clock);

	return 0;
}

static int s3c_adc_resume(struct platform_device *pdev)
{
	clk_enable(adc_clock);

	writel(adccon, base_addr + S3C_ADCCON);
	writel(adctsc, base_addr + S3C_ADCTSC);
	writel(adcdly, base_addr + S3C_ADCDLY);

	return 0;
}
#else
#define s3c_adc_suspend NULL
#define s3c_adc_resume  NULL
#endif

static struct platform_driver s3c_adc_driver = {
       .probe          = s3c_adc_probe,
       .remove         = s3c_adc_remove,
       .suspend        = s3c_adc_suspend,
       .resume         = s3c_adc_resume,
       .driver		= {
		.owner	= THIS_MODULE,
		.name	= "s3c-adc",
	},
};

static char banner[] __initdata = KERN_INFO "S5PV210 ADC driver, (c) 2010 Samsung Electronics\n";

int __init s3c_adc_init(void)
{
	printk(banner);
	return platform_driver_register(&s3c_adc_driver);
}

void __exit s3c_adc_exit(void)
{
	platform_driver_unregister(&s3c_adc_driver);
}

module_init(s3c_adc_init);
module_exit(s3c_adc_exit);

MODULE_AUTHOR("dsfine.ha@samsung.com");
MODULE_DESCRIPTION("S5PV210 ADC driver");
MODULE_LICENSE("GPL");
