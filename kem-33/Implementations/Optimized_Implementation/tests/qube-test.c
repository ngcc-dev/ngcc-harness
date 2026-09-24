#include <stdio.h>
#include "api.h"
#include "parameters.h"
#include "gf.h"
#include "gf2x.h"
#include "poly-test.h"
#include "qube.h"
#include "kem_qube.h"

#include "code.h"
#include "randombytes.h"
#include "reed_muller.h"
#include "reed_solomon.h"
#include "symmetric.h"
#include "vector.h"
#define DO_BENCHMARK
#if defined(DO_BENCHMARK)
#include "benchmark.h"
#else

#include "stdint.h"

static inline
void report(char *buf, size_t bufsize, void *recs, unsigned len) {
    sprintf(buf,"");
    (void) bufsize;
    (void) recs;
    (void) len;
}

#define REC_TIMING(recs,len,call) do { \
        call; \
    } while (0)

#endif

int vec_equal(unsigned char *a, unsigned char *b, int len) {
    for(int i = 0 ; i < len ; ++i) {
        if(a[i] != b[i]) return 0;
    }
    return 1;
}

#define TEST_RUN (1000)


// ===== 主测试 =====
int main(void) {
    uint8_t seed[48] = {0};
    CryptoRandomBytes(seed, sizeof(seed));
    prng_init(seed, NULL, sizeof(seed), 0);

///////////////////////////////////////////////additive FFT test///////////////////////////////////////////////////
    uint8_t a[NB]={0}, b[NB]={0};
    uint8_t c1[NB], c2[NB];

    __m256i a_[VEC_N_256_NUM_WORDS];
    __m256i b_[VEC_N_256_NUM_WORDS];
    
    printf("additive FFT test:\n");
    random_poly(a);
    random_poly(b);
    // a[0]=0x02;
    // b[0]=0x01;
    memset(a_, 0, sizeof(a_));
    memset(b_, 0, sizeof(b_));

    memcpy(a_, a, NB);
    memcpy(b_, b, NB);
    
    printf("Running naive...\n");
    ring_mul_naive(c1, a, b);

    printf("Running your ring_mul...\n");
    vect_mul(c2, a_, b_);
    poly_equal(c1, c2);
////////////////////////////////////////////////////invs polynomial///////////////////////////////////////////////////
    // static const int f[] = {3,15,63,255,1023,4095};//f=消失多项式/x 
    // int f_len = 6;
    // int n = 1664;//mod x^n
    // int invs[1664];

    // int cnt = compute_series(f, f_len, n, invs);

    // // 输出invs数组
    // printf("invs[] = {");
    // for (int i = 0; i < cnt; i++) {
    //     printf("%d", invs[i]);
    //     if (i != cnt - 1) printf(", ");
    // }
    // printf("}\n");
    
/////////////////////////////////////////////////KEM test/////////////////////////////////////////////////
	printf("\n");
	printf("**********************\n");
	printf("**** QUBE-%d-%d ****\n", PARAM_SECURITY, PARAM_DFR_EXP);
	printf("**********************\n");

	printf("\n");
	printf("N: %d   ", PARAM_N);
	printf("N1: %d   ", PARAM_N1);
	printf("N2: %d   ", PARAM_N2);
    // printf("N1N2: %d   ", PARAM_N1N2);

    // printf("\nOMEGA_X1: %d   ", PARAM_OMEGA_X1);
    // printf("OMEGA_X2: %d   ", PARAM_OMEGA_X2);
    // printf("\nOMEGA_Y1: %d   ", PARAM_OMEGA_Y1);
    // printf("OMEGA_Y2: %d   ", PARAM_OMEGA_Y2);

    // printf("\nOMEGA_R1: %d, %d ", PARAM_OMEGA_R11, PARAM_OMEGA_R12);
    // printf("OMEGA_R2: %d, %d ", PARAM_OMEGA_R21, PARAM_OMEGA_R22);

    // printf("OMEGA_E: %d   ", PARAM_OMEGA_E);

    // printf("\nPUBLICKEYBYTES: %d   ", CRYPTO_PUBLICKEYBYTES);
    // printf("SECRETKEYBYTES: %d   ", CRYPTO_SECRETKEYBYTES);
    // printf("CIPHERTEXTBYTES: %d   ", CRYPTO_CIPHERTEXTBYTES);

	printf("\nFailure rate: 2^-%d   ", PARAM_DFR_EXP);
	printf("Sec: %d bits", PARAM_SECURITY);
    printf("\n");

	unsigned char pk[PUBLIC_KEY_BYTES];
	unsigned char sk[SECRET_KEY_BYTES];
	unsigned char ct[CIPHERTEXT_BYTES];
	unsigned char key1[SHARED_SECRET_BYTES];
	unsigned char key2[SHARED_SECRET_BYTES];

    unsigned long long pk_len_bytes, sk_len_bytes;
    unsigned long long ct_len_bytes;
    unsigned long long key1_len_bytes, key2_len_bytes;

    memset(pk, 0, PUBLIC_KEY_BYTES);
    memset(sk, 0, SECRET_KEY_BYTES);
    memset(ct, 0, CIPHERTEXT_BYTES);
    memset(key1, 0, SHARED_SECRET_BYTES);
    memset(key2, 0, SHARED_SECRET_BYTES);

    uint64_t rec_k[TEST_RUN] = {0}; unsigned len_k[1] = {0};
    uint64_t rec_e[TEST_RUN] = {0}; unsigned len_e[1] = {0};
    uint64_t rec_d[TEST_RUN] = {0}; unsigned len_d[1] = {0};
    char mesg[256];
#if defined(DO_BENCHMARK)
    bm_init(NULL);
#endif

    int passed = 1;
    for(int i=0;i<TEST_RUN;i++) {
        REC_TIMING( rec_k , len_k , {
        kem_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes);
    	//crypto_kem_keypair(pk, sk); 
        });
        REC_TIMING( rec_e , len_e , {
        kem_enc(pk, pk_len_bytes, key1, &key1_len_bytes, ct, &ct_len_bytes);
        //crypto_kem_enc(ct, key1, pk); 
        });
        REC_TIMING( rec_d , len_d , {
        kem_dec(sk, sk_len_bytes, ct, ct_len_bytes, key2, &key2_len_bytes);
	    //crypto_kem_dec(key2, ct, sk);
        });

        if(!vec_equal(key1, key2, SHARED_SECRET_BYTES)) {
            printf("[%d] Error: key1 != key2\n", i );
            passed = 0;
            break;
        }
    }

	printf("\n\nsecret1: ");
	for(int i = 0 ; i < SHARED_SECRET_BYTES ; ++i) printf("%x", key1[i]);

	printf("\nsecret2: ");
	for(int i = 0 ; i < SHARED_SECRET_BYTES ; ++i) printf("%x", key2[i]);
	printf("\n\n");

    report(mesg,sizeof(mesg),rec_k,len_k[0]);
    printf("Keygen: %s\n", mesg);
    report(mesg,sizeof(mesg),rec_e,len_e[0]);
    printf("Encaps: %s\n", mesg);
    report(mesg,sizeof(mesg),rec_d,len_d[0]);
    printf("Decaps: %s\n", mesg);
    printf("\n");
    printf("TEST [%d] %s\n", TEST_RUN ,  passed ? "PASSED" : "FAILED");

///////////////////////////////generate polynomial of RS code/////////////////////////////////////////////////////
    
    int n = 48;//n=2delta n次多项式
    uint8_t poly[256];
    compute_poly(n, poly);

    //输出poly数组
    print_poly_generate(poly,n);

////////////////////////////////////////////PKE-test////////////////////////////////////////////////////////////

    // uint8_t seed_pke[SEED_BYTES] = {0};
    // uint8_t ek_pke[PUBLIC_KEY_BYTES] = {0};
    // uint8_t dk_pke[SEED_BYTES] = {0};

    // uint8_t m[PARAM_SECURITY_BYTES] = {0};uint8_t m_prime[PARAM_SECURITY_BYTES] = {0};
    // uint8_t theta[SEED_BYTES] = {0};
    // ciphertext_pke_t c_pke;
    // // Sample message m
    // prng_get_bytes(m, PARAM_SECURITY_BYTES);
    // // Sample seed
    // hqc_pke_encrypt(&c_pke, ek_pke, (uint64_t *)m, theta);
    // printf("\nQUBE-PKE encryption completed.\n");
    // hqc_pke_decrypt((uint64_t *)m_prime, dk_pke, &c_pke);
    // printf("\nQUBE-PKE decryption completed.\n");

    // //输出原始明文和解密明文
    // printf("\n\nm: \n");
    // vect_print(m, PARAM_SECURITY_BYTES);
    // printf("\n\nm_prime: \n");
    // vect_print(m_prime, PARAM_SECURITY_BYTES);
    // printf("\n");

    // if(vec_equal(m, m_prime, PARAM_SECURITY_BYTES)) {
    //         printf("pass\n");
    //     }

    
    // prng_get_bytes(m, PARAM_SECURITY_BYTES);
    // passed = 1;
    // for(int i=0;i<TEST_RUN;i++) {
    //     // Sample message m
        
    //     prng_get_bytes(theta, SEED_BYTES);
    // 	hqc_pke_keygen(ek_pke, dk_pke, seed_pke); 
        
    //     hqc_pke_encrypt(&c_pke, ek_pke, m, theta);

	//     hqc_pke_decrypt(m_prime, dk_pke, &c_pke);

    //     if(!vec_equal(m, m_prime, PARAM_SECURITY_BYTES)) {
    //         printf("\n\n[%d] Error: m != m_prime\n", i );
    //         passed = 0;
    //         break;
    //     }
    // }
    // if(passed==1) printf("\n✅ 测试通过!\n");

	// printf("\nm: \n");
	// for(int i = 0 ; i < PARAM_SECURITY_BYTES ; ++i) printf("%x", m[i]);

	// printf("\nm_prime: \n");
	// for(int i = 0 ; i < PARAM_SECURITY_BYTES ; ++i) printf("%x", m_prime[i]);
	// printf("\n\n");
///////////////////////////////////////////////////////////////////////////
    uint8_t m[PARAM_SECURITY_BYTES] = {0};
    uint8_t mm[PARAM_SECURITY_BYTES] = {0};
    uint64_t m_rs_enc[VEC_N1_SIZE_64] = {0};
    uint64_t em[VEC_N1N2_256_SIZE_64];
    uint64_t m_rs_dec[VEC_N1_SIZE_64] = {0};
    
    for(int i=0;i<TEST_RUN;i++)
    {
        CryptoRandomBytes(m_rs_enc, VEC_N1_SIZE_64);
        
        reed_muller_encode(em,m_rs_enc);
        reed_muller_decode(m_rs_dec,em);
        
        if(!vec_equal(m_rs_enc, m_rs_dec, VEC_N1_SIZE_64)) {
            printf("[%d] RM Error: m != mm\n", i );
            passed = 0;
            printf("\n\nm: ");
            vect_print((uint64_t *)m_rs_enc, VEC_N1_SIZE_64);
            printf("\n\nm: ");
            vect_print((uint64_t *)m_rs_dec, VEC_N1_SIZE_64);
            printf("\n\n");
            break;
        }
    }

    for(int i=0;i<TEST_RUN;i++)
    {
        CryptoRandomBytes(m, PARAM_SECURITY_BYTES);
        
        reed_solomon_encode(m_rs_enc,m);
        reed_solomon_decode(mm,m_rs_enc);
        
        if(!vec_equal(m, mm, PARAM_SECURITY_BYTES)) {
            printf("[%d] RS Error: m != mm\n", i );
            passed = 0;
            printf("\n\nm: ");
            vect_print((uint64_t *)m, PARAM_SECURITY_BYTES);
            printf("\n\nm: ");
            vect_print((uint64_t *)mm, PARAM_SECURITY_BYTES);
            printf("\n\n");
            break;
        }
    }
    
    
    
///////////////////////////////////////////////table//////////////////////////////////////////////////////////////
    freopen("../table.txt", "w", stdout);

    build_table();
    print_table();

    fclose(stdout);
/////////////////////////////////////////////////////////////////////////

    

    return 0;
}
