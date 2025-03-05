// SPDX-License-Identifier: GPL-3.0

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/atomic.h>
#include <linux/mutex.h>
#include "echo_module.h"

#define PROC_NAME "echo_status"
#define CDEV_NAME "echo_device"

DEFINE_MUTEX(buffer_lock);

atomic_t device_enabled = ATOMIC_INIT(1);
char buffer[BUF_SIZE] = "";

static struct proc_dir_entry *proc_entry;
static struct cdev echo_cdev;
static dev_t dev_num;
static struct class *echo_class;
static struct device *echo_device;

static int __init my_module_init(void)
{
	int err_ret = 0;

	proc_entry = proc_create(PROC_NAME, 0664, NULL, &echo_proc_ops);
	if (!proc_entry) {
		pr_err("echo_device: Failed to create /proc file");
		return -ENOMEM;
	}

	err_ret = (alloc_chrdev_region(&dev_num, 0, 1, CDEV_NAME) < 0);
	if (err_ret) {
		pr_err("echo_device: Failed to allocate cdev");
		goto alloc_chrdev_err;
	}

	cdev_init(&echo_cdev, &echo_cdev_ops);
	err_ret = (cdev_add(&echo_cdev, dev_num, 1) < 0);
	if (err_ret) {
		pr_err("echo_device: Failed to add cdev");
		goto cdev_add_err;
	}

	echo_class = class_create(CDEV_NAME);
	if (IS_ERR(echo_class)) {
		pr_err("echo_device: Failed creating class");
		err_ret = PTR_ERR(echo_class);
		goto class_create_err;
	}

	echo_device = device_create(echo_class, NULL, dev_num, NULL, CDEV_NAME);
	if (IS_ERR(echo_device)) {
		pr_err("echo_device: Failed creating device");
		err_ret = PTR_ERR(echo_device);
		goto device_create_err;
	}

	pr_info("echo_device: Initialized\n");
	return 0;

device_create_err:
	class_destroy(echo_class);
class_create_err:
	cdev_del(&echo_cdev);
cdev_add_err:
	unregister_chrdev_region(dev_num, 1);
alloc_chrdev_err:
	proc_remove(proc_entry);
	return err_ret;
}

static void __exit my_module_exit(void)
{
	proc_remove(proc_entry);

	device_destroy(echo_class, dev_num);
	class_destroy(echo_class);
	cdev_del(&echo_cdev);
	unregister_chrdev_region(dev_num, 1);

	pr_info("echo_device: Exited\n");
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_AUTHOR("Michael Harris <michaelharriscode@gmail.com>");
MODULE_DESCRIPTION("Echo stored data back to user");
MODULE_LICENSE("GPL");
