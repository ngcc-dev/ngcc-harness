/********************************************************************************************
* MAMBA-Frost: test utilities.
*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "ds_benchmark.h"
#include "../src/api_frost192.h"


#define SYSTEM_NAME    "MAMBA-Frost-192"

#define crypto_kem_keypair            crypto_kem_keypair_Frost192
#define crypto_kem_enc                crypto_kem_enc_Frost192
#define crypto_kem_dec                crypto_kem_dec_Frost192
#define shake                         shake256

#include "test_kem.c"
