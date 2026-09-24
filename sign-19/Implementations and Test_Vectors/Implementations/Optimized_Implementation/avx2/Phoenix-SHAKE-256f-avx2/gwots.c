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

/* Compute chain value by iterating hash function from start position. */
static void gen_chain(unsigned char *out, const unsigned char *in,
                      unsigned int start, unsigned int steps,
                      const spx_ctx *ctx, uint32_t addr[8],
                      uint32_t w)
{
    uint32_t idx;
    memcpy(out, in, SPX_N);

    for (idx = start; idx < (start + steps) && idx < w; idx++)
    {
        set_hash_addr(addr, idx);
        thash(out, out, 1, ctx, addr);
    }
}

/* Convert bytes to base-w integers for variable-length chain parameters. */
static void base_w(unsigned int *output,
                   const unsigned char *input)
{
    int seg_idx, bit_idx;
    unsigned int bit_offset = 0;

    /* W1 chain segment */
    for (seg_idx = 0; seg_idx < SPX_GWOTS_W1_LEN; seg_idx++)
    {
        output[seg_idx] = 0;
        for (bit_idx = 0; bit_idx < SPX_GWOTS_LOGW1; bit_idx++)
        {
            unsigned int byte_idx = bit_offset >> 3;
            unsigned int bit_pos = bit_offset & 0x7;
            unsigned int bit_val = 0;
            if (byte_idx < SPX_N) {
                bit_val = (unsigned int)((input[byte_idx] >> bit_pos) & 0x1);
            }
            output[seg_idx] ^= bit_val << bit_idx;
            bit_offset++;
        }
    }

    /* W2 chain segment */
    for (seg_idx = SPX_GWOTS_W1_LEN; seg_idx < SPX_GWOTS_LEN1; seg_idx++)
    {
        output[seg_idx] = 0;
        for (bit_idx = 0; bit_idx < SPX_GWOTS_LOGW2; bit_idx++)
        {
            unsigned int byte_idx = bit_offset >> 3;
            unsigned int bit_pos = bit_offset & 0x7;
            unsigned int bit_val = 0;
            if (byte_idx < SPX_N) {
                bit_val = (unsigned int)((input[byte_idx] >> bit_pos) & 0x1);
            }
            output[seg_idx] ^= bit_val << bit_idx;
            bit_offset++;
        }
    }
}

/* Compute checksum for message in variable base-w representation. */
static unsigned int gwots_checksum(const unsigned int *msg_base_w)
{
    unsigned int csum = 0;
    unsigned int seg_idx = 0;

    for (seg_idx = 0; seg_idx < SPX_GWOTS_W1_LEN; seg_idx++)
    {
        csum += SPX_GWOTS_W1 - 1 - msg_base_w[seg_idx];
    }

    for (seg_idx = 0; seg_idx < SPX_GWOTS_W2_LEN; seg_idx++)
    {
        csum += SPX_GWOTS_W2 - 1 - msg_base_w[SPX_GWOTS_W1_LEN + seg_idx];
    }

    return csum;
}

/* Derive chain lengths from message using base-w conversion. */
unsigned int chain_lengths(unsigned int *lengths, const unsigned char *msg)
{
    unsigned int csum;
    base_w(lengths, msg);
    csum = gwots_checksum(lengths);
    return csum;
}

/* Reconstruct GGWOT public key from signature using range-constrained chains. */
void gwots_pk_from_sig(unsigned char *pk,
                      const unsigned char *sig, const unsigned char *msg,
                      const spx_ctx *ctx, uint32_t addr[8], uint32_t counter)
{
    unsigned int lengths[SPX_GWOTS_LEN];
    uint32_t chain_idx;
    unsigned char bitmask[SPX_N];
    int csum;
    unsigned char digest[SPX_N];
    uint32_t gwots_pk_addr[8] = {0};

    copy_tree_addr(gwots_pk_addr, addr);
    copy_keypair_addr(gwots_pk_addr, addr);
    set_addr_type(gwots_pk_addr, SPX_ADDR_TYPE_COMPRESS_GWOTS);
    thash_init_bitmask(bitmask, 1, ctx, gwots_pk_addr);

    ull_to_bytes(((unsigned char *)(gwots_pk_addr)) + (PH_OFFSET_COUNTER), COUNTER_SIZE, counter);
    thash_fin(digest, msg, 1, ctx, gwots_pk_addr, bitmask);

    memset(lengths, 0, sizeof(lengths));
    csum = chain_lengths(lengths, digest);

    if ((csum < GWOTS_SUM_BASE) ||
        (csum > (GWOTS_SUM_BASE + GWOTS_SUM_RANGE)))
    {
        memset(pk, 0, SPX_GWOTS_BYTES);
    }
    else
    {
        set_addr_type(addr, SPX_ADDR_TYPE_GWOTS);
        ull_to_bytes(((unsigned char *)(addr)) + (PH_OFFSET_COUNTER), COUNTER_SIZE, 0);

        /* W1 chain reconstruction */
        for (chain_idx = 0; chain_idx < SPX_GWOTS_W1_LEN; chain_idx++)
        {
            set_chain_addr(addr, chain_idx);
            gen_chain(pk + chain_idx * SPX_N, sig + chain_idx * SPX_N,
                      lengths[chain_idx], SPX_GWOTS_W1 - 1 - lengths[chain_idx], ctx, addr, SPX_GWOTS_W1);
        }

        /* W2 chain reconstruction */
        for (; chain_idx < SPX_GWOTS_LEN1; chain_idx++)
        {
            set_chain_addr(addr, chain_idx);
            gen_chain(pk + chain_idx * SPX_N, sig + chain_idx * SPX_N,
                      lengths[chain_idx], SPX_GWOTS_W2 - 1 - lengths[chain_idx], ctx, addr, SPX_GWOTS_W2);
        }

        /* Checksum chain reconstruction */
        if (SPX_GWOTS_LEN2 > 0) {
            lengths[SPX_GWOTS_LEN1] = (unsigned int)(csum - GWOTS_SUM_BASE);
            set_chain_addr(addr, chain_idx);
            gen_chain(pk + chain_idx * SPX_N, sig + chain_idx * SPX_N,
                      lengths[chain_idx], SPX_GWOTS_CHECKSUM_W - 1 - lengths[chain_idx], ctx, addr, SPX_GWOTS_CHECKSUM_W);
        }
    }
}
