#include <stdio.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <unistd.h>
int main()
{
	long int res = syscall(389);
	printf("System call bhanujprocess returned %ld\n", res);
	return 0;
}
