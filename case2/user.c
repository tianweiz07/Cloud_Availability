#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#define __NR_CpuPreempt 188

int main(int argc, char* argv[]) {
	int t = atoi(argv[1]);
	syscall(__NR_CpuPreempt, t);
	printf("CPU Preemption Done!\n");
	return 0;
}
