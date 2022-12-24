#include <stdio.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <unistd.h>

int main()
{
	printf("System call bhanujgetppid returned %ld\n", syscall(390));
}
