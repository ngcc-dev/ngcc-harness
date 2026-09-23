// SPDX-FileCopyrightText: 2026 The Project OSIDH-LD Authors
// SPDX-License-Identifier: Apache-2.0

#ifndef BUNDLEPROTOCOLS_INTERNAL
#define BUNDLEPROTOCOLS_INTERNAL

#include "protocols_helper.h"
#include "bundleprotocols.h"

void encode_bundlevertex(void *dst, const bundlevertex_t *in);
void decode_bundlevertex(bundlevertex_t *out, void *in);
void make_SecretKey(secretkey_t sk, DRNG_ctx *drng);
void print_SecretKey(const secretkey_t sk);
void print_PublicKey(const orient_cycle_t *pk);
void print_SecretAgreement(const fp2_t a);

#endif