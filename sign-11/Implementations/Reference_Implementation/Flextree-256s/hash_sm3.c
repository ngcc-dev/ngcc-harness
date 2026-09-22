#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "address.h"
#include "utils.h"
#include "params.h"
#include "hash.h"

#include "auxfunc.h"


void initialize_hash_function(spx_ctx* ctx)
{
    (void)ctx; /* Suppress an 'unused parameter' warning. */
}

void prf_addr(unsigned char *out, const spx_ctx *ctx,
              const uint32_t addr[8])
{
    unsigned char buf[2*SPX_N + SPX_ADDR_BYTES];

    memcpy(buf, ctx->pub_seed, SPX_N);
    memcpy(buf + SPX_N, addr, SPX_ADDR_BYTES);
    memcpy(buf + SPX_N + SPX_ADDR_BYTES, ctx->sk_seed, SPX_N);

    unsigned long long in_len_bits = (unsigned long long)(2*SPX_N + SPX_ADDR_BYTES) * 8;

    if (SPX_N <= 32) {
        unsigned char temp_out[32];
        sm3hash(256, buf, in_len_bits, temp_out);
        memcpy(out, temp_out, SPX_N);
    } else {
        unsigned long long out_len_bits = (unsigned long long)SPX_N * 8;
        pseudoXOF(out_len_bits, buf, in_len_bits, out);
    }
}

void gen_message_random(unsigned char *R, const unsigned char *sk_prf,
                        const unsigned char *optrand,
                        const unsigned char *m, unsigned long long mlen,
                        const spx_ctx *ctx)
{
    (void)ctx;
    
    unsigned long long total_bytes = 2 * SPX_N + mlen;
    unsigned char *buf = (unsigned char *)malloc(total_bytes);
    if (buf == NULL) {
        return; 
    }

    memcpy(buf, sk_prf, SPX_N);
    memcpy(buf + SPX_N, optrand, SPX_N);
    memcpy(buf + 2 * SPX_N, m, mlen);

    unsigned long long in_len_bits = total_bytes * 8;

    if (SPX_N <= 32) {
        unsigned char temp_out[32];
        sm3hash(256, buf, in_len_bits, temp_out);
        memcpy(R, temp_out, SPX_N);
    } else {
        unsigned long long out_len_bits = (unsigned long long)SPX_N * 8;
        pseudoXOF(out_len_bits, buf, in_len_bits, R);
    }

    free(buf);
}

static void hash_message_digest(unsigned char *buf_out, const unsigned char *R, const unsigned char *pk,
                                const unsigned char *m, unsigned long long mlen)
{
#define SPX_TREE_BITS (SPX_FULL_HEIGHT - (SPX_BOTTOM_TREE_HEIGHT))
#define SPX_TREE_BYTES ((SPX_TREE_BITS + 7) / 8)
#define SPX_LEAF_BITS (SPX_BOTTOM_TREE_HEIGHT)
#define SPX_LEAF_BYTES ((SPX_LEAF_BITS + 7) / 8)
#define SPX_DGST_BYTES (SPX_PORS_FP_MSG_BYTES + SPX_TREE_BYTES + SPX_LEAF_BYTES)

    unsigned long long total_bytes = SPX_N + SPX_PK_BYTES + mlen;
    unsigned char *buf = (unsigned char *)malloc(total_bytes);
    if (buf == NULL) {
        return;
    }

    memcpy(buf, R, SPX_N);
    memcpy(buf + SPX_N, pk, SPX_PK_BYTES);
    memcpy(buf + SPX_N + SPX_PK_BYTES, m, mlen);

    unsigned long long in_len_bits = total_bytes * 8;
    unsigned long long out_len_bits = (unsigned long long)SPX_DGST_BYTES * 8;

    pseudoXOF(out_len_bits, buf, in_len_bits, buf_out);

    free(buf);
}


int hash_message(unsigned char *digest, uint64_t *tree, uint32_t *leaf_idx,
                  const unsigned char *R, const unsigned char *pk,
                  const unsigned char *m, unsigned long long mlen,
                  const spx_ctx *ctx)
{
    (void)ctx;
    unsigned char buf[SPX_DGST_BYTES];
    unsigned char *bufp = buf;
    
    hash_message_digest(buf, R, pk, m, mlen);

#if SPX_TREE_BITS > 64
    #error For given height and depth, 64 bits cannot represent all subtrees
#endif

    *tree = bytes_to_ull(bufp, SPX_TREE_BYTES);
    *tree &= (~(uint64_t)0) >> (64 - SPX_TREE_BITS);
    bufp += SPX_TREE_BYTES;

    *leaf_idx = bytes_to_ull(bufp, SPX_LEAF_BYTES);
    *leaf_idx &= (~(uint32_t)0) >> (32 - SPX_LEAF_BITS);
    bufp += SPX_LEAF_BYTES;

    memcpy(digest, bufp, SPX_PORS_FP_MSG_BYTES);
    return 0;
}
