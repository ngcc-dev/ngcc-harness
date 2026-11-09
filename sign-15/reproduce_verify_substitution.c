/*
 * sign-15-14: sig_verify hashes one message and returns another.
 *
 *   forge     one signature on 0^32 verifies three chosen 32-byte messages
 *             when the signed message sn = sig || M* is opened in place
 *             (sn == m) or into a separate zeroed output buffer, as in
 *             specification Section 4.7; detached verification of the same
 *             messages and in-place opening of an honest signature on a
 *             nonzero message are controls and must reject.
 *   detached  an honest detached verification with the advertised
 *             CRYPTO_BYTES signature placed directly before an inaccessible
 *             guard page; the final copy loop reads past the signature and
 *             faults.
 */
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ngcc_seed(const unsigned char *, unsigned long long);
unsigned long long sig_get_pk_len_bytes(void), sig_get_sk_len_bytes(void),
    sig_get_sn_len_bytes(void);
int sig_keygen(unsigned char *, unsigned long long *, unsigned char *, unsigned long long *);
int sig_sign(const unsigned char *, unsigned long long, const unsigned char *,
             unsigned long long, unsigned char *, unsigned long long *);
int sig_verify(const unsigned char *, unsigned long long, const unsigned char *,
               unsigned long long, unsigned char *, unsigned long long);

#define R 32

static unsigned char *pk, *sk;
static unsigned long long pl, sl, cb;

static void setup(void)
{
    unsigned char seed[48];
    memset(seed, 0x15, sizeof seed);
    pl = sig_get_pk_len_bytes();
    sl = sig_get_sk_len_bytes();
    cb = sig_get_sn_len_bytes();
    pk = malloc(pl);
    sk = malloc(sl);
    if (!pk || !sk || ngcc_seed(seed, sizeof seed) || sig_keygen(pk, &pl, sk, &sl))
        exit(2);
}

/* Signature on msg, in a buffer large enough for the returned length. */
static unsigned char *sign(const unsigned char msg[R])
{
    unsigned long long len = cb + R;
    unsigned char *sig = calloc(cb + R, 1);
    if (!sig || sig_sign(sk, sl, msg, R, sig, &len))
        exit(3);
    return sig;
}

static int forge(void)
{
    static const char *target[3] = {
        "PAY MALLORY 1000000 EUR NOW!!!!!",
        "transfer all funds to account 66",
        "attacker-chosen 32-byte message.",
    };
    unsigned char zero[R] = {0}, one[R];
    unsigned char *sig0 = sign(zero), *sig1;
    int t, inplace = 0, separate = 0, detached_rejects = 0, rc;

    for (t = 0; t < 3; t++) {
        unsigned char *s = malloc(cb + R), *out = calloc(R, 1), *m = malloc(R);
        memcpy(s, sig0, cb);
        memcpy(s + cb, target[t], R);
        if (sig_verify(pk, pl, s, cb + R, s, R) == 0 && !memcmp(s, target[t], R))
            inplace++;
        memcpy(s, sig0, cb);
        memcpy(s + cb, target[t], R);
        if (sig_verify(pk, pl, s, cb + R, out, R) == 0 && !memcmp(out, target[t], R))
            separate++;
        memcpy(s, sig0, cb);
        memcpy(s + cb, target[t], R);
        memcpy(m, target[t], R);
        if (sig_verify(pk, pl, s, cb + R, m, R) == -1)
            detached_rejects++;
        free(s); free(out); free(m);
    }
    memset(one, 1, R);
    sig1 = sign(one);
    memcpy(sig1 + cb, one, R);
    rc = sig_verify(pk, pl, sig1, cb + R, sig1, R);
    printf("in_place_forgeries=%d separate_output_forgeries=%d "
           "detached_rejects=%d honest_nonzero_in_place_rc=%d\n",
           inplace, separate, detached_rejects, rc);
    return inplace == 3 && separate == 3 && detached_rejects == 3 && rc == -1 ? 0 : 1;
}

static int detached(void)
{
    unsigned char msg[R], *sig, *page, *exact;
    long ps = sysconf(_SC_PAGESIZE);
    size_t span = ((cb + ps - 1) / ps) * ps;
    int rc;
    memset(msg, 0x41, R);
    sig = sign(msg);
    page = mmap(NULL, span + ps, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (page == MAP_FAILED || mprotect(page + span, ps, PROT_NONE))
        return 2;
    exact = page + span - cb;      /* advertised size, ending at the guard */
    memcpy(exact, sig, cb);
    printf("detached_verify_start\n");
    fflush(stdout);
    rc = sig_verify(pk, pl, exact, cb, msg, R);
    printf("detached_verify_rc=%d (no fault)\n", rc);
    return 0;
}

int main(int argc, char **argv)
{
    setup();
    if (argc > 1 && !strcmp(argv[1], "detached"))
        return detached();
    return forge();
}
