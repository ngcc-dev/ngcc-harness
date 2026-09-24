// SPDX-FileCopyrightText: 2026 The Project OSIDH-LD Authors
// SPDX-License-Identifier: Apache-2.0

#ifndef PROTOCOLS_H
#define PROTOCOLS_H

#include "ec.h"
#include "isog.h"
#include "drng.h"

#include "protocol_setup.h"

typedef struct vertex_t {
    ec_point_t A24;
    ec_point_t Ps;
    ec_point_t Pt;
    ec_point_t Qs;
    ec_point_t Qt;
} vertex_t;

typedef vertex_t orient_cycle_t[CYCLE_LENGTH];

typedef int secretkey_t[P_LEN + M_LEN];

void make_SecretKey(secretkey_t sk, DRNG_ctx *drng);

void print_SecretKey(const secretkey_t sk);
void print_PublicKey(const orient_cycle_t *pk);
void print_SecretAgreement(const fp2_t a);

#endif