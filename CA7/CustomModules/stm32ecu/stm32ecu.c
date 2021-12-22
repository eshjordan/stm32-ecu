// SPDX-License-Identifier: MIT
/*
 * Copyright (C) Jordan Esh 2021 - All Rights Reserved
 * Author: Jordan Esh <jordan.esh@monashmotorsport.com> for Monash Motorsport.
 */

#include <linux/kernel.h>
#include <linux/module.h>

static int __init stm32ecu_init(void)
{
//	int
	printk(KERN_ALERT "JORDAN IS INITIALISING STM32ECU");
	return 0;
}

static void __exit stm32ecu_exit(void)
{
	printk(KERN_ALERT "JORDAN IS EXITING STM32ECU");
}

module_init(stm32ecu_init);
module_exit(stm32ecu_exit);

MODULE_AUTHOR("Jordan Esh <jordan.esh@monashmotorsport.com>");
MODULE_DESCRIPTION("STM32 based ECU kernel driver for Monash Motorsport");
MODULE_LICENSE("GPL v2");
