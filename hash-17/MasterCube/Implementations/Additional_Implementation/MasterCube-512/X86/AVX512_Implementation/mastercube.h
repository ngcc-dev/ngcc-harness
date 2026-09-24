#ifndef MASTERCUBE_H
#define MASTERCUBE_H

/* Merged from common.h. */
#include <stdint.h>

/*
 * pad10*1 on a zero-initialized last_block.
 *
 * last_block: zeroed buffer that may be larger than the real block
 * msg_bits:   number of valid message bits already stored in last_block
 * block_bits: real logical block size in bits, always a multiple of 8
 *
 * Bit order inside each byte: MSB-first.
 */
static inline void pad10star1(uint8_t last_block[136], const int message_bits, const int block_bits)
{
    const int partial_bits = message_bits % 8;
    if (partial_bits != 0)
    {
        last_block[message_bits / 8] &= (uint8_t)(0xFFu << (8 - partial_bits));
    }

    /* append the first '1' bit immediately after the message */
    last_block[message_bits / 8] |= (uint8_t)(0x80u >> partial_bits);

    /* set the last bit of the real block to 1 */
    last_block[(block_bits / 8) - 1] |= 0x01u;

    return;
}

/* Merged from macros.h. */
#define UNUSED(x) (void)(x)

#define _MM_SHUFFLE_R(w, x, y, z) _MM_SHUFFLE(z, y, x, w)

/* Public MasterCube API declarations. */
#ifdef __cplusplus
extern "C"
{
#endif

#ifndef MasterCube512
#define MasterCube512 MasterCube512_avx512_256
#endif
#ifndef MasterCube768
#define MasterCube768 MasterCube768_avx512_256
#endif
#ifndef MasterCube1024
#define MasterCube1024 MasterCube1024_avx512_256
#endif

int MasterCube512(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);
int MasterCube768(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);
int MasterCube1024(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);

int MasterCube512_avx512_256(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);
int MasterCube768_avx512_256(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);
int MasterCube1024_avx512_256(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);

#ifdef __cplusplus
}
#endif

#endif /* MASTERCUBE_H */

