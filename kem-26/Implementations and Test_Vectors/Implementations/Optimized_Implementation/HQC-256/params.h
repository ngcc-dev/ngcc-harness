#ifndef NSS_HQC_PARAMS_H
#define NSS_HQC_PARAMS_H

/* Provisional theory-aligned parameters extracted from ??????/??????/sections/06_params.tex. */
#define NSS_HQC_INSTANCE_NAME "HQC-256"

#define NSS_HQC_N 54493u
#define NSS_HQC_N2 128u
#define NSS_HQC_W_SK 117u
#define NSS_HQC_W_R1 106u
#define NSS_HQC_W_R2 106u
#define NSS_HQC_W_E 187u
#define NSS_HQC_N1 70u
#define NSS_HQC_K1 32u
#define NSS_HQC_MULT 5u
#define NSS_HQC_Q 4u
#define NSS_HQC_B 2u
#define NSS_HQC_TAU_NUM 28u
#define NSS_HQC_TAU_DEN 100u
#define NSS_HQC_DELTA1 39u
#define NSS_HQC_KAPPA_BITS 256u
#define NSS_HQC_SALT_BITS 256u
#define NSS_HQC_SEED_BITS 256u
#define NSS_HQC_NC 44800u
#define NSS_HQC_CT_V_BITS 17920u

#define NSS_HQC_N_BYTES 6812u
#define NSS_HQC_NC_BYTES 5600u
#define NSS_HQC_CT_V_BYTES 2240u
#define NSS_HQC_SEED_PKE_BYTES 32u
#define NSS_HQC_SEED_SK_BYTES 32u
#define NSS_HQC_SALT_BYTES 32u
#define NSS_HQC_SIGMA_BYTES 32u
#define NSS_HQC_SS_BYTES 32u
#define NSS_HQC_PK_BYTES 6844u
#define NSS_HQC_SK_BYTES 6908u
#define NSS_HQC_CT_PKE_BYTES 9052u
#define NSS_HQC_CT_KEM_BYTES 9084u

/* Compatibility alias: old core code uses CT_BYTES for the PKE ciphertext u || vhat. */
#define NSS_HQC_CT_BYTES NSS_HQC_CT_PKE_BYTES

#endif
