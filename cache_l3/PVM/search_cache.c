#include <stdlib.h>
#include <stdio.h>
#include <sys/syscall.h>

#define __NR_CacheSearch 188
#define __NR_CacheClean 189


int main(int argc, char* argv[]) {
	int error_num;
	printf("Cache Search Start!\n");
	syscall(__NR_CacheSearch, &error_num);
	printf("Cache Search Done!\n");
	printf("# of error sets: %d\n", error_num);
	return 0;
}
