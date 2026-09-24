#include <stdint.h>
#include "params.h"
#include "zpntt.h"
#include "kg_ntt.h"

/* prime and primitive 2N-th root come from the keygen prime table */
static uint32_t P;        /* kg_prime(0), 31-bit, P = 1 mod 2N */
static uint32_t PINV;     /* -P^-1 mod 2^32 */
static uint32_t F_INV;    /* N^-1 * R^2 mod P (plain), final inv scaling */
static uint32_t zetas[N]; /* psi^brv(k) in Montgomery domain, Dilithium layout */
static int ready;

uint32_t zpntt_mont_mul(uint32_t a, uint32_t b) {
    uint64_t t = (uint64_t)a * b;
    uint32_t m = (uint32_t)t * PINV;
    uint32_t r = (uint32_t)((t + (uint64_t)m * P) >> 32);
    return r >= P ? r - P : r;
}

static uint32_t addm(uint32_t a, uint32_t b) {
    uint32_t r = a + b;
    return r >= P ? r - P : r;
}
static uint32_t subm(uint32_t a, uint32_t b) {
    return a >= b ? a - b : a + P - b;
}

uint32_t zpntt_prime(void) { return P; }

void zpntt_init(void) {
    if (ready) return;
    P = kg_prime(0);
    /* PINV = -P^-1 mod 2^32 by Newton iteration */
    uint32_t inv = P;                 /* inverse of P mod 2^32 */
    for (int i = 0; i < 4; i++) inv *= 2 - P * inv;
    PINV = (uint32_t)(0u - inv);
    uint32_t psi = kg_psi(0, N);      /* primitive 2N-th root */
    uint32_t R2 = (uint32_t)((((unsigned __int128)1 << 64) % P));
    unsigned logn = 0;
    while ((1u << logn) < N) logn++;
    /* zetas[k] = mont(psi^brv(k)) */
    uint32_t *pw = zetas;             /* reuse as scratch for powers */
    uint32_t cur = 1;
    static uint32_t pows[N];
    for (unsigned i = 0; i < N; i++) {
        pows[i] = cur;
        cur = (uint32_t)(((uint64_t)cur * psi) % P);
    }
    for (unsigned k = 0; k < N; k++) {
        unsigned r = 0, x = k;
        for (unsigned b = 0; b < logn; b++) { r = (r << 1) | (x & 1); x >>= 1; }
        pw[k] = zpntt_mont_mul(pows[r], R2);   /* to Montgomery */
    }
    uint32_t ninv = kg_powmod((uint32_t)N, P - 2, P);
    F_INV = (uint32_t)(((uint64_t)ninv * R2) % P);
    ready = 1;
}

/* forward complete negacyclic NTT, values stay in the standard domain */
void zpntt_fwd(uint32_t a[N]) {
    unsigned k = 0;
    for (unsigned len = N / 2; len >= 1; len >>= 1) {
        for (unsigned start = 0; start < N; start += 2 * len) {
            uint32_t zeta = zetas[++k];
            for (unsigned j = start; j < start + len; j++) {
                uint32_t t = zpntt_mont_mul(zeta, a[j + len]);
                a[j + len] = subm(a[j], t);
                a[j] = addm(a[j], t);
            }
        }
    }
}

/* inverse; output multiplied by R (compensates the R^-1 deficit of the
 * pointwise Montgomery products, so fwd -> mont_mul -> inv gives ab mod P) */
void zpntt_inv_tomont(uint32_t a[N]) {
    unsigned k = N;
    for (unsigned len = 1; len < N; len <<= 1) {
        for (unsigned start = 0; start < N; start += 2 * len) {
            uint32_t zeta = P - zetas[--k];
            for (unsigned j = start; j < start + len; j++) {
                uint32_t t = a[j];
                a[j] = addm(t, a[j + len]);
                a[j + len] = zpntt_mont_mul(zeta, subm(t, a[j + len]));
            }
        }
    }
    for (unsigned j = 0; j < N; j++)
        a[j] = zpntt_mont_mul(a[j], F_INV);
}
