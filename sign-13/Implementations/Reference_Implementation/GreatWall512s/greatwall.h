#include "config.h"
#include "vole_params.h"


#ifndef GREATWALL_H
#define GREATWALL_H

#include "block.h"

#if defined(OWF_GREATWALL)

#if SECURITY_PARAM == 128
extern const uint64_t greatwall_rc_137 [2][3];
extern const uint64_t greatwall_mat_137_0 [137][3];
extern const uint64_t greatwall_mat_137_1 [137][3];
extern const uint64_t greatwall_mat_137_2 [137][3];
extern const uint64_t greatwall_mat_137_2_inv [137][3];
extern const uint64_t greatwall_pow_mat_137_70 [137][3];
extern const uint64_t greatwall_pow_mat_137_75 [137][3];
#elif SECURITY_PARAM == 192
extern const uint64_t greatwall_rc_197 [2][4];
extern const uint64_t greatwall_mat_197_0 [197][4];
extern const uint64_t greatwall_mat_197_1 [197][4];
extern const uint64_t greatwall_mat_197_2 [197][4];
extern const uint64_t greatwall_mat_197_2_inv [197][4];
extern const uint64_t greatwall_pow_mat_197_94 [197][4];
extern const uint64_t greatwall_pow_mat_197_105 [197][4];
#elif SECURITY_PARAM == 256
extern const uint64_t greatwall_rc_263 [2][5];
extern const uint64_t greatwall_mat_263_0 [263][5];
extern const uint64_t greatwall_mat_263_1 [263][5];
extern const uint64_t greatwall_mat_263_2 [263][5];
extern const uint64_t greatwall_mat_263_2_inv [263][5];
extern const uint64_t greatwall_pow_mat_263_129 [263][5];
extern const uint64_t greatwall_pow_mat_263_136 [263][5];
#elif SECURITY_PARAM == 512
extern const uint64_t greatwall_rc_521 [2][9];
extern const uint64_t greatwall_mat_521_0 [521][9];
extern const uint64_t greatwall_mat_521_1 [521][9];
extern const uint64_t greatwall_mat_521_2 [521][9];
extern const uint64_t greatwall_mat_521_2_inv [521][9];
extern const uint64_t greatwall_pow_mat_521_248 [521][9];
extern const uint64_t greatwall_pow_mat_521_273 [521][9];
#endif

#include "greatwall_impl.h"

void greatwall_encrypt_block(uint64_t* block, const uint64_t* key);

#endif

#endif
