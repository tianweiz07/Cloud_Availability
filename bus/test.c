#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#define __NR_BusLock 188

int main(int argc, char* argv[]) {
	pid_t pid1, pid2, pid3;
	int cpu_id;
	int time = atoi(argv[1]);
	pid1 = fork();
	pid2 = fork();
	pid3 = fork();
	if ((pid1<0)||(pid2<0)||(pid3<0)) {
		printf("fork error\n");
		return 0;
	}
	cpu_id = (pid1>0)*4 + (pid2>0)*2 + (pid3>0);
	printf("Bus Lock Start!\n");
	syscall(__NR_BusLock, time, cpu_id);
	printf("Bus Lock Done!\n");
	return 0;
}
