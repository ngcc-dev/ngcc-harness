/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-128 instance.
*/
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "params.h"
#include "drng.h"
#include "auxfunc.h"
#include "ntt.h"
#include "poly.h"
#include "pke.h"
#include "KEM_AlgorithmInstance.h"
#include "cpucycles.h"

static void *aligned_calloc_32(size_t n, size_t size)
{
    size_t bytes = n * size;
    void *ptr = NULL;

    if (posix_memalign(&ptr, 32, bytes) != 0)
        return NULL;

    memset(ptr, 0, bytes);
    return ptr;
}

#define aligned_free_32(ptr) free(ptr)

#define NTESTS 100000

uint64_t t[NTESTS];
uint8_t seed[SEED_LEN_BYTES] = {0};

DRNG_ctx drng_algorithm;

int main()
{
    unsigned int i;
    unsigned char alg_nonce;
    unsigned char *nonce;
	DRNG_ctx drng_seed;
    unsigned char *seed, *ss, *ss1, *ct, *pk, *sk, *m;
    unsigned long long pk_len_bytes, sk_len_bytes, ss_len_bytes, ct_len_bytes;
    int16_t *ap, *bp, *cp;

    m = (unsigned char *)aligned_calloc_32(ZEN_INDCPA_MSG_LEN_BYTES, sizeof(unsigned char));
    pk = (unsigned char *)aligned_calloc_32(ZEN_PUBLICKEY_LEN_BYTES, sizeof(unsigned char));
    sk = (unsigned char *)aligned_calloc_32(ZEN_SECREKEY_LEN_BYTES, sizeof(unsigned char));
    ss = (unsigned char *)aligned_calloc_32(ZEN_SHAREDKEY_LEN_BYTES, sizeof(unsigned char));
    ss1 = (unsigned char *)aligned_calloc_32(ZEN_SHAREDKEY_LEN_BYTES, sizeof(unsigned char));
    ct = (unsigned char *)aligned_calloc_32(ZEN_CIPHERTEXT_LEN_BYTES, sizeof(unsigned char));
    seed = (unsigned char *)aligned_calloc_32(SEED_LEN_BYTES, sizeof(unsigned char));
    ap = (int16_t *)aligned_calloc_32(ZEN_N, sizeof(int16_t));
    bp = (int16_t *)aligned_calloc_32(ZEN_N, sizeof(int16_t));
    cp = (int16_t *)aligned_calloc_32(ZEN_N, sizeof(int16_t));

    nonce = (unsigned char *)aligned_calloc_32(SEED_LEN_BYTES, sizeof(unsigned char));
	for (int i = 0; i < SEED_LEN_BYTES / 4; i++)
	{
		memcpy(nonce + 4 * i, "seed", 4);
	}
	init_random_number(&drng_seed, nonce, SEED_LEN_BYTES);
    get_random_number(&drng_seed, seed, SEED_LEN_BYTES*8);

    alg_nonce = 0;
    poly_generate_f(ap, seed, alg_nonce++);
    poly_ntt_mq(ap, nttdata);
    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
        check_poly_inv_Zq(ap);
    }
    print_results("check_poly_inv_Zq: ", t, NTESTS);

    poly_generate_f(ap, seed, alg_nonce++);
    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
        check_poly_inv_Z2(ap);
    }
    print_results("check_poly_inv_Z2: ", t, NTESTS);

    poly_generate_f(ap, seed, alg_nonce++);
    for(i = 0; i < ZEN_N4; i++)
    {
        ap[i] = (ap[i] & 1) ^ (ap[i + ZEN_N4] & 1) ^ (ap[i + 2*ZEN_N4] & 1) ^ (ap[i + 3*ZEN_N4] & 1);
    }
    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
        FastInversion(bp, ap);
    }
    print_results("FastInversion: ", t, NTESTS);

    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
        poly_generate_g(ap, seed, alg_nonce++);
    }
    print_results("poly_generate_g: ", t, NTESTS);

    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
        poly_generate_f(ap, seed, alg_nonce++);
    }
    print_results("poly_generate_f: ", t, NTESTS);

    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
        poly_generate_s(ap, seed, alg_nonce++);
    }
    print_results("poly_generate_s: ", t, NTESTS);

    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
        poly_generate_e(ap, seed, alg_nonce++);
    }
    print_results("poly_generate_e: ", t, NTESTS);

    poly_generate_f(ap, seed, alg_nonce++);
    poly_ntt_mq(ap, nttdata);
    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
        poly_secretkey_pack(sk, ap);
    }
    print_results("poly_secretkey_pack: ", t, NTESTS);

    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
        poly_secretkey_unpack(ap, sk);
    }
    print_results("poly_secretkey_unpack: ", t, NTESTS);

    poly_generate_f(ap, seed, alg_nonce++);
    poly_generate_g(bp, seed, alg_nonce++);
    poly_ntt(ap, nttdata);
    poly_ntt(bp, nttdata);
    poly_baseinv_ntt(cp, ap);
    poly_basemul_ntt_mq(ap, bp, cp, muldata);
    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
        poly_publickey_pack(pk, ap);
    }
    print_results("poly_publickey_pack: ", t, NTESTS);

    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
        poly_publickey_unpack(ap, pk);
    }
    print_results("poly_publickey_unpack: ", t, NTESTS);

    poly_generate_f(ap, seed, alg_nonce++);
    poly_ntt_mq(ap, nttdata);
    poly_compress(ap);
    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
        poly_ciphertext_pack(ct, ap);
    }
    print_results("poly_ciphertext_pack: ", t, NTESTS);

    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
        poly_ciphertext_unpack(ap, ct);
    }
    print_results("poly_ciphertext_unpack: ", t, NTESTS);

    poly_generate_f(ap, seed, alg_nonce++);
    poly_ntt_mq(ap, nttdata);
    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
        poly_compress(ap);
    }
    print_results("poly_compress: ", t, NTESTS);

    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
        poly_decompress(ap);
    }
    print_results("poly_decompress: ", t, NTESTS);

    poly_generate_f(ap, seed, alg_nonce++);
    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
        poly_ntt(ap, nttdata);
    }
    print_results("poly_ntt: ", t, NTESTS);

    poly_generate_f(ap, seed, alg_nonce++);
    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
        poly_ntt_mq(ap, nttdata);
    }
    print_results("poly_ntt_mq: ", t, NTESTS);

    poly_generate_f(ap, seed, alg_nonce++);
    poly_generate_f(bp, seed, alg_nonce++);
    poly_ntt(ap, nttdata);
    poly_ntt(bp, nttdata);
    poly_basemul_ntt(cp, ap, bp, muldata);
    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
        poly_intt(cp, inttdata);
    }
    print_results("poly_intt: ", t, NTESTS);

    poly_generate_f(ap, seed, alg_nonce++);
    poly_generate_f(bp, seed, alg_nonce++);
    poly_ntt(ap, nttdata);
    poly_ntt(bp, nttdata);
    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
        poly_basemul_ntt(cp, ap, bp, muldata);
    }
    print_results("poly_basemul_ntt: ", t, NTESTS);

    poly_generate_f(ap, seed, alg_nonce++);
    poly_generate_f(bp, seed, alg_nonce++);
    poly_ntt(ap, nttdata);
    poly_ntt(bp, nttdata);
    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
        poly_basemul_ntt_mq(cp, ap, bp, muldata);
    }
    print_results("poly_basemul_ntt_mq: ", t, NTESTS);

    poly_generate_f(ap, seed, alg_nonce++);
    poly_ntt(ap, nttdata);
    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
        poly_baseinv_ntt(bp, ap);
    }
    print_results("poly_baseinv_ntt: ", t, NTESTS);

    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
        pke_keygen(pk, sk);
    }
    print_results("pke_keygen: ", t, NTESTS);

    pke_keygen(pk, sk);
    get_random_number(&drng_seed, seed, SEED_LEN_BYTES*8);
    get_random_number(&drng_seed, m, ZEN_INDCPA_MSG_LEN_BYTES*8);
    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
        pke_enc(pk, m, seed, ct);
    }
    print_results("pke_enc: ", t, NTESTS);

    pke_keygen(pk, sk);
    pke_enc(pk, m, seed, ct);
    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
        pke_dec(sk, ct, m);
    }
    print_results("pke_dec: ", t, NTESTS);

    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
        kem_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes);
    }
    print_results("kem_keygen: ", t, NTESTS);

    kem_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes);
    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
	    kem_enc(pk, pk_len_bytes, ss, &ss_len_bytes, ct, &ct_len_bytes);
    }
    print_results("kem_enc: ", t, NTESTS);

    kem_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes);
    kem_enc(pk, pk_len_bytes, ss, &ss_len_bytes, ct, &ct_len_bytes);
    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
	    kem_dec(sk, sk_len_bytes, ct, ct_len_bytes, ss1, &ss_len_bytes);
    }
    print_results("kem_dec: ", t, NTESTS);

  return 0;
}
