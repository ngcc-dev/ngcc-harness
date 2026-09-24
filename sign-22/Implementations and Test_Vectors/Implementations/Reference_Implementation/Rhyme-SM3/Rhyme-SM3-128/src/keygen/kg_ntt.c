#include <stdint.h>
#include <string.h>
#include "kg_ntt.h"

/* tables from kg_primes.c */
extern const unsigned rhyme_kg_nprimes;
extern const uint32_t rhyme_kg_primes[];
extern const uint32_t rhyme_kg_roots[];   /* primitive 2048-th roots */

uint32_t kg_prime(unsigned idx) { return rhyme_kg_primes[idx]; }
unsigned kg_nprimes(void) { return rhyme_kg_nprimes; }

static uint32_t mulmod(uint32_t a, uint32_t b, uint32_t p) {
    return (uint32_t)(((uint64_t)a * b) % p);
}
uint32_t kg_powmod(uint32_t a, uint32_t e, uint32_t p) {
    uint64_t r = 1, base = a % p;
    while (e) {
        if (e & 1) r = (r * base) % p;
        base = (base * base) % p;
        e >>= 1;
    }
    return (uint32_t)r;
}

/* psi = primitive 2n-th root mod p (derived from the 2048-th root by squaring) */
uint32_t kg_psi(unsigned pidx, unsigned n) {
    uint32_t p = rhyme_kg_primes[pidx];
    uint32_t w = rhyme_kg_roots[pidx];     /* order 2048 */
    unsigned target = 2 * n;               /* must divide 2048 */
    unsigned e = 2048 / target;
    return kg_powmod(w, e, p);
}

/* iterative cyclic NTT of size n (power of two), wn = primitive n-th root */
static void ntt_cyclic(uint32_t *a, unsigned n, uint32_t wn, uint32_t p) {
    /* bit reversal */
    for (unsigned i = 1, j = 0; i < n; i++) {
        unsigned bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j |= bit;
        if (i < j) { uint32_t t = a[i]; a[i] = a[j]; a[j] = t; }
    }
    for (unsigned len = 2; len <= n; len <<= 1) {
        uint32_t wl = kg_powmod(wn, n / len, p);
        for (unsigned i = 0; i < n; i += len) {
            uint32_t w = 1;
            for (unsigned k = i; k < i + len / 2; k++) {
                uint32_t u = a[k];
                uint32_t v = mulmod(a[k + len / 2], w, p);
                a[k] = u + v >= p ? u + v - p : u + v;
                a[k + len / 2] = u >= v ? u - v : u + p - v;
                w = mulmod(w, wl, p);
            }
        }
    }
}

/* negacyclic forward: out[i] = sum a_j psi^(j) evaluated ... (twist + cyclic) */
void kg_ntt_fwd(uint32_t *a, unsigned n, unsigned pidx) {
    uint32_t p = rhyme_kg_primes[pidx];
    uint32_t psi = kg_psi(pidx, n);
    uint32_t cur = 1;
    for (unsigned i = 0; i < n; i++) {
        a[i] = mulmod(a[i], cur, p);
        cur = mulmod(cur, psi, p);
    }
    ntt_cyclic(a, n, mulmod(psi, psi, p), p);
}

void kg_ntt_inv(uint32_t *a, unsigned n, unsigned pidx) {
    uint32_t p = rhyme_kg_primes[pidx];
    uint32_t psi = kg_psi(pidx, n);
    uint32_t wn = mulmod(psi, psi, p);
    uint32_t wn_inv = kg_powmod(wn, p - 2, p);
    ntt_cyclic(a, n, wn_inv, p);
    uint32_t n_inv = kg_powmod((uint32_t)n, p - 2, p);
    uint32_t psi_inv = kg_powmod(psi, p - 2, p);
    uint32_t cur = n_inv;
    for (unsigned i = 0; i < n; i++) {
        a[i] = mulmod(a[i], cur, p);
        cur = mulmod(cur, psi_inv, p);
    }
}
