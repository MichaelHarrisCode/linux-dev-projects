// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/atomic.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include "watchdog_timer.h"

atomic_t timer_enabled = ATOMIC_INIT(0); // Disabled by default
atomic_t timeout = ATOMIC_INIT(10);

static int __init wtimer_init(void)
{
	int ret = 0;

	ret = wtimer_dev_init();
	if (ret)
		return ret;

	ret = wtimer_hrtimer_init();
	if (ret) {
		wtimer_dev_exit();
		return ret;
	}

	pr_info("watchdog_timer: initialized\n");

	return 0;
}

static void __exit wtimer_exit(void)
{
	wtimer_dev_exit();
	wtimer_hrtimer_exit();
	pr_info("watchdog_timer: exited\n");
}

module_init(wtimer_init);
module_exit(wtimer_exit);

MODULE_AUTHOR("Michael Harris <michaelharriscode@gmail.com>");
MODULE_DESCRIPTION("Simple watchdog timer");
MODULE_LICENSE("GPL");
