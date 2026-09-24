#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "api.h"
#include "drng.h"
#include "hal.h"

#define SEED_LEN_BYTES 64
#ifndef RUN_COUNT
#define RUN_COUNT 100
#endif

DRNG_ctx drng_algorithm;

static const unsigned char benchmark_seed[SEED_LEN_BYTES] = {
    0x92, 0x7F, 0x06, 0xB5, 0x94, 0x79, 0x8A, 0x4D,
    0xDF, 0xAA, 0x4F, 0x03, 0xF9, 0x2A, 0xAA, 0xF0,
    0x4E, 0x1D, 0x45, 0x3F, 0x6E, 0xD1, 0xDE, 0x19,
    0xB8, 0x6D, 0x3A, 0xAD, 0xE0, 0x48, 0xA6, 0x54,
    0x83, 0xDF, 0x47, 0x54, 0x04, 0x9B, 0x5C, 0xD3,
    0xF5, 0x86, 0x40, 0x6C, 0xF2, 0xC6, 0x48, 0x75,
    0xC5, 0x1E, 0xDB, 0x57, 0x6D, 0xF3, 0x42, 0xB5,
    0xA9, 0x70, 0xF4, 0x0E, 0xAB, 0xC1, 0x5D, 0x3C
};

static unsigned char pk[CRYPTO_PUBLICKEYBYTES];
static unsigned char sk[CRYPTO_SECRETKEYBYTES];
static unsigned char ct[CRYPTO_CIPHERTEXTBYTES];
static unsigned char ss_enc[CRYPTO_BYTES];
static unsigned char ss_dec[CRYPTO_BYTES];

static void init_drng(void)
{
    init_random_number(&drng_algorithm, benchmark_seed, sizeof(benchmark_seed));
}

static void send_u64(const char *label, uint64_t value)
{
    char line[80];
    char digits[20];
    size_t pos = 0;
    size_t dpos = 0;

    while (*label != '\0' && pos < sizeof(line) - 1) {
        line[pos++] = *label++;
    }
    if (pos < sizeof(line) - 1) {
        line[pos++] = ' ';
    }
    if (pos < sizeof(line) - 1) {
        line[pos++] = '=';
    }
    if (pos < sizeof(line) - 1) {
        line[pos++] = ' ';
    }

    if (value == 0) {
        digits[dpos++] = '0';
    } else {
        while (value != 0 && dpos < sizeof(digits)) {
            digits[dpos++] = (char)('0' + (value % 10));
            value /= 10;
        }
    }

    while (dpos != 0 && pos < sizeof(line) - 1) {
        line[pos++] = digits[--dpos];
    }
    line[pos] = '\0';
    hal_send_str(line);
}

static int run_speed_once(unsigned int run, int emit)
{
    uint64_t t0;
    uint64_t t1;
    uint64_t keypair_cycles;
    uint64_t encaps_cycles;
    uint64_t decaps_cycles;
    int fail = 0;

    if (emit) {
        send_u64("run", run);
    }

    t0 = hal_get_time();
    fail |= crypto_kem_keypair(pk, sk);
    t1 = hal_get_time();
    keypair_cycles = t1 - t0;

    t0 = hal_get_time();
    fail |= crypto_kem_enc(ct, ss_enc, pk);
    t1 = hal_get_time();
    encaps_cycles = t1 - t0;

    t0 = hal_get_time();
    fail |= crypto_kem_dec(ss_dec, ct, sk);
    t1 = hal_get_time();
    decaps_cycles = t1 - t0;

    if (emit) {
        if (fail != 0 || memcmp(ss_enc, ss_dec, CRYPTO_BYTES) != 0) {
            hal_send_str("result = FAIL");
        } else {
            hal_send_str("result = OK");
        }

        send_u64("keypair cycles", keypair_cycles);
        send_u64("encaps cycles", encaps_cycles);
        send_u64("decaps cycles", decaps_cycles);
    }

    return fail != 0 || memcmp(ss_enc, ss_dec, CRYPTO_BYTES) != 0;
}

static void stop(void)
{
    while (1) {
    }
}

int main(void)
{
    unsigned int run;
    int fail = 0;

    hal_setup(CLOCK_BENCHMARK);

    hal_send_str("==========================");
    hal_send_str("SCABBARD512 SPEED TEST");
    hal_send_str("CLOCK_BENCHMARK 24MHz");
    send_u64("measured runs", RUN_COUNT);
    hal_send_str("warmup runs = 1");
    hal_send_str("DRNG seeded once; warmup ignored; measured runs use next random inputs");
    init_drng();

    fail |= run_speed_once(0, 0);
    for (run = 0; run < RUN_COUNT; run++) {
        fail |= run_speed_once(run, 1);
    }


    if (fail != 0) {
        hal_send_str("FINAL_RESULT = FAIL");
        hal_send_str("#");
        stop();
    }

    hal_send_str("FINAL_RESULT = OK");
    hal_send_str("#");
    stop();

    return 0;
}
