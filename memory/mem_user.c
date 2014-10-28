#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/resource.h>


#define ROUND_NR 1000000000

void cache_flush(uint8_t *address) {
        __asm__ volatile("clflush (%0)"
                         :
                         :"r"(address)
                         :"memory"
                        );
        return;
}

/*void single_access(uint8_t *addr, uint8_t value)
{
        __asm__ volatile(
                "mov %0, %%r9\n\t"
                "mov %1, %%r10\n\t"
                "mov %%r10, (%%r9)\n\t"
                "lfence\n\t"
                :
                :"r"(addr), "r"(value)
                :
                );
        return;
}
*/

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

	int i;
	for (i=0; i<mem_size/sizeof(uint8_t); i++)
		mem_chunk[i] = 1;

	int bit, offset;
	volatile uint8_t next = 0;;
	uint64_t start, end;

	for (bit=6; bit<30; bit++) {
		offset = (1<<bit)/sizeof(uint8_t);

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

	return 0;
}
