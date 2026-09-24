#ifndef POLY_H
#define POLY_H

#include <stdint.h>
#include "ntt.h"
#include "radix16_r2.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /// @brief Check whether a polynomial is invertible in Zq using its NTT-domain representation
    /// @param[in] a Base address of input polynomial coefficient array in NTT domain
    /// @return 1 if the polynomial is not invertible in Zq, otherwise 0
    int check_poly_inv_Zq(int16_t *a);

    /// @brief Check whether a polynomial is invertible in Z2
    /// @param[in] a Base address of input polynomial coefficient array
    /// @return 1 if the polynomial is not invertible in Z2, otherwise 0
    int check_poly_inv_Z2(int16_t *a);

    /// @brief Assembly fused fold/check: emit radix16 x^(N/4)+1 fold and return Z2 non-invertibility
    /// @param[out] r_rad Base address of output radix16 word array of length R2_RADIX16_WORDS(ZEN_N4)
    /// @param[in] a Base address of input polynomial coefficient array of length ZEN_N
    /// @return 1 if the folded polynomial is not invertible in Z2, otherwise 0
    int poly_xor4_radix16(uint32_t *r_rad, const int16_t *a);

    /// @brief Compute the inverse of a binary polynomial already packed in radix16 R2 representation
    /// @param[out] f_inv Base address of output radix16 word array of length R2_RADIX16_WORDS(ZEN_N4)
    /// @param[in] f_rad Base address of input radix16 word array of length R2_RADIX16_WORDS(ZEN_N4)
    /// @return None
    void FastInversion(uint32_t *f_inv, const uint32_t *f_rad);

    /// @brief Generate a polynomial by summing a CBD-1 polynomial and a ternary polynomial from an input seed and nonce
    /// @param[out] a Base address of output polynomial coefficient array
    /// @param[in] seed Base address of input seed byte array
    /// @param[in] nonce Single-byte nonce used to diversify the pseudoXOF input
    /// @return None
    void poly_generate_g(int16_t *a, const uint8_t *seed, uint8_t nonce);

    /// @brief Generate a polynomial whose coefficients follow a centered binomial distribution with parameter eta = 1 from an input seed and nonce
    /// @param[out] a Base address of output polynomial coefficient array
    /// @param[in] seed Base address of input seed byte array
    /// @param[in] nonce Single-byte nonce used to diversify the pseudoXOF input
    /// @return None
    void poly_generate_f(int16_t *a, const uint8_t *seed, uint8_t nonce);

    /// @brief Generate a polynomial by summing a CBD-1 polynomial and a ternary polynomial from an input seed and nonce
    /// @param[out] a Base address of output polynomial coefficient array
    /// @param[in] seed Base address of input seed byte array
    /// @param[in] nonce Single-byte nonce used to diversify the pseudoXOF input
    /// @return None
    void poly_generate_s(int16_t *a, const uint8_t *seed, uint8_t nonce);
    
    /// @brief Generate a polynomial whose coefficients follow a centered binomial distribution with parameter eta = 2 from an input seed and nonce
    /// @param[out] a Base address of output polynomial coefficient array
    /// @param[in] seed Base address of input seed byte array
    /// @param[in] nonce Single-byte nonce used to diversify the pseudoXOF input
    /// @return None
    void poly_generate_e(int16_t *a, const uint8_t *seed, uint8_t nonce);

    /// @brief Pack a binary polynomial coefficient array into a byte array
    /// @param[out] pa Base address of output packed byte array
    /// @param[in] a Base address of input polynomial coefficient array whose entries are 0 or 1
    /// @param[in] n Total number of coefficients in the input polynomial coefficient array
    /// @return None
    void poly_bit2byte_pack(uint8_t *pa, const int16_t *a, const unsigned int n);

    /// @brief Unpack a byte array into a binary polynomial coefficient array
    /// @param[out] a Base address of output polynomial coefficient array whose entries are 0 or 1
    /// @param[in] pa Base address of input packed byte array
    /// @param[in] n Total number of coefficients in the output polynomial coefficient array
    /// @return None
    void poly_byte2bit_unpack(int16_t *a, const uint8_t *pa, const unsigned int n);

    /// @brief Pack a polynomial into a byte array for secret key
    /// @param[out] ss Base address of output byte array
    /// @param[in] a Base address of input polynomial coefficient array
    /// @return None
    void poly_secretkey_pack(uint8_t *ss, const int16_t *a);

    /// @brief Unpack a serialized secret key byte array into a polynomial coefficient array 
    /// @param[out] a Base address of output polynomial coefficient array 
    /// @param[in] pa Base address of input packed byte array 
    /// @return None
    void poly_secretkey_unpack(int16_t *a, const uint8_t *ss);

    /// @brief Pack a polynomial into a byte array for public key serialization 
    /// @param[out] pa Base address of output packed byte array 
    /// @param[in] a Base address of input polynomial coefficient array 
    /// @return None
    void poly_publickey_pack(uint8_t *pa, const int16_t *a);

    /// @brief Unpack a serialized public key byte array into a polynomial coefficient array 
    /// @param[out] a Base address of output polynomial coefficient array 
    /// @param[in] pa Base address of input packed byte array 
    /// @return None
    void poly_publickey_unpack(int16_t *a, const uint8_t *pa);

    /// @brief Pack a polynomial with coefficients uniformly distributed over Z256 into a byte array
    /// @param[out] pa Base address of output packed byte array of length ZEN_INDCPA_CIPHERTEXT_LEN_BYTES
    /// @param[in] a Base address of input polynomial coefficient array of length ZEN_N
    /// @return None
    void poly_ciphertext_pack(uint8_t *pa, const int16_t *a);

    /// @brief Unpack a byte array into a polynomial with coefficients in Z256; inverse of poly_ciphertext_pack
    /// @param[out] a Base address of output polynomial coefficient array of length ZEN_N
    /// @param[in] pa Base address of input packed byte array of length ZEN_INDCPA_CIPHERTEXT_LEN_BYTES
    /// @return None
    void poly_ciphertext_unpack(int16_t *a, const uint8_t *pa);

    /// @brief Compress polynomial coefficients from Z769 to 8-bit values
    /// @param[in,out] a Base address of input polynomial coefficient array of length ZEN_N; each coefficient is replaced by its compressed 8-bit representation
    /// @return None
    void poly_compress(int16_t *a);

    /// @brief Decompress 8-bit polynomial coefficients to approximate values in Z769
    /// @param[in,out] a Base address of input polynomial coefficient array of length ZEN_N; each compressed 8-bit coefficient is replaced by its decompressed value in Z769
    /// @return None
    void poly_decompress(int16_t *a);



#define poly_ntt(a) mq_poly_ntt(a)
#define poly_ntt_mq(a) mq_poly_ntt_mq(a)
#define poly_intt(a) mq_poly_intt(a)
#define poly_basemul_ntt(r, a, b) mq_poly_pointwise_mul(r, a, b)
#define poly_basemul_ntt_mq(r, a, b) mq_poly_pointwise_mul_mq(r, a, b)
#define poly_baseinv_ntt(r, a) mq_poly_inv_ntt(r, a)
#define check_poly_inv_Z2(a) check_poly_inv_Z2_asm(a)
#define check_poly_inv_Zq(a) check_poly_inv_Zq_asm(a)

#ifdef __cplusplus
}
#endif
#endif