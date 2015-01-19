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

int size;

time_t total_time;
int *array;
int *target_array;
int *array1;
int *target_array1;
int *index_array;

int num_thread;
int mode;

uint64_t rdtsc(void) {
        uint64_t a, d;
        __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
        return (d<<32) | a;
}

int initial() {
	int i, j;
	uint64_t mem_size = 1024*1024*1024;
	int fd, fd1;
        fd = open("/mnt/hugepages/nebula1", O_CREAT|O_RDWR, 0755);
       	if (fd < 0) {
               	printf("file open error!\n");
                return 0;
        }
        array = mmap(0, mem_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	
        if (array == MAP_FAILED) {
       	        printf("map 1 error!\n");
               	unlink("/mnt/hugepages/nebula1");
                return 0;
        }
	target_array = &array[size/sizeof(int)];

        fd1 = open("/mnt/hugepages/nebula5", O_CREAT|O_RDWR, 0755);
        if (fd1 < 0) {
                printf("file open error!\n");
                return 0;
        }
        array1 = mmap(0, mem_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd1, 0);

        if (array1 == MAP_FAILED) {
                printf("map 2 error!\n");
                unlink("/mnt/hugepages/nebula5");
                return 0;
        }
	target_array1 = &array1[size/sizeof(int)];

	index_array = (int *)malloc(size/line_size*sizeof(int));

	for (i=0; i<size/line_size; i++)
		index_array[i] = i*line_size/sizeof(int);


	for (i=0; i <size/sizeof(int); i++) {
		array[i] = rand();
		target_array[i] = rand();
		array1[i] = rand();
		target_array1[i] = rand();
	}
	return 0;
}

void *stream_access(void *index_ptr) {
	int *index = (int *)index_ptr;
	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET((*index*2+1), &set);
	if (sched_setaffinity(syscall(SYS_gettid), sizeof(cpu_set_t), &set)) {
		fprintf(stderr, "Error set affinity\n");
		return NULL;
	}

	int i, j, k;
	time_t start_time;

	start_time = time(NULL);
	if ((*index)<mode) {
		printf("access nebula1\n");
		while (time(NULL) - start_time < total_time) {
			for (j=size/line_size/num_thread*(*index); j < size/line_size/num_thread*(*index+1); j++) {
				target_array[index_array[j]] = array[index_array[j]];
			}
		}
	}
	else {
		printf("access nebula2\n");
		while (time(NULL) - start_time < total_time) {
			for (j=size/line_size/num_thread*(*index); j < size/line_size/num_thread*(*index+1); j++) {
				target_array1[index_array[j]] = array1[index_array[j]];
			}
		}
	}
	return;
}

int main(int argc, char *argv[]) {
	int cpu_id[12];
	pthread_t mem_thread[12];


	num_thread = atoi(argv[1]);
	total_time = atoi(argv[2]);
	size = 1024*1024*atoi(argv[3]);

	mode = atoi(argv[4]);

	initial();
	int thread_index = num_thread;

	if ((thread_index > 12)||(thread_index < 0)) {
		printf("# of Threads are not corrected\n");
		return 0;
	}

	int i;
	for (i=0; i<num_thread; i++) {
		cpu_id[i] = i;
	        pthread_create(&mem_thread[i], NULL, stream_access, &cpu_id[i]);
	}

	for (i=0; i<num_thread; i++) {
	        pthread_join(mem_thread[i], NULL);
	}

	return 0;
}
