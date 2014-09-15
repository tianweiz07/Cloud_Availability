#include <stdlib.h>
#include <stdio.h>
#include <sys/syscall.h>

#define __NR_CachePrime 188

int main(int argc, char* argv[]) {
	syscall(__NR_CachePrime);
	return 0;
}
