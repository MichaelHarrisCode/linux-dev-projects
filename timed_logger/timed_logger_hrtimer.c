// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include "timed_logger.h"

static struct hrtimer timer;

static enum hrtimer_restart timer_callback(struct hrtimer *timer)
{
	schedule_work(&timer_queue);

	hrtimer_forward_now(timer, ktime_set(interval_sec, 0));
	return HRTIMER_RESTART;
}

int timed_logger_hrtimer_init(void)
{
	hrtimer_init(&timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	timer.function = timer_callback;
	hrtimer_start(&timer, ktime_set(interval_sec, 0),
	       HRTIMER_MODE_REL);

	return 0;
}

void timed_logger_hrtimer_exit(void)
{
	hrtimer_cancel(&timer);
}
