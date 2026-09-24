#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "parameters.h"
#include "reed_solomon.h"

static int run_case(unsigned errors, unsigned erasures_count, uint8_t salt) {
    uint8_t msg[PARAM_K];
    uint8_t out[PARAM_K];
    uint8_t erasures[PARAM_N1];
    uint64_t cdw[VEC_N1_SIZE_64];

    memset(cdw, 0, sizeof cdw);
    memset(out, 0, sizeof out);
    memset(erasures, 0, sizeof erasures);
    for (unsigned i = 0; i < PARAM_K; ++i) msg[i] = (uint8_t)(salt + 29U * i);

    reed_solomon_encode(cdw, msg);
    uint8_t *bytes = (uint8_t *)(void *)cdw;
    unsigned pos = 0;
    for (unsigned i = 0; i < errors; ++i, ++pos) bytes[pos] ^= (uint8_t)(0xA5U ^ (uint8_t)i);
    for (unsigned i = 0; i < erasures_count; ++i, ++pos) {
        bytes[pos] ^= (uint8_t)(0x5AU ^ (uint8_t)i);
        erasures[pos] = 1U;
    }
    reed_solomon_decode(out, cdw, erasures);
    return memcmp(msg, out, PARAM_K) == 0 ? 0 : 1;
}

int main(void) {
    const unsigned parity = PARAM_N1 - PARAM_K;
    const unsigned errors_at_radius = parity / 2U;
    const unsigned erasures_at_radius = parity & 1U;

    if (run_case(0U, parity, 0x11U) != 0) {
        puts("rs_boundary_all_erasures_fail");
        return 1;
    }
    if (parity >= 2U && run_case(1U, parity - 2U, 0x33U) != 0) {
        puts("rs_boundary_mixed_fail");
        return 2;
    }
    if (run_case(errors_at_radius, erasures_at_radius, 0x55U) != 0) {
        puts("rs_boundary_radius_fail");
        return 3;
    }
    puts("rs_boundary_pass");
    return 0;
}
