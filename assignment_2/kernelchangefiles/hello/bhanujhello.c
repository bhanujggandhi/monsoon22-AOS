#include <linux/kernel.h>

asmlinkage long sys_bhanujhello(void)
{
	printk("Hello Bhanuj\n");
	return 0;
}
