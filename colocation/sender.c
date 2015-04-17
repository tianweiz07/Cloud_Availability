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

#define line_size 64
#define size 1048576

char *data_array;

int main (int argc, char *argv[]) {

        int fd = open("nebula1", O_RDWR, 0755);
        if (fd < 0) {
                printf("file open error!\n");
                return 0;
        }
        data_array = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);

        if (data_array == MAP_FAILED) {
                printf("map 1 error!\n");
                unlink("nebula1");
                return 0;
        }


        cpu_set_t set;
        CPU_ZERO(&set);
        CPU_SET(0, &set);
        if (sched_setaffinity(syscall(SYS_gettid), sizeof(cpu_set_t), &set)) {
                fprintf(stderr, "Error set affinity\n")  ;
                return 0;
        }

	int *read_address;
	read_address = (int *)(data_array + line_size - 1);
	int value = 0x0;
	int index;

	while (1) {
        	__asm__(
			"lock; xaddl %%eax, %1\n\t"
                        :"=a"(value)
                        :"m"(*read_address), "a"(value)
                        :"memory");
	}

	return 0;
}
