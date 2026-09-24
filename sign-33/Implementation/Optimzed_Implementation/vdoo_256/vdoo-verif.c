#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "vdoo_config.h"
#include "vdoo_keypair.h"
#include "api.h"
#include "utils.h"

int main(int argc, char **argv)
{
    printf("%s\n", CRYPTO_ALGNAME);
    printf("sk size: %lu\n", CRYPTO_SECRETKEYBYTES);
    printf("pk size: %lu\n", CRYPTO_PUBLICKEYBYTES);
    printf("hash size: %d\n", HASH_LEN);
    printf("signature size: %d\n\n", CRYPTO_BYTES);

    if (argc != 4)
    {
        printf("Usage:\n\n\tvdoo-verif pk_file_name signature_file_name message_file_name\n\n");
        return -1;
    }

    uint8_t *pk = (uint8_t *)malloc(CRYPTO_PUBLICKEYBYTES);
    if (!pk)
    {
        printf("Memory allocation failed for public key\n");
        return -1;
    }

    FILE *fp;
    int r;

    fp = fopen(argv[1], "r");
    if (!fp)
    {
        printf("Fail to open public key file.\n");
        free(pk);
        return -1;
    }

    r = byte_fget(fp, pk, CRYPTO_PUBLICKEYBYTES);
    fclose(fp);

    if (CRYPTO_PUBLICKEYBYTES != r)
    {
        printf("Fail to load key file.\n");
        free(pk);
        return -1;
    }

    // Read message
    unsigned char *msg = NULL;
    unsigned long long mlen = 0;
    r = byte_read_file(&msg, &mlen, argv[3]);
    if (0 != r)
    {
        printf("Fail to read message file.\n");
        free(pk);
        return -1;
    }

    // Read signature
    unsigned char *signature = malloc(mlen + CRYPTO_BYTES);
    if (!signature)
    {
        printf("Alloc memory for signature buffer fail.\n");
        free(msg);
        free(pk);
        return -1;
    }
    memcpy(signature, msg, mlen);

    fp = fopen(argv[2], "r");
    if (!fp)
    {
        printf("Fail to open signature file.\n");
        free(signature);
        free(msg);
        free(pk);
        return -1;
    }

    r = byte_fget(fp, signature + mlen, CRYPTO_BYTES);
    fclose(fp);
    if (CRYPTO_BYTES != r)
    {
        printf("Fail to load signature file.\n");
        free(signature);
        free(msg);
        free(pk);
        return -1;
    }
    r = crypto_sign_open(msg, &mlen, signature, mlen + CRYPTO_BYTES, pk);

    free(msg);
    free(signature);
    free(pk);

    if (0 == r)
    {
        printf("Correctly verified.\n");
        return 0;
    }
    else
    {
        printf("Verification fails.\n");
        return -1;
    }
}