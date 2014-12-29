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
#define SET_NUM (WAY_SIZE/LINE_SIZE)

time_t total_time;
int num_thread;

char *buf;
char **head;


int set_index[SET_NUM][SLICES];  // Indicate if these sets are polluted (1) or not (2)
int assoc_index[SET_NUM][SLICES]; // Indicate how many cache lines are needed to fill up one set
int conflict_sets[SET_NUM][SLICES][ASSOC+1];  // Records the conflicted set indexes.
int total_conflict_num;  // Records the total number of polluted cache sets.

void initialize() {
	char **ptr1, **ptr2;
	int i, j, k;
	char *tmp;
	int idx1, idx2;

	int set_checked = 0;

	for (i=0; i<SET_NUM; i++) {
		for (j=0; j<SLICES; j++) {
			if (set_index[i][j] > 0) {
				for (k=0; k<assoc_index[i][j]; k++) {
					idx1 = conflict_sets[i][j][k];
					ptr1 = (char **)&buf[idx1*WAY_SIZE+i*LINE_SIZE];
					*ptr1 = (char *)ptr1;
				}

				for (k=assoc_index[i][j]-1; k>=1; k--){
					idx1 = conflict_sets[i][j][k];
					idx2 = conflict_sets[i][j][k-1];
					ptr1 = (char**)&buf[idx1*WAY_SIZE+i*LINE_SIZE];
					ptr2 = (char**)&buf[idx2*WAY_SIZE+i*LINE_SIZE];
					tmp = *ptr1;
					*ptr1 = *ptr2;
					*ptr2 = tmp;
				}

				for (k=0; k<assoc_index[i][j]; k++){
					idx1 = conflict_sets[i][j][k];
					ptr1 = (char **)&buf[idx1*WAY_SIZE+i*LINE_SIZE];
					ptr2 = (char **)*ptr1;
					*(ptr2+1) = (char*)(ptr1+1);
				}

				idx1 = conflict_sets[i][j][0];
				head[set_checked] = &buf[idx1*WAY_SIZE+i*LINE_SIZE];

				set_checked ++;
			}

		}

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
	time_t end_time = time(NULL) + total_time;
	printf("Cleansing starts at Core #%d\n", *index);
	while(time(NULL) < end_time) {
		for (i=total_conflict_num/num_thread*(*index); i<total_conflict_num/num_thread*(*index+1); i++) {
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
        FILE *set_file = fopen("conflict_sets", "r");
        FILE *diff_file = fopen("diff_assoc_num", "r");
        FILE *orig_file = fopen("original_assoc_num", "r");
	
        if ((!set_file)||(!diff_file)||(!orig_file)) {
                printf("error\n");
		return 0;
	}

	total_conflict_num = 0;
	for (i=0; i<SET_NUM; i++) {
		fseek(diff_file, 0, SEEK_SET);
		fseek(orig_file, 0, SEEK_SET);
		for (j=0; j<i; j++) {
			fscanf(diff_file, "%*[^\n]\n", NULL);
			fscanf(orig_file, "%*[^\n]\n", NULL);
		}
       	        for (j=0; j<SLICES; j++) {
			fscanf(diff_file, "%d", &set_index[i][j]);
			fscanf(orig_file, "%d", &assoc_index[i][j]);
			if (set_index[i][j] > 0)
				total_conflict_num ++;

			fseek(set_file, 0, SEEK_SET);
			for (k=0; k<1+i*(SLICES+1)+j; k++)
				fscanf(set_file, "%*[^\n]\n", NULL);
			for (k=0; k<assoc_index[i][j]; k++)
				fscanf(set_file, "%d", &conflict_sets[i][j][k]);
		}
	}
	
        fclose(set_file);
	fclose(diff_file);
	fclose(orig_file);

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

	head = (char **)calloc(total_conflict_num, sizeof(char *));

	initialize();

	num_thread = atoi(argv[1]);
	total_time = atoi(argv[2]);

	if ((num_thread > 12)||(num_thread < 0)) {
                printf("# of Threads are not corrected\n");
                return 0;
        }

	int cpu_id[12];
	pthread_t llc_thread[12];

        for (i=0; i<num_thread; i++) {
                cpu_id[i] = i*2;
                pthread_create(&llc_thread[i], NULL, clean, &cpu_id[i]);
        }

        for (i=0; i<num_thread; i++) {
                pthread_join(llc_thread[i], NULL);
        }
	
	munmap(buf, buf_size);
}
