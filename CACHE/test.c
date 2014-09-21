#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#define __NR_CachePrime 188

int main(int argc, char* argv[]) {
	int t = 500;
	pid_t pid1, pid2;
	pid1 = fork();
	pid2 = fork();
	if ((pid1 < 0)||(pid2 < 0)) {
		printf("Fork Error\n");
		return 0;
	}
	if ((pid1 == 0)&&(pid2 == 0)) {
		syscall(__NR_CachePrime, t, 0);
	}

	if ((pid1 == 0)&&(pid2 > 0)) {
		usleep(20000);
		syscall(__NR_CachePrime, t, 1);
	}

	if ((pid1 > 0)&&(pid2 == 0)) {
		usleep(40000);
		syscall(__NR_CachePrime, t, 2);
	}

	if ((pid1 > 0)&&(pid2 > 0)) {
		usleep(60000);
		syscall(__NR_CachePrime, t, 3);
	}

	printf("Cache Prime Done!\n");
	return 0;
}
