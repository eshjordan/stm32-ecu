// SPDX-License-Identifier: MIT
/*
 * Copyright (C) Jordan Esh 2021 - All Rights Reserved
 * Author: Jordan Esh <jordan.esh@monashmotorsport.com> for Monash Motorsport.
 */

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/uaccess.h>
#include <linux/stm32ecu/stm32ecu.h>

#define MSG "hello world from stm32ecu!"

#define WR_VALUE _IOW('a','a',int32_t*)
#define RD_VALUE _IOR('a','b',int32_t*)

int32_t value = 0;


static int etx_open(struct inode *inode, struct file *file);
static int etx_release(struct inode *inode, struct file *file);
static ssize_t etx_read(struct file *filp, char __user *buf, size_t len,
			loff_t *off);
static ssize_t etx_write(struct file *filp, const char *buf, size_t len,
			 loff_t *off);
static long etx_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = etx_read,
	.write = etx_write,
	.open = etx_open,
	.unlocked_ioctl = etx_ioctl,
	.release = etx_release,
};

static int etx_open(struct inode *inode, struct file *file)
{
	printk(KERN_WARNING "Device File Opened...!!!\n");
	return 0;
}


/*
** This function will be called when we close the Device file
*/
static int etx_release(struct inode *inode, struct file *file)
{
	printk(KERN_WARNING "Device File Closed...!!!\n");
	return 0;
}
/*
** This function will be called when we read the Device file
*/
static ssize_t etx_read(struct file *filp, char __user *buf, size_t len,
			loff_t *off)
{
	printk(KERN_WARNING "Read Function\n");
	return 0;
}
/*
** This function will be called when we write the Device file
*/
static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len,
			 loff_t *off)
{
	printk(KERN_WARNING, "Write function\n");
	return len;
}
/*
** This function will be called when we write IOCTL on the Device file
*/
static long etx_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case WR_VALUE:
		if (copy_from_user(&value, (int32_t *)arg, sizeof(value))) {
			printk(KERN_ERR "Data Write : Err!\n");
		}
		printk(KERN_WARNING "Value = %d\n", value);
		break;
	case RD_VALUE:
		if (copy_to_user((int32_t *)arg, &value, sizeof(value))) {
			printk(KERN_ERR "Data Read : Err!\n");
		}
		break;
	default:
		printk(KERN_WARNING "Default\n");
		break;
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
	int ret;
	struct instance_data *idata = dev_get_drvdata(&rpdev->dev);

	dev_info(&rpdev->dev, "incoming msg %d (src: 0x%x): %s\n",
		 ++idata->rx_count, src, (char *)data);

	print_hex_dump_debug(__func__, DUMP_PREFIX_NONE, 16, 1, data, len,
			     true);

	// /* samples should not live forever */
	// if (idata->rx_count >= count)
	// {
	//     dev_info(&rpdev->dev, "goodbye!\n");
	//     return 0;
	// }

	// /* send a new message now */
	// ret = rpmsg_send(rpdev->ept, MSG, strlen(MSG));
	// if (ret) dev_err(&rpdev->dev, "rpmsg_send failed: %d\n", ret);

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

	while (!rpdev->dev.offline) {
		/* send a message to our remote processor */
		ret = rpmsg_send(rpdev->ept, MSG, strlen(MSG));
		if (ret) {
			dev_err(&rpdev->dev, "rpmsg_send failed: %d\n", ret);
			return ret;
		}

		msleep(1000);
	}

	dev_err(&rpdev->dev, "rpmsg disconnected!\n");

	return 0;
}

static void rpmsg_sample_remove(struct rpmsg_device *rpdev)
{
	dev_info(&rpdev->dev, "rpmsg sample client driver is removed\n");
}

static struct rpmsg_device_id rpmsg_driver_sample_id_table[] = {
	{ .name = "rpmsg-client-sample" },
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
module_rpmsg_driver(rpmsg_sample_client);

// static int __init stm32ecu_init(void)
// {
// //	int
// 	printk(KERN_ALERT "JORDAN IS INITIALISING STM32ECU");
// 	return 0;
// }

// static void __exit stm32ecu_exit(void)
// {
// 	printk(KERN_ALERT "JORDAN IS EXITING STM32ECU");
// }

// module_init(stm32ecu_init);
// module_exit(stm32ecu_exit);

MODULE_AUTHOR("Jordan Esh <jordan.esh@monashmotorsport.com>");
MODULE_DESCRIPTION("STM32 based ECU kernel driver for Monash Motorsport");
MODULE_LICENSE("GPL v2");
