#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/atomic.h>
#include <linux/mutex.h>
#include "echo_module.h"

static ssize_t echo_proc_read(struct file *file, char __user *buf, size_t count, 
		       loff_t *pos) 
{
	char *enabled = "enabled\n";
	char *disabled = "disabled\n";
	char **message;
	size_t len;

	if (atomic_read(&device_enabled))
		message = &enabled;
	else
		message = &disabled;
	
	len = min(strlen(*message), count);

	if (*pos >= len)
		return 0;

	*message[len - 1] = '\0';

	if (copy_to_user(buf, *message + *pos, len))
		return -EFAULT;

	*pos += len;
	return len;
}

static ssize_t echo_proc_write(struct file *file, const char __user *buf, 
			       size_t count, loff_t *pos)
{
	char input_buf[BUF_SIZE];
	count = min_t(size_t, BUF_SIZE - 1, count);

	if (count > 2)
		return -EPERM;

	input_buf[1] = '\0';

	if (copy_from_user(input_buf, buf, count))
		return -EFAULT;

	if (input_buf[0] == '1') {
		printk(KERN_INFO "echo_device: enabled\n");
		atomic_set(&device_enabled, 1);
	} else if (input_buf[0] == '0') {
		printk(KERN_INFO "echo_device: disabled\n");
		atomic_set(&device_enabled, 0);
	}

	return count;
}

const struct proc_ops echo_proc_ops = {
	.proc_read = echo_proc_read,
	.proc_write = echo_proc_write,
};
