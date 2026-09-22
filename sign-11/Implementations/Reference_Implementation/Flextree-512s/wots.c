#include <stdint.h>
#include <string.h>

#include "utils.h"
#include "utilsx1.h"
#include "hash.h"
#include "thash.h"
#include "wots.h"
#include "wotsx1.h"
#include "address.h"
#include "params.h"

const unsigned int wots_w[]    = SPX_WOTS_W_ARRAY;
const unsigned int wots_wlog[] = SPX_WOTS_LOGW_ARRAY;

/**
 * Computes the chaining function.
 * out and in have to be n-byte arrays.
 *
 * Interprets in as start-th value of the chain.
 * addr has to contain the address of the chain.
 */
static void gen_chain(unsigned char *out, const unsigned char *in,
                      unsigned int start, unsigned int steps,
                      const spx_ctx *ctx, uint32_t addr[8],uint32_t chain_idx);

static void gen_chain(unsigned char *out, const unsigned char *in,
                      unsigned int start, unsigned int steps,
                      const spx_ctx *ctx, uint32_t addr[8],uint32_t chain_idx)
{
    uint32_t i;

    /* Initialize out with the value at position 'start'. */
    memcpy(out, in, SPX_N);

    /* Iterate 'steps' calls to the hash function. */
    for (i = start; i < (start+steps) && i < wots_w[chain_idx]; i++) {
        set_hash_addr(addr, i);
        thash(out, out, 1, ctx, addr);
    }
}

/**
 * Takes a message and derives the matching chain lengths.
 */
unsigned int chain_lengths(unsigned int *lengths, const unsigned char *msg)
{
    int bitbuf = 0;
    unsigned int bits_in_buf = 0;
    unsigned int csum = 0;
    const unsigned char *in = msg;

    for (unsigned int i = 0; i < SPX_WOTS_LEN; i++) {
        while (bits_in_buf < wots_wlog[i]) {
            bitbuf = (bitbuf << 8) | *in++;
            bits_in_buf += 8;
        }
        bits_in_buf -= wots_wlog[i];
        lengths[i] = (unsigned int)(bitbuf >> bits_in_buf) & (wots_w[i] - 1u);
        csum += wots_w[i] - 1u - lengths[i];
    }

    return csum;
}

/**
 * Takes a WOTS signature and an n-byte message, computes a WOTS public key.
 *
 * Writes the computed public key to 'pk'.
 */
void wots_pk_from_sig(unsigned char *pk,
                      const unsigned char *sig, const unsigned char *msg,
                      const spx_ctx *ctx, uint32_t addr[8], uint32_t counter)
{
    unsigned int lengths[SPX_WOTS_LEN];
    uint32_t i;
    uint32_t mask =  (~0U << (8-WOTS_ZERO_BITS));
    unsigned char bitmask[SPX_N];

    /*Initial parameters for validation of checksum*/
    int csum;
    unsigned char digest[SPX_N];

    /*Set thash address for custom hash to type 6 & PK format*/
    uint32_t wots_pk_addr[8] = {0};
    set_type(wots_pk_addr, SPX_ADDR_TYPE_COMPRESS_WOTS);
    copy_keypair_addr(wots_pk_addr, addr);
    thash_init_bitmask(bitmask, 1, ctx, wots_pk_addr);

    /*Set padding*/
    ull_to_bytes(((unsigned char *) (wots_pk_addr))+(SPX_OFFSET_COUNTER) , COUNTER_SIZE, counter);
    /*Calculate checksum*/
    thash_fin(digest, msg, 1, ctx, wots_pk_addr, bitmask);
    

    csum = chain_lengths(lengths, digest);

    /*Validate Checksum*/
    if ((csum != WANTED_CHECKSUM) || (((digest[SPX_N-1]) & (mask)) !=0)){   
        memset(pk,0,SPX_PK_BYTES);
    }
    else
    {
    for (i = 0; i < SPX_WOTS_LEN; i++) {
        set_chain_addr(addr, i);
        gen_chain(pk + i*SPX_N, sig + i*SPX_N,
                  lengths[i], wots_w[i] - 1 - lengths[i], ctx, addr,i);
        }
    }
}
