#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "KEX_AlgorithmInstance.h"
#include "drng.h"
#include "primitive_interfaces.h"

#define SEED_LEN_BYTES 64

#define aligned_free_32(ptr) free(ptr)

DRNG_ctx drng_algorithm;

static void *aligned_calloc_32(size_t n, size_t size)
{
    size_t bytes = n * size;
    void *ptr = NULL;

    if (posix_memalign(&ptr, 32, bytes) != 0)
        return NULL;

    memset(ptr, 0, bytes);
    return ptr;
}

static double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

int main(void)
{
    const int N = 10000;
    int i, ret;
    unsigned long long pka_l, ska_l, sta_l, pkb_l, skb_l, stb_l, m1_l, m2_l;
    double t_init_a = 0.0, t_init_b = 0.0, t_pass1 = 0.0, t_pass2 = 0.0, t_derive = 0.0;
   
    unsigned char *seed = (unsigned char *)calloc(SEED_LEN_BYTES, sizeof(unsigned char));

    unsigned char *pka, *ska, *pkb, *skb, *sta, *stb, *ssa, *ssb, *m1, *m2;
	unsigned long long pass, pka_len_bytes, ska_len_bytes, pkb_len_bytes,
		skb_len_bytes, sta_len_bytes, stb_len_bytes, ssa_len_bytes, ssb_len_bytes,
		total_len_bytes;

    pass = kex_get_passes_num();
	pka_len_bytes = kex_get_pk_len_bytes_initiator();
	ska_len_bytes = kex_get_sk_len_bytes_initiator();
	pkb_len_bytes = kex_get_pk_len_bytes_responder();
	skb_len_bytes = kex_get_sk_len_bytes_responder();
	sta_len_bytes = kex_get_sta_len_bytes();
	stb_len_bytes = kex_get_stb_len_bytes();
	ssa_len_bytes = kex_get_ss_len_bytes();
	ssb_len_bytes = kex_get_ss_len_bytes();
	total_len_bytes = kex_get_total_msg_len_bytes();
	pka = (unsigned char *)aligned_calloc_32(pka_len_bytes, sizeof(unsigned char));
	ska = (unsigned char *)aligned_calloc_32(ska_len_bytes, sizeof(unsigned char));
	pkb = (unsigned char *)aligned_calloc_32(pkb_len_bytes, sizeof(unsigned char));
	skb = (unsigned char *)aligned_calloc_32(skb_len_bytes, sizeof(unsigned char));
	sta = (unsigned char *)aligned_calloc_32(sta_len_bytes, sizeof(unsigned char));
	stb = (unsigned char *)aligned_calloc_32(stb_len_bytes, sizeof(unsigned char));
	ssa = (unsigned char *)aligned_calloc_32(ssa_len_bytes, sizeof(unsigned char));
	ssb = (unsigned char *)aligned_calloc_32(ssb_len_bytes, sizeof(unsigned char));
	m1 = (unsigned char *)aligned_calloc_32(total_len_bytes, sizeof(unsigned char));
	m2 = (unsigned char *)aligned_calloc_32(total_len_bytes, sizeof(unsigned char));
	//ma = NULL;
	//mb = NULL;

    printf("=== %s speed test ===\n", ALGORITHM_INSTANCE);
    printf("iterations: %d\n", N);
    // init drng_algorithm using seed
	init_random_number(&drng_algorithm, seed, SEED_LEN_BYTES);
    
    double t0, t1= now_sec();
    for (i = 0; i < N; ++i) {
        
        stb_l = 0;
        pka_l = 0;
        ska_l = 0;
        pkb_l = 0;
        skb_l = 0;
        sta_l = 0;
        stb_l = 0;
        m1_l = 0;
        m2_l = 0;

        t0 = now_sec();
        ret = kex_init_a(pka, &pka_l, ska, &ska_l, sta, &sta_l);
        t1 = now_sec();
        if (ret != 0) return 1;
        t_init_a += (t1 - t0);

        t0 = now_sec();
        ret = kex_init_b(pkb, &pkb_l, skb, &skb_l, stb, &stb_l);
        t1 = now_sec();
        if (ret != 0) return 2;
        t_init_b += (t1 - t0);

        t0 = now_sec();
        ret = kex_generate_pass1_msg_a(ska, ska_l, pkb, pkb_l, sta, &sta_l, m1, &m1_l);
        t1 = now_sec();
        if (ret != 0) return 3;
        t_pass1 += (t1 - t0);

        t0 = now_sec();
        ret = kex_generate_pass2_msg_b(skb, skb_l, pka, pka_l, m1, m1_l, stb, &stb_l, m2, &m2_l);
        ret = kex_derive_ss_b(skb, skb_l, pka, pka_l, m1, m1_l, stb, stb_l, ssb, &ssb_len_bytes);
        t1 = now_sec();
        if (ret < 0) return 4;
        t_pass2 += (t1 - t0);

        t0 = now_sec();
        ret = kex_derive_ss_a(ska, ska_l, pkb, pkb_l, m2, m2_l, sta, sta_l, ssa, &ssa_len_bytes);
        t1 = now_sec();
        if (ret != 0) return 5;
        t_derive += (t1 - t0);
    }

    printf("avg_init_a   : %.2f us\n", t_init_a * 1e6 / N);
    printf("avg_init_b   : %.2f us\n", t_init_b * 1e6 / N);
    printf("avg_pass1_a  : %.2f us\n", t_pass1 * 1e6 / N);
    printf("avg_pass2&derive_b  : %.2f us\n", t_pass2 * 1e6 / N);
    printf("avg_derive_a : %.2f us\n", t_derive * 1e6 / N);
    
    printf("\ninitiator online : %.2f us\n", t_pass1 * 1e6 / N + t_derive * 1e6 / N);
    printf("responder online : %.2f us\n", t_pass2 * 1e6 / N);
    printf("total online (A+B online full round) : %.2f us\n", (t_pass1 + t_pass2 + t_derive) * 1e6 / N);

    printf("\nkex pass : %lld\n", pass);
    printf("initiator bandwidth : %lld Bytes\n", m1_l);
    printf("responder bandwidth : %lld Bytes\n", m2_l);
    printf("============== speed test done ==============\n");

    free(seed);
	free(m1);
	free(m2);
	free(ssa);
	free(ssb);
	free(stb);
	free(sta);
	free(skb);
	free(pkb);
	free(ska);
	free(pka);
    return 0;
}
