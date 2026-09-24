#include "config.h"
#include "vole_params.h"


#ifndef MQ_H
#define MQ_H

#if defined(OWF_MQ_2_1) || defined(OWF_MQ_2_8)

#include "block.h"
#include "polynomials.h"

#if defined(OWF_MQ_2_1)
#define MQ_GF_BITS 1
#if SECURITY_PARAM == 128
#define MQ_M 152
#elif SECURITY_PARAM == 192
#define MQ_M 224
#elif SECURITY_PARAM == 256
#define MQ_M 320
#endif
#endif

#if defined(OWF_MQ_2_8)
#define MQ_GF_BITS 8
#if SECURITY_PARAM == 128
#define MQ_M 48
#elif SECURITY_PARAM == 192
#define MQ_M 72
#elif SECURITY_PARAM == 256
#define MQ_M 96
#endif
#endif

#define MQ_N_BYTES ((MQ_M*MQ_GF_BITS)/8)


#if defined(OWF_MQ_2_1)
#define MQ_TRI_MAT_LEN (MQ_M * (MQ_M - 1) / 2)

#else
#define MQ_TRI_MAT_LEN (MQ_M * (MQ_M + 1) / 2)
#endif

#define MQ_A_B_LENGTH ((MQ_M + MQ_TRI_MAT_LEN) * OWF_NUM_CONSTRAINTS)

inline uint8_t mq_2_8_mul(uint8_t a, uint8_t b) {
    uint8_t p = 0;
    while (a != 0 && b != 0) {
        p ^= (a * (b & 1));
        a = (a << 1) ^ (0x1b * ((a & 0x80) >> 7));
        b >>= 1;
    }
    return p;
}

void mq_initialize_pk(block_secpar seed, const uint8_t* y, block_secpar* A_b, poly_secpar_vec* y_gfsecpar);
void mq_initialize(const uint8_t* x, block_secpar seed, block_secpar* A_b, poly_secpar_vec* y_gfsecpar, uint8_t* y);

#endif

#endif
