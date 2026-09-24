#include <stdint.h>
#include <string.h>

#include "utils.h"
#include "utilsx1.h"
#include "hash.h"
#include "thash.h"
#include "gwots.h"
#include "gwotsx1.h"
#include "address.h"
#include "params.h"


/**
 * Computes the chaining function.
 * out and in have to be n-byte arrays.
 *
 * Interprets in as start-th value of the chain.
 * addr has to contain the address of the chain.
 */
/**
 * Computes the chaining function.
 * out and in have to be n-byte arrays.
 *
 * Interprets in as start-th value of the chain.
 * addr has to contain the address of the chain.
 * Support w=SPX_GWOTS_W1 or w=SPX_GWOTS_W2.
 */
static void gen_chain(unsigned char *out, const unsigned char *in,
                      unsigned int start, unsigned int steps,
                      const spx_ctx *ctx, uint32_t addr[8],
                      uint32_t w)
{
    uint32_t i;

    /* Initialize out with the value at position 'start'. */
    memcpy(out, in, SPX_N);

    /* Iterate 'steps' calls to the hash function. */
    for (i = start; i < (start + steps) && i < w; i++)
    {
        set_hash_addr(addr, i);
        thash(out, out, 1, ctx, addr);
    }
}

/**
 * base_w algorithm as described in draft.
 * Interprets an array of bytes as integers in base w.
 */
static void base_w(unsigned int *output,
                   const unsigned char *input)
{
    int i, j;
    unsigned int offset = 0;

    /* Handle first part: W1 chain */
    for (i = 0; i < SPX_GWOTS_W1_LEN; i++)
    {
        output[i] = 0;
        for (j = 0; j < SPX_GWOTS_LOGW1; j++)
        {
            unsigned int byte_pos = offset >> 3;
            unsigned int bit_in_byte = offset & 0x7;
            unsigned int bit = 0;
            if (byte_pos < SPX_N) {
                bit = (unsigned int)((input[byte_pos] >> bit_in_byte) & 0x1);
            }
            output[i] ^= bit << j;
            offset++;
        }
    }

    /* Handle second part: W2 chain */
    for (i = SPX_GWOTS_W1_LEN; i < SPX_GWOTS_LEN1; i++)
    {
        output[i] = 0;
        for (j = 0; j < SPX_GWOTS_LOGW2; j++)
        {
            unsigned int byte_pos = offset >> 3;
            unsigned int bit_in_byte = offset & 0x7;
            unsigned int bit = 0;
            if (byte_pos < SPX_N) {
                bit = (unsigned int)((input[byte_pos] >> bit_in_byte) & 0x1);
            }
            output[i] ^= bit << j;
            offset++;
        }
    }
}

/** 
 * Computes the checksum over a message (in variable base_w). 
 * Compare with the functions in sphincsplus, we treat void functions as functions with return values as done in sphincsplusc.
*/
static unsigned int gwots_checksum(const unsigned int *msg_base_w)
{
    unsigned int csum = 0;
    unsigned int i = 0;

    /* Compute checksum. */
    for (; i < SPX_GWOTS_W1_LEN; i++)
    {
        csum += SPX_GWOTS_W1 - 1 - msg_base_w[i];
    }

    for (; i < SPX_GWOTS_W1_LEN + SPX_GWOTS_W2_LEN; i++)
    {
        csum += SPX_GWOTS_W2 - 1 - msg_base_w[i];
    }

    return csum;
}

/* Takes a message and derives the matching chain lengths. */
unsigned int chain_lengths(unsigned int *lengths, const unsigned char *msg)
{
    unsigned int csum;

    base_w(lengths, msg);
    csum = gwots_checksum(lengths);
    return csum;
}

/**
 * Takes a GWOTS signature and an n-byte message, computes a GWOTS public key.
 * Uses the range-constrained search logic with variable-length chains.
 * Writes the computed public key to 'pk'.
 */
void gwots_pk_from_sig(unsigned char *pk,
                      const unsigned char *sig, const unsigned char *msg,
                      const spx_ctx *ctx, uint32_t addr[8], uint32_t counter)
{
    unsigned int lengths[SPX_GWOTS_LEN];
    uint32_t i;
    unsigned char bitmask[SPX_N];

    /*Initial parameters for validation of checksum*/
    int csum;
    unsigned char digest[SPX_N];

    /*Set thash address for custom hash to type 6 & PK format*/
    uint32_t gwots_pk_addr[8] = {0};
    copy_subtree_addr(gwots_pk_addr, addr);

    copy_keypair_addr(gwots_pk_addr, addr); 
    set_type(gwots_pk_addr, SPX_ADDR_TYPE_COMPRESS_GWOTS);
    thash_init_bitmask(bitmask, 1, ctx, gwots_pk_addr);

    /*Set padding*/
    ull_to_bytes(((unsigned char *)(gwots_pk_addr)) + (SPX_OFFSET_COUNTER), COUNTER_SIZE, counter);
    /*Calculate checksum*/
    thash_fin(digest, msg, 1, ctx, gwots_pk_addr, bitmask);


    memset(lengths, 0, sizeof(lengths));
    csum = chain_lengths(lengths, digest);
   
    /*Validate Checksum*/
    if ((csum < GWOTS_SUM_BASE) ||
        (csum > (GWOTS_SUM_BASE + GWOTS_SUM_RANGE)))
    {
        /* Validation failed: invalidate the public key */
        memset(pk, 0, SPX_GWOTS_BYTES);
    }
    else
    {
        /* Validation successful: proceed with PK reconstruction */

        set_type(addr, SPX_ADDR_TYPE_GWOTS); 
        ull_to_bytes(((unsigned char *)(addr)) + (SPX_OFFSET_COUNTER), COUNTER_SIZE, 0);

        /* Reconstruct W1 chains. */
        for (i = 0; i < SPX_GWOTS_W1_LEN; i++)
        {
            set_chain_addr(addr, i);
            gen_chain(pk + i * SPX_N, sig + i * SPX_N,
                      lengths[i], SPX_GWOTS_W1 - 1 - lengths[i], ctx, addr, SPX_GWOTS_W1);
        }

        /* Reconstruct W2 chains. */
        for (; i < SPX_GWOTS_LEN1; i++)
        {
            set_chain_addr(addr, i);
            gen_chain(pk + i * SPX_N, sig + i * SPX_N,
                      lengths[i], SPX_GWOTS_W2 - 1 - lengths[i], ctx, addr, SPX_GWOTS_W2);
        }

        if (SPX_GWOTS_LEN2 > 0) {
            lengths[SPX_GWOTS_LEN1] = (unsigned int)(csum - GWOTS_SUM_BASE);
            set_chain_addr(addr, i);
            gen_chain(pk + i * SPX_N, sig + i * SPX_N,
                      lengths[i], SPX_GWOTS_CHECKSUM_W - 1 - lengths[i], ctx, addr, SPX_GWOTS_CHECKSUM_W);
        }
    }
}
