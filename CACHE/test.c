#include <stdlib.h>
#include <stdio.h>
#include <sys/syscall.h>

#define __NR_CachePrime 188

#define CACHE_SET_NR 30720

int main(int argc, char* argv[]) {
	int t1 = 200;
	int t2 = 550;
	unsigned long *timestamp;
	int *flag;
	timestamp = (unsigned long*)malloc(CACHE_SET_NR*sizeof(unsigned long));
	flag = (int*)malloc(CACHE_SET_NR*sizeof(int));
	syscall(__NR_CachePrime, t1, t2, timestamp, flag);

	int i;
	int count = 0;
	for (i=0; i<CACHE_SET_NR; i++) {
		if (flag[i]==0)
			count ++;
	}			
	printf("count: %d\n", count);
	return 0;
}
