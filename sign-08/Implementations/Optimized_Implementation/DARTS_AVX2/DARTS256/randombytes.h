#ifndef RANDOMBYTES_H
#define RANDOMBYTES_H

#include <stddef.h>
#include <stdint.h>

#define RNG_SUCCESS 0
#define RNG_BAD_MAXLEN -1
#define RNG_BAD_OUTBUF -2
#define RNG_BAD_REQ_LEN -3
#define RNG_FAIL_SYSCALL -4

typedef struct {
    unsigned char buffer[16];
    int buffer_pos;
    unsigned long length_remaining;
    unsigned char key[32];
    unsigned char ctr[16];
} AES_XOF_struct;

typedef struct {
    unsigned char Key[32];
    unsigned char V[16];
    int reseed_counter;
    int init;
} AES256_CTR_DRBG_struct;

void AES256_CTR_DRBG_Update(unsigned char *provided_data, unsigned char *Key,
                            unsigned char *V);

void randombytes_init(unsigned char *entropy_input,
                      unsigned char *personalization_string,
                      int security_strength);

int randombytes(uint8_t *out, size_t outlen);

#endif
