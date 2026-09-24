/* AES-256-CTR via AES-NI (no OpenSSL).  Used as the fast PRF/XOF substitute
 * in hybrid mode (RHYME_USE_AES): Agen expansion (stream128) and all sampling
 * streams (stream256), mirroring the old scheme's hybrid design. */
#include <stdint.h>
#include <string.h>
#include <wmmintrin.h>
#include <emmintrin.h>
#include "aes256ctr.h"

static __m128i expand_step(__m128i k0, __m128i kga) {
    kga = _mm_shuffle_epi32(kga, 0xff);
    k0 = _mm_xor_si128(k0, _mm_slli_si128(k0, 4));
    k0 = _mm_xor_si128(k0, _mm_slli_si128(k0, 4));
    k0 = _mm_xor_si128(k0, _mm_slli_si128(k0, 4));
    return _mm_xor_si128(k0, kga);
}
static __m128i expand_step2(__m128i k1, __m128i k0n) {
    __m128i kga = _mm_aeskeygenassist_si128(k0n, 0);
    kga = _mm_shuffle_epi32(kga, 0xaa);
    k1 = _mm_xor_si128(k1, _mm_slli_si128(k1, 4));
    k1 = _mm_xor_si128(k1, _mm_slli_si128(k1, 4));
    k1 = _mm_xor_si128(k1, _mm_slli_si128(k1, 4));
    return _mm_xor_si128(k1, kga);
}
static void key_expand(__m128i rk[15], const uint8_t key[32]) {
    rk[0] = _mm_loadu_si128((const __m128i *)key);
    rk[1] = _mm_loadu_si128((const __m128i *)(key + 16));
#define EXP(i, rc) \
    rk[i] = expand_step(rk[i-2], _mm_aeskeygenassist_si128(rk[i-1], rc)); \
    if (i + 1 < 15) rk[i+1] = expand_step2(rk[i-1], rk[i]);
    EXP(2, 0x01) EXP(4, 0x02) EXP(6, 0x04) EXP(8, 0x08)
    EXP(10, 0x10) EXP(12, 0x20) EXP(14, 0x40)
#undef EXP
}
static __m128i enc_block(const __m128i rk[15], __m128i x) {
    x = _mm_xor_si128(x, rk[0]);
    for (int i = 1; i < 14; i++) x = _mm_aesenc_si128(x, rk[i]);
    return _mm_aesenclast_si128(x, rk[14]);
}

void aes256ctr_init(aes256ctr_ctx *state, const uint8_t key[32], uint64_t nonce) {
    key_expand(state->rk, key);
    state->ctr = _mm_set_epi64x(0, (long long)nonce);
}
void aes256ctr_set_nonce(aes256ctr_ctx *state, uint64_t nonce) {
    state->ctr = _mm_set_epi64x(0, (long long)nonce);
}
void aes256ctr_squeezeblocks(uint8_t *out, size_t nblocks, aes256ctr_ctx *state) {
    for (size_t b = 0; b < nblocks; b++) {
        for (int i = 0; i < 4; i++) {  /* AES256CTR_BLOCKBYTES = 64 */
            __m128i o = enc_block(state->rk, state->ctr);
            _mm_storeu_si128((__m128i *)(out + 64 * b + 16 * i), o);
            state->ctr = _mm_add_epi64(state->ctr, _mm_set_epi64x(1, 0));
        }
    }
}
void aes256ctr_prf(uint8_t *out, size_t outlen, const uint8_t seed[32], uint64_t nonce) {
    aes256ctr_ctx st;
    uint8_t buf[64];
    aes256ctr_init(&st, seed, nonce);
    while (outlen) {
        aes256ctr_squeezeblocks(buf, 1, &st);
        size_t take = outlen > 64 ? 64 : outlen;
        memcpy(out, buf, take);
        out += take; outlen -= take;
    }
    aes256ctr_free(&st);
}
void aes256ctr_free(aes256ctr_ctx *state) { (void)state; }
