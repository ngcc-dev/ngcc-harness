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
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#define NTESTS 1000

uint64_t t[NTESTS];
uint8_t seed[SEED_LEN_BYTES] = {0};

DRNG_ctx drng_algorithm;

/* ---------------- 墙钟计时（新增）---------------- */
static inline uint64_t now_us(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (uint64_t)tv.tv_sec * 1000000ull + (uint64_t)tv.tv_usec;
}

static inline void wall_print_result(const char *label, uint64_t total_us, int n) {
  double avg_us = (double)total_us / (double)n;        // 平均用时 (us)
  double ops_s  = (double)n * 1e6 / (double)total_us;  // 吞吐率 (ops/s)
  printf("%s time | total: %llu us | average: %.3f us | throughput: %.2f ops/s\n",
         label, (unsigned long long)total_us, avg_us, ops_s);
}
/* ------------------------------------------------ */

int main()
{
    unsigned int i;
    unsigned char alg_nonce;
    unsigned char *nonce;
	  DRNG_ctx drng_seed;
    unsigned char *seed, *ss, *ss1, *ct, *pk, *sk, *m;
    unsigned long long pk_len_bytes, sk_len_bytes, ss_len_bytes, ct_len_bytes;
    int16_t *ap, *bp, *cp;
    uint64_t wc_t0, wc_total;

    m = (unsigned char *)calloc(ZEN_INDCPA_MSG_LEN_BYTES, sizeof(unsigned char));
    pk = (unsigned char *)calloc(ZEN_PUBLICKEY_LEN_BYTES, sizeof(unsigned char));
    sk = (unsigned char *)calloc(ZEN_SECREKEY_LEN_BYTES, sizeof(unsigned char));
    ss = (unsigned char *)calloc(ZEN_SHAREDKEY_LEN_BYTES, sizeof(unsigned char));
    ss1 = (unsigned char *)calloc(ZEN_SHAREDKEY_LEN_BYTES, sizeof(unsigned char));
    ct = (unsigned char *)calloc(ZEN_CIPHERTEXT_LEN_BYTES, sizeof(unsigned char));
    seed = (unsigned char *)calloc(SEED_LEN_BYTES, sizeof(unsigned char));
    ap = (int16_t *)calloc(ZEN_N, sizeof(int16_t));
    bp = (int16_t *)calloc(ZEN_N, sizeof(int16_t));
    cp = (int16_t *)calloc(ZEN_N, sizeof(int16_t));

    printf("Public key size: %d Byte\n", ZEN_PUBLICKEY_LEN_BYTES);
    printf("Secret key size: %d Byte\n", ZEN_SECREKEY_LEN_BYTES);
    printf("Ciphertext size: %d Byte\n", ZEN_CIPHERTEXT_LEN_BYTES);
    printf("Shared secret key size: %d Byte\n", ZEN_SHAREDKEY_LEN_BYTES);
    printf("Running %d iterations.\n", NTESTS);
    
    // /* -------- ZEN KeyGen -------- */
    // sleep(3);
    // //printf("-------- ZEN KeyGen begin --------\n");
    // wc_t0 = now_us();
    // for(i=0;i<NTESTS;i++) {
    //   kem_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes); 
    // }
    // wc_total = now_us() - wc_t0; 
    // wall_print_result("ZEN KeyGen", wc_total, NTESTS);
    // //printf("-------- ZEN KeyGen end --------\n");
    
    // /* -------- ZEN Encrypt -------- */
    // kem_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes);
    // sleep(3);
    // //printf("-------- ZEN Encrypt begin --------\n");
    // wc_t0 = now_us();
    // for(i=0;i<NTESTS;i++) {
    //   kem_enc(pk, pk_len_bytes, ss, &ss_len_bytes, ct, &ct_len_bytes);
    // }
    // wc_total = now_us() - wc_t0; 
    // wall_print_result("ZEN Encrypt", wc_total, NTESTS);
    // //printf("-------- ZEN Encrypt end --------\n");

    /* -------- ZEN Decrypt -------- */
    kem_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes);
    kem_enc(pk, pk_len_bytes, ss, &ss_len_bytes, ct, &ct_len_bytes);
    sleep(3);
    //printf("-------- ZEN Decrypt begin --------\n");
    wc_t0 = now_us();
    for(i=0;i<NTESTS;i++) {
      kem_dec(sk, sk_len_bytes, ct, ct_len_bytes, ss1, &ss_len_bytes);
    }
    wc_total = now_us() - wc_t0; 
    wall_print_result("ZEN Decrypt", wc_total, NTESTS);
    //printf("-------- ZEN Decrypt end --------\n");

  return 0;
}
