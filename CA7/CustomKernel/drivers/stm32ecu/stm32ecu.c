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
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/vmalloc.h>
#include <linux/kfifo.h>
#include <linux/stm32ecu/stm32ecu.h>
#include <linux/stm32ecu/shared/CRC.h>
#include <linux/stm32ecu/shared/Interproc_Msg.h>

#define MSG "hello world from stm32ecu!"

static struct class *dev_class;
static dev_t dev = 0;
static struct device *my_device;
static struct cdev cdev;

// Waitqueue
DECLARE_WAIT_QUEUE_HEAD(wait_queue_poll_data);

enum sent_msg_t {
	SENT_MSG_INTERNAL = 0,
	SENT_MSG_EXTERNAL_UNIQUE = 1,
	SENT_MSG_EXTERNAL_FIFO = 2,
};

// Used to track whether the msg was sent internally, or externally using write() or ioctl()
DEFINE_KFIFO(fifo_send_types, u8, 512);

// Used to store response msgs, when the tx used write()
DEFINE_KFIFO(fifo_external_msg, Interproc_Msg_t, 256);

// Used to store response msgs, when the tx used ioctl(fd, STM32ECU_SEND_MSG, msg)
DEFINE_KFIFO(fifo_unique_ids, u32, 256);
DEFINE_XARRAY_ALLOC1(rx_unique_buffer);

static int32_t value = 0;
static int device_open = 0;

static struct rpmsg_device *global_rproc_ref = NULL;

static int ioctl_open(struct inode *inode, struct file *file);
static int ioctl_release(struct inode *inode, struct file *file);
static ssize_t ioctl_read(struct file *filp, char __user *buf, size_t len,
			  loff_t *off);
static ssize_t ioctl_write(struct file *filp, const char *buf, size_t len,
			   loff_t *off);
static long ioctl_call(struct file *file, unsigned int cmd, unsigned long arg);

static unsigned int poll_cb(struct file *filp, struct poll_table_struct *wait);

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = ioctl_read,
	.write = ioctl_write,
	.open = ioctl_open,
	.unlocked_ioctl = ioctl_call,
	.release = ioctl_release,
	.poll = poll_cb,
};

static int ioctl_open(struct inode *inode, struct file *file)
{
	if (device_open)
		return -EBUSY;

	device_open++;
	printk(KERN_WARNING "Device File Opened...!!!\n");
	return 0;
}

/*
** This function will be called when we close the Device file
*/
static int ioctl_release(struct inode *inode, struct file *file)
{
	device_open--;
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

	size_t num_msgs = len / sizeof(Interproc_Msg_t);
	size_t expected_len = sizeof(Interproc_Msg_t) * num_msgs;
	if (len != expected_len) {
		printk(KERN_ERR
		       "Data Read : Err, buffer length is not a multiple of sizeof(Interproc_Msg_t)!\n");
		return 0;
	}

	auto num_stored_msgs = kfifo_size(&fifo_external_msg);
	if (0 == num_stored_msgs) {
		printk(KERN_ERR "Data Read : Err, no messages to read!\n");
		return 0;
	}

	auto num_output_msgs = min(num_msgs, num_stored_msgs);

	ssize_t bytes_read = 0;
	int i;
	for (i = 0; i < num_output_msgs; i++) {
		Interproc_Msg_t *out_msg = ((Interproc_Msg_t *)buf)+i;
		kfifo_get(&fifo_external_msg, out_msg);
		bytes_read += sizeof(Interproc_Msg_t);
	}

	return bytes_read;
}

/*
** This function will be called when we write the Device file
*/
static ssize_t ioctl_write(struct file *filp, const char __user *buf,
			   size_t len, loff_t *off)
{
	printk(KERN_WARNING "Write function\n");
	kfifo_put(&fifo_send_types, SENT_MSG_EXTERNAL_FIFO);

	size_t num_msgs = len / sizeof(Interproc_Msg_t);
	size_t expected_len = sizeof(Interproc_Msg_t) * num_msgs;
	if (len != expected_len) {
		printk(KERN_ERR
		       "Data Write : Err, buffer length is not a multiple of sizeof(Interproc_Msg_t)!\n");
		return 0;
	}

	Interproc_Msg_t *msg_list = (Interproc_Msg_t *)vmalloc(len);
	if (copy_from_user(msg_list, buf, len)) {
		printk(KERN_ERR "Data Write : Err!\n");
		return 0;
	}

	int i;
	for (i = 0; i < num_msgs; i++) {
		Interproc_Msg_t *msg = msg_list+i;
		if (msg->checksum !=
		    calc_crc(msg, offsetof(Interproc_Msg_t, checksum))) {
			printk(KERN_ERR
			       "Interproc_Msg_t at idx %d has invalid CRC!\n",
			       i);
			return 0;
		}
	}

	ssize_t bytes_written = 0;
	for (i = 0; i < num_msgs; i++) {
		Interproc_Msg_t *msg = msg_list+i;
		int ret = rpmsg_send(global_rproc_ref->ept, msg,
				     sizeof(Interproc_Msg_t));
		if (ret) {
			dev_err(&global_rproc_ref->dev,
				"rpmsg_send of msg at idx %d failed: %d\n",
				i, ret);
			return bytes_written;
		}

		bytes_written += sizeof(Interproc_Msg_t);
		kfifo_put(&fifo_send_types, SENT_MSG_EXTERNAL_FIFO);
	}

	return bytes_written;
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
			return -EINVAL;
		}
		printk(KERN_WARNING "Value = %d\n", value);
		break;
	}
	case STM32ECU_GET_STATE: {
		if (copy_to_user((int32_t *)arg, &value, sizeof(value))) {
			printk(KERN_ERR "Data Read : Err!\n");
			return -EINVAL;
		}
		break;
	}
	case STM32ECU_PING_RPROC: {
		if (!global_rproc_ref || global_rproc_ref->dev.offline) {
			printk(KERN_ERR "No RPROC connected\n");
			return -EINVAL;
		}

		Interproc_Msg_t msg = { .command = CMD_PING,
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

		if (!kfifo_put(&fifo_send_types, SENT_MSG_INTERNAL)) {
			return -EINVAL;
		}

		break;
	}
	case STM32ECU_SEND_MSG: {
		Interproc_Msg_t msg = { 0 };
		if (copy_from_user(&msg, (Interproc_Msg_t *)arg,
				   sizeof(Interproc_Msg_t))) {
			printk(KERN_ERR "Data Write Msg : Err!\n");
			return -EINVAL;
		}

		int ret = rpmsg_send(global_rproc_ref->ept, &msg,
				     sizeof(Interproc_Msg_t));
		if (ret) {
			dev_err(&global_rproc_ref->dev,
				"rpmsg_send failed: %d\n", ret);
			return ret;
		}

		if (!kfifo_put(&fifo_send_types, SENT_MSG_EXTERNAL_UNIQUE)) {
			return -EINVAL;
		}

		// Give a UID to userspace so we can give them their msg once it's received
		u32 id;
		static u32 next = 1;
		if (xa_alloc_cyclic(&rx_unique_buffer, &id, NULL,
				    XA_LIMIT(1, 1024), &next, GFP_KERNEL) < 0) {
			return -EINVAL;
		}

		// Store our idx for this message, so we know where to place it when it's received
		if (!kfifo_put(&fifo_unique_ids, id)) {
			return -EINVAL;
		}

		if (copy_to_user((u32 *)arg, &id, sizeof(u32))) {
			printk(KERN_ERR "Data Write Msg : Err!\n");
			return -EINVAL;
		}

		return 0;
	}
	case STM32ECU_RECV_MSG: {
		u32 id;
		if (copy_from_user(&id, (u32 *)arg, sizeof(u32))) {
			printk(KERN_ERR "Data Write Msg : Err!\n");
			return -EINVAL;
		}

		// xa_reserve
		// xa_release
		// xa_alloc
		// xa_store
		// xas_load
		// xa_erase

		if (NULL == xa_load(&rx_unique_buffer, id)) {
			printk(KERN_ERR "No message with uid %d in buffer yet!\n", id);
			return -EAGAIN;
		}

		Interproc_Msg_t *msg =
			(Interproc_Msg_t *)xa_erase(&rx_unique_buffer, id);

		if (copy_to_user((Interproc_Msg_t *)arg, msg,
				 sizeof(Interproc_Msg_t))) {
			printk(KERN_ERR "Data Write Msg : Err!\n");
			return -EINVAL;
		}

		return 0;
	}
	default: {
		printk(KERN_WARNING "Default\n");
		break;
	}
	}
	return 0;
}

static unsigned int poll_cb(struct file *filp, struct poll_table_struct *wait)
{
	__poll_t mask = (POLLOUT | POLLWRNORM);

	poll_wait(filp, &wait_queue_poll_data, wait);
	pr_info("Poll function\n");

	/* Do your Operation */
	if (!kfifo_is_empty(&fifo_external_msg)) {
		mask |= (POLLIN | POLLRDNORM);
	}

	return mask;
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

		u8 sent_type;
		if (!kfifo_get(&fifo_send_types, &sent_type)) {
			return -EINVAL;
		}

		if (crc == msg->checksum) {
			printk(KERN_WARNING "CRC OK\n");
			printk(KERN_WARNING "msg->command = %u\n",
			       msg->command);
			int i;
			for (i = 0; i < 11; i++) {
				printk(KERN_WARNING "msg->data[%d] = %u\n", i,
				       msg->data[i]);
			}
			printk(KERN_WARNING "msg->data (str) = [%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s]\n",
			       msg->data[0], msg->data[1], msg->data[2],
			       msg->data[3], msg->data[4], msg->data[5],
			       msg->data[6], msg->data[7], msg->data[8],
			       msg->data[9], msg->data[10]);

			switch (sent_type) {
			case SENT_MSG_EXTERNAL_FIFO: {
				// Store the message in a buffer, ready to give to userspace
				printk(KERN_WARNING "Storing in FIFO");
				if (!kfifo_put(&fifo_external_msg, *msg)) {
					return -EINVAL;
				}
				break;
			}
			case SENT_MSG_EXTERNAL_UNIQUE: {
				// Store the message in an indexed list, ready to give to userspace
				printk(KERN_WARNING "Storing in XArray");
				u32 id;
				kfifo_get(&fifo_unique_ids, &id);
				Interproc_Msg_t *ptr =
					vmalloc(sizeof(Interproc_Msg_t));
				*ptr = *msg;
				if (xa_is_err(xa_store(&rx_unique_buffer, id,
						       ptr, GFP_KERNEL))) {
					return -EINVAL;
				}
				break;
			}
			case SENT_MSG_INTERNAL:
			default: {
				// Sent as part of a ping request or something, don't do anything
				printk(KERN_WARNING "Not storing, internal request\n");
				break;
			}
			}
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

	unsigned long i = 0;
	Interproc_Msg_t *entry;
	xa_for_each (&rx_unique_buffer, i, entry) {
		vfree(entry);
	}

	xa_destroy(&rx_unique_buffer);

	kfifo_free(&fifo_send_types);
	kfifo_free(&fifo_external_msg);
	kfifo_free(&fifo_unique_ids);

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
