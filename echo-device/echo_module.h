/* SPDX-License-Identifier: GPL-3.0 */

#ifndef MODULE_ECHO_H
#define MODULE_ECHO_H

#include <linux/atomic.h>
#include <linux/mutex.h>

#define BUF_SIZE 256

extern atomic_t device_enabled;
extern char buffer[BUF_SIZE];
extern struct mutex buffer_lock;

extern const struct proc_ops echo_proc_ops;
extern const struct file_operations echo_cdev_ops;

#endif /* MODULE_ECHO_H */
