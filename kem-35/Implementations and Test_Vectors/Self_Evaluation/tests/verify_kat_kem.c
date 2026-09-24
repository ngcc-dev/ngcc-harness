#include "KEM_AlgorithmInstance.h"
#include "drng.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SCLOUDPLUS_KAT_INSTANCE_NAME
#define SCLOUDPLUS_KAT_INSTANCE_NAME ALGORITHM_INSTANCE
#endif

#define EXPECTED_RECORDS 10
#define SEED_LEN_BYTES 64u

DRNG_ctx drng_algorithm;

static int read_line_dynamic(FILE *f, char **out)
{
    size_t cap = 256u;
    size_t len = 0u;
    char *line = NULL;
    int ch;

    if (out == NULL)
    {
        return -1;
    }
    *out = NULL;
    line = (char *)malloc(cap);
    if (line == NULL)
    {
        return -1;
    }

    while ((ch = fgetc(f)) != EOF)
    {
        if (ch == '\n')
        {
            break;
        }
        if (len + 1u >= cap)
        {
            const size_t new_cap = cap * 2u;
            char *new_line;

            if (new_cap <= cap)
            {
                free(line);
                return -1;
            }
            new_line = (char *)realloc(line, new_cap);
            if (new_line == NULL)
            {
                free(line);
                return -1;
            }
            line = new_line;
            cap = new_cap;
        }
        line[len++] = (char)ch;
    }

    if (ferror(f))
    {
        free(line);
        return -1;
    }
    if (ch == EOF && len == 0u)
    {
        free(line);
        return 0;
    }
    if (len > 0u && line[len - 1u] == '\r')
    {
        len--;
    }
    line[len] = '\0';
    *out = line;
    return 1;
}

static int read_nonempty_line(FILE *f, char **out)
{
    int rc;
    char *line = NULL;

    do
    {
        rc = read_line_dynamic(f, &line);
        if (rc <= 0)
        {
            return rc;
        }
        if (line[0] == '\0')
        {
            free(line);
            line = NULL;
        }
    } while (line == NULL);

    *out = line;
    return 1;
}

static int read_required_line(FILE *f, char **line)
{
    const int rc = read_nonempty_line(f, line);

    if (rc <= 0)
    {
        fprintf(stderr, "unexpected_eof\n");
        return -1;
    }
    return 0;
}

static int hex_digit_value(char c)
{
    const unsigned char uc = (unsigned char)c;

    if (uc >= (unsigned char)'0' && uc <= (unsigned char)'9')
    {
        return (int)(uc - (unsigned char)'0');
    }
    if (uc >= (unsigned char)'a' && uc <= (unsigned char)'f')
    {
        return (int)(uc - (unsigned char)'a' + 10u);
    }
    if (uc >= (unsigned char)'A' && uc <= (unsigned char)'F')
    {
        return (int)(uc - (unsigned char)'A' + 10u);
    }
    return -1;
}

static int hex_to_bytes(const char *hex, unsigned char *out, size_t out_len)
{
    const size_t hex_len = strlen(hex);

    if (hex_len != out_len * 2u)
    {
        return -1;
    }
    for (size_t i = 0u; i < out_len; i++)
    {
        const int hi = hex_digit_value(hex[2u * i]);
        const int lo = hex_digit_value(hex[2u * i + 1u]);

        if (hi < 0 || lo < 0)
        {
            return -1;
        }
        out[i] = (unsigned char)((hi << 4) | lo);
    }
    return 0;
}

static int parse_len_line(const char *line, const char *prefix,
                          unsigned long long *value)
{
    const size_t n = strlen(prefix);
    char *endptr = NULL;
    unsigned long long parsed;

    if (strncmp(line, prefix, n) != 0)
    {
        return -1;
    }
    errno = 0;
    parsed = strtoull(line + n, &endptr, 10);
    if (errno != 0 || endptr == line + n || *endptr != '\0')
    {
        return -1;
    }
    *value = parsed;
    return 0;
}

static int read_len_field(FILE *f, const char *prefix,
                          unsigned long long expected, const char *name)
{
    char *line = NULL;
    unsigned long long actual = 0;

    if (read_required_line(f, &line) != 0)
    {
        return -1;
    }
    if (parse_len_line(line, prefix, &actual) != 0 || actual != expected)
    {
        fprintf(stderr, "%s file=%llu impl=%llu\n", name, actual, expected);
        free(line);
        return -1;
    }
    free(line);
    return 0;
}

static int read_hex_field(FILE *f, const char *prefix, unsigned char *buf,
                          size_t len, const char *name)
{
    char *line = NULL;
    const size_t n = strlen(prefix);
    int ok = 0;

    if (read_required_line(f, &line) != 0)
    {
        return -1;
    }
    ok = strncmp(line, prefix, n) == 0 &&
         hex_to_bytes(line + n, buf, len) == 0;
    if (!ok)
    {
        fprintf(stderr, "%s\n", name);
        free(line);
        return -1;
    }
    free(line);
    return 0;
}

static int resize_file_buf(unsigned char **buf, size_t len)
{
    unsigned char *new_buf;

    if (len == 0u)
    {
        return -1;
    }
    new_buf = (unsigned char *)realloc(*buf, len);
    if (new_buf == NULL)
    {
        return -1;
    }
    *buf = new_buf;
    return 0;
}

static int read_count_line(FILE *f, int expected)
{
    char *line = NULL;
    char expected_line[32];
    int expected_len;
    int ok;

    expected_len = snprintf(expected_line, sizeof(expected_line), "Count = %d",
                            expected);
    if (expected_len < 0 || (size_t)expected_len >= sizeof(expected_line))
    {
        return -1;
    }
    if (read_required_line(f, &line) != 0)
    {
        return -1;
    }
    ok = strcmp(line, expected_line) == 0;
    free(line);
    return ok ? 0 : -1;
}

static int reject_trailing_nonempty_lines(FILE *f)
{
    char *line = NULL;
    const int rc = read_nonempty_line(f, &line);

    if (rc == 0)
    {
        return 0;
    }
    if (line != NULL)
    {
        fprintf(stderr, "trailing_kat_data %s\n", line);
        free(line);
    }
    return -1;
}

static int replay_records(FILE *f)
{
    const unsigned long long pk_expected = kem_get_pk_len_bytes();
    const unsigned long long sk_expected = kem_get_sk_len_bytes();
    const unsigned long long ct_expected = kem_get_ct_len_bytes();
    const unsigned long long ss_expected = kem_get_ss_len_bytes();
    unsigned char seed[SEED_LEN_BYTES];
    unsigned char *pk = NULL;
    unsigned char *sk = NULL;
    unsigned char *ct = NULL;
    unsigned char *ss = NULL;
    unsigned char *ss_dec = NULL;
    unsigned char *file_buf = NULL;
    int rc = -1;

    if (pk_expected == 0 || sk_expected == 0 || ct_expected == 0 ||
        ss_expected == 0)
    {
        goto out;
    }

    pk = (unsigned char *)malloc((size_t)pk_expected);
    sk = (unsigned char *)malloc((size_t)sk_expected);
    ct = (unsigned char *)malloc((size_t)ct_expected);
    ss = (unsigned char *)malloc((size_t)ss_expected);
    ss_dec = (unsigned char *)malloc((size_t)ss_expected);
    if (!pk || !sk || !ct || !ss || !ss_dec)
    {
        goto out;
    }

    for (int i = 0; i < EXPECTED_RECORDS; i++)
    {
        unsigned long long pk_len = 0;
        unsigned long long sk_len = 0;
        unsigned long long ct_len = 0;
        unsigned long long ss_len = 0;
        unsigned long long ss_dec_len = 0;

        if (read_count_line(f, i) != 0 ||
            read_len_field(f, "Seed_Len = ", SEED_LEN_BYTES, "seed_len") != 0 ||
            read_hex_field(f, "Seed = ", seed, sizeof(seed), "seed") != 0)
        {
            goto out;
        }
        if (init_random_number(&drng_algorithm, seed, sizeof(seed)) != 0)
        {
            goto out;
        }
        if (kem_keygen(pk, &pk_len, sk, &sk_len) != 0 ||
            pk_len != pk_expected || sk_len != sk_expected)
        {
            goto out;
        }

        if (resize_file_buf(&file_buf, (size_t)pk_expected) != 0 ||
            read_len_field(f, "PK_Len = ", pk_expected, "pk_len") != 0 ||
            read_hex_field(f, "PK = ", file_buf, (size_t)pk_expected, "pk") != 0 ||
            memcmp(pk, file_buf, (size_t)pk_expected) != 0)
        {
            goto out;
        }

        if (resize_file_buf(&file_buf, (size_t)sk_expected) != 0 ||
            read_len_field(f, "SK_Len = ", sk_expected, "sk_len") != 0 ||
            read_hex_field(f, "SK = ", file_buf, (size_t)sk_expected, "sk") != 0 ||
            memcmp(sk, file_buf, (size_t)sk_expected) != 0)
        {
            goto out;
        }

        if (kem_enc(pk, pk_len, ss, &ss_len, ct, &ct_len) != 0 ||
            ss_len != ss_expected || ct_len != ct_expected)
        {
            goto out;
        }

        if (resize_file_buf(&file_buf, (size_t)ct_expected) != 0 ||
            read_len_field(f, "CT_Len = ", ct_expected, "ct_len") != 0 ||
            read_hex_field(f, "CT = ", file_buf, (size_t)ct_expected, "ct") != 0 ||
            memcmp(ct, file_buf, (size_t)ct_expected) != 0)
        {
            goto out;
        }

        if (resize_file_buf(&file_buf, (size_t)ss_expected) != 0 ||
            read_len_field(f, "SS_Len = ", ss_expected, "ss_len") != 0 ||
            read_hex_field(f, "SS = ", file_buf, (size_t)ss_expected, "ss") != 0 ||
            memcmp(ss, file_buf, (size_t)ss_expected) != 0)
        {
            goto out;
        }
        if (kem_dec(sk, sk_len, ct, ct_len, ss_dec, &ss_dec_len) != 0 ||
            ss_dec_len != ss_expected ||
            memcmp(ss, ss_dec, (size_t)ss_expected) != 0)
        {
            fprintf(stderr, "kat_decaps_mismatch count=%d\n", i);
            goto out;
        }
    }
    if (reject_trailing_nonempty_lines(f) != 0)
    {
        goto out;
    }

    rc = 0;

out:
    free(file_buf);
    free(pk);
    free(sk);
    free(ct);
    free(ss);
    free(ss_dec);
    memset(seed, 0, sizeof(seed));
    return rc;
}

int main(void)
{
    char path[192];
    FILE *f;
    int path_len;
    int rc;

    path_len = snprintf(path, sizeof(path), "Test_Vectors/KAT_KEM_%s.txt",
                        SCLOUDPLUS_KAT_INSTANCE_NAME);
    if (path_len < 0 || (size_t)path_len >= sizeof(path))
    {
        fprintf(stderr, "kat_path_too_long\n");
        return 2;
    }
    f = fopen(path, "rb");
    if (f == NULL)
    {
        fprintf(stderr, "cannot_open_kat %s\n", path);
        return 2;
    }
    rc = replay_records(f);
    fclose(f);
    if (rc != 0)
    {
        return 3;
    }
    printf("kat_replay_pass %s\n", SCLOUDPLUS_KAT_INSTANCE_NAME);
    return 0;
}
