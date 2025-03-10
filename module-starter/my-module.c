// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

static int __init my_module_init(void)
{
	pr_info("Hello, World!\n");
	return 0;
}

static void __exit my_module_exit(void)
{
	pr_info("Goodbye, World!\n");
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
