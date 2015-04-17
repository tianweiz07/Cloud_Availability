#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <openssl/blowfish.h>
#include <sys/time.h>


int main(int argc, char *argv[])
{
    unsigned char rawKey[] = "password";

    BF_KEY key;
    BF_set_key(&key, strlen((char*) rawKey), rawKey);

    unsigned char msg[128];
    unsigned char enc[64];

    unsigned char ivec[8];

    int i;

    double total_time;
    struct timeval start_time;
    struct timeval end_time;
    int traversals = atoi(argv[1]);

    while (1) {
        gettimeofday(&start_time, NULL);

        int j;
        for (j=0; j<traversals; j++) {
            for (i = 0; i < 128; i++) 
                msg[i] = random() % 256;

            memset(enc, 0, 64);
            memset(ivec, 0, 8);

            BF_cbc_encrypt(msg, enc, strlen((char*) msg) + 1, &key, ivec, BF_ENCRYPT);
        }

        gettimeofday(&end_time, NULL);
        total_time = (end_time.tv_sec-start_time.tv_sec) + (end_time.tv_usec-start_time.tv_usec)/1000000.0;
        printf("%f\n", total_time);
    }

    return 0;
}
