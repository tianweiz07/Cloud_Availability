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
#include <fcntl.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>



#define ASSOC 20
#define SLICES 6
#define WAY_SIZE 131072
#define LINE_SIZE 64

#define TRIALS 10000
#define THRESHOLD 3000

char *buf;
char *head[4];
char value;
unsigned int time_buf;

int set;
int slice;
int assoc;

unsigned int conflict_sets[SLICES][21];

unsigned int num_line[SLICES];

void initialize(int core, int start, int end) {
	char **ptr1, **ptr2;
	int j,k;
	char *tmp;
	int idx1, idx2;

	for (j=start; j<end; j++){
		idx1 = conflict_sets[slice][j];
		ptr1 = (char **)&buf[idx1*WAY_SIZE+set*LINE_SIZE];
		*ptr1 = (char *)ptr1;
	}

        for (j=end-1; j>=start+1; j--){
                k = rand()%j;
                idx1 = conflict_sets[slice][j];
                idx2 = conflict_sets[slice][k];
                ptr1 = (char**)&buf[idx1*WAY_SIZE+set*LINE_SIZE];
                ptr2 = (char**)&buf[idx2*WAY_SIZE+set*LINE_SIZE];
                tmp = *ptr1;
                *ptr1 = *ptr2;
                *ptr2 = tmp;
        }

	for (j=start; j<end; j++){
		idx1 = conflict_sets[slice][j];
		ptr1 = (char **)&buf[idx1*WAY_SIZE+set*LINE_SIZE];
		ptr2 = (char **)*ptr1;
		*(ptr2+1) = (char*)(ptr1+1);
	}

	idx1 = conflict_sets[slice][start];
	head[core] = &buf[idx1*WAY_SIZE+set*LINE_SIZE];

	ptr1 = (char **)(head[core]);

	for (j=start; j<end-1; j++){
		ptr1 = (char**)(*ptr1);
	}
   
} 

inline unsigned long gettime() {
  volatile unsigned long tl;
  asm __volatile__("lfence\nrdtsc" : "=a" (tl): : "%edx");
  return tl;
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

	int start = (*index)*5;
	int end = ((*index) + 1)*5>assoc?assoc:((*index) + 1)*5;

	initialize(*index, start, end);
	printf("Cleansing starts at Core #%d, start=%d end=%d\n", *index, start, end);

	sleep(3);
	while (1) {
//		unsigned int time1 = gettime();
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
//		printf("%lu\n", gettime()-time1);
			
	}

}

int main (int argc, char *argv[]) {
        set = atoi(argv[1]);
	slice = atoi(argv[2]);

        int i, j;
        FILE *set_file = fopen("conflict_sets", "r");
	FILE *line_file = fopen("original_assoc_num", "r");
        if ((!set_file)||(!line_file))
                printf("error\n");

        char *str = malloc(sizeof(char) * 1024);
        unsigned int _set = 0;
        do {
            fscanf(set_file, "%s %d\n", str, &_set);
            if (_set == set)
                break;
            for (j=0; j<SLICES; j++) {
                fscanf(set_file, "%*[^\n]\n", NULL);
            }
        } while (!feof(set_file));

        if (!feof(set_file)) {
            for (i=0; i<SLICES; i++) {
                for (j=0; j<21; j++) {
                    fscanf(set_file, "%d", &conflict_sets[i][j]);
                }
            }
        }

        fclose(set_file);

        do {
            fscanf(line_file, "%s %d\n", str, &_set);
            if (_set == set)
                break;
            fscanf(line_file, "%*[^\n]\n", NULL);
        } while (!feof(line_file));

        if (!feof(line_file)) {
            for (i=0; i<SLICES; i++) {
                fscanf(line_file, "%d", &num_line[i]);
            }
        }

	fclose(line_file);
	assoc = num_line[slice];
	printf("assoc=%d\n", assoc);


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


	int cpu_id[4];
	pthread_t llc_thread[4];
	for (i=0; i<4; i++) {
                cpu_id[i] = i;
                pthread_create(&llc_thread[i], NULL, clean, &cpu_id[i]);
        }


	for (i=0; i<4; i++) {
                pthread_join(llc_thread[i], NULL);
        }

	munmap(buf, buf_size);
}
