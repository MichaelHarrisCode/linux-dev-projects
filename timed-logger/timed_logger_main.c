// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include "timed_logger.h"

int interval_sec = 5;

module_param(interval_sec, int, 0664); // Does this need to be 0666?
MODULE_PARM_DESC(interval_sec, "Interval between each log print (in seconds)");

static int __init timed_logger_init(void)
{
	int ret;

	ret = timed_logger_hrtimer_init();
	if (ret)
		return ret;

	pr_info("timed_logger: initialized with interval: %d\n", interval_sec);
	return 0;
}

static void __exit timed_logger_exit(void)
{
	timed_logger_hrtimer_exit();
	logger_workqueue_exit();
	pr_info("timed_logger: exited\n");
}

module_init(timed_logger_init);
module_exit(timed_logger_exit);

MODULE_LICENSE("GPL");
