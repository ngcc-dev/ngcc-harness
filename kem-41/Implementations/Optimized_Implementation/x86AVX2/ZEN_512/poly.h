/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-512 instance.
*/
#ifndef POLY_H
#define POLY_H

#include <stdint.h>
#include <immintrin.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /// @brief Multiply two AVX2 vector registers using Montgomery reduction modulo ZEN_Q
    /// @param[out] tmp_c Base address of output AVX2 vector register
    /// @param[in] tmp_a Base address of first input AVX2 vector register
    /// @param[in] tmp_b Base address of second input AVX2 vector register
    /// @param[in] tmp_Q Base address of AVX2 vector register containing ZEN_Q
    /// @param[in] tmp_QINV Base address of AVX2 vector register containing QINV
    /// @return None
    void montmul(__m256i *tmp_c, __m256i *tmp_a, __m256i *tmp_b, __m256i *tmp_Q, __m256i *tmp_QINV);

    /// @brief Perform block-wise polynomial multiplication in the NTT domain using AVX2
    /// @param[out] r Base address of output polynomial coefficient array
    /// @param[in] a Base address of first input polynomial coefficient array in NTT domain
    /// @param[in] b Base address of second input polynomial coefficient array in NTT domain
    /// @param[in] muldata Base address of precomputed multiplication constants and twiddle-factor table
    /// @return None
    void poly_basemul_ntt(int16_t *r, int16_t *a, int16_t *b, const int16_t *muldata);

    /// @brief Perform block-wise polynomial multiplication in the NTT domain using AVX2 and map coefficients into the standard range modulo ZEN_Q
    /// @param[out] r Base address of output polynomial coefficient array
    /// @param[in] a Base address of first input polynomial coefficient array in NTT domain
    /// @param[in] b Base address of second input polynomial coefficient array in NTT domain
    /// @param[in] muldata Base address of precomputed multiplication constants and twiddle-factor table
    /// @return None
    void poly_basemul_ntt_mq(int16_t *r, int16_t *a, int16_t *b, const int16_t *muldata);

    /// @brief Perform block-wise inversion of a polynomial in the NTT domain using AVX2
    /// @param[out] r Base address of output inverse polynomial coefficient array in NTT domain
    /// @param[in] a Base address of input polynomial coefficient array in NTT domain
    /// @return None
    void poly_baseinv_ntt(int16_t *r, int16_t *a);

    /// @brief Check whether a polynomial is invertible in Zq using its NTT-domain representation
    /// @param[in] a Base address of input polynomial coefficient array in NTT domain
    /// @return 1 if the polynomial is not invertible in Zq, otherwise 0
    int check_poly_inv_Zq(int16_t *a);

    /// @brief Check whether a polynomial is invertible in Z2
    /// @param[in] a Base address of input polynomial coefficient array
    /// @return 1 if the polynomial is not invertible in Z2, otherwise 0
    int check_poly_inv_Z2(int16_t *a);

    /// @brief Multiply two binary polynomials in R2 of degree less than 1024 using a constant-time cyclic shift-and-XOR method
    /// @param[in] a Base address of first input polynomial coefficient array of length 256
    /// @param[in] b Base address of second input polynomial coefficient array of length 256
    /// @param[out] res Base address of output polynomial coefficient array of length 256
    /// @return None
    void mul_in_R2_1024(int16_t *a, int16_t *b, int16_t *res);

    /// @brief Compute the inverse of a polynomial in R2 using a fast iterative inversion algorithm
    /// @param[in] f Base address of input polynomial coefficient array of length ZEN_N2
    /// @param[out] f_inv Base address of output polynomial inverse coefficient array of length ZEN_N2
    /// @return None
    void FastInversion(int16_t *f_inv, int16_t *f);
    
    /// @brief Generate a ternary polynomial from an input seed and nonce
    /// @param[out] a Base address of output polynomial coefficient array
    /// @param[in] seed Base address of input seed byte array
    /// @param[in] nonce Single-byte nonce used to diversify the pseudoXOF input
    /// @return None
    void poly_generate_gf(int16_t *a, const uint8_t *seed, uint8_t nonce);

    /// @brief Generate a ternary polynomial from an input seed and nonce
    /// @param[out] a Base address of output polynomial coefficient array
    /// @param[in] seed Base address of input seed byte array
    /// @param[in] nonce Single-byte nonce used to diversify the pseudoXOF input
    /// @return None
    void poly_generate_se(int16_t *a, const uint8_t *seed, uint8_t nonce);

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



#ifdef __cplusplus
}
#endif
#endif