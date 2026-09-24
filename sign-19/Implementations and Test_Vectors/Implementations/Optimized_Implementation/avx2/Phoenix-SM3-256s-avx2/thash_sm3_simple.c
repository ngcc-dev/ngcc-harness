#include <stdint.h>
#include <string.h>

#include "thash.h"
#include "address.h"
#include "params.h"
#include "utils.h"
#include "hash_sm3.h"

/**
 * Takes an array of inblocks concatenated arrays of SPX_N bytes.
 */
void thash(unsigned char *out, const unsigned char *in, unsigned int inblocks,
           const spx_ctx *ctx, uint32_t addr[8])
{
    uint8_t sm3_state[40];
    SPX_VLA(uint8_t, buf, SPX_SM3_ADDR_BYTES + inblocks*SPX_N);

    /* Retrieve precomputed state containing pub_seed */
    memcpy(sm3_state, ctx->state_seeded_sm3, 40 * sizeof(uint8_t));

    memcpy(buf, addr, SPX_SM3_ADDR_BYTES);
    memcpy(buf + SPX_SM3_ADDR_BYTES, in, inblocks * SPX_N);

    /* Use sm3_xof for arbitrary length output */
    sm3_xof(out, SPX_N, buf, SPX_SM3_ADDR_BYTES + inblocks*SPX_N);
}

/**
 * Pre-computes the bitmask using MGF1-SM3.
 */
void thash_init_bitmask(unsigned char *bitmask_out, unsigned int inblocks,
                        const spx_ctx *ctx, uint32_t addr[8])
{
    unsigned char buf[SPX_N + SPX_SM3_ADDR_BYTES];

    memcpy(buf, ctx->pub_seed, SPX_N);
    memcpy(buf + SPX_N, addr, SPX_SM3_ADDR_BYTES);

    sm3_xof(bitmask_out, inblocks * SPX_N, buf, SPX_N + SPX_SM3_ADDR_BYTES);
}

/**
 * Completes the thash by XORing with a pre-computed bitmask.
 */
void thash_fin(unsigned char *out, const unsigned char *in, unsigned int inblocks,
               const spx_ctx *ctx, uint32_t addr[8], const unsigned char *bitmask)
{
    unsigned char buf[SPX_SM3_ADDR_BYTES + inblocks * SPX_N];
    unsigned int i;

    /* 1. Copy the address into the buffer */
    memcpy(buf, addr, SPX_SM3_ADDR_BYTES);

    /* 2. XOR the input with the pre-computed bitmask */
    {
        uint32_t *dst32 = (uint32_t *)(buf + SPX_SM3_ADDR_BYTES);
        const uint32_t *in32 = (const uint32_t *)in;
        const uint32_t *mask32 = (const uint32_t *)bitmask;
        unsigned int n = (inblocks * SPX_N) / 4;
        for (i = 0; i < n; i++) {
            dst32[i] = in32[i] ^ mask32[i];
        }
        for (i = n * 4; i < inblocks * SPX_N; i++) {
            buf[SPX_SM3_ADDR_BYTES + i] = in[i] ^ bitmask[i];
        }
    }

    /* 3. Use sm3_xof for arbitrary length output */
    sm3_xof(out, SPX_N, buf, SPX_SM3_ADDR_BYTES + inblocks * SPX_N);
}
