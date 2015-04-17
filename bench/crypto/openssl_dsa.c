#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <openssl/dsa.h>
#include <openssl/engine.h> 
#include <openssl/err.h> 
#include <openssl/md5.h>
#include <sys/time.h>


int main(int argc, char *argv[]) { 

    DSA *dsa = DSA_generate_parameters(1024,NULL,0,NULL,NULL,NULL,NULL); 
    if(dsa==NULL) { 
        DSA_free(dsa); 
        return 0;
    } 

    if(DSA_generate_key(dsa) == 0) { 
        DSA_free(dsa); 
        return 0;
    } 

    unsigned char msg[16];
    unsigned int Siglen; 
    unsigned char *Sig = malloc(DSA_size(dsa)); 

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
                msg[i] = random()%256;

            if((DSA_sign(0, msg, strlen((char *)msg), Sig, &Siglen, dsa)) != 1) { 
                DSA_free(dsa); 
                return 0;
            }    
        }

        gettimeofday(&end_time, NULL);
        total_time = (end_time.tv_sec-start_time.tv_sec) + (end_time.tv_usec-start_time.tv_usec)/1000000.0;
        printf("%f\n", total_time);
    }

    return 0;
}
