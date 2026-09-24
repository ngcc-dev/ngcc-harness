/*
 * test_rsdp.c - RSDP layer (CROSS conventions): ExpandSK determinism,
 * syndrome H=[V|I], F_p / F_z bit-packing roundtrips, and the restricted /
 * pad-bit rejection that rsdp_unpack_fz enforces.
 */
#include "rsdp.h"
#include "restr.h"
#include "fq_arith.h"
#include "params.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

int main(void)
{
    restr_init();

    uint8_t seed_sk[PARAM_KEYSEED_BYTES];
    for (int i = 0; i < PARAM_KEYSEED_BYTES; i++) seed_sk[i] = (uint8_t)(7 * i + 1);

    /* 1. ExpandSK determinism + range */
    uint8_t eta1[PARAM_N], eta2[PARAM_N];
    fq_t V1[PARAM_R * PARAM_K], V2[PARAM_R * PARAM_K];
    rsdp_expand_sk(eta1, V1, seed_sk);
    rsdp_expand_sk(eta2, V2, seed_sk);
    assert(memcmp(eta1, eta2, PARAM_N) == 0);
    assert(memcmp(V1, V2, sizeof(V1)) == 0);
    for (int i = 0; i < PARAM_N; i++) assert(eta1[i] < PARAM_Z);
    for (int i = 0; i < PARAM_R * PARAM_K; i++) assert(V1[i] < PARAM_Q);

    /* 2. Syndrome s = e H^T = V*e[0:k] + e[k:n], recomputed independently */
    fq_t e[PARAM_N];
    restr_vec_from_exp(e, eta1, PARAM_N);
    for (int i = 0; i < PARAM_N; i++) {        /* e is restricted (in E) */
        int in_E = 0;
        for (int x = 0; x < PARAM_Z; x++) if (e[i] == restr_val((uint8_t)x)) in_E = 1;
        assert(in_E);
    }
    fq_t s[PARAM_R];
    rsdp_compute_syndrome(s, V1, e);
    for (int i = 0; i < PARAM_R; i++) {
        uint64_t acc = 0;
        for (int j = 0; j < PARAM_K; j++)
            acc += (uint64_t)V1[i * PARAM_K + j] * (uint64_t)e[j];
        fq_t man = (fq_t)(acc % PARAM_Q);
        man = fq_add(man, e[PARAM_K + i]);
        assert(man == s[i]);
    }

    /* 3. F_p pack/unpack roundtrip (used for s and y) */
    fq_t y[PARAM_N], y2[PARAM_N];
    for (int i = 0; i < PARAM_N; i++) y[i] = (fq_t)((13 * i + 5) % PARAM_Q);
    uint8_t yb[QINGLUAN_Y_BYTES];
    rsdp_pack_fp(yb, y, PARAM_N);
    rsdp_unpack_fp(y2, yb, PARAM_N);
    assert(memcmp(y, y2, sizeof(y)) == 0);

    /* 4. F_z pack/unpack roundtrip + checks */
    uint8_t v[PARAM_N], v2[PARAM_N];
    for (int i = 0; i < PARAM_N; i++) v[i] = (uint8_t)((5 * i + 2) % PARAM_Z);
    uint8_t vb[QINGLUAN_V_BYTES];
    rsdp_pack_fz(vb, v, PARAM_N);
    assert(rsdp_unpack_fz(v2, vb, PARAM_N) == 0);
    assert(memcmp(v, v2, PARAM_N) == 0);

    /* 4a. exponent >= z must be rejected (restricted-membership check) */
    uint8_t vbad[PARAM_N];
    memcpy(vbad, v, PARAM_N);
    vbad[0] = (uint8_t)PARAM_Z;          /* 7 == z, invalid exponent */
    uint8_t vbb[QINGLUAN_V_BYTES];
    rsdp_pack_fz(vbb, vbad, PARAM_N);
    uint8_t vtmp[PARAM_N];
    assert(rsdp_unpack_fz(vtmp, vbb, PARAM_N) == -1);

    /* 4b. nonzero high pad bit in the final byte must be rejected */
    uint8_t vbp[QINGLUAN_V_BYTES];
    rsdp_pack_fz(vbp, v, PARAM_N);
    vbp[QINGLUAN_V_BYTES - 1] |= 0x80;
    assert(rsdp_unpack_fz(vtmp, vbp, PARAM_N) == -1);

    printf("test_rsdp OK\n");
    return 0;
}
