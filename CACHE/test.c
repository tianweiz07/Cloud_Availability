#include <stdlib.h>
#include <stdio.h>
#include <sys/syscall.h>

#define __NR_CachePrime 188

int main(int argc, char* argv[]) {
	int t = 500;
	syscall(__NR_CachePrime, t);
	printf("Cache Prime Done!\n");
	return 0;
}
