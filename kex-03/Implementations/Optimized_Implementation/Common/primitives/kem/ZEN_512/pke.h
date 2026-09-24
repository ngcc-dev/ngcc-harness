/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-512 instance.
*/
#ifndef PKE_H
#define PKE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif
    /// @brief Deterministically Generate a public key and a secret key for the PKE scheme from an input seed
    /// @param[out] pk Base address of output public key byte array
    /// @param[out] sk Base address of output secret key byte array
    /// @param[in] seed Base address of input seed byte array used for deterministic key generation
    /// @return None
    void pke_keygen_derand(unsigned char *pk, unsigned char *sk, const unsigned char *seed);

    /// @brief Generate a public key and a secret key for the PKE scheme
    /// @param[out] pk Base address of output public key byte array
    /// @param[out] sk Base address of output secret key byte array
    /// @return None
    void pke_keygen(unsigned char *pk, unsigned char *sk);

    /// @brief Encrypt a message using the input public key and randomness seed
    /// @param[in] pk Base address of input public key byte array
    /// @param[in] m Base address of input message byte array
    /// @param[in] seed Base address of input randomness seed byte array
    /// @param[out] ct Base address of output ciphertext byte array
    /// @return None
    void pke_enc(unsigned char *pk, unsigned char *m, unsigned char *seed, unsigned char *ct);

    /// @brief Decrypt a ciphertext using the input secret key and recover the message
    /// @param[in] sk Base address of input secret key byte array
    /// @param[in] ct Base address of input ciphertext byte array
    /// @param[out] m Base address of output message byte array
    /// @return None
    void pke_dec(unsigned char *sk, unsigned char *ct, unsigned char *m);

#ifdef __cplusplus
}
#endif
#endif