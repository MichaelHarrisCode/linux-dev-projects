/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef WATCHDOG_TIMER_H
#define WATCHDOG_TIMER_H

#include <linux/atomic.h>

extern atomic_t timer_enabled;
extern atomic_t timeout;

int wtimer_dev_init(void);
void wtimer_dev_exit(void);
int wtimer_hrtimer_init(void);
void wtimer_hrtimer_exit(void);

void wtimer_hrtimer_restart(void);
void wtimer_hrtimer_start(void);
void wtimer_hrtimer_stop(void);

#endif /* WATCHDOG_TIMER_H */
