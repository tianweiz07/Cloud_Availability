#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <inttypes.h>

uint64_t rdtsc(void) {
        uint64_t a, d;
        __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
        return (d<<32) | a;
}

int main(int argc, char **argv) {
    uint  *buf;

    int Bsize = atoi(argv[1]);
    int count = atoi(argv[2]);

    uint64_t *time = malloc(count*sizeof(uint64_t));

    int i;
    if (!(buf = (uint *) valloc((unsigned) Bsize))) {
        perror("VALLOC");
        exit(1);
    }
    bzero((char *) buf, Bsize);

    int out = open(argv[3], O_WRONLY | O_TRUNC | O_CREAT, 0644);

    for (;;) {

        if (count-- <= 0) {
            break;
        }

        int bsize = Bsize/2 + Bsize/20*(rand()%10);

        if (out >= 0) {
            int moved2;

            uint64_t start = rdtsc();
            moved2 = write(out, buf, bsize);
            fsync(out);
            uint64_t end = rdtsc();
            time[count] = end - start;

            if (moved2 != bsize) {
                fprintf(stderr, "write: wanted=%d got=%d\n", bsize, moved2);
                fsync(out);
                exit(0);
			
            }
        }
    }

    for (i=0; i<atoi(argv[2]); i++)
        printf("%"PRIu64"\n", time[i]);

    return 0;
}
