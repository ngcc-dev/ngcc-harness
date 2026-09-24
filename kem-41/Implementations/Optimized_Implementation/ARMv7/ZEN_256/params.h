/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-256 instance.
*/
#ifndef PARAMS_H
#define PARAMS_H

#define SEED_LEN_BYTES 64

#define ZEN_N 1024
#define ZEN_N_LEN_BYTES ZEN_N/8
#define ZEN_N2 ZEN_N/2
#define ZEN_N4 ZEN_N/4
#define ZEN_N4_LOG2 8
#define ZEN_Q 769
#define ZEN_Q2 384

#define ZEN_SYM_LEN_BYTES 32 /* size in bytes of hashes, and seeds */
#define ZEN_SHAREDKEY_LEN_BYTES 32 /* size in bytes of shared key */
#define ZEN_F_NTT_PACK 1280
#define ZEN_F2_PACK 32

#define ZEN_INDCPA_MSG_LEN_BYTES 32
#define ZEN_INDCPA_PUBLICKEY_LEN_BYTES 1229
#define ZEN_INDCPA_SECREKEY_LEN_BYTES ZEN_F_NTT_PACK+ZEN_F2_PACK
#define ZEN_INDCPA_CIPHERTEXT_LEN_BYTES 1024

#define ZEN_PUBLICKEY_LEN_BYTES ZEN_INDCPA_PUBLICKEY_LEN_BYTES
#define ZEN_SECREKEY_LEN_BYTES ZEN_INDCPA_SECREKEY_LEN_BYTES+ZEN_INDCPA_PUBLICKEY_LEN_BYTES+ZEN_SYM_LEN_BYTES+ZEN_SHAREDKEY_LEN_BYTES
#define ZEN_CIPHERTEXT_LEN_BYTES ZEN_INDCPA_CIPHERTEXT_LEN_BYTES

#endif