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

time_t total_time;
int num_thread;

char *buf;
char **head;

void initialize() {
	char **ptr1, **ptr2;
	int i, j, k;
	char *tmp;
	int idx1, idx2;


	int line_nr = (CACHE_SIZE/LINE_SIZE/num_thread);
	for (i=0; i<num_thread; i++) {
		for (j=i*line_nr; j<(i+1)*line_nr; j++) {
			ptr1 = (char **)&buf[j*LINE_SIZE];
			*ptr1 = (char *)ptr1;
		}

		for (j=(i+1)*line_nr-1; j>=i*line_nr+1; j--){
			ptr1 = (char**)&buf[j*LINE_SIZE];
			ptr2 = (char**)&buf[(j-1)*LINE_SIZE];
			tmp = *ptr1;
			*ptr1 = *ptr2;
			*ptr2 = tmp;
		}

		for (j=i*line_nr; j<(i+1)*line_nr; j++) {
			ptr1 = (char **)&buf[j*LINE_SIZE];
			ptr2 = (char **)*ptr1;
			*(ptr2+1) = (char*)(ptr1+1);
		}
		head[i] = &buf[i*line_nr*LINE_SIZE];

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
	printf("Begin Cleansing\n");

	int *read_address;
	int value = 0x0;
	read_address = (int *)(head[*index]+LINE_SIZE-1);
	while(time(NULL) < end_time) {
			__asm__(
        	              "lock; xaddl %%eax, %1\n\t"
                	      :"=a"(value)
	                      :"m"(*read_address), "a"(value)
        	              :"memory");
	}
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
	total_time = atoi(argv[2]);

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
                cpu_id[i] = i*2;
                pthread_create(&llc_thread[i], NULL, clean, &cpu_id[i]);
        }

        for (i=0; i<num_thread; i++) {
                pthread_join(llc_thread[i], NULL);
        }
	
	munmap(buf, buf_size);
}
