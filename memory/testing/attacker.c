#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <pthread.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/syscall.h>

#define SIZE 256*1024
#define LINE_SIZE 64

uint8_t *mem_chunk;
uint64_t mem_size;

int **index_array;
int bank_index[6];
int access_index[64];
int cpu_index[12];

int thread_nr;

void cache_flush(uint8_t *address) {
        __asm__ volatile("clflush (%0)": :"r"(address) :"memory");
        return;
}


void *mem_access(void *index_ptr) {
	volatile uint8_t next = 0;;
	int *index = (int *)index_ptr;
        cpu_set_t set;
        CPU_ZERO(&set);
        CPU_SET(cpu_index[*index], &set);
        if (sched_setaffinity(syscall(SYS_gettid), sizeof(cpu_set_t), &set)) {
                fprintf(stderr, "Error set affinity\n")  ;
                return NULL;
        }
	int i, j;
	while (1) {
		for (i=0; i<SIZE/LINE_SIZE; i++){
			next += mem_chunk[index_array[*index][i]];
			cache_flush(&mem_chunk[index_array[*index][i]]);
		}
	}
	return;
}


uint64_t rdtsc(void) {
        uint64_t a, d;
        __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
        return (d<<32) | a;
}

int main(int argc, char* argv[]) {
	mem_size = 1024*1024*1024;
	int fd;
	thread_nr = atoi(argv[1]);
	int chunk_index = atoi(argv[2]);
	int channel_index = atoi(argv[3]);
	if (chunk_index == 1)
		fd = open("/mnt/hugepages/nebula1", O_CREAT|O_RDWR, 0755);
	if (chunk_index == 2)
		fd = open("/mnt/hugepages/nebula2", O_CREAT|O_RDWR, 0755);
	

	if (fd < 0) {
		printf("file open error!\n");
		return 0;
	}

	mem_chunk = mmap(0, mem_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

	if (mem_chunk == MAP_FAILED) {
		printf("map error!\n");
		if (chunk_index == 1)
			unlink("/mnt/hugepages/nebula3");
		if (chunk_index == 2)
			unlink("/mnt/hugepages/nebula2");
		return 0;
	}

	int i, j;
	for (i=0; i<mem_size/sizeof(uint8_t); i++)
		mem_chunk[i] = rand()%255;

	bank_index[0] = 6;
	bank_index[1] = 7;
	bank_index[2] = 15;
	bank_index[3] = 16;
	bank_index[4] = 20;
	bank_index[5] = 21;

	for (i=0; i<64; i++) {
		access_index[i] = (((i>>5)&0x1)<<bank_index[5]) +
				  (((i>>4)&0x1)<<bank_index[4]) +
				  (((i>>3)&0x1)<<bank_index[3]) +
				  (((i>>2)&0x1)<<bank_index[2]) +
				  (((i>>1)&0x1)<<bank_index[1]) +
				  (((i>>0)&0x1)<<bank_index[0]);
	}

	index_array = (int **)malloc(thread_nr*sizeof(int *));
	for (i=0; i<thread_nr; i++) {
		index_array[i] = (int *)malloc(SIZE/LINE_SIZE*sizeof(int));
	}


	int index_value;
	int k;
	for (k=0; k<thread_nr; k++) {
		i = 0;
		j = 0;
		while (j<SIZE/LINE_SIZE) {
			index_value = i * LINE_SIZE;
			if ((int)(index_value&access_index[63])== access_index[channel_index]) {
				index_array[k][j] = index_value/sizeof(uint8_t);
				j++;
			}
			i++;
		}
	}

	int cpu_id[12];
	pthread_t mem_thread[12];

	for (i=0; i<thread_nr; i++) {
		cpu_id[i] = i;
		cpu_index[i] = i*2+1;
		pthread_create(&mem_thread[i], NULL, mem_access, &cpu_id[i]);
	}
	
	printf("Begin Execution: \n");
	for (i=0; i<thread_nr; i++) {
		pthread_join(mem_thread[i], NULL);
	}

	return 0;
}
