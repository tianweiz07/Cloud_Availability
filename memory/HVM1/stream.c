#define _POSIX_C_SOURCE 199309
#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>

#define ASSOC 20
#define SLICES 6
#define WAY_SIZE 131072
#define LINE_SIZE 64
#define SIZE ((ASSOC+1)*SLICES*WAY_SIZE)

#define TRIALS 1

#define MISS_NR 21
#define WAY_NR (MISS_NR*SLICES)
#define SET_NR (WAY_SIZE/LINE_SIZE)

char* buf;

uint64_t rdtsc(void) {
        uint64_t a, d;
        __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
        return (d<<32) | a;
}

void prime(char *ptr) {
   __asm__("mov %0,%%r8\n\t"
           "mov %%r8,%%rsi\n\t"
           "xor %%eax, %%eax\n\t"
           "cpuid\n\t"
           "loop: mov (%%r8), %%r8\n\t"
           "cmp %%r8,%%rsi\n\t"
           "jne loop\n\t"
           "xor %%eax, %%eax\n\t"
           "cpuid\n\t"
           :
           :"r"(ptr)
           :"esi","r8","eax");
}

void *stream_access(void *index_ptr) {
	int *index = (int *)index_ptr;
	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET(*index, &set);
	if (sched_setaffinity(syscall(SYS_gettid), sizeof(cpu_set_t), &set)) {
		fprintf(stderr, "Error set affinity\n")  ;
		return NULL;
	}

	int i, j, k;
	char **ptr1, **ptr2;
	char *tmp;
	uint64_t start, end;
	

	for (i=SET_NR/4*(*index); i<SET_NR/4*(*index+1); i++) {
		for (j=0; j<WAY_NR; j++){
			ptr1 = (char **)&buf[i*LINE_SIZE+j*WAY_SIZE];
			*ptr1 = (char*)ptr1;
        	}
		for (j=WAY_NR-1; j>=1; j--){
			k = rand()%j;
			ptr1 = (char **)&buf[i*LINE_SIZE+j*WAY_SIZE];
			ptr2 = (char **)&buf[i*LINE_SIZE+k*WAY_SIZE];
			tmp = *ptr1;
			*ptr1 = *ptr2;
			*ptr2 = tmp;
		}
	}

	k = 0;
	start = rdtsc();
	while (k < TRIALS){
		for (i=SET_NR/4*(*index); i<SET_NR/4*(*index+1); i++) {
			prime(&buf[i*LINE_SIZE]);
			k++;
		}
	}
	end = rdtsc();
	printf("#%d: %ld\n", (*index), (end - start)/TRIALS/WAY_NR);
	return;
} 


int main (int argc, char *argv[]) {
	int seed = 0;
	srand(seed);

	uint64_t buf_size = 1024*1024*1024;
	int fd = open("/mnt/hugepages/nebula1", O_CREAT|O_RDWR, 0755);
	if (fd<0) {
		printf("file open error!\n");
		return 0;
	}

	buf = mmap(0, buf_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (buf == MAP_FAILED) {
		printf("map error!\n");
		unlink("/mnt/hugepages/nebula1");
		return 0;
	}


        int cpu_id_0 = 0;
        int cpu_id_1 = 1;
        int cpu_id_2 = 2;
        int cpu_id_3 = 3;
        pthread_t mem_thread_0;
        pthread_t mem_thread_1;
        pthread_t mem_thread_2;
        pthread_t mem_thread_3;

        pthread_create(&mem_thread_0, NULL, stream_access, &cpu_id_0);
        pthread_create(&mem_thread_1, NULL, stream_access, &cpu_id_1);
        pthread_create(&mem_thread_2, NULL, stream_access, &cpu_id_2);
        pthread_create(&mem_thread_3, NULL, stream_access, &cpu_id_3);

        pthread_join(mem_thread_0, NULL);
        pthread_join(mem_thread_1, NULL);
        pthread_join(mem_thread_2, NULL);
        pthread_join(mem_thread_3, NULL);

	munmap(buf, 1024*1024*1024);
}
