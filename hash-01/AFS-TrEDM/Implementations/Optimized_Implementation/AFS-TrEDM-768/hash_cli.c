/*
 * Small command-line helper for AFS-TrEDM self-tests.
 *
 * Usage:
 *   ./hash_cli <msg_len_bits> <msg_hex>
 *
 * msg_hex encodes the message bytes in the ICCS/KAT convention.  For non-byte
 * aligned messages, the unused low bits of the final byte are ignored and may
 * be zero.  For an empty message, use an empty string or "-".
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "CryptHash_AlgorithmInstance.h"

/* Function hexval: converts one hexadecimal input character to its nibble value. */
static int hexval(int c)
{
    if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return c - 'a' + 10;
    if ('A' <= c && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* Function print_hex: prints a byte string in hexadecimal. */
static void print_hex(const unsigned char *p, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        printf("%02X", p[i]);
    }
    printf("\n");
}

static char *read_text_file(const char *path)
{
    FILE *fp = fopen(path, "rb");
    long n;
    char *buf;
    if (fp == NULL) {
        return NULL;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }
    n = ftell(fp);
    if (n < 0) {
        fclose(fp);
        return NULL;
    }
    rewind(fp);
    buf = (char *)malloc((size_t)n + 1U);
    if (buf == NULL) {
        fclose(fp);
        return NULL;
    }
    if (fread(buf, 1U, (size_t)n, fp) != (size_t)n) {
        free(buf);
        fclose(fp);
        return NULL;
    }
    fclose(fp);
    buf[n] = '\0';
    return buf;
}

/* Function main: executes this standalone test, benchmark, or utility program. */
int main(int argc, char **argv)
{
    unsigned long long msg_len_bits;
    unsigned long long msg_len_bytes;
    const char *hex;
    char *file_hex = NULL;
    size_t hex_len;
    unsigned char *msg = NULL;
    unsigned char digest[DIGEST_BIT_LENGTH / 8];
    unsigned long long i;
    int rc;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <msg_len_bits> <msg_hex|->\n", argv[0]);
        return 2;
    }

    msg_len_bits = strtoull(argv[1], NULL, 10);
    msg_len_bytes = (msg_len_bits + 7ULL) / 8ULL;
    hex = argv[2];
    if (strcmp(hex, "-") == 0) {
        hex = "";
    } else if (hex[0] == '@') {
        file_hex = read_text_file(hex + 1);
        if (file_hex == NULL) {
            fprintf(stderr, "error: cannot read msg_hex file\n");
            return 3;
        }
        hex = file_hex;
    }
    hex_len = strlen(hex);

    if (hex_len < (size_t)(msg_len_bytes * 2ULL)) {
        fprintf(stderr, "error: msg_hex too short for msg_len_bits\n");
        free(file_hex);
        return 3;
    }

    msg = (unsigned char *)calloc(msg_len_bytes ? (size_t)msg_len_bytes : 1U, 1U);
    if (msg == NULL) {
        fprintf(stderr, "error: out of memory\n");
        free(file_hex);
        return 4;
    }

    for (i = 0; i < msg_len_bytes; i++) {
        int hi = hexval((unsigned char)hex[2ULL * i]);
        int lo = hexval((unsigned char)hex[2ULL * i + 1ULL]);
        if (hi < 0 || lo < 0) {
            fprintf(stderr, "error: invalid hex input\n");
            free(msg);
            free(file_hex);
            return 5;
        }
        msg[i] = (unsigned char)((hi << 4) | lo);
    }

    rc = CryptHash(DIGEST_BIT_LENGTH, msg, msg_len_bits, digest);
    free(msg);
    free(file_hex);
    if (rc != 0) {
        fprintf(stderr, "error: CryptHash returned %d\n", rc);
        return 6;
    }

    print_hex(digest, DIGEST_BIT_LENGTH / 8);
    return 0;
}
