/* sign-01: the AVX2 polyt0_unpack issues 32-byte loads while advancing
 * PARAM_D bytes (15 at sets I/II, 13 at set III), so unpack_sk reads
 * 32 - PARAM_D bytes past the last t0 polynomial, which ends the secret key.
 *
 * A valid key pair is generated; sk (exactly sig_get_sk_len_bytes() bytes) is
 * copied so that it ends argv[2] bytes (default 0) before a page boundary.
 * argv[1] "guard" makes the following page PROT_NONE, "ctrl" leaves it
 * readable. The driver then signs with that sk and verifies the signature.
 * "pkguard" / "pkctrl" do the same with pk for verification (the AVX2
 * polyt1_unpack reads 8-byte words).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "drng.h"

DRNG_ctx drng_algorithm;
unsigned long long sig_get_pk_len_bytes(void), sig_get_sk_len_bytes(void), sig_get_sn_len_bytes(void);
int sig_keygen(unsigned char *, unsigned long long *, unsigned char *, unsigned long long *);
int sig_sign(unsigned char *, unsigned long long, unsigned char *, unsigned long long,
             unsigned char *, unsigned long long *);
int sig_verify(unsigned char *, unsigned long long, unsigned char *, unsigned long long,
               unsigned char *, unsigned long long);

int main(int argc, char **argv)
{
    unsigned long long PL = sig_get_pk_len_bytes(), SL = sig_get_sk_len_bytes(), NL = sig_get_sn_len_bytes(), a, b, sl;
    unsigned char *pk = malloc(PL), *sk = malloc(SL), *sig = malloc(NL + 64), m[33], seed[48];
    const char *mode = argc > 1 ? argv[1] : "ctrl";
    size_t slack = argc > 2 ? (size_t)atoi(argv[2]) : 0;
    for (int i = 0; i < 48; i++) seed[i] = (unsigned char)(13 * i + 5);
    for (int i = 0; i < 33; i++) m[i] = (unsigned char)(i * 7);
    init_random_number(&drng_algorithm, seed, 48);
    if (sig_keygen(pk, &a, sk, &b)) return 3;
    if (!strncmp(mode, "pk", 2)) {
        long pg = sysconf(_SC_PAGESIZE);
        size_t span = ((PL + slack + pg - 1) / pg) * pg;
        unsigned char *map = mmap(NULL, span + pg, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (map == MAP_FAILED) return 4;
        unsigned char *p = map + span - slack - PL;
        memcpy(p, pk, PL);
        if (sig_sign(sk, SL, m, sizeof m, sig, &sl)) return 6;
        if (mprotect(map + span, pg, strcmp(mode, "pkguard") ? PROT_READ : PROT_NONE)) return 5;
        printf("%s: pk %llu bytes ends %zu bytes before the page boundary; verifying...\n", mode, PL, slack);
        fflush(stdout);
        printf("%s: verify rc=%d\n", mode, sig_verify(p, PL, sig, sl, m, sizeof m));
        return 0;
    }
    long pg = sysconf(_SC_PAGESIZE);
    size_t span = ((SL + slack + pg - 1) / pg) * pg;
    unsigned char *map = mmap(NULL, span + pg, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (map == MAP_FAILED) return 4;
    unsigned char *p = map + span - slack - SL;
    memcpy(p, sk, SL);
    if (mprotect(map + span, pg, strcmp(mode, "guard") ? PROT_READ : PROT_NONE)) return 5;
    printf("%s: sk %llu bytes ends %zu bytes before the page boundary; signing...\n", mode, SL, slack);
    fflush(stdout);
    int rc = sig_sign(p, SL, m, sizeof m, sig, &sl);
    int vr = sig_verify(pk, PL, sig, sl, m, sizeof m);
    printf("%s: sign rc=%d verify rc=%d\n", mode, rc, vr);
    return 0;
}
