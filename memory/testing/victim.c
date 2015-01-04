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

int ROUND_NR;
int thread_nr;

time_t total_time;

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
	time_t start_time,  end_time;
	start_time = time(NULL);
	for (j=0; j<ROUND_NR; j++) {
		for (i=0; i<SIZE/LINE_SIZE; i++){
			next += mem_chunk[index_array[*index][i]];
			cache_flush(&mem_chunk[index_array[*index][i]]);
		}
	}
	end_time = time(NULL);
	printf("Core %d: %lld\n", *index, (long long int)end_time-(long long int)start_time);
	total_time += end_time - start_time;
	return;
}


uint64_t rdtsc(void) {
        uint64_t a, d;
        __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
        return (d<<32) | a;
}

int main(int argc, char* argv[]) {
	mem_size = 1024*1024*1024;
	int fd = open("/mnt/hugepages/nebula3", O_CREAT|O_RDWR, 0755);
	
	thread_nr = atoi(argv[1]);
	ROUND_NR = atoi(argv[2]);

	if (fd < 0) {
		printf("file open error!\n");
		return 0;
	}
	mem_chunk = mmap(0, mem_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

	if (mem_chunk == MAP_FAILED) {
		printf("map error!\n");
		unlink("/mnt/hugepages/nebula3");
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
			if ((int)(index_value&access_index[63])== 0) {
				index_array[k][j] = index_value/sizeof(uint8_t);
				j++;
			}
			i++;
		}
	}


	int if_rand = atoi(argv[3]);
	if (if_rand == 1) {
		int temp_index1, temp_index2, temp_value;
		for (i=0; i<SIZE/LINE_SIZE/2; i++) {
			for (k=0; k<thread_nr; k++) {
				temp_index1 = rand()%(SIZE/LINE_SIZE);
				temp_index2 = rand()%(SIZE/LINE_SIZE);
				temp_value = index_array[k][temp_index1];
				index_array[k][temp_index1] = index_array[k][temp_index2];
				index_array[k][temp_index2] = temp_value;
			}
		}
	}
	
	total_time = 0;

	int cpu_id[12];
	pthread_t mem_thread[12];

	for (i=0; i<thread_nr; i++) {
		cpu_id[i] = i;
		cpu_index[i] = i*2;
		pthread_create(&mem_thread[i], NULL, mem_access, &cpu_id[i]);
	}
	
	printf("Begin Execution: \n");
	for (i=0; i<thread_nr; i++) {
		pthread_join(mem_thread[i], NULL);
	}

	printf("Total Time: %lld\n", (long long int)total_time);

	return 0;
}
