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
int *index_array;

int num_thread;

uint64_t rdtsc(void) {
        uint64_t a, d;
        __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
        return (d<<32) | a;
}

int initial() {
	int i, j;
	uint64_t mem_size = 1024*1024*1024;
        int fd = open("/mnt/hugepages/nebula1", O_CREAT|O_RDWR, 0755);
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

        int fd1 = open("/mnt/hugepages/nebula3", O_CREAT|O_RDWR, 0755);
        if (fd1 < 0) {
                printf("file open error!\n");
                return 0;
        }
        target_array = mmap(0, mem_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd1, 0);

        if (target_array == MAP_FAILED) {
                printf("map 2 error!\n");
                unlink("/mnt/hugepages/nebula3");
                return 0;
        }

	index_array = (int *)malloc(size/line_size*sizeof(int));

	for (i=0; i<size/line_size; i++)
		index_array[i] = i*line_size/sizeof(int);


	for (i=0; i <size/sizeof(int); i++) {
		array[i] = rand();
		target_array[i] = rand();
	}
	return 0;
}

void *stream_access(void *index_ptr) {
	int *index = (int *)index_ptr;
	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET((*index)*2, &set);
	if (sched_setaffinity(syscall(SYS_gettid), sizeof(cpu_set_t), &set)) {
		fprintf(stderr, "Error set affinity\n");
		return NULL;
	}

	int i, j, k;
	time_t start_time;

	start_time = time(NULL);

	int start_index, end_index;

 	if ((*index)<(num_thread/2)) {
		start_index = size/line_size/(num_thread/2)*(*index);
		end_index = size/line_size/(num_thread/2)*(*index+1)-1;
		while (time(NULL) - start_time < total_time) {
			for (j=start_index; j<end_index; j++) {
				array[index_array[j+1]] += array[index_array[j]];
			}
		}
	}
	else{
		start_index = size/line_size/(num_thread/2)*(*index-num_thread/2);
		end_index = size/line_size/(num_thread/2)*(*index-num_thread/2+1)-1;
		while (time(NULL) - start_time < total_time) {
			for (j=start_index; j<end_index; j++) {
				target_array[index_array[j+1]] += target_array[index_array[j]];
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
