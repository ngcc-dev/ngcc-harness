#include "packing.h"
#include "params.h"
#include "poly.h"
#include <string.h>

/*************************************************
 * Name:        pack_pk
 *
 * Description: Bit-pack public key pk = NTT(h).
 *
 * Arguments:   - uint8_t pk[]: output byte array
 *              - const poly *h: polynomial h to be packed
 **************************************************/
void pack_pk(uint8_t pk[KEM_CPAPKE_PUBLICKEYBYTES], const poly *h) {
    poly_to_bytes(pk, h);
}

/*************************************************
 * Name:        unpack_pk
 *
 * Description: Unpack public key pk = NTT(h).
 *
 * Arguments:   - uint8_t pk[]: input byte array
 *              - poly *h: output polynomial h
 **************************************************/
void unpack_pk(poly *h, const uint8_t pk[KEM_CPAPKE_PUBLICKEYBYTES]) {
    poly_from_bytes(h, pk);
}

/*************************************************
 * Name:        pack_sk
 *
 * Description: Bit-pack secret key sk = (f, f1).
 *
 * Arguments:   - uint8_t sk[]: output byte array
 *              - const poly *f: polynomial f to be packed
 *              - const poly_f1 *f1: polynomial f1 to be packed
 **************************************************/
void pack_sk(uint8_t sk[KEM_CPAPKE_SECRETKEYBYTES], const poly *f, const poly_f1 *f1) {

    memset(sk, 0, KEM_CPAPKE_SECRETKEYBYTES);
    poly_freeze((poly *)f);

#if Q == 769
    for (unsigned int i = 0; i < N/4; i++) {
        sk[5*i + 0] = (f->coeffs[4*i + 0]) & 0xFF;
        sk[5*i + 1] = ((f->coeffs[4*i + 0] >> 8) | (f->coeffs[4*i + 1] << 2)) & 0xFF;
        sk[5*i + 2] = ((f->coeffs[4*i + 1] >> 6) | (f->coeffs[4*i + 2] << 4)) & 0xFF;
        sk[5*i + 3] = ((f->coeffs[4*i + 2] >> 4) | (f->coeffs[4*i + 3] << 6)) & 0xFF;
        sk[5*i + 4] = (f->coeffs[4*i + 3] >> 2) & 0xFF;
    }
#elif Q == 3329
    for (unsigned int i = 0; i < N/2; i++) {
        sk[3*i + 0] =  f->coeffs[2*i + 0]        & 0xFF;
        sk[3*i + 1] = (f->coeffs[2*i + 0] >> 8)  & 0x0F;
        sk[3*i + 1] |= (f->coeffs[2*i + 1] & 0x0F) << 4;
        sk[3*i + 2] = (f->coeffs[2*i + 1] >> 4)  & 0xFF;
    }
#endif

#if KEM_MODE == 128
    for (unsigned int i = 0; i < N/2; i++) {
        sk[N * Q_BITS / 8 + i/8] |= f1->coeffs[i] << ((i % 8));
    }
#elif KEM_MODE == 256 || KEM_MODE == 512
    for (unsigned int i = 0; i < N/4; i++) {
        sk[N * Q_BITS / 8 + i/8] |= f1->coeffs[i] << ((i % 8));
    }
#endif
}

/*************************************************
 * Name:        unpack_sk
 *
 * Description: Unpack secret key sk = (f, f1).
 *
 * Arguments:   - uint8_t sk[]: input byte array
 *              - poly *f: output polynomial f
 *              - poly_f1 *f1: output polynomial f1
 **************************************************/
void unpack_sk(poly *f, poly_f1 *f1, const uint8_t sk[KEM_CPAPKE_SECRETKEYBYTES]) {

#if Q == 769
    for (unsigned int i = 0; i < N/4; i++) {
        f->coeffs[4*i + 0] = sk[5*i + 0] | ((sk[5*i + 1] & 0x03) << 8);
        f->coeffs[4*i + 1] = (sk[5*i + 1] >> 2) | ((sk[5*i + 2] & 0x0F) << 6);
        f->coeffs[4*i + 2] = (sk[5*i + 2] >> 4) | ((sk[5*i + 3] & 0x3F) << 4);
        f->coeffs[4*i + 3] = (sk[5*i + 3] >> 6) | (sk[5*i + 4] << 2);
    }
#elif Q == 3329
    for (unsigned int i = 0; i < N/2; i++) {
        f->coeffs[2*i + 0] =  sk[3*i + 0] | ((sk[3*i + 1] & 0x0F) << 8);
        f->coeffs[2*i + 1] = (sk[3*i + 1] >> 4) | (sk[3*i + 2] << 4);
    }
#endif

#if KEM_MODE == 128
    for (unsigned int i = 0; i < N/2; i++) {
        f1->coeffs[i] = (sk[N * Q_BITS / 8 + i/8] >> (i % 8)) & 0x01;
    }
#elif KEM_MODE == 256 || KEM_MODE == 512
    for (unsigned int i = 0; i < N/4; i++) {
        f1->coeffs[i] = (sk[N * Q_BITS / 8 + i/8] >> (i % 8)) & 0x01;
    }
#endif
}