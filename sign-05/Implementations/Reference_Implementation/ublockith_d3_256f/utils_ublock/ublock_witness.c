#include "ublock.h"
#include "ublock_internal.h"
#include "utils.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* Round update matching ublock256_round_slow() arithmetic, without T-box dependency. */
static inline void ublock_round_witness(uint32_t* X0, uint32_t* X1, const uint32_t* rki)
{
    for (uint32_t w = 0; w < 4; ++w) {

        // Step 19-20: compute x0 oplus k0, x1 oplus k1
        uint32_t a = X0[w] ^ rki[w];
        uint32_t b = X1[w] ^ rki[w + 4];

        // Step 19-20: compute subQWord by table
        a = ((uint32_t)UBLOCK_SBOX_BYTE[a >> 24] << 24) |
            ((uint32_t)UBLOCK_SBOX_BYTE[(a >> 16) & 0xFF] << 16) |
            ((uint32_t)UBLOCK_SBOX_BYTE[(a >> 8) & 0xFF] << 8) |
            (uint32_t)UBLOCK_SBOX_BYTE[a & 0xFF];

        b = ((uint32_t)UBLOCK_SBOX_BYTE[b >> 24] << 24) |
            ((uint32_t)UBLOCK_SBOX_BYTE[(b >> 16) & 0xFF] << 16) |
            ((uint32_t)UBLOCK_SBOX_BYTE[(b >> 8) & 0xFF] << 8) |
            (uint32_t)UBLOCK_SBOX_BYTE[b & 0xFF];

        // Step 21-26
        b ^= a;
        a ^= rotl32_u(b, 4);
        b ^= rotl32_u(a, 8);
        a ^= rotl32_u(b, 8);
        b ^= rotl32_u(a, 20);
        a ^= b;

        X0[w] = a;
        X1[w] = b;
    }

    // Step 27-28
    /* ublock_perm_PL/PR read all 4 words into locals before writing,
       so in-place (out == in) is safe here. */
    ublock_perm_PL(X0, X0);
    ublock_perm_PR(X1, X1);
}

static void ublock_store_roundkey_256(uint8_t* out, const uint32_t* rk)
{
    for (uint32_t i = 0; i < 8; ++i) {
        store_u32_be(rk[i], out + 4 * i);
    }
}

static void ublock_store_roundkey_prefix64(uint8_t* out, const uint32_t* rk)
{
    store_u32_be(rk[0], out + 0);
    store_u32_be(rk[1], out + 4);
}

static void ublock_store_state_256(uint8_t* out, const uint32_t* X0, const uint32_t* X1)
{
    for (uint32_t i = 0; i < 4; ++i) {
        store_u32_be(X0[i], out + 4 * i);
        store_u32_be(X1[i], out + 16 + 4 * i);
    }
}

void ublock_extend_witness(const params_t* params, uint8_t* w, const uint8_t* key, const uint8_t* in)
{
    const bool use_em = is_em(params);

#if !defined(NDEBUG)
    uint8_t* const w_out = w;
#endif

    // For em, key is owf_key, is uBlock plaintext input
    // in is owf_input, is uBlock key
    // For non-em, key is uBlock key, in is uBlock plaintext input
    if (use_em) {
        // switch input and key for EM
        const uint8_t* tmp = key;
        key                = in;
        in                 = tmp;
    }

    ublock256_key_t ks;
    // Step 5
    if (ublock256_set_key(key, &ks) != 0) {
        return;
    }

    // Step 6
    if (!use_em) {
        // Step 7
        ublock_store_roundkey_256(w, ks.rk[0]);
        w += UBLOCK_BLOCK;

        // Step 8
        for (uint32_t i = 1; i <= UBLOCK_ROUNDS; ++i) {
            // Step 9-10
            ublock_store_roundkey_prefix64(w, ks.rk[i]);
            w += 8;
        }
    // Step 11
    } else {
        // Step 12-13
        memcpy(w, in, UBLOCK_BLOCK);
        w += UBLOCK_BLOCK;
    }

    // Step 14
    assert((size_t)(w - w_out) == params->lke / 8);

    // Step 15-16
    uint32_t X0[4], X1[4];
    for (uint32_t i = 0; i < 4; ++i) {
        X0[i] = load_u32_be(in, i);
        X1[i] = load_u32_be(in + 16, i);
    }

    // Step 17
    for (uint32_t i = 0; i < UBLOCK_ROUNDS; ++i) {
        // Step 18-28
        ublock_round_witness(X0, X1, ks.rk[i]);

        /* Already store S0 in Line 97 for em,
        * or S0 is plaintext for nom-em
        */
        /* Store S2,S4,...,S22 (11 states for 24 rounds). */
        if (((i % 2) == 1u) && (i < (UBLOCK_ROUNDS - 1u))) {
            ublock_store_state_256(w, X0, X1);
            w += UBLOCK_BLOCK;
        }
    }

    assert((size_t)(w - w_out) == params->ell / 8);
}
