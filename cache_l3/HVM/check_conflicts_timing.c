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

#define ASSOC 20
#define SLICES 6
#define WAY_SIZE 131072
#define LINE_SIZE 64

#define TRIALS 10000

char *buf;
char *head;
char *tail;
char value;
unsigned int time_buf;

int set;
int assoc;
int slice;

unsigned int conflict_sets[SLICES][21];

void initialize() {
	char **ptr1, **ptr2;
	int j,k;
	char *tmp;
	int idx1, idx2;

	for (j=0; j<assoc; j++){
		idx1 = conflict_sets[slice][j];
		ptr1 = (char **)&buf[idx1*WAY_SIZE+set*LINE_SIZE];
		*ptr1 = (char *)ptr1;
	}

	for (j=assoc-1; j>=1; j--){
		k = rand()%j;
		idx1 = conflict_sets[slice][j];
		idx2 = conflict_sets[slice][k];
		ptr1 = (char**)&buf[idx1*WAY_SIZE+set*LINE_SIZE];
		ptr2 = (char**)&buf[idx2*WAY_SIZE+set*LINE_SIZE];
		tmp = *ptr1;
		*ptr1 = *ptr2;
		*ptr2 = tmp;
	}

	for (j=0; j<assoc; j++){
		idx1 = conflict_sets[slice][j];
		ptr1 = (char **)&buf[idx1*WAY_SIZE+set*LINE_SIZE];
		ptr2 = (char **)*ptr1;
		*(ptr2+1) = (char*)(ptr1+1);
	}

	idx1 = conflict_sets[slice][0];
	head = &buf[idx1*WAY_SIZE+set*LINE_SIZE];

	ptr1 = (char **)head;

	for (j=0; j<assoc-1; j++){
		ptr1 = (char**)(*ptr1);
	}
   
	tail = (char*)ptr1;
} 



void prime()
{
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
           :"r"(head)
           :"esi","r8","eax");
}

void probe()
{
   __asm__("mov %0,%%r8\n\t"
           "mov %%r8,%%rsi\n\t"
           "mov %1,%%r10\n\t"
           "xor %%eax, %%eax\n\t"
           "cpuid\n\t"
         //  "lfence\n\t"
           "rdtsc\n\t"
           "mov %%eax,%%edi\n\t"
           "loop1: mov (%%r8), %%r8\n\t"
           "cmp %%r8,%%rsi\n\t"
           "jne loop1\n\t"
           "xor %%eax, %%eax\n\t"
           "cpuid\n\t"
         //  "lfence\n\t"
           "rdtsc\n\t"
           "sub %%edi, %%eax\n\t"
           "mov %%eax, (%%r10)\n\t"
           :
           :"r"(head),"r"(&time_buf)
           :"eax","edx","esi","edi","r8","r10"
           );
}

int main (int argc, char *argv[]) {
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(0, &mask);
        sched_setaffinity(0, sizeof(cpu_set_t), &mask);

        set = atoi(argv[1]);
	slice = atoi(argv[2]);
	assoc = atoi(argv[3]);

        int i, j;
        FILE *set_file = fopen("/root/conflict_sets", "r");
        if (!set_file)
                printf("error\n");

        fscanf(set_file, "%*[^\n]\n", NULL);
        for (i=0; i<set; i++) {
                for (j=0; j<SLICES+1; j++) {
                        fscanf(set_file, "%*[^\n]\n", NULL);
                }
        }

        for (i=0; i<SLICES; i++) {
                for (j=0; j<21; j++) {
                        fscanf(set_file, "%d", &conflict_sets[i][j]);
                }
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

	unsigned int total_time;

	total_time = 0;
	i = 0;
	initialize();
	while (i < TRIALS){
		prime();
		probe();
		total_time += time_buf;
		i++;
	} 
	printf("%u ", total_time/TRIALS);
	munmap(buf, buf_size);
}
