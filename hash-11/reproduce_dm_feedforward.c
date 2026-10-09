/* Native witness for hash-11-2. Include the submitted reference source so its
 * static absorption and permutation functions are the functions under test. */
#include <stdio.h>
#include <string.h>
#include "Garnet_1024.c"

static const int rate_cells[7] = {0, 3, 5, 6, 9, 10, 15};

static int same_state(const struct garnet_u128_t *a,
                      const struct garnet_u128_t *b) {
    for (int i = 0; i < 16; ++i) {
        if (a[i].v[0] != b[i].v[0] || a[i].v[1] != b[i].v[1]) return 0;
    }
    return 1;
}

int main(void) {
    for (int trial = 0; trial < 4; ++trial) {
        struct garnet_u128_t old[16], actual[16], x[16], specified[16];
        struct garnet_u128_t code_formula[16];
        struct garnet_u128_t expected_delta[16] = {{0}};
        struct garnet_u128_t message[7], capacity[9], zero_key = {0};

        for (int i = 0; i < 16; ++i) {
            old[i].v[0] = UINT64_C(0x9e3779b97f4a7c15) * (i + 1 + trial);
            old[i].v[1] = UINT64_C(0xd1b54a32d192ed03) * (i + 3 + trial);
        }
        for (int i = 0; i < 7; ++i) {
            message[i].v[0] = UINT64_C(0x94d049bb133111eb) * (i + 5 + trial);
            message[i].v[1] = UINT64_C(0x2545f4914f6cdd1d) * (i + 7 + trial);
        }
        if (trial == 0) {
            /* Control: with S_r = 0, both feed-forward formulas agree. */
            for (int i = 0; i < 7; ++i) old[rate_cells[i]] = zero_key;
        }

        memcpy(actual, old, sizeof actual);
        keep_capacity5_4x4_st(actual, capacity);
        absorb_message5_4x4_st(actual, message);
        for (int round = 0; round < 12; ++round)
            permutation_p_4x4_st(actual, zero_key, round);
        absorb_message5_4x4_st(actual, message);
        absorb_capacity5_4x4_st(actual, capacity);

        memcpy(x, old, sizeof x);
        absorb_message5_4x4_st(x, message);
        memcpy(specified, x, sizeof specified);
        for (int round = 0; round < 12; ++round)
            permutation_p_4x4_st(specified, zero_key, round);
        for (int i = 0; i < 16; ++i)
            specified[i] = xor128(specified[i], x[i]);

        for (int i = 0; i < 7; ++i)
            expected_delta[rate_cells[i]] = old[rate_cells[i]];
        memcpy(code_formula, specified, sizeof code_formula);
        for (int i = 0; i < 16; ++i)
            code_formula[i] = xor128(code_formula[i], expected_delta[i]);
        if (!same_state(actual, code_formula) ||
            (trial == 0) != same_state(actual, specified)) {
            fprintf(stderr, "hash-11-2: unexpected trial %d result\n", trial);
            return 1;
        }
    }
    puts("CONFIRMED hash-11-2: reference update omits old rate-state feed-forward");
    return 0;
}
