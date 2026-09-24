#ifndef WISH_H_
#define WISH_H_

#include <arm_neon.h>

typedef uint8x16_t u128;

enum {

    WISH_1024_BLOCK_SIZE_BITS = 1024,
    WISH_1024_BLOCK_SIZE_BYTES = WISH_1024_BLOCK_SIZE_BITS / 8,
    WISH_1024_NLANES = 1024 / 128 * 2,
    WISH_1024_NSTEPS = 12,
};


int Wish1024Absorb(u128 *state, u128 counter, int last_block, const unsigned char *block);
int Wish1024(const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);


#endif  // WISH_H_
