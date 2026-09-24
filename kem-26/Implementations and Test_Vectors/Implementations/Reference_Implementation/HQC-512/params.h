#ifndef NSS_HQC_PARAMS_H
#define NSS_HQC_PARAMS_H

/* Provisional theory-aligned parameters extracted from ??????/??????/sections/06_params.tex. */
#define NSS_HQC_INSTANCE_NAME "HQC-512"

#define NSS_HQC_N 217901u
#define NSS_HQC_N2 128u
#define NSS_HQC_W_SK 233u
#define NSS_HQC_W_R1 213u
#define NSS_HQC_W_R2 213u
#define NSS_HQC_W_E 372u
#define NSS_HQC_N1 130u
#define NSS_HQC_K1 64u
#define NSS_HQC_MULT 5u
#define NSS_HQC_Q 4u
#define NSS_HQC_B 2u
#define NSS_HQC_TAU_NUM 25u
#define NSS_HQC_TAU_DEN 100u
#define NSS_HQC_DELTA1 67u
#define NSS_HQC_KAPPA_BITS 512u
#define NSS_HQC_SALT_BITS 512u
#define NSS_HQC_SEED_BITS 512u
#define NSS_HQC_NC 83200u
#define NSS_HQC_CT_V_BITS 33280u

#define NSS_HQC_N_BYTES 27238u
#define NSS_HQC_NC_BYTES 10400u
#define NSS_HQC_CT_V_BYTES 4160u
#define NSS_HQC_SEED_PKE_BYTES 64u
#define NSS_HQC_SEED_SK_BYTES 64u
#define NSS_HQC_SALT_BYTES 64u
#define NSS_HQC_SIGMA_BYTES 64u
#define NSS_HQC_SS_BYTES 64u
#define NSS_HQC_PK_BYTES 27302u
#define NSS_HQC_SK_BYTES 27430u
#define NSS_HQC_CT_PKE_BYTES 31398u
#define NSS_HQC_CT_KEM_BYTES 31462u

/* Compatibility alias: old core code uses CT_BYTES for the PKE ciphertext u || vhat. */
#define NSS_HQC_CT_BYTES NSS_HQC_CT_PKE_BYTES

#endif
