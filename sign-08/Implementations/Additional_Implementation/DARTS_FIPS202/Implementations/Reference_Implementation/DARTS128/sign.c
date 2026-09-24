#include "sign.h"
#include "packing.h"
#include "params.h"
#include "poly.h"
#include "polyfix.h"
#include "polymat.h"
#include "polyvec.h"
#include "randombytes.h"
#include "symmetric.h"
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/*************************************************
 * Name:        crypto_sign_keypair
 *
 * Description: Generates public and private key.
 *
 * Arguments:   - uint8_t *pk: pointer to output public key (allocated
 *                             array of CRYPTO_PUBLICKEYBYTES bytes)
 *              - uint8_t *sk: pointer to output private key (allocated
 *                             array of CRYPTO_SECRETKEYBYTES bytes)
 *
 * Returns 0 (success)
 **************************************************/
int crypto_sign_keypair(uint8_t *pk, uint8_t *sk) {
    uint8_t seedbuf[2 * SEEDBYTES + CRHBYTES] = {0};
    uint16_t counter = 0;
    const uint8_t *seed_A1, *seed_s, *key;

    polyvecl_1 A1[K];      // 多项式矩阵 A1，维度 k × (l-1)
    poly s0, neg_s0_inv_hat;   // 私钥多项式向量 s0 和其逆的 NTT 表示 
    polyvecl_1 s1;         // 私钥多项式向量 s1
    polyveck A0, e;        // 私钥多项式向量 e
    polyveck half_q_beta;  // 多项式向量 ( (Q+1)/2, 0, ..., 0 )

    poly neg_s0_hat;        // NTT 表示 -s0
    polyvecl_1 s1hat;  // NTT 表示 s1
    polyveck ehat;     // NTT 表示 e
    polyveck half_q_beta_hat; // NTT 表示 ( (Q+1)/2, 0, ..., 0 )  

    xof256_state state;
    // 随机生成种子 seedbuf
    randombytes(seedbuf, SEEDBYTES);

    // 通过 SHAKE256(seedbuf) 生成 seed_A1, seed_s, key
    xof256_absorbe_once(&state, seedbuf, SEEDBYTES);
    xof256_squeeze(seedbuf, 2 * SEEDBYTES + CRHBYTES, &state); 
    
    seed_A1 = seedbuf;
    seed_s  = seed_A1 + SEEDBYTES;
    key     = seed_s + CRHBYTES;

    polyveck_halfq_beta(&half_q_beta); // 生成多项式向量 ( (Q+1)/2, 0, ..., 0 )
    half_q_beta_hat = half_q_beta;
    polyveck_ntt(&half_q_beta_hat);

    polymatkl_1_expand(A1, seed_A1); // 生成多项式矩阵 A1 (直接生成的 NTT 表示)

reject:
    polyveckl_ternary_p(&s0, &s1, &e, seed_s, counter); // 生成私钥 s = (s0, s1, e)
    neg_s0_hat = s0;
    poly_neg(&neg_s0_hat); // 计算 -s0
    poly_ntt(&neg_s0_hat); // 计算 -s0 的 NTT 表示
    
    counter += L + K; // 计数器增加，防止重复采样

    int result = poly_baseinv(&neg_s0_inv_hat, &neg_s0_hat); // result = 0 表示 s0 可逆，否则不可逆 
    int64_t squared_singular_value = polyveckl_sqsing_value(&s0, &s1, &e);
    if (result != 0 || squared_singular_value >= GAMMA * GAMMA * N) {
        goto reject;
    } 
    
    s1hat = s1, ehat = e; // NTT 表示
    
    polyvecl_1_ntt(&s1hat);
    polyveck_ntt(&ehat);

    // 计算公钥 A0 = -s0^{-1} * (A1 * s1 + e - (Q+1)/2 * beta) 
    polymatkl_1_pointwise_montgomery(&A0, A1, &s1hat);     // A0 = A1 * s1
    polyveck_frommont(&A0);                                // 转换为普通 NTT

    polyveck_add(&A0, &A0, &ehat);                         // A0 = A1 * s1 + e
    polyveck_sub(&A0, &A0, &half_q_beta_hat);              // A0 = A1 * s1 + e - (Q+1)/2 * beta
    
    polyveck tmp;
    tmp = A0;
    polymatk1_pointwise_montgomery(&A0, &tmp, &neg_s0_inv_hat);      // A0 = -s0^{-1} * (A1 * s1 + e - (Q+1)/2 * beta)
    
    polyveck_frommont(&A0); // 转换为不带 MONT 的 NTT 表示
    
    // 打包公钥和私钥
    pack_pk(pk, &A0, seed_A1);
    pack_sk(sk, pk, &s0, &s1, &e, key);

    return 0;
} 

/*************************************************
 * Name:        crypto_sign_signature
 *
 * Description: Computes signature.
 *
 * Arguments:   - uint8_t *sig:   pointer to output signature (of length
 *                                CRYPTO_SIGNATUREBYTES)
 *              - size_t *siglen: pointer to output length of signature
 *              - uint8_t *m:     pointer to message to be signed
 *              - size_t mlen:    length of message
 *              - uint8_t *sk:    pointer to bit-packed secret key
 *
 * Returns 0 (success)
 **************************************************/
int crypto_sign_signature(uint8_t *sig, size_t *siglen, const uint8_t *m, size_t mlen, const uint8_t *sk) {
    
    uint8_t buf[POLYVECK_HIGHBITS_PACKEDBYTES] = {0}; // 用于存储 w 的高位部分
    uint8_t seedbuf[CRHBYTES] = {0}; // 用于采样随机向量 y1, y2 的种子
    uint8_t key[SEEDBYTES] = {0};    // s = (pk, s, K) 中的 K
    uint8_t mu[CRHBYTES] = {0};      // μ = H(pk, m)  

    uint8_t b; // 1 字节
    uint16_t counter = 0;

    uint64_t reject1, reject2, reject3; 
    // reject1, 2, 3 用于拒绝采样

    poly s0; polyvecl_1 s1; polyveck e; // s = (s0, s1, e)

    polyvecl A[K];
    polyveck w; // w = (A0 | A1 | I_k) * round(y)
    polyveck h, htmp;

    polyfixvecl y1, z1, z1tmp;
    polyfixveck y2, z2, z2tmp;  
    polyvecl z1rnd; // round
    polyveck z2rnd; // round
    polyvecl highbits_z1, lowbits_z1;
    
    poly c, c_hat;  // challenge 多项式及其 NTT 表示
    poly c_1_b;     // (1-b)c
    poly w0, w0_highbits;       // w.vec[0] 及其高位部分
    poly w0_cb, w0_cb_highbits; // w.vec[0] + (1-b)c 及其高位部分

    polyvecl cs;  // (c * s0, c * s1)
    polyveck ce;  // c * e

    xof256_state state;

    unsigned int i;

    // unpack sk = (pk, s = (s0, s1, e), key)
    unpack_sk(A, &s0, &s1, &e, key, sk);

    xof256_absorbe_twice(&state, sk, CRYPTO_PUBLICKEYBYTES, m, mlen);
    xof256_squeeze(mu, CRHBYTES, &state);
    xof256_absorbe_twice(&state, key, SEEDBYTES, mu, CRHBYTES);
    xof256_squeeze(seedbuf, CRHBYTES, &state);

    // 将 s = (s0, s1, e) 转换为 NTT 表示
    poly_ntt(&s0);
    polyvecl_1_ntt(&s1);
    polyveck_ntt(&e);

reject:
    // 采样随机向量 y = (y1, y2)
    counter = polyfixveclk_sample_hyperball(&y1, &y2, &b, seedbuf, counter);
    
    polyfixvecl_round(&z1rnd, &y1); // round
    polyfixveck_round(&z2rnd, &y2); // round

    /*--------------------- Compute w -------------------------*/
    polyvecl_ntt(&z1rnd);
    polymatkl_pointwise_montgomery(&w, A, &z1rnd);
    polyveck_invntt_tomont(&w);
    polyveck_add(&w, &w, &z2rnd); // w = A * y1 + y2 的标准表示
    polyveck_freeze(&w); // make coefficients in [0, Q)
    
    /*------------------ Generate chanllenge c -----------------*/
    polyveck_pack_highbits(buf, &w);
    poly_challenge(&c, buf, mu);

    /*--------------------- Compress Check ---------------------*/
    w0 = w.vec[0];
    poly_mul_1_b(&c_1_b, &c, b & 0x1); 
    poly_add(&w0_cb, &w0, &c_1_b); // w0_cb = w0 + (1-b)c

    poly_freeze(&w0);     // make coefficients in [0, Q)
    poly_freeze(&w0_cb);  // make coefficients in [0, Q)

    poly_compress(&w0_highbits, &w0);
    poly_compress(&w0_cb_highbits, &w0_cb);

    int compress_reject = poly_equal(&w0_highbits, &w0_cb_highbits);
    if (compress_reject == 0) {
        goto reject;
    }

    /*-------------------- Compute (cs, ce) --------------------*/
    // 生成 (cs, ce)
    c_hat = c;
    poly_ntt(&c_hat);
    poly_basemul_montgomery(&cs.vec[0], &c_hat, &s0);
    
    for (i = 0; i < L - 1; ++i)
        poly_basemul_montgomery(&cs.vec[i + 1], &c_hat, &s1.vec[i]);
    for (i = 0; i < K; ++i)
        poly_basemul_montgomery(&ce.vec[i], &c_hat, &e.vec[i]);

    polyvecl_invntt_tomont(&cs);              
    polyveck_invntt_tomont(&ce);                      

    /*------------ Compute z = y + (-1)^b * (c * s) ---------------*/
    polyvecl_cneg(&cs, b & 0x1);
    polyveck_cneg(&ce, b & 0x1);
    polyfixvecl_add(&z1, &y1, &cs); // z1 = y1 + (-1)^b * (c * s1)
    polyfixveck_add(&z2, &y2, &ce); // z2 = y2 + (-1)^b * (c * e)

    /*-------------------- Check z1, z2 --------------------*/
    // reject1 = ( |z| >= B1 ) （不在中球内）
    reject1 = ((uint64_t)B1SQ * LN * LN - polyfixveclk_sqnorm2(&z1, &z2)) >> 63;
    reject1 &= 1;
    
    polyfixvecl_double(&z1tmp, &z1); // z1tmp = 2 * z1
    polyfixveck_double(&z2tmp, &z2); // z2tmp = 2 * z2
    polyfixfixvecl_sub(&z1tmp, &z1tmp, &y1); // z1tmp = 2z1 - y1
    polyfixfixveck_sub(&z2tmp, &z2tmp, &y2); // z2tmp = 2z2 - y2

    // reject2 = ( |2z-y| < B ) （在中球内）
    reject2 = (polyfixveclk_sqnorm2(&z1tmp, &z2tmp) - BSQ * LN * LN) >> 63;
    reject2 &= 1;
    // reject3 = ( |z| > B0 ) （不在小球内）
    reject3 = ((uint64_t)B0SQ * LN * LN - polyfixveclk_sqnorm2(&z1, &z2)) >> 63;
    reject3 &= 1;

    // Rejection: reject1 or (reject2 and reject3 and b=0)
    if (reject1 || (reject2 && reject3 && ((b & 0x2) == 0))) {
        goto reject;
    }
    
    /*-------------------- Prepare signature --------------------*/
    polyfixvecl_round(&z1rnd, &z1);
    polyfixveck_round(&z2rnd, &z2);

    polyvecl_freeze(&z1rnd); // make coefficients in [0, Q)
    polyveck_freeze(&z2rnd); // make coefficients in [0, Q)
    
    polyvecl_lowbits(&lowbits_z1, &z1rnd);   // low bits
    polyvecl_compress(&highbits_z1, &z1rnd); // high bits

    // Compute h = Compress(w, d) - Compress(w - round(z2) + (1-b)cβ, d)
    polyveck_compress(&htmp, &w); // htmp = Compress(w, d)
    polyveck_sub(&w, &w, &z2rnd); // w = w - round(z2)---这里改变了w
    poly_add(&w.vec[0], &w.vec[0], &c_1_b); // w.vec[0] += (1-b)c

    polyveck_freeze(&w);          // make coefficients in [0, Q)

    polyveck_compress(&h, &w);    // h = Compress(w - round(z2) + (1-b)cβ, d)
    polyveck_sub(&h, &htmp, &h);  // h = Compress(w, d) - Compress(w - round(z2) + (1-b)cβ, d)

    // 打包签名 sig = (c, lowbits_z1, highbits_z1, h)
    if (pack_sig(sig, &c, &lowbits_z1, &highbits_z1, &h)) {
        goto reject;
    }

    *siglen = CRYPTO_SIGNATUREBYTES;

    return 0;
} 

/*************************************************
 * Name:        crypto_sign
 *
 * Description: Compute signed message.
 *
 * Arguments:   - uint8_t *sm: pointer to output signed message (allocated
 *                             array with CRYPTO_SIGNATUREBYTES + mlen bytes),
 *                             can be equal to m
 *              - size_t *smlen: pointer to output length of signed
 *                               message
 *              - const uint8_t *m: pointer to message to be signed
 *              - size_t mlen: length of message
 *              - const uint8_t *sk: pointer to bit-packed secret key
 *
 * Returns 0 (success)
 **************************************************/
int crypto_sign_sign(uint8_t *sm, size_t *smlen, const uint8_t *m, size_t mlen, const uint8_t *sk) {

    size_t i;

    for (i = 0; i < mlen; ++i)
        sm[CRYPTO_SIGNATUREBYTES + mlen - 1 - i] = m[mlen - 1 - i];
    crypto_sign_signature(sm, smlen, sm + CRYPTO_SIGNATUREBYTES, mlen, sk);
    *smlen += mlen;
    return 0;
}


/*************************************************
 * Name:        crypto_sign_verify
 *
 * Description: Verifies signed message.
 *
 * Arguments:   - const uint8_t *sig: pointer to signature
 *              - size_t siglen: length of signature
 *              - const uint8_t *m: pointer to message
 *              - size_t mlen: length of message
 *              - const uint8_t *pk: pointer to bit-packed public key
 *
 * Returns 0 if signature is valid, -1 otherwise
 **************************************************/
int crypto_sign_verify(const uint8_t *sig, size_t siglen, const uint8_t *m, size_t mlen, const uint8_t *pk) {

    unsigned int i;
    uint8_t buf[POLYVECK_HIGHBITS_PACKEDBYTES] = {0}; 
    uint8_t seedA1[SEEDBYTES] = {0}, mu[CRHBYTES] = {0};
    uint64_t sqnorm2;
    polyvecl A[K], z1, lowbits_z1, highbits_z1;
    polyveck A0, h, w1, Az1_cb, z2, Az1_cb_compressed;

    poly c, c1, c_half_q;
    
    xof256_state state;

    // Check signature length
    if (siglen != CRYPTO_SIGNATUREBYTES) {
        return -1;
    }

    // Unpack public key: pk = (seedA1, A0)
    unpack_pk(&A0, seedA1, pk);

    // Unpack signature: sig = (c, lowbits_z1, highbits_z1, h) 
    if (unpack_sig(&c, &lowbits_z1, &highbits_z1, &h, sig)) {
        return -1;
    }

    polyvecl_compose(&z1, &lowbits_z1, &highbits_z1); // 恢复 z1

    // Recover A = (A0 | A1)
    polymatkl_expand(A, seedA1);
    for (i = 0; i < K; ++i) {
        A[i].vec[0] = A0.vec[i];
    }

    // sqnorm2 = ||z1||^2
    sqnorm2 = polyvecl_sqnorm2(&z1); 

    // 计算 w = h + Compress(A * z1 + (q+1)/2 * c * beta, d)
    polyvecl_ntt(&z1);

    poly_mul_halfq(&c_half_q, &c); // c_half_q = (Q+1)/2 * c
    
    polymatkl_pointwise_montgomery(&Az1_cb, A, &z1);  // w = A * z1 (NTT)
    polyveck_invntt_tomont(&Az1_cb); // A * z1 的标准表示

    poly_add(&Az1_cb.vec[0], &Az1_cb.vec[0], &c_half_q); // w.vec[0] += c * (Q+1)/2 * beta
        
    polyveck_freeze(&Az1_cb); // make coefficients in [0, Q)
    
    polyveck_compress(&Az1_cb_compressed, &Az1_cb); // Az1_cb_compressed = Compress(A * z1 + (Q+1)/2 * c * beta, d)
    polyveck_add(&w1, &Az1_cb_compressed, &h); //  w = h + Compress(A * z1 + (Q+1)/2 * c * beta, d)
    
    xof256_absorbe_twice(&state, pk, CRYPTO_PUBLICKEYBYTES, m, mlen);
    xof256_squeeze(mu, CRHBYTES, &state);
    polyveck_pack_compressed(buf, &w1);  // Pack the compressed value (no further compression)
    poly_challenge(&c1, buf, mu);        // Regenerate challenge polynomial c1

    // Check if c = c1
    for (i = 0; i < N; ++i) {
        if (c.coeffs[i] != c1.coeffs[i]) {
            return -1;
        }
    }

    // 计算 z2 = Decompress(w_compressed, d) - Az1_cb 并检查范数
    polyveck_decompress(&z2, &w1); // z2 = Decompress(w1, d)
    polyveck_sub(&z2, &z2, &Az1_cb); // z2 = Decompress(w_compressed, d) - (A * z1 + (Q+1)/2 * c * beta)

    // 检查 z = (z1, z2) 是否符合范围
    if (sqnorm2 + polyveck_sqnorm2(&z2) >= B11SQ * LN * LN) {
        return -1;
    }

    return 0;
}


/*************************************************
 * Name:        crypto_sign_open
 *
 * Description: Verify signed message.
 *
 * Arguments:   - uint8_t *m: pointer to output message (allocated
 *                            array with smlen bytes), can be equal to sm
 *              - size_t *mlen: pointer to output length of message
 *              - const uint8_t *sm: pointer to signed message
 *              - size_t smlen: length of signed message
 *              - const uint8_t *pk: pointer to bit-packed public key
 *
 * Returns 0 if signed message could be verified correctly and -1 otherwise
 **************************************************/
int crypto_sign_open(uint8_t *m, size_t *mlen, const uint8_t *sm, size_t smlen,
                     const uint8_t *pk) {
    size_t i;

    if (smlen < CRYPTO_SIGNATUREBYTES)
        goto badsig;

    *mlen = smlen - CRYPTO_SIGNATUREBYTES;
    if (crypto_sign_verify(sm, CRYPTO_SIGNATUREBYTES, sm + CRYPTO_SIGNATUREBYTES, *mlen, pk))
        goto badsig;
    else {
        /* All good, copy msg, return 0 */
        for (i = 0; i < *mlen; ++i)
            m[i] = sm[CRYPTO_SIGNATUREBYTES + i];
        return 0;
    }

badsig:
    /* Signature verification failed */
    *mlen = -1;
    for (i = 0; i < smlen; ++i)
        m[i] = 0;

    return -1;
}