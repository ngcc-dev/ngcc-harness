#ifndef NSS_HQC_PARAMS_H
#define NSS_HQC_PARAMS_H

/* Provisional theory-aligned parameters extracted from ??????/??????/sections/06_params.tex. */
#define NSS_HQC_INSTANCE_NAME "HQC-128"

#define NSS_HQC_N 29443u
#define NSS_HQC_N2 128u
#define NSS_HQC_W_SK 73u
#define NSS_HQC_W_R1 66u
#define NSS_HQC_W_R2 66u
#define NSS_HQC_W_E 117u
#define NSS_HQC_N1 46u
#define NSS_HQC_K1 16u
#define NSS_HQC_MULT 5u
#define NSS_HQC_Q 4u
#define NSS_HQC_B 2u
#define NSS_HQC_TAU_NUM 30u
#define NSS_HQC_TAU_DEN 100u
#define NSS_HQC_DELTA1 31u
#define NSS_HQC_KAPPA_BITS 256u
#define NSS_HQC_SALT_BITS 256u
#define NSS_HQC_SEED_BITS 256u
#define NSS_HQC_NC 29440u
#define NSS_HQC_CT_V_BITS 11776u

#define NSS_HQC_N_BYTES 3681u
#define NSS_HQC_NC_BYTES 3680u
#define NSS_HQC_CT_V_BYTES 1472u
#define NSS_HQC_SEED_PKE_BYTES 32u
#define NSS_HQC_SEED_SK_BYTES 32u
#define NSS_HQC_SALT_BYTES 32u
#define NSS_HQC_SIGMA_BYTES 32u
#define NSS_HQC_SS_BYTES 32u
#define NSS_HQC_PK_BYTES 3713u
#define NSS_HQC_SK_BYTES 3777u
#define NSS_HQC_CT_PKE_BYTES 5153u
#define NSS_HQC_CT_KEM_BYTES 5185u

/* Compatibility alias: old core code uses CT_BYTES for the PKE ciphertext u || vhat. */
#define NSS_HQC_CT_BYTES NSS_HQC_CT_PKE_BYTES

#endif
