#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

SYSCALL_DEFINE1(bhanujprint, char *, message)
{
	char buf[256];
	long copied = strncpy_from_user(buf, message, sizeof(buf));
	
	if(copied < 0 || copied == sizeof(buf))
		return -EFAULT;
	printk(KERN_INFO "bhanujprint syscall called with \"%s\"\n", buf);
	return 0;
}
