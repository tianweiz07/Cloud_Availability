#include <stdlib.h>
#include <stdio.h>
#include <sys/syscall.h>

#define __NR_CachePrime 188

int main(int argc, char* argv[]) {
	int t = 5;
	syscall(__NR_CachePrime, 5);
	return 0;
}
