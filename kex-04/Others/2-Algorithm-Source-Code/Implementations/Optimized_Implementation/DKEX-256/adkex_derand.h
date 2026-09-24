#ifndef ADKEX_DERAND_H
#define ADKEX_DERAND_H

#include <stdint.h>
#include "ADKEX_parameters.h"

#ifdef __cplusplus
extern "C"
{
#endif

/// @brief Derandomized SIG key pair generation for ADKEX-128 initiator
/// @param[out] pk_A  initiator verification key
/// @param[out] sk_A  initiator signing key
/// @param[in]  coins random coins
void ADKEX_init_a_derand(
    uint8_t pk_A[ADKEX_SIGPKBITS / 8],
    uint8_t sk_A[ADKEX_SIGSKBITS / 8],
    const uint8_t coins[ADKEX_INIT_A_COINBITS / 8]);

/// @brief Derandomized SIG key pair generation for ADKEX-128 responder
/// @param[out] pk_B  responder verification key
/// @param[out] sk_B  responder signing key
/// @param[in]  coins random coins
void ADKEX_init_b_derand(
    uint8_t pk_B[ADKEX_SIGPKBITS / 8],
    uint8_t sk_B[ADKEX_SIGSKBITS / 8],
    const uint8_t coins[ADKEX_INIT_B_COINBITS / 8]);

/// @brief Derandomized pass-1 (A -> B) for ADKEX-128
/// @param[out] m1    pass-1 message m_1 = M_1
/// @param[out] sta   initiator state sk_e || M_1 || pk_A
/// @param[in]  pk_A  initiator verification key
/// @param[in]  coins random coins for DKEX.Initiate
void ADKEX_pass1_msg_a_derand(
    uint8_t m1[ADKEX_M1_BITS / 8],
    uint8_t sta[ADKEX_STA_MAX_BITS / 8],
    const uint8_t pk_A[ADKEX_SIGPKBITS / 8],
    const uint8_t coins[ADKEX_PASS1_COINBITS / 8]);

/// @brief Derandomized pass-2 (B -> A) for ADKEX-128
/// @param[out] m2    pass-2 message M_2 || sigma_B
/// @param[out] stb   responder state ss_raw || T_A
/// @param[in]  m1    pass-1 message
/// @param[in]  pk_A  initiator verification key
/// @param[in]  pk_B  responder verification key
/// @param[in]  sk_B  responder signing key
/// @param[in]  coins random coins for DKEX.Response
void ADKEX_pass2_msg_b_derand(
    uint8_t m2[ADKEX_M2_BITS / 8],
    uint8_t stb[ADKEX_STB_MAX_BITS / 8],
    const uint8_t m1[ADKEX_M1_BITS / 8],
    const uint8_t pk_A[ADKEX_SIGPKBITS / 8],
    const uint8_t pk_B[ADKEX_SIGPKBITS / 8],
    const uint8_t sk_B[ADKEX_SIGSKBITS / 8],
    const uint8_t coins[ADKEX_PASS2_DKEX_COINBITS / 8]);

/// @brief Derandomized pass-3 (A -> B) for ADKEX-128
/// @param[out]    m3   pass-3 message m_3 = sigma_A
/// @param[in,out] sta  initiator state; on success replaced with ss_raw
/// @param[in]     m2   pass-2 message
/// @param[in]     pk_B responder verification key
/// @param[in]     sk_A initiator signing key
/// @return 0 on success, -1 if B's signature didn't verify
int ADKEX_pass3_msg_a_derand(
    uint8_t m3[ADKEX_M3_BITS / 8],
    uint8_t sta[ADKEX_STA_MAX_BITS / 8],
    const uint8_t m2[ADKEX_M2_BITS / 8],
    const uint8_t pk_B[ADKEX_SIGPKBITS / 8],
    const uint8_t sk_A[ADKEX_SIGSKBITS / 8]);

/// @brief ADKEX-128 final shared-key derivation by the initiator
/// @param[out] ss   shared secret
/// @param[in]  sta  initiator state (holds ss_raw)
void ADKEX_derive_ss_a(
    uint8_t ss[ADKEX_SSBITS / 8],
    const uint8_t sta[ADKEX_STA_MAX_BITS / 8]);

/// @brief ADKEX-128 final shared-key derivation by the responder
/// @param[out] ss   shared secret
/// @param[in]  ma   last initiator message m_3 = sigma_A
/// @param[in]  stb  responder state ss_raw || T_A
/// @param[in]  pk_A initiator verification key
/// @return 0 on success, -1 if A's signature didn't verify
int ADKEX_derive_ss_b(
    uint8_t ss[ADKEX_SSBITS / 8],
    const uint8_t ma[ADKEX_M3_BITS / 8],
    const uint8_t stb[ADKEX_STB_MAX_BITS / 8],
    const uint8_t pk_A[ADKEX_SIGPKBITS / 8]);

#ifdef __cplusplus
}
#endif
#endif /* ADKEX_DERAND_H */
