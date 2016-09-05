#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>

#define RANDOM 50
#define CACHE_LINE_SIZE 64

uint64_t rdtsc(void) {
        uint64_t a, d;
        __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
        return (d<<32) | a;
}

int main(int argc, char **argv) {

    int SIZE = atoi(argv[1]);
    int count = atoi(argv[2]);

    uint64_t *time_val = malloc(count*sizeof(uint64_t));
    int *array = mmap(0, SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);

    int i, j;

    srand(time(NULL));
    for (i=0; i<SIZE/sizeof(int); i++)
        array[i] = rand();


    for (i=0; i<count; i++) {
        int size = SIZE/10*(rand()%10);
        uint64_t start = rdtsc();
        for (j=0; j<size/sizeof(int); j+= CACHE_LINE_SIZE/sizeof(int))
            array[j] ++;
        uint64_t end = rdtsc();
        time_val[i] = end - start;
    }

    for (i=0; i<atoi(argv[2]); i++)
        printf("%"PRIu64"\n", time_val[i]);

    return 0;
}
