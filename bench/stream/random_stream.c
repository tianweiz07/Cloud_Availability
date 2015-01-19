#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sched.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sched.h>
#include <sys/types.h>

#define line_size 64
#define size 32768

time_t total_time;
int *array;
int *target_array;
int *index_array;

int num_thread;
int traversals;

uint64_t rdtsc(void) {
        uint64_t a, d;
        __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
        return (d<<32) | a;
}

int initial() {
	int i, j;
	uint64_t mem_size = size;
        int fd = open("/root/nebula1", O_CREAT|O_RDWR, 0755);
        if (fd < 0) {
                printf("file open error!\n");
                return 0;
        }
        array = mmap(NULL, mem_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	
        if (array == MAP_FAILED) {
                printf("map 1 error!\n");
                unlink("/root/nebula1");
                return 0;
        }

        int fd1 = open("/root/nebula2", O_CREAT|O_RDWR, 0755);
        if (fd1 < 0) {
                printf("file open error!\n");
                return 0;
        }
        target_array = mmap(NULL, mem_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd1, 0);

        if (target_array == MAP_FAILED) {
                printf("map 2 error!\n");
                unlink("/root/nebula2");
                return 0;
        }

	index_array = (int *)malloc(size/line_size*sizeof(int));

	for (i=0; i<size/line_size; i++)
		index_array[i] = (rand()%(size/line_size))*line_size/sizeof(int);

	for (i=0; i <size/sizeof(int); i++) {
		array[i] = rand();
		target_array[i] = rand();
	}
	return 0;
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
  time_t start_time;
  time_t end_time;
  uint64_t start, end;

  start_time = time(NULL);
  for (i=0; i < traversals; i++) {
    for (j=size/line_size/num_thread*((*index)); j < size/line_size/num_thread*((*index)+1); j++) {
      target_array[index_array[j]] = array[index_array[j]];
    }
  }
  end_time = time(NULL);
  total_time += end_time-start_time;
  printf("Time diff: %lld\n", (long long int)end_time-(long long int)start_time);
}

int main(int argc, char *argv[]) {
	total_time = 0;
	int cpu_id[12];
	pthread_t mem_thread[12];

	initial();

	num_thread = atoi(argv[1]);
	traversals = atoi(argv[2]);

	int thread_index = num_thread;

	if ((thread_index > 12)||(thread_index < 0)) {
		printf("# of Threads are not corrected\n");
		return 0;
	}

	int i;
	for (i=0; i<num_thread; i++) {
		cpu_id[i] = i;
	        pthread_create(&mem_thread[i], NULL, stream_access, &cpu_id[i]);
	}

	for (i=0; i<num_thread; i++) {
	        pthread_join(mem_thread[i], NULL);
	}

	printf("Total Time: %lld\n", (long long int)total_time);
	return 0;
}
