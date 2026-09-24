#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "address.h"
#include "utils.h"
#include "params.h"
#include "hash.h"
#include "hash_sm3.h"

/* For SM3, there is no immediate reason to initialize at the start,
   so this function is an empty operation. */
void initialize_hash_function(spx_ctx *ctx)
{
    seed_state_sm3(ctx);
}

/*
 * Computes PRF(pk_seed, sk_seed, addr).
 */
void prf_addr(unsigned char *out, const spx_ctx *ctx,
              const uint32_t addr[8])
{
    uint8_t sm3_state[40];
    unsigned char buf[SPX_SM3_ADDR_BYTES + SPX_N];
    unsigned char outbuf[SPX_SM3_OUTPUT_BYTES];

    /* Retrieve precomputed state containing pub_seed */
    memcpy(sm3_state, ctx->state_seeded_sm3, 40 * sizeof(uint8_t));

    /* Remainder: ADDR^c ‖ SK.seed */
    memcpy(buf, addr, SPX_SM3_ADDR_BYTES);
    memcpy(buf + SPX_SM3_ADDR_BYTES, ctx->sk_seed, SPX_N);

    sm3_inc_finalize(outbuf, sm3_state, buf, SPX_SM3_ADDR_BYTES + SPX_N);

    memcpy(out, outbuf, SPX_N);
}

/**
 * Computes the message-dependent randomness R, using a secret seed as a key
 * for HMAC, and an optional randomization value prefixed to the message.
 */
void gen_message_random(unsigned char *R, const unsigned char *sk_prf,
                        const unsigned char *optrand,
                        const unsigned char *m, unsigned long long mlen,
                        const spx_ctx *ctx)
{
    (void)ctx;

    unsigned char buf[SPX_SM3_BLOCK_BYTES + SPX_SM3_OUTPUT_BYTES];
    uint8_t state[8 + SPX_SM3_OUTPUT_BYTES];
    int i;

#if SPX_N > SPX_SM3_BLOCK_BYTES
    #error "Currently only supports SPX_N of at most SPX_SM3_BLOCK_BYTES"
#endif

    /* This implements HMAC-SM3 */
    for (i = 0; i < SPX_N / 4; i++) {
        ((uint32_t *)buf)[i] = 0x36363636u ^ ((const uint32_t *)sk_prf)[i];
    }
    for (i = (SPX_N / 4) * 4; i < SPX_N; i++) {
        buf[i] = 0x36 ^ sk_prf[i];
    }
    memset(buf + SPX_N, 0x36, SPX_SM3_BLOCK_BYTES - SPX_N);

    sm3_inc_init(state);
    sm3_inc_blocks(state, buf, 1);

    memcpy(buf, optrand, SPX_N);

    /* If optrand + message cannot fill up an entire block */
    if (SPX_N + mlen < SPX_SM3_BLOCK_BYTES) {
        memcpy(buf + SPX_N, m, mlen);
        sm3_inc_finalize(buf + SPX_SM3_BLOCK_BYTES, state,
                            buf, mlen + SPX_N);
    }
    /* Otherwise first fill a block, so that finalize only uses the message */
    else {
        memcpy(buf + SPX_N, m, SPX_SM3_BLOCK_BYTES - SPX_N);
        sm3_inc_blocks(state, buf, 1);

        m += SPX_SM3_BLOCK_BYTES - SPX_N;
        mlen -= SPX_SM3_BLOCK_BYTES - SPX_N;
        sm3_inc_finalize(buf + SPX_SM3_BLOCK_BYTES, state, m, mlen);
    }

    for (i = 0; i < SPX_N / 4; i++) {
        ((uint32_t *)buf)[i] = 0x5c5c5c5cu ^ ((const uint32_t *)sk_prf)[i];
    }
    for (i = (SPX_N / 4) * 4; i < SPX_N; i++) {
        buf[i] = 0x5c ^ sk_prf[i];
    }
    memset(buf + SPX_N, 0x5c, SPX_SM3_BLOCK_BYTES - SPX_N);

    sm3(buf, buf, SPX_SM3_BLOCK_BYTES + SPX_SM3_OUTPUT_BYTES);
    memcpy(R, buf, SPX_N);
}

/**
 * Computes the message hash using R, the public key, and the message.
 * Outputs the message digest and the index of the leaf. The index is split in
 * the tree index and the leaf index, for convenient copying to an address.
 */
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

    unsigned char seed[2*SPX_N + SPX_SM3_OUTPUT_BYTES];

    /* Round to nearest multiple of SPX_SM3_BLOCK_BYTES */
#if (SPX_SM3_BLOCK_BYTES & (SPX_SM3_BLOCK_BYTES-1)) != 0
    #error "Assumes that SPX_SM3_BLOCK_BYTES is a power of 2"
#endif
#define SPX_INBLOCKS (((SPX_N + SPX_PK_BYTES + SPX_SM3_BLOCK_BYTES - 1) & \
                        -SPX_SM3_BLOCK_BYTES) / SPX_SM3_BLOCK_BYTES)
    unsigned char inbuf[SPX_INBLOCKS * SPX_SM3_BLOCK_BYTES];

    unsigned char buf[SPX_DGST_BYTES];
    unsigned char *bufp = buf;
    uint8_t state[8 + SPX_SM3_OUTPUT_BYTES];

    sm3_inc_init(state);

    // seed: SM3(R ‖ PK.seed ‖ PK.root ‖ M)
    memcpy(inbuf, R, SPX_N);
    memcpy(inbuf + SPX_N, pk, SPX_PK_BYTES);

    /* If R + pk + message cannot fill up an entire block */
    if (SPX_N + SPX_PK_BYTES + mlen < SPX_INBLOCKS * SPX_SM3_BLOCK_BYTES) {
        memcpy(inbuf + SPX_N + SPX_PK_BYTES, m, mlen);
        sm3_inc_finalize(seed + 2*SPX_N, state, inbuf, SPX_N + SPX_PK_BYTES + mlen);
    }
    /* Otherwise first fill a block, so that finalize only uses the message */
    else {
        memcpy(inbuf + SPX_N + SPX_PK_BYTES, m,
               SPX_INBLOCKS * SPX_SM3_BLOCK_BYTES - SPX_N - SPX_PK_BYTES);
        sm3_inc_blocks(state, inbuf, SPX_INBLOCKS);

        m += SPX_INBLOCKS * SPX_SM3_BLOCK_BYTES - SPX_N - SPX_PK_BYTES;
        mlen -= SPX_INBLOCKS * SPX_SM3_BLOCK_BYTES - SPX_N - SPX_PK_BYTES;
        sm3_inc_finalize(seed + 2*SPX_N, state, m, mlen);
    }

    // H_msg: MGF1-SM3(R ‖ PK.seed ‖ seed)
    memcpy(seed, R, SPX_N);
    memcpy(seed + SPX_N, pk, SPX_N);

    /* By doing this in two steps, we prevent hashing the message twice;
       otherwise each iteration in MGF1 would hash the message again. */
    mgf1_sm3(bufp, SPX_DGST_BYTES, seed, 2*SPX_N + SPX_SM3_OUTPUT_BYTES);

    memcpy(digest, bufp, SPX_TFORS_MSG_BYTES);
    bufp += SPX_TFORS_MSG_BYTES;

#if SPX_TREE_BITS > 64
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

/**
 * H₂ function: Generates k distinct key-set indices using SM3-based MGF1.
 */
int h2_generate_indices(uint32_t *indices,
                        const unsigned char *msg_digest,
                        const spx_ctx *ctx)
{
    const uint32_t k = SPX_TFORS_K;
    const uint32_t k_prime = SPX_TFORS_K_PRIME;
    const uint64_t mask_kp = ((1ULL << SPX_TFORS_LOG_K_PRIME) - 1);

    unsigned char hash_input[SPX_N + SPX_N];
    memcpy(hash_input, msg_digest, SPX_N);
    memcpy(hash_input + SPX_N, ctx->pub_seed, SPX_N);

    unsigned char rnd[512];
    mgf1_sm3(rnd, sizeof(rnd), hash_input, sizeof(hash_input));
    size_t rnd_ptr = 0;

    // 统一使用排除法，输出天然有序，无需排序
    SPX_VLA(uint8_t, reject_mask, (k_prime + 7) / 8);
    memset(reject_mask, 0, (k_prime + 7) / 8);

    uint32_t reject_need = k_prime - k;
    uint32_t reject_count = 0;

    while (reject_count < reject_need) {
        if (rnd_ptr + 8 > sizeof(rnd)) return -1;

        uint64_t val = bytes_to_ull(rnd + rnd_ptr, 8);
        rnd_ptr += 8;

        uint32_t idx = (uint32_t)(val & mask_kp);
        if (idx < k_prime) {
            uint32_t byte_idx = idx >> 3;    // idx / 8
            uint32_t bit_idx = idx & 7;      // idx % 8
            uint8_t bit_mask = (uint8_t)(1U << bit_idx);
            if (!(reject_mask[byte_idx] & bit_mask)) {
                reject_mask[byte_idx] |= bit_mask;
                reject_count++;
            }
        }
    }

    uint32_t idx_count = 0;
    for (uint32_t i = 0; i < k_prime && idx_count < k; i++) {
        uint32_t byte_idx = i >> 3;    // i / 8
        uint32_t bit_idx = i & 7;      // i % 8
        if (!(reject_mask[byte_idx] & ((uint8_t)(1U << bit_idx)))) {
            indices[idx_count++] = i;
        }
    }

    return 0;
}
