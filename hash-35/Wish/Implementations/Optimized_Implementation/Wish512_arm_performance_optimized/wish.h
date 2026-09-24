#ifndef WISH_H_
#define WISH_H_

#include <arm_neon.h>

typedef uint8x16_t u128;

enum {
    WISH_512_BLOCK_SIZE_BITS = 512,
    WISH_512_BLOCK_SIZE_BYTES = WISH_512_BLOCK_SIZE_BITS / 8,
    WISH_512_NLANES = 512 / 128 * 2,
    WISH_512_NSTEPS = 9,

};

int Wish512Absorb(u128 *state, u128 counter, int last_block, const unsigned char *block);
int Wish512(const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);



#endif  // WISH_H_
