#define _POSIX_C_SOURCE 199309
#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>

#define L1_CACHE_SET_NR 64
#define L1_CACHE_WAY_NR 8
#define L1_CACHE_LINE_SIZE 64
#define L1_CACHE_SIZE (L1_CACHE_LINE_SIZE*L1_CACHE_WAY_NR*L1_CACHE_SET_NR)

#define L2_CACHE_SET_NR 512
#define L2_CACHE_WAY_NR 8
#define L2_CACHE_LINE_SIZE 64
#define L2_CACHE_SIZE (L2_CACHE_LINE_SIZE*L2_CACHE_WAY_NR*L2_CACHE_SET_NR)

char *l1_icache_array;
uint64_t *l1_dcache_array;
uint64_t *l2_cache_array;

uint64_t rdtsc(void) {
        uint64_t a, d;
        __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
        return (d<<32) | a;
}

void cache_init() {
	int i, j;
	for (i=0; i<L1_CACHE_SET_NR; i++) {
		for (j=0; j<L1_CACHE_WAY_NR-1; j++) {
			l1_dcache_array[(L1_CACHE_SET_NR*j+i)*L1_CACHE_LINE_SIZE/sizeof(uint64_t)] = (uint64_t)(&l1_dcache_array[(L1_CACHE_SET_NR*(j+1)+i)*L1_CACHE_LINE_SIZE/sizeof(uint64_t)]);
		}
		l1_dcache_array[(L1_CACHE_SET_NR*(L1_CACHE_WAY_NR-1)+i)*L1_CACHE_LINE_SIZE/sizeof(uint64_t)] = 0x0;
	}

	for (i=0; i<L1_CACHE_SET_NR; i++) {
		for (j=0; j<L1_CACHE_WAY_NR-1; j++) {
			l1_icache_array[(L1_CACHE_SET_NR*j+i)*L1_CACHE_LINE_SIZE/sizeof(char)] = 0xe9;
			l1_icache_array[(L1_CACHE_SET_NR*j+i)*L1_CACHE_LINE_SIZE/sizeof(char)+1] = 0xfb;
			l1_icache_array[(L1_CACHE_SET_NR*j+i)*L1_CACHE_LINE_SIZE/sizeof(char)+2] = 0x0f;
			l1_icache_array[(L1_CACHE_SET_NR*j+i)*L1_CACHE_LINE_SIZE/sizeof(char)+3] = 0x00;
			l1_icache_array[(L1_CACHE_SET_NR*j+i)*L1_CACHE_LINE_SIZE/sizeof(char)+4] = 0x00;
		}
		l1_icache_array[(L1_CACHE_SET_NR*(L1_CACHE_WAY_NR-1)+i)*L1_CACHE_LINE_SIZE/sizeof(char)] = 0xc3;
	}

	for (i=0; i<L2_CACHE_SET_NR; i++) {
		for (j=0; j<L2_CACHE_WAY_NR-1; j++) {
			l2_cache_array[(L2_CACHE_SET_NR*j+i)*L2_CACHE_LINE_SIZE/sizeof(uint64_t)] = (uint64_t)(&l2_cache_array[(L2_CACHE_SET_NR*(j+1)+i)*L2_CACHE_LINE_SIZE/sizeof(uint64_t)]);
		}
		l2_cache_array[(L2_CACHE_SET_NR*(L2_CACHE_WAY_NR-1)+i)*L2_CACHE_LINE_SIZE/sizeof(uint64_t)] = 0x0;
	}
	return;
}


void clean_l1_dcache(void) {
	int i;
	uint64_t * block_addr;
	for (i=0; i<L1_CACHE_SET_NR; i++) {
		block_addr = &l1_dcache_array[i*L1_CACHE_LINE_SIZE/sizeof(uint64_t)];
		__asm__ volatile("mov %0, %%r9\n\t" 
				 "mov (%%r9), %%r9\n\t" 
				 "mov (%%r9), %%r9\n\t" 
				 "mov (%%r9), %%r9\n\t" 
				 "mov (%%r9), %%r9\n\t" 
				 "mov (%%r9), %%r9\n\t" 
				 "mov (%%r9), %%r9\n\t" 
				 "mov (%%r9), %%r9\n\t" 
				 "mov (%%r9), %%r9\n\t" 
				 : 
				 :"r"(block_addr)
				 : );
	}
        return;
}

void clean_l1_icache(void) {
	int i;
	char * block_addr;
	uint64_t temp;
	for (i=0; i<L1_CACHE_SET_NR; i++) {
		block_addr = l1_icache_array+i*L1_CACHE_LINE_SIZE/sizeof(char);
		__asm__ volatile("call *%0": :"r"(block_addr) : );
	}
	temp = rdtsc();
        return;
}

void clean_l2_cache(void) {
	int i;
	uint64_t * block_addr;
	for (i=0; i<L2_CACHE_SET_NR; i++) {
		block_addr = &(l2_cache_array[i*L2_CACHE_LINE_SIZE/sizeof(uint64_t)]);
		__asm__ volatile("mov %0, %%r9\n\t" 
				 "mov (%%r9), %%r9\n\t" 
				 "mov (%%r9), %%r9\n\t" 
				 "mov (%%r9), %%r9\n\t" 
				 "mov (%%r9), %%r9\n\t" 
				 "mov (%%r9), %%r9\n\t" 
				 "mov (%%r9), %%r9\n\t" 
				 "mov (%%r9), %%r9\n\t" 
				 "mov (%%r9), %%r9\n\t" 
				 : 
				 :"r"(block_addr)
				 : );
	}
        return;
}

int main (int argc, char *argv[]) {
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(0, &mask);
	sched_setaffinity(0, sizeof(cpu_set_t), &mask);

	int mode = atoi(argv[1]);
	int period = atoi(argv[2]);

	l1_icache_array = mmap(NULL, L1_CACHE_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
	l1_dcache_array = mmap(NULL, L1_CACHE_SIZE, PROT_READ|PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
	l2_cache_array = mmap(NULL, L2_CACHE_SIZE, PROT_READ|PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);

	if ((l1_icache_array == MAP_FAILED)||(l1_dcache_array == MAP_FAILED)||(l2_cache_array == MAP_FAILED)) {
		printf("map error!\n");
		return 0;
	}

	cache_init();

	time_t end_time = time(NULL) + period;
	switch(mode) {
		case 0:
			while (time(NULL) < end_time)
				clean_l1_dcache();
			break;
		case 1:
			while (time(NULL) < end_time)
				clean_l1_icache();
			break;
		case 2:
			while (time(NULL) < end_time) {
				clean_l1_dcache();
				clean_l1_icache();
			}
			break;
		case 3:
			while (time(NULL) < end_time)
				clean_l2_cache();
			break;
	}
	munmap(l1_icache_array, L1_CACHE_SIZE);
	munmap(l1_dcache_array, L1_CACHE_SIZE);
	munmap(l2_cache_array, L2_CACHE_SIZE);
	return 0;
}
