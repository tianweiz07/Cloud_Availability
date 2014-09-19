#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#define __NR_CachePrime 188

int main(int argc, char* argv[]) {
	int t = 500;
	pid_t pid;
	pid = fork();
	if (pid < 0) {
		printf("Fork Error\n");
		return 0;
	}
	if (pid == 0)
		syscall(__NR_CachePrime, t, 0);
	else {
		usleep(20000);
		syscall(__NR_CachePrime, t, 1);
	}
	printf("Cache Prime Done!\n");
	return 0;
}
