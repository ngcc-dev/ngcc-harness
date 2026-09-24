/********************************************************************************************
* MAMBA-Frost-CC: test utilities.
*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "ds_benchmark.h"
#include "../src/api_frostcc256.h"


#define SYSTEM_NAME    "MAMBA-Frost-CC-256"

#define crypto_kem_keypair            crypto_kem_keypair_FrostCC256
#define crypto_kem_enc                crypto_kem_enc_FrostCC256
#define crypto_kem_dec                crypto_kem_dec_FrostCC256
#define shake                         shake256

#include "test_kem.c"
