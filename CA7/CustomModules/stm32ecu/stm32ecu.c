// SPDX-License-Identifier: MIT
/*
 * Copyright (C) Jordan Esh 2021 - All Rights Reserved
 * Author: Jordan Esh <jordan.esh@monashmotorsport.com> for Monash Motorsport.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/delay.h>

#define MSG "hello world from stm32ecu!"

static int count = 100;
module_param(count, int, 0644);

struct instance_data {
    int rx_count;
};

static int rpmsg_sample_cb(struct rpmsg_device *rpdev, void *data, int len, void *priv, u32 src)
{
    int ret;
    struct instance_data *idata = dev_get_drvdata(&rpdev->dev);

    dev_info(&rpdev->dev, "incoming msg %d (src: 0x%x): %s\n", ++idata->rx_count, src, (char*)data);

    print_hex_dump_debug(__func__, DUMP_PREFIX_NONE, 16, 1, data, len, true);

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

    dev_info(&rpdev->dev, "new channel: 0x%x -> 0x%x!\n", rpdev->src, rpdev->dst);

    idata = devm_kzalloc(&rpdev->dev, sizeof(*idata), GFP_KERNEL);
    if (!idata) { return -ENOMEM; }

    dev_set_drvdata(&rpdev->dev, idata);

    while (!rpdev->dev.offline)
    {
        /* send a message to our remote processor */
        ret = rpmsg_send(rpdev->ept, MSG, strlen(MSG));
        if (ret)
        {
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
    {.name = "rpmsg-client-sample"},
    {},
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_driver_sample_id_table);

static struct rpmsg_driver rpmsg_sample_client = {
    .drv.name = KBUILD_MODNAME,
    .id_table = rpmsg_driver_sample_id_table,
    .probe    = rpmsg_sample_probe,
    .callback = rpmsg_sample_cb,
    .remove   = rpmsg_sample_remove,
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
