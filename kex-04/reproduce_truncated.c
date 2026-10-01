#include <stdlib.h>

#include "KEX_AlgorithmInstance.h"
#include "drng.h"

DRNG_ctx drng_algorithm;

int main(void) {
    unsigned char seed[55] = {0}, dummy[1] = {0};
    unsigned long long pk_cap = kex_get_pk_len_bytes();
    unsigned long long sk_cap = kex_get_sk_len_bytes();
    unsigned long long sta_cap = kex_get_sta_len_bytes();
    unsigned long long stb_cap = kex_get_stb_len_bytes();
    unsigned long long msg_cap = kex_get_total_msg_len_bytes();
    unsigned char *pkb = calloc(1, pk_cap), *skb = calloc(1, sk_cap);
    unsigned char *sta = calloc(1, sta_cap), *stb = calloc(1, stb_cap);
    unsigned char *m1 = calloc(1, msg_cap), *m2 = calloc(1, msg_cap);
    unsigned char *short_m1 = malloc(1);
    unsigned long long pkb_len=0, skb_len=0, stb_len=0, sta_len=0;
    unsigned long long m1_len=0, m2_len=0;
    if (!pkb || !skb || !sta || !stb || !m1 || !m2 || !short_m1) return 2;
    if (init_random_number(&drng_algorithm, seed, sizeof seed)) return 3;
    if (kex_init_b(pkb, &pkb_len, skb, &skb_len, stb, &stb_len)) return 4;
    if (kex_generate_pass1_msg_a(dummy, 0, pkb, pkb_len, sta, &sta_len, m1, &m1_len)) return 5;
    short_m1[0] = m1[0];
    return kex_generate_pass2_msg_b(skb, skb_len, dummy, 0, short_m1, 1,
                                    stb, &stb_len, m2, &m2_len) < 0;
}
