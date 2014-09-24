#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#define __NR_CachePrime 188
#define TIME 10

int main(int argc, char* argv[]) {
	syscall(__NR_CachePrime, TIME);
	printf("Bus Lock Done!\n");
	return 0;
}
