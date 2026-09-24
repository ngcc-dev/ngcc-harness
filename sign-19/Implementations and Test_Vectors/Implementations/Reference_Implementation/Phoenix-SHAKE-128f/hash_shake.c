#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "address.h"
#include "utils.h"
#include "params.h"
#include "hash.h"
#include "fips202.h"

void initialize_hash_function(spx_ctx* ctx)
{
    shake256_inc_init(ctx->state_seeded_shake);
    shake256_inc_absorb(ctx->state_seeded_shake, ctx->pub_seed, SPX_N);
}

void prf_addr(unsigned char *out, const spx_ctx *ctx,
              const uint32_t addr[8])
{
    unsigned char buf[2*SPX_N + SPX_ADDR_BYTES];

    memcpy(buf, ctx->pub_seed, SPX_N);
    memcpy(buf + SPX_N, addr, SPX_ADDR_BYTES);
    memcpy(buf + SPX_N + SPX_ADDR_BYTES, ctx->sk_seed, SPX_N);

    shake256(out, SPX_N, buf, 2*SPX_N + SPX_ADDR_BYTES);
}

void gen_message_random(unsigned char *R, const unsigned char *sk_prf,
                        const unsigned char *optrand,
                        const unsigned char *m, unsigned long long mlen,
                        const spx_ctx *ctx)
{
    (void)ctx;
    uint64_t s_inc[26];

    shake256_inc_init(s_inc);
    shake256_inc_absorb(s_inc, sk_prf, SPX_N);
    shake256_inc_absorb(s_inc, optrand, SPX_N);
    shake256_inc_absorb(s_inc, m, mlen);
    shake256_inc_finalize(s_inc);
    shake256_inc_squeeze(R, SPX_N, s_inc);
}

void hash_message(unsigned char *digest, uint64_t *tree, uint32_t *leaf_idx,
                  const unsigned char *R, const unsigned char *pk,
                  const unsigned char *m, unsigned long long mlen,
                  const spx_ctx *ctx)
{
    (void)ctx;
#define SPX_TREE_BITS (SPX_TREE_HEIGHT * (SPX_D - 1))
#define SPX_TREE_BYTES ((SPX_TREE_BITS + 7) / 8)
#define SPX_LEAF_BITS SPX_TREE_HEIGHT
#define SPX_LEAF_BYTES ((SPX_LEAF_BITS + 7) / 8)
#define SPX_DGST_BYTES (SPX_TFORS_MSG_BYTES + SPX_TREE_BYTES + SPX_LEAF_BYTES)

    unsigned char buf[SPX_DGST_BYTES];
    unsigned char *bufp = buf;
    uint64_t s_inc[26];

    shake256_inc_init(s_inc);
    shake256_inc_absorb(s_inc, R, SPX_N);
    shake256_inc_absorb(s_inc, pk, SPX_PK_BYTES);
    shake256_inc_absorb(s_inc, m, mlen);
    shake256_inc_finalize(s_inc);
    shake256_inc_squeeze(buf, SPX_DGST_BYTES, s_inc);

    memcpy(digest, bufp, SPX_TFORS_MSG_BYTES);
    bufp += SPX_TFORS_MSG_BYTES;

#if SPX_TREE_BITS > 64 && !defined(ALLOW_DEEP_TREES)
    #error For given height and depth, 64 bits cannot represent all subtrees
#endif

    if (SPX_D == 1) {
        *tree = 0;
    } else {
        *tree = bytes_to_ull(bufp, SPX_TREE_BYTES);
        *tree &= (~(uint64_t)0) >> (64 - SPX_TREE_BITS);
    }
    bufp += SPX_TREE_BYTES;

    *leaf_idx = (uint32_t)bytes_to_ull(bufp, SPX_LEAF_BYTES);
    *leaf_idx &= (~(uint32_t)0) >> (32 - SPX_LEAF_BITS);
}

int SPX_h2_generate_indices(uint32_t *indices,
                        const unsigned char *msg_digest,
                        const spx_ctx *ctx)
{
    const uint32_t k = SPX_TFORS_K;
    const uint32_t k_prime = SPX_TFORS_K_PRIME;
    const uint32_t log2_kp = SPX_TFORS_LOG_K_PRIME;


    unsigned char input[SPX_N + SPX_N];
    memcpy(input, msg_digest, SPX_N);
    memcpy(input + SPX_N, ctx->pub_seed, SPX_N);

    unsigned char rnd[512];
    shake256(rnd, sizeof(rnd), input, sizeof(input));
    size_t rnd_ptr = 0;

    SPX_VLA(uint8_t, mask, (k_prime + 7) / 8);
    memset(mask, 0, (k_prime + 7) / 8);
    uint32_t count = 0;

    if (k <= k_prime / 2) {
        while (count < k) {
            if (rnd_ptr + 8 > sizeof(rnd)) return -1;
            uint64_t val = 0;
            for (int i = 0; i < 8; i++) {
                val = (val << 8) | rnd[rnd_ptr++];
            }
            uint32_t idx = (uint32_t)(val & ((1U << log2_kp) - 1));
            if (idx >= k_prime) continue;
            if (!(mask[idx / 8] & (1 << (idx % 8)))) {
                mask[idx / 8] |= (uint8_t)(1 << (idx % 8));
                indices[count++] = idx;
            }
        }
    } else {
        SPX_VLA(uint8_t, reject_mask, (k_prime + 7) / 8);
        memset(reject_mask, 0, (k_prime + 7) / 8);
        uint32_t reject_need = k_prime - k;
        count = 0;
        while (count < reject_need) {
            if (rnd_ptr + 8 > sizeof(rnd)) return -1;
            uint64_t val = 0;
            for (int i = 0; i < 8; i++) {
                val = (val << 8) | rnd[rnd_ptr++];
            }
            uint32_t idx = (uint32_t)(val & ((1U << log2_kp) - 1));
            if (idx >= k_prime) continue;
            if (!(reject_mask[idx / 8] & (1 << (idx % 8)))) {
                reject_mask[idx / 8] |= (uint8_t)(1 << (idx % 8));
                count++;
            }
        }
        count = 0;
        for (uint32_t i = 0; i < k_prime && count < k; i++) {
            if (!(reject_mask[i / 8] & (1 << (i % 8)))) {
                indices[count++] = i;
            }
        }
    }

    for (uint32_t i = 0; i < k; i++) {
        for (uint32_t j = i + 1; j < k; j++) {
            if (indices[i] > indices[j]) {
                uint32_t temp = indices[i];
                indices[i] = indices[j];
                indices[j] = temp;
            }
        }
    }

    return 0;
}