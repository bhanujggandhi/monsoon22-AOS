#include <linux/kernel.h>

// Question 1

asmlinkage long sys_bhanujhello(void)
{
	printk("Hello Bhanuj\n");
	return 0;
}
