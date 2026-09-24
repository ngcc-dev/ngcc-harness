/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-128 instance.
*/
#ifndef POLY_H
#define POLY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /// @brief Check whether a polynomial satisfies the inversion-failure condition modulo ZEN_SWIFT_Q
    /// @param[in] a Base address of input polynomial coefficient array
    /// @return 1 if at least one checked coefficient block has zero sum, otherwise 0
    int check_poly_inv_Zq(int16_t *a);

    /// @brief Check whether a polynomial satisfies the inversion-failure condition modulo 2
    /// @param[in] a Base address of input polynomial coefficient array
    /// @return 1 if the XOR of the checked coefficients is zero, otherwise 0
    int check_poly_inv_Z2(int16_t *a);

    /// @brief Multiply two binary polynomials in R2 of degree 255
    /// @param[in] a Base address of first input binary polynomial coefficient array
    /// @param[in] b Base address of second input binary polynomial coefficient array
    /// @param[out] res Base address of output binary polynomial coefficient array
    /// @return None
    void mul_in_R2_256(int16_t *a, int16_t *b, int16_t *res);

    /// @brief Compute the inverse of a binary polynomial using the fast inversion algorithm
    /// @param[out] f_inv Base address of output inverse polynomial coefficient array
    /// @param[in] f Base address of input binary polynomial coefficient array
    /// @return None
    void FastInversion(int16_t *f_inv, int16_t *f);

    /// @brief Generate a polynomial g from a seed and nonce
    /// @param[out] a Base address of output polynomial coefficient array
    /// @param[in] seed Base address of input seed byte array
    /// @param[in] nonce Domain-separation nonce used for pseudorandom generation
    /// @return None
    void poly_generate_g(int16_t *a, const uint8_t *seed, uint8_t nonce);

    /// @brief Generate a polynomial f from a seed and nonce
    /// @param[out] a Base address of output polynomial coefficient array
    /// @param[in] seed Base address of input seed byte array
    /// @param[in] nonce Domain-separation nonce used for pseudorandom generation
    /// @return None
    void poly_generate_f(int16_t *a, const uint8_t *seed, uint8_t nonce);

    /// @brief Generate a polynomial s from a seed and nonce
    /// @param[out] a Base address of output polynomial coefficient array
    /// @param[in] seed Base address of input seed byte array
    /// @param[in] nonce Domain-separation nonce used for pseudorandom generation
    /// @return None
    void poly_generate_s(int16_t *a, const uint8_t *seed, uint8_t nonce);

    /// @brief Generate an error polynomial e from a seed and nonce
    /// @param[out] a Base address of output polynomial coefficient array
    /// @param[in] seed Base address of input seed byte array
    /// @param[in] nonce Domain-separation nonce used for pseudorandom generation
    /// @return None
    void poly_generate_e(int16_t *a, const uint8_t *seed, uint8_t nonce);

    /// @brief Pack a binary polynomial coefficient array into a byte array
    /// @param[out] pa Base address of output packed byte array
    /// @param[in] a Base address of input binary polynomial coefficient array
    /// @param[in] n Number of binary coefficients to be packed
    /// @return None
    void poly_bit2byte_pack(uint8_t *pa, const int16_t *a, const unsigned int n);

    /// @brief Unpack a byte array into a binary polynomial coefficient array
    /// @param[out] a Base address of output binary polynomial coefficient array
    /// @param[in] pa Base address of input packed byte array
    /// @param[in] n Number of binary coefficients to be unpacked
    /// @return None
    void poly_byte2bit_unpack(int16_t *a, const uint8_t *pa, const unsigned int n);

    /// @brief Pack a secret-key polynomial coefficient array into a byte array
    /// @param[out] ss Base address of output packed secret-key byte array
    /// @param[in] a Base address of input secret-key polynomial coefficient array
    /// @return None
    void poly_secretkey_pack(uint8_t *ss, const int16_t *a);

    /// @brief Unpack a byte array into a secret-key polynomial coefficient array
    /// @param[out] a Base address of output secret-key polynomial coefficient array
    /// @param[in] ss Base address of input packed secret-key byte array
    /// @return None
    void poly_secretkey_unpack(int16_t *a, const uint8_t *ss);

    /// @brief Pack a public-key polynomial coefficient array into a byte array
    /// @param[out] pa Base address of output packed public-key byte array
    /// @param[in] a Base address of input public-key polynomial coefficient array
    /// @return None
    void poly_publickey_pack(uint8_t *pa, const int16_t *a);

    /// @brief Unpack a byte array into a public-key polynomial coefficient array
    /// @param[out] a Base address of output public-key polynomial coefficient array
    /// @param[in] pa Base address of input packed public-key byte array
    /// @return None
    void poly_publickey_unpack(int16_t *a, const uint8_t *pa);

    /// @brief Pack a ciphertext polynomial coefficient array into a byte array
    /// @param[out] pa Base address of output packed ciphertext byte array
    /// @param[in] a Base address of input ciphertext polynomial coefficient array
    /// @return None
    void poly_ciphertext_pack(uint8_t *pa, const int16_t *a);

    /// @brief Unpack a byte array into a ciphertext polynomial coefficient array
    /// @param[out] a Base address of output ciphertext polynomial coefficient array
    /// @param[in] pa Base address of input packed ciphertext byte array
    /// @return None
    void poly_ciphertext_unpack(int16_t *a, const uint8_t *pa);

    /// @brief Compress polynomial coefficients from modulo ZEN_SWIFT_Q representation to 8-bit representation
    /// @param[in,out] a Base address of polynomial coefficient array; replaced by compressed coefficients
    /// @return None
    void poly_compress(int16_t *a);

    /// @brief Decompress polynomial coefficients from 8-bit representation to modulo ZEN_SWIFT_Q representation
    /// @param[in,out] a Base address of polynomial coefficient array; replaced by decompressed coefficients
    /// @return None
    void poly_decompress(int16_t *a);




#ifdef __cplusplus
}
#endif
#endif