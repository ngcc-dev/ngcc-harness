#include "packing.h"
#include "encoding.h"
#include "params.h"
#include "poly.h"
#include "polymat.h"
#include "polyvec.h"
#include <string.h>


/*************************************************
 * Name:        pack_pk
 *
 * Description: Bit-pack public key pk = (seedA1, A0).
 *
 * Arguments:   - uint8_t pk[]: output byte array
 *              - const polyveck *b: polynomial vector of length K containg A0 (NTT form)
 *              - const uint8_t seedA1[]: seed for A1
 **************************************************/
void pack_pk(uint8_t pk[CRYPTO_PUBLICKEYBYTES], polyveck *A0, const uint8_t seedA1[SEEDBYTES]) {
    unsigned int i;
    
    memcpy(pk, seedA1, SEEDBYTES);

    pk += SEEDBYTES;
    for (i = 0; i < K; ++i) {
        poly_q_pack(pk + i * POLY_Q_PACKEDBYTES, &A0->vec[i]);
    }
}

/*************************************************
 * Name:        unpack_pk
 *
 * Description: Unpack public key pk = (seedA1, A0).
 *
 * Arguments:   - uint8_t seedA1[]: seed for A1
 *              - polyveck *A0: polynomial vector of length K containg A0 (NTT form)
 *              - const uint8_t pk[]: output byte array
 **************************************************/
void unpack_pk(polyveck *A0, uint8_t seedA1[SEEDBYTES], const uint8_t pk[CRYPTO_PUBLICKEYBYTES]) {
    unsigned int i;

    memcpy(seedA1, pk, SEEDBYTES);

    pk += SEEDBYTES;
    for (i = 0; i < K; ++i) {
        poly_q_unpack(&A0->vec[i], pk + i * POLY_Q_PACKEDBYTES);
    }
}

/*************************************************
 * Name:        pack_sk
 *
 * Description: Bit-pack secret key sk = (pk, s = (s0, s1, e)).
 *
 * Arguments:   - uint8_t sk[]: output byte array
 *              - const uint8_t pk[PUBLICKEYBYTES]: packed pk
 *              - const polyvecl *s0: polyvecl pointer containing s0 (encoding
 *starting at offset 1)
 *              - const polyveck *s1: polyveck pointer containing s1
 **************************************************/
void pack_sk(uint8_t sk[CRYPTO_SECRETKEYBYTES], const uint8_t pk[CRYPTO_PUBLICKEYBYTES], const poly *s0, const polyvecl_1 *s1, const polyveck *e, const uint8_t key[SEEDBYTES]) {
    unsigned int i;

    // Pack pk
    memcpy(sk, pk, CRYPTO_PUBLICKEYBYTES);
    sk += CRYPTO_PUBLICKEYBYTES;

    // Pack s0
    poly_p_pack(sk, s0);
    sk += POLY_S_PACKEDBYTES;

    // Pack s1
    for (i = 0; i < L - 1; ++i) {
        poly_p_pack(sk + i * POLY_S_PACKEDBYTES, &s1->vec[i]);
    }
    sk += (L - 1) * POLY_S_PACKEDBYTES;

    // Pack e
    for (i = 0; i < K; ++i) {
        poly_p_pack(sk + i * POLY_S_PACKEDBYTES, &e->vec[i]);
    }
    sk += K * POLY_S_PACKEDBYTES;

    // Pack key
    memcpy(sk, key, SEEDBYTES);

}

/*************************************************
 * Name:        unpack_sk
 *
 * Description: Unpack secret key sk = (pk, s = (s0, s1, e)).
 *
 * Arguments:   - polyvecl A[K]: A = (A0 | A1) 
 *              - polyvecl *s: polyvecl pointer containing s1
 *              - polyveck *e: polyveck pointer containing e
 *              - uint8_t *key: byte array to store key
 *              - const uint8_t sk[]: input byte array
 **************************************************/
void unpack_sk(polyvecl A[K], poly *s0, polyvecl_1 *s, polyveck *e, uint8_t *key, const uint8_t sk[CRYPTO_SECRETKEYBYTES]) {

    unsigned int i;
    uint8_t seedA1[SEEDBYTES];
    polyveck A0;
    unpack_pk(&A0, seedA1, sk); // 恢复 A0 和 seedA1
    sk += CRYPTO_PUBLICKEYBYTES;

    poly_p_unpack(s0, sk); // 恢复 s0
    sk += POLY_S_PACKEDBYTES;

    // 恢复 s1
    for (i = 0; i < L - 1; ++i) {
        poly_p_unpack(&s->vec[i], sk + i * POLY_S_PACKEDBYTES);
    }
    sk += (L - 1) * POLY_S_PACKEDBYTES;

    // 恢复 e
    for (i = 0; i < K; ++i) {
        poly_p_unpack(&e->vec[i], sk + i * POLY_S_PACKEDBYTES);
    }
    sk += K * POLY_S_PACKEDBYTES;

    memcpy(key, sk, SEEDBYTES);

    // 恢复 A1 并组合成 A = (A0 | A1)
    polymatkl_expand(A, seedA1);
    for (i = 0; i < K; ++i) {
        A[i].vec[0] = A0.vec[i];
    }
}

/*************************************************
 * Name:        pack_sig
 *
 * Description: Bit-pack signature sig = (c, LB(z1), len(x), len(y), x = Enc(HB(z1)), y = Enc(h)), Zeropadding.
 *
 * Arguments:   - uint8_t sig[]: output byte array
 *              - const poly *c: pointer to challenge polynomial
 *              - const polyvecl *lowbits_z1: pointer to vector LowBits(z1) of length L
 *              - const polyvecl *highbits_z1: pointer to vector HighBits(z1) of length L
 *              - const polyveck *h: pointer to vector h of length K
 * 
 * Returns 1 in case the signature packing failed; otherwise 0.
 **************************************************/
int pack_sig(uint8_t sig[CRYPTO_SIGNATUREBYTES], const poly *c, const polyvecl *lowbits_z1, const polyvecl *highbits_z1, const polyveck *h) {
    
    uint8_t encoded_h[N * K];
    uint8_t encoded_hb_z1[N * L];
    uint16_t size_enc_h, size_enc_hb_z1;
    uint8_t offset_enc_h, offset_enc_hb_z1;

    // init/padding with zeros
    memset(sig, 0, CRYPTO_SIGNATUREBYTES);

    // 编码 challenge 多项式 c
    for (size_t i = 0; i < N; i++) {
      sig[i/8] |= c->coeffs[i] << (i%8);
    }
    sig += N / 8;

    // 编码 LowBits(z1)
    polyvecl_pack_lowbits(sig, lowbits_z1);
    sig += L * POLY_LOWBITS_PACKEDBYTES;

    // 编码 HighBits(z1) 和 h
    size_enc_hb_z1 = encode_hb_z1(encoded_hb_z1, &highbits_z1->vec[0].coeffs[0]);
    size_enc_h = encode_h(encoded_h, &h->vec[0].coeffs[0]);

    if (size_enc_hb_z1 == 0 || size_enc_h == 0) {
        return 1; // 编码失败
    }
    
    // h 和 HighBits(z1) 的大小无法用1个字节表示，使用偏移量存储
    if (size_enc_h < BASE_ENC_H || 
        size_enc_h > 255 + BASE_ENC_H ||
        size_enc_hb_z1 < BASE_ENC_HB_Z1 || 
        size_enc_hb_z1 > 255 + BASE_ENC_HB_Z1) {
        return 1; // 编码尺寸超出范围
    }

    offset_enc_h = size_enc_h - BASE_ENC_H;
    offset_enc_hb_z1 = size_enc_hb_z1 - BASE_ENC_HB_Z1;

    if (POLY_C_PACKEDBYTES + POLY_LOWBITS_PACKEDBYTES * L + 2 + size_enc_h + size_enc_hb_z1 > CRYPTO_SIGNATUREBYTES ) {
        return 1; // 签名尺寸超出范围
    }

    // 编码 Encode(HighBits(z1)) 和 Encode(h) 的长度
    sig[0] = offset_enc_hb_z1;
    sig[1] = offset_enc_h;
    sig += 2;

    memcpy(sig, encoded_hb_z1, size_enc_hb_z1);
    sig += size_enc_hb_z1;

    memcpy(sig, encoded_h, size_enc_h);
    sig += size_enc_h;

    return 0;
}

/*************************************************
 * Name:        unpack_sig
 *
 * Description: Unpack signature sig = (c, LB(z1), len(x), len(y), x =
 *Enc(HB(z1)), y = Enc(h)), Zeropadding.
 *
 * Arguments:   - poly *c: pointer to challenge polynomial
 *              - polyvecl *lowbits_z1: pointer to output vector LowBits(z1)
 *              - polyvecl *highbits_z1: pointer to output vector HighBits(z1)
 *              - polyveck *h: pointer to output vector h
 *              - const uint8_t sig[]: byte array containing
 *                bit-packed signature
 *
 * Returns 1 in case of malformed signature; otherwise 0.
 **************************************************/
int unpack_sig(poly *c, polyvecl *lowbits_z1, polyvecl *highbits_z1, polyveck *h, const uint8_t sig[CRYPTO_SIGNATUREBYTES]) {
    
    unsigned int i;
    uint16_t size_enc_hb_z1, size_enc_h;

    // 解码 challenge 多项式 c
    for (i = 0; i < N; i++) {
      c->coeffs[i] = (sig[i/8] >> (i%8)) & 1;
    }
    sig += N / 8;

    // 解码 LowBits(z1)
    polyvecl_unpack_lowbits(lowbits_z1, sig);
    sig += L * POLY_LOWBITS_PACKEDBYTES;

    size_enc_hb_z1 = sig[0] + BASE_ENC_HB_Z1;
    size_enc_h = sig[1] + BASE_ENC_H;
    sig += 2;

    if (POLY_C_PACKEDBYTES + POLY_LOWBITS_PACKEDBYTES * L + 2 + size_enc_h + size_enc_hb_z1 > CRYPTO_SIGNATUREBYTES ) {
        return 1; // 签名尺寸超出范围
    }

    if (decode_hb_z1(&highbits_z1->vec[0].coeffs[0], sig, size_enc_hb_z1)) {
        return 1; // 解码失败
    }

    sig += size_enc_hb_z1;

    if (decode_h(&h->vec[0].coeffs[0], sig, size_enc_h)) {
        return 1; // 解码失败
    }

    sig += size_enc_h;

    for (unsigned int j = 0; j < CRYPTO_SIGNATUREBYTES - (POLY_C_PACKEDBYTES + POLY_LOWBITS_PACKEDBYTES * L + 2 + size_enc_h + size_enc_hb_z1); j++) {
        if (sig[j] != 0) {
            return 1; // 非零填充
        }
    }

    return 0;
}
