#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <linux/sched.h>
#include <linux/cred.h>
#include <linux/syscalls.h>

// Question 4

asmlinkage long sys_bhanujgetppid(void)
{
        return task_ppid_nr(current);
}
