#include <stdio.h>
#include <string.h>
#include <openssl/rc4.h>
#include <stdlib.h>
#include <sys/time.h>

int main(int argc, char *argv[])
{

    RC4_KEY key;
    static unsigned char key_data[256] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};

    unsigned char text[16];
    unsigned char*  ciphertext = (unsigned char*) malloc(sizeof(char) * strlen((char *)text)); 

    int i;
    RC4_set_key(&key, 8, key_data);

    double total_time;
    struct timeval start_time;
    struct timeval end_time;
    int traversals = atoi(argv[1]);

    while(1) {
        gettimeofday(&start_time, NULL);
        int j;
        for (j=0; j<traversals; j++) {
            for (i = 0; i<16; i++)
                text[i] = random()%256;
            RC4(&key, strlen((char *)text), text, ciphertext);
        }
        gettimeofday(&end_time, NULL);
        total_time = (end_time.tv_sec-start_time.tv_sec) + (end_time.tv_usec-start_time.tv_usec)/1000000.0;
        printf("%f\n", total_time);
    }

    free(ciphertext);

    return 0;
}
