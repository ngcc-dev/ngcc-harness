/* Source-linked component witness for kem-26-5, not a timing/key-recovery PoC.
 * Include the frozen submitted decoder so its static functions are exercised
 * unchanged. Build with the reference HQC-128 parameters.
 */
#include "Implementations and Test_Vectors/Implementations/Reference_Implementation/HQC-128/code_layer.c"

#include <stdio.h>
#include <string.h>

static int count_trials(uint8_t *out, const uint8_t *received,
                        const uint8_t *erased, unsigned *trials)
{
    unsigned erasures = 0;
    unsigned i;
    int errors;
    for (i = 0; i < NSS_HQC_N1; ++i) erasures += erased[i] != 0;
    *trials = 0;
    errors = (int)((NSS_HQC_N1 - NSS_HQC_K1 - erasures) / 2u);
    for (; errors >= 0; --errors) {
        ++*trials;
        if (rs_try_decode(out, received, erased, erasures, (unsigned)errors) == 0)
            return 0;
    }
    return -1;
}

int main(void)
{
    uint8_t msg[NSS_HQC_K1] = {0};
    uint8_t rs[RS_MAX_N], changed[RS_MAX_N], erased[RS_MAX_N] = {0};
    uint8_t out[RS_MAX_N], check[RS_MAX_N];
    unsigned counts[32], distinct = 0, i, j;
    uint32_t state = 0x79b9c1a5u;
    int rc, direct;

    for (i = 0; i < NSS_HQC_K1; ++i) msg[i] = (uint8_t)(17u * i + 3u);
    rs_eval_encode(rs, msg);
    for (i = 0; i < 32; ++i) {
        memcpy(changed, rs, NSS_HQC_N1);
        if (i < 16) {
            for (j = 0; j < i; ++j) changed[j] ^= (uint8_t)(j + 1u);
        } else {
            for (j = 0; j < NSS_HQC_N1; ++j) {
                state ^= state << 13;
                state ^= state >> 17;
                state ^= state << 5;
                changed[j] = (uint8_t)state;
            }
        }
        rc = count_trials(out, changed, erased, &counts[i]);
        direct = rs_decode(check, changed, erased);
        if (rc != direct || (rc == 0 && memcmp(out, check, NSS_HQC_K1) != 0)) {
            fprintf(stderr, "decoder result mismatch at %u errors\n", i);
            return 1;
        }
        if (i && counts[i] != counts[0]) distinct = 1;
    }
    printf("clean_trials=%u corrupted_10_trials=%u random_word_trials=%u\n",
           counts[0], counts[10], counts[16]);
    if (!distinct) {
        fputs("no input-dependent trial count observed\n", stderr);
        return 1;
    }
    puts("COMPONENT kem-26-5 CONFIRMED: decoder trial count varies");
    puts("These received words are not a secret-key timing or recovery witness.");
    return 0;
}
