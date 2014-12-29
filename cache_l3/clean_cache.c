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

#define NUM_SET 20
#define NUM_CPU 4

char *buf;
char *head[NUM_SET];
char *tail[NUM_SET];


int set_index[NUM_SET]   = {832,833,834,835,836,837,838,839,840,841,842,843,844,845,846,847,848,849,850,851};
int slice_index[NUM_SET] = {1,  5,  0,  3,  3,  4,  1,  4,  1,  0,  0,  1,  3,  4,  1,  4,  5,  1,  4,  0};
int assoc_index[NUM_SET] = {20, 20, 19, 19, 20, 20, 19, 20, 20, 20, 19, 20, 20, 20, 19, 20, 20, 20, 19, 19};

unsigned int conflict_sets[NUM_SET][21];

void initialize() {
	char **ptr1, **ptr2;
	int i, j,k;
	char *tmp;
	int idx1, idx2;

	for (i=0; i<NUM_SET; i++) {
		for (j=0; j<assoc_index[i]; j++){
			idx1 = conflict_sets[i][j];
			ptr1 = (char **)&buf[idx1*WAY_SIZE+set_index[i]*LINE_SIZE];
			*ptr1 = (char *)ptr1;
		}

		for (j=assoc_index[i]-1; j>=1; j--){
			k = rand()%j;
			idx1 = conflict_sets[i][j];
			idx2 = conflict_sets[i][k];
			ptr1 = (char**)&buf[idx1*WAY_SIZE+set_index[i]*LINE_SIZE];
			ptr2 = (char**)&buf[idx2*WAY_SIZE+set_index[i]*LINE_SIZE];
			tmp = *ptr1;
			*ptr1 = *ptr2;
			*ptr2 = tmp;
		}

		for (j=0; j<assoc_index[i]; j++){
			idx1 = conflict_sets[i][j];
			ptr1 = (char **)&buf[idx1*WAY_SIZE+set_index[i]*LINE_SIZE];
			ptr2 = (char **)*ptr1;
			*(ptr2+1) = (char*)(ptr1+1);
		}

		idx1 = conflict_sets[i][0];
		head[i] = &buf[idx1*WAY_SIZE+set_index[i]*LINE_SIZE];

		ptr1 = (char **)head;

		for (j=0; j<assoc_index[i]-1; j++){
			ptr1 = (char**)(*ptr1);
		}
   
		tail[i] = (char*)ptr1;
	}
} 



void *clean(void *index_ptr) {
	int *index = (int *)index_ptr;
	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET(*index, &set);
	if (sched_setaffinity(syscall(SYS_gettid), sizeof(cpu_set_t), &set)) {
		fprintf(stderr, "Error set affinity\n")  ;
		return NULL;
	}
	int i;
	while (1) {
		for (i=NUM_SET/NUM_CPU*(*index); i<NUM_SET/NUM_CPU*(*index+1); i++) {
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
				:"r"(head[i])
				:"esi","r8","eax");
		}
	}
}

int main (int argc, char *argv[]) {
        int i, j, k;
        FILE *set_file = fopen("/root/conflict_sets", "r");
        if (!set_file)
                printf("error\n");
	
	int line_nr;
	for (i=0; i<NUM_SET; i++) {
		fseek(set_file, 0, SEEK_SET);
		line_nr = 1+set_index[i]*(SLICES+1) + slice_index[i];
		for (j=0; j<line_nr; j++)
		        fscanf(set_file, "%*[^\n]\n", NULL);

       	        for (j=0; j<assoc_index[i]; j++) 
               	        fscanf(set_file, "%d", &conflict_sets[i][j]);
	}

        fclose(set_file);

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

	initialize();

        int cpu_id_0 = 0;
        int cpu_id_1 = 1;
        int cpu_id_2 = 2;
        int cpu_id_3 = 3;
        pthread_t mem_thread_0;
        pthread_t mem_thread_1;
        pthread_t mem_thread_2;
        pthread_t mem_thread_3;
        pthread_create(&mem_thread_0, NULL, clean, &cpu_id_0);
        pthread_create(&mem_thread_1, NULL, clean, &cpu_id_1);
        pthread_create(&mem_thread_2, NULL, clean, &cpu_id_2);
        pthread_create(&mem_thread_3, NULL, clean, &cpu_id_3);
        pthread_join(mem_thread_0, NULL);
        pthread_join(mem_thread_1, NULL);
        pthread_join(mem_thread_2, NULL);

	munmap(buf, buf_size);
}
