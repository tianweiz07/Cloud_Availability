#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#define __NR_BusLock 188

int main(int argc, char* argv[]) {
	int time = atoi(argv[1]);
	syscall(__NR_BusLock, time);
	printf("Bus Lock Done!\n");
	return 0;
}
