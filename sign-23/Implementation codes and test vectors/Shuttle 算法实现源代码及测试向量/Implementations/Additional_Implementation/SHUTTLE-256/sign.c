/*
 * sign.c -- KeyGen / Sign / Verify top-level orchestration for SHUTTLE.
 *
 * This file is pure glue: it sequences the subroutines in the
 * EXACT order the spec algorithm blocks mandate (Description.tex KeyGen
 * alg:keygen-internal L416-459, Sign alg:sign-internal L473-547, Verify
 * alg:verify-internal L557-601) and owns the four structurally
 * load-bearing decisions that no subroutine can make for it:
 *
 *   1. The kappa-counter timing.  KeyGen increments kappa BEFORE the
 *      first expansion (first attempt uses kappa=1).  Sign uses the
 * CURRENT kappa to derive seed_y then increments AFTER (first iteration
 * uses kappa=0).  The two have OPPOSITE timing; an off-by-one breaks every
 *      KAT.  Both Sign restart causes (B_v norm fail, sigEncode=bottom)
 *      re-enter the loop with kappa ALREADY advanced -- there is no reset.
 *
 *   2. The derived-matrix asymmetry.  Sign multiplies the FULL matrix
 *        A-hat = [ NTT(2(a-b)+q*j) | 2*A-hat_gen | 2*I_m ]   (width KVEC)
 *      by NTT(y') (length KVEC = 1+ELL+EM).  Verify multiplies the
 * NARROWER A1-hat = [ NTT(2(a-b)+q*j) | 2*A-hat_gen ]          (width
 * Z1LEN) by NTT(z1) (length Z1LEN = 1+ELL); the 2*I_m identity block and
 * the z2 operand are absent because z2 is reconstructed from the hint,
 *      never re-multiplied.  These are wired through TWO distinct
 *      helpers -- mat_mul_2q (signer, KVEC columns + e-block) and
 *      mat_mul_z1_2q (verifier, Z1LEN columns + the -q*c*j correction) --
 *      so the asymmetry is explicit in code, not hidden in a shared
 * helper.
 *
 *   3. The cached-matrix column-0 wiring (THE subtle bit).  The
 *      mat_mul_* helpers compute t_i = -bhat_i . x0 + sum_j Ahat_ij .
 * xs_j, i.e. they put -bhat against the constant slot x0.  The spec's
 * derived matrix column 0 is 2(a_gen - b) + q*j, so to make -bhat_i . x0
 * equal (a_gen - b)_i . x0 we must pass
 *
 *          bhat[i] = NTT( (b[i] - a_gen[i]) mod q )      (NOT NTT(b)!)
 *
 *      Then -bhat_i . x0 = -(b - a_gen)_i . x0 = (a_gen - b)_i . x0,
 * exactly the spec column-0 contribution; the factor 2 and the q*j shift
 * are applied inside the mat_mul lift step.  This is the wiring the
 *      "naive reference will catch a mismatch" note warns about; a sanity
 *      derivation: at the secret, t_0 = (a_gen-b) + A_gen.s + e' and since
 *      b = a_gen + A_gen.s + e' (KeyGen), t_0 = 0, so comY_0 = q*j -- the
 *      KeyGen relation A.[1,s,e']^T == q*j (mod 2q).
 *
 *   4. The challenge-reconstruction-consistency + commitment-parity lemmas
 *      (why Verify recomputes the SAME seed_c).  (a) Commitment-parity
 *      (lem:commitment-parity): A.u == u_0 * j (mod 2) for any u with
 *      leading entry u_0, so LSB(w) = LSB(y'_0) * j on the signer side and
 *      the verifier recovers the low bit as LSB(z0 - c) * j -- only the
 *      FIRST commitment poly carries a nonzero low bit (j kills the rest).
 *      (b) Challenge-consistency (lem:challenge-consistency): the signed
 *      challenge c' recovered by IRS satisfies c' == c (mod 2) (it differs
 *      from c only in the SIGNS of nonzero coefficients), so z0 - c ==
 * y'_0 (mod 2) and w_0' = w_0.  Combined with the hint reconstructing w_h
 *      exactly when ||z2'|| is in-bound, EncodeCom(w_h,w_0') ==
 *      EncodeCom(w_h,w_0), hence seed_c' = seed_c.  An off-by-one in the
 * LSB lift (raw parity vs freeze parity) silently breaks w_0' = w_0
 *      and the signature fails to verify with no other symptom -- which is
 *      why the lift uses RAW coefficient parity, not freeze() parity.
 *      For this same reason the SIGNER reconstructs z2' through the
 * verifier path (use_hint) rather than its own +2*z2 formula, so the value
 * it gates on is byte-identical to what the verifier will reconstruct (the
 * spec deliberately mirrors verification, Description.tex L2541).
 *
 * Raw-packing rationale: -DSIG_RAW selects pack_sig_raw/unpack_sig_raw
 * (fixed-length, never-fail) so the ENTIRE algorithm is proven correct
 * end-to-end without depending on the rANS size question.  The real
 * rANS pack_sig/unpack_sig is wired too (drop -DSIG_RAW); it round-trips
 * identically, only the byte length differs.
 *
 * Constant-time discipline: the ONLY data-dependent
 * branches are the PUBLIC rejection gates -- KeyGen's norm window (over a
 * public key statistic) and Sign's B_v gate / sigEncode-bottom restart
 * (public rejection-sampling outcomes, exactly like ML-DSA's bound check).
 * The squared-norm accumulation, the lift, the hint, and the matrix
 * products are all branchless.  The seed_c compare in Verify is
 * constant-time (ct_bytes_equal).  No floating point, no secret-dependent
 * division.
 */

#include <stdint.h>
#include <string.h>

#include "api.h" /* crypto_sign_* prototypes (mangled via namespace.h) */
#include "irs.h"
#include "packing.h"
#include "params.h"
#include "poly.h"
#include "poly_ntt.h"
#include "polyvec.h"
#include "rans.h" /* SIG_RAW_PACKED_BYTES / SIG_PACKED_BYTES + ENCODECOM */
#include "reduce.h"
#include "rounding.h"
#include "symmetric.h" /* xof256_init/_squeeze, xof_ctx */
#include "test/prof.h" /* PROF_* probes -- all ((void)0) unless PROF_TIME/PROF_RAND */

/* Defensive cap on the retry loops.  IRS is rejection-free, so the only
 * aborts are the bounded-probability B_v restart and (rANS path)
 * sigEncode=bottom; the loop terminates with overwhelming probability in a
 * handful of iterations.  The cap guards against a logic bug only and is
 * NOT a spec quantity. */
#ifndef SIGN_MAX_ITER
#    define SIGN_MAX_ITER 1000
#endif

/* EncodeCom over the full commitment VECTOR (EM polys).  The spec packs
 * the vector as PolyToBytes(w_h, d_h) || PolyToBytes(w_0, 1) applied
 * per-component and concatenated -- i.e. ALL w_h polys first, THEN all w_0
 * polys (block order, NOT interleaved).  pack_com is the per-poly
 * primitive (w_h[i] || w_0[i]); we lay the vector out in spec block order
 * here so the encoded transcript is reproducible.  ENCODECOM_VEC_BYTES is
 * the full vector length. */
#define ENCODECOM_VEC_BYTES \
    (EM * POLYWH_PACKEDBYTES + EM * POLYW0_PACKEDBYTES)

static void encode_com_vec(uint8_t *out, const poly comY_h[EM],
                           const poly comY_0[EM])
{
    int i;
    uint8_t *p = out;
    /* w_h block: EM polys, d_h bits each. */
    for (i = 0; i < EM; ++i) {
        poly_to_bytes(p, &comY_h[i], DH_BITS);
        p += POLYWH_PACKEDBYTES;
    }
    /* w_0 block: EM polys, 1 bit each. */
    for (i = 0; i < EM; ++i) {
        poly_to_bytes(p, &comY_0[i], 1);
        p += POLYW0_PACKEDBYTES;
    }
}

/* ===================================================================== *
 *  Local constant-time + small helpers                                  *
 * ===================================================================== */

/* Constant-time byte-equality: returns 1 iff a[0..len) == b[0..len), 0
 * otherwise, with NO early exit (no timing oracle on the public seed_c).
 * Used for the Verify seed_c' == seed_c compare (defense in depth: both
 * operands are public, but the audit harness scans Verify too). */
static int ct_bytes_equal(const uint8_t *a, const uint8_t *b, size_t len)
{
    unsigned acc = 0;
    size_t i;
    for (i = 0; i < len; ++i)
        acc |= (unsigned)(a[i] ^ b[i]);
    /* acc == 0 <=> equal.  For acc in [0,255]: (acc | (0u-acc)) has bit 31
     * set iff acc != 0 (0u-acc borrows into the high bits when acc>0); the
     * >>31 yields 1 iff not-equal, so 1 ^ that yields 1 iff equal. */
    return (int)(1u ^ (((acc | (0u - acc)) >> 31) & 1u));
}

/* Set v to the constant polynomial "1" (coeff 0 = 1, all others 0).  This
 * is the leading slot of the full secret vector sk_full = [1, s, e']^T
 * (the "constant 1" of the spec), the multiplicative identity of R. */
static void poly_set_one(poly *v)
{
    memset(v, 0, sizeof *v);
    v->coeffs[0] = 1;
}

/* ===================================================================== *
 *  Cached public matrix build (shared by Sign and Verify setup)         *
 *                                                                       *
 *  Produces bhat[EM] = NTT((b - a_gen) mod q) and Ahat[EM*ELL] = hAgen   *
 *  (already NTT-domain from ExpandA).  This is the cached column-0 +     *
 *  A_gen block of the derived matrix; the factor-2, the q*j shift, the   *
 *  2*I_m e-block (Sign) and the -q*c*j correction (Verify) are applied   *
 *  inside the mat_mul_* lift, NOT here.  See note (3) in the file        *
 *  banner for why column 0 is (b - a_gen), negated by mat_mul.           *
 *                                                                       *
 *  The bhat forward NTT goes through poly_ntt_cache, which lands in the  *
 *  backend-native slot order (ref: canonical == native; AVX: vectorized  *
 *  NTT).  bhat is consumed only by mat_mul_*, whose SIMD path therefore  *
 *  imports it directly without a per-use nttunpack.  Ahat = hAgen stays  *
 *  canonical (ExpandA order) and is still imported inside mat_mul_*.     *
 * ===================================================================== */
static void build_cached_matrix(poly16 bhat[EM], poly16 Ahat[EM * ELL],
                                const poly b[EM], const poly16 agen[EM],
                                const poly16 hAgen[EM * ELL])
{
    int i;
    unsigned k;
    for (i = 0; i < EM; ++i) {
        poly16 col;
        for (k = 0; k < N; ++k) {
            /* b in [0,q), agen in [0,q): difference in (-q,q); freeze to
             * [0,q).  freeze() takes a signed int32 and returns [0,q). */
            int32_t d = b[i].coeffs[k] - (int32_t)agen[i].coeffs[k];
            col.coeffs[k] = (uint16_t)freeze(d);
        }
        poly_ntt_cache(&col);
        bhat[i] = col;
    }
    for (i = 0; i < EM * ELL; ++i)
        Ahat[i] = hAgen[i];
}

/* Test/diagnostic instrumentation (NOT used by the protocol): the number
 * of kappa attempts the LAST keygen_from_xi consumed, i.e. the count of
 * norm-window evaluations before acceptance.  The empirical accept rate is
 * the reciprocal of the mean of this over many keygens.  A single global
 * write per attempt; no branch on secret data, no effect on output. */
uint32_t shuttle_last_keygen_attempts = 0;

/* Number of Sign-loop iterations the LAST sign_internal consumed (1 ==
 * accepted on the first try).  Public diagnostic for the per-iteration
 * B_v/sigEncode acceptance rate (1 / mean(attempts)); a single global
 * write on the accepting iteration, no branch on secret data, no effect on
 * output. */
uint32_t shuttle_last_sign_attempts = 0;

/* ===================================================================== *
 *  KeyGen  (crypto_sign_keypair) -- alg:keygen-internal                 *
 * ===================================================================== *
 * Deterministic given xi.  kappa timing: kappa starts at 0 and is
 * incremented to 1 BEFORE the first ExpandSeeds (first attempt uses
 * kappa=1, OPPOSITE to Sign).  Each norm-window failure loops back with
 * kappa incremented again.  KeyGen is over a PUBLIC key statistic, so the
 * window retry is a public-data branch.
 */
static int keygen_from_xi(uint8_t *pk, uint8_t *sk,
                          const uint8_t xi[SEEDBYTES])
{
    uint32_t kappa = 0;
    int iter;

    uint8_t T[EXPAND_SEEDS_BYTES];
    const uint8_t *seedA, *seedsk, *masterK;
    poly16 agen[EM];
    poly16 hAgen[EM * ELL];
    poly s1s2[ELL + EM]; /* aliased: s = s1s2[0..ELL), e = s1s2[ELL..) */
    poly *s = s1s2;
    poly *e = s1s2 + ELL;
    poly b0[EM];
    poly b[EM];
    poly ep[EM];
    poly stretched[KVEC];
    uint8_t tr[CHALLENGESEEDBYTES];
    int j;

    for (iter = 0; iter < SIGN_MAX_ITER; ++iter) {
        kappa += 1; /* increment BEFORE use: first attempt kappa=1 */

        /* Step 2: T = ExpandSeeds(...); slice seedA | seedsk | K. */
        expand_seeds(T, xi, kappa);
        seedA = T;
        seedsk = T + SEEDBYTES;
        masterK = T + SEEDBYTES + CHALLENGESEEDBYTES;

        /* Step 3: (a_gen, A-hat_gen) = ExpandA(seedA). */
        PROF_CTX(PC_A);
        {
            PROF_START(t_kga);
            expand_a(agen, hAgen, seedA);
            PROF_STOP(PT_KG_EXPAND_A, t_kga);
        }

        /* Step 4: (s, e) = ExpandS(seedsk). */
        PROF_CTX(PC_NOISE);
        {
            PROF_START(t_kgn);
            expand_s(s1s2, seedsk);
            PROF_STOP(PT_KG_NOISE, t_kgn);
        }
        PROF_CTX(PC_OTHER);

        /* Step 5: b_0 = a_gen + iNTT(A-hat_gen o NTT(s)) + e (mod q).
         * Routed through rounding.c keygen_bproduct: the scalar (ref)
         * build runs the canonical NTT oracle; the avx2/avx512 forks route
         * the matrix-vector product through the SIMD NTT (byte-exact). */
        {
            PROF_START(t_kgb);
            keygen_bproduct(b0, agen, hAgen, s, e);
            PROF_STOP(PT_KG_BPRODUCT, t_kgb);
        }

        /* Steps 6-9: b = RoundB(b_0); e' = e + (b-b0) bmodpm q (fused). */
        {
            PROF_START(t_kgr);
            roundB_update_s2(b, ep, b0, e);
            PROF_STOP(PT_KG_ROUNDB, t_kgr);
        }

        /* Step 10: norm-window gate over StretchS(1, s, e'). */
        {
            poly full[KVEC];
            PROF_START(t_kgs);
            poly_set_one(&full[0]);
            for (j = 0; j < ELL; ++j)
                full[1 + j] = s[j];
            for (j = 0; j < EM; ++j)
                full[1 + ELL + j] = ep[j];
            stretch_s(stretched, full);
            PROF_STOP(PT_KG_STRETCH, t_kgs);
        }
        {
            int ok;
            PROF_START(t_kgw);
            ok = keygen_norm_ok(stretched);
            PROF_STOP(PT_KG_NORM, t_kgw);
            if (!ok)
                continue; /* loop back, kappa increments again */
        }

        /* Steps 11-13: pkEncode, HashPK(tr), skEncode. */
        {
            PROF_START(t_kgp);
            /* Step 11: pk = pkEncode(seedA, b). */
            pack_pk(pk, seedA, b);

            /* Step 12: tr = HashPK(pk) over the ALREADY-ENCODED pk (0x05).
             */
            {
                uint8_t pkhash_in[1 + CRYPTO_PUBLICKEYBYTES];
                xof_ctx hctx;
                pkhash_in[0] = DS_HASH_PK;
                memcpy(pkhash_in + 1, pk, CRYPTO_PUBLICKEYBYTES);
                xof256_init(&hctx, pkhash_in, 1 + CRYPTO_PUBLICKEYBYTES);
                xof256_squeeze(&hctx, tr, CHALLENGESEEDBYTES);
            }

            /* Step 13: sk = skEncode(seedA, b, K, tr, s, e'). */
            pack_sk(sk, seedA, b, masterK, tr, s, ep);
            PROF_STOP(PT_KG_PACK, t_kgp);
        }
        shuttle_last_keygen_attempts = kappa; /* attempts == final kappa */
        return 0;
    }
    return -3; /* defensive: window never accepted within the cap */
}

/* Exposed xi-driven KeyGen so the NGCC adapter and the test harness can
 * inject the DRNG-drawn / KAT-fixed seed.  Mangled via namespace.h. */
int crypto_sign_keypair_xi(uint8_t *pk, uint8_t *sk,
                           const uint8_t xi[SEEDBYTES])
{
    return keygen_from_xi(pk, sk, xi);
}

int crypto_sign_keypair(uint8_t *pk, uint8_t *sk)
{
    /* In-tree NIST entry: deterministic (xi = 0).  Real randomness is
     * injected by the NGCC adapter / test harness through _xi.  (The NGCC
     * harness owns the DRNG; the standalone NIST entry is used by tests
     * that drive xi explicitly via crypto_sign_keypair_xi.) */
    uint8_t xi[SEEDBYTES];
    memset(xi, 0, sizeof xi);
    return keygen_from_xi(pk, sk, xi);
}

/* ===================================================================== *
 *  Sign  (crypto_sign_signature) -- alg:sign-internal                   *
 * ===================================================================== */

/* HashMsg staging cap.  The NGCC KAT signs <= 128-byte messages; the
 * reference build stages tag||tr||M for one-shot XOF absorb (the unified
 * XOF init absorbs one contiguous buffer).  Longer messages would need a
 * streaming absorb (out of scope for the NGCC harness). */
#define MU_STAGE_CAP 8192

static int sign_internal(uint8_t *sig, size_t *siglen, const uint8_t *m,
                         size_t mlen, const uint8_t *sk,
                         const uint8_t rnd[RNDBYTES])
{
    uint8_t seedA[SEEDBYTES];
    poly b[EM];
    uint8_t masterK[CHALLENGESEEDBYTES];
    uint8_t tr[CHALLENGESEEDBYTES];
    poly s[ELL];
    poly ep[EM];

    poly16 agen[EM];
    poly16 hAgen[EM * ELL];
    poly16 bhat[EM];
    poly16 Ahat[EM * ELL];
    poly sk_full[KVEC];
    poly sk_tilde[KVEC];
    uint8_t mu[CHALLENGESEEDBYTES];

    uint32_t kappa;
    int iter, j;
    unsigned k;

    /* Pre-loop step 1: skDecode.  -2 on a malformed sk (usage error). */
    PROF_CTX(PC_SETUP);
    {
        PROF_START(t_su);
        if (unpack_sk(seedA, b, masterK, tr, s, ep, sk) != 0)
            return -2;

        /* Pre-loop step 2: sk_full = [1, s, e']^T. */
        poly_set_one(&sk_full[0]);
        for (j = 0; j < ELL; ++j)
            sk_full[1 + j] = s[j];
        for (j = 0; j < EM; ++j)
            sk_full[1 + ELL + j] = ep[j];
        PROF_STOP(PT_SETUP, t_su);
    }

    /* Pre-loop step 3: (a_gen, A-hat_gen) = ExpandA(seedA). */
    PROF_CTX(PC_A);
    {
        PROF_START(t_ea);
        expand_a(agen, hAgen, seedA);
        /* Pre-loop step 4: cached column-0 (b-a_gen NTT) + A_gen block.
         * The FULL derived matrix (with 2*I_m) is realized in mat_mul_2q.
         */
        build_cached_matrix(bhat, Ahat, b, agen, hAgen);
        PROF_STOP(PT_EXPAND_A, t_ea);
    }
    PROF_CTX(PC_SETUP);

    /* Pre-loop step 5: mu = HashMsg(tr || M) (0x06). */
    {
        uint8_t mustage[1 + CHALLENGESEEDBYTES + MU_STAGE_CAP];
        xof_ctx hctx;
        if (mlen > MU_STAGE_CAP)
            return -4;
        mustage[0] = DS_HASH_MSG;
        memcpy(mustage + 1, tr, CHALLENGESEEDBYTES);
        memcpy(mustage + 1 + CHALLENGESEEDBYTES, m, mlen);
        xof256_init(&hctx, mustage, 1 + CHALLENGESEEDBYTES + mlen);
        xof256_squeeze(&hctx, mu, CHALLENGESEEDBYTES);
    }

    /* Pre-loop: sk_tilde = StretchS(sk_full) (loop-invariant). */
    {
        PROF_START(t_st);
        stretch_s(sk_tilde, sk_full);
        PROF_STOP(PT_SETUP, t_st);
    }
    PROF_CTX(PC_OTHER);

    kappa = 0; /* Step 6 */
    for (iter = 0; iter < SIGN_MAX_ITER; ++iter) {
        uint8_t seedY[SEEDBYTES];
        poly y[KVEC];
        poly yp[KVEC];
        poly comY[EM];
        poly comY_h[EM];
        poly comY_0[EM];
        uint8_t seedC[CHALLENGESEEDBYTES];
        poly c;
        poly z_tilde[KVEC];
        poly z[KVEC];
        poly z1[Z1LEN];
        poly z2[EM];
        poly z2p[EM];
        poly hint[EM];
        poly wh_chk[EM];
        uint8_t encbuf[1 + ENCODECOM_VEC_BYTES + CHALLENGESEEDBYTES];
        xof_ctx irs_ctx;
        uint8_t irs_seed[1 + SEEDBYTES];

        /* Step a: seed_y = ExpandSigningSeeds(K||rnd||mu||I2B(kappa,4))
         * (0x01).  Uses the CURRENT kappa. */
        PROF_CTX(PC_GAUSS);
        {
            PROF_START(t_sy);
            expand_signing_seeds(seedY, masterK, rnd, mu, kappa);

            /* Step b: kappa++ AFTER use.  Both restart causes below
             * re-enter with kappa already advanced; there is NO reset. */
            kappa += 1;

            /* Step c: y = SampleY(seed_y) (0x08). */
            sample_y(y, seedY);

            /* Step d: y' = CompressY(y).  Keep BOTH live (IRS uses y). */
            compress_y(yp, y);
            PROF_STOP(PT_SAMPLE_Y, t_sy);
        }

        /* Steps e-g: commitment via the FULL matrix.  mat_mul_2q does
         *   bar_w = iNTT(A-hat o NTT(y')) mod q, then
         *   w = LiftToModTwoQ(bar_w, LSB(y'_0)*j), in [0,2q). */
        PROF_CTX(PC_OTHER);
        {
            PROF_START(t_cm);
            mat_mul_2q(comY, yp, bhat, Ahat);
            PROF_STOP(PT_COMMIT, t_cm);
        }

        /* w_0 = LSB(w); w_h = round(w/alpha_h) mod H_h. */
        {
            PROF_START(t_hb);
            for (j = 0; j < EM; ++j)
                for (k = 0; k < N; ++k) {
                    comY_0[j].coeffs[k] = lsb_coeff(comY[j].coeffs[k]);
                    comY_h[j].coeffs[k] =
                        highbits_reduced(comY[j].coeffs[k]);
                }
            PROF_STOP(PT_HIGHBITS, t_hb);
        }

        /* Steps h-i: seed_c = HashCh(EncodeCom(w_h, w_0) || mu) (0x04);
         * c = SampleC(seed_c) (0x07, weight tau, all +1). */
        PROF_CTX(PC_CHALLENGE);
        {
            PROF_START(t_ch);
            encbuf[0] = DS_HASH_CH;
            encode_com_vec(encbuf + 1, comY_h, comY_0);
            memcpy(encbuf + 1 + ENCODECOM_VEC_BYTES, mu,
                   CHALLENGESEEDBYTES);
            {
                xof_ctx hctx;
                xof256_init(&hctx, encbuf,
                            1 + ENCODECOM_VEC_BYTES + CHALLENGESEEDBYTES);
                xof256_squeeze(&hctx, seedC, CHALLENGESEEDBYTES);
            }
            sample_c(&c, seedC);
            PROF_STOP(PT_CHALLENGE, t_ch);
        }

        /* Steps j-l: FRESH IRS ctx (0x09||seed_y); RejectSample [IRS];
         * z = CompressY(z_tilde). */
        PROF_CTX(PC_IRS);
        {
            PROF_START(t_irs);
            irs_seed[0] = DS_IRS;
            memcpy(irs_seed + 1, seedY, SEEDBYTES);
            xof256_init(&irs_ctx, irs_seed, 1 + SEEDBYTES);

            /* NOTE: passes y (uncompressed), NOT y'. */
            reject_sample(&irs_ctx, z_tilde, y, &c, sk_tilde);
            compress_y(z, z_tilde);
            PROF_STOP(PT_IRS, t_irs);
        }
        PROF_CTX(PC_OTHER);

        /* Step m: (z1, z2) = z.  z1 = first Z1LEN, z2 = next EM. */
        for (j = 0; j < Z1LEN; ++j)
            z1[j] = z[j];
        for (j = 0; j < EM; ++j)
            z2[j] = z[Z1LEN + j];

        /* Step n: h = MakeHint(z2, w). */
        {
            PROF_START(t_mh);
            make_hint(hint, comY, z2);
            PROF_STOP(PT_MAKEHINT, t_mh);
        }

        /* Step o: z2' via the VERIFIER reconstruction (per the spec's Sign
         * norm-check: the signer reconstructs z2' the UseHint-equivalent way
         * before the B_v check).  We mirror Verify
         * EXACTLY -- reconstruct comY_0' = LSB(z0-c)*j and comY_tilde,
         * then use_hint.  This makes the gated z2' byte-identical to the
         * one the verifier will compute, so the B_v gate predicts
         * verify-accept.
         *
         * The verifier obtains comY_tilde from z1+c via mat_mul_z1_2q (the
         * NARROW Z1LEN matrix product).  The signer does NOT need that
         * second NTT-domain matrix product: with half-up CompressY the
         * hint identity comY_tilde == (comY - 2*z2) mod 2q holds EXACTLY
         * (rounding.h), and the signer already has comY (from mat_mul_2q)
         * and z2.  So we reconstruct comY_tilde with the same cheap
         * per-coeff branchless reduce_mod_2q that make_hint uses --
         * byte-identical to the verifier's mat_mul_z1_2q result, at a
         * fraction of the cost. */
        {
            poly comY0p_sig[EM];
            poly comY_tilde_sig[EM];
            int ok;
            unsigned kk;
            PROF_START(t_nc);
            recon_comY0p(&comY0p_sig[0], &z1[0], &c);
            for (j = 1; j < EM; ++j)
                memset(&comY0p_sig[j], 0, sizeof comY0p_sig[j]);
            for (j = 0; j < EM; ++j)
                for (kk = 0; kk < N; ++kk)
                    comY_tilde_sig[j].coeffs[kk] = reduce_mod_2q(
                        comY[j].coeffs[kk] - 2 * z2[j].coeffs[kk]);
            use_hint(wh_chk, z2p, hint, comY_tilde_sig, &comY0p_sig[0]);

            /* Step p: B_v gate on (z1, z2') -- NOT z2. */
            ok = response_norm_ok(z1, z2p);
            PROF_STOP(PT_NORMCHECK, t_nc);
            if (!ok)
                continue; /* kappa already advanced */
        }

        /* Step q: sigEncode.  RAW path never fails; rANS may return
         * bottom (out of support) -> restart with kappa advanced.  The
         * rANS arithmetic + byte layout are timed together as PT_RANS
         * (the variable-cost block; PT_PACK is reserved for a future
         * split of the non-rANS layout work). */
#if defined(SIG_RAW)
        {
            PROF_START(t_pk);
            pack_sig_raw(sig, seedC, z1, hint);
            PROF_STOP(PT_RANS, t_pk);
        }
        *siglen = SIG_RAW_PACKED_BYTES;
        shuttle_last_sign_attempts = (uint32_t)iter + 1;
        return 0;
#else
        {
            int rc;
            PROF_START(t_pk);
            rc = pack_sig(sig, seedC, z1, hint);
            PROF_STOP(PT_RANS, t_pk);
            if (rc == 0) {
                *siglen = SIG_PACKED_BYTES;
                shuttle_last_sign_attempts = (uint32_t)iter + 1;
                return 0;
            }
        }
        continue; /* sigEncode = bottom; kappa already advanced */
#endif
    }
    return -5; /* defensive: loop cap hit (impossible by construction) */
}

int crypto_sign_signature(uint8_t *sig, size_t *siglen, const uint8_t *m,
                          size_t mlen, const uint8_t *sk)
{
    /* In-tree NIST entry: deterministic signing (rnd = 0), KAT convention.
     * The NGCC adapter draws rnd from drng_algorithm (hedged) and calls
     * crypto_sign_signature_rnd directly. */
    uint8_t rnd[RNDBYTES];
    memset(rnd, 0, sizeof rnd);
    return sign_internal(sig, siglen, m, mlen, sk, rnd);
}

int crypto_sign_signature_rnd(uint8_t *sig, size_t *siglen,
                              const uint8_t *m, size_t mlen,
                              const uint8_t *sk,
                              const uint8_t rnd[RNDBYTES])
{
    return sign_internal(sig, siglen, m, mlen, sk, rnd);
}

/* ===================================================================== *
 *  Verify  (crypto_sign_verify) -- alg:verify-internal                  *
 * ===================================================================== *
 * Return contract: 0 valid, -1 invalid, -2..-99 other error.
 */
int crypto_sign_verify(const uint8_t *sig, size_t siglen, const uint8_t *m,
                       size_t mlen, const uint8_t *pk)
{
    uint8_t seedA[SEEDBYTES];
    poly b[EM];
    uint8_t tr[CHALLENGESEEDBYTES];
    uint8_t mu[CHALLENGESEEDBYTES];
    uint8_t seedC[CHALLENGESEEDBYTES];
    uint8_t seedCp[CHALLENGESEEDBYTES];
    poly z1[Z1LEN];
    poly hint[EM];
    poly c;
    poly16 agen[EM];
    poly16 hAgen[EM * ELL];
    poly16 bhat[EM];
    poly16 Ahat[EM * ELL];
    poly comY0p[EM];
    poly comY_tilde[EM];
    poly comY_h[EM];
    poly z2p[EM];
    int j;

#if defined(SIG_RAW)
    const size_t expected_siglen = SIG_RAW_PACKED_BYTES;
#else
    const size_t expected_siglen = SIG_PACKED_BYTES;
#endif
    if (siglen != expected_siglen)
        return -2; /* length mismatch: usage error, not invalid-sig */

    /* Step 1: (seedA, b) = pkDecode(pk); Step 3: sigDecode(sig). */
    PROF_CTX(PC_OTHER);
    {
        int rc;
        PROF_START(t_up);
        rc = unpack_pk(seedA, b, pk);
        if (rc == 0) {
            /* sigDecode: any canonical-decode rejection -> -1 (invalid
             * signature, not a usage error).  Decoded here (after the pk
             * unpack) so PT_VF_UNPACK covers both decode stages. */
#if defined(SIG_RAW)
            rc = (unpack_sig_raw(seedC, z1, hint, sig) != 0) ? -1 : 0;
#else
            rc = (unpack_sig(seedC, z1, hint, sig) != 0) ? -1 : 0;
#endif
            PROF_STOP(PT_VF_UNPACK, t_up);
            if (rc != 0)
                return -1; /* sigDecode rejection */
        } else {
            PROF_STOP(PT_VF_UNPACK, t_up);
            return -2; /* pkDecode failure: usage error */
        }
    }

    /* Step 2: tr = HashPK(pk) (0x05); mu = HashMsg(tr || M) (0x06). */
    PROF_CTX(PC_SETUP);
    {
        uint8_t pkhash_in[1 + CRYPTO_PUBLICKEYBYTES];
        xof_ctx hctx;
        PROF_START(t_su);
        pkhash_in[0] = DS_HASH_PK;
        memcpy(pkhash_in + 1, pk, CRYPTO_PUBLICKEYBYTES);
        xof256_init(&hctx, pkhash_in, 1 + CRYPTO_PUBLICKEYBYTES);
        xof256_squeeze(&hctx, tr, CHALLENGESEEDBYTES);
        {
            uint8_t mustage[1 + CHALLENGESEEDBYTES + MU_STAGE_CAP];
            xof_ctx hctx2;
            if (mlen > MU_STAGE_CAP) {
                PROF_STOP(PT_VF_SETUP, t_su);
                return -4;
            }
            mustage[0] = DS_HASH_MSG;
            memcpy(mustage + 1, tr, CHALLENGESEEDBYTES);
            memcpy(mustage + 1 + CHALLENGESEEDBYTES, m, mlen);
            xof256_init(&hctx2, mustage, 1 + CHALLENGESEEDBYTES + mlen);
            xof256_squeeze(&hctx2, mu, CHALLENGESEEDBYTES);
        }
        PROF_STOP(PT_VF_SETUP, t_su);
    }

    /* Step 4: c = SampleC(seed_c) (0x07). */
    PROF_CTX(PC_CHALLENGE);
    {
        PROF_START(t_sc);
        sample_c(&c, seedC);
        PROF_STOP(PT_VF_CHALLENGE, t_sc);
    }

    /* Steps 5,7: ExpandA + A1-hat build (NO 2*I_m). */
    PROF_CTX(PC_A);
    {
        PROF_START(t_va);
        expand_a(agen, hAgen, seedA);
        build_cached_matrix(bhat, Ahat, b, agen, hAgen);
        PROF_STOP(PT_VF_A, t_va);
    }
    PROF_CTX(PC_OTHER);

    /* Step 6: w_0' = LSB(z0 - c) * j (only poly 0 carries the bit). */
    recon_comY0p(&comY0p[0], &z1[0], &c);
    for (j = 1; j < EM; ++j)
        memset(&comY0p[j], 0, sizeof comY0p[j]);

    /* Step 8a: commitment reconstruction (1+ELL columns). */
    {
        PROF_START(t_mm);
        mat_mul_z1_2q(comY_tilde, z1, &c, bhat, Ahat);
        PROF_STOP(PT_VF_MATMUL, t_mm);
    }

    /* Step 8b: (w_h, z2') = UseHint(...). */
    {
        PROF_START(t_uh);
        use_hint(comY_h, z2p, hint, comY_tilde, &comY0p[0]);
        PROF_STOP(PT_VF_HINT, t_uh);
    }

    /* Step 9: seed_c' = HashCh(EncodeCom(w_h, w_0') || mu) (0x04). */
    PROF_CTX(PC_CHALLENGE);
    {
        uint8_t encbuf[1 + ENCODECOM_VEC_BYTES + CHALLENGESEEDBYTES];
        xof_ctx hctx;
        PROF_START(t_cp);
        encbuf[0] = DS_HASH_CH;
        encode_com_vec(encbuf + 1, comY_h, comY0p);
        memcpy(encbuf + 1 + ENCODECOM_VEC_BYTES, mu, CHALLENGESEEDBYTES);
        xof256_init(&hctx, encbuf,
                    1 + ENCODECOM_VEC_BYTES + CHALLENGESEEDBYTES);
        xof256_squeeze(&hctx, seedCp, CHALLENGESEEDBYTES);
        PROF_STOP(PT_VF_CHALLENGE, t_cp);
    }
    PROF_CTX(PC_OTHER);

    /* Step 10: accept iff seed_c' == seed_c (constant-time) AND
     * ||(z1,z2')||^2 <= BV_SQ. */
    {
        int ok;
        PROF_START(t_vn);
        ok = ct_bytes_equal(seedCp, seedC, CHALLENGESEEDBYTES) &&
             response_norm_ok(z1, z2p);
        PROF_STOP(PT_VF_NORM, t_vn);
        if (!ok)
            return -1;
    }
    return 0;
}
