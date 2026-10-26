/* HEP-QC: each public salt is the prefix of the generator's next state.
 *
 * Runs the unmodified reference encapsulation. It checks that
 *   - the 208-byte generator context is written only in its first
 *     CTX_LEN = 26 bytes;
 *   - a ciphertext's salt equals the first 16 bytes of the next state;
 *   - the remaining 80 state bits, checked against the next ciphertext's
 *     salt, regenerate that ciphertext and its shared secret.
 * The 80-bit search is scaled: SEARCH_BITS low bits are enumerated and the
 * other 80 - SEARCH_BITS come from the true state. The true state, secret
 * key and shared secret are used only as final controls.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "KEM_HEP_QC.h"
#include "parameters.h"
#include "symmetric.h"
#include "auxfunc.h"

extern shake256_xof_ctx shake256_prng_ctx;

#define STATE_BYTES (CTX_LEN * 8)          /* 208-byte context buffer */
#define LIVE_BYTES CTX_LEN                 /* 26 bytes actually written */
#define UNKNOWN_BITS ((LIVE_BYTES - SALT_BYTES) * 8)

static unsigned search_bits = 16;

static void F(uint8_t *out, unsigned out_bytes, const uint8_t state[STATE_BYTES]) {
    pseudoXOF(out_bytes * 8ULL, state, STATE_BYTES * 8ULL, out);
}

/* Replace the low `bits` bits of the 80 unknown state bits by `guess`. */
static void set_guess(uint8_t state[STATE_BYTES], uint64_t guess, unsigned bits) {
    for (unsigned b = 0; b < bits; ++b) {
        unsigned pos = LIVE_BYTES * 8 - 1 - b;
        uint8_t mask = (uint8_t)(1u << (7 - pos % 8));
        if (guess >> b & 1) state[pos / 8] |= mask;
        else state[pos / 8] &= (uint8_t)~mask;
    }
}

static int zero_tail(const uint8_t *state) {
    for (unsigned j = LIVE_BYTES; j < STATE_BYTES; ++j)
        if (state[j]) return 0;
    return 1;
}

/* Candidate state -> the salt the following encapsulation would publish. */
static int enc_salt_matches(const uint8_t state[STATE_BYTES], const uint8_t *salt) {
    uint8_t after_m[STATE_BYTES] = {0}, got[SALT_BYTES];
    F(after_m, LIVE_BYTES, state);
    F(got, SALT_BYTES, after_m);
    return memcmp(got, salt, SALT_BYTES) == 0;
}

/* Search salt || u for the u consistent with the next salt. */
static unsigned search(const uint8_t *salt, const uint8_t *truth,
                       const uint8_t *next_salt, uint8_t *found) {
    uint8_t cand[STATE_BYTES] = {0};
    unsigned hits = 0;
    memcpy(cand, salt, SALT_BYTES);
    /* Scaling hint: the high 80 - search_bits unknown bits from the truth. */
    memcpy(cand + SALT_BYTES, truth + SALT_BYTES, LIVE_BYTES - SALT_BYTES);
    for (uint64_t g = 0; g < (1ULL << search_bits); ++g) {
        set_guess(cand, g, search_bits);
        if (enc_salt_matches(cand, next_salt)) {
            if (!hits) memcpy(found, cand, STATE_BYTES);
            ++hits;
        }
    }
    return hits;
}

int main(int argc, char **argv) {
    static uint8_t pk[PUBLIC_KEY_BYTES], sk[SECRET_KEY_BYTES];
    static uint8_t ct1[CIPHERTEXT_BYTES], ct2[CIPHERTEXT_BYTES], ct3[CIPHERTEXT_BYTES];
    uint8_t ss1[SHARED_SECRET_BYTES], ss2[SHARED_SECRET_BYTES];
    uint8_t ss3[SHARED_SECRET_BYTES], ss_dec[SHARED_SECRET_BYTES];
    uint8_t truth[STATE_BYTES], found[STATE_BYTES], ignored[STATE_BYTES];
    uint8_t bad_salt[SALT_BYTES];
    unsigned long long pkl, skl, ctl, ssl;
    const unsigned salt_off = VEC_N_SIZE_BYTES + VEC_N1N2_SIZE_BYTES;

    if (argc > 1) search_bits = (unsigned)strtoul(argv[1], NULL, 0);
    if (search_bits < 8 || search_bits > 32 || search_bits > UNKNOWN_BITS) return 2;

    /* The submitted wrapper: no seeding, key generation then two encapsulations. */
    kem_keygen(pk, &pkl, sk, &skl);
    kem_enc(pk, PUBLIC_KEY_BYTES, ss1, &ssl, ct1, &ctl);
    memcpy(truth, shake256_prng_ctx.ctx, STATE_BYTES);
    int prefix = memcmp(truth, ct1 + salt_off, SALT_BYTES) == 0 && zero_tail(truth);
    kem_enc(pk, PUBLIC_KEY_BYTES, ss2, &ssl, ct2, &ctl);

    unsigned hits = search(ct1 + salt_off, truth, ct2 + salt_off, found);
    memcpy(bad_salt, ct1 + salt_off, SALT_BYTES);
    bad_salt[0] ^= 1;
    unsigned control = search(bad_salt, truth, ct2 + salt_off, ignored);

    int session = 0, state_match = 0;
    if (hits == 1) {
        state_match = memcmp(found, truth, STATE_BYTES) == 0;
        memcpy(shake256_prng_ctx.ctx, found, STATE_BYTES);
        kem_enc(pk, PUBLIC_KEY_BYTES, ss3, &ssl, ct3, &ctl);
        kem_dec(sk, SECRET_KEY_BYTES, ct2, CIPHERTEXT_BYTES, ss_dec, &ssl);
        session = memcmp(ct3, ct2, CIPHERTEXT_BYTES) == 0 &&
                  memcmp(ss3, ss2, SHARED_SECRET_BYTES) == 0 &&
                  memcmp(ss_dec, ss2, SHARED_SECRET_BYTES) == 0;
    }
    printf("salt_is_next_state_prefix=%d live_state_bits=%d search_bits=%u "
           "candidates=%u wrong_salt_candidates=%u state_match=%d "
           "second_session_key_recovered=%d\n",
           prefix, LIVE_BYTES * 8, search_bits, hits, control, state_match, session);
    if (prefix && hits == 1 && control == 0 && state_match && session) {
        printf("CONFIRMED kem-17-3 %s: salt is the next state's prefix; scaled %u-of-80-bit "
               "search regenerates the second encapsulation\n", CRYPTO_ALGNAME, search_bits);
        return 0;
    }
    printf("NOT REPRODUCED kem-17-3 %s\n", CRYPTO_ALGNAME);
    return 1;
}
