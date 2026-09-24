#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "api.h"
#include "drng.h"
#include "hal.h"

#define SEED_LEN_BYTES 64
#define HEX_LINE_BYTES 16

DRNG_ctx drng_algorithm;

static const unsigned char kat0_seed[SEED_LEN_BYTES] = {
    0x92, 0x7F, 0x06, 0xB5, 0x94, 0x79, 0x8A, 0x4D,
    0xDF, 0xAA, 0x4F, 0x03, 0xF9, 0x2A, 0xAA, 0xF0,
    0x4E, 0x1D, 0x45, 0x3F, 0x6E, 0xD1, 0xDE, 0x19,
    0xB8, 0x6D, 0x3A, 0xAD, 0xE0, 0x48, 0xA6, 0x54,
    0x83, 0xDF, 0x47, 0x54, 0x04, 0x9B, 0x5C, 0xD3,
    0xF5, 0x86, 0x40, 0x6C, 0xF2, 0xC6, 0x48, 0x75,
    0xC5, 0x1E, 0xDB, 0x57, 0x6D, 0xF3, 0x42, 0xB5,
    0xA9, 0x70, 0xF4, 0x0E, 0xAB, 0xC1, 0x5D, 0x3C
};

static const unsigned char kat0_expected_ss[CRYPTO_BYTES] = {
    0xA6, 0xAF, 0xC5, 0x5A, 0x47, 0x4E, 0x98, 0xD7,
    0xB4, 0x60, 0xDA, 0x7F, 0xE4, 0x85, 0xC8, 0xED
};

static unsigned char pk[CRYPTO_PUBLICKEYBYTES];
static unsigned char sk[CRYPTO_SECRETKEYBYTES];
static unsigned char ct[CRYPTO_CIPHERTEXTBYTES];
static unsigned char ss_enc[CRYPTO_BYTES];
static unsigned char ss_dec[CRYPTO_BYTES];

static void init_drng(void)
{
    init_random_number(&drng_algorithm, kat0_seed, sizeof(kat0_seed));
}

static char hex_digit(unsigned int x)
{
    x &= 0x0F;
    return (char)(x < 10 ? ('0' + x) : ('A' + x - 10));
}

static void send_u32(const char *label, uint32_t value)
{
    char line[64];
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

static void send_hex_dump(const char *label, const unsigned char *buf, size_t len)
{
    size_t offset;

    hal_send_str(label);
    send_u32("LEN", (uint32_t)len);

    for (offset = 0; offset < len; offset += HEX_LINE_BYTES) {
        char line[4 + 2 + HEX_LINE_BYTES * 3 + 1];
        size_t pos = 0;
        size_t i;
        size_t chunk = len - offset;

        if (chunk > HEX_LINE_BYTES) {
            chunk = HEX_LINE_BYTES;
        }

        line[pos++] = hex_digit((unsigned int)(offset >> 12));
        line[pos++] = hex_digit((unsigned int)(offset >> 8));
        line[pos++] = hex_digit((unsigned int)(offset >> 4));
        line[pos++] = hex_digit((unsigned int)offset);
        line[pos++] = ':';
        line[pos++] = ' ';

        for (i = 0; i < chunk; i++) {
            unsigned char b = buf[offset + i];

            line[pos++] = hex_digit((unsigned int)(b >> 4));
            line[pos++] = hex_digit((unsigned int)b);
            if (i + 1 < chunk) {
                line[pos++] = ' ';
            }
        }

        line[pos] = '\0';
        hal_send_str(line);
    }
}

static int run_kem_test(void)
{
    int rc;

    hal_send_str("==========================");
    hal_send_str("SCABBARD128 BOARD TEST KAT0 SEED");
    send_hex_dump("SEED_KAT0", kat0_seed, sizeof(kat0_seed));

    init_drng();

    rc = crypto_kem_keypair(pk, sk);
    if (rc != 0) {
        return 1;
    }
    hal_send_str("KEYPAIR_OK");
    send_hex_dump("PK_GENERATED", pk, CRYPTO_PUBLICKEYBYTES);
    send_hex_dump("SK_GENERATED", sk, CRYPTO_SECRETKEYBYTES);

    rc = crypto_kem_enc(ct, ss_enc, pk);
    if (rc != 0) {
        return 2;
    }
    hal_send_str("ENCAPS_OK");
    send_hex_dump("CT_GENERATED", ct, CRYPTO_CIPHERTEXTBYTES);
    send_hex_dump("SS_EXPECTED_KAT0", kat0_expected_ss, CRYPTO_BYTES);
    send_hex_dump("SS_ENC_GENERATED", ss_enc, CRYPTO_BYTES);

    rc = crypto_kem_dec(ss_dec, ct, sk);
    if (rc != 0) {
        return 3;
    }
    hal_send_str("DECAPS_OK");
    send_hex_dump("SS_DEC_RECOVERED", ss_dec, CRYPTO_BYTES);

    if (memcmp(ss_enc, ss_dec, CRYPTO_BYTES) != 0) {
        return 4;
    }
    if (memcmp(ss_enc, kat0_expected_ss, CRYPTO_BYTES) != 0) {
        return 5;
    }

    hal_send_str("RESULT_OK");
    return 0;
}

static void stop(void)
{
    while (1) {
    }
}

int main(void)
{
    int fail;

    hal_setup(CLOCK_BENCHMARK);

    fail = run_kem_test();

    if (fail == 0) {
        hal_send_str("FINAL_RESULT = OK");
        hal_send_str("#");
        stop();
    }

    hal_send_str("FINAL_RESULT = FAIL");
    send_u32("fail code", (uint32_t)fail);
    hal_send_str("#");
    stop();

    return 0;
}
