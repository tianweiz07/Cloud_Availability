#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/sha.h>
#include <sys/time.h>

int main(int argc, char *argv[])
{
    char text[16];
    unsigned char obuf[20];
    int i;
    double total_time;
    struct timeval start_time;
    struct timeval end_time;
    int traversals = atoi(argv[1]);

    while (1) {
        gettimeofday(&start_time, NULL);

        int j;
        for (j=0; j<traversals; j++) {
            for (i = 0; i<16; i++)
                text[i] = random()%256;
            SHA1((unsigned char *)text, strlen(text), obuf);
        }

        gettimeofday(&end_time, NULL);
        total_time = (end_time.tv_sec-start_time.tv_sec) + (end_time.tv_usec-start_time.tv_usec)/1000000.0;
        printf("%f\n", total_time);
    }
    return 0;
}
