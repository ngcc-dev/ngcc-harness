/*
 *  This file implements the VOLE protocol.
 */

#ifndef VOLE_H
#define VOLE_H

#include <stdbool.h>

#include "bavc.h"
#include "params.h"

bool decode_all_chall_3(const params_t* params, uint16_t* decoded_chall, const uint8_t* chall);

void vole_commit(const params_t* params, const uint8_t* rootKey, const uint8_t* iv,
                 unsigned int ellhat, bavc_t* vecCom, uint8_t* c, uint8_t* u, uint8_t** v);

bool vole_reconstruct(const params_t* params, uint8_t* com, uint8_t** q, const uint8_t* iv,
                      const uint8_t* chall_3, const uint8_t* decom_i, const uint8_t* c,
                      unsigned int ellhat);

#endif
