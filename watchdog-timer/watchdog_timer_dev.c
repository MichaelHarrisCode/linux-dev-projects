// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/atomic.h>
#include "watchdog_timer.h"

#define CDEV_NAME "watchdog_timer"

static struct class *watchdog_class;
static struct device *watchdog_device;
static dev_t dev_num;
static struct cdev watchdog_cdev;
static int written_to;

/*
 * Interactions with cdev still trigger with a disabled device:
 * - writing to cdev starts the timer if it wasn't already one
 *   - do I check for enabled in cdev write or in wtimer_hrtimer_restart()?
 * - reading the cdev causes timeout when disabled
 *
 */
static int cdev_release(struct inode *inode, struct file *file)
{
	if (!written_to)
		pr_err("watchdog_timer: timed out\n");

	written_to = 0;
	return 0;
}

static ssize_t cdev_write(struct file *file, const char __user *buf,
			    size_t count, loff_t *pos)
{
	written_to = 1;

	wtimer_hrtimer_restart();

	return count;
}

static const struct file_operations cdev_fops = {
	.write = cdev_write,
	.release = cdev_release,
};

static ssize_t timeout_show(const struct class *class,
			    const struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", atomic_read(&timeout));
}

static ssize_t timeout_store(const struct class *class,
			     const struct class_attribute *attr,
			     const char *buf, size_t count)
{
	int passed_value;

	if (kstrtoint(buf, 10, &passed_value) < 0)
		return -EINVAL;

	if (passed_value < 0)
		return -EINVAL;

	atomic_set(&timeout, passed_value);
	wtimer_hrtimer_restart();

	return count;
}

static CLASS_ATTR_RW(timeout);

static ssize_t enabled_show(const struct class *class,
			    const struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", atomic_read(&timer_enabled));
}

static ssize_t enabled_store(const struct class *class,
			     const struct class_attribute *attr,
			     const char *buf, size_t count)
{
	int passed_value;

	if (kstrtoint(buf, 10, &passed_value) < 0)
		return -EINVAL;

	if (passed_value < 0 || passed_value > 1)
		return -EINVAL;

	atomic_set(&timer_enabled, passed_value);

	if (passed_value)
		wtimer_hrtimer_start();
	else
		wtimer_hrtimer_stop();

	return count;
}

static CLASS_ATTR_RW(enabled);

static int wtimer_cdev_init(void)
{
	int ret = 0;

	ret = alloc_chrdev_region(&dev_num, 0, 1, CDEV_NAME);
	if (ret)
		return ret;

	cdev_init(&watchdog_cdev, &cdev_fops);
	ret = cdev_add(&watchdog_cdev, dev_num, 1);
	if (ret) {
		unregister_chrdev_region(dev_num, 1);
		return ret;
	}

	return 0;
}

static void wtimer_cdev_exit(void)
{
	cdev_del(&watchdog_cdev);
	unregister_chrdev_region(dev_num, 1);
}

static int wtimer_devnode_init(void)
{
	watchdog_class = class_create(CDEV_NAME);
	if (IS_ERR(watchdog_class))
		return PTR_ERR(watchdog_class);

	watchdog_device = device_create(watchdog_class, NULL, dev_num, NULL,
				 CDEV_NAME);
	if (IS_ERR(watchdog_device)) {
		class_destroy(watchdog_class);
		return PTR_ERR(watchdog_device);
	}

	return 0;
}

static void wtimer_devnode_exit(void)
{
	device_destroy(watchdog_class, dev_num);
	class_destroy(watchdog_class);
}

static int wtimer_sys_attributes_init(void)
{
	int ret = 0;

	ret = class_create_file(watchdog_class, &class_attr_enabled);
	if (ret)
		return ret;

	ret = class_create_file(watchdog_class, &class_attr_timeout);
	if (ret) {
		class_remove_file(watchdog_class, &class_attr_enabled);
		return ret;
	}

	return 0;
}

static void wtimer_sys_attributes_exit(void)
{
	class_remove_file(watchdog_class, &class_attr_timeout);
	class_remove_file(watchdog_class, &class_attr_enabled);
}

int wtimer_dev_init(void)
{
	int ret = 0;

	ret = wtimer_cdev_init();
	if (ret)
		return ret;

	ret = wtimer_devnode_init();
	if (ret) {
		wtimer_cdev_exit();
		return ret;
	}

	ret = wtimer_sys_attributes_init();
	if (ret) {
		wtimer_cdev_exit();
		wtimer_devnode_exit();
		return ret;
	}

	return 0;
}

void wtimer_dev_exit(void)
{
	wtimer_sys_attributes_exit();
	wtimer_devnode_exit();
	wtimer_cdev_exit();
}
