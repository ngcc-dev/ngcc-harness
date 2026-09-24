/*
Copyright (c) 2026 Ziyao Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares polar encoding and decoding helpers for the optimized POLARLAC-256 instance.
*/

#ifndef POLAR_H
#define POLAR_H

#include <stdint.h>

/// @brief Runtime description of the polar encoder/decoder configuration.
/// @details The global instance is defined in `polar.c` and records the code
///          length, dimension and auxiliary storage size used by the current
///          reference implementation.
struct polar_control {
	unsigned int    N;
	unsigned int    n;
	unsigned int    K;
	unsigned int    ecc_bytes;
};

extern struct polar_control polar;
extern uint8_t info_nodes[];

/// @brief Encode a binary source vector in place using the configured polar code.
/// @param[in,out] u Base address of the one-bit-per-element source/codeword array.
/// @details The buffer must contain `polar.N` binary elements on entry. After
///          return, it stores the encoded codeword in the same format.
void encode_polar_opt(uint64_t *u_64);

/// @brief Decode a polar codeword from log-likelihood ratios.
/// @param[out] m_cap Base address of the recovered information-bit array.
/// @param[in] llr Base address of the input LLR array of length `polar.N`.
/// @details The output buffer stores one decoded information bit per element.
void decode_polar(uint8_t *m_cap, const int64_t *llr);

#endif
