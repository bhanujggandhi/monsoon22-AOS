#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <linux/sched.h>
#include <linux/cred.h>
#include <linux/syscalls.h>

// Question 3

asmlinkage long sys_bhanujprocess(void)
{
	printk(KERN_INFO "Current Process ID from bhanujprocess: %d\n", current->pid);
	printk(KERN_INFO "Parent Process ID from bhanujprocess: %d\n", task_ppid_nr(current));
        return 0;
}
