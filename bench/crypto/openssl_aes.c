#include <stdlib.h>
#include <stdio.h>
#include <openssl/aes.h>

#include <sys/time.h>

static const unsigned char key[] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
};

int main(int argc, char *argv[])
{
    unsigned char text[16];
    unsigned char * enc_out = malloc(16*sizeof(char)); 

    int i;
    AES_KEY enc_key;
    AES_set_encrypt_key(key, 128, &enc_key);



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
            AES_encrypt(text, enc_out, &enc_key);  
        }
        gettimeofday(&end_time, NULL);
        total_time = (end_time.tv_sec-start_time.tv_sec) + (end_time.tv_usec-start_time.tv_usec)/1000000.0;
        printf("%f\n", total_time);
    }

    free(enc_out);

    return 0;
}
