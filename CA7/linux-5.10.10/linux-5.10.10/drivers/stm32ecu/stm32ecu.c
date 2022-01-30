// SPDX-License-Identifier: MIT
/*
 * Copyright (C) Jordan Esh 2021 - All Rights Reserved
 * Author: Jordan Esh <jordan.esh@monashmotorsport.com> for Monash Motorsport.
 */

#include <linux/crc32poly.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/stm32ecu/stm32ecu.h>
#include <linux/stm32ecu/CRC.h>
#include <linux/stm32ecu/Interproc_Msg.h>

#define MSG "hello world from stm32ecu!"

static struct class *dev_class;
static dev_t dev = 0;
static struct device *my_device;
static struct cdev cdev;

static int32_t value = 0;

static struct rpmsg_device *global_rproc_ref = NULL;

static int ioctl_open(struct inode *inode, struct file *file);
static int ioctl_release(struct inode *inode, struct file *file);
static ssize_t ioctl_read(struct file *filp, char __user *buf, size_t len,
			  loff_t *off);
static ssize_t ioctl_write(struct file *filp, const char *buf, size_t len,
			   loff_t *off);
static long ioctl_call(struct file *file, unsigned int cmd, unsigned long arg);

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = ioctl_read,
	.write = ioctl_write,
	.open = ioctl_open,
	.unlocked_ioctl = ioctl_call,
	.release = ioctl_release,
};

static int ioctl_open(struct inode *inode, struct file *file)
{
	printk(KERN_WARNING "Device File Opened...!!!\n");
	return 0;
}

/*
** This function will be called when we close the Device file
*/
static int ioctl_release(struct inode *inode, struct file *file)
{
	printk(KERN_WARNING "Device File Closed...!!!\n");
	return 0;
}
/*
** This function will be called when we read the Device file
*/
static ssize_t ioctl_read(struct file *filp, char __user *buf, size_t len,
			  loff_t *off)
{
	printk(KERN_WARNING "Read Function\n");
	return 0;
}
/*
** This function will be called when we write the Device file
*/
static ssize_t ioctl_write(struct file *filp, const char __user *buf,
			   size_t len, loff_t *off)
{
	printk(KERN_WARNING "Write function\n");
	return len;
}
/*
** This function will be called when we write IOCTL on the Device file
*/
static long ioctl_call(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case STM32ECU_RESET: {
		break;
	}
	case STM32ECU_OFFLINE: {
		break;
	}
	case STM32ECU_SET_STATE: {
		if (copy_from_user(&value, (int32_t *)arg, sizeof(value))) {
			printk(KERN_ERR "Data Write : Err!\n");
		}
		printk(KERN_WARNING "Value = %d\n", value);
		break;
	}
	case STM32ECU_GET_STATE: {
		if (copy_to_user((int32_t *)arg, &value, sizeof(value))) {
			printk(KERN_ERR "Data Read : Err!\n");
		}
		break;
	}
	case STM32ECU_PING_RPROC: {
		if (!global_rproc_ref || global_rproc_ref->dev.offline) {
			printk(KERN_ERR "No RPROC connected\n");
			return -EINVAL;
		}

		Interproc_Msg_t msg = { .command = PING_CMD,
					.data = { 0 },
					.checksum = 0 };

		msg.checksum =
			calc_crc(&msg, offsetof(Interproc_Msg_t, checksum));

		int ret = rpmsg_send(global_rproc_ref->ept, &msg,
				     sizeof(Interproc_Msg_t));
		if (ret) {
			dev_err(&global_rproc_ref->dev,
				"rpmsg_send failed: %d\n", ret);
			return ret;
		}

		break;
	}
	case STM32ECU_SEND_MSG: {
		Interproc_Msg_t msg = { 0 };
		if (copy_from_user(&msg, (Interproc_Msg_t *)arg,
				   sizeof(Interproc_Msg_t))) {
			printk(KERN_ERR "Data Write Msg : Err!\n");
		}

		int ret = rpmsg_send(global_rproc_ref->ept, &msg,
				     sizeof(Interproc_Msg_t));
		if (ret) {
			dev_err(&global_rproc_ref->dev,
				"rpmsg_send failed: %d\n", ret);
			return ret;
		}

		break;
	}
	default: {
		printk(KERN_WARNING "Default\n");
		break;
	}
	}
	return 0;
}

static int count = 100;
module_param(count, int, 0644);

struct instance_data {
	int rx_count;
};

static int rpmsg_sample_cb(struct rpmsg_device *rpdev, void *data, int len,
			   void *priv, u32 src)
{
	struct instance_data *idata = dev_get_drvdata(&rpdev->dev);

	dev_info(&rpdev->dev, "incoming msg %d (src: 0x%x): %s\n",
		 ++idata->rx_count, src, (char *)data);

	if (len == sizeof(Interproc_Msg_t) && data != NULL) {
		Interproc_Msg_t *msg = (Interproc_Msg_t *)data;

		CRC crc = calc_crc(msg, offsetof(Interproc_Msg_t, checksum));

		if (crc == msg->checksum) {
			printk(KERN_WARNING "CRC OK\n");
			printk(KERN_WARNING "msg->command = %u\n",
			       msg->command);
		} else {
			printk(KERN_WARNING
			       "CRC ERR - CALC (%u) != RECV (%u)\n",
			       crc, msg->checksum);
			printk(KERN_WARNING "msg->command = %u\n",
			       msg->command);
		}
	}

	print_hex_dump_debug(__func__, DUMP_PREFIX_NONE, 16, 1, data, len,
			     true);

	return 0;
}

static int rpmsg_sample_probe(struct rpmsg_device *rpdev)
{
	int ret;
	struct instance_data *idata;

	dev_info(&rpdev->dev, "new channel: 0x%x -> 0x%x!\n", rpdev->src,
		 rpdev->dst);

	idata = devm_kzalloc(&rpdev->dev, sizeof(*idata), GFP_KERNEL);
	if (!idata) {
		return -ENOMEM;
	}

	dev_set_drvdata(&rpdev->dev, idata);

	global_rproc_ref = rpdev;

	return 0;
}

static void rpmsg_sample_remove(struct rpmsg_device *rpdev)
{
	dev_info(&rpdev->dev, "rpmsg sample client driver is removed\n");
}

static struct rpmsg_device_id rpmsg_driver_sample_id_table[] = {
	{ .name = "stm32ecu" },
	{},
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_driver_sample_id_table);

static struct rpmsg_driver rpmsg_sample_client = {
	.drv.name = KBUILD_MODNAME,
	.id_table = rpmsg_driver_sample_id_table,
	.probe = rpmsg_sample_probe,
	.callback = rpmsg_sample_cb,
	.remove = rpmsg_sample_remove,
};

static int __init stm32ecu_init(void)
{
	int status;

	printk(KERN_ALERT "JORDAN IS INITIALISING STM32ECU\n");

	/*Allocating Major number*/
	if ((alloc_chrdev_region(&dev, 0, 1, "stm32ecu_dev")) < 0) {
		pr_err("Cannot allocate major number for device\n");
		return -1;
	}
	pr_info("Major = %d Minor = %d \n", MAJOR(dev), MINOR(dev));

	/*Creating cdev structure*/
	cdev_init(&cdev, &fops);

	/*Adding character device to the system*/
	if ((cdev_add(&cdev, dev, 1)) < 0) {
		pr_err("Cannot add the device to the system\n");
		goto r_class;
	}

	/*Creating struct class*/
	if ((dev_class = class_create(THIS_MODULE, "stm32ecu_class")) == NULL) {
		pr_err("Cannot create the struct class for device\n");
		goto r_class;
	}

	/*Creating device*/
	if ((my_device = device_create(dev_class, NULL, dev, NULL,
				       "stm32ecu")) == NULL) {
		pr_err("Cannot create the Device\n");
		goto r_device;
	}

	status = register_rpmsg_driver(&(rpmsg_sample_client));
	if (status < 0) {
		printk(KERN_ERR "FAILED INITIALISING STM32ECU\n");
		return -1;
	}

	pr_info("Kernel Module Inserted Successfully...\n");

	return 0;

r_device:
	class_destroy(dev_class);
r_class:
	unregister_chrdev_region(dev, 1);
	return -1;
}

static void __exit stm32ecu_exit(void)
{
	printk(KERN_ALERT "JORDAN IS EXITING STM32ECU\n");

	device_destroy(dev_class, dev);
	class_destroy(dev_class);
	unregister_chrdev_region(dev, 1);

	unregister_rpmsg_driver(&(rpmsg_sample_client));

	pr_info("Kernel Module Removed Successfully...\n");
}

module_init(stm32ecu_init);
module_exit(stm32ecu_exit);

MODULE_AUTHOR("Jordan Esh <jordan.esh@monashmotorsport.com>");
MODULE_DESCRIPTION("STM32 based ECU kernel driver for Monash Motorsport");
MODULE_LICENSE("GPL v2");
