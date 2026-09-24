/*************************************************
* File:        sign.c
*
* Description: Rhyme signature scheme API: key-pair generation, signing
*              (Fiat-Shamir with cut-F / widened-D unimodular basis, rANS
*              entropy coding), and verification, plus the combined
*              signed-message wrappers crypto_sign / crypto_sign_open.
**************************************************/
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "params.h"
#include "sign.h"
#include "poly.h"
#include "packing.h"
#include "encoding.h"
#include "sampler.h"
#include "symmetric.h"
#include "rhyme_xof.h"
#include "randombytes.h"
#include "reduce.h"
#include <stdio.h>

/* keygen solver (C port of the module-NTRU det=1 solver) */
int rhyme_keygen_basis(secret_basis *B, const uint8_t seed[SEEDBYTES]);

/* ------------------------------------------------------------------ helpers */

/* w = 2*[ (Agen*ys + ye - b*y1) mod q ] + q*(parity j-term), all mod 2q.
 * Agen in NTT domain (K x (L-1)); b in NTT domain (K); inputs y* standard.
 * jpar: parity bit polynomial coefficient for the j-term (only row 0 gets q*par). */
/*************************************************
* Name:        compute_w
*
* Description: Computes the commitment vector w = 2*[(Agen*ys + ye - b*y1) mod q] plus the
*              parity j-term, reduced mod 2q. Agen and b are in NTT domain; the y* masks are
*              in standard domain.
*
* Arguments:   - poly w[K]:                   output commitment vector
*              - const poly Agen[K][M_SGEN]:  public matrix A in NTT domain
*              - const poly b_ntt[K]:         public vector b in NTT domain
*              - const poly *y1:              first mask polynomial
*              - const poly y_rest[D_REST]:   remaining mask polynomials
*              - int sub_par:                 parity bit for the j-term
**************************************************/
static void compute_w(poly w[K], const poly Agen[K][M_SGEN], const poly b_ntt[K],
                      const poly *y1, const poly y_rest[D_REST], int sub_par) {
    poly y1h, t;
    poly yrh[M_SGEN];          /* only ys is needed in the NTT domain */
    y1h = *y1;
    poly_ntt(&y1h);
    for (int i = 0; i < M_SGEN; i++) { yrh[i] = y_rest[i]; poly_ntt(&yrh[i]); }

    for (int i = 0; i < K; i++) {
        poly acc;
        poly_zero(&acc);
        /* + Agen * ys  (ys = first L-1 of y_rest) */
        for (int j = 0; j < M_SGEN; j++)
            poly_basemul_acc(&acc, &Agen[i][j], &yrh[j]);
        /* - b*y1 */
        poly_basemul(&t, &b_ntt[i], &y1h);
        poly_sub(&acc, &acc, &t);
        poly_invntt_tomont(&acc);
        /* + ye_i (standard domain; ye = last K of y_rest) */
        poly_add(&acc, &acc, (const poly *)&y_rest[M_SGEN + i]);
        /* mod q, double */
        for (int j = 0; j < N; j++)
            w[i].coeffs[j] = 2 * freeze(acc.coeffs[j]);
    }
    /* j-term: row 0 gets q * ((y1 + sub_par*c) mod 2) -- caller folds c parity via sub_par flag:
     * sign:    sub_par = 0  -> + q*(y1 mod 2)
     * verify:  sub_par = 1  -> + q*((z1 + c) mod 2), with c added by caller into y1 copy.
     * Here we only use parity of y1 input. */
    (void)sub_par;
}

/* parity j-term applied separately for clarity */
/*************************************************
* Name:        add_jterm
*
* Description: Adds the q-scaled parity (j-) term to row 0 of the commitment, derived from
*              the least-significant bits of the given polynomials.
*
* Arguments:   - poly w[K]:        commitment vector to update
*              - const poly *p1:   first parity source polynomial
*              - const poly *p2:   second parity source polynomial (may be NULL)
**************************************************/
static void add_jterm(poly w[K], const poly *p1, const poly *p2 /* may be NULL */) {
    for (int j = 0; j < N; j++) {
        int32_t par = p1->coeffs[j];
        if (p2) par += p2->coeffs[j];
        par &= 1;
        w[0].coeffs[j] = freeze2q(w[0].coeffs[j] + par * Q);
    }
    for (int i = 1; i < K; i++)
        for (int j = 0; j < N; j++)
            w[i].coeffs[j] = freeze2q(w[i].coeffs[j]);
}

/*************************************************
* Name:        expand_A
*
* Description: Deterministically expands the public matrix A (in NTT domain) from a public
*              seed via rejection sampling.
*
* Arguments:   - poly Agen[K][M_SGEN]:            output matrix in NTT domain
*              - const uint8_t seedA[SEEDBYTES]:  public seed
**************************************************/
static void expand_A(poly Agen[K][M_SGEN], const uint8_t seedA[SEEDBYTES]) {
    for (int i = 0; i < K; i++)
        for (int j = 0; j < M_SGEN; j++)
            poly_uniform_ntt(&Agen[i][j], seedA, (uint16_t)((i << 8) | j));
}

#ifdef RHYME_PROFILE_SECTIONS
#include <stdio.h>
uint64_t rhyme_prof[8];
/*************************************************
* Name:        prof_rdtsc
*
* Description: Reads the CPU timestamp counter (used only for optional section profiling).
*
* Returns:     Current 64-bit timestamp counter value.
**************************************************/
static uint64_t prof_rdtsc(void) {
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}
#define PROF_T(var) uint64_t var = prof_rdtsc()
#define PROF_ACC(idx, from) do { uint64_t now_ = prof_rdtsc(); rhyme_prof[idx] += now_ - (from); (from) = now_; } while (0)
#else
#define PROF_T(var) do {} while (0)
#define PROF_ACC(idx, from) do {} while (0)
#endif

/* ---- exact negacyclic products over Z via one 31-bit prime NTT ----
 * All products in the signing hot path are bounded well below p/2
 * (max ~2^26 vs p/2 ~ 2^30), so a single-prime NTT with centered lift
 * is exact and ~20x cheaper than schoolbook.  Bit-identical results. */
#include "zpntt.h"

/*************************************************
* Name:        zp_fwd
*
* Description: Lifts a polynomial into the auxiliary 31-bit prime NTT domain used for exact
*              integer products during signing.
*
* Arguments:   - uint32_t out[N]:   output residues in the auxiliary NTT domain
*              - const poly *a:     input polynomial
**************************************************/
static void zp_fwd(uint32_t out[N], const poly *a) {
    uint32_t p = zpntt_prime();
    for (int i = 0; i < N; i++) {
        int64_t v = a->coeffs[i] % (int64_t)p;
        if (v < 0) v += p;
        out[i] = (uint32_t)v;
    }
    zpntt_fwd(out);
}
/* c += centered_lift(inv_tomont(h)); h holds Montgomery pointwise products */
/*************************************************
* Name:        zp_add_lift
*
* Description: Performs the inverse auxiliary-prime NTT on h and accumulates the lifted
*              result into polynomial c.
*
* Arguments:   - poly *c:        accumulator polynomial
*              - uint32_t h[N]:  residues in the auxiliary NTT domain (consumed)
**************************************************/
static void zp_add_lift(poly *c, uint32_t h[N]) {
    uint32_t p = zpntt_prime();
    zpntt_inv_tomont(h);
    for (int i = 0; i < N; i++) {
        int64_t v = h[i];
        if (v > p / 2) v -= p;
        c->coeffs[i] += (int32_t)v;
    }
}

/* ------------------------------------------------------------------ keypair */

/*************************************************
* Name:        crypto_sign_keypair_from_basis
*
* Description: Builds a public/secret key pair from an already-solved secret unimodular
*              basis and a public seed for A.
*
* Arguments:   - uint8_t *pk:                     output public key
*              - uint8_t *sk:                     output secret key
*              - const secret_basis *B:           solved unimodular basis
*              - const uint8_t seedA[SEEDBYTES]:  public seed for A
*
* Returns:     0 on success, nonzero on failure.
**************************************************/
int crypto_sign_keypair_from_basis(uint8_t *pk, uint8_t *sk,
                                   const secret_basis *B,
                                   const uint8_t seedA[SEEDBYTES],
                                   const uint8_t key[SEEDBYTES]) {
    poly Agen[K][M_SGEN], b[K];
    expand_A(Agen, seedA);

    /* s_tail = first d entries of column 0 of B = the d SHORT rows' column-0
     * polys.  (Under cut-F the (d+1)-th entry, row_big[0], is the discarded
     * extension dimension and is NOT part of s_tail.)
     * sgen = first L-1 entries, egen = last K entries of s_tail. */
    poly s_tail[D_UNI];
    for (int r = 0; r < D_UNI; r++) s_tail[r] = B->row_small[r][0];

    /* b = Agen*sgen + egen mod q */
    poly sgen_ntt[M_SGEN];
    for (int j = 0; j < M_SGEN; j++) { sgen_ntt[j] = s_tail[j]; poly_ntt(&sgen_ntt[j]); }
    for (int i = 0; i < K; i++) {
        poly acc; poly_zero(&acc);
        for (int j = 0; j < M_SGEN; j++)
            poly_basemul_acc(&acc, &Agen[i][j], &sgen_ntt[j]);
        poly_invntt_tomont(&acc);
        poly_add(&acc, &acc, &s_tail[M_SGEN + i]);
        poly_freeze(&acc);
        b[i] = acc;
        }
    pack_pk(pk, seedA, b);
    pack_sk(sk, pk, B, key);
    return 0;
}

/*************************************************
* Name:        crypto_sign_keypair
*
* Description: Generates a Rhyme key pair: samples a fresh random seed, solves the secret
*              unimodular basis, and encodes the public and secret keys.
*
* Arguments:   - uint8_t *pk:   output public key
*              - uint8_t *sk:   output secret key
*
* Returns:     0 on success, nonzero on failure.
**************************************************/
int crypto_sign_keypair(uint8_t *pk, uint8_t *sk) {
    rhyme_encoding_init();   /* build rANS tables once, outside the sign hot path */
    uint8_t seedbuf[2 * SEEDBYTES + SEEDBYTES];
    uint8_t root[SEEDBYTES];
    randombytes(root, SEEDBYTES);
    rhyme_shake256_hash(seedbuf, sizeof seedbuf, root, SEEDBYTES);
    const uint8_t *seedA = seedbuf;
    const uint8_t *seedB = seedbuf + SEEDBYTES;
    const uint8_t *key = seedbuf + 2 * SEEDBYTES;

    secret_basis B;
    if (rhyme_keygen_basis(&B, seedB) != 0)
        return -1;
    return crypto_sign_keypair_from_basis(pk, sk, &B, seedA, key);
}

/* ------------------------------------------------------------------ sign */

/*************************************************
* Name:        crypto_sign_signature
*
* Description: Produces a detached Rhyme signature on a message. Runs the Fiat-Shamir loop:
*              samples masks, computes the commitment w, derives the challenge, forms z via
*              Algorithm-5 rejection on z1 and the D-term superposition on z_rest, checks the
*              norm gates, and rANS-encodes the accepted signature.
*
* Arguments:   - uint8_t *sig:       output signature buffer (capacity CRYPTO_BYTES)
*              - size_t *siglen:     set to the actual signature length
*              - const uint8_t *m:   message
*              - size_t mlen:        message length in bytes
*              - const uint8_t *sk:  secret key
*
* Returns:     0 on success, nonzero on failure.
**************************************************/
int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen, const uint8_t *sk) {
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t key[SEEDBYTES], seedA[SEEDBYTES];
    uint8_t mu[CRHBYTES], rhoprime[CRHBYTES], c_tilde[CTILDEBYTES];
    uint8_t wbuf[W_PACKEDBYTES];
    secret_basis B;
    poly Agen[K][M_SGEN], b[K];
    poly y1, z1, c, v;
    poly y_rest[D_REST], z_rest[D_REST];
    poly w[K];
    keccak_state ks;

    zpntt_init();
    unpack_sk(pk, &B, key, sk);
    unpack_pk(seedA, b, pk);
    expand_A(Agen, seedA);
    for (int i = 0; i < K; i++) poly_ntt(&b[i]);

    /* s_tail (first d entries of column 0 = the d short rows' col-0 polys)
     * and B' (columns 1..d of the d short rows). */
    poly s_tail[D_UNI];
    for (int r = 0; r < D_UNI; r++) s_tail[r] = B.row_small[r][0];

    /* cache NTT_p forms of 2*B' and s_tail (constant for the whole signature).
     * B_short = the D_UNI short rows; B' = its columns 1..D_SOLVE-1 (= d columns). */
    static uint32_t Bhat[D_UNI][D_SOLVE - 1][N];
    static uint32_t shat[D_UNI][N];
    for (int r = 0; r < D_UNI; r++) {
        for (int cidx = 1; cidx < D_SOLVE; cidx++) {
            const poly *Bent = &B.row_small[r][cidx];   /* always a short row */
            poly two_b;
            for (int j = 0; j < N; j++) two_b.coeffs[j] = 2 * Bent->coeffs[j];
            zp_fwd(Bhat[r][cidx - 1], &two_b);
        }
        zp_fwd(shat[r], &s_tail[r]);
    }

    /* mu = CRH(pk || m) ; rhoprime = CRH(key || mu) */
    rhyme_shake256_init(&ks);
    rhyme_shake256_absorb(&ks, pk, CRYPTO_PUBLICKEYBYTES);
    rhyme_shake256_absorb(&ks, m, mlen);
    rhyme_shake256_finalize(&ks);
    rhyme_shake256_squeeze(mu, CRHBYTES, &ks);

    rhyme_shake256_init(&ks);
    rhyme_shake256_absorb(&ks, key, SEEDBYTES);
    rhyme_shake256_absorb(&ks, mu, CRHBYTES);
    rhyme_shake256_finalize(&ks);
    rhyme_shake256_squeeze(rhoprime, CRHBYTES, &ks);

    for (uint16_t iter = 0; iter < MAX_SIGN_ITERATIONS; iter++) {
        uint16_t base = (uint16_t)(iter * 64);
#ifdef RHYME_PROFILE_SECTIONS
        uint64_t tp = prof_rdtsc();
#endif

        /* y1 and X', e_bottom.  X' has D_SOLVE-1 = d components (the masks for
         * B' columns 1..d); e_main has D_REST = d components (one per short row). */
        SampleY1(&y1, rhoprime, base + 0);
        poly X[D_SOLVE - 1], e[D_REST];
        for (int i = 0; i < D_SOLVE - 1; i++) SampleGauss(&X[i], rhoprime, (uint16_t)(base + 1 + i));
        for (int i = 0; i < D_REST; i++)      SampleGaussE(&e[i], rhoprime, (uint16_t)(base + 16 + i));

        PROF_ACC(0, tp);
        /* y_bottom = 2*B'*X' + e via single-prime NTT (exact, cached Bhat).
         * Sum over the D_SOLVE-1 = d columns of B'; one output per short row. */
        {
            uint32_t Xhat[D_SOLVE - 1][N], rowacc[N];
            uint32_t p = zpntt_prime();
            for (int cidx = 1; cidx < D_SOLVE; cidx++)
                zp_fwd(Xhat[cidx - 1], &X[cidx - 1]);
            for (int r = 0; r < D_UNI; r++) {
                for (int j = 0; j < N; j++) {
                    uint64_t t = 0;
                    for (int cidx = 1; cidx < D_SOLVE; cidx++)
                        t += zpntt_mont_mul(Bhat[r][cidx - 1][j], Xhat[cidx - 1][j]);
                    /* each term < p, so t < (D_SOLVE-1)*p < 2^34; reduce by
                     * conditional subtraction (quotient < D_SOLVE-1 <= 7)
                     * instead of a 64-bit division. */
                    while (t >= p) t -= p;
                    rowacc[j] = (uint32_t)t;
                }
                z_rest[r] = e[r];
                zp_add_lift(&z_rest[r], rowacc);
            }
        }
        for (int i = 0; i < D_REST; i++) y_rest[i] = z_rest[i];     /* y_bottom */

        /* w = A*y mod 2q */
        PROF_ACC(1, tp);
        compute_w(w, (const poly (*)[M_SGEN])Agen, (const poly *)b, &y1, y_rest, 0);
        PROF_ACC(6, tp);   /* compute_w alone */
        add_jterm(w, &y1, NULL);

        /* c = H(w, mu) */
        pack_w(wbuf, w);
        rhyme_shake256_init(&ks);
        rhyme_shake256_absorb(&ks, wbuf, W_PACKEDBYTES);
        rhyme_shake256_absorb(&ks, mu, CRHBYTES);
        rhyme_shake256_finalize(&ks);
        rhyme_shake256_squeeze(c_tilde, CTILDEBYTES, &ks);
        PROF_ACC(7, tp);   /* pack_w + hash */
        SampleChallenge(&c, c_tilde);
        PROF_ACC(2, tp);   /* challenge only */

        /* z1, v via Algorithm 5 (fresh randomness per iteration) */
        stream256_state rejst;
        stream256_init(&rejst, rhoprime, (uint16_t)(base + 48));
        int sz_ok = Sample_Z(&z1, &v, &c, &y1, &rejst);
        PROF_ACC(3, tp);
        if (!sz_ok)
            continue;

        /* z_bottom = y_bottom + s_tail * v  (exact over Z, cached shat) */
        {
            uint32_t vhat[N], w2[N];
            zp_fwd(vhat, &v);
            for (int r = 0; r < D_REST; r++) {
                for (int j = 0; j < N; j++)
                    w2[j] = zpntt_mont_mul(shat[r][j], vhat[j]);
                z_rest[r] = y_rest[r];
                zp_add_lift(&z_rest[r], w2);
            }
        }

        /* L_inf bound on z_rest: reject if any |z_rest| > B1 (z1 is already
         * guaranteed |z1|<=B0 by the Algorithm-5 rejection sampler). */
        {
            int linf_bad = 0;
            for (int r = 0; r < D_REST; r++)
                if (poly_chknorm(&z_rest[r], B1)) { linf_bad = 1; break; }
            if (linf_bad) continue;
        }

        /* global L2 bound */
        uint64_t sq = 0;
        poly_sqnorm_acc(&sq, &z1);
        for (int r = 0; r < D_REST; r++) poly_sqnorm_acc(&sq, &z_rest[r]);
        PROF_ACC(4, tp);
        if (sq > RHYME_L2SQ) {
#ifdef RHYME_DEBUG_L2
            if (iter < 5) fprintf(stderr, "iter %u: ||z||^2 = %llu vs L2SQ %llu\n",
                                  iter, (unsigned long long)sq, (unsigned long long)RHYME_L2SQ);
#endif
            continue;
        }

#ifdef RHYME_PROFILE_SECTIONS
        uint64_t tp2 = prof_rdtsc();
#endif
        if (pack_sig(sig, siglen, c_tilde, &z1, z_rest) != 0)
            continue;
#ifdef RHYME_PROFILE_SECTIONS
        rhyme_prof[5] += prof_rdtsc() - tp2;
#endif
#ifdef RHYME_PROFILE_ITERS
        fprintf(stderr, "ITERS %u\n", iter + 1);
#endif
        return 0;
    }
    return -1;
}

/*************************************************
* Name:        crypto_sign
*
* Description: Produces a combined signed message sm = signature || message. The true
*              signature length is stored in the signature header, so no padding is used.
*
* Arguments:   - uint8_t *sm:        output signed message
*              - size_t *smlen:      set to the signed-message length
*              - const uint8_t *m:   message
*              - size_t mlen:        message length
*              - const uint8_t *sk:  secret key
*
* Returns:     0 on success, nonzero on failure.
**************************************************/
int crypto_sign(uint8_t *sm, size_t *smlen, const uint8_t *m, size_t mlen,
                const uint8_t *sk) {
    size_t siglen;
    /* sign first into the front of sm, then place the message immediately
     * after the *actual* (variable-length) signature -- no padding. */
    if (crypto_sign_signature(sm, &siglen, m, mlen, sk))
        return -1;
    memmove(sm + siglen, m, mlen);
    *smlen = siglen + mlen;
    return 0;
}

/* ------------------------------------------------------------------ verify */

/*************************************************
* Name:        crypto_sign_verify
*
* Description: Verifies a detached Rhyme signature against a message and public key. Decodes
*              and length-checks the signature (exact-length, malleability-resistant),
*              re-derives the commitment and challenge, and checks all norm bounds.
*
* Arguments:   - const uint8_t *sig:  signature
*              - size_t siglen:       signature length
*              - const uint8_t *m:    message
*              - size_t mlen:         message length
*              - const uint8_t *pk:   public key
*
* Returns:     0 if the signature is valid, nonzero otherwise.
**************************************************/
int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen, const uint8_t *pk) {
    rhyme_encoding_init();   /* ensure tables exist for verify-only processes */
    uint8_t seedA[SEEDBYTES];
    uint8_t mu[CRHBYTES], c_tilde[CTILDEBYTES], c_tilde2[CTILDEBYTES];
    uint8_t wbuf[W_PACKEDBYTES];
    poly Agen[K][M_SGEN], b[K];
    poly z1, c;
    poly z_rest[D_REST];
    poly w[K];
    keccak_state ks;

    if (unpack_sig(c_tilde, &z1, z_rest, sig, siglen) != 0)
        return -1;

    /* bounds: |z1|_inf <= B0, |z_rest|_inf <= B1, and ||z||_2^2 <= L2SQ */
    if (poly_chknorm(&z1, B0)) return -1;
    uint64_t sq = 0;
    poly_sqnorm_acc(&sq, &z1);
    for (int r = 0; r < D_REST; r++) {
        if (poly_chknorm(&z_rest[r], B1)) return -1;   /* L_inf union bound */
        poly_sqnorm_acc(&sq, &z_rest[r]);
    }
    if (sq > RHYME_L2SQ) return -1;

    unpack_pk(seedA, b, pk);
    expand_A(Agen, seedA);
    for (int i = 0; i < K; i++) poly_ntt(&b[i]);

    rhyme_shake256_init(&ks);
    rhyme_shake256_absorb(&ks, pk, CRYPTO_PUBLICKEYBYTES);
    rhyme_shake256_absorb(&ks, m, mlen);
    rhyme_shake256_finalize(&ks);
    rhyme_shake256_squeeze(mu, CRHBYTES, &ks);

    SampleChallenge(&c, c_tilde);

    /* w = A z - q j c mod 2q  ==  2*[(Agen zs + ze - b z1) mod q] + q*((z1+c) mod 2) j */
    compute_w(w, (const poly (*)[M_SGEN])Agen, (const poly *)b, &z1, z_rest, 1);
    add_jterm(w, &z1, &c);

    pack_w(wbuf, w);
    rhyme_shake256_init(&ks);
    rhyme_shake256_absorb(&ks, wbuf, W_PACKEDBYTES);
    rhyme_shake256_absorb(&ks, mu, CRHBYTES);
    rhyme_shake256_finalize(&ks);
    rhyme_shake256_squeeze(c_tilde2, CTILDEBYTES, &ks);

    if (memcmp(c_tilde, c_tilde2, CTILDEBYTES) != 0)
        return -1;
    return 0;
}

/*************************************************
* Name:        crypto_sign_open
*
* Description: Opens a combined signed message: reads the exact signature length from the
*              header, splits off the message, verifies the signature, and outputs the
*              message on success.
*
* Arguments:   - uint8_t *m:         output message buffer
*              - size_t *mlen:       set to the message length
*              - const uint8_t *sm:  signed message
*              - size_t smlen:       signed-message length
*              - const uint8_t *pk:  public key
*
* Returns:     0 if valid (message recovered), nonzero otherwise.
**************************************************/
int crypto_sign_open(uint8_t *m, size_t *mlen, const uint8_t *sm, size_t smlen,
                     const uint8_t *pk) {
    /* the first two bytes of the signature hold its real total length */
    if (smlen < 2) return -1;
    size_t siglen = (size_t)sm[0] | ((size_t)sm[1] << 8);
    if (siglen < 2 || siglen > smlen) return -1;
    size_t msglen = smlen - siglen;
    if (crypto_sign_verify(sm, siglen, sm + siglen, msglen, pk))
        return -1;
    memmove(m, sm + siglen, msglen);
    *mlen = msglen;
    return 0;
}
