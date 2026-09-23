#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api.h"
#include "code.h"
#include "drng.h"
#include "gf2x.h"
#include "parameters.h"
#include "qube.h"
#include "symmetric.h"
#include "vector.h"

DRNG_ctx drng_algorithm;

static void set_bit(uint64_t *v, uint32_t pos) {
    v[pos >> 6] |= UINT64_C(1) << (pos & 63U);
}

static void toggle_bit(uint64_t *v, uint32_t pos) {
    v[pos >> 6] ^= UINT64_C(1) << (pos & 63U);
}

static int get_bit(const uint64_t *v, uint32_t pos) {
    return (int)((v[pos >> 6] >> (pos & 63U)) & 1U);
}

static int bytes_equal(const uint8_t *a, const uint8_t *b, size_t len) {
    return memcmp(a, b, len) == 0;
}

static void trace_step(const char *name) {
    if (getenv("QUBE_TEST_VERBOSE") != NULL) {
        fprintf(stderr, "%s\n", name);
        fflush(stderr);
    }
}

static int test_pack_roundtrip(void) {
    uint8_t in_r[VEC_N_SIZE_BYTES];
    uint8_t out_r[VEC_N_SIZE_BYTES];
    uint8_t in_l[VEC_N1N2_SIZE_BYTES];
    uint8_t out_l[VEC_N1N2_SIZE_BYTES];
    uint64_t v_r[VEC_N_SIZE_64];
    uint64_t v_l[VEC_N1N2_SIZE_64];

    for (size_t i = 0; i < sizeof in_r; i++) {
        in_r[i] = (uint8_t)(17U * i + 3U);
    }
    if ((PARAM_N & 7U) != 0) {
        in_r[sizeof in_r - 1] &= (uint8_t)((1U << (PARAM_N & 7U)) - 1U);
    }

    for (size_t i = 0; i < sizeof in_l; i++) {
        in_l[i] = (uint8_t)(31U * i + 11U);
    }

    vect_from_bytes(v_r, in_r, PARAM_N);
    vect_to_bytes(out_r, v_r, PARAM_N);
    vect_from_bytes(v_l, in_l, PARAM_N1N2);
    vect_to_bytes(out_l, v_l, PARAM_N1N2);

    if (!bytes_equal(in_r, out_r, sizeof in_r) || !bytes_equal(in_l, out_l, sizeof in_l)) {
        fprintf(stderr, "pack roundtrip failed\n");
        return 1;
    }
    return 0;
}

static int test_sampling(void) {
    static const uint16_t weights[] = {
        PARAM_OMEGA_Y1, PARAM_OMEGA_X1, PARAM_OMEGA_R21, PARAM_OMEGA_R11, PARAM_OMEGA_E,
    };
    uint8_t seed[SEED_BYTES];
    uint64_t v[VEC_N_SIZE_64];
    uint32_t support[PARAM_OMEGA_MAX];

    for (size_t i = 0; i < sizeof seed; i++) {
        seed[i] = (uint8_t)(0xA5U ^ (uint8_t)i);
    }

    for (size_t w = 0; w < sizeof weights / sizeof weights[0]; w++) {
        qube_xof_stream_t xof;
        uint64_t seen[VEC_N_SIZE_64] = {0};
        if (qube_xof_stream_init(&xof, seed) != 0 ||
            vect_sample_fixed_weight(&xof, v, support, weights[w]) != 0) {
            fprintf(stderr, "sampling call failed\n");
            return 1;
        }
        qube_xof_stream_release(&xof);

        if (vect_weight(v, VEC_N_SIZE_64) != weights[w]) {
            fprintf(stderr, "sample weight mismatch\n");
            return 1;
        }
        if ((v[VEC_N_SIZE_64 - 1] & ~QUBE_TAIL_MASK(PARAM_N)) != 0) {
            fprintf(stderr, "sample tail bits not masked\n");
            return 1;
        }
        for (uint16_t i = 0; i < weights[w]; i++) {
            if (support[i] >= PARAM_N || get_bit(seen, support[i]) != 0 || get_bit(v, support[i]) == 0) {
                fprintf(stderr, "sample support invalid\n");
                return 1;
            }
            set_bit(seen, support[i]);
        }
        seed[0] ^= (uint8_t)(w + 1U);
    }
    return 0;
}

static int test_ring_shifts(void) {
    uint64_t dense[VEC_N_SIZE_64] = {0};
    uint64_t out[VEC_N_SIZE_64] = {0};
    uint64_t expected[VEC_N_SIZE_64] = {0};
    uint32_t support[8];
    const uint32_t positions[] = {0, 1, 63, 64, PARAM_N - 1U};

    for (size_t i = 0; i < sizeof positions / sizeof positions[0]; i++) {
        set_bit(dense, positions[i]);
    }

    support[0] = 0;
    ring_mul_by_support(out, dense, support, 1);
    if (memcmp(out, dense, sizeof out) != 0) {
        fprintf(stderr, "ring multiply by one failed\n");
        return 1;
    }

    memset(expected, 0, sizeof expected);
    support[0] = 1;
    for (size_t i = 0; i < sizeof positions / sizeof positions[0]; i++) {
        set_bit(expected, (positions[i] + 1U) % PARAM_N);
    }
    ring_mul_by_support(out, dense, support, 1);
    if (memcmp(out, expected, sizeof out) != 0) {
        fprintf(stderr, "ring shift by one failed\n");
        return 1;
    }

    memset(expected, 0, sizeof expected);
    support[0] = PARAM_N - 1U;
    for (size_t i = 0; i < sizeof positions / sizeof positions[0]; i++) {
        set_bit(expected, (positions[i] + PARAM_N - 1U) % PARAM_N);
    }
    ring_mul_by_support(out, dense, support, 1);
    if (memcmp(out, expected, sizeof out) != 0) {
        fprintf(stderr, "ring wrap shift failed\n");
        return 1;
    }

    memset(dense, 0, sizeof dense);
    memset(expected, 0, sizeof expected);
    support[0] = 0;
    support[1] = 3;
    support[2] = 65;
    support[3] = PARAM_N - 2U;
    support[4] = PARAM_N / 2U;
    support[5] = (PARAM_N / 2U) + 17U;
    support[6] = 127;
    support[7] = PARAM_N - 1U;
    for (uint32_t pos = 0; pos < PARAM_N; pos += 97U) {
        set_bit(dense, pos);
    }
    for (uint32_t pos = 11; pos < PARAM_N; pos += 211U) {
        set_bit(dense, pos);
    }
    for (uint32_t pos = 0; pos < PARAM_N; pos++) {
        if (get_bit(dense, pos) == 0) {
            continue;
        }
        for (size_t i = 0; i < sizeof support / sizeof support[0]; i++) {
            toggle_bit(expected, (pos + support[i]) % PARAM_N);
        }
    }
    ring_mul_by_support(out, dense, support, (uint16_t)(sizeof support / sizeof support[0]));
    if (memcmp(out, expected, sizeof out) != 0) {
        fprintf(stderr, "ring sparse oracle failed\n");
        return 1;
    }
    return 0;
}

static int test_code_roundtrip(void) {
    uint8_t m[PARAM_SECURITY_BYTES];
    uint8_t dec[PARAM_SECURITY_BYTES] = {0};
    uint64_t em[VEC_N1N2_SIZE_64] = {0};

    for (size_t i = 0; i < sizeof m; i++) {
        m[i] = (uint8_t)(0xC3U + 7U * i);
    }
    code_encode(em, m);
    code_decode(dec, em);

    if (memcmp(m, dec, sizeof m) != 0) {
        fprintf(stderr, "code roundtrip failed\n");
        return 1;
    }
    return 0;
}

static int test_pke_deterministic(void) {
    uint8_t rho[SEED_BYTES];
    uint8_t rho_c[SEED_BYTES];
    uint8_t pk1[PUBLIC_KEY_BYTES] = {0};
    uint8_t pk2[PUBLIC_KEY_BYTES] = {0};
    uint8_t sk1[SEED_BYTES] = {0};
    uint8_t sk2[SEED_BYTES] = {0};
    uint8_t m[PARAM_SECURITY_BYTES];
    uint8_t dec[PARAM_SECURITY_BYTES] = {0};
    ciphertext_pke_t c1;
    ciphertext_pke_t c2;

    memset(&c1, 0, sizeof c1);
    memset(&c2, 0, sizeof c2);
    for (size_t i = 0; i < SEED_BYTES; i++) {
        rho[i] = (uint8_t)(0x11U + 13U * i);
        rho_c[i] = (uint8_t)(0x77U + 5U * i);
    }
    for (size_t i = 0; i < PARAM_SECURITY_BYTES; i++) {
        m[i] = (uint8_t)(0x21U + 9U * i);
    }

    if (qube_pke_keygen(pk1, sk1, rho) != 0 ||
        qube_pke_keygen(pk2, sk2, rho) != 0 ||
        memcmp(pk1, pk2, sizeof pk1) != 0 ||
        memcmp(sk1, sk2, sizeof sk1) != 0) {
        fprintf(stderr, "PKE keygen determinism failed\n");
        return 1;
    }
    if (qube_pke_encrypt(&c1, pk1, m, rho_c) != 0 ||
        qube_pke_encrypt(&c2, pk1, m, rho_c) != 0 ||
        memcmp(&c1, &c2, sizeof c1) != 0) {
        fprintf(stderr, "PKE encryption determinism failed\n");
        return 1;
    }
    if (qube_pke_decrypt(dec, sk1, &c1) != 0 || memcmp(m, dec, sizeof m) != 0) {
        fprintf(stderr, "PKE decrypt failed\n");
        return 1;
    }
    return 0;
}

int main(void) {
    trace_step("pack");
    if (test_pack_roundtrip()) {
        return 1;
    }
    trace_step("sampling");
    if (test_sampling()) {
        return 1;
    }
    trace_step("ring");
    if (test_ring_shifts()) {
        return 1;
    }
    trace_step("code");
    if (test_code_roundtrip()) {
        return 1;
    }
    trace_step("pke");
    if (test_pke_deterministic()) {
        return 1;
    }
    trace_step("done");

    printf("%s component tests: PASS\n", CRYPTO_ALGNAME);
    return 0;
}
