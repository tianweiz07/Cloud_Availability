#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#define __NR_MemoryHit 188

int main(int argc, char* argv[]) {
	int time = atoi(argv[1]);
	printf("Memory Hit Start!\n");
	syscall(__NR_MemoryHit, time);
	printf("Memory Hit Done!\n");
	return 0;
}
