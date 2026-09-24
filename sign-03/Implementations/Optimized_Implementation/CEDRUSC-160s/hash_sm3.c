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

void hash_with_counter(unsigned char *buf_out, const unsigned char *R, const unsigned char *pk,
                       const unsigned char *m, unsigned long long mlen, unsigned char *counter_bytes)
{
#define MAX_HASH_TRIALS_FORS (1 << (SPX_FORS_ZERO_LAST_BITS + 10))
#define SPX_FORS_ZEROED_BYTES ((SPX_FORS_ZERO_LAST_BITS + 7) / 8)

#define SPX_TREE_BITS (SPX_FULL_HEIGHT - SPX_TREE_HEIGHT + 1)
#define SPX_TREE_BYTES ((SPX_TREE_BITS + 7) / 8)
#define SPX_LEAF_BITS SPX_BOTTOM_TREE_HEIGHT
#define SPX_LEAF_BYTES ((SPX_LEAF_BITS + 7) / 8)
#define SPX_DGST_BYTES (SPX_FORS_ZEROED_BYTES+SPX_FORS_MSG_BYTES + SPX_TREE_BYTES + SPX_LEAF_BYTES)

    unsigned long long total_bytes = SPX_N + SPX_PK_BYTES + mlen + COUNTER_SIZE;
    unsigned char *buf = (unsigned char *)malloc(total_bytes);
    if (buf == NULL) {
        return;
    }

    memcpy(buf, R, SPX_N);
    memcpy(buf + SPX_N, pk, SPX_PK_BYTES);
    memcpy(buf + SPX_N + SPX_PK_BYTES, m, mlen);
    memcpy(buf + SPX_N + SPX_PK_BYTES + mlen, counter_bytes, COUNTER_SIZE);

    unsigned long long in_len_bits = total_bytes * 8;
    unsigned long long out_len_bits = (unsigned long long)SPX_DGST_BYTES * 8;

    pseudoXOF(out_len_bits, buf, in_len_bits, buf_out);

    free(buf);
}


int hash_message(unsigned char *digest, uint64_t *tree, uint32_t *leaf_idx,
                  const unsigned char *R, const unsigned char *pk,
                  const unsigned char *m, unsigned long long mlen,
                  const spx_ctx *ctx, uint32_t *counter)
{
    (void)ctx;
    unsigned char buf[SPX_DGST_BYTES];
    unsigned char *bufp = buf;
    int found_flag=1;
    unsigned long long int mask =  ~(~0U << (SPX_FORS_ZERO_LAST_BITS));
    unsigned long long int zero_bits;
    unsigned long long total_bytes = SPX_N + SPX_PK_BYTES + mlen + COUNTER_SIZE;
    unsigned long long in_len_bits = total_bytes * 8;
    unsigned long long out_len_bits = (unsigned long long)SPX_DGST_BYTES * 8;
    unsigned char *hash_in = (unsigned char *)malloc(total_bytes);
    unsigned char *counter_ptr;

    if (hash_in == NULL) {
        return -1;
    }

    memcpy(hash_in, R, SPX_N);
    memcpy(hash_in + SPX_N, pk, SPX_PK_BYTES);
    memcpy(hash_in + SPX_N + SPX_PK_BYTES, m, mlen);
    counter_ptr = hash_in + SPX_N + SPX_PK_BYTES + mlen;

    /*verify stage*/
    if (*counter!=0)
    {
        ull_to_bytes(counter_ptr, COUNTER_SIZE, *counter);
        pseudoXOF(out_len_bits, hash_in, in_len_bits, buf);
        //If the expected bits are not zero the verification fails.
        zero_bits = bytes_to_ull(bufp, SPX_FORS_ZEROED_BYTES);
        if( (zero_bits& (mask))!=0) {
            free(hash_in);
            return -1;
        }
    }
    /*sign stage, searches for counter*/
    else
    {
        while(found_flag)
        {
            //counter should be 1 or larger.
            (*counter)++;
            if (*counter > MAX_HASH_TRIALS_FORS) {
                free(hash_in);
                return -1;
            }
            ull_to_bytes(counter_ptr, COUNTER_SIZE, *counter);
            pseudoXOF(out_len_bits, hash_in, in_len_bits, buf);
            zero_bits = bytes_to_ull(bufp, SPX_FORS_ZEROED_BYTES);
            if( (zero_bits & (mask))==0)
            {
                found_flag = 0;
                /*Save Counter*/
                break;
            }

        }
    }
    bufp += SPX_FORS_ZEROED_BYTES;

    memcpy(digest, bufp, SPX_FORS_MSG_BYTES);
    bufp += SPX_FORS_MSG_BYTES;

#if SPX_TREE_BITS > 64
    #error For given height and depth, 64 bits cannot represent all subtrees
#endif

    *tree = 0;
    bufp += SPX_TREE_BYTES;

    *leaf_idx = bytes_to_ull(bufp, SPX_LEAF_BYTES);
    *leaf_idx &= (~(uint32_t)0) >> (32 - SPX_LEAF_BITS);
    free(hash_in);
    return 0;
}
