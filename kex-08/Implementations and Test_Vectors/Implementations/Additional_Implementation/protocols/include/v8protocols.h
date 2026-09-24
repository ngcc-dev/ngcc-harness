// SPDX-FileCopyrightText: 2026 The Project OSIDH-LD Authors
// SPDX-License-Identifier: Apache-2.0

#ifndef V8PROTOCOLS_H
#define V8PROTOCOLS_H

#include "protocols.h"
#include "v8ec.h"
#include "v8isog.h"

void niike_PublicKey(orient_cycle_t* out, secretkey_t s);
void niike_SecretAgreement(fp2_t* out, const orient_cycle_t* in, secretkey_t s);
void encode_orient_cycle(void *dst, const orient_cycle_t *in);
void decode_orient_cycle(orient_cycle_t *out, void *in);

#endif