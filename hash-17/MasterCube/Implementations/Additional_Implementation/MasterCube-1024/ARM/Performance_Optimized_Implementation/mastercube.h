#ifndef MASTERCUBE_H
#define MASTERCUBE_H

/* Merged from common.h. */
#ifndef MASTERCUBE_ROUND4_ARM_COMMON_H
#define MASTERCUBE_ROUND4_ARM_COMMON_H

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

#endif

/* Merged from macros.h. */
#define UNUSED(x) (void)(x)

#if defined(MCUBE_USE_SSE2NEON) || defined(__SSE4_2__) || defined(__AVX2__)

#define _MM_SHUFFLE_R(w, x, y, z) _MM_SHUFFLE(z, y, x, w)

#endif

/* Public MasterCube API declarations. */
#ifdef __cplusplus
extern "C"
{
#endif

#if (defined(__aarch64__) || defined(__arm64__)) && !defined(MCUBE_USE_SSE2NEON)
#define MCUBE_USE_SSE2NEON 1
#endif

    /*
     * Generic API selection:
     * - MCUBE_USE_SSE2NEON selects the SSE implementation translated through sse2neon on ARM.
     * - x86 paths keep the original ISA-based dispatch order.
     */
    #if defined(MCUBE_USE_SSE2NEON) && (defined(__aarch64__) || defined(__arm64__))
        #define  MasterCube512  MasterCube512_arm64_neon
        #define  MasterCube768  MasterCube768_arm64_neon
        #define MasterCube1024 MasterCube1024_arm64_neon
    #elif !defined(MCUBE_USE_SSE2NEON) && (defined(__aarch64__) || defined(__arm64__))
        #define  MasterCube512  MasterCube512_arm64_plain
        #define  MasterCube768  MasterCube768_arm64_plain
        #define MasterCube1024 MasterCube1024_arm64_plain
    #elif defined(MCUBE_USE_SSE2NEON)
        #define  MasterCube512  MasterCube512_sse
        #define  MasterCube768  MasterCube768_sse
        #define MasterCube1024 MasterCube1024_sse
    #elif defined (__AVX512F__) && defined (__AVX512BW__) && defined (__AVX512VL__)
        #define  MasterCube512  MasterCube512_avx512_256
        #define  MasterCube768  MasterCube768_avx512_256
        #define MasterCube1024 MasterCube1024_avx512_256
    #elif defined (__AVX2__)
        #define  MasterCube512  MasterCube512_avx2
        #define  MasterCube768  MasterCube768_avx2
        #define MasterCube1024 MasterCube1024_avx2
    #elif defined (__SSE4_2__)
        #define  MasterCube512  MasterCube512_sse
        #define  MasterCube768  MasterCube768_sse
        #define MasterCube1024 MasterCube1024_sse
    #else
        #define  MasterCube512  MasterCube512_plain
        #define  MasterCube768  MasterCube768_plain
        #define MasterCube1024 MasterCube1024_plain
    #endif

    int  MasterCube512(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);
    int  MasterCube768(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);
    int MasterCube1024(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);

    int  MasterCube512_plain(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);
    int  MasterCube768_plain(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);
    int MasterCube1024_plain(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);

#if !defined(MCUBE_USE_SSE2NEON) && (defined(__aarch64__) || defined(__arm64__))
    int  MasterCube512_arm64_plain(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);
    int  MasterCube768_arm64_plain(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);
    int MasterCube1024_arm64_plain(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);
#endif

    int  MasterCube512_sse(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);
    int  MasterCube768_sse(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);
    int MasterCube1024_sse(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);

#if defined(MCUBE_USE_SSE2NEON) && defined(MCUBE_ENABLE_ARM64_NEON_B003)
    int  MasterCube512_neon_b003(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);
#endif

#if defined(MCUBE_USE_SSE2NEON) && (defined(__aarch64__) || defined(__arm64__))
    int  MasterCube512_arm64_neon(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);
    int  MasterCube768_arm64_neon(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);
    int MasterCube1024_arm64_neon(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);
#endif

    int  MasterCube512_avx2(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);
    int  MasterCube768_avx2(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);
    int MasterCube1024_avx2(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);

    int  MasterCube512_avx512_256(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);
    int  MasterCube768_avx512_256(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);
    int MasterCube1024_avx512_256(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest);

#ifdef __cplusplus
}
#endif

#endif /* MASTERCUBE_H */

