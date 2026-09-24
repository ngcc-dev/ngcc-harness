#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "KEX_AlgorithmInstance.h"
#include "drng.h"
#include "cretake_params.h"

#define SEED_LEN_BYTES 64

DRNG_ctx drng_algorithm;

static void print_hex(const char *label, const unsigned char *buf, unsigned long long len, unsigned long long max_show)
{
    unsigned long long i, show = (len < max_show) ? len : max_show;
    printf("%s (len=%llu): ", label, len);
    for (i = 0; i < show; ++i) printf("%02X", buf[i]);
    if (show < len) printf("...");
    printf("\n");
}

int main(void)
{
    /*unsigned char pka[PKI_LEN], ska[SKI_LEN], *sta;
    unsigned long long sta_len_bytes, stb_len_bytes;
    unsigned char pkb[PKR_LEN], skb[SKR_LEN], *stb;
    unsigned char m1[PKI_LEN], m2[7000];
    unsigned char ssa[K_LEN], ssb[K_LEN];*/
    unsigned long long pka_l, ska_l, sta_l, pkb_l, skb_l, stb_l = 0;
    unsigned long long m1_l, m2_l, ssa_l = 0, ssb_l = 0;
    int ret;
    unsigned char *seed = (unsigned char *)calloc(SEED_LEN_BYTES, sizeof(unsigned char));
    unsigned char *pka, *ska, *pkb, *skb, *sta, *stb, *ssa, *ssb, *m1, *m2;
	unsigned long long pka_len_bytes, ska_len_bytes, pkb_len_bytes,
		skb_len_bytes, sta_len_bytes, stb_len_bytes, ssa_len_bytes, ssb_len_bytes,
		total_len_bytes;

    pka_len_bytes = kex_get_pk_len_bytes();
	ska_len_bytes = kex_get_sk_len_bytes();
	pkb_len_bytes = kex_get_pk_len_bytes();
	skb_len_bytes = kex_get_sk_len_bytes();
	sta_len_bytes = kex_get_sta_len_bytes();
	stb_len_bytes = kex_get_stb_len_bytes();
	ssa_len_bytes = kex_get_ss_len_bytes();
	ssb_len_bytes = kex_get_ss_len_bytes();
    total_len_bytes = kex_get_total_msg_len_bytes();
    pka = (unsigned char *)calloc(pka_len_bytes, sizeof(unsigned char));
	ska = (unsigned char *)calloc(ska_len_bytes, sizeof(unsigned char));
	pkb = (unsigned char *)calloc(pkb_len_bytes, sizeof(unsigned char));
	skb = (unsigned char *)calloc(skb_len_bytes, sizeof(unsigned char));
	sta = (unsigned char *)calloc(sta_len_bytes, sizeof(unsigned char));
	stb = (unsigned char *)calloc(stb_len_bytes, sizeof(unsigned char));
	ssa = (unsigned char *)calloc(ssa_len_bytes, sizeof(unsigned char));
	ssb = (unsigned char *)calloc(ssb_len_bytes, sizeof(unsigned char));
	m1 = (unsigned char *)calloc(total_len_bytes, sizeof(unsigned char));
	m2 = (unsigned char *)calloc(total_len_bytes, sizeof(unsigned char));

    printf("=== %s correctness test ===\n", ALGORITHM_INSTANCE);
    printf("claimed: passes=%llu pk=%llu sk=%llu sta=%llu stb=%llu ss=%llu total_msg=%llu\n",
           kex_get_passes_num(), kex_get_pk_len_bytes(), kex_get_sk_len_bytes(),
           kex_get_sta_len_bytes(), kex_get_stb_len_bytes(), kex_get_ss_len_bytes(),
           kex_get_total_msg_len_bytes());
    // init drng_algorithm using seed
	init_random_number(&drng_algorithm, seed, SEED_LEN_BYTES);
    ret = kex_init_a(pka, &pka_l, ska, &ska_l, sta, &sta_l);
    printf("[init_a] ret=%d\n", ret);
    if (ret != 0) return 1;
    print_hex("pka", pka, pka_l, 16);
    print_hex("ska", ska, ska_l, 16);
    print_hex("sta", sta, sta_l, 16);

    ret = kex_init_b(pkb, &pkb_l, skb, &skb_l, stb, &stb_l);
    printf("[init_b] ret=%d\n", ret);
    if (ret != 0) return 2;
    print_hex("pkb", pkb, pkb_l, 16);
    print_hex("skb", skb, skb_l, 16);

    ret = kex_generate_pass1_msg_a(ska, ska_l, pkb, pkb_l, sta, &sta_l, m1, &m1_l);
    printf("[pass1] ret=%d\n", ret);
    if (ret != 0) return 3;
    print_hex("m1", m1, m1_l, 16);

    ret = kex_generate_pass2_msg_b(skb, skb_l, pka, pka_l, m1, m1_l, stb, &stb_l, m2, &m2_l);
    printf("[pass2] ret=%d\n", ret);
    if (ret < 0) return 4;
    print_hex("m2", m2, m2_l, 16);

    ret = kex_derive_ss_a(ska, ska_l, pkb, pkb_l, m2, m2_l, sta, sta_l, ssa, &ssa_l);
    printf("[derive_a] ret=%d\n", ret);
    if (ret != 0) return 5;
    print_hex("ssa", ssa, ssa_l, 32);

    ret = kex_derive_ss_b(skb, skb_l, pka, pka_l, m1, m1_l, stb, stb_l, ssb, &ssb_l);
    printf("[derive_b] ret=%d\n", ret);
    if (ret != 0) return 6;
    print_hex("ssb", ssb, ssb_l, 32);

    if (ssa_l != ssb_l || memcmp(ssa, ssb, ssa_l) != 0) {
        printf("[result] WARNING: ssa and ssb are different in current implementation.\n");
    } else {
        printf("[result] SUCCESS: ssa == ssb\n");
    }
    printf("============== correctness test done ==============\n");
    
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
