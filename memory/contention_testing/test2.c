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
#define CORE_NR 6

/*  This is for multi-thread task */

uint8_t *mem_chunk1;
uint8_t *mem_chunk2;

int **index_array;
int bank_index[6];
int access_index[64];
int ROUND_NR;
int thread_nr;

void cache_flush(uint8_t *address) {
        __asm__ volatile("clflush (%0)": :"r"(address) :"memory");
        return;
}


void *mem_hit(void *index_ptr) {
	volatile uint8_t next = 0;;
	int *index = (int *)index_ptr;
        cpu_set_t set;
        CPU_ZERO(&set);
        CPU_SET(*index, &set);
        if (sched_setaffinity(syscall(SYS_gettid), sizeof(cpu_set_t), &set)) {
                fprintf(stderr, "Error set affinity\n")  ;
                return NULL;
        }
	int i, j;
	int core_id = (*index)/2;
//	while (1) {
	time_t start_time,  end_time;
	start_time = time(NULL);
	for (j=0; j<ROUND_NR; j++) {
		for (i=0; i<SIZE/LINE_SIZE; i++){
			next += mem_chunk2[index_array[core_id][i]];
			cache_flush(&mem_chunk2[index_array[core_id][i]]);
		}
	}
	end_time = time(NULL);
	printf("Core %d: %lld\n", core_id, (long long int)end_time-(long long int)start_time);
}


uint64_t rdtsc(void) {
        uint64_t a, d;
        __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
        return (d<<32) | a;
}

int main(int argc, char* argv[]) {
	uint64_t mem_size = 1024*1024*1024;
	int fd1 = open("/mnt/hugepages/nebula3", O_CREAT|O_RDWR, 0755);
	int fd2 = open("/mnt/hugepages/nebula4", O_CREAT|O_RDWR, 0755);
	
	ROUND_NR = atoi(argv[1]);
	thread_nr = atoi(argv[2]);

	if ((fd1 < 0)||(fd2 < 0)) {
		printf("file open error!\n");
		return 0;
	}
	mem_chunk1 = mmap(0, mem_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd1, 0);
	mem_chunk2 = mmap(0, mem_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd2, 0);
	if ((mem_chunk1 == MAP_FAILED)||(mem_chunk2 == MAP_FAILED)) {
		printf("map error!\n");
		unlink("/mnt/hugepages/nebula3");
		unlink("/mnt/hugepages/nebula4");
		return 0;
	}

	int i, j;
	for (i=0; i<mem_size/sizeof(uint8_t); i++){
		mem_chunk1[i] = 3;
		mem_chunk2[i] = 4;
	}


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


	index_array = (int **)malloc(CORE_NR*sizeof(int*));
	for (i=0; i<CORE_NR; i++) {
		index_array[i] = (int *)malloc(SIZE/LINE_SIZE*sizeof(int));
	}

	int index_value;
	int k;
	for (k=0; k<thread_nr; k++) {
		i = 0;
		j = 0;
		while (j<SIZE/LINE_SIZE) {
			index_value = i * LINE_SIZE;
			if ((int)(index_value&access_index[63])== access_index[4]) {
				index_array[k][j] = index_value/sizeof(uint8_t);
				j++;
			}
			i++;
		}
	}

	for (k=thread_nr; k<CORE_NR; k++) {
		i = 0;
		j = 0;
		while (j<SIZE/LINE_SIZE) {
			index_value = i * LINE_SIZE;
			if ((int)(index_value&access_index[63])== access_index[k*4+2]) {
				index_array[k][j] = index_value/sizeof(uint8_t);
				j++;
			}
			i++;
		}
	}

/**********************Setp3: perform the attacks*****************************/

	int cpu_id_0 = 0;
	int cpu_id_1 = 2;
	int cpu_id_2 = 4;
	int cpu_id_3 = 6;
	int cpu_id_4 = 8;
	int cpu_id_5 = 10;
/*
	int cpu_id_6 = 12;
	int cpu_id_7 = 14;
	int cpu_id_8 = 16;
	int cpu_id_9 = 18;
	int cpu_id_10 = 20;
	int cpu_id_11 = 22;
*/

	pthread_t mem_thread_0;
	pthread_t mem_thread_1;
	pthread_t mem_thread_2;
	pthread_t mem_thread_3;
	pthread_t mem_thread_4;
	pthread_t mem_thread_5;
/*
	pthread_t mem_thread_6;
	pthread_t mem_thread_7;
	pthread_t mem_thread_8;
	pthread_t mem_thread_9;
	pthread_t mem_thread_10;
	pthread_t mem_thread_11;
*/
	pthread_create(&mem_thread_0, NULL, mem_hit, &cpu_id_0);
	pthread_create(&mem_thread_1, NULL, mem_hit, &cpu_id_1);
	pthread_create(&mem_thread_2, NULL, mem_hit, &cpu_id_2);
	pthread_create(&mem_thread_3, NULL, mem_hit, &cpu_id_3);
	pthread_create(&mem_thread_4, NULL, mem_hit, &cpu_id_4);
	pthread_create(&mem_thread_5, NULL, mem_hit, &cpu_id_5);
/*
	pthread_create(&mem_thread_6, NULL, mem_hit, &cpu_id_6);
	pthread_create(&mem_thread_7, NULL, mem_hit, &cpu_id_7);
	pthread_create(&mem_thread_8, NULL, mem_hit, &cpu_id_8);
	pthread_create(&mem_thread_9, NULL, mem_hit, &cpu_id_9);
	pthread_create(&mem_thread_10, NULL, mem_hit, &cpu_id_10);
	pthread_create(&mem_thread_11, NULL, mem_hit, &cpu_id_11);
*/
	pthread_join(mem_thread_0, NULL);
	pthread_join(mem_thread_1, NULL);
	pthread_join(mem_thread_2, NULL);
	pthread_join(mem_thread_3, NULL);
	pthread_join(mem_thread_4, NULL);
	pthread_join(mem_thread_5, NULL);
/*
	pthread_join(mem_thread_6, NULL);
	pthread_join(mem_thread_7, NULL);
	pthread_join(mem_thread_8, NULL);
	pthread_join(mem_thread_9, NULL);
	pthread_join(mem_thread_10, NULL);
	pthread_join(mem_thread_11, NULL);
*/
	return 0;
}
