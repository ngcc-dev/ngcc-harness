#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "ds_benchmark.h"
#include "../src/api_frost384.h"

#define SYSTEM_NAME    "MAMBA-Frost-384"
#define crypto_kem_keypair crypto_kem_keypair_Frost384
#define crypto_kem_enc crypto_kem_enc_Frost384
#define crypto_kem_dec crypto_kem_dec_Frost384
#define shake shake256

#include "test_kem.c"
