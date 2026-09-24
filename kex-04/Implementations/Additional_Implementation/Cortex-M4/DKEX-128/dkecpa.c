#include "parameters.h"
#include "dkecpa.h"
#include "auxfunc.h"
#include "dke_hash.h"
#include "poly.h"
#include "polyvec.h"
#include "random_sampling.h"
#include "dke_utils.h"
#include "packing.h"
#include <stdint.h>
#include <string.h>
#if defined(_MSC_VER)
#include <immintrin.h>  /* _mm_malloc / _mm_free */
#endif

void DKE_CPA_keygen_derand(uint8_t pk[DKE_PKBYTES],
                           uint8_t sk[DKE_CPA_SKABYTES],
                           const uint8_t coins[DKE_SEEDBYTES]) {
    uint8_t buffer[2 * DKE_SEEDBYTES];
    const uint8_t *seed = buffer;
    const uint8_t *rand = buffer + DKE_SEEDBYTES;
    polyvec mat[DKE_K];
    polyvec pA, eA, sA;
    unsigned int i, nonce = 0;
#if defined(DKE_USE_MATACC) && defined(DKE_USE_CORTEX_M4_PLANTARD) && (DKE_MODE != 512)
    polyvec sA_prime;                       /* matacc cache of sA*zeta */
#endif

    memcpy(buffer, coins, DKE_SEEDBYTES);
    dke_hash_g(buffer, DKE_SEEDBYTES, buffer);

#if !(defined(DKE_USE_MATACC) && defined(DKE_USE_CORTEX_M4_PLANTARD) && (DKE_MODE != 512))
    gen_a(mat, seed);                       /* matacc path generates A on the fly */
#if (defined(DKE_AVX2_NTT256_ASM) && DKE_N == 256) || (defined(DKE_NTT512_PACKED) && DKE_N == 512)
    /* permute the freshly-sampled matrix into ntt(packed) order for the asm basemul */
    { unsigned mm, nn; for (mm = 0; mm < DKE_K; mm++) for (nn = 0; nn < DKE_K; nn++)
        DKE_poly_nttunpack(&mat[mm].vec[nn]); }
#endif
#endif /* !matacc */
#if 1
#if (defined(DKE_USE_AVX2) || defined(DKE_USE_AARCH64)) && DKE_HASH == 0
    /* SM3 mode: 4x/8x parallel noise sampling via sm3x8/sm3x4 */
  #if DKE_K == 2
    DKE_getnoise_sm3x8_4x(&sA.vec[0], &sA.vec[1], &eA.vec[0], &eA.vec[1],
                            rand, 0, 1, 2, 3);
    nonce = 4;
  #elif DKE_K == 4
    DKE_getnoise_sm3x8_8x(&sA.vec[0], &sA.vec[1], &sA.vec[2], &sA.vec[3],
                            &eA.vec[0], &eA.vec[1], &eA.vec[2], &eA.vec[3],
                            rand, 0, 1, 2, 3, 4, 5, 6, 7);
    nonce = 8;
  #else
    for (i = 0; i < DKE_K; i++) DKE_getsecretA(&sA.vec[i], rand, nonce++);
    for (i = 0; i < DKE_K; i++) DKE_geterrorA(&eA.vec[i], rand, nonce++);
  #endif
#elif defined(DKE_USE_OPT_C) && (DKE_HASH == 1 || DKE_HASH == 2)
    /* 4x interleaved noise sampling */
  #if DKE_K == 2
    DKE_getnoise_4x(&sA.vec[0], &sA.vec[1], &eA.vec[0], &eA.vec[1],
                     rand, 0, 1, 2, 3);
    nonce = 4;
  #elif DKE_K == 4
    DKE_getnoise_4x(&sA.vec[0], &sA.vec[1], &sA.vec[2], &sA.vec[3],
                     rand, 0, 1, 2, 3);
    DKE_getnoise_4x(&eA.vec[0], &eA.vec[1], &eA.vec[2], &eA.vec[3],
                     rand, 4, 5, 6, 7);
    nonce = 8;
  #else
    for (i = 0; i < DKE_K; i++) DKE_getsecretA(&sA.vec[i], rand, nonce++);
    for (i = 0; i < DKE_K; i++) DKE_geterrorA(&eA.vec[i], rand, nonce++);
  #endif
#else
    for (i = 0; i < DKE_K; i++) DKE_getsecretA(&sA.vec[i], rand, nonce++);
    for (i = 0; i < DKE_K; i++) DKE_geterrorA(&eA.vec[i], rand, nonce++);
#endif

    DKE_polyvec_ntt(&sA);
    DKE_polyvec_ntt(&eA);

#if defined(DKE_USE_MATACC) && defined(DKE_USE_CORTEX_M4_PLANTARD) && (DKE_MODE != 512)
    /* pA[i] = A[i]·sA, A generated on the fly (fused). tomont == fromplant here. */
    DKE_matacc_cache32(&pA.vec[0], &sA, &sA_prime, 0, seed, 0);
    DKE_poly_tomont(&pA.vec[0]);
    for (i = 1; i < DKE_K; i++) {
        DKE_matacc_opt32(&pA.vec[i], &sA, &sA_prime, i, seed, 0);
        DKE_poly_tomont(&pA.vec[i]);
    }
#else
    for (i = 0; i < DKE_K; i++) {
        DKE_polyvec_basemul_acc_montgomery(&pA.vec[i], &mat[i], &sA);
        DKE_poly_tomont(&pA.vec[i]);
    }
#endif

    DKE_polyvec_add(&pA, &pA, &eA);
    DKE_polyvec_reduce(&pA);

    DKE_packpk(pk, &pA, seed);
    DKE_CPA_packsk(sk, &sA);
#endif
}

void DKE_CPA_enc_derand(uint8_t ct[DKE_CPA_CTBYTES],
                        uint8_t ss[DKE_SSBYTES],
                        const uint8_t pk[DKE_PKBYTES],
                        const uint8_t coins[DKE_SEEDBYTES + DKE_N/8]) {
    uint8_t seed[DKE_SEEDBYTES];
    uint8_t sig[DKE_SIGNALBYTES];
    polyvec matt[DKE_K];
    polyvec pA, pB, sB, eB;
    poly kB, e;
    unsigned int i, nonce = 0;
#if defined(DKE_USE_MATACC) && defined(DKE_USE_CORTEX_M4_PLANTARD) && (DKE_MODE != 512)
    polyvec sB_prime;                       /* matacc cache of sB*zeta */
    (void)matt;
#endif

    DKE_unpackpk(&pA, seed, pk);

#if defined(DKE_USE_AVX2) && (DKE_HASH == 1 || DKE_HASH == 2) && DKE_K == 2
    /* Overlap e-poly absorb with gen_at (~3500 cycles):
     * absorb(nonce=4) before gen_at, squeeze+AVX2_CBD after getnoise_4x.
     * The absorb cost (~200 cycles) is fully hidden inside gen_at. */
    {
        keccak_state st_e;
        DKE_getnoise_absorb(&st_e, coins, 4);         /* absorb e early */

        gen_at(matt, seed);                           /* ~3500 cycles */
#if (defined(DKE_AVX2_NTT256_ASM) && DKE_N == 256) || (defined(DKE_NTT512_PACKED) && DKE_N == 512)
        { unsigned mm, nn; for (mm = 0; mm < DKE_K; mm++) for (nn = 0; nn < DKE_K; nn++)
            DKE_poly_nttunpack(&matt[mm].vec[nn]); }
#endif

        DKE_getnoise_4x(&sB.vec[0], &sB.vec[1], &eB.vec[0], &eB.vec[1],
                         coins, 0, 1, 2, 3);
        nonce = 5;

        DKE_getnoise_squeeze(&e, &st_e);              /* squeeze + AVX2 CBD */
    }
#else
#if !(defined(DKE_USE_MATACC) && defined(DKE_USE_CORTEX_M4_PLANTARD) && (DKE_MODE != 512))
    gen_at(matt, seed);                     /* matacc path generates A^T on the fly */
#if (defined(DKE_AVX2_NTT256_ASM) && DKE_N == 256) || (defined(DKE_NTT512_PACKED) && DKE_N == 512)
    { unsigned mm, nn; for (mm = 0; mm < DKE_K; mm++) for (nn = 0; nn < DKE_K; nn++)
        DKE_poly_nttunpack(&matt[mm].vec[nn]); }
#endif
#endif /* !matacc */

  #if (defined(DKE_USE_AVX2) || defined(DKE_USE_AARCH64)) && DKE_HASH == 0
    /* SM3 mode: 4x/8x parallel noise sampling via sm3x8/sm3x4 */
    #if DKE_K == 2
    DKE_getnoise_sm3x8_4x(&sB.vec[0], &sB.vec[1], &eB.vec[0], &eB.vec[1],
                            coins, 0, 1, 2, 3);
    nonce = 4;
    #elif DKE_K == 4
    DKE_getnoise_sm3x8_8x(&sB.vec[0], &sB.vec[1], &sB.vec[2], &sB.vec[3],
                            &eB.vec[0], &eB.vec[1], &eB.vec[2], &eB.vec[3],
                            coins, 0, 1, 2, 3, 4, 5, 6, 7);
    nonce = 8;
    #else
    for (i = 0; i < DKE_K; i++) DKE_getsecretB(sB.vec + i, coins, nonce++);
    for (i = 0; i < DKE_K; i++) DKE_geterrorB(eB.vec + i, coins, nonce++);
    #endif
  #elif defined(DKE_USE_OPT_C) && (DKE_HASH == 1 || DKE_HASH == 2)
    #if DKE_K == 2
    DKE_getnoise_4x(&sB.vec[0], &sB.vec[1], &eB.vec[0], &eB.vec[1],
                     coins, 0, 1, 2, 3);
    nonce = 4;
    #elif DKE_K == 4
    DKE_getnoise_4x(&sB.vec[0], &sB.vec[1], &sB.vec[2], &sB.vec[3],
                     coins, 0, 1, 2, 3);
    DKE_getnoise_4x(&eB.vec[0], &eB.vec[1], &eB.vec[2], &eB.vec[3],
                     coins, 4, 5, 6, 7);
    nonce = 8;
    #else
    for (i = 0; i < DKE_K; i++) DKE_getsecretB(sB.vec + i, coins, nonce++);
    for (i = 0; i < DKE_K; i++) DKE_geterrorB(eB.vec + i, coins, nonce++);
    #endif
  #else
    for (i = 0; i < DKE_K; i++) DKE_getsecretB(sB.vec + i, coins, nonce++);
    for (i = 0; i < DKE_K; i++) DKE_geterrorB(eB.vec + i, coins, nonce++);
  #endif
#endif /* AVX2 && HASH==1 && K==2 fast path */

    DKE_polyvec_ntt(&sB);

#if defined(DKE_USE_MATACC) && defined(DKE_USE_CORTEX_M4_PLANTARD) && (DKE_MODE != 512)
    /* pB[i] = A^T[i]·sB, A^T generated on the fly (fused, transposed=1). */
    DKE_matacc_cache32(&pB.vec[0], &sB, &sB_prime, 0, seed, 1);
    for (i = 1; i < DKE_K; i++)
        DKE_matacc_opt32(&pB.vec[i], &sB, &sB_prime, i, seed, 1);
#else
    for (i = 0; i < DKE_K; i++)
        DKE_polyvec_basemul_acc_montgomery(&pB.vec[i], &matt[i], &sB);
#endif

    DKE_polyvec_invntt_tomont(&pB);
    DKE_polyvec_add(&pB, &pB, &eB);
    DKE_polyvec_reduce(&pB);

    DKE_polyvec_basemul_acc_montgomery(&kB, &pA, &sB);
    DKE_poly_invntt_tomont(&kB);
#if !(defined(DKE_USE_AVX2) && (DKE_HASH == 1 || DKE_HASH == 2) && DKE_K == 2)
    DKE_geterrorA(&e, coins, nonce++);
#endif
    DKE_poly_add(&kB, &kB, &e);
    DKE_poly_scale2(&kB);
    DKE_poly_reduce(&kB);

    DKE_signal(sig, &kB, coins + DKE_SEEDBYTES);
    DKE_CPA_packciphertext(ct, &pB, sig);
    DKE_derive_ss(ss, &kB, sig);
}

void DKE_CPA_dec(uint8_t ss[DKE_SSBYTES],
                 const uint8_t sk[DKE_CPA_SKABYTES],
                 const uint8_t ct[DKE_CPA_CTBYTES]) {
    polyvec sA, pB;
    uint8_t sig[DKE_SIGNALBYTES];
    poly kA;

    DKE_CPA_unpacksk(&sA, sk);
    DKE_CPA_unpackciphertext(&pB, sig, ct);

    DKE_polyvec_ntt(&pB);
    DKE_polyvec_basemul_acc_montgomery(&kA, &sA, &pB);
    DKE_poly_invntt_tomont(&kA);
    DKE_poly_scale2(&kA);
    DKE_poly_reduce(&kA);

    DKE_derive_ss(ss, &kA, sig);
}
