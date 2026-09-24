#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "api.h"
#include "drng.h"
#include "hal.h"

#define SEED_LEN_BYTES 64
#define CANARY 0x42
#define STACK_SCAN_LOW_GUARD_BYTES 1024
#define STACK_RUNS 50
#define NOINLINE __attribute__((noinline))

extern void *_sbrk(ptrdiff_t incr);

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

static void send_u32(const char *label, uint32_t value)
{
    char line[80];
    char digits[10];
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

static void send_result(int ok)
{
    hal_send_str(ok ? "result = OK" : "result = FAIL");
}

static void stop(void)
{
    while (1) {
    }
}

static NOINLINE int op_keypair(void)
{
    return crypto_kem_keypair(pk, sk);
}

static NOINLINE int op_encaps(void)
{
    return crypto_kem_enc(ct, ss_enc, pk);
}

static NOINLINE int op_decaps(void)
{
    return crypto_kem_dec(ss_dec, ct, sk);
}

static void prepare_for_encaps(void)
{
    crypto_kem_keypair(pk, sk);
}

static void prepare_for_decaps(void)
{
    crypto_kem_keypair(pk, sk);
    crypto_kem_enc(ct, ss_enc, pk);
}

static unsigned char *stack_scan_low(void)
{
    return (unsigned char *)_sbrk(0) + STACK_SCAN_LOW_GUARD_BYTES;
}

static void warm_runtime_heap(void)
{
    prepare_for_decaps();
    crypto_kem_dec(ss_dec, ct, sk);
}

static NOINLINE uint32_t measure_stack(int (*op)(void), int *rc)
{
    unsigned char *high;
    unsigned char *low = stack_scan_low();
    unsigned char *p;
    volatile unsigned char *vp;

    __asm volatile ("mov %0, sp" : "=r"(high) :: "memory");

    if (low >= high) {
        *rc = -1;
        return 0;
    }

    vp = high;
    while (vp > (volatile unsigned char *)low) {
        vp--;
        *vp = CANARY;
    }

    *rc = op();

    p = low;
    while (p < high && *((volatile unsigned char *)p) == CANARY) {
        p++;
    }

    if (p == high) {
        return 0;
    }

    return (uint32_t)(high - p);
}

static NOINLINE int run_stack_once(size_t run)
{
    int rc_keypair;
    int rc_encaps;
    int rc_decaps;
    uint32_t stack_keypair;
    uint32_t stack_encaps;
    uint32_t stack_decaps;

    send_u32("run", (uint32_t)run);

    stack_keypair = measure_stack(op_keypair, &rc_keypair);

    prepare_for_encaps();
    stack_encaps = measure_stack(op_encaps, &rc_encaps);

    prepare_for_decaps();
    stack_decaps = measure_stack(op_decaps, &rc_decaps);

    if (rc_keypair != 0 || rc_encaps != 0 || rc_decaps != 0 ||
        memcmp(ss_enc, ss_dec, CRYPTO_BYTES) != 0) {
        send_result(0);
    } else {
        send_result(1);
    }

    send_u32("keypair stack bytes", stack_keypair);
    send_u32("encaps stack bytes", stack_encaps);
    send_u32("decaps stack bytes", stack_decaps);

    if (rc_keypair != 0 || rc_encaps != 0 || rc_decaps != 0 ||
        memcmp(ss_enc, ss_dec, CRYPTO_BYTES) != 0) {
        return -1;
    }

    return 0;
}

int main(void)
{
    size_t run;
    int failed = 0;
    hal_setup(CLOCK_BENCHMARK);

    hal_send_str("==========================");
    hal_send_str("SCABBARD256 STACK TEST");
    send_u32("runs", STACK_RUNS);
    hal_send_str("DRNG seeded once; stack runs use next random inputs");
    init_drng();
    hal_send_str("runtime prewarm before stack canary");
    warm_runtime_heap();

    for (run = 0; run < STACK_RUNS; run++) {
        if (run_stack_once(run) != 0) {
            failed = 1;
        }
    }


    if (failed) {
        hal_send_str("FINAL_RESULT = FAIL");
        hal_send_str("#");
        stop();
    }

    hal_send_str("FINAL_RESULT = OK");
    hal_send_str("#");
    stop();

    return 0;
}
