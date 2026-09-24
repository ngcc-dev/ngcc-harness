#ifndef TCCR_HASH_HPP
#define TCCR_HASH_HPP

#include "aes.hpp"
#include "aes_defs.hpp"
#include "block.hpp"
#include "constants.hpp"
#include "parameters.hpp"

#include <cstring>

#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#include <wmmintrin.h>
#endif

namespace sig
{

// ============================================================================
// TCCR Hash: Tweakable Circular Correlation Robust hash
// Ported from lynx-ref tccr.c.
//
// TCCRHash(x, s, iv) where x, s in {0,1}^csp, iv in {0,1}^120
//   x_bar = x XOR s
//   x_L = x_bar[0, block_bytes - 1), x_R = x_bar[block_bytes - 1, csp_bytes)
//   key = x_R || iv || 0*
//   block_i = i || x_L
//   h_i = Enc(key, block_i) XOR block_i
// ============================================================================

namespace tccr_detail
{

inline void build_tccr_nblock2_key(uint8_t* key, size_t key_bytes,
                                   const uint8_t* x_bar, size_t x_l_bytes,
                                   size_t x_r_bytes, const uint8_t* iv)
{
    memset(key, 0, key_bytes);
    memcpy(key, x_bar + x_l_bytes, x_r_bytes);
    memcpy(key + x_r_bytes, iv, 15);
}

inline void build_tccr_nblock2_plaintext(uint8_t* plaintext, const uint8_t* x_bar,
                                         size_t x_l_bytes, uint8_t block_index)
{
    plaintext[0] = block_index;
    memcpy(plaintext + 1, x_bar, x_l_bytes);
}

// ============================================================================
// SHACAL-2: 32-byte block, 64-byte key, 64 rounds
// SHA-256 compression function used as a block cipher (no feed-forward)
// ============================================================================

static const uint32_t K_SHA256[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
    0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
    0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
    0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
    0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
    0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
    0xc67178f2,
};

namespace
{
inline uint32_t rotr32(uint32_t x, unsigned int n) { return (x >> n) | (x << (32 - n)); }
inline uint32_t Ch(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); }
inline uint32_t Maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
inline uint32_t Sigma0(uint32_t x) { return rotr32(x, 2) ^ rotr32(x, 13) ^ rotr32(x, 22); }
inline uint32_t Sigma1(uint32_t x) { return rotr32(x, 6) ^ rotr32(x, 11) ^ rotr32(x, 25); }
inline uint32_t _sigma0(uint32_t x) { return rotr32(x, 7) ^ rotr32(x, 18) ^ (x >> 3); }
inline uint32_t _sigma1(uint32_t x) { return rotr32(x, 17) ^ rotr32(x, 19) ^ (x >> 10); }

inline uint32_t load32be(const uint8_t* p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}
inline void store32be(uint8_t* p, uint32_t x)
{
    p[0] = (uint8_t)(x >> 24); p[1] = (uint8_t)(x >> 16);
    p[2] = (uint8_t)(x >> 8);  p[3] = (uint8_t)x;
}
} // namespace

inline void shacal2_key_schedule(const uint8_t* key, uint32_t W[64])
{
    for (int i = 0; i < 16; ++i) W[i] = load32be(key + i * 4);

#if defined(__SHA__) && (defined(__x86_64__) || defined(__i386__))
    for (int i = 16; i < 64; i += 4)
    {
        __m128i W0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&W[i - 16]));
        __m128i W1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&W[i - 12]));
        __m128i T = _mm_sha256msg1_epu32(W0, W1);
        __m128i W7 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&W[i - 7]));
        T = _mm_add_epi32(T, W7);

        alignas(16) uint32_t tmp[4];
        _mm_store_si128(reinterpret_cast<__m128i*>(tmp), T);
        W[i] = _sigma1(W[i - 2]) + tmp[0];
        W[i + 1] = _sigma1(W[i - 1]) + tmp[1];
        W[i + 2] = _sigma1(W[i]) + tmp[2];
        W[i + 3] = _sigma1(W[i + 1]) + tmp[3];
    }
#else
    for (int i = 16; i < 64; ++i)
        W[i] = _sigma1(W[i - 2]) + W[i - 7] + _sigma0(W[i - 15]) + W[i - 16];
#endif
}

inline void shacal2_precompute_wk_pairs(const uint32_t W[64], uint32_t wk[16][4])
{
    for (int r = 0; r < 64; r += 4)
    {
        const int group = r / 4;
        wk[group][0] = W[r] + K_SHA256[r];
        wk[group][1] = W[r + 1] + K_SHA256[r + 1];
        wk[group][2] = W[r + 2] + K_SHA256[r + 2];
        wk[group][3] = W[r + 3] + K_SHA256[r + 3];
    }
}

inline void shacal2_key_schedule_wk(const uint8_t* key, uint32_t W[64], uint32_t wk[16][4])
{
    shacal2_key_schedule(key, W);
    shacal2_precompute_wk_pairs(W, wk);
}

inline void shacal2_key_schedule_wk_x4_interleaved(const uint8_t* const keys[4],
                                                   uint32_t* const W_out[4],
                                                   uint32_t* const wk_out[4], size_t active)
{
    if (active == 0)
        return;

#if defined(__SHA__) && (defined(__x86_64__) || defined(__i386__))
    alignas(16) uint32_t W_scratch[3][64];
    uint32_t* W[4];
    size_t scratch = 0;
    for (size_t s = 0; s < 4; ++s)
        W[s] = s < active ? W_out[s] : W_scratch[scratch++];

    for (size_t s = 0; s < 4; ++s)
    {
        const size_t src = s < active ? s : 0;
        for (int i = 0; i < 16; ++i)
            W[s][i] = load32be(keys[src] + i * 4);
    }

    for (int t = 16; t < 64; t += 4)
    {
        alignas(16) uint32_t tmp[4][4];
        for (size_t s = 0; s < 4; ++s)
        {
            __m128i W0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&W[s][t - 16]));
            __m128i W1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&W[s][t - 12]));
            __m128i T = _mm_sha256msg1_epu32(W0, W1);
            __m128i W7 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&W[s][t - 7]));
            T = _mm_add_epi32(T, W7);
            _mm_store_si128(reinterpret_cast<__m128i*>(tmp[s]), T);
        }
        for (size_t s = 0; s < 4; ++s)
        {
            W[s][t] = _sigma1(W[s][t - 2]) + tmp[s][0];
            W[s][t + 1] = _sigma1(W[s][t - 1]) + tmp[s][1];
            W[s][t + 2] = _sigma1(W[s][t]) + tmp[s][2];
            W[s][t + 3] = _sigma1(W[s][t + 1]) + tmp[s][3];
        }
    }

    for (size_t s = 0; s < active; ++s)
    {
        for (int r = 0; r < 64; r += 4)
        {
            const int group = r / 4;
            wk_out[s][4 * group] = W[s][r] + K_SHA256[r];
            wk_out[s][4 * group + 1] = W[s][r + 1] + K_SHA256[r + 1];
            wk_out[s][4 * group + 2] = W[s][r + 2] + K_SHA256[r + 2];
            wk_out[s][4 * group + 3] = W[s][r + 3] + K_SHA256[r + 3];
        }
    }
#else
    for (size_t s = 0; s < active; ++s)
    {
        shacal2_key_schedule(keys[s], W_out[s]);
        for (int r = 0; r < 64; r += 4)
        {
            const int group = r / 4;
            wk_out[s][4 * group] = W_out[s][r] + K_SHA256[r];
            wk_out[s][4 * group + 1] = W_out[s][r + 1] + K_SHA256[r + 1];
            wk_out[s][4 * group + 2] = W_out[s][r + 2] + K_SHA256[r + 2];
            wk_out[s][4 * group + 3] = W_out[s][r + 3] + K_SHA256[r + 3];
        }
    }
#endif
}

inline void shacal2_encrypt_scheduled_scalar(const uint32_t W[64], const uint8_t* plaintext,
                                             uint8_t* ciphertext)
{
    uint32_t a = load32be(plaintext + 0),  b = load32be(plaintext + 4);
    uint32_t c = load32be(plaintext + 8),  d = load32be(plaintext + 12);
    uint32_t e = load32be(plaintext + 16), f = load32be(plaintext + 20);
    uint32_t g = load32be(plaintext + 24), h = load32be(plaintext + 28);

    for (int i = 0; i < 64; ++i)
    {
        uint32_t T1 = h + Sigma1(e) + Ch(e, f, g) + K_SHA256[i] + W[i];
        uint32_t T2 = Sigma0(a) + Maj(a, b, c);
        h = g; g = f; f = e; e = d + T1;
        d = c; c = b; b = a; a = T1 + T2;
    }

    store32be(ciphertext + 0, a);  store32be(ciphertext + 4, b);
    store32be(ciphertext + 8, c);  store32be(ciphertext + 12, d);
    store32be(ciphertext + 16, e); store32be(ciphertext + 20, f);
    store32be(ciphertext + 24, g); store32be(ciphertext + 28, h);
}

#if defined(__SHA__) && (defined(__x86_64__) || defined(__i386__))
inline void shacal2_encrypt_wk_sha_ni(const uint32_t wk_pairs[16][4], const uint8_t* plaintext,
                                      uint8_t* ciphertext)
{
    __m128i state1 = _mm_set_epi32(static_cast<int>(load32be(plaintext + 0)),
                                   static_cast<int>(load32be(plaintext + 4)),
                                   static_cast<int>(load32be(plaintext + 16)),
                                   static_cast<int>(load32be(plaintext + 20)));
    __m128i state2 = _mm_set_epi32(static_cast<int>(load32be(plaintext + 8)),
                                   static_cast<int>(load32be(plaintext + 12)),
                                   static_cast<int>(load32be(plaintext + 24)),
                                   static_cast<int>(load32be(plaintext + 28)));

    for (int r = 0; r < 64; r += 4)
    {
        __m128i wk = _mm_load_si128(reinterpret_cast<const __m128i*>(wk_pairs[r / 4]));
        state2 = _mm_sha256rnds2_epu32(state2, state1, wk);
        wk = _mm_unpackhi_epi64(wk, wk);
        state1 = _mm_sha256rnds2_epu32(state1, state2, wk);
    }

    __m128i abcd = _mm_unpackhi_epi64(state2, state1);
    __m128i efgh = _mm_unpacklo_epi64(state2, state1);
    const __m128i byteswap = _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7,
                                          8, 9, 10, 11, 12, 13, 14, 15);
    abcd = _mm_shuffle_epi8(abcd, byteswap);
    efgh = _mm_shuffle_epi8(efgh, byteswap);

    _mm_storeu_si128(reinterpret_cast<__m128i*>(ciphertext), abcd);
    _mm_storeu_si128(reinterpret_cast<__m128i*>(ciphertext + 16), efgh);
}

inline void shacal2_encrypt_scheduled_sha_ni(const uint32_t W[64], const uint8_t* plaintext,
                                             uint8_t* ciphertext)
{
    __m128i state1 = _mm_set_epi32(static_cast<int>(load32be(plaintext + 0)),
                                   static_cast<int>(load32be(plaintext + 4)),
                                   static_cast<int>(load32be(plaintext + 16)),
                                   static_cast<int>(load32be(plaintext + 20)));
    __m128i state2 = _mm_set_epi32(static_cast<int>(load32be(plaintext + 8)),
                                   static_cast<int>(load32be(plaintext + 12)),
                                   static_cast<int>(load32be(plaintext + 24)),
                                   static_cast<int>(load32be(plaintext + 28)));

    for (int r = 0; r < 64; r += 4)
    {
        __m128i wk = _mm_set_epi32(static_cast<int>(W[r + 3] + K_SHA256[r + 3]),
                                   static_cast<int>(W[r + 2] + K_SHA256[r + 2]),
                                   static_cast<int>(W[r + 1] + K_SHA256[r + 1]),
                                   static_cast<int>(W[r] + K_SHA256[r]));
        state2 = _mm_sha256rnds2_epu32(state2, state1, wk);
        wk = _mm_unpackhi_epi64(wk, wk);
        state1 = _mm_sha256rnds2_epu32(state1, state2, wk);
    }

    __m128i abcd = _mm_unpackhi_epi64(state2, state1);
    __m128i efgh = _mm_unpacklo_epi64(state2, state1);
    const __m128i byteswap = _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7,
                                          8, 9, 10, 11, 12, 13, 14, 15);
    abcd = _mm_shuffle_epi8(abcd, byteswap);
    efgh = _mm_shuffle_epi8(efgh, byteswap);

    _mm_storeu_si128(reinterpret_cast<__m128i*>(ciphertext), abcd);
    _mm_storeu_si128(reinterpret_cast<__m128i*>(ciphertext + 16), efgh);
}
#endif

inline void shacal2_encrypt_scheduled(const uint32_t W[64], const uint8_t* plaintext,
                                      uint8_t* ciphertext)
{
#if defined(__SHA__) && (defined(__x86_64__) || defined(__i386__))
    shacal2_encrypt_scheduled_sha_ni(W, plaintext, ciphertext);
#else
    shacal2_encrypt_scheduled_scalar(W, plaintext, ciphertext);
#endif
}

inline void shacal2_encrypt_wk(const uint32_t wk_pairs[16][4], const uint32_t W[64],
                               const uint8_t* plaintext, uint8_t* ciphertext)
{
#if defined(__SHA__) && (defined(__x86_64__) || defined(__i386__))
    (void)W;
    shacal2_encrypt_wk_sha_ni(wk_pairs, plaintext, ciphertext);
#else
    shacal2_encrypt_scheduled_scalar(W, plaintext, ciphertext);
#endif
}

inline void shacal2_encrypt_scheduled_x4(const uint32_t* const W[4],
                                         const uint8_t* const plaintext[4],
                                         uint8_t* const ciphertext[4], size_t active)
{
#if defined(__SHA__) && (defined(__x86_64__) || defined(__i386__))
    __m128i state1[4];
    __m128i state2[4];
    for (size_t g = 0; g < 4; ++g)
    {
        const size_t src = g < active ? g : 0;
        state1[g] = _mm_set_epi32(static_cast<int>(load32be(plaintext[src] + 0)),
                                  static_cast<int>(load32be(plaintext[src] + 4)),
                                  static_cast<int>(load32be(plaintext[src] + 16)),
                                  static_cast<int>(load32be(plaintext[src] + 20)));
        state2[g] = _mm_set_epi32(static_cast<int>(load32be(plaintext[src] + 8)),
                                  static_cast<int>(load32be(plaintext[src] + 12)),
                                  static_cast<int>(load32be(plaintext[src] + 24)),
                                  static_cast<int>(load32be(plaintext[src] + 28)));
    }

    for (int r = 0; r < 64; r += 4)
    {
        __m128i wk[4];
        for (size_t g = 0; g < 4; ++g)
        {
            const size_t src = g < active ? g : 0;
            wk[g] = _mm_set_epi32(static_cast<int>(W[src][r + 3] + K_SHA256[r + 3]),
                                  static_cast<int>(W[src][r + 2] + K_SHA256[r + 2]),
                                  static_cast<int>(W[src][r + 1] + K_SHA256[r + 1]),
                                  static_cast<int>(W[src][r] + K_SHA256[r]));
        }
        for (size_t g = 0; g < 4; ++g)
            state2[g] = _mm_sha256rnds2_epu32(state2[g], state1[g], wk[g]);
        for (size_t g = 0; g < 4; ++g)
        {
            wk[g] = _mm_unpackhi_epi64(wk[g], wk[g]);
            state1[g] = _mm_sha256rnds2_epu32(state1[g], state2[g], wk[g]);
        }
    }

    const __m128i byteswap = _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7,
                                          8, 9, 10, 11, 12, 13, 14, 15);
    for (size_t g = 0; g < active; ++g)
    {
        __m128i abcd = _mm_unpackhi_epi64(state2[g], state1[g]);
        __m128i efgh = _mm_unpacklo_epi64(state2[g], state1[g]);
        abcd = _mm_shuffle_epi8(abcd, byteswap);
        efgh = _mm_shuffle_epi8(efgh, byteswap);

        _mm_storeu_si128(reinterpret_cast<__m128i*>(ciphertext[g]), abcd);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(ciphertext[g] + 16), efgh);
    }
#else
    for (size_t g = 0; g < active; ++g)
        shacal2_encrypt_scheduled_scalar(W[g], plaintext[g], ciphertext[g]);
#endif
}

inline void shacal2_encrypt_wk_x4(const uint32_t* const wk_pairs[4],
                                  const uint8_t* const plaintext[4],
                                  uint8_t* const ciphertext[4], size_t active)
{
#if defined(__SHA__) && (defined(__x86_64__) || defined(__i386__))
    __m128i state1[4];
    __m128i state2[4];
    for (size_t g = 0; g < 4; ++g)
    {
        const size_t src = g < active ? g : 0;
        state1[g] = _mm_set_epi32(static_cast<int>(load32be(plaintext[src] + 0)),
                                  static_cast<int>(load32be(plaintext[src] + 4)),
                                  static_cast<int>(load32be(plaintext[src] + 16)),
                                  static_cast<int>(load32be(plaintext[src] + 20)));
        state2[g] = _mm_set_epi32(static_cast<int>(load32be(plaintext[src] + 8)),
                                  static_cast<int>(load32be(plaintext[src] + 12)),
                                  static_cast<int>(load32be(plaintext[src] + 24)),
                                  static_cast<int>(load32be(plaintext[src] + 28)));
    }

    for (int r = 0; r < 64; r += 4)
    {
        __m128i wk[4];
        for (size_t g = 0; g < 4; ++g)
        {
            const size_t src = g < active ? g : 0;
            wk[g] = _mm_load_si128(
                reinterpret_cast<const __m128i*>(wk_pairs[src] + 4 * (r / 4)));
        }
        for (size_t g = 0; g < 4; ++g)
            state2[g] = _mm_sha256rnds2_epu32(state2[g], state1[g], wk[g]);
        for (size_t g = 0; g < 4; ++g)
        {
            wk[g] = _mm_unpackhi_epi64(wk[g], wk[g]);
            state1[g] = _mm_sha256rnds2_epu32(state1[g], state2[g], wk[g]);
        }
    }

    const __m128i byteswap = _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7,
                                          8, 9, 10, 11, 12, 13, 14, 15);
    for (size_t g = 0; g < active; ++g)
    {
        __m128i abcd = _mm_unpackhi_epi64(state2[g], state1[g]);
        __m128i efgh = _mm_unpacklo_epi64(state2[g], state1[g]);
        abcd = _mm_shuffle_epi8(abcd, byteswap);
        efgh = _mm_shuffle_epi8(efgh, byteswap);

        _mm_storeu_si128(reinterpret_cast<__m128i*>(ciphertext[g]), abcd);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(ciphertext[g] + 16), efgh);
    }
#else
    (void)wk_pairs;
    (void)plaintext;
    (void)ciphertext;
    (void)active;
#endif
}

inline void shacal2_encrypt(const uint8_t* key, const uint8_t* plaintext, uint8_t* ciphertext)
{
    uint32_t W[64];
    shacal2_key_schedule(key, W);
    shacal2_encrypt_scheduled(W, plaintext, ciphertext);
}

} // namespace tccr_detail

// ============================================================================
// Public TCCR hash interface — CSP-specific template
// ============================================================================

// CSP = security parameter bits (not secpar enum, to support 160-bit)
// TCCR hash for 256-bit CSP (AES-256, 16-byte block)
inline void tccr_hash_256(const uint8_t* x, const uint8_t* s, const uint8_t* iv, uint8_t* out)
{
    constexpr size_t csp_bytes = 32;
    constexpr size_t blocklen = 16;
    constexpr size_t x_l_bytes = blocklen - 1;
    constexpr size_t x_r_bytes = csp_bytes - x_l_bytes;

    uint8_t x_bar[32];
    for (size_t i = 0; i < csp_bytes; ++i) x_bar[i] = x[i] ^ s[i];

    uint8_t key[32] = {0};
    tccr_detail::build_tccr_nblock2_key(key, sizeof(key), x_bar, x_l_bytes, x_r_bytes, iv);
    block256 key_block;
    block128 plaintexts[2];
    aes_round_keys<secpar::s256> round_keys;
    uint8_t plain_bytes[2][16] = {};

    tccr_detail::build_tccr_nblock2_plaintext(plain_bytes[0], x_bar, x_l_bytes, 0);
    tccr_detail::build_tccr_nblock2_plaintext(plain_bytes[1], x_bar, x_l_bytes, 1);

    std::memcpy(&key_block, key, sizeof(key));
    std::memcpy(plaintexts, plain_bytes, sizeof(plaintexts));
    aes_keygen_ecb<secpar::s256, 1, 2>(&key_block, &round_keys, plaintexts);
    for (size_t i = 0; i < blocklen; ++i)
    {
        out[i] = reinterpret_cast<uint8_t*>(&plaintexts[0])[i] ^ plain_bytes[0][i];
        out[blocklen + i] = reinterpret_cast<uint8_t*>(&plaintexts[1])[i] ^ plain_bytes[1][i];
    }
}

inline void tccr_hash_x0_x1_256(const uint8_t* x, const uint8_t* s, const uint8_t* iv,
                                uint8_t* out0, uint8_t* out1)
{
    constexpr size_t csp_bytes = 32;
    constexpr size_t blocklen = 16;
    constexpr size_t x_l_bytes = blocklen - 1;
    constexpr size_t x_r_bytes = csp_bytes - x_l_bytes;

    uint8_t x_bar[32];
    for (size_t i = 0; i < csp_bytes; ++i)
        x_bar[i] = x[i] ^ s[i];

    uint8_t key[32] = {0};
    tccr_detail::build_tccr_nblock2_key(key, sizeof(key), x_bar, x_l_bytes, x_r_bytes, iv);

    block256 key_block;
    block128 plaintexts[4];
    aes_round_keys<secpar::s256> round_keys;
    uint8_t plain_bytes[4][16] = {};

    tccr_detail::build_tccr_nblock2_plaintext(plain_bytes[0], x_bar, x_l_bytes, 0);
    tccr_detail::build_tccr_nblock2_plaintext(plain_bytes[2], x_bar, x_l_bytes, 1);
    x_bar[0] ^= 1;
    tccr_detail::build_tccr_nblock2_plaintext(plain_bytes[1], x_bar, x_l_bytes, 0);
    tccr_detail::build_tccr_nblock2_plaintext(plain_bytes[3], x_bar, x_l_bytes, 1);
    x_bar[0] ^= 1;

    std::memcpy(&key_block, key, 32);
    std::memcpy(plaintexts, plain_bytes, sizeof(plaintexts));
    aes_keygen_ecb<secpar::s256, 1, 4>(&key_block, &round_keys, plaintexts);
    for (size_t i = 0; i < blocklen; ++i)
    {
        out0[i] = reinterpret_cast<uint8_t*>(&plaintexts[0])[i] ^ plain_bytes[0][i];
        out1[i] = reinterpret_cast<uint8_t*>(&plaintexts[1])[i] ^ plain_bytes[1][i];
        out0[blocklen + i] = reinterpret_cast<uint8_t*>(&plaintexts[2])[i] ^ plain_bytes[2][i];
        out1[blocklen + i] = reinterpret_cast<uint8_t*>(&plaintexts[3])[i] ^ plain_bytes[3][i];
    }
}

// TCCR hash for 384-bit CSP (Rijndael-256, 32-byte block)
inline void tccr_hash_384(const uint8_t* x, const uint8_t* s, const uint8_t* iv, uint8_t* out)
{
    constexpr size_t csp_bytes = 48;
    constexpr size_t blocklen = 32;
    constexpr size_t x_l_bytes = blocklen - 1;
    constexpr size_t x_r_bytes = csp_bytes - x_l_bytes;

    uint8_t x_bar[48];
    for (size_t i = 0; i < csp_bytes; ++i) x_bar[i] = x[i] ^ s[i];

    uint8_t key[32] = {0};
    tccr_detail::build_tccr_nblock2_key(key, sizeof(key), x_bar, x_l_bytes, x_r_bytes, iv);
    block256 key_block;
    block256 plaintexts[2];
    rijndael_round_keys<secpar::s256> round_keys;
    uint8_t plain_bytes[2][32] = {};

    tccr_detail::build_tccr_nblock2_plaintext(plain_bytes[0], x_bar, x_l_bytes, 0);
    tccr_detail::build_tccr_nblock2_plaintext(plain_bytes[1], x_bar, x_l_bytes, 1);

    std::memcpy(&key_block, key, sizeof(key));
    std::memcpy(plaintexts, plain_bytes, sizeof(plaintexts));
    rijndael_keygen<secpar::s256>(&round_keys, key_block);
    for (size_t round = 0; round <= RIJNDAEL_ROUNDS<secpar::s256>; ++round)
        rijndael256_round(&round_keys, plaintexts, 1, 2, round);
    for (size_t i = 0; i < blocklen; ++i)
        out[i] = reinterpret_cast<uint8_t*>(&plaintexts[0])[i] ^ plain_bytes[0][i];
    for (size_t i = 0; i < csp_bytes - blocklen; ++i)
        out[blocklen + i] = reinterpret_cast<uint8_t*>(&plaintexts[1])[i] ^ plain_bytes[1][i];
}

inline void tccr_hash_x0_x1_384(const uint8_t* x, const uint8_t* s, const uint8_t* iv,
                                uint8_t* out0, uint8_t* out1)
{
    constexpr size_t csp_bytes = 48;
    constexpr size_t blocklen = 32;
    constexpr size_t x_l_bytes = blocklen - 1;
    constexpr size_t x_r_bytes = csp_bytes - x_l_bytes;

    uint8_t x_bar[48];
    for (size_t i = 0; i < csp_bytes; ++i)
        x_bar[i] = x[i] ^ s[i];

    uint8_t key[32] = {0};
    tccr_detail::build_tccr_nblock2_key(key, sizeof(key), x_bar, x_l_bytes, x_r_bytes, iv);

    block256 key_block;
    block256 plaintexts[4];
    rijndael_round_keys<secpar::s256> round_keys;
    uint8_t plain_bytes[4][32] = {};

    tccr_detail::build_tccr_nblock2_plaintext(plain_bytes[0], x_bar, x_l_bytes, 0);
    tccr_detail::build_tccr_nblock2_plaintext(plain_bytes[2], x_bar, x_l_bytes, 1);
    x_bar[0] ^= 1;
    tccr_detail::build_tccr_nblock2_plaintext(plain_bytes[1], x_bar, x_l_bytes, 0);
    tccr_detail::build_tccr_nblock2_plaintext(plain_bytes[3], x_bar, x_l_bytes, 1);
    x_bar[0] ^= 1;

    std::memcpy(&key_block, key, sizeof(key));
    std::memcpy(plaintexts, plain_bytes, sizeof(plaintexts));
    rijndael_keygen<secpar::s256>(&round_keys, key_block);
    for (size_t round = 0; round <= RIJNDAEL_ROUNDS<secpar::s256>; ++round)
        rijndael256_round(&round_keys, plaintexts, 1, 4, round);

    for (size_t i = 0; i < blocklen; ++i)
    {
        out0[i] = reinterpret_cast<uint8_t*>(&plaintexts[0])[i] ^ plain_bytes[0][i];
        out1[i] = reinterpret_cast<uint8_t*>(&plaintexts[1])[i] ^ plain_bytes[1][i];
    }
    for (size_t i = 0; i < csp_bytes - blocklen; ++i)
    {
        out0[blocklen + i] = reinterpret_cast<uint8_t*>(&plaintexts[2])[i] ^ plain_bytes[2][i];
        out1[blocklen + i] = reinterpret_cast<uint8_t*>(&plaintexts[3])[i] ^ plain_bytes[3][i];
    }
}

// TCCR hash for 512-bit CSP (SHACAL-2, 32-byte block, generic multi-block)
inline void tccr_hash_512(const uint8_t* x, const uint8_t* s, const uint8_t* iv, uint8_t* out)
{
    constexpr size_t csp_bytes = 64;
    constexpr size_t blocklen = 32;
    constexpr size_t keylen = 64;
    constexpr size_t x_l_bytes = blocklen - 1;
    constexpr size_t x_r_len = csp_bytes - x_l_bytes;
    constexpr size_t N_block = 2;

    uint8_t x_bar[64];
    for (size_t i = 0; i < csp_bytes; ++i) x_bar[i] = x[i] ^ s[i];

    uint8_t key[64] = {};
    tccr_detail::build_tccr_nblock2_key(key, keylen, x_bar, x_l_bytes, x_r_len, iv);
    uint8_t plaintext[32] = {};
    uint8_t ciphertext[32];
    uint32_t W[64];
    tccr_detail::shacal2_key_schedule(key, W);

    for (size_t i = 0; i < N_block; ++i)
    {
        tccr_detail::build_tccr_nblock2_plaintext(plaintext, x_bar, x_l_bytes,
                                                  static_cast<uint8_t>(i));
        tccr_detail::shacal2_encrypt_scheduled(W, plaintext, ciphertext);

        size_t blk = (i == N_block - 1) ? (csp_bytes - i * blocklen) : blocklen;
        for (size_t j = 0; j < blk; ++j)
            out[i * blocklen + j] = ciphertext[j] ^ plaintext[j];
    }
}

inline void tccr_hash_x0_x1_512(const uint8_t* x, const uint8_t* s, const uint8_t* iv,
                                uint8_t* out0, uint8_t* out1)
{
    constexpr size_t csp_bytes = 64;
    constexpr size_t blocklen = 32;
    constexpr size_t keylen = 64;
    constexpr size_t x_l_bytes = blocklen - 1;
    constexpr size_t x_r_len = csp_bytes - x_l_bytes;

    uint8_t x_bar[64];
    for (size_t i = 0; i < csp_bytes; ++i)
        x_bar[i] = x[i] ^ s[i];

    uint8_t key[64] = {};
    tccr_detail::build_tccr_nblock2_key(key, keylen, x_bar, x_l_bytes, x_r_len, iv);

    uint8_t plain_bytes[4][32] = {};
    uint8_t ciphertext[32];
    uint32_t W[64];

    tccr_detail::build_tccr_nblock2_plaintext(plain_bytes[0], x_bar, x_l_bytes, 0);
    tccr_detail::build_tccr_nblock2_plaintext(plain_bytes[2], x_bar, x_l_bytes, 1);
    x_bar[0] ^= 1;
    tccr_detail::build_tccr_nblock2_plaintext(plain_bytes[1], x_bar, x_l_bytes, 0);
    tccr_detail::build_tccr_nblock2_plaintext(plain_bytes[3], x_bar, x_l_bytes, 1);
    x_bar[0] ^= 1;

    tccr_detail::shacal2_key_schedule(key, W);
    tccr_detail::shacal2_encrypt_scheduled(W, plain_bytes[0], ciphertext);
    for (size_t i = 0; i < blocklen; ++i)
        out0[i] = ciphertext[i] ^ plain_bytes[0][i];
    tccr_detail::shacal2_encrypt_scheduled(W, plain_bytes[1], ciphertext);
    for (size_t i = 0; i < blocklen; ++i)
        out1[i] = ciphertext[i] ^ plain_bytes[1][i];
    tccr_detail::shacal2_encrypt_scheduled(W, plain_bytes[2], ciphertext);
    for (size_t i = 0; i < csp_bytes - blocklen; ++i)
        out0[blocklen + i] = ciphertext[i] ^ plain_bytes[2][i];
    tccr_detail::shacal2_encrypt_scheduled(W, plain_bytes[3], ciphertext);
    for (size_t i = 0; i < csp_bytes - blocklen; ++i)
        out1[blocklen + i] = ciphertext[i] ^ plain_bytes[3][i];
}

// ============================================================================
// TCCR hash for 160-bit CSP (AES-192, 16-byte block)
// ============================================================================

inline void tccr_hash_160(const uint8_t* x, const uint8_t* s, const uint8_t* iv, uint8_t* out)
{
    constexpr size_t csp_bytes = 20;
    constexpr size_t blocklen = 16;
    constexpr size_t x_l_bytes = blocklen - 1;
    constexpr size_t x_r_bytes = csp_bytes - x_l_bytes;

    uint8_t x_bar[20];
    for (size_t i = 0; i < csp_bytes; ++i) x_bar[i] = x[i] ^ s[i];

    uint8_t key[24] = {0};
    tccr_detail::build_tccr_nblock2_key(key, sizeof(key), x_bar, x_l_bytes, x_r_bytes, iv);
    block192 key_block;
    block128 plaintexts[2];
    aes_round_keys<secpar::s192> round_keys;
    uint8_t plain_bytes[2][16] = {};

    tccr_detail::build_tccr_nblock2_plaintext(plain_bytes[0], x_bar, x_l_bytes, 0);
    tccr_detail::build_tccr_nblock2_plaintext(plain_bytes[1], x_bar, x_l_bytes, 1);

    std::memcpy(&key_block, key, sizeof(key));
    std::memcpy(plaintexts, plain_bytes, sizeof(plaintexts));
    aes_keygen_ecb<secpar::s192, 1, 2>(&key_block, &round_keys, plaintexts);
    for (size_t i = 0; i < blocklen; ++i)
        out[i] = reinterpret_cast<uint8_t*>(&plaintexts[0])[i] ^ plain_bytes[0][i];
    for (size_t i = 0; i < csp_bytes - blocklen; ++i)
        out[blocklen + i] = reinterpret_cast<uint8_t*>(&plaintexts[1])[i] ^ plain_bytes[1][i];
}

inline void tccr_hash_x0_x1_160(const uint8_t* x, const uint8_t* s, const uint8_t* iv,
                                uint8_t* out0, uint8_t* out1)
{
    constexpr size_t csp_bytes = 20;
    constexpr size_t blocklen = 16;
    constexpr size_t x_l_bytes = blocklen - 1;
    constexpr size_t x_r_bytes = csp_bytes - x_l_bytes;

    uint8_t x_bar[20];
    for (size_t i = 0; i < csp_bytes; ++i)
        x_bar[i] = x[i] ^ s[i];

    uint8_t key[24] = {0};
    tccr_detail::build_tccr_nblock2_key(key, sizeof(key), x_bar, x_l_bytes, x_r_bytes, iv);

    block192 key_block;
    block128 plaintexts[4];
    aes_round_keys<secpar::s192> round_keys;
    uint8_t plain_bytes[4][16] = {};

    tccr_detail::build_tccr_nblock2_plaintext(plain_bytes[0], x_bar, x_l_bytes, 0);
    tccr_detail::build_tccr_nblock2_plaintext(plain_bytes[2], x_bar, x_l_bytes, 1);
    x_bar[0] ^= 1;
    tccr_detail::build_tccr_nblock2_plaintext(plain_bytes[1], x_bar, x_l_bytes, 0);
    tccr_detail::build_tccr_nblock2_plaintext(plain_bytes[3], x_bar, x_l_bytes, 1);
    x_bar[0] ^= 1;

    std::memcpy(&key_block, key, 24);
    std::memcpy(plaintexts, plain_bytes, sizeof(plaintexts));
    aes_keygen_ecb<secpar::s192, 1, 4>(&key_block, &round_keys, plaintexts);
    for (size_t i = 0; i < csp_bytes - blocklen; ++i)
    {
        out0[blocklen + i] = reinterpret_cast<uint8_t*>(&plaintexts[2])[i] ^ plain_bytes[2][i];
        out1[blocklen + i] = reinterpret_cast<uint8_t*>(&plaintexts[3])[i] ^ plain_bytes[3][i];
    }
    for (size_t i = 0; i < blocklen; ++i)
    {
        out0[i] = reinterpret_cast<uint8_t*>(&plaintexts[0])[i] ^ plain_bytes[0][i];
        out1[i] = reinterpret_cast<uint8_t*>(&plaintexts[1])[i] ^ plain_bytes[1][i];
    }
}

// ============================================================================
// Dispatch wrapper by CSP bits (used when CSP is not known at compile time)
// ============================================================================

inline void tccr_hash_dispatch(const uint8_t* x, const uint8_t* s, const uint8_t* iv,
                               uint8_t* out, unsigned int csp_bits)
{
    switch (csp_bits)
    {
    case 160: tccr_hash_160(x, s, iv, out); break;
    case 256: tccr_hash_256(x, s, iv, out); break;
    case 384: tccr_hash_384(x, s, iv, out); break;
    case 512: tccr_hash_512(x, s, iv, out); break;
    default: break;
    }
}

inline void tccr_hash_x0_x1_dispatch(const uint8_t* x, const uint8_t* s, const uint8_t* iv,
                                     uint8_t* out0, uint8_t* out1, unsigned int csp_bits)
{
    switch (csp_bits)
    {
    case 160: tccr_hash_x0_x1_160(x, s, iv, out0, out1); break;
    case 256: tccr_hash_x0_x1_256(x, s, iv, out0, out1); break;
    case 384: tccr_hash_x0_x1_384(x, s, iv, out0, out1); break;
    case 512: tccr_hash_x0_x1_512(x, s, iv, out0, out1); break;
    default: break;
    }
}

} // namespace sig

#endif
