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
#define CACHE_SIZE (WAY_SIZE*SLICES*ASSOC)
#define BUFFER_SIZE 524288
#define START_SET 0


int traversal;
int num_thread;

char *buf;
char **head;

void initialize() {
	char **ptr1, **ptr2;
	int i, j, k, r;
	char *tmp;
	int idx1, idx2;

	int block_nr = SLICES*ASSOC/num_thread;
	int line_nr = BUFFER_SIZE/(SLICES*ASSOC)/LINE_SIZE;

	for (i=0; i<num_thread; i++) {
		for (j=i*block_nr; j<(i+1)*block_nr; j++) {
			for (k=START_SET; k<(START_SET+line_nr); k++) {
				ptr1 = (char **)&buf[j*WAY_SIZE+k*LINE_SIZE];
				*ptr1 = (char *)ptr1;

			}
		}

		for (j=(i+1)*block_nr-1; j>=i*block_nr; j--) {
			for (k=(START_SET+line_nr-1); k>=START_SET+1; k--) {
				r = rand()%k;
				ptr1 = (char**)&buf[j*WAY_SIZE+k*LINE_SIZE];
				ptr2 = (char**)&buf[j*WAY_SIZE+r*LINE_SIZE];
				tmp = *ptr1;
				*ptr1 = *ptr2;
				*ptr2 = tmp;
			}
		}

		for (j=(i+1)*block_nr-1; j>=i*block_nr+1; j--) {
			r = rand()%j;
			ptr1 = (char**)&buf[j*WAY_SIZE+START_SET*LINE_SIZE];
			ptr2 = (char**)&buf[r*WAY_SIZE+(START_SET+line_nr-1)*LINE_SIZE];
			tmp = *ptr1;
			*ptr1 = *ptr2;
			*ptr2 = tmp;

		}

		for (j=i*block_nr; j<(i+1)*block_nr; j++) {
			for (k=START_SET; k<(START_SET+line_nr); k++) {
				ptr1 = (char **)&buf[j*WAY_SIZE+k*LINE_SIZE];
				ptr2 = (char **)*ptr1;
				*(ptr2+1) = (char*)(ptr1+1);
			}
		}

		head[i] = &buf[i*block_nr*WAY_SIZE+START_SET*LINE_SIZE];

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
	time_t start_time, end_time;
	start_time = time(NULL);
	for (i=0; i<traversal; i++) {
		__asm__("mov %0,%%r8\n\t"
			"mov %%r8,%%rsi\n\t"
			"xor %%eax, %%eax\n\t"
			"loop: mov (%%r8), %%r8\n\t"
			"cmp %%r8,%%rsi\n\t"
			"jne loop\n\t"
			"xor %%eax, %%eax\n\t"
			:
			:"r"(head[*index])
			:"esi","r8","eax");
	}
	end_time = time(NULL);
	printf("Time diff: %lld\n", (long long int)end_time-(long long int)start_time);
}

int main (int argc, char *argv[]) {
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

	num_thread = atoi(argv[1]);
	traversal = atoi(argv[2]);

	head = (char **)calloc(num_thread, sizeof(char *));
	initialize();


	if ((num_thread > 12)||(num_thread < 0)) {
                printf("# of Threads are not corrected\n");
                return 0;
        }

	int cpu_id[12];
	pthread_t llc_thread[12];

	int i;
        for (i=0; i<num_thread; i++) {
                cpu_id[i] = i;
                pthread_create(&llc_thread[i], NULL, clean, &cpu_id[i]);
        }

        for (i=0; i<num_thread; i++) {
                pthread_join(llc_thread[i], NULL);
        }
	
	munmap(buf, buf_size);
}
