/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef TIMED_LOGGER_H
#define TIMED_LOGGER_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/workqueue.h>

extern int interval_sec;
extern struct work_struct timer_queue;

int timed_logger_hrtimer_init(void);
void timed_logger_hrtimer_exit(void);
void logger_workqueue_exit(void);

#endif /* TIMED_LOGGER_H */
