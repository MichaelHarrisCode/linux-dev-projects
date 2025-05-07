// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/hrtimer.h>
#include <linux/atomic.h>
#include "watchdog_timer.h"

static struct hrtimer timer;

static enum hrtimer_restart timer_callback(struct hrtimer *timer)
{
	pr_err("watchdog_timer: timed out\n");

	hrtimer_forward_now(timer, ktime_set(atomic_read(&timeout), 0));
	return HRTIMER_RESTART;
}

void wtimer_hrtimer_restart(void)
{
	wtimer_hrtimer_stop();
	wtimer_hrtimer_start();
}

void wtimer_hrtimer_start(void)
{
	hrtimer_start(&timer, ktime_set(atomic_read(&timeout), 0),
	       HRTIMER_MODE_REL);
}

void wtimer_hrtimer_stop(void)
{
	hrtimer_cancel(&timer);
}

int wtimer_hrtimer_init(void)
{
	hrtimer_init(&timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	timer.function = timer_callback;

	return 0;
}

void wtimer_hrtimer_exit(void)
{
	hrtimer_cancel(&timer);
}
