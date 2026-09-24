#ifndef NSS_HQC_PARAMS_H
#define NSS_HQC_PARAMS_H

/* Provisional theory-aligned parameters extracted from ??????/??????/sections/06_params.tex. */
#define NSS_HQC_INSTANCE_NAME "HQC-384"

#define NSS_HQC_N 122579u
#define NSS_HQC_N2 128u
#define NSS_HQC_W_SK 175u
#define NSS_HQC_W_R1 159u
#define NSS_HQC_W_R2 159u
#define NSS_HQC_W_E 279u
#define NSS_HQC_N1 100u
#define NSS_HQC_K1 48u
#define NSS_HQC_MULT 5u
#define NSS_HQC_Q 4u
#define NSS_HQC_B 2u
#define NSS_HQC_TAU_NUM 26u
#define NSS_HQC_TAU_DEN 100u
#define NSS_HQC_DELTA1 53u
#define NSS_HQC_KAPPA_BITS 384u
#define NSS_HQC_SALT_BITS 384u
#define NSS_HQC_SEED_BITS 384u
#define NSS_HQC_NC 64000u
#define NSS_HQC_CT_V_BITS 25600u

#define NSS_HQC_N_BYTES 15323u
#define NSS_HQC_NC_BYTES 8000u
#define NSS_HQC_CT_V_BYTES 3200u
#define NSS_HQC_SEED_PKE_BYTES 48u
#define NSS_HQC_SEED_SK_BYTES 48u
#define NSS_HQC_SALT_BYTES 48u
#define NSS_HQC_SIGMA_BYTES 48u
#define NSS_HQC_SS_BYTES 48u
#define NSS_HQC_PK_BYTES 15371u
#define NSS_HQC_SK_BYTES 15467u
#define NSS_HQC_CT_PKE_BYTES 18523u
#define NSS_HQC_CT_KEM_BYTES 18571u

/* Compatibility alias: old core code uses CT_BYTES for the PKE ciphertext u || vhat. */
#define NSS_HQC_CT_BYTES NSS_HQC_CT_PKE_BYTES

#endif
