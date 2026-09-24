#include <stdint.h>
#include <stdio.h>

#include "../params.h"
#include "../poly.h"
#include "../ntt.h"
#include "../indcpa.h"
#include "../kem.h"
#include "../decode.h"
#include "../symmetric.h"
#include "../drng.h"

DRNG_ctx drng_algorithm;
static void randombytes(unsigned char *x, unsigned long long xlen) { get_random_number(&drng_algorithm, x, xlen*8); }
#include "cpucycles.h"
#include "speed_print.h"

#define NTESTS 10000

static uint64_t t[NTESTS + 1];

int main(void)
{
    unsigned int i;
    uint8_t seed[SEEDBYTES];
    uint8_t nonce = 0;

    poly a, b, c;
    poly c0;
    poly f;
    poly_f1 f1;
    uint8_t packed[KEM_POLYCOMPRESSEDBYTES];
    uint8_t packed_full[KEM_POLYBYTES];

    uint8_t m[KEM_CPAPKE_MSGBYTES];
    uint8_t m2[KEM_CPAPKE_MSGBYTES];
    uint8_t coins[SEEDBYTES];
    uint8_t pk[KEM_CPAPKE_PUBLICKEYBYTES];
    uint8_t sk[KEM_CPAPKE_SECRETKEYBYTES];
    uint8_t ct[KEM_CPAPKE_CIPHERTEXTBYTES];

    uint8_t kem_pk[KEM_PUBLICKEYBYTES];
    uint8_t kem_sk[KEM_SECRETKEYBYTES];
    uint8_t kem_ct[KEM_CIPHERTEXTBYTES];
    uint8_t ss1[SEEDBYTES], ss2[SEEDBYTES];

    /* Seed DRNG once */
    unsigned char init_seed[64];
    for (i = 0; i < 64; i++) init_seed[i] = (unsigned char)i;
    init_random_number(&drng_algorithm, init_seed, 64);

    /* ================================================================
     *  Low-level arithmetic: fqmul, freeze, fqinv
     * ================================================================ */
    int16_t rand_x[NTESTS + 1], rand_y[NTESTS + 1];
    for (i = 0; i <= NTESTS; ++i) {
        randombytes((uint8_t *)&rand_x[i], sizeof(int16_t));
        randombytes((uint8_t *)&rand_y[i], sizeof(int16_t));
    }

    for (i = 0; i <= NTESTS; ++i) {
        t[i] = cpucycles();
        montgomery_reduce((int32_t)rand_x[i] * rand_y[i]);
    }
    print_results("fqmul (montgomery_reduce):", t, NTESTS + 1);

    for (i = 0; i <= NTESTS; ++i) {
        t[i] = cpucycles();
        freeze(rand_x[i]);
    }
    print_results("freeze:", t, NTESTS + 1);

    for (i = 0; i <= NTESTS; ++i) {
        t[i] = cpucycles();
        fqinv(rand_x[i]);
    }
    print_results("fqinv:", t, NTESTS + 1);

    /* ================================================================
     *  DRNG: get_random_number
     * ================================================================ */
    {
        uint8_t rng_buf[SEEDBYTES];
        for (i = 0; i <= NTESTS; ++i) {
            t[i] = cpucycles();
            get_random_number(&drng_algorithm, rng_buf,
                              (unsigned long long)sizeof(rng_buf) * 8);
        }
        print_results("get_random_number:", t, NTESTS + 1);
    }

    /* ================================================================
     *  Polynomial sampling (ternary)
     * ================================================================ */
    for (i = 0; i <= NTESTS; ++i) {
        t[i] = cpucycles();
        poly_f_ternary_p(&a, seed, nonce++);
    }
    print_results("poly_f_ternary_p:", t, NTESTS + 1);

    for (i = 0; i <= NTESTS; ++i) {
        t[i] = cpucycles();
        poly_g_ternary_p(&c, seed, nonce++);
    }
    print_results("poly_g_ternary_p:", t, NTESTS + 1);

    for (i = 0; i <= NTESTS; ++i) {
        t[i] = cpucycles();
        poly_r_ternary_p(&c, seed, nonce++);
    }
    print_results("poly_r_ternary_p:", t, NTESTS + 1);

    for (i = 0; i <= NTESTS; ++i) {
        t[i] = cpucycles();
        poly_e_ternary_p(&c, seed, nonce++);
    }
    print_results("poly_e_ternary_p:", t, NTESTS + 1);

    /* ================================================================
     *  Hashes: hash_h, hash_g
     * ================================================================ */
    uint8_t h_in[KEM_CIPHERTEXTBYTES], h_out[SEEDBYTES];
    uint8_t g_in[2 * SEEDBYTES], g_out[2 * SEEDBYTES];
    randombytes(h_in, sizeof(h_in));
    randombytes(g_in, sizeof(g_in));

    for (i = 0; i <= NTESTS; ++i) {
        t[i] = cpucycles();
        hash_h(h_out, h_in, sizeof(h_in));
    }
    print_results("hash_h:", t, NTESTS + 1);

    for (i = 0; i <= NTESTS; ++i) {
        t[i] = cpucycles();
        hash_g(g_out, g_in, sizeof(g_in));
    }
    print_results("hash_g:", t, NTESTS + 1);

    /* KDF: shake256(ss, SEEDBYTES, kr, 2*SEEDBYTES) */
    uint8_t kdf_in[2 * SEEDBYTES], kdf_out[SEEDBYTES];
    randombytes(kdf_in, sizeof(kdf_in));

    for (i = 0; i <= NTESTS; ++i) {
        t[i] = cpucycles();
        kdf(kdf_out, kdf_in, sizeof(kdf_in));
    }
    print_results("kdf (shake256):", t, NTESTS + 1);

    /* ================================================================
     *  NTT domain: ntt, invntt, basemul, baseinv
     * ================================================================ */
    poly_f_ternary_p(&a, seed, nonce++);
     for (i = 0; i <= NTESTS; ++i) {
       
        t[i] = cpucycles();
        poly_ntt(&a);
    }
    print_results("poly_ntt:", t, NTESTS + 1);
    
    poly_f_ternary_p(&a, seed, nonce++);
    poly_ntt(&a);
    for (i = 0; i <= NTESTS; ++i) {

        t[i] = cpucycles();
        poly_invntt_tomont(&a);
    }
    print_results("poly_invntt_tomont:", t, NTESTS + 1);

    poly_f_ternary_p(&a, seed, nonce++);
    poly_g_ternary_p(&b, seed, nonce++);
    poly_ntt(&a);
    poly_ntt(&b);
    for (i = 0; i <= NTESTS; ++i) {
        t[i] = cpucycles();
        poly_basemul_montgomery(&c, &a, &b);
    }
    print_results("poly_basemul_montgomery:", t, NTESTS + 1);

    poly_f_ternary_p(&a, seed, nonce++);
    poly_ntt(&a);
    for (i = 0; i <= NTESTS; ++i) {
        t[i] = cpucycles();
        poly_baseinv(&c, &a);
    }
    print_results("poly_baseinv:", t, NTESTS + 1);

    /* ================================================================
     *  Inverse in F2 + decode
     * ================================================================ */
    do {
        poly_f_ternary_p(&f, seed, nonce++);
    } while (!poly_inv_in_F2(&f1, &f));

    for (i = 0; i <= NTESTS; ++i) {
        t[i] = cpucycles();
        poly_inv_in_F2(&f1, &f);
    }
    print_results("poly_inv_in_F2:", t, NTESTS + 1);

    poly_g_ternary_p(&c0, seed, nonce++);
    poly_compress_and_pack(packed, &c0);
    poly_unpack_and_decompress(&c0, packed);
    for (i = 0; i <= NTESTS; ++i) {
        c = c0;
        t[i] = cpucycles();
        poly_decode_to_msg(m2, &c, &f1);
    }
    print_results("poly_decode_to_msg:", t, NTESTS + 1);

    /* ================================================================
     *  Packing / unpacking
     * ================================================================ */
    poly_f_ternary_p(&a, seed, nonce++);
    for (i = 0; i <= NTESTS; ++i) {
        t[i] = cpucycles();
        poly_compress_and_pack(packed, &a);
    }
    print_results("poly_compress_and_pack:", t, NTESTS + 1);

    poly_g_ternary_p(&a, seed, nonce++);
    poly_compress_and_pack(packed, &a);
    for (i = 0; i <= NTESTS; ++i) {
        t[i] = cpucycles();
        poly_unpack_and_decompress(&b, packed);
    }
    print_results("poly_unpack_and_decompress:", t, NTESTS + 1);

    /* ── poly_to_bytes / poly_from_bytes (uncompressed) ─────────── */
    poly_f_ternary_p(&a, seed, nonce++);
    poly_freeze(&a);
    for (i = 0; i <= NTESTS; ++i) {
        t[i] = cpucycles();
        poly_to_bytes(packed_full, &a);
    }
    print_results("poly_to_bytes:", t, NTESTS + 1);

    poly_f_ternary_p(&a, seed, nonce++);
    poly_freeze(&a);
    poly_to_bytes(packed_full, &a);
    for (i = 0; i <= NTESTS; ++i) {
        t[i] = cpucycles();
        poly_from_bytes(&b, packed_full);
    }
    print_results("poly_from_bytes:", t, NTESTS + 1);

    /* ================================================================
     *  IND-CPA: keypair, enc, dec
     * ================================================================ */
    for (i = 0; i <= NTESTS; ++i) {
        t[i] = cpucycles();
        indcpa_keypair(pk, sk);
    }
    print_results("indcpa_keypair:", t, NTESTS + 1);

    indcpa_keypair(pk, sk);
    for (i = 0; i <= NTESTS; ++i) {
        t[i] = cpucycles();
        indcpa_enc(ct, m, pk, coins);
    }
    print_results("indcpa_enc:", t, NTESTS + 1);

    indcpa_enc(ct, m, pk, coins);
    for (i = 0; i <= NTESTS; ++i) {
        t[i] = cpucycles();
        indcpa_dec(m2, ct, sk);
    }
    print_results("indcpa_dec:", t, NTESTS + 1);

    /* ================================================================
     *  Full KEM: keypair, enc, dec
     * ================================================================ */
    for (i = 0; i <= NTESTS; ++i) {
        t[i] = cpucycles();
        crypto_kem_keypair(kem_pk, kem_sk);
    }
    print_results("crypto_kem_keypair:", t, NTESTS + 1);

    crypto_kem_keypair(kem_pk, kem_sk);
    for (i = 0; i <= NTESTS; ++i) {
        t[i] = cpucycles();
        crypto_kem_enc(kem_ct, ss1, kem_pk);
    }
    print_results("crypto_kem_enc:", t, NTESTS + 1);

    crypto_kem_enc(kem_ct, ss1, kem_pk);
    for (i = 0; i <= NTESTS; ++i) {
        t[i] = cpucycles();
        crypto_kem_dec(ss2, kem_ct, kem_sk);
    }
    print_results("crypto_kem_dec:", t, NTESTS + 1);

    return 0;
}