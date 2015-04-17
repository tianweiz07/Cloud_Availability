#include <stdio.h>
#include <string.h>
#include <openssl/hmac.h>
#include <sys/time.h>
 
int main(int argc, char *argv[])
{
    char key[] = "012345678";
    char text[16];
    unsigned char* digest;
 
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

            digest = HMAC(EVP_sha1(), key, strlen(key), (unsigned char*)text, strlen(text), NULL, NULL);    
            if (digest == NULL)
	        return 0;
        }

        gettimeofday(&end_time, NULL);
        total_time = (end_time.tv_sec-start_time.tv_sec) + (end_time.tv_usec-start_time.tv_usec)/1000000.0;
        printf("%f\n", total_time);
    }
   
    return 0;
}
