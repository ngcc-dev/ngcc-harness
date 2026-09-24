#include "config.h"
#include "vole_params.h"


#ifndef RAIN_H
#define RAIN_H

#include "block.h"

#if defined(OWF_RAIN_3)
#define RAIN_ROUNDS 3
#if SECURITY_PARAM == 128
extern const uint64_t rain_rc_128 [3][2];
extern const uint64_t rain_mat_128 [128*2][2];
#elif SECURITY_PARAM == 192
extern const uint64_t rain_rc_192 [3][3];
extern const uint64_t rain_mat_192 [192*2][4];
#elif SECURITY_PARAM == 256
extern const uint64_t rain_rc_256 [3][4];
extern const uint64_t rain_mat_256 [256*2][4];
#endif

#elif defined(OWF_RAIN_4)
#define RAIN_ROUNDS 4
#if SECURITY_PARAM == 128
extern const uint64_t rain_rc_128 [4][2];
extern const uint64_t rain_mat_128 [128*3][2];
#elif SECURITY_PARAM == 192
extern const uint64_t rain_rc_192 [4][3];
extern const uint64_t rain_mat_192 [192*3][4];
#elif SECURITY_PARAM == 256
extern const uint64_t rain_rc_256 [4][4];
extern const uint64_t rain_mat_256 [256*3][4];
#endif

#endif

#include "rain_impl.h"

void rain_encrypt_block(uint64_t* block, const uint64_t* key);

#endif
