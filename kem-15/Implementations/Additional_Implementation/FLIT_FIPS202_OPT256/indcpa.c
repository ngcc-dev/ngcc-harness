#include <stddef.h>
#include <stdint.h>
#include "params.h"
#include "indcpa.h"
#include "poly.h"
#include "packing.h"
#include "rng.h"
#include "ntt.h"
#include "symmetric.h"
#include "decode.h"


/*************************************************
* Name:        indcpa_keypair
*
* Description: Generates public and private key for the CPA-secure
*              public-key encryption scheme underlying KEM
*
* Arguments:   - uint8_t *pk: pointer to output public key
*                             (of length KEM_CPAPKE_PUBLICKEYBYTES bytes)
*              - uint8_t *sk: pointer to output private key
                              (of length KEM_CPAPKE_SECRETKEYBYTES bytes)
**************************************************/
void indcpa_keypair(uint8_t pk[KEM_CPAPKE_PUBLICKEYBYTES],
                    uint8_t sk[KEM_CPAPKE_SECRETKEYBYTES])
{
    uint8_t seed[SEEDBYTES];
    uint8_t nonce = 0;
    poly f, f_inv, g, h;
    poly_f1 f1;
    int r1, r2;

    randombytes(seed, SEEDBYTES);

reject:
    poly_f_ternary_p(&f, seed, nonce++);
    r1 = poly_inv_in_F2(&f1, &f);
    if (r1 == 0) {
        goto reject;
    }    
    
    poly_ntt(&f);
    
    r2 = poly_baseinv(&f_inv, &f);
    if (r2 != 0) {
        goto reject;
    }
    
    poly_g_ternary_p(&g, seed, nonce++);

    poly_ntt(&g);

    poly_basemul_montgomery(&h, &f_inv, &g); // h = g/f
    poly_tomont(&h);
    poly_freeze(&h);

    pack_sk(sk, &f, &f1); // Pack f in NTT domain and f1 in normal domain
    pack_pk(pk, &h);      // Pack h in NTT domain
}

/*************************************************
* Name:        indcpa_enc
*
* Description: Encryption function of the CPA-secure
*              public-key encryption scheme underlying FLIT.
*
* Arguments:   - uint8_t *c:           pointer to output ciphertext
*                                      (of length KEM_CPAPKE_CIPHERTEXTBYTES bytes)
*              - const uint8_t *m:     pointer to input message
*                                      (of length KEM_MSGBYTES bytes)
*              - const uint8_t *pk:    pointer to input public key
*                                      (of length KEM_CPAPKE_PUBLICKEYBYTES)
*              - const uint8_t *coins: pointer to input random coins
*                                      used as seed (of length SEEDBYTES)
*                                      to deterministically generate all
*                                      randomness
**************************************************/
void indcpa_enc(uint8_t c[KEM_CPAPKE_CIPHERTEXTBYTES],
                const uint8_t m[KEM_MSGBYTES],
                const uint8_t pk[KEM_CPAPKE_PUBLICKEYBYTES],
                const uint8_t coins[SEEDBYTES])
{
    uint8_t nonce = 0;
    poly h, r, e, u, v, c1;

    unpack_pk(&h, pk);

    poly_r_ternary_p(&r, coins, nonce++);
    poly_e_ternary_p(&e, coins, nonce++);

    poly_ntt(&r);

    poly_basemul_montgomery(&u, &r, &h);  // u = h·r
    poly_invntt_tomont(&u);
    
    poly_add(&u, &u, &e);  // u = h·r + e 
    poly_from_msg(&v, m);  // v = (q+1)/2 * Encode(m)
    poly_add(&c1, &v, &u); // c = h·r + e + (q+1)/2 * Encode(m)
    
    poly_compress_and_pack(c, &c1); // Pack c in normal domain
}

/*************************************************
* Name:        indcpa_dec
*
* Description: Decryption function of the CPA-secure
*              public-key encryption scheme underlying FLIT.
*
* Arguments:   - uint8_t *m:        pointer to output decrypted message
*                                   (of length KEM_CPAPKE_MSGBYTES)
*              - const uint8_t *c:  pointer to input ciphertext
*                                   (of length KEM_CPAPKE_CIPHERTEXTBYTES)
*              - const uint8_t *sk: pointer to input secret key
*                                   (of length KEM_CPAPKE_SECRETKEYBYTES)
**************************************************/
void indcpa_dec(uint8_t m[KEM_CPAPKE_MSGBYTES],
                const uint8_t c[KEM_CPAPKE_CIPHERTEXTBYTES],
                const uint8_t sk[KEM_CPAPKE_SECRETKEYBYTES])
{
    poly c1, f, u;
    poly_f1 f1;

    unpack_sk(&f, &f1, sk);

    poly_unpack_and_decompress(&c1, c);
    poly_ntt(&c1);

    poly_basemul_montgomery(&u, &f, &c1); // u = f·c = gr + fe + f·(q+1)/2 * Encode(m)
    
    poly_invntt_tomont(&u);
    poly_decode_to_msg(m, &u, &f1);
}