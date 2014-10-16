#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#define __NR_CacheSearch 188
#define __NR_CacheClean 189

int main(int argc, char* argv[]) {
	int i, j;

	int time1 = atoi(argv[1]);
	int time2 = atoi(argv[2]);
	unsigned long threshold = atoi(argv[3]);

	unsigned long num1 = 0;
	unsigned long num2 = 0;
	unsigned long **timestamp;
	timestamp = (unsigned long**)malloc(2048*sizeof(unsigned long*));
	for (i=0; i<2048; i++)
		timestamp[i] = (unsigned long*)malloc(6*sizeof(unsigned long));

	printf("Cache Clean Start!\n");
	syscall(__NR_CacheClean, 0, time1, timestamp, &num1, threshold);
	syscall(__NR_CacheClean, 1, time2, timestamp, &num2, threshold);

	printf("Cache Clean Done!\n");
		for (i=0; i<2048; i++) {
			for (j=0; j<6; j++)
				printf("%lu ", timestamp[i][j]);
			printf("\n");
		}

	printf("num1 is: %lu\n", num1);
	printf("num2 is: %lu\n", num2);
	return 0;
}
