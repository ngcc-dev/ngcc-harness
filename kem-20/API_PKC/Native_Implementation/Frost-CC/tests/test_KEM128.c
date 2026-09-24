/********************************************************************************************
* MAMBA-Frost-CC: test utilities.
*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "ds_benchmark.h"
#include "../src/api_frostcc128.h"


#define SYSTEM_NAME    "MAMBA-Frost-CC-128"

#define crypto_kem_keypair            crypto_kem_keypair_FrostCC128
#define crypto_kem_enc                crypto_kem_enc_FrostCC128
#define crypto_kem_dec                crypto_kem_dec_FrostCC128
#define shake                         shake128

#include "test_kem.c"
