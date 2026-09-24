#include <stdint.h>
#include <string.h>
#include "params.h"
#include "packing.h"
#include "poly.h"
#include "reduce.h"

/* ---- generic little-endian bit writer/reader ---- */
typedef struct { uint8_t *p; uint64_t acc; unsigned nbits; } bitw;
static void bw_init(bitw *w, uint8_t *p) { w->p = p; w->acc = 0; w->nbits = 0; }
static void bw_put(bitw *w, uint32_t v, unsigned bits) {
    w->acc |= (uint64_t)(v & ((1u << bits) - 1)) << w->nbits;
    w->nbits += bits;
    while (w->nbits >= 8) { *w->p++ = (uint8_t)w->acc; w->acc >>= 8; w->nbits -= 8; }
}
static void bw_flush(bitw *w) { if (w->nbits) { *w->p++ = (uint8_t)w->acc; w->acc = 0; w->nbits = 0; } }

typedef struct { const uint8_t *p; uint64_t acc; unsigned nbits; } bitr;
static void br_init(bitr *r, const uint8_t *p) { r->p = p; r->acc = 0; r->nbits = 0; }
static uint32_t br_get(bitr *r, unsigned bits) {
    while (r->nbits < bits) { r->acc |= (uint64_t)(*r->p++) << r->nbits; r->nbits += 8; }
    uint32_t v = (uint32_t)(r->acc & ((1u << bits) - 1));
    r->acc >>= bits; r->nbits -= bits;
    return v;
}

/* ---- pk ---- */
void pack_pk(uint8_t pk[CRYPTO_PUBLICKEYBYTES], const uint8_t seedA[SEEDBYTES],
             const poly b[K]) {
    memcpy(pk, seedA, SEEDBYTES);
    bitw w; bw_init(&w, pk + SEEDBYTES);
    for (int i = 0; i < K; i++)
        for (int j = 0; j < N; j++)
            bw_put(&w, (uint32_t)freeze(b[i].coeffs[j]), POLYQ_BITS);
    bw_flush(&w);
}

void unpack_pk(uint8_t seedA[SEEDBYTES], poly b[K],
               const uint8_t pk[CRYPTO_PUBLICKEYBYTES]) {
    memcpy(seedA, pk, SEEDBYTES);
    bitr r; br_init(&r, pk + SEEDBYTES);
    for (int i = 0; i < K; i++)
        for (int j = 0; j < N; j++)
            b[i].coeffs[j] = (int32_t)br_get(&r, POLYQ_BITS);
}

/* ---- sk ---- */
void pack_sk(uint8_t sk[CRYPTO_SECRETKEYBYTES],
             const uint8_t pk[CRYPTO_PUBLICKEYBYTES],
             const secret_basis *B, const uint8_t key[SEEDBYTES]) {
    size_t off = 0;
    memcpy(sk, pk, CRYPTO_PUBLICKEYBYTES); off += CRYPTO_PUBLICKEYBYTES;
    for (int r = 0; r < D_UNI; r++)
        for (int c = 0; c < D_SOLVE; c++)
            for (int j = 0; j < N; j++)
                sk[off++] = (uint8_t)(int8_t)B->row_small[r][c].coeffs[j];
    /* row_big (F) is not stored: never used in signing under cut-F. */
    memcpy(sk + off, key, SEEDBYTES); off += SEEDBYTES;
    (void)off;
}

void unpack_sk(uint8_t pk[CRYPTO_PUBLICKEYBYTES], secret_basis *B,
               uint8_t key[SEEDBYTES], const uint8_t sk[CRYPTO_SECRETKEYBYTES]) {
    size_t off = 0;
    memcpy(pk, sk, CRYPTO_PUBLICKEYBYTES); off += CRYPTO_PUBLICKEYBYTES;
    for (int r = 0; r < D_UNI; r++)
        for (int c = 0; c < D_SOLVE; c++)
            for (int j = 0; j < N; j++)
                B->row_small[r][c].coeffs[j] = (int8_t)sk[off++];
    /* row_big (F) is not stored; zero it so the struct is well-defined. */
    for (int c = 0; c < D_SOLVE; c++)
        for (int j = 0; j < N; j++)
            B->row_big[c].coeffs[j] = 0;
    memcpy(key, sk + off, SEEDBYTES); off += SEEDBYTES;
}

/* ---- w packing for the hash ---- */
void pack_w(uint8_t buf[W_PACKEDBYTES], const poly w[K]) {
    bitw bw; bw_init(&bw, buf);
    for (int i = 0; i < K; i++)
        for (int j = 0; j < N; j++)
            bw_put(&bw, (uint32_t)freeze2q(w[i].coeffs[j]), POLY2Q_BITS);
    bw_flush(&bw);
}

/* ---- signature v2: [u16 total][c_tilde][zblock(rANS)] ---- */
#include "encoding.h"

int pack_sig(uint8_t sig[CRYPTO_BYTES], size_t *siglen,
             const uint8_t c_tilde[CTILDEBYTES], const poly *z1, const poly z_rest[D_REST]) {
    size_t zlen = encode_z(sig + 2 + CTILDEBYTES, CRYPTO_BYTES - 2 - CTILDEBYTES, z1, z_rest);
    if (zlen == 0) return -1;
    size_t total = 2 + CTILDEBYTES + zlen;
    sig[0] = (uint8_t)total;
    sig[1] = (uint8_t)(total >> 8);
    memcpy(sig + 2, c_tilde, CTILDEBYTES);
    *siglen = total;
    return 0;
}

int unpack_sig(uint8_t c_tilde[CTILDEBYTES], poly *z1, poly z_rest[D_REST],
               const uint8_t *sig, size_t siglen) {
    if (siglen < 2 + CTILDEBYTES + 4) return -1;
    size_t total = (size_t)sig[0] | ((size_t)sig[1] << 8);
    /* Exact-length check: the embedded total must match siglen precisely.
     * Accepting total < siglen (trailing ignored bytes) would make signatures
     * malleable and break SUF-CMA (strong unforgeability): an attacker could
     * append arbitrary bytes to a valid signature and still verify.  The
     * attached-message path (crypto_sign_open) derives the exact siglen from
     * the 2-byte header before calling verify, so this does not break it. */
    if (total != siglen) return -1;
    if (total < 2 + CTILDEBYTES + 4) return -1;
    memcpy(c_tilde, sig + 2, CTILDEBYTES);
    if (decode_z(z1, z_rest, sig + 2 + CTILDEBYTES, total - 2 - CTILDEBYTES) != 0) return -1;
    return 0;
}
