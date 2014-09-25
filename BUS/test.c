#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#define __NR_BusLock 188
#define TIME 50

int main(int argc, char* argv[]) {
	syscall(__NR_BusLock, TIME);
	printf("Bus Lock Done!\n");
	return 0;
}
