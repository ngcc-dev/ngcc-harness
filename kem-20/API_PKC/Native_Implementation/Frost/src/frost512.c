/********************************************************************************************
* MAMBA-Frost: unstructured LWQ-Z key encapsulation mechanism.
*
* Abstract: functions for MAMBA-Frost-512.
*           Instantiates "frost_macrify.c" with the necessary matrix arithmetic functions.
*
*********************************************************************************************/

#include "api_frost512.h"
#include "frost_macrify.h"


// Parameters for "Frost-512"
#define PARAMS_N 2600
#define PARAMS_NBAR 8
#define PARAMS_NBAR_R 8
#define PARAMS_NBAR_S 16
#define PARAMS_LOGQ 16
#define PARAMS_Q (1 << PARAMS_LOGQ)
#define PARAMS_EXTRACTED_BITS 4
#define PARAMS_STRIPE_STEP 8
#define PARAMS_PARALLEL 4
#define BYTES_SEED_A 32
#define BYTES_MU (PARAMS_EXTRACTED_BITS*PARAMS_NBAR_R*PARAMS_NBAR_S)/8
#define BYTES_SALT 32
#define BYTES_SEED_SE (2*CRYPTO_BYTES)
#define BYTES_PKHASH 32
#define PARAMS_PK_LOGP 14
#define PARAMS_U_LOGP  14
#define PARAMS_ETA_S 1
#define PARAMS_ETA_R 1
#define PARAMS_ETA PARAMS_ETA_S
#define PARAMS_V_LOGP  7

#if (PARAMS_NBAR_R != 8 || PARAMS_NBAR_R % 8 != 0)
#error MAMBA-Frost E8 profiles require PARAMS_NBAR_R to be 8.
#endif
#if (PARAMS_NBAR_S % 4 != 0)
#error MAMBA-Frost requires PARAMS_NBAR_S to be compatible with the vectorized 4-row path.
#endif

// Selecting SHAKE XOF function for the KEM and noise sampling
#define shake     shake256

#define crypto_kem_keypair            crypto_kem_keypair_Frost512
#define crypto_kem_enc                crypto_kem_enc_Frost512
#define crypto_kem_dec                crypto_kem_dec_Frost512

#include "kem.c"
#include "noise.c"
#if defined(USE_REFERENCE)
#include "frost_macrify_reference.c"
#else
#include "frost_macrify.c"
#endif
