/* sign-01-11: two verifier memory defects in Aigis-Sig+ msig_verf.
 *
 * LEAK build (-DLEAK -Wl,--wrap=malloc -Wl,--wrap=free): msig_verf allocates
 * buf = malloc(CRHBYTES + mlen) (sign.c:167) and frees it only on the
 * accepting path (sign.c:203); the reject returns at sign.c:194 (challenge
 * mismatch) and sign.c:201 (norm bound) skip free(buf). The driver counts
 * live heap blocks across 200 verifications of a valid signature (control)
 * and 200 verifications of the same signature for a different message
 * (rejected at the challenge check).
 *
 * The LEAK build also writes pk, siglen and a valid signature for m to the
 * file named by argv[1].
 *
 * ASAN build (-fsanitize=address): msig_verf only checks that smlen lies in
 * [SIG_MIN_SIZE_PACKED, SIG_MAX_SIZE_PACKED] (sign.c:159); unpack_sig/unpack_h
 * then read the hint from counts inside the signature. The valid signature
 * from that file is copied into a heap buffer of exactly siglen - 1 bytes and
 * verified with that declared length. argv[2] "full" uses a siglen-byte
 * buffer (control). The ASAN build only verifies, so the separate
 * honest-signing stack overflow (sign-01-4) is not reached.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "params.h"
#include "api.h"
#include "drng.h"

DRNG_ctx drng_algorithm;
int msig_keygen(unsigned char *pk, unsigned char *sk);
int msig_sign(unsigned char *sk, unsigned char *m, unsigned long long mlen,
              unsigned char *sm, unsigned long long *smlen);
int msig_verf(unsigned char *pk, unsigned char *sm, unsigned long long smlen,
              unsigned char *m, unsigned long long mlen);

#ifdef LEAK
void *__real_malloc(size_t);
void __real_free(void *);
static long live_blocks;
void *__wrap_malloc(size_t n) { void *p = __real_malloc(n); if (p) live_blocks++; return p; }
void __wrap_free(void *p) { if (p) live_blocks--; __real_free(p); }
#endif

int main(int argc, char **argv)
{
    static uint8_t pk[SIG_PUBLICKEYBYTES], sk[SIG_SECRETKEYBYTES], sig[SIG_BYTES];
    unsigned long long siglen;
    uint8_t m[28], m2[28];
    for (int i = 0; i < 28; i++) { m[i] = (uint8_t)i; m2[i] = (uint8_t)(i ^ 0x33); }
    if (argc < 2) return 2;
#ifdef LEAK
    uint8_t seed[48];
    for (int i = 0; i < 48; i++) seed[i] = (uint8_t)(5 * i + 1);
    init_random_number(&drng_algorithm, seed, 48);
    if (msig_keygen(pk, sk) || msig_sign(sk, m, sizeof m, sig, &siglen)) return 1;
    long b0 = live_blocks; int acc = 0, rej = 0;
    for (int i = 0; i < 200; i++) acc += msig_verf(pk, sig, siglen, m, sizeof m) == 0;
    long b1 = live_blocks;
    for (int i = 0; i < 200; i++) rej += msig_verf(pk, sig, siglen, m2, sizeof m2) != 0;
    long b2 = live_blocks;
    printf("accepted=%d leaked_blocks=%ld; rejected=%d leaked_blocks=%ld\n", acc, b1 - b0, rej, b2 - b1);
    FILE *f = fopen(argv[1], "wb");
    if (!f || fwrite(pk, 1, sizeof pk, f) != sizeof pk || fwrite(&siglen, sizeof siglen, 1, f) != 1 ||
        fwrite(sig, 1, sizeof sig, f) != sizeof sig || fclose(f)) return 1;
#else
    FILE *f = fopen(argv[1], "rb");
    if (!f || fread(pk, 1, sizeof pk, f) != sizeof pk || fread(&siglen, sizeof siglen, 1, f) != 1 ||
        fread(sig, 1, sizeof sig, f) != sizeof sig) return 1;
    fclose(f);
    int full = argc > 2 && strcmp(argv[2], "full") == 0;
    unsigned long long n = full ? siglen : siglen - 1;
    if (n < SIG_MIN_SIZE_PACKED) { puts("signature too short for this test"); return 1; }
    uint8_t *h = malloc(n);
    memcpy(h, sig, n);
    printf("siglen=%llu declared=%llu min=%d max=%d\n", siglen, n, SIG_MIN_SIZE_PACKED, SIG_MAX_SIZE_PACKED);
    fflush(stdout);
    int rc = msig_verf(pk, h, n, m, sizeof m);
    printf("verify rc=%d\n", rc);
    free(h);
#endif
    return 0;
}
