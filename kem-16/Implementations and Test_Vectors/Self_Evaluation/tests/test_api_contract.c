#include <stdio.h>
#include "api.h"
#include "parameters.h"

int main(void) {
    const int derived_pk = SEED_BYTES + VEC_N_SIZE_BYTES;
    const int derived_sk = derived_pk + SEED_BYTES + PARAM_SECURITY_BYTES + SEED_BYTES;
    const int derived_ct = VEC_N_SIZE_BYTES + VEC_COMPRESSED_PAYLOAD_BYTES + SALT_BYTES;

    if (PARAM_N1N2 != PARAM_N1 * PARAM_N2 || PARAM_L1 != PARAM_N1N2 || PARAM_L1 > PARAM_N) {
        puts("api_contract_parameter_length_fail");
        return 1;
    }
    if (PUBLIC_KEY_BYTES != derived_pk || CRYPTO_PUBLICKEYBYTES != derived_pk) {
        puts("api_contract_pk_fail");
        return 2;
    }
    if (SECRET_KEY_BYTES != derived_sk || CRYPTO_SECRETKEYBYTES != derived_sk) {
        puts("api_contract_sk_fail");
        return 3;
    }
    if (CIPHERTEXT_BYTES != derived_ct || CRYPTO_CIPHERTEXTBYTES != derived_ct) {
        puts("api_contract_ct_fail");
        return 4;
    }
    if (CRYPTO_BYTES != SHARED_SECRET_BYTES) {
        puts("api_contract_ss_fail");
        return 5;
    }
    puts("api_contract_pass");
    return 0;
}
