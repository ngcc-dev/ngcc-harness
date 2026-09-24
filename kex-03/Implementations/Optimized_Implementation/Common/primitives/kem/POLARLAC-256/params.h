/*
Copyright (c) 2026 Ying Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Defines parameter constants for the optimized POLARLAC-256 implementation.
*/

#ifndef PARAM_H
#define PARAM_H

#define KEM_SEED_LEN_BYTES 64
#define PK_SEED_LEN_BYTES 32
#define PKE_EXPANDED_SEED_BYTES (PK_SEED_LEN_BYTES + KEM_SEED_LEN_BYTES)
#define RL_KEM_N 512
#define RL_KEM_N_LEN_BYTES 64
#define RL_KEM_K 2
#define RL_KEM_Q 257
#define RL_KEM_T 1296
#define RL_KEM_N_Half 256
#define MESSAGE_LEN_BYTES 32
#define MLWE_VEC_COEFFS (RL_KEM_K * RL_KEM_N)
#define SK_LEN_BYTES (MLWE_VEC_COEFFS * 2) // raw NTT-domain secret vector, int16_t per coefficient
#define CODE_LEN 64
#define RL_KEM_Lv 512
#define RATIO 129
#define PK_ZERO_SELECTOR_BYTES 2
#define PK_ZERO_SELECTOR_BITS (PK_ZERO_SELECTOR_BYTES * 8)
#define PK_POLY_BYTES (RL_KEM_N + PK_ZERO_SELECTOR_BYTES)
#define PK_LEN_BYTES (RL_KEM_K * PK_POLY_BYTES)
#define C1_POLY_BYTES RL_KEM_N
#define C1_LEN_BYTES MLWE_VEC_COEFFS
#define D_C2_BITS 4 // number of quantization bits retained per c2 coefficient
#define C2_LEN_BYTES ((RL_KEM_Lv * D_C2_BITS) / 8)
#define SS_KEY_BYTES MESSAGE_LEN_BYTES


#ifndef BIT_USE_SHAKE
#define BIT_USE_SHAKE 0
#endif
#define QINV -255
#define SK_NTT_LEN_BYTES     SK_LEN_BYTES
#define PKE_PUBLIC_KEY_BYTES  (PK_SEED_LEN_BYTES + PK_LEN_BYTES)
#define PKE_SECRET_KEY_BYTES  SK_NTT_LEN_BYTES
#define PKE_CIPHERTEXT_BYTES  (C1_LEN_BYTES + C2_LEN_BYTES)
#define PKE_MESSAGE_BYTES     MESSAGE_LEN_BYTES
#define KEM_REJECT_SEED_BYTES 32
#define KEM_SK_BYTES (PKE_SECRET_KEY_BYTES + PKE_PUBLIC_KEY_BYTES + KEM_REJECT_SEED_BYTES)
#define KEM_SS_BYTES SS_KEY_BYTES
#define SM3_DIGEST_BYTES 32

#endif
