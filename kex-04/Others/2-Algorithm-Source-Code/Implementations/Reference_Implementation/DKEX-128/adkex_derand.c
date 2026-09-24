#include <string.h>
#include <stdint.h>

#include "adkex_derand.h"
#include "ADKEX_parameters.h"
#include "adkex_sig.h"
#include "auxfunc.h"

#include "dkecpa.h"

// T <- H(LABEL_A || LABEL_B || pk_A || pk_B || M_1 || M_2; n)   (single binder, v20260618)
static void adkex_transcript(
    uint8_t       T[ADKEX_SSBITS / 8],
    const uint8_t pk_A[ADKEX_SIGPKBITS / 8],
    const uint8_t pk_B[ADKEX_SIGPKBITS / 8],
    const uint8_t M_1[ADKEX_DKEXPKBITS / 8],
    const uint8_t M_2[ADKEX_DKEXCTBITS / 8])
{
    uint8_t buf[ADKEX_TRANSCRIPT_BITS / 8];
    uint8_t *p = buf;

    memcpy(p, ADKEX_LABEL_A, ADKEX_LABEL_BITS / 8);  p += ADKEX_LABEL_BITS / 8;
    memcpy(p, ADKEX_LABEL_B, ADKEX_LABEL_BITS / 8);  p += ADKEX_LABEL_BITS / 8;
    memcpy(p, pk_A,  ADKEX_SIGPKBITS / 8);   p += ADKEX_SIGPKBITS / 8;
    memcpy(p, pk_B,  ADKEX_SIGPKBITS / 8);   p += ADKEX_SIGPKBITS / 8;
    memcpy(p, M_1,   ADKEX_DKEXPKBITS / 8);  p += ADKEX_DKEXPKBITS / 8;
    memcpy(p, M_2,   ADKEX_DKEXCTBITS / 8);

    ADKEX_KDF(ADKEX_KDF_OUTBITS, buf, ADKEX_TRANSCRIPT_BITS, T);

    memset(buf, 0, sizeof buf);
}

void ADKEX_init_a_derand(
    uint8_t pk_A[ADKEX_SIGPKBITS / 8],
    uint8_t sk_A[ADKEX_SIGSKBITS / 8],
    const uint8_t coins[ADKEX_INIT_A_COINBITS / 8])
{
    adkex_sig_keygen(pk_A, sk_A, coins);
}

void ADKEX_init_b_derand(
    uint8_t pk_B[ADKEX_SIGPKBITS / 8],
    uint8_t sk_B[ADKEX_SIGSKBITS / 8],
    const uint8_t coins[ADKEX_INIT_B_COINBITS / 8])
{
    adkex_sig_keygen(pk_B, sk_B, coins);
}

void ADKEX_pass1_msg_a_derand(
    uint8_t m1[ADKEX_M1_BITS / 8],
    uint8_t sta[ADKEX_STA_MAX_BITS / 8],
    const uint8_t pk_A[ADKEX_SIGPKBITS / 8],
    const uint8_t coins[ADKEX_PASS1_COINBITS / 8])
{
    uint8_t *sk_e       = sta + ADKEX_STA_OFF_SK_E_BITS / 8;
    uint8_t *M_1_in_st  = sta + ADKEX_STA_OFF_M1_BITS   / 8;
    uint8_t *pk_A_in_st = sta + ADKEX_STA_OFF_PK_A_BITS / 8;

    DKE_CPA_keygen_derand(m1, sk_e, coins);

    memcpy(M_1_in_st,  m1,   ADKEX_DKEXPKBITS / 8);
    memcpy(pk_A_in_st, pk_A, ADKEX_SIGPKBITS  / 8);
}

void ADKEX_pass2_msg_b_derand(
    uint8_t m2[ADKEX_M2_BITS / 8],
    uint8_t stb[ADKEX_STB_MAX_BITS / 8],
    const uint8_t m1[ADKEX_M1_BITS / 8],
    const uint8_t pk_A[ADKEX_SIGPKBITS / 8],
    const uint8_t pk_B[ADKEX_SIGPKBITS / 8],
    const uint8_t sk_B[ADKEX_SIGSKBITS / 8],
    const uint8_t coins[ADKEX_PASS2_DKEX_COINBITS / 8])
{
    const uint8_t *M_1 = m1;

    uint8_t *M_2     = m2 + ADKEX_M2_OFF_M2_BITS    / 8;
    uint8_t *sigma_B = m2 + ADKEX_M2_OFF_SIGMA_BITS / 8;

    uint8_t *ss_out = stb + ADKEX_STB_OFF_SS_BITS / 8;
    uint8_t *T_st   = stb + ADKEX_STB_OFF_T_BITS  / 8;

    DKE_CPA_enc_derand(M_2, ss_out, M_1, coins);

    // single transcript binder T, signed by B and stashed in st_B = (ss, T)
    adkex_transcript(T_st, pk_A, pk_B, M_1, M_2);

    adkex_sig_sign(sigma_B, sk_B, T_st, ADKEX_SSBITS);
}

int ADKEX_pass3_msg_a_derand(
    uint8_t m3[ADKEX_M3_BITS / 8],
    uint8_t sta[ADKEX_STA_MAX_BITS / 8],
    const uint8_t m2[ADKEX_M2_BITS / 8],
    const uint8_t pk_B[ADKEX_SIGPKBITS / 8],
    const uint8_t sk_A[ADKEX_SIGSKBITS / 8])
{
    uint8_t *sk_e = sta + ADKEX_STA_OFF_SK_E_BITS / 8;
    uint8_t *M_1  = sta + ADKEX_STA_OFF_M1_BITS   / 8;
    uint8_t *pk_A = sta + ADKEX_STA_OFF_PK_A_BITS / 8;

    const uint8_t *M_2     = m2 + ADKEX_M2_OFF_M2_BITS    / 8;
    const uint8_t *sigma_B = m2 + ADKEX_M2_OFF_SIGMA_BITS / 8;

    uint8_t T[ADKEX_SSBITS / 8];
    adkex_transcript(T, pk_A, pk_B, M_1, M_2);

    if (!adkex_sig_verify(pk_B, sigma_B, T, ADKEX_SSBITS)) {
        memset(T, 0, sizeof T);
        return -1;
    }

    uint8_t ss[ADKEX_SSBITS / 8];
    DKE_CPA_dec(ss, sk_e, M_2);

    // A signs the same single transcript T
    adkex_sig_sign(m3, sk_A, T, ADKEX_SSBITS);

    // erase sk_e, M_1, pk_A from st_A; replace with st_A = (ss, T)
    memset(sta, 0, ADKEX_STA_MAX_BITS / 8);
    memcpy(sta + ADKEX_STA_OFF_SS_BITS / 8, ss, ADKEX_SSBITS / 8);
    memcpy(sta + ADKEX_STA_OFF_T_BITS  / 8, T,  ADKEX_SSBITS / 8);

    memset(T,  0, sizeof T);
    memset(ss, 0, sizeof ss);
    return 0;
}

void ADKEX_derive_ss_a(
    uint8_t ss[ADKEX_SSBITS / 8],
    const uint8_t sta[ADKEX_STA_MAX_BITS / 8])
{
    // st_A = (ss, T); ss = KDF(st_A; N)
    ADKEX_KDF(ADKEX_KDF_OUTBITS, sta, ADKEX_KDF_INBITS, ss);
}

int ADKEX_derive_ss_b(
    uint8_t ss[ADKEX_SSBITS / 8],
    const uint8_t ma[ADKEX_M3_BITS / 8],
    const uint8_t stb[ADKEX_STB_MAX_BITS / 8],
    const uint8_t pk_A[ADKEX_SIGPKBITS / 8])
{
    const uint8_t *sigma_A = ma;
    const uint8_t *T       = stb + ADKEX_STB_OFF_T_BITS / 8;

    if (!adkex_sig_verify(pk_A, sigma_A, T, ADKEX_SSBITS)) return -1;

    // st_B = (ss, T); ss = KDF(st_B; N)
    ADKEX_KDF(ADKEX_KDF_OUTBITS, stb, ADKEX_KDF_INBITS, ss);
    return 0;
}
