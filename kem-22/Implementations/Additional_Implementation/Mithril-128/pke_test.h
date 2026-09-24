#ifndef PKE_TEST_H
#define PKE_TEST_H

#include "pke.h"
#include "kem_test.h"

int pke_keygen_test(unsigned char pk[RRLWR_PKE_PK_LEN],
                    unsigned char sk[RRLWR_PKE_SK_LEN],
                    const unsigned char seedA[RRLWR_PKE_SEED_A_LEN],
                    const unsigned char seedS[RRLWR_SEED_S_LEN]);

int pke_encrypt_test(unsigned char ct[RRLWR_PKE_CT_LEN],
                     const unsigned char pk[RRLWR_PKE_PK_LEN],
                     const unsigned char m[RRLWR_PKE_MESSAGE_LEN],
                     const unsigned char seedSp[RRLWR_SEED_S_LEN]);

int pke_decrypt_test(unsigned char m[RRLWR_PKE_MESSAGE_LEN],
                     const unsigned char ct[RRLWR_PKE_CT_LEN],
                     const unsigned char sk[RRLWR_PKE_SK_LEN]);

#endif
