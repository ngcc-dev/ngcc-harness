/*
 * Independent official-KAT validator (verify-only).
 *
 * Reads a published Test_Vectors/KAT_SIG_QingLuan-<L>.txt and runs each
 * (PK, M, Sn) triple through the REFERENCE crypto_sign_verify (src/verify.c).
 * It does NOT regenerate anything from the seed — it consumes the published
 * vectors and asserts the published signatures verify under the published
 * public keys. Cross-checks adapter-generated vectors vs the reference verifier.
 *
 * Build (per instance):
 *   gcc -O2 -Iinclude ../src <this> -o kat_verify -lbcrypt   (Windows)
 * Run:
 *   ./kat_verify <path-to-KAT_SIG_QingLuan-L.txt>
 */
#include "api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int hexval(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* Parse a run of hex starting at p (just after "X = "); malloc bytes, set *outlen. */
static unsigned char *parse_hex(const char *p, size_t *outlen)
{
    size_t n = 0;
    while (hexval((unsigned char)p[n]) >= 0) n++;
    size_t bytes = n / 2;
    unsigned char *buf = (unsigned char *)malloc(bytes ? bytes : 1);
    for (size_t i = 0; i < bytes; i++)
        buf[i] = (unsigned char)((hexval((unsigned char)p[2*i]) << 4) |
                                  hexval((unsigned char)p[2*i + 1]));
    *outlen = bytes;
    return buf;
}

static const char *find_after(const char *from, const char *tok)
{
    const char *h = strstr(from, tok);
    return h ? h + strlen(tok) : NULL;
}

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "usage: %s KAT.txt\n", argv[0]); return 2; }
    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror("fopen"); return 2; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc((size_t)sz + 1);
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) { perror("fread"); return 2; }
    buf[sz] = 0; fclose(f);

    const char *cur = buf;
    int total = 0, ok = 0;
    for (;;) {
        const char *pkp = find_after(cur, "\nPK = "); if (!pkp) break;
        const char *mp  = find_after(pkp, "\nM = ");  if (!mp)  break;
        const char *snp = find_after(mp,  "\nSn = "); if (!snp) break;

        size_t pklen, mlen, snlen;
        unsigned char *pk = parse_hex(pkp, &pklen);
        unsigned char *m  = parse_hex(mp,  &mlen);
        unsigned char *sn = parse_hex(snp, &snlen);

        /* sanity vs this instance's fixed sizes */
        if (pklen != (size_t)CRYPTO_PUBLICKEYBYTES || snlen != (size_t)CRYPTO_BYTES)
            printf("  vector %d: SIZE MISMATCH pk=%zu(exp %d) sn=%zu(exp %d)\n",
                   total, pklen, CRYPTO_PUBLICKEYBYTES, snlen, CRYPTO_BYTES);

        int rc = crypto_sign_verify(sn, snlen, m, mlen, pk);
        total++;
        if (rc == 0) ok++;
        else printf("  vector %d: VERIFY FAILED (m_len=%zu)\n", total - 1, mlen);

        free(pk); free(m); free(sn);
        cur = snp;
    }
    printf("%s [%s]: %d/%d official vectors verify OK\n",
           CRYPTO_ALGNAME, argv[1], ok, total);
    free(buf);
    return (total > 0 && ok == total) ? 0 : 1;
}
