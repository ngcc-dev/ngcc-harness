/* KAT_2_12 vector counter for the NGCC CryptHash API.
 *
 * Usage: reproduce_spec_mismatch KATFILE
 * Reads every (Msg_Len, Msg, Dst_Len, Dst) entry, calls CryptHash() on the
 * message and compares the digest. Prints
 *   match=M total=T first_fail=F fails=<comma-separated Msg_Len list, or "none",
 *   or "N(list omitted)" when more than 100 vectors fail>; F = -1 if none fail
 * Exit status 0 unless the file cannot be read.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int CryptHash(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest);

static int hexval(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static size_t parsehex(const char *s, unsigned char *out, size_t cap)
{
    size_t n = 0;
    while (n < cap && hexval((unsigned char)s[0]) >= 0 && hexval((unsigned char)s[1]) >= 0) {
        out[n++] = (unsigned char)(hexval((unsigned char)s[0]) * 16 + hexval((unsigned char)s[1]));
        s += 2;
    }
    return n;
}

int main(int argc, char **argv)
{
    static char line[1 << 16];
    static unsigned char msg[8192], want[1024], got[1024];
    static long fails[8192];
    long mlen = -1, total = 0, match = 0, nfail = 0;
    int dlen = 0;
    FILE *f;

    if (argc != 2 || (f = fopen(argv[1], "r")) == NULL) {
        fprintf(stderr, "usage: %s KATFILE\n", argv[0]);
        return 2;
    }
    while (fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (!strncmp(line, "Msg_Len = ", 10)) {
            mlen = atol(line + 10);
            memset(msg, 0, sizeof msg);
        } else if (!strncmp(line, "Msg = ", 6)) {
            parsehex(line + 6, msg, sizeof msg);
        } else if (!strncmp(line, "Dst_Len = ", 10)) {
            dlen = atoi(line + 10);
        } else if (!strncmp(line, "Dst = ", 6)) {
            size_t nb = (size_t)(dlen + 7) / 8;
            size_t wb = parsehex(line + 6, want, sizeof want);
            int rc;
            memset(got, 0, sizeof got);
            rc = CryptHash(dlen, msg, (unsigned long long)mlen, got);
            total++;
            if (rc == 0 && wb == nb && memcmp(got, want, nb) == 0)
                match++;
            else
                fails[nfail++] = mlen;
        }
    }
    fclose(f);
    printf("match=%ld total=%ld first_fail=%ld fails=", match, total, nfail ? fails[0] : -1L);
    if (nfail == 0) {
        printf("none");
    } else if (nfail > 100) {
        printf("%ld(list omitted)", nfail);
    } else {
        for (long i = 0; i < nfail; i++)
            printf(i ? ",%ld" : "%ld", fails[i]);
    }
    printf("\n");
    return 0;
}
