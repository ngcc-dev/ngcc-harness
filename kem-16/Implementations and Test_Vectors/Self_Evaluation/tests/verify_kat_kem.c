#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "KEM_AlgorithmInstance.h"
#include "crypto_memset.h"
#include "drng.h"

#ifndef HARE_INSTANCE_NAME
#define HARE_INSTANCE_NAME ALGORITHM_INSTANCE
#endif

#define EXPECTED_RECORDS 10
#define SEED_LEN_BYTES 64u

DRNG_ctx drng_algorithm;

/**
 * Zero and free a dynamically allocated byte buffer.
 */
static void secure_free(unsigned char *p, size_t n) {
    if (p != NULL) {
        memset_zero(p, n);
        free(p);
    }
}

/**
 * Read one line of arbitrary length from the KAT file.
 */
static int read_line_dynamic(FILE *f, char **out) {
    size_t cap = 256u;
    size_t len = 0u;
    char *line = NULL;
    int ch;

    if (out == NULL) {
        return -1;
    }
    *out = NULL;

    line = (char *)malloc(cap);
    if (line == NULL) {
        return -1;
    }

    while ((ch = fgetc(f)) != EOF) {
        if (ch == '\n') {
            break;
        }
        if (len + 1u >= cap) {
            size_t new_cap = cap * 2u;
            char *new_line;
            if (new_cap <= cap) {
                free(line);
                return -1;
            }
            new_line = (char *)realloc(line, new_cap);
            if (new_line == NULL) {
                free(line);
                return -1;
            }
            line = new_line;
            cap = new_cap;
        }
        line[len++] = (char)ch;
    }

    if (ch == EOF && len == 0u) {
        free(line);
        return 0;
    }

    if (len > 0u && line[len - 1u] == '\r') {
        len--;
    }
    line[len] = '\0';
    *out = line;
    return 1;
}

/**
 * Read the next non-empty line from the KAT file.
 */
static int read_nonempty_line(FILE *f, char **out) {
    int rc;
    char *line = NULL;

    do {
        rc = read_line_dynamic(f, &line);
        if (rc <= 0) {
            return rc;
        }
        if (line[0] == '\0') {
            free(line);
            line = NULL;
        }
    } while (line == NULL);

    *out = line;
    return 1;
}

/**
 * Convert one hexadecimal digit to its integer value.
 */
static int hex_digit_value(char c) {
    unsigned char uc = (unsigned char)c;

    if (uc >= (unsigned char)'0' && uc <= (unsigned char)'9') {
        return (int)(uc - (unsigned char)'0');
    }
    if (uc >= (unsigned char)'a' && uc <= (unsigned char)'f') {
        return (int)(uc - (unsigned char)'a' + 10u);
    }
    if (uc >= (unsigned char)'A' && uc <= (unsigned char)'F') {
        return (int)(uc - (unsigned char)'A' + 10u);
    }
    return -1;
}

/**
 * Decode a fixed-length hexadecimal string into bytes.
 */
static int hex_to_bytes(const char *hex, unsigned char *out, size_t out_len) {
    size_t hex_len;

    if (hex == NULL || out == NULL) {
        return -1;
    }
    if (out_len > ((size_t)-1) / 2u) {
        return -1;
    }

    hex_len = strlen(hex);
    if (hex_len != out_len * 2u) {
        return -1;
    }

    for (size_t i = 0u; i < out_len; ++i) {
        const int hi = hex_digit_value(hex[2u * i]);
        const int lo = hex_digit_value(hex[2u * i + 1u]);
        if (hi < 0 || lo < 0) {
            return -1;
        }
        out[i] = (unsigned char)((hi << 4) | lo);
    }

    return 0;
}

/**
 * Parse a length line with a required prefix.
 */
static int parse_len_line(const char *line, const char *prefix, unsigned long long *value) {
    size_t n;
    char *endptr = NULL;
    unsigned long long parsed;

    if (line == NULL || prefix == NULL || value == NULL) {
        return -1;
    }

    n = strlen(prefix);
    if (strncmp(line, prefix, n) != 0) {
        return -1;
    }

    errno = 0;
    parsed = strtoull(line + n, &endptr, 10);
    if (errno != 0 || endptr == line + n || *endptr != '\0') {
        return -1;
    }

    *value = parsed;
    return 0;
}

/**
 * Parse and decode a hex field with a required prefix.
 */
static int parse_hex_line(const char *line, const char *prefix, unsigned char *buf, size_t len) {
    size_t n;

    if (line == NULL || prefix == NULL || buf == NULL) {
        return -1;
    }

    n = strlen(prefix);
    if (strncmp(line, prefix, n) != 0) {
        return -1;
    }

    return hex_to_bytes(line + n, buf, len);
}

/**
 * Read one required non-empty line or report EOF/error.
 */
static int read_required_line(FILE *f, char **line) {
    int rc = read_nonempty_line(f, line);
    if (rc <= 0) {
        fprintf(stderr, "unexpected_eof\n");
        return -1;
    }
    return 0;
}

/**
 * Read and parse a named length field from the KAT file.
 */
static int read_len_field(FILE *f,
                          const char *prefix,
                          unsigned long long expected,
                          const char *error_name) {
    char *line = NULL;
    unsigned long long actual = 0u;

    if (read_required_line(f, &line) != 0) {
        return -1;
    }
    if (parse_len_line(line, prefix, &actual) != 0 || actual != expected) {
        fprintf(stderr, "%s file=%llu impl=%llu\n", error_name, actual, expected);
        free(line);
        return -1;
    }

    free(line);
    return 0;
}

/**
 * Read and parse a named hexadecimal byte field from the KAT file.
 */
static int read_hex_field(FILE *f,
                          const char *prefix,
                          unsigned char *buf,
                          size_t len,
                          const char *error_name) {
    char *line = NULL;

    if (read_required_line(f, &line) != 0) {
        return -1;
    }
    if (parse_hex_line(line, prefix, buf, len) != 0) {
        fprintf(stderr, "%s\n", error_name);
        free(line);
        return -1;
    }

    free(line);
    return 0;
}

/**
 * Replay the Reference KAT file against the compiled KEM instance.
 */
int main(void) {
    char path[256];
    FILE *f = NULL;
    char *line = NULL;
    int status = 0;
    int records = 0;

    const unsigned long long pk_len_ull = kem_get_pk_len_bytes();
    const unsigned long long sk_len_ull = kem_get_sk_len_bytes();
    const unsigned long long ct_len_ull = kem_get_ct_len_bytes();
    const unsigned long long ss_len_ull = kem_get_ss_len_bytes();

    const size_t pk_len = (size_t)pk_len_ull;
    const size_t sk_len = (size_t)sk_len_ull;
    const size_t ct_len = (size_t)ct_len_ull;
    const size_t ss_len = (size_t)ss_len_ull;

    unsigned char *seed = NULL;
    unsigned char *pk = NULL;
    unsigned char *sk = NULL;
    unsigned char *ct = NULL;
    unsigned char *ss = NULL;
    unsigned char *pk2 = NULL;
    unsigned char *sk2 = NULL;
    unsigned char *ct2 = NULL;
    unsigned char *ss2 = NULL;
    unsigned char *ss3 = NULL;

    if ((unsigned long long)pk_len != pk_len_ull ||
        (unsigned long long)sk_len != sk_len_ull ||
        (unsigned long long)ct_len != ct_len_ull ||
        (unsigned long long)ss_len != ss_len_ull) {
        fprintf(stderr, "length_too_large_for_platform\n");
        return 1;
    }

    snprintf(path, sizeof(path), "Test_Vectors/KAT_KEM_%s.txt", ALGORITHM_INSTANCE);
    f = fopen(path, "rb");
    if (f == NULL) {
        fprintf(stderr, "missing_kat_file %s\n", path);
        return 2;
    }

    seed = (unsigned char *)calloc(SEED_LEN_BYTES, 1u);
    pk = (unsigned char *)calloc(pk_len, 1u);
    sk = (unsigned char *)calloc(sk_len, 1u);
    ct = (unsigned char *)calloc(ct_len, 1u);
    ss = (unsigned char *)calloc(ss_len, 1u);
    pk2 = (unsigned char *)calloc(pk_len, 1u);
    sk2 = (unsigned char *)calloc(sk_len, 1u);
    ct2 = (unsigned char *)calloc(ct_len, 1u);
    ss2 = (unsigned char *)calloc(ss_len, 1u);
    ss3 = (unsigned char *)calloc(ss_len, 1u);

    if (seed == NULL || pk == NULL || sk == NULL || ct == NULL || ss == NULL ||
        pk2 == NULL || sk2 == NULL || ct2 == NULL || ss2 == NULL || ss3 == NULL) {
        fprintf(stderr, "alloc_failed\n");
        status = 3;
        goto cleanup;
    }

    while (1) {
        unsigned long long count_value = 0u;
        unsigned long long pk2_len = pk_len_ull;
        unsigned long long sk2_len = sk_len_ull;
        unsigned long long ct2_len = ct_len_ull;
        unsigned long long ss2_len = ss_len_ull;
        unsigned long long ss3_len = ss_len_ull;

        const int line_rc = read_nonempty_line(f, &line);
        if (line_rc == 0) {
            break;
        }
        if (line_rc < 0) {
            fprintf(stderr, "read_error\n");
            status = 4;
            goto cleanup;
        }

        if (parse_len_line(line, "Count = ", &count_value) != 0 || count_value != (unsigned long long)records) {
            fprintf(stderr, "bad_count_line expected=%d line=%s\n", records, line);
            status = 4;
            goto cleanup;
        }
        free(line);
        line = NULL;

        if (read_len_field(f, "Seed_Len = ", SEED_LEN_BYTES, "bad_seed_len") != 0) { status = 5; goto cleanup; }
        if (read_hex_field(f, "Seed = ", seed, SEED_LEN_BYTES, "bad_seed_hex") != 0) { status = 6; goto cleanup; }

        if (read_len_field(f, "PK_Len = ", pk_len_ull, "pk_len_mismatch") != 0) { status = 7; goto cleanup; }
        if (read_hex_field(f, "PK = ", pk, pk_len, "bad_pk_hex") != 0) { status = 8; goto cleanup; }

        if (read_len_field(f, "SK_Len = ", sk_len_ull, "sk_len_mismatch") != 0) { status = 9; goto cleanup; }
        if (read_hex_field(f, "SK = ", sk, sk_len, "bad_sk_hex") != 0) { status = 10; goto cleanup; }

        if (read_len_field(f, "CT_Len = ", ct_len_ull, "ct_len_mismatch") != 0) { status = 11; goto cleanup; }
        if (read_hex_field(f, "CT = ", ct, ct_len, "bad_ct_hex") != 0) { status = 12; goto cleanup; }

        if (read_len_field(f, "SS_Len = ", ss_len_ull, "ss_len_mismatch") != 0) { status = 13; goto cleanup; }
        if (read_hex_field(f, "SS = ", ss, ss_len, "bad_ss_hex") != 0) { status = 14; goto cleanup; }

        init_random_number(&drng_algorithm, seed, SEED_LEN_BYTES);

        memset(pk2, 0, pk_len);
        memset(sk2, 0, sk_len);
        memset(ct2, 0, ct_len);
        memset(ss2, 0, ss_len);
        memset(ss3, 0, ss_len);

        if (kem_keygen(pk2, &pk2_len, sk2, &sk2_len) != 0 || pk2_len != pk_len_ull || sk2_len != sk_len_ull) {
            fprintf(stderr, "kem_keygen_failed record=%d\n", records);
            status = 15;
            goto cleanup;
        }
        if (memcmp(pk, pk2, pk_len) != 0 || memcmp(sk, sk2, sk_len) != 0) {
            fprintf(stderr, "keypair_mismatch record=%d\n", records);
            status = 16;
            goto cleanup;
        }

        if (kem_enc(pk2, pk2_len, ss2, &ss2_len, ct2, &ct2_len) != 0 || ss2_len != ss_len_ull || ct2_len != ct_len_ull) {
            fprintf(stderr, "kem_enc_failed record=%d\n", records);
            status = 17;
            goto cleanup;
        }
        if (memcmp(ct, ct2, ct_len) != 0 || memcmp(ss, ss2, ss_len) != 0) {
            fprintf(stderr, "encaps_mismatch record=%d\n", records);
            status = 18;
            goto cleanup;
        }

        if (kem_dec(sk, sk_len_ull, ct, ct_len_ull, ss3, &ss3_len) != 0 || ss3_len != ss_len_ull) {
            fprintf(stderr, "kem_dec_failed record=%d\n", records);
            status = 19;
            goto cleanup;
        }
        if (memcmp(ss, ss3, ss_len) != 0) {
            fprintf(stderr, "decaps_mismatch record=%d\n", records);
            status = 20;
            goto cleanup;
        }

        records++;
    }

    if (records != EXPECTED_RECORDS) {
        fprintf(stderr, "record_count_mismatch expected=%d got=%d\n", EXPECTED_RECORDS, records);
        status = 21;
        goto cleanup;
    }

    printf("verify_kat_pass %s records=%d pk=%llu sk=%llu ct=%llu ss=%llu\n",
           HARE_INSTANCE_NAME, records, pk_len_ull, sk_len_ull, ct_len_ull, ss_len_ull);

cleanup:
    if (line != NULL) {
        free(line);
    }
    if (f != NULL) {
        fclose(f);
    }

    secure_free(seed, SEED_LEN_BYTES);
    secure_free(pk, pk_len);
    secure_free(sk, sk_len);
    secure_free(ct, ct_len);
    secure_free(ss, ss_len);
    secure_free(pk2, pk_len);
    secure_free(sk2, sk_len);
    secure_free(ct2, ct_len);
    secure_free(ss2, ss_len);
    secure_free(ss3, ss_len);

    return status;
}
