#include <stdio.h>
#include <string.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <sys/time.h>

#define KEY_LENGTH  2048
#define PUB_EXP     3

int main(int argc, char *argv[]) {
    size_t pri_len;            // Length of private key
    size_t pub_len;            // Length of public key
    char   *pri_key;           // Private key
    char   *pub_key;           // Public key
    char   msg[16];  // Message to encrypt

    char   *encrypt = NULL;    // Encrypted message
    int encrypt_len;

    RSA *keypair = RSA_generate_key(KEY_LENGTH, PUB_EXP, NULL, NULL);

    BIO *pri = BIO_new(BIO_s_mem());
    BIO *pub = BIO_new(BIO_s_mem());

    PEM_write_bio_RSAPrivateKey(pri, keypair, NULL, NULL, 0, NULL, NULL);
    PEM_write_bio_RSAPublicKey(pub, keypair);

    pri_len = BIO_pending(pri);
    pub_len = BIO_pending(pub);

    pri_key = malloc(pri_len + 1);
    pub_key = malloc(pub_len + 1);

    BIO_read(pri, pri_key, pri_len);
    BIO_read(pub, pub_key, pub_len);

    pri_key[pri_len] = '\0';
    pub_key[pub_len] = '\0';


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

            encrypt = malloc(RSA_size(keypair));
            encrypt_len = RSA_public_encrypt(strlen(msg)+1, (unsigned char*)msg, (unsigned char*)encrypt, keypair, RSA_PKCS1_OAEP_PADDING);
            if (encrypt_len == -1)
                return 0;
            free(encrypt);
        }
        gettimeofday(&end_time, NULL);
        total_time = (end_time.tv_sec-start_time.tv_sec) + (end_time.tv_usec-start_time.tv_usec)/1000000.0;
        printf("%f\n", total_time);
    }

    RSA_free(keypair);
    BIO_free_all(pub);
    BIO_free_all(pri);
    free(pri_key);
    free(pub_key);


    return 0;
}
