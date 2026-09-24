#ifndef SYMMETRIC_H
#define SYMMETRIC_H

#include <stddef.h>
#include <stdint.h>
#include "params.h"
// #include "../sig/params.h"

// default: ICCS symmetric - use SM3.
#ifndef LOOM_USE_SHAKE
/*
 * ICCS symmetric layer.
 * Hash/XOF via auxfunc (sm3hash, pseudohash, pseudoXOF).
 */
#define XOF_BLOCKBYTES 168
#define SHAKE256_RATE 136

#define LOOM_HMAC_BLOCKBYTES 64

typedef struct {
  uint8_t msg[WEAVER_SYMBYTES + 2];
  size_t msg_len;
  size_t pos;
} xof_state;

#define weaver_iccs_xof_absorb_wrap LOOM_NAMESPACE(weaver_iccs_xof_absorb_wrap)
void weaver_iccs_xof_absorb_wrap(xof_state *state,
                                 const uint8_t seed[WEAVER_SYMBYTES],
                                 uint8_t x,
                                 uint8_t y);

#define weaver_iccs_xof_squeezeblocks_wrap LOOM_NAMESPACE(weaver_iccs_xof_squeezeblocks_wrap)
void weaver_iccs_xof_squeezeblocks_wrap(uint8_t *out, size_t nblocks, xof_state *state);

#define weaver_iccs_expand_keypair_seeds LOOM_NAMESPACE(weaver_iccs_expand_keypair_seeds)
void weaver_iccs_expand_keypair_seeds(uint8_t *out, const uint8_t *in, size_t inlen);

#define weaver_iccs_hash_kr LOOM_NAMESPACE(weaver_iccs_hash_kr)
void weaver_iccs_hash_kr(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen);

#define weaver_iccs_prf LOOM_NAMESPACE(weaver_iccs_prf)
void weaver_iccs_prf(uint8_t *out, size_t outlen, const uint8_t key[WEAVER_SYMBYTES], uint8_t nonce);

#define expand_keypair_seeds(OUT, IN, INBYTES) weaver_iccs_expand_keypair_seeds(OUT, IN, INBYTES)
#define xof_absorb(STATE, SEED, X, Y) weaver_iccs_xof_absorb_wrap(STATE, SEED, X, Y)
#define xof_squeezeblocks(OUT, OUTBLOCKS, STATE) weaver_iccs_xof_squeezeblocks_wrap(OUT, OUTBLOCKS, STATE)
#define prf(OUT, OUTBYTES, KEY, NONCE) weaver_iccs_prf(OUT, OUTBYTES, KEY, NONCE)

#define shake256(OUT, OUTLEN, IN, INLEN) weaver_iccs_hash_kr((OUT), (OUTLEN), (IN), (INLEN))

#include "../sig/test/prof.h"
#    include "drng.h"
typedef DRNG_ctx xof_ctx;
/* OUTLEN: one SM3 compression emits 32 bytes; a request of L bytes costs
 * ceil(L/4) SM3 calls.  Both rates are 32 because the two primitives
 * collapse onto the same SM3 DRBG under NGCC_MODE. */
#    define XOF128_RATE 32
#    define XOF256_RATE 32

/* ---- unified XOF wrappers (symmetric.{c,h}, xof.h) ---- */
#    define xof128_init LOOM_NAMESPACE(xof128_init)
#    define xof128_squeeze LOOM_NAMESPACE(xof128_squeeze)
#    define xof256_init LOOM_NAMESPACE(xof256_init)
#    define xof256_squeeze LOOM_NAMESPACE(xof256_squeeze)

/* ---- the four scalar primitives (bodies in symmetric.h) ---- */
void xof128_init(xof_ctx *ctx, const uint8_t *seed, size_t seed_len);
void xof128_squeeze(xof_ctx *ctx, uint8_t *out, size_t out_len);
void xof256_init(xof_ctx *ctx, const uint8_t *seed, size_t seed_len);
void xof256_squeeze(xof_ctx *ctx, uint8_t *out, size_t out_len);

#    define XOF_LANES_AVX2 8
#    define XOF_LANES_AVX512 16

#define XOF_STREAMS 16
#define XOF_NUM_PASSES(lanes) ((XOF_STREAMS + (lanes)-1) / (lanes))

#    define XOF_SQUEEZE_GRANULARITY_BYTES (32 * 4) /* 128 */

#else /* USE_SHAKE begin: */

#include "fips202.h"

#define LOOM_HMAC_BLOCKBYTES SHAKE256_RATE

typedef keccak_state xof_state;

#define weaver_seedexpbytes (2 * WEAVER_SYMBYTES)

typedef struct {
  uint8_t bytes[weaver_seedexpbytes];
} weaver_seedexpbuf;

#define weaver_expand_keypair_seeds LOOM_NAMESPACE(weaver_expand_keypair_seeds)
void weaver_expand_keypair_seeds(uint8_t out[weaver_seedexpbytes], const uint8_t *in, size_t inlen);

#define weaver_shake128_absorb LOOM_NAMESPACE(weaver_shake128_absorb)
void weaver_shake128_absorb(keccak_state *s,
                           const uint8_t seed[WEAVER_SYMBYTES],
                           uint8_t x,
                           uint8_t y);

#define weaver_shake256_prf LOOM_NAMESPACE(weaver_shake256_prf)
void weaver_shake256_prf(uint8_t *out, size_t outlen, const uint8_t key[WEAVER_SYMBYTES], uint8_t nonce);

#define XOF_BLOCKBYTES SHAKE128_RATE
#define expand_keypair_seeds(OUT, IN, INBYTES) weaver_expand_keypair_seeds((OUT), (IN), (INBYTES))
#define xof_absorb(STATE, SEED, X, Y) weaver_shake128_absorb(STATE, SEED, X, Y)
#define xof_squeezeblocks(OUT, OUTBLOCKS, STATE) shake128_squeezeblocks(OUT, OUTBLOCKS, STATE)
#define prf(OUT, OUTBYTES, KEY, NONCE) weaver_shake256_prf(OUT, OUTBYTES, KEY, NONCE)

#include "../sig/test/prof.h"

typedef keccak_state xof_ctx;
#    define XOF128_RATE SHAKE128_RATE /* 168 */
#    define XOF256_RATE SHAKE256_RATE /* 136 */

/* ---- unified XOF wrappers (symmetric.{c,h}, xof.h) ---- */
#    define xof128_init LOOM_NAMESPACE(xof128_init)
#    define xof128_squeeze LOOM_NAMESPACE(xof128_squeeze)
#    define xof256_init LOOM_NAMESPACE(xof256_init)
#    define xof256_squeeze LOOM_NAMESPACE(xof256_squeeze)

/* ---- the four scalar primitives (bodies in symmetric.h) ---- */
void xof128_init(xof_ctx *ctx, const uint8_t *seed, size_t seed_len);
void xof128_squeeze(xof_ctx *ctx, uint8_t *out, size_t out_len);
void xof256_init(xof_ctx *ctx, const uint8_t *seed, size_t seed_len);
void xof256_squeeze(xof_ctx *ctx, uint8_t *out, size_t out_len);

#    define XOF_LANES_AVX2 4
#    define XOF_LANES_AVX512 8

#define XOF_STREAMS 16
#define XOF_NUM_PASSES(lanes) ((XOF_STREAMS + (lanes)-1) / (lanes))

#    define XOF_SQUEEZE_GRANULARITY_BYTES SHAKE128_RATE /* 168 */

#    if defined(LOOM_AVX2) && !defined(WEAVER_USE_KECCAK4X)
#        define SHUTTLE_XOF_DECLARE_AVX2 1
#        include "../sig/fips202x4.h"
typedef keccakx4_state xof_ctx_avx2;
void xof256_avx2_init(xof_ctx_avx2 *ctx,
                      const uint8_t *const seed[XOF_LANES_AVX2],
                      size_t seed_len);
void xof256_avx2_squeeze(xof_ctx_avx2 *ctx,
                         uint8_t *const out[XOF_LANES_AVX2],
                         size_t out_len);
void xof128_avx2_init(xof_ctx_avx2 *ctx,
                      const uint8_t *const seed[XOF_LANES_AVX2],
                      size_t seed_len);
void xof128_avx2_squeeze(xof_ctx_avx2 *ctx,
                         uint8_t *const out[XOF_LANES_AVX2],
                         size_t out_len);
#    endif

#endif /* USE_SHAKE */ 

#endif /* SYMMETRIC_H */
