#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "KEX_AlgorithmInstance.h"
#include "drng.h"

DRNG_ctx drng_algorithm;

int main(void)
{
    static const uint8_t seed[48] = {
        0x41, 0x46, 0x53, 0x2d, 0x4b, 0x45, 0x58, 0x2d,
        0x72, 0x65, 0x73, 0x6f, 0x75, 0x72, 0x63, 0x65,
        0x2d, 0x74, 0x65, 0x73, 0x74, 0x2d, 0x73, 0x65,
        0x65, 0x64, 0x2d, 0x30, 0x31, 0x2d, 0x41, 0x46,
        0x53, 0x2d, 0x4b, 0x45, 0x58, 0x2d, 0x72, 0x65,
        0x73, 0x6f, 0x75, 0x72, 0x63, 0x65, 0x21, 0x21
    };
    unsigned long long pka_len = kex_get_pk_len_bytes();
    unsigned long long ska_len = kex_get_sk_len_bytes();
    unsigned long long pkb_len = pka_len;
    unsigned long long skb_len = ska_len;
    unsigned long long sta_len = kex_get_sta_len_bytes();
    unsigned long long stb_len = kex_get_stb_len_bytes();
    unsigned long long ssa_len = kex_get_ss_len_bytes();
    unsigned long long ssb_len = ssa_len;
    unsigned long long msg_capacity = kex_get_total_msg_len_bytes();
    unsigned long long msg_len[4] = {0, 0, 0, 0};
    unsigned char *pka = calloc((size_t)pka_len, 1);
    unsigned char *ska = calloc((size_t)ska_len, 1);
    unsigned char *pkb = calloc((size_t)pkb_len, 1);
    unsigned char *skb = calloc((size_t)skb_len, 1);
    unsigned char *sta = calloc((size_t)sta_len, 1);
    unsigned char *stb = calloc((size_t)stb_len, 1);
    unsigned char *ssa = calloc((size_t)ssa_len, 1);
    unsigned char *ssb = calloc((size_t)ssb_len, 1);
    unsigned char *msg[4] = {
        calloc((size_t)msg_capacity, 1),
        calloc((size_t)msg_capacity, 1),
        calloc((size_t)msg_capacity, 1),
        calloc((size_t)msg_capacity, 1)
    };
    unsigned char *last_a = NULL;
    unsigned char *last_b = NULL;
    unsigned long long last_a_len = 0;
    unsigned long long last_b_len = 0;
    int rc = 1;
    int status;

    if (pka == NULL || ska == NULL || pkb == NULL || skb == NULL ||
        sta == NULL || stb == NULL || ssa == NULL || ssb == NULL ||
        msg[0] == NULL || msg[1] == NULL || msg[2] == NULL || msg[3] == NULL)
        goto cleanup;
    if (init_random_number(&drng_algorithm, seed, sizeof(seed)) != 0)
        goto cleanup;
    if (kex_init_a(pka, &pka_len, ska, &ska_len, sta, &sta_len) < 0)
        goto cleanup;
    if (kex_init_b(pkb, &pkb_len, skb, &skb_len, stb, &stb_len) < 0)
        goto cleanup;

    status = kex_generate_pass1_msg_a(
        ska, ska_len, pkb, pkb_len, sta, &sta_len, msg[0], &msg_len[0]);
    if (status < 0)
        goto cleanup;
    last_a = msg[0];
    last_a_len = msg_len[0];

    if (status == 0) {
        status = kex_generate_pass2_msg_b(
            skb, skb_len, pka, pka_len, msg[0], msg_len[0],
            stb, &stb_len, msg[1], &msg_len[1]);
        if (status < 0)
            goto cleanup;
        last_b = msg[1];
        last_b_len = msg_len[1];
    }

    if (status == 0) {
        status = kex_generate_pass3_msg_a(
            ska, ska_len, pkb, pkb_len, msg[1], msg_len[1],
            sta, &sta_len, msg[2], &msg_len[2]);
        if (status < 0)
            goto cleanup;
        last_a = msg[2];
        last_a_len = msg_len[2];
    }

    if (status == 0) {
        status = kex_generate_pass4_msg_b(
            skb, skb_len, pka, pka_len, msg[2], msg_len[2],
            stb, &stb_len, msg[3], &msg_len[3]);
        if (status < 0)
            goto cleanup;
        last_b = msg[3];
        last_b_len = msg_len[3];
    }

    if (status != 1 || last_a == NULL || last_b == NULL)
        goto cleanup;
    if (kex_derive_ss_a(
            ska, ska_len, pkb, pkb_len, last_b, last_b_len,
            sta, sta_len, ssa, &ssa_len) != 0)
        goto cleanup;
    if (kex_derive_ss_b(
            skb, skb_len, pka, pka_len, last_a, last_a_len,
            stb, stb_len, ssb, &ssb_len) != 0)
        goto cleanup;
    if (ssa_len != ssb_len || memcmp(ssa, ssb, (size_t)ssa_len) != 0)
        goto cleanup;

    rc = 0;

cleanup:
    free(msg[3]);
    free(msg[2]);
    free(msg[1]);
    free(msg[0]);
    free(ssb);
    free(ssa);
    free(stb);
    free(sta);
    free(skb);
    free(pkb);
    free(ska);
    free(pka);
    return rc;
}
