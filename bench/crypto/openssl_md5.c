#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <openssl/md5.h>
#include <sys/time.h>
 
int main(int argc, char *argv[])
{
    unsigned char digest[MD5_DIGEST_LENGTH];
    char text[16];

    double total_time;
    struct timeval start_time;
    struct timeval end_time;
    int traversals = atoi(argv[1]);

    int i;
    while (1) {
        gettimeofday(&start_time, NULL);

        int j;
        for (j=0; j<traversals; j++) {
            for (i = 0; i<16; i++)
                text[i] = random()%256;
  
            MD5((unsigned char*)&text, strlen(text), (unsigned char*)&digest);    
        }

        gettimeofday(&end_time, NULL);
        total_time = (end_time.tv_sec-start_time.tv_sec) + (end_time.tv_usec-start_time.tv_usec)/1000000.0;
        printf("%f\n", total_time);
    }
    return 0;
}
