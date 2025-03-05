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
static dev_t dev_num = 0;
static struct class *echo_class;
static struct device *echo_device;

static int __init my_module_init(void)
{
	int err_ret = 0;

	proc_entry = proc_create(PROC_NAME, S_IRUGO | S_IWUGO, NULL, 
			  &echo_proc_ops);

	if ((err_ret = (alloc_chrdev_region(&dev_num, 0, 1, CDEV_NAME) < 0))) {
		printk(KERN_ERR "echo_device: Failed to allocate cdev");
		return err_ret;	
	}

	cdev_init(&echo_cdev, &echo_cdev_ops);
	if ((err_ret = (cdev_add(&echo_cdev, dev_num, 1) < 0))) {
		printk(KERN_ERR "echo_device: Failed to add cdev");
		goto cdev_add_err;
	}

	echo_class = class_create(CDEV_NAME);
	if (IS_ERR(echo_class)) {
		printk(KERN_ERR "echo_device: Failed creating class");
		err_ret = PTR_ERR(echo_class);
		goto class_create_err;
	}

	echo_device = device_create(echo_class, NULL, dev_num, NULL, CDEV_NAME);
	if(IS_ERR(echo_device)) {
		printk(KERN_ERR "echo_device: Failed creating device");
		err_ret = PTR_ERR(echo_device);
		goto device_create_err;
	}

	printk(KERN_INFO "echo_device: Initialized\n");
	return 0;

device_create_err:
	class_destroy(echo_class);
class_create_err:
	cdev_del(&echo_cdev);
cdev_add_err:
	unregister_chrdev_region(dev_num, 1);
	return err_ret;
}

static void __exit my_module_exit(void)
{
	proc_remove(proc_entry);

	device_destroy(echo_class, dev_num);
	class_destroy(echo_class);
	cdev_del(&echo_cdev);
	unregister_chrdev_region(dev_num, 1);

	printk(KERN_INFO "echo_device: Exited\n");
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_AUTHOR("Michael Harris <michaelharriscode@gmail.com>");
MODULE_DESCRIPTION("Echo stored data back to user");
MODULE_LICENSE("GPL");
