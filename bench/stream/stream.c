#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/syscall.h>

#define CACHE_LINE_SIZE 	64
#define MAX_CORE_NR		12

double total_time;
int *source_array;
int *dest_array;
int *index_array;

int num_thread;
int traversals;
int size;
int locality;

uint64_t rdtsc(void) {
        uint64_t a, d;
        __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
        return (d<<32) | a;
}

int initial(int flag) {
	int i;
	uint64_t mem_size = size;

        source_array = mmap(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	
        if (source_array == MAP_FAILED) {
                printf("map source buffer error!\n");
                return 0;
        }

        dest_array = mmap(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);

        if (dest_array == MAP_FAILED) {
                printf("map destination buffer error!\n");
                return 0;
        }

	index_array = (int *)malloc(size/CACHE_LINE_SIZE*sizeof(int));

	for (i=0; i<size/CACHE_LINE_SIZE; i++)
		index_array[i] = i*CACHE_LINE_SIZE/sizeof(int);


	/* randomize the index to decrease the locality */
	int index1, index2, temp_index;
	if (flag == 1) {
		for (i=0; i<size/CACHE_LINE_SIZE; i++) {
			index1 = rand()%(size/CACHE_LINE_SIZE);
			index2 = rand()%(size/CACHE_LINE_SIZE);
			temp_index = index_array[index1];
			index_array[index1] = index_array[index2];
			index_array[index2] = temp_index;
		}
	}

	for (i=0; i <size/sizeof(int); i++) {
		source_array[i] = rand();
		dest_array[i] = rand();
	}
	return 0;
}

void *stream_access(void *index_ptr) {
	int *index = (int *)index_ptr;
	/* Allocate the threads to the physical cores */  
	cpu_set_t set;
	CPU_ZERO(&set);
 	CPU_SET(*index, &set);
	if (sched_setaffinity(syscall(SYS_gettid), sizeof(cpu_set_t), &set)) {
		fprintf(stderr, "Error set affinity\n")  ;
		return NULL;
	}

	int i, j, k;

	struct timeval start_time;
	struct timeval end_time;

	gettimeofday(&start_time, NULL);
  
	for (i=0; i < traversals; i++) {
		for (j=size/CACHE_LINE_SIZE/num_thread*((*index)); j < size/CACHE_LINE_SIZE/num_thread*((*index)+1); j++) {
		dest_array[index_array[j]] = source_array[index_array[j]];
		}
	}

	gettimeofday(&end_time, NULL);
	total_time += (end_time.tv_sec-start_time.tv_sec) + (end_time.tv_usec-start_time.tv_usec)/1000000.0;
}

int main(int argc, char *argv[]) {
	num_thread = atoi(argv[1]);	/* Number of threads for the victim */
	traversals = atoi(argv[2]);	/* Total execution time */
	size = atoi(argv[3])/2;		/* buffer size */
	locality = atoi(argv[4]);	/* buffer locallity */

	total_time = 0;
	int cpu_id[MAX_CORE_NR];
	pthread_t mem_thread[MAX_CORE_NR];


	initial(locality);

	int thread_index = num_thread;

	int i;
	/* Allocate threads for victim */
	for (i=0; i<num_thread; i++) {
		cpu_id[i] = i;
	        pthread_create(&mem_thread[i], NULL, stream_access, &cpu_id[i]);
	}

	/* Execute the threads */
	for (i=0; i<num_thread; i++) {
	        pthread_join(mem_thread[i], NULL);
	}

	printf("%f\n", total_time);
	munmap(source_array, size);
	munmap(dest_array, size);
	return 0;
}
