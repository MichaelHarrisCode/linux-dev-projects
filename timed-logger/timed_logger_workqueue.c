// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/workqueue.h>
#include "timed_logger.h"

#define SEC_PER_MIN 60
#define SEC_PER_HR 3600
#define SEC_PER_DAY 86400

static void print_time(struct work_struct *workqueue)
{
	s64 time_raw_seconds  = ktime_divns(ktime_get_coarse_boottime(),
				     NSEC_PER_SEC);
	s64 time_s = time_raw_seconds % 60;
	s64 time_min = (time_raw_seconds / SEC_PER_MIN) % 60;
	s64 time_hr = (time_raw_seconds / SEC_PER_HR) % 24;
	s64 time_day = time_raw_seconds / SEC_PER_DAY;

	pr_info("timed_logger: %lldd %02lldh %02lldm %02llds",
		time_day, time_hr, time_min, time_s);
}

DECLARE_WORK(timer_queue, print_time);

void logger_workqueue_exit(void)
{
	flush_work(&timer_queue);
	cancel_work_sync(&timer_queue);
}
