/*
 *  isl29001.c - Linux kernel modules for ambient light sensor
 *
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/platform_device.h>

#define ISL29001_DRV_NAME	"isl29001"
#define DRIVER_VERSION		"0.0.0"

/*
 * Defines
 */

#define ISL29001_POWER_DOWN			0x8C
#define ISL29001_RESET				0x0C
#define ISL29001_CONNVERTS_DIODE1		0x00 
#define ISL29001_CONNVERTS_DIODE2		0x04
#define ISL29001_CONNVERTS_DIODE12		0x08

#define COMMAND_TEST				10000000B
#define EVENT_TYPE_LIGHT			0x3a
/*
 * Structs
 */

struct isl29001_data {
	struct i2c_client *client;
	struct work_struct work;
	struct input_dev *input;//goo
	unsigned int power_state : 1;
};

static struct timer_list isl_timer;
static int last_value = 0;//goo

static int isl29001_set_power_state(struct i2c_client *client, int state)
{
	struct isl29001_data *data = i2c_get_clientdata(client);
	int ret = 0;
	u8 powerdown = ISL29001_POWER_DOWN;
	u8 reset = ISL29001_RESET;
	struct i2c_msg send_msg0 = { client->addr, 0, 1, &powerdown };
	struct i2c_msg send_msg1 = { client->addr, 0, 1, &reset };
	if (state == 0)
		ret = i2c_transfer(client->adapter, &send_msg0, 1);
	else
		ret = i2c_transfer(client->adapter, &send_msg1, 1);
	if (ret < 0) 
	{
		return -EIO;
	}

	data->power_state = state;
	if (ret > 0)
	ret = 0;
	return ret;
}




static int isl29001_get_adc_value(struct i2c_client *client, unsigned char reg,unsigned char * buf)
{
	int ret;
	struct i2c_msg send_msg = { client->addr, 0, 1, &reg };
	struct i2c_msg recv_msg = { client->addr, I2C_M_RD, 4, buf };
 
	
	ret = i2c_transfer(client->adapter, &send_msg, 1);
	if (ret < 0) 
	{
		return -EIO;
	}
	msleep(350);

	ret = i2c_transfer(client->adapter, &recv_msg, 1);
	if (ret < 0)
	{
		
		return -EIO;
	}

	return 0 ;     	
}

static void isl_report_event(struct work_struct *work)
{
	struct isl29001_data *data =
                 container_of(work, struct isl29001_data, work);
	struct input_dev *input = data->input;
	unsigned int type = EV_ABS;
	unsigned int code = EVENT_TYPE_LIGHT;
	u8 buffer[4];
	u16 value = 0;

	isl29001_get_adc_value(data->client, ISL29001_CONNVERTS_DIODE12,buffer);

	value = buffer[0];
	value |= (buffer[1] << 8 );

	//printk("!!!!!!!!!!isl_timer_handler value = %d\n", value);
	if(last_value == value){
		mod_timer(&isl_timer, jiffies + msecs_to_jiffies(2000));
		return;}
	else{
		last_value = value;
		input_event(input, type, code, value);
		input_sync(input);
		mod_timer(&isl_timer, jiffies + msecs_to_jiffies(2000));
	}
}

/*
 * SysFS support
 */

static ssize_t isl29001_show_power_state(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct isl29001_data *data = i2c_get_clientdata(to_i2c_client(dev));

	return sprintf(buf, "%u\n", data->power_state);
}

static ssize_t isl29001_store_power_state(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);	
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int ret;
	struct isl29001_data *data = i2c_get_clientdata(client);

	if (val < 0 || val > 1)
		return -EINVAL;	
	if (val == 0){
		del_timer_sync(&isl_timer);
		cancel_work_sync(&data->work);
	}else if (val == 1){	
		INIT_WORK(&data->work, isl_report_event);
		mod_timer(&isl_timer, jiffies + msecs_to_jiffies(2000));
	}

	ret = isl29001_set_power_state(client, val);
	if (ret < 0)
		return ret;

	return count;
}

static DEVICE_ATTR(power_state, S_IWUSR | S_IRUGO,
		   isl29001_show_power_state, isl29001_store_power_state);



static ssize_t __isl29001_show_lux(struct i2c_client *client, char *buf)
{
	
	u8 buffer[4];
	u16 ret= 0;

	isl29001_get_adc_value(client, ISL29001_CONNVERTS_DIODE12,buffer);

	ret = buffer[0];
	ret |= (buffer[1] << 8 );

	return sprintf(buf, "%d\n", ret);
}

static ssize_t isl29001_show_lux1_input(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct isl29001_data *data = i2c_get_clientdata(client);
	int ret;

	/* No LUX data if not operational */
	if (!data->power_state)
		return -EBUSY;


	ret = __isl29001_show_lux(client, buf);


	return ret;
}

static DEVICE_ATTR(lux1_input, S_IRUGO,
		   isl29001_show_lux1_input, NULL);

static struct attribute *isl29001_attributes[] = {
	&dev_attr_power_state.attr,
	&dev_attr_lux1_input.attr,
	NULL
};

static const struct attribute_group isl29001_attr_group = {
	.attrs = isl29001_attributes,
};

/*
 * Initialization function
 */

static int isl29001_init_client(struct i2c_client *client)
{
	struct isl29001_data *data = i2c_get_clientdata(client);
	data->power_state = 1;
	return 0;
}

static void isl_timer_handler(unsigned long _data)
{
	struct isl29001_data *data = (struct isl29001_data *)_data;

	schedule_work(&data->work);
}

/*
 * I2C init/probing/exit functions
 */

static struct i2c_driver isl29001_driver;
static int __devinit isl29001_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct isl29001_data *data;
	struct input_dev *input;//goo

	int  err = 0;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE)) {
		err = -EIO;
		goto exit;
	}

	data = kzalloc(sizeof(struct isl29001_data), GFP_KERNEL);
	if (!data) {
		err = -ENOMEM;
		goto exit;
	}
	data->client = client;
	i2c_set_clientdata(client, data);

	input = input_allocate_device();
	if (!input) {
		err = -ENOMEM;
		goto exit;
	}

	input->name = client->name;
	input->id.bustype = BUS_HOST;
	input->dev.parent = &client->dev;

	data->input = input;
	input_register_device(input);//goo

	/* Scan timer init */
	init_timer(&isl_timer);
	isl_timer.function = isl_timer_handler;
	isl_timer.data = (unsigned long)data;

	INIT_WORK(&data->work, isl_report_event);
	mod_timer(&isl_timer, jiffies + msecs_to_jiffies(2000));
	input_set_capability(input, EV_ABS, EVENT_TYPE_LIGHT);

	/* Initialize the ISL29001 chip */
	err = isl29001_init_client(client);
	if (err)
		goto exit_kfree;

	/* Register sysfs hooks */
	
	err = sysfs_create_group(&client->dev.kobj, &isl29001_attr_group);

	if (err)
		goto exit_kfree;

	dev_info(&client->dev, "support ver. %s enabled\n", DRIVER_VERSION);

	mdelay(10);
	isl29001_set_power_state(client, 0);
	mdelay(10);
	isl29001_set_power_state(client, 1);
	return 0;

exit_kfree:
	kfree(data);
exit:
	input_free_device(input);//goo
	return err;
}

static int __devexit isl29001_remove(struct i2c_client *client)
{
	sysfs_remove_group(&client->dev.kobj, &isl29001_attr_group);

	/* Power down the device */
	isl29001_set_power_state(client, 0);

	kfree(i2c_get_clientdata(client));

	return 0;
}

#ifdef CONFIG_PM

static int isl29001_suspend(struct i2c_client *client, pm_message_t mesg)
{
	isl29001_set_power_state(client, 0);
	return 0;
}

static int isl29001_resume(struct i2c_client *client)
{
	isl29001_set_power_state(client, 1);
	return 0;
}

#else

#define isl29001_suspend		NULL
#define isl29001_resume		NULL

#endif /* CONFIG_PM */

static const struct i2c_device_id isl29001_id[] = {
	{ "isl29001", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, isl29001_id);

static struct i2c_driver isl29001_driver = {
	.driver = {
		.name	= ISL29001_DRV_NAME,
		.owner	= THIS_MODULE,
	},
	.suspend = isl29001_suspend,
	.resume	= isl29001_resume,
	.probe	= isl29001_probe,
	.remove	= __devexit_p(isl29001_remove),
	.id_table = isl29001_id,
};

static int __init isl29001_init(void)
{
	return i2c_add_driver(&isl29001_driver);
}

static void __exit isl29001_exit(void)
{
	i2c_del_driver(&isl29001_driver);
}

MODULE_AUTHOR("Rodolfo Giometti <giometti@linux.it>");
MODULE_DESCRIPTION("ISL29001 ambient light sensor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);

module_init(isl29001_init);
module_exit(isl29001_exit);
