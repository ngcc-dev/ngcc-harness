#include <string.h>
#include <stdint.h>

#include "adkex_derand.h"
#include "ADKEX_parameters.h"
#include "auxfunc.h"

#include "dkecca.h"

// T <- H(LABEL || pk_B || m_1 || m_2; n)
static void adkex_transcript(
    uint8_t T[ADKEX_SSBITS / 8],
    const uint8_t pk_B[ADKEX_PKBITS / 8],
    const uint8_t m1[ADKEX_M1_BITS / 8],
    const uint8_t m2[ADKEX_M2_BITS / 8])
{
    uint8_t buf[ADKEX_TRANSCRIPT_BITS / 8];
    uint8_t *p = buf;

    memcpy(p, ADKEX_LABEL, ADKEX_LABEL_BITS / 8); p += ADKEX_LABEL_BITS / 8;
    memcpy(p, pk_B,         ADKEX_PKBITS    / 8);  p += ADKEX_PKBITS    / 8;
    memcpy(p, m1,           ADKEX_M1_BITS   / 8);  p += ADKEX_M1_BITS   / 8;
    memcpy(p, m2,           ADKEX_M2_BITS   / 8);

    ADKEX_KDF(ADKEX_KDF_OUTBITS, buf, ADKEX_TRANSCRIPT_BITS, T);

    memset(buf, 0, sizeof buf);
}

void ADKEX_init_b_derand(
    uint8_t pk_B[ADKEX_PKBITS / 8],
    uint8_t sk_B[ADKEX_SKBITS / 8],
    const uint8_t coins[ADKEX_INIT_B_COINBITS / 8])
{
    DKE_CCA_keygen_derand(pk_B, sk_B, coins);
}

void ADKEX_pass1_msg_a_derand(
    uint8_t m1[ADKEX_M1_BITS / 8],
    uint8_t sta[ADKEX_STA_MAX_BITS / 8],
    const uint8_t pk_B[ADKEX_PKBITS / 8],
    const uint8_t coins[ADKEX_PASS1_COINBITS / 8])
{
    uint8_t *sk_e      = sta + ADKEX_STA_OFF_SK_E_BITS / 8;
    uint8_t *ss_s      = sta + ADKEX_STA_OFF_SS_S_BITS / 8;
    uint8_t *m1_in_sta = sta + ADKEX_STA_OFF_M1_BITS   / 8;

    // (M_1, sk_e) <- DKEM.KeyGen(rho_1)
    DKE_CCA_keygen_derand(m1, sk_e, coins);

    // (ss_s, ct_S) <- DKEM.Encaps(pk_B, rho_3)
    DKE_CCA_enc_derand(m1 + ADKEX_PKBITS / 8, ss_s, pk_B,
                     coins + ADKEX_PASS1_KEYGEN_COINBITS / 8);

    // stash m_1 for derive_ss_a's transcript recomputation
    memcpy(m1_in_sta, m1, ADKEX_M1_BITS / 8);
}

void ADKEX_pass2_msg_b_derand(
    uint8_t m2[ADKEX_M2_BITS / 8],
    uint8_t stb[ADKEX_STB_MAX_BITS / 8],
    const uint8_t m1[ADKEX_M1_BITS / 8],
    const uint8_t pk_B[ADKEX_PKBITS / 8],
    const uint8_t sk_B[ADKEX_SKBITS / 8],
    const uint8_t coins[ADKEX_PASS2_COINBITS / 8])
{
    const uint8_t *M_1  = m1;
    const uint8_t *ct_S = m1 + ADKEX_PKBITS / 8;

    uint8_t *ss_e = stb + ADKEX_STB_OFF_SS_E_BITS / 8;
    uint8_t *ss_s = stb + ADKEX_STB_OFF_SS_S_BITS / 8;
    uint8_t *T    = stb + ADKEX_STB_OFF_T_BITS    / 8;

    // (ss_e, M_2) <- DKEM.Encaps(M_1, rho_2)
    DKE_CCA_enc_derand(m2, ss_e, M_1, coins);

    // ss_s <- DKEM.Decaps(sk_B, ct_S)
    DKE_CCA_dec(ss_s, sk_B, ct_S);

    adkex_transcript(T, pk_B, m1, m2);
}

void ADKEX_derive_ss_a(
    uint8_t ss[ADKEX_SSBITS / 8],
    const uint8_t m2[ADKEX_M2_BITS / 8],
    const uint8_t sta[ADKEX_STA_MAX_BITS / 8],
    const uint8_t pk_B[ADKEX_PKBITS / 8])
{
    const uint8_t *sk_e = sta + ADKEX_STA_OFF_SK_E_BITS / 8;
    const uint8_t *ss_s = sta + ADKEX_STA_OFF_SS_S_BITS / 8;
    const uint8_t *m1   = sta + ADKEX_STA_OFF_M1_BITS   / 8;

    uint8_t kdf_in[(3 * ADKEX_SSBITS) / 8];     // ss_e || ss_s || T

    DKE_CCA_dec(kdf_in, sk_e, m2);
    memcpy(kdf_in + ADKEX_SSBITS / 8, ss_s, ADKEX_SSBITS / 8);
    adkex_transcript(kdf_in + 2 * (ADKEX_SSBITS / 8), pk_B, m1, m2);

    ADKEX_KDF(ADKEX_KDF_OUTBITS, kdf_in, 3 * ADKEX_SSBITS, ss);

    memset(kdf_in, 0, sizeof kdf_in);
}

void ADKEX_derive_ss_b(
    uint8_t ss[ADKEX_SSBITS / 8],
    const uint8_t stb[ADKEX_STB_MAX_BITS / 8])
{
    ADKEX_KDF(ADKEX_KDF_OUTBITS, stb, 3 * ADKEX_SSBITS, ss);
}
