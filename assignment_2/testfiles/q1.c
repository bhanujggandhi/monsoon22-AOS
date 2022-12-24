#include <stdio.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <unistd.h>

int main()
{
	long int message = syscall(387);
	printf("System Call bhanujhello returned %ld\n", message);
	return 0;
}
