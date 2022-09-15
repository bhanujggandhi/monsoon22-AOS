#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	if(argc <= 1) {
		printf("Must provide a string to give to system call.\n");
		return -1;
	}

	char *arg = argv[1];
	printf("Making bhanujprint system call with \"%s\".\n", arg);
	long res = syscall(388, arg);
	printf("bhanujprint system call returned %ld.\n", res);
	return res;
}
