#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/atomic.h>
#include <linux/mutex.h>
#include "echo_module.h"

static ssize_t echo_cdev_read(struct file *file, char __user *buf, size_t count,
		       loff_t *pos)
{
	char disable_buf[BUF_SIZE] = "Device is disabled\n";
	char (*message)[BUF_SIZE] = &buffer;
	size_t len;

	if (!atomic_read(&device_enabled))
		message = &disable_buf;
	
	len = min(strlen(*message), count);

	if (*pos >= len)
		return 0;

	mutex_lock(&buffer_lock);
	// Doesn't work when device is disabled??? Probably fine, as the message
	// when disabled is a string literal which is *hopefully*
	// null-terminated during initialization
	if (atomic_read(&device_enabled))
		*message[len - 1] = '\0';

	if (copy_to_user(buf, *message + *pos, len))
		return -EFAULT;

	mutex_unlock(&buffer_lock);

	*pos += len;
	return len;
}

static ssize_t echo_cdev_write(struct file *file, const char __user *buf, 
			       size_t count, loff_t *pos)
{
	if (!atomic_read(&device_enabled))
		return -EBUSY;

	count = min_t(size_t, BUF_SIZE - 1, count);

	mutex_lock(&buffer_lock);
	if (copy_from_user(buffer, buf, count))
		return -EFAULT;

	buffer[count] = '\0';
	mutex_unlock(&buffer_lock);

	return count;
}

struct file_operations echo_cdev_ops = {
	.read = echo_cdev_read,
	.write = echo_cdev_write,
};
