#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sched.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sched.h>
#include <sys/types.h>

#define line_size 64

int *array;
int *target_array;
int *index_array;

int main(int argc, char *argv[]) {

        cpu_set_t set;
        CPU_ZERO(&set);
        CPU_SET(0, &set);
        if (sched_setaffinity(syscall(SYS_gettid), sizeof(cpu_set_t), &set)) {
                fprintf(stderr, "Error set affinity\n")  ;
                return 0;
        }

	int mem_size = 1048576;
	int interval = atoi(argv[1]);
	int size = atoi(argv[2])/2;

        int fd = open("nebula1", O_RDWR, 0755);
        if (fd < 0) {
                printf("file open error!\n");
                return 0;
        }
        array = mmap(NULL, mem_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if (array == MAP_FAILED) {
                printf("map 1 error!\n");
                unlink("nebula1");
                return 0;
        }

	target_array = &array[size/sizeof(int)];

	index_array = (int *)malloc(size/line_size*sizeof(int));

	int i, j;
	for (i=0; i<size/line_size; i++)
		index_array[i] = i*line_size/sizeof(int);

	for (i=0; i <size/sizeof(int); i++) {
		array[i] = rand();
		target_array[i] = rand();
	}

	time_t finish_time;
	int index;
	while (1) {
		index = 0;
		finish_time = time(NULL) + interval;
		while(time(NULL)<finish_time) {
			index ++;
			for (j=0; j < size/line_size; j++)
				target_array[index_array[j]] = array[index_array[j]];
		}
		printf("%d\n", index);
	}

	return 0;
}
