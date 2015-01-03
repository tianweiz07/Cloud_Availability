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

#define ROUND_NR 100000000
#define STREAM_NR 100000
#define CPU_NR 8
#define SIZE 256*1024
#define LINE_SIZE 64

uint8_t *mem_chunk;

int bank_index[6];
int access_index[64];
int **index_array;
int cpu_index[CPU_NR];

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
	time_t start_time, end_time;
	start_time = time(NULL);
	for (j=0; j<STREAM_NR; j++) {
		for (i=0; i<SIZE/LINE_SIZE; i++) {
			next += mem_chunk[index_array[*index][i]];
			cache_flush(&mem_chunk[index_array[*index][i]]);
		}
	}
	end_time = time(NULL);
	total_time += end_time - start_time;

	return;
}

uint64_t rdtsc(void) {
        uint64_t a, d;
        __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
        return (d<<32) | a;
}

void row_index_identify() {
	int i;
	int bit, offset;
	volatile uint8_t next = 0;
	uint64_t start, end;
	printf("High timing indicates the row indexes:\n");
	for (bit=6; bit<30; bit++) {
		offset = (1<<bit)/sizeof(uint8_t);

		start = rdtsc();
		for (i=0; i<ROUND_NR; i++) {
			next += mem_chunk[0];
			cache_flush(&mem_chunk[0]);
			next -= mem_chunk[offset];
			cache_flush(&mem_chunk[offset]);
		}
		end = rdtsc();

		printf("Bit %d: %lu cycles\n", bit, (end - start));
	}
	return;
}

void bank_index_identify() {
	int i;
	int bit, offset;
	volatile uint8_t next = 0;
	uint64_t start, end;
	printf("Low timing indicates the bank indexes:\n");
	for (bit=1; bit<30; bit++) {
		offset = ((1<<23) + (1<<bit))/sizeof(uint8_t);

		start = rdtsc();
		for (i=0; i<ROUND_NR; i++) {
			next += mem_chunk[0];
			cache_flush(&mem_chunk[0]);
			next -= mem_chunk[offset];
			cache_flush(&mem_chunk[offset]);
		}
		end = rdtsc();

		printf("Bit %d: %lu cycles\n", bit, (end - start));
	}
	return;
}

void bank_xor_identify() {
	bank_index[0] = 6;
	bank_index[1] = 7;
	bank_index[2] = 15;
	bank_index[3] = 16;
	bank_index[4] = 20;
	bank_index[5] = 21;

	int i;
	int bit1, bit2, offset;
	volatile uint8_t next = 0;
	uint64_t start, end;
	printf("High timing indicates the XOR indexes:\n");
	for (bit1=0; bit1<5; bit1++) {
		for (bit2=bit1+1; bit2<6; bit2++) {
			offset = ((1<<bank_index[bit1]) + (1<<bank_index[bit2]))/sizeof(uint8_t);
			start = rdtsc();
			for (i=0; i<ROUND_NR; i++) {
				next += mem_chunk[0];
				cache_flush(&mem_chunk[0]);
				next -= mem_chunk[offset];
				cache_flush(&mem_chunk[offset]);
			}
			end = rdtsc();
			printf("Bit %d XOR %d: %lu cycles\n", bank_index[bit1],bank_index[bit2], (end - start));
		}
	}
	return;
}

void channel_index_identify() {
	int i, j, k;

	bank_index[0] = 6;
	bank_index[1] = 7;
	bank_index[2] = 15;
	bank_index[3] = 16;
	bank_index[4] = 20;
	bank_index[5] = 21;

	for (i=0; i<64; i++) {
		access_index[i] = (((i>>5)&0x1)<<bank_index[5])/sizeof(uint8_t) +
				  (((i>>4)&0x1)<<bank_index[4])/sizeof(uint8_t) +
				  (((i>>3)&0x1)<<bank_index[3])/sizeof(uint8_t) +
				  (((i>>2)&0x1)<<bank_index[2])/sizeof(uint8_t) +
				  (((i>>1)&0x1)<<bank_index[1])/sizeof(uint8_t) +
				  (((i>>0)&0x1)<<bank_index[0])/sizeof(uint8_t);
	}

	index_array = (int **)malloc(CPU_NR*sizeof(int *));
	for (i=0; i<CPU_NR; i++) 
		index_array[i] = (int *)malloc(SIZE/LINE_SIZE*sizeof(int));

	int index_value;
	int channel_index = 0;

	printf("Low timing indicates the channel indexes:\n");
	for (k=0; k<CPU_NR/2; k++) {
		i = 0; 
		j = 0;
		while (j<SIZE/LINE_SIZE) {
			index_value = i * LINE_SIZE;
			if ((int)((index_value>>bank_index[channel_index])&0x1) == 0) {
				index_array[k][j] = index_value/sizeof(uint8_t);
				j++;
			}
			i++;
		}
	}

	for (k=CPU_NR/2; k<CPU_NR; k++) {
		i = 0; 
		j = 0;
		while (j<SIZE/LINE_SIZE) {
			index_value = i * LINE_SIZE;
			if ((int)((index_value>>bank_index[channel_index])&0x1) == 1) {
				index_array[k][j] = index_value/sizeof(uint8_t);
				j++;
			}
			i++;
		}
	}

	for (k=0; k<CPU_NR/2; k++)
		cpu_index[k] = k*2;

	for (k=CPU_NR/2; k<CPU_NR; k++) 
		cpu_index[k] = (k-CPU_NR/2)*2+1;

	total_time = 0;
	int cpu_id[CPU_NR];
	pthread_t mem_thread[CPU_NR];

	for (i=0; i<CPU_NR; i++) {
		cpu_id[i] = i;
		pthread_create(&mem_thread[i], NULL, mem_access, &cpu_id[i]);
	}
	
	for (i=0; i<CPU_NR; i++) {
		pthread_join(mem_thread[i], NULL);
	}

	printf("Bit %d: %lld seconds\n", bank_index[channel_index], (long long int)total_time);

	return;

}


int main(int argc, char* argv[]) {

	uint64_t mem_size = 1024*1024*1024;
	int fd = open("/mnt/hugepages/nebula1", O_CREAT|O_RDWR, 0755);
	
	if (fd < 0) {
		printf("file open error!\n");
		return 0;
	}
	mem_chunk = mmap(0, mem_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (mem_chunk == MAP_FAILED) {
		printf("map error!\n");
		unlink("/mnt/hugepages/nebula1");
		return 0;
	}

	int i;
	for (i=0; i<mem_size/sizeof(uint8_t); i++)
		mem_chunk[i] = 1;

	int mode = atoi(argv[1]);
	switch(mode) {
		case 0:
			row_index_identify();
			break;
		case 1:
			bank_index_identify();
			break;
		case 2:
			bank_xor_identify();
			break;
		case 3:
			channel_index_identify();
			break;
	}
	
	munmap(mem_chunk, mem_size);
	return 0;

/**********************Setp1: identify the row indexes (longer time)*****************************/
/**********************Setp2: identify the bank indexes (shoter time)*****************************/

/**********************Setp3: identify the XOR relation*****************************/

/**********************Setp3: perform the attacks*****************************/
/*

	int cpu_id_0 = 0;
	int cpu_id_1 = 1;
	int cpu_id_2 = 2;
	int cpu_id_3 = 3;
	int cpu_id_4 = 4;
	int cpu_id_5 = 5;
	int cpu_id_6 = 6;
	int cpu_id_7 = 7;
	pthread_t mem_thread_0;
	pthread_t mem_thread_1;
	pthread_t mem_thread_2;
	pthread_t mem_thread_3;
	pthread_t mem_thread_4;
	pthread_t mem_thread_5;
	pthread_t mem_thread_6;
	pthread_t mem_thread_7;

	pthread_create(&mem_thread_0, NULL, mem_hit, &cpu_id_0);
	pthread_create(&mem_thread_1, NULL, mem_hit, &cpu_id_1);
	pthread_create(&mem_thread_2, NULL, mem_hit, &cpu_id_2);
	pthread_create(&mem_thread_3, NULL, mem_hit, &cpu_id_3);
	pthread_create(&mem_thread_4, NULL, mem_hit, &cpu_id_4);
	pthread_create(&mem_thread_5, NULL, mem_hit, &cpu_id_5);
	pthread_create(&mem_thread_6, NULL, mem_hit, &cpu_id_6);
	pthread_create(&mem_thread_7, NULL, mem_hit, &cpu_id_7);
	pthread_join(mem_thread_0, NULL);
	pthread_join(mem_thread_1, NULL);
	pthread_join(mem_thread_2, NULL);
	pthread_join(mem_thread_3, NULL);
	pthread_join(mem_thread_4, NULL);
	pthread_join(mem_thread_5, NULL);
	pthread_join(mem_thread_6, NULL);
	pthread_join(mem_thread_7, NULL);
*/
}
