#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#define __NR_CacheSearch 188
#define __NR_CacheClean 189

int main(int argc, char* argv[]) {
	int i, j;
	pid_t pid1;
//	pid_t pid1, pid2, pid3;
	int cpu_id;
	int mode = atoi(argv[1]);
	int time = atoi(argv[2]);
	unsigned long num;
	unsigned long **timestamp;
	timestamp = (unsigned long**)malloc(1024*sizeof(unsigned long*));
	for (i=0; i<1024; i++)
		timestamp[i] = (unsigned long*)malloc(6*sizeof(unsigned long));
	pid1 = fork();
//	pid2 = fork();
//	pid3 = fork();
//	if ((pid1<0)||(pid2<0)||(pid3<0)) {
	if (pid1<0){
		printf("fork error\n");
		return 0;
	}

//	cpu_id = ((pid1>0)<<2)+((pid2>0)<<1)+(pid3>0);
	cpu_id = (pid1>0);
	printf("Cache Clean Start!\n");
	syscall(__NR_CacheClean, mode, time, timestamp, &num, cpu_id);

	printf("Cache Clean Done!\n");
	if (mode == 0) 
		for (i=0; i<1024; i++) {
			for (j=0; j<6; j++)
				printf("%lu ", timestamp[i][j]/num);
			printf("\n");
		}

	printf("num is: %lu\n", num);
	return 0;
}
