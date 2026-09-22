#include <stdlib.h>
#include <string.h>

#include "ccakem.h"
#include "hamming.h"

#include "auxfunc.h"
#include "drng.h"

// DRNG_ctx for generating pseudorandom numbers within the KEM scheme
extern DRNG_ctx drng_algorithm;

void ID(uint8_t id[RLWE_SEED_LEN], const uint8_t pk[RLWE_CCA_PK_LEN]) {
    memcpy(id, pk + RLWE_SEED_LEN, RLWE_SEED_LEN);
}

int32_t cmp(uint8_t* c1, uint8_t* c2, int32_t len) {
    int32_t tag = 0;
    for (int32_t i = 0; i < len; i += 4) {
        tag += (c1[i] != c2[i]);  // Do not break: constant time
    }
    return tag;
}

void CCAKEM_KeyGen(uint8_t sk[RLWE_CCA_SK_LEN], uint8_t pk[RLWE_CCA_PK_LEN]) {
    CPAPKE_KeyGen(sk, pk);
    memcpy(sk + RLWE_CPA_SK_LEN, pk, RLWE_CPA_PK_LEN);
    get_random_number(&drng_algorithm, sk + RLWE_CPA_SK_LEN + RLWE_CPA_PK_LEN, RLWE_SEED_LEN * 8);
}

void CCAKEM_Encaps(uint8_t ct[RLWE_CCA_CT_LEN], uint8_t key[RLWE_KEY_LEN], const uint8_t pk[RLWE_CCA_PK_LEN]) {
    memset(ct, 0, RLWE_CCA_CT_LEN);
    memset(key, 0, RLWE_KEY_LEN);

    uint8_t m[RLWE_MSG_LEN + 1 + RLWE_SEED_LEN] = {0};
    uint8_t Kr[RLWE_SEED_LEN * 2] = {0};
    get_random_number(&drng_algorithm, m, RLWE_MSG_LEN * 8);
    ID(m + RLWE_MSG_LEN + 1, pk);

    size_t outLen = 0;
    while (outLen < RLWE_SEED_LEN * 2) {
        sm3hash(256, m, (RLWE_MSG_LEN + 1 + RLWE_SEED_LEN) << 3, Kr + outLen);  
        m[RLWE_MSG_LEN]++;
        outLen += 32; // assert: 32 | RLWE_SEED_LEN
    }
    CPAPKE_Encrypt(ct, (uint8_t*)pk, (uint8_t*)m, Kr + RLWE_SEED_LEN);

    uint8_t state[RLWE_CCA_CT_LEN + RLWE_SEED_LEN];
    memcpy(state, ct, RLWE_CCA_CT_LEN);
    memcpy(state + RLWE_CCA_CT_LEN, Kr, RLWE_SEED_LEN);
    pseudoXOF(RLWE_KEY_LEN << 3, state, (RLWE_CCA_CT_LEN + RLWE_SEED_LEN) << 3, key);
}

int CCAKEM_Decaps(uint8_t key[RLWE_KEY_LEN],
                  const uint8_t sk[RLWE_CCA_SK_LEN],
                  const uint8_t ct[RLWE_CCA_CT_LEN]) {
    memset(key, 0, RLWE_KEY_LEN);

    uint8_t m[RLWE_MSG_LEN + 1 + RLWE_SEED_LEN] = {0};
    uint8_t Kr[RLWE_SEED_LEN * 2] = {0};
    if (CPAPKE_Decrypt((uint8_t*)m, (uint8_t*)sk, (uint8_t*)ct) != 0) {
        return -1;
    }
    ID(m + RLWE_MSG_LEN + 1, sk + RLWE_CPA_SK_LEN);

    size_t outLen = 0;
    while (outLen < RLWE_SEED_LEN * 2) {
        sm3hash(256, m, (RLWE_MSG_LEN + 1 + RLWE_SEED_LEN) << 3, Kr + outLen);  
        m[RLWE_MSG_LEN]++;
        outLen += 32; // assert: 32 | RLWE_SEED_LEN
    }

    uint8_t ct2[RLWE_CPA_CT_LEN];
    CPAPKE_Encrypt(ct2, (uint8_t*)sk + RLWE_CPA_SK_LEN, (uint8_t*)m, Kr + RLWE_SEED_LEN);
    int32_t tag = cmp((uint8_t*)ct, ct2, RLWE_CPA_CT_LEN);

    uint8_t state[RLWE_CCA_CT_LEN + RLWE_SEED_LEN];
    memcpy(state, ct, RLWE_CCA_CT_LEN);

    if (tag == 0) {
        memcpy(state + RLWE_CCA_CT_LEN, Kr, RLWE_SEED_LEN);
        pseudoXOF(RLWE_KEY_LEN << 3, state, (RLWE_CCA_CT_LEN + RLWE_SEED_LEN) << 3, key);
    } else {
        memcpy(state + RLWE_CCA_CT_LEN, sk + RLWE_CPA_SK_LEN + RLWE_CPA_PK_LEN, RLWE_SEED_LEN);
        pseudoXOF(RLWE_KEY_LEN << 3, state, (RLWE_CCA_CT_LEN + RLWE_SEED_LEN) << 3, key);
    }
    return 0;
}