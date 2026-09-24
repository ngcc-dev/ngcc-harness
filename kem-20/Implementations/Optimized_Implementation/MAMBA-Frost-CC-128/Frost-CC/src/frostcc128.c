/********************************************************************************************
* MAMBA-Frost-CC: unstructured LWQ-Z key encapsulation mechanism.
*
* Abstract: functions for MAMBA-Frost-CC-128.
*           Instantiates "frost_macrify.c" with the necessary matrix arithmetic functions.
*
*********************************************************************************************/

#include "api_frostcc128.h"
#include "frost_macrify.h"


// Parameters for "Frost-CC-128"
#define PARAMS_N 512
#define PARAMS_NBAR 8
#define PARAMS_NBAR_R 8
#define PARAMS_NBAR_S 8
#define PARAMS_LOGQ 15
#define PARAMS_Q (1 << PARAMS_LOGQ)
#define PARAMS_EXTRACTED_BITS 2
#define PARAMS_STRIPE_STEP 8
#define PARAMS_PARALLEL 4
#define BYTES_SEED_A 32
#define BYTES_MU (PARAMS_EXTRACTED_BITS*PARAMS_NBAR_R*PARAMS_NBAR_S)/8
#define BYTES_SALT 32
#define BYTES_SEED_SE (2*CRYPTO_BYTES)
#define BYTES_PKHASH 32
#define PARAMS_PK_LOGP 10
#define PARAMS_U_LOGP  10
#define PARAMS_ETA_S 2
#define PARAMS_ETA_R 2
#define PARAMS_ETA PARAMS_ETA_S
#define PARAMS_V_LOGP  5

#if (PARAMS_NBAR_R != 8 || PARAMS_NBAR_R % 8 != 0)
#error MAMBA-Frost E8 profiles require PARAMS_NBAR_R to be 8.
#endif
#if (PARAMS_NBAR_S % 4 != 0)
#error MAMBA-Frost requires PARAMS_NBAR_S to be compatible with the vectorized 4-row path.
#endif

// Selecting SHAKE XOF function for the KEM and noise sampling
#define shake     shake128

#define crypto_kem_keypair            crypto_kem_keypair_FrostCC128
#define crypto_kem_enc                crypto_kem_enc_FrostCC128
#define crypto_kem_dec                crypto_kem_dec_FrostCC128

#include "kem.c"
#include "noise.c"
#if defined(USE_REFERENCE)
#include "frost_macrify_reference.c"
#else
#include "frost_macrify.c"
#endif
