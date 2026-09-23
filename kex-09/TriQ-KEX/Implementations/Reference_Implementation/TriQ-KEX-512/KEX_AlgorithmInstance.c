/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "KEX_AlgorithmInstance.h"
#include "api.h"
#include "auxfunc.h"
#include "parameters.h"
#include "parsing.h"
#include "drng.h"
#include "symmetric.h"
#include "triq_pke.h"

extern DRNG_ctx drng_algorithm;

#define TRIQ_KEX_PASS_NUM 2ULL
#define TRIQ_KEX_PK_BYTES ((unsigned long long)CRYPTO_PUBLICKEYBYTES)
#define TRIQ_KEX_KEM_SK_BYTES ((unsigned long long)CRYPTO_SECRETKEYBYTES)
#define TRIQ_KEX_PRF_KEY_BYTES ((unsigned long long)SEED_BYTES)
#define TRIQ_KEX_SK_BYTES (TRIQ_KEX_KEM_SK_BYTES + 2ULL * TRIQ_KEX_PRF_KEY_BYTES)
#define TRIQ_KEX_CT_BYTES ((unsigned long long)CRYPTO_CIPHERTEXTBYTES)
#define TRIQ_KEX_SS_BYTES ((unsigned long long)CRYPTO_BYTES)
#define TRIQ_KEX_ID_BYTES 4ULL
#define TRIQ_KEX_ID_PAIR_BYTES (2ULL * TRIQ_KEX_ID_BYTES)
#define TRIQ_KEX_M1_BYTES (TRIQ_KEX_ID_PAIR_BYTES + TRIQ_KEX_CT_BYTES + TRIQ_KEX_PK_BYTES)
#define TRIQ_KEX_M2_BYTES (TRIQ_KEX_ID_PAIR_BYTES + 2ULL * TRIQ_KEX_CT_BYTES)
#define TRIQ_KEX_STA_BYTES (TRIQ_KEX_CT_BYTES + TRIQ_KEX_PK_BYTES + TRIQ_KEX_KEM_SK_BYTES + TRIQ_KEX_SS_BYTES)
#define TRIQ_KEX_STB_BYTES (2ULL * TRIQ_KEX_CT_BYTES + 3ULL * TRIQ_KEX_SS_BYTES)

#define SK_KEM_OFF 0ULL
#define SK_SIGMA_OFF TRIQ_KEX_KEM_SK_BYTES
#define SK_SIGMA_PRIME_OFF (SK_SIGMA_OFF + TRIQ_KEX_PRF_KEY_BYTES)

#define STA_CT_A_OFF 0ULL
#define STA_PK_T_OFF (STA_CT_A_OFF + TRIQ_KEX_CT_BYTES)
#define STA_SK_T_OFF (STA_PK_T_OFF + TRIQ_KEX_PK_BYTES)
#define STA_K_A_OFF (STA_SK_T_OFF + TRIQ_KEX_KEM_SK_BYTES)

#define MSG_ID_A_OFF 0ULL
#define MSG_ID_B_OFF (MSG_ID_A_OFF + TRIQ_KEX_ID_BYTES)
#define M1_CT_A_OFF (MSG_ID_B_OFF + TRIQ_KEX_ID_BYTES)
#define M1_PK_T_OFF (M1_CT_A_OFF + TRIQ_KEX_CT_BYTES)
#define M2_CT_B_OFF (MSG_ID_B_OFF + TRIQ_KEX_ID_BYTES)
#define M2_CT_T_OFF (M2_CT_B_OFF + TRIQ_KEX_CT_BYTES)

#define STB_CT_B_OFF 0ULL
#define STB_CT_T_OFF (STB_CT_B_OFF + TRIQ_KEX_CT_BYTES)
#define STB_K_A_OFF (STB_CT_T_OFF + TRIQ_KEX_CT_BYTES)
#define STB_K_B_OFF (STB_K_A_OFF + TRIQ_KEX_SS_BYTES)
#define STB_K_T_OFF (STB_K_B_OFF + TRIQ_KEX_SS_BYTES)

static const unsigned char triq_kex_id_a[TRIQ_KEX_ID_BYTES] = { 'I', 'D', '_', 'A' };
static const unsigned char triq_kex_id_b[TRIQ_KEX_ID_BYTES] = { 'I', 'D', '_', 'B' };

static void triq_kex_write_id_pair(unsigned char *msg)
{
    memcpy(msg + MSG_ID_A_OFF, triq_kex_id_a, (size_t)TRIQ_KEX_ID_BYTES);
    memcpy(msg + MSG_ID_B_OFF, triq_kex_id_b, (size_t)TRIQ_KEX_ID_BYTES);
}

static int triq_kex_has_id_pair(const unsigned char *msg)
{
    return memcmp(msg + MSG_ID_A_OFF, triq_kex_id_a, (size_t)TRIQ_KEX_ID_BYTES) == 0 &&
           memcmp(msg + MSG_ID_B_OFF, triq_kex_id_b, (size_t)TRIQ_KEX_ID_BYTES) == 0;
}

static int triq_seed_prng(void)
{
    unsigned char entropy[48] = {0};
    unsigned char personalization[48] = {0};

    if (get_random_number(&drng_algorithm, entropy, sizeof(entropy) * 8ULL) != 0)
        return -11;
    if (get_random_number(&drng_algorithm, personalization, sizeof(personalization) * 8ULL) != 0)
        return -12;

    prng_init(entropy, personalization, (uint32_t)sizeof(entropy), (uint32_t)sizeof(personalization));
    return 0;
}

static int triq_random_bytes(unsigned char *out, unsigned long long outlen)
{
    if (get_random_number(&drng_algorithm, out, outlen * 8ULL) != 0) {
        memset(out, 0, (size_t)outlen);
        return -13;
    }
    return 0;
}

static int triq_xof_label(unsigned char *out, unsigned long long outlen,
                          const unsigned char *label, unsigned long long label_len,
                          const unsigned char *a, unsigned long long a_len,
                          const unsigned char *b, unsigned long long b_len,
                          const unsigned char *c, unsigned long long c_len)
{
    const unsigned long long input_len = label_len + a_len + b_len + c_len;
    unsigned char *input = (unsigned char *)malloc((size_t)(input_len == 0 ? 1ULL : input_len));
    unsigned long long off = 0;
    int ret;

    if (input == NULL)
        return -14;
    if (label_len != 0) { memcpy(input + off, label, (size_t)label_len); off += label_len; }
    if (a_len != 0) { memcpy(input + off, a, (size_t)a_len); off += a_len; }
    if (b_len != 0) { memcpy(input + off, b, (size_t)b_len); off += b_len; }
    if (c_len != 0) { memcpy(input + off, c, (size_t)c_len); off += c_len; }

    ret = pseudoXOF(outlen * 8ULL, input, input_len * 8ULL, out);
    memset(input, 0, (size_t)input_len);
    free(input);
    return ret == 0 ? 0 : -15;
}

static int triq_fsxy_prf(unsigned char *out, unsigned long long outlen,
                         const unsigned char *key, unsigned long long key_len,
                         const unsigned char *input, unsigned long long input_len,
                         const char *domain)
{
    const unsigned char *label = (const unsigned char *)domain;
    return triq_xof_label(out, outlen, label, (unsigned long long)strlen(domain), key, key_len, input, input_len, NULL, 0);
}

static int triq_fsxy_coins(unsigned char *coins, const unsigned char *sigma,
                           const unsigned char *sigma_prime, const char *domain)
{
    unsigned char r[SEED_BYTES] = {0};
    unsigned char r_prime[SEED_BYTES] = {0};
    unsigned char left[PARAM_SECURITY_BYTES + SALT_BYTES] = {0};
    unsigned char right[PARAM_SECURITY_BYTES + SALT_BYTES] = {0};
    unsigned long long i;
    int ret;

    ret = triq_random_bytes(r, SEED_BYTES);
    if (ret != 0)
        return ret;
    ret = triq_random_bytes(r_prime, SEED_BYTES);
    if (ret != 0)
        return ret;

    ret = triq_fsxy_prf(left, sizeof(left), sigma, SEED_BYTES, r, SEED_BYTES, domain);
    if (ret != 0)
        return ret;
    ret = triq_fsxy_prf(right, sizeof(right), r_prime, SEED_BYTES, sigma_prime, SEED_BYTES, "TriQ-FSXY-Fprime");
    if (ret != 0)
        return ret;

    for (i = 0; i < (unsigned long long)sizeof(left); i++)
        coins[i] = left[i] ^ right[i];

    memset(r, 0, sizeof r);
    memset(r_prime, 0, sizeof r_prime);
    memset(left, 0, sizeof left);
    memset(right, 0, sizeof right);
    return 0;
}

static int triq_kem_keypair_from_seed(unsigned char *pk, unsigned char *sk, const unsigned char seed_kem[SEED_BYTES])
{
    uint8_t seed_pke[SEED_BYTES] = {0};
    uint8_t sigma[PARAM_SECURITY_BYTES] = {0};
    uint8_t dk_pke[SEED_BYTES] = {0};
    triq_xof_ctx ctx_kem;

    xof_init(&ctx_kem, seed_kem, SEED_BYTES);
    xof_get_bytes(&ctx_kem, seed_pke, SEED_BYTES);
    xof_get_bytes(&ctx_kem, sigma, PARAM_SECURITY_BYTES);

    triq_pke_keygen(pk, dk_pke, seed_pke);
    memcpy(sk, pk, PUBLIC_KEY_BYTES);
    memcpy(sk + PUBLIC_KEY_BYTES, dk_pke, SEED_BYTES);
    memcpy(sk + PUBLIC_KEY_BYTES + SEED_BYTES, sigma, PARAM_SECURITY_BYTES);
    memcpy(sk + PUBLIC_KEY_BYTES + SEED_BYTES + PARAM_SECURITY_BYTES, seed_kem, SEED_BYTES);

    memset(seed_pke, 0, sizeof seed_pke);
    memset(sigma, 0, sizeof sigma);
    memset(dk_pke, 0, sizeof dk_pke);
    memset(&ctx_kem, 0, sizeof ctx_kem);
    return 0;
}

static int triq_kem_enc_with_coins(unsigned char *ct, unsigned char *ss, const unsigned char *pk,
                                   const unsigned char *coins)
{
    uint8_t hash_ek[SEED_BYTES] = {0};
    uint8_t k_theta[SHARED_SECRET_BYTES + SEED_BYTES] = {0};
    uint8_t theta[SEED_BYTES] = {0};
    ciphertext_kem_t c_kem = {0};

    hash_h(hash_ek, pk);
    hash_g(k_theta, hash_ek, coins, coins + PARAM_SECURITY_BYTES);
    memcpy(theta, k_theta + SHARED_SECRET_BYTES, SEED_BYTES);
    triq_pke_encrypt(&c_kem.c_pke, pk, (const uint64_t *)coins, theta);
    memcpy(c_kem.salt, coins + PARAM_SECURITY_BYTES, SALT_BYTES);
    triq_c_kem_to_string(ct, &c_kem);
    memcpy(ss, k_theta, SHARED_SECRET_BYTES);

    memset(hash_ek, 0, sizeof hash_ek);
    memset(k_theta, 0, sizeof k_theta);
    memset(theta, 0, sizeof theta);
    memset(&c_kem, 0, sizeof c_kem);
    return 0;
}

static int triq_wkem_dec(unsigned char *k_prime, const unsigned char *ct, const unsigned char *sk)
{
    uint8_t ek_pke[PUBLIC_KEY_BYTES] = {0};
    uint8_t dk_pke[SEED_BYTES] = {0};
    uint8_t m_prime[PARAM_SECURITY_BYTES] = {0};
    uint8_t h_ek[SEED_BYTES] = {0};
    uint8_t k_theta[SHARED_SECRET_BYTES + SEED_BYTES] = {0};
    ciphertext_kem_t c_kem = {0};
    uint8_t result;

    memcpy(ek_pke, sk, PUBLIC_KEY_BYTES);
    memcpy(dk_pke, sk + PUBLIC_KEY_BYTES, SEED_BYTES);
    triq_c_kem_from_string(&c_kem.c_pke, c_kem.salt, ct);

    result = triq_pke_decrypt((uint64_t *)m_prime, dk_pke, &c_kem.c_pke);
    if (result != 0)
        return -1;

    hash_h(h_ek, ek_pke);
    hash_g(k_theta, h_ek, m_prime, c_kem.salt);
    memcpy(k_prime, k_theta, SHARED_SECRET_BYTES);

    memset(dk_pke, 0, sizeof dk_pke);
    memset(m_prime, 0, sizeof m_prime);
    memset(k_theta, 0, sizeof k_theta);
    return 0;
}

static int triq_fsxy_final_key(unsigned char *ss,
                               const unsigned char *pka, const unsigned char *pkb,
                               const unsigned char *ct_a, const unsigned char *pk_t,
                               const unsigned char *ct_b, const unsigned char *ct_t,
                               const unsigned char *k_a, const unsigned char *k_b, const unsigned char *k_t)
{
    static const unsigned char kdf_label[] = "TriQ-FSXY-KDF";
    static const unsigned char kdf_s[SEED_BYTES] = {
        0x9b, 0x4a, 0x78, 0x1b, 0x18, 0xba, 0x11, 0x5c,
        0x3e, 0x59, 0xd3, 0xff, 0x1d, 0x4d, 0xbc, 0x91,
        0xf7, 0x1e, 0x3a, 0x7a, 0x78, 0xaa, 0x08, 0x25,
        0xff, 0x8c, 0x0b, 0x41, 0xde, 0x73, 0xa1, 0x60,
        0xbb, 0x1c, 0x42, 0x1d, 0x39, 0x6b, 0x41, 0x07,
        0x0c, 0xa9, 0xbf, 0xb1, 0xe7, 0xe5, 0xb8, 0x59,
        0xef, 0x25, 0xb8, 0x69, 0xe5, 0x02, 0x63, 0xbe,
        0x27, 0x3a, 0xc5, 0x95, 0x25, 0xa4, 0x0e, 0xf5
    };
    static const unsigned char st_label[] = "TriQ-FSXY-ST";
    unsigned char k1[SEED_BYTES] = {0};
    unsigned char k2[SEED_BYTES] = {0};
    unsigned char k3[SEED_BYTES] = {0};
    unsigned char g1[SHARED_SECRET_BYTES] = {0};
    unsigned char g2[SHARED_SECRET_BYTES] = {0};
    unsigned char g3[SHARED_SECRET_BYTES] = {0};
    const unsigned long long st_len =
        (unsigned long long)(sizeof(st_label) - 1U) +
        TRIQ_KEX_ID_PAIR_BYTES +
        3ULL * TRIQ_KEX_PK_BYTES + 3ULL * TRIQ_KEX_CT_BYTES;
    unsigned char *st = (unsigned char *)malloc((size_t)st_len);
    unsigned long long off = 0;
    int ret;

    if (st == NULL)
        return -16;
#define APPEND_ST(ptr, len) do { memcpy(st + off, (ptr), (size_t)(len)); off += (unsigned long long)(len); } while (0)
    APPEND_ST(st_label, sizeof(st_label) - 1U);
    APPEND_ST(triq_kex_id_a, TRIQ_KEX_ID_BYTES);
    APPEND_ST(triq_kex_id_b, TRIQ_KEX_ID_BYTES);
    APPEND_ST(pka, TRIQ_KEX_PK_BYTES);
    APPEND_ST(pkb, TRIQ_KEX_PK_BYTES);
    APPEND_ST(ct_a, TRIQ_KEX_CT_BYTES);
    APPEND_ST(pk_t, TRIQ_KEX_PK_BYTES);
    APPEND_ST(ct_b, TRIQ_KEX_CT_BYTES);
    APPEND_ST(ct_t, TRIQ_KEX_CT_BYTES);
#undef APPEND_ST

    ret = triq_xof_label(k1, SEED_BYTES, kdf_label, sizeof(kdf_label) - 1U, kdf_s, SEED_BYTES, k_a, TRIQ_KEX_SS_BYTES, NULL, 0);
    if (ret == 0) ret = triq_xof_label(k2, SEED_BYTES, kdf_label, sizeof(kdf_label) - 1U, kdf_s, SEED_BYTES, k_b, TRIQ_KEX_SS_BYTES, NULL, 0);
    if (ret == 0) ret = triq_xof_label(k3, SEED_BYTES, kdf_label, sizeof(kdf_label) - 1U, kdf_s, SEED_BYTES, k_t, TRIQ_KEX_SS_BYTES, NULL, 0);
    if (ret == 0) ret = triq_fsxy_prf(g1, SHARED_SECRET_BYTES, k1, SEED_BYTES, st, st_len, "TriQ-FSXY-G");
    if (ret == 0) ret = triq_fsxy_prf(g2, SHARED_SECRET_BYTES, k2, SEED_BYTES, st, st_len, "TriQ-FSXY-G");
    if (ret == 0) ret = triq_fsxy_prf(g3, SHARED_SECRET_BYTES, k3, SEED_BYTES, st, st_len, "TriQ-FSXY-G");
    if (ret == 0) {
        unsigned long long i;
        for (i = 0; i < TRIQ_KEX_SS_BYTES; i++)
            ss[i] = g1[i] ^ g2[i] ^ g3[i];
    }

    memset(st, 0, (size_t)st_len);
    free(st);
    memset(k1, 0, sizeof k1);
    memset(k2, 0, sizeof k2);
    memset(k3, 0, sizeof k3);
    memset(g1, 0, sizeof g1);
    memset(g2, 0, sizeof g2);
    memset(g3, 0, sizeof g3);
    return ret == 0 ? 0 : -17;
}

unsigned long long kex_get_passes_num(void) { return TRIQ_KEX_PASS_NUM; }
unsigned long long kex_get_pk_len_bytes(void) { return TRIQ_KEX_PK_BYTES; }
unsigned long long kex_get_sk_len_bytes(void) { return TRIQ_KEX_SK_BYTES; }
unsigned long long kex_get_sta_len_bytes(void) { return TRIQ_KEX_STA_BYTES; }
unsigned long long kex_get_stb_len_bytes(void) { return TRIQ_KEX_STB_BYTES; }
unsigned long long kex_get_ss_len_bytes(void) { return TRIQ_KEX_SS_BYTES; }
unsigned long long kex_get_total_msg_len_bytes(void) { return TRIQ_KEX_M1_BYTES + TRIQ_KEX_M2_BYTES; }

int kex_init_a(unsigned char *pka, unsigned long long *pka_len_bytes,
               unsigned char *ska, unsigned long long *ska_len_bytes,
               unsigned char *sta, unsigned long long *sta_len_bytes)
{
    int ret = triq_seed_prng();
    (void)sta;
    if (ret != 0) return ret;
    if (crypto_kem_keypair(pka, ska + SK_KEM_OFF) != 0) return -21;
    if (triq_random_bytes(ska + SK_SIGMA_OFF, TRIQ_KEX_PRF_KEY_BYTES) != 0) return -22;
    if (triq_random_bytes(ska + SK_SIGMA_PRIME_OFF, TRIQ_KEX_PRF_KEY_BYTES) != 0) return -23;
    *pka_len_bytes = TRIQ_KEX_PK_BYTES;
    *ska_len_bytes = TRIQ_KEX_SK_BYTES;
    *sta_len_bytes = 0;
    return 0;
}

int kex_init_b(unsigned char *pkb, unsigned long long *pkb_len_bytes,
               unsigned char *skb, unsigned long long *skb_len_bytes,
               unsigned char *stb, unsigned long long *stb_len_bytes)
{
    int ret = triq_seed_prng();
    (void)stb;
    if (ret != 0) return ret;
    if (crypto_kem_keypair(pkb, skb + SK_KEM_OFF) != 0) return -24;
    if (triq_random_bytes(skb + SK_SIGMA_OFF, TRIQ_KEX_PRF_KEY_BYTES) != 0) return -25;
    if (triq_random_bytes(skb + SK_SIGMA_PRIME_OFF, TRIQ_KEX_PRF_KEY_BYTES) != 0) return -26;
    *pkb_len_bytes = TRIQ_KEX_PK_BYTES;
    *skb_len_bytes = TRIQ_KEX_SK_BYTES;
    *stb_len_bytes = 0;
    return 0;
}

int kex_generate_pass1_msg_a(unsigned char *ska, unsigned long long ska_len_bytes,
                             unsigned char *pkb, unsigned long long pkb_len_bytes,
                             unsigned char *sta, unsigned long long *sta_len_bytes,
                             unsigned char *m1, unsigned long long *m1_len_bytes)
{
    unsigned char *ct_a = m1 + M1_CT_A_OFF;
    unsigned char *pk_t = m1 + M1_PK_T_OFF;
    unsigned char *sk_t = sta + STA_SK_T_OFF;
    unsigned char *k_a = sta + STA_K_A_OFF;
    unsigned char coins[PARAM_SECURITY_BYTES + SALT_BYTES] = {0};
    unsigned char seed_t[SEED_BYTES] = {0};
    int ret;

    if (ska_len_bytes != TRIQ_KEX_SK_BYTES) return -31;
    if (pkb_len_bytes != TRIQ_KEX_PK_BYTES) return -32;

    triq_kex_write_id_pair(m1);

    ret = triq_fsxy_coins(coins, ska + SK_SIGMA_OFF, ska + SK_SIGMA_PRIME_OFF, "TriQ-FSXY-F");
    if (ret != 0) return ret;
    if (triq_kem_enc_with_coins(ct_a, k_a, pkb, coins) != 0) return -33;

    ret = triq_random_bytes(seed_t, SEED_BYTES);
    if (ret != 0) return ret;
    if (triq_kem_keypair_from_seed(pk_t, sk_t, seed_t) != 0) return -34;

    memcpy(sta + STA_CT_A_OFF, ct_a, (size_t)TRIQ_KEX_CT_BYTES);
    memcpy(sta + STA_PK_T_OFF, pk_t, (size_t)TRIQ_KEX_PK_BYTES);
    *sta_len_bytes = TRIQ_KEX_STA_BYTES;
    *m1_len_bytes = TRIQ_KEX_M1_BYTES;
    memset(coins, 0, sizeof coins);
    memset(seed_t, 0, sizeof seed_t);
    return 0;
}

int kex_generate_pass2_msg_b(unsigned char *skb, unsigned long long skb_len_bytes,
                             unsigned char *pka, unsigned long long pka_len_bytes,
                             unsigned char *m1, unsigned long long m1_len_bytes,
                             unsigned char *stb, unsigned long long *stb_len_bytes,
                             unsigned char *m2, unsigned long long *m2_len_bytes)
{
    const unsigned char *ct_a = m1 + M1_CT_A_OFF;
    const unsigned char *pk_t = m1 + M1_PK_T_OFF;
    unsigned char *ct_b = m2 + M2_CT_B_OFF;
    unsigned char *ct_t = m2 + M2_CT_T_OFF;
    unsigned char *k_a = stb + STB_K_A_OFF;
    unsigned char *k_b = stb + STB_K_B_OFF;
    unsigned char *k_t = stb + STB_K_T_OFF;
    unsigned char coins_b[PARAM_SECURITY_BYTES + SALT_BYTES] = {0};
    unsigned char coins_t[PARAM_SECURITY_BYTES + SALT_BYTES] = {0};
    int ret;

    if (skb_len_bytes != TRIQ_KEX_SK_BYTES) return -41;
    if (pka_len_bytes != TRIQ_KEX_PK_BYTES) return -42;
    if (m1_len_bytes != TRIQ_KEX_M1_BYTES) return -43;
    if (!triq_kex_has_id_pair(m1)) return -47;

    triq_kex_write_id_pair(m2);

    if (crypto_kem_dec(k_a, ct_a, skb + SK_KEM_OFF) != 0) return -44;

    ret = triq_fsxy_coins(coins_b, skb + SK_SIGMA_OFF, skb + SK_SIGMA_PRIME_OFF, "TriQ-FSXY-F");
    if (ret != 0) return ret;
    if (triq_kem_enc_with_coins(ct_b, k_b, pka, coins_b) != 0) return -45;

    ret = triq_random_bytes(coins_t, sizeof(coins_t));
    if (ret != 0) return ret;
    if (triq_kem_enc_with_coins(ct_t, k_t, pk_t, coins_t) != 0) return -46;

    memcpy(stb + STB_CT_B_OFF, ct_b, (size_t)TRIQ_KEX_CT_BYTES);
    memcpy(stb + STB_CT_T_OFF, ct_t, (size_t)TRIQ_KEX_CT_BYTES);
    *stb_len_bytes = TRIQ_KEX_STB_BYTES;
    *m2_len_bytes = TRIQ_KEX_M2_BYTES;
    memset(coins_b, 0, sizeof coins_b);
    memset(coins_t, 0, sizeof coins_t);
    return 1;
}

int kex_generate_pass3_msg_a(unsigned char *ska, unsigned long long ska_len_bytes,
                             unsigned char *pkb, unsigned long long pkb_len_bytes,
                             unsigned char *m2, unsigned long long m2_len_bytes,
                             unsigned char *sta, unsigned long long *sta_len_bytes,
                             unsigned char *m3, unsigned long long *m3_len_bytes)
{
    (void)ska; (void)ska_len_bytes; (void)pkb; (void)pkb_len_bytes;
    (void)m2; (void)m2_len_bytes; (void)sta; (void)sta_len_bytes;
    (void)m3; (void)m3_len_bytes;
    return -50;
}

int kex_derive_ss_a(unsigned char *ska, unsigned long long ska_len_bytes,
                    unsigned char *pkb, unsigned long long pkb_len_bytes,
                    unsigned char *mb, unsigned long long mb_len_bytes,
                    unsigned char *sta, unsigned long long sta_len_bytes,
                    unsigned char *ssa, unsigned long long *ssa_len_bytes)
{
    const unsigned char *pka = ska + SK_KEM_OFF;
    const unsigned char *ct_a = sta + STA_CT_A_OFF;
    const unsigned char *pk_t = sta + STA_PK_T_OFF;
    const unsigned char *sk_t = sta + STA_SK_T_OFF;
    const unsigned char *k_a = sta + STA_K_A_OFF;
    const unsigned char *ct_b = mb + M2_CT_B_OFF;
    const unsigned char *ct_t = mb + M2_CT_T_OFF;
    unsigned char k_b[CRYPTO_BYTES] = {0};
    unsigned char k_t[CRYPTO_BYTES] = {0};

    if (ska_len_bytes != TRIQ_KEX_SK_BYTES) return -61;
    if (pkb_len_bytes != TRIQ_KEX_PK_BYTES) return -62;
    if (mb_len_bytes != TRIQ_KEX_M2_BYTES) return -63;
    if (sta_len_bytes != TRIQ_KEX_STA_BYTES) return -64;
    if (!triq_kex_has_id_pair(mb)) return -68;

    if (crypto_kem_dec(k_b, ct_b, ska + SK_KEM_OFF) != 0) return -65;
    if (triq_wkem_dec(k_t, ct_t, sk_t) != 0) return -66;
    if (triq_fsxy_final_key(ssa, pka, pkb, ct_a, pk_t, ct_b, ct_t, k_a, k_b, k_t) != 0) return -67;
    *ssa_len_bytes = TRIQ_KEX_SS_BYTES;
    return 0;
}

int kex_derive_ss_b(unsigned char *skb, unsigned long long skb_len_bytes,
                    unsigned char *pka, unsigned long long pka_len_bytes,
                    unsigned char *ma, unsigned long long ma_len_bytes,
                    unsigned char *stb, unsigned long long stb_len_bytes,
                    unsigned char *ssb, unsigned long long *ssb_len_bytes)
{
    const unsigned char *pkb = skb + SK_KEM_OFF;
    const unsigned char *ct_a = ma + M1_CT_A_OFF;
    const unsigned char *pk_t = ma + M1_PK_T_OFF;
    const unsigned char *ct_b = stb + STB_CT_B_OFF;
    const unsigned char *ct_t = stb + STB_CT_T_OFF;
    const unsigned char *k_a = stb + STB_K_A_OFF;
    const unsigned char *k_b = stb + STB_K_B_OFF;
    const unsigned char *k_t = stb + STB_K_T_OFF;

    if (skb_len_bytes != TRIQ_KEX_SK_BYTES) return -71;
    if (pka_len_bytes != TRIQ_KEX_PK_BYTES) return -72;
    if (ma_len_bytes != TRIQ_KEX_M1_BYTES) return -73;
    if (stb_len_bytes != TRIQ_KEX_STB_BYTES) return -74;
    if (!triq_kex_has_id_pair(ma)) return -76;
    if (triq_fsxy_final_key(ssb, pka, pkb, ct_a, pk_t, ct_b, ct_t, k_a, k_b, k_t) != 0) return -75;
    *ssb_len_bytes = TRIQ_KEX_SS_BYTES;
    return 0;
}
