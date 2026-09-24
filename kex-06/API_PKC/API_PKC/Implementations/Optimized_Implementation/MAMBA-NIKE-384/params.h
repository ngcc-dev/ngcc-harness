#ifndef PARAMS_H
#define PARAMS_H

#define NIKE_LEVEL 384
#define NIKE_SECURITY_BITS 384
#define PARAM_N 2048
#define PARAM_K 2
#define PARAM_Q 8192
#define LOG2Q 13
#define PARAM_T_PK 11
#define PARAM_T_U 11
#define PARAM_T_V 6

#define PARAM_H_PK 2
#define PARAM_H_U  2
#define PARAM_H_V  7

#if PARAM_Q != (1u << LOG2Q)
#error "PARAM_Q must equal 2^LOG2Q"
#endif
#if PARAM_T_PK != LOG2Q - PARAM_H_PK
#error "PARAM_T_PK/Delta_pk mismatch"
#endif
#if PARAM_T_U != LOG2Q - PARAM_H_U
#error "PARAM_T_U/Delta_u mismatch"
#endif
#if PARAM_K < 1 || PARAM_K > 16
#error "MAMBA-NIKE CBD sampler supports eta values in [1,16]"
#endif

#define PARAM_P_PK     (1u << PARAM_T_PK)
#define PARAM_P_U      (1u << PARAM_T_U)
#define PARAM_P_V      (1u << PARAM_T_V)
#define PARAM_DELTA_PK (1u << PARAM_H_PK)
#define PARAM_DELTA_U  (1u << PARAM_H_U)
#define PARAM_DELTA_V  (1u << PARAM_H_V)

#define POLY_BYTES (2 * PARAM_N)
#define NIKE_SEEDBYTES 32
#define NIKE_RECGROUPS (PARAM_N / 4)
#define NIKE_RECCOEFFS (4 * NIKE_RECGROUPS)
#define NIKE_RECBYTES NIKE_RECGROUPS
#define NIKE_KEYBYTES (PARAM_N / 32)
#define NIKE_PACKEDPOLYBYTES(bits) (((PARAM_N * (bits)) + 7) / 8)
#define NIKE_PKPOLYBYTES NIKE_PACKEDPOLYBYTES(PARAM_T_PK)
#define NIKE_UPOLYBYTES  NIKE_PACKEDPOLYBYTES(PARAM_T_U)

#define NIKE_SSBYTES 48
#if NIKE_SSBYTES > NIKE_KEYBYTES
#error "NIKE_SSBYTES exceeds raw reconciliation key bytes"
#endif

#define NIKE_SENDABYTES (NIKE_PKPOLYBYTES + NIKE_SEEDBYTES)
#define NIKE_SENDBBYTES (NIKE_SEEDBYTES + NIKE_UPOLYBYTES + NIKE_RECBYTES)
#define NIKE_SKBYTES (POLY_BYTES + NIKE_SENDABYTES)
#define NIKE_KAT_SUFFIX (NIKE_SENDABYTES + NIKE_SENDBBYTES)
#define NIKE_KDF_DOMAINBYTES 16
#define NIKE_KDF_INPUTBYTES (NIKE_KDF_DOMAINBYTES + NIKE_KEYBYTES + NIKE_SENDABYTES + NIKE_SENDBBYTES)

#if NIKE_SENDABYTES != 2848
#error "manifest pk length mismatch"
#endif
#if NIKE_SKBYTES != 6944
#error "manifest sk_api length mismatch"
#endif
#if NIKE_SENDBBYTES != 3360
#error "manifest M1 length mismatch"
#endif
#if NIKE_KAT_SUFFIX != 6208
#error "manifest KAT suffix mismatch"
#endif

#endif
