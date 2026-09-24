// SPDX-FileCopyrightText: 2026 The Project OSIDH-LD Authors
// SPDX-License-Identifier: Apache-2.0

#ifndef BUNDLEPROTOCOLS_H
#define BUNDLEPROTOCOLS_H

#include "ec.h"
#include "isog.h"
#include "bundle_ec.h"
#include "bundle_isog.h"
#include "protocols.h"

typedef struct bundlevertex_t {
    bundleec_point_t A24;
    bundleec_point_t Ps;
    bundleec_point_t Pt;
    bundleec_point_t Qs;
    bundleec_point_t Qt;
} bundlevertex_t;

void niike_PublicKey(bundlevertex_t* out, secretkey_t s);
void niike_SecretAgreement(fp2_t* out, const bundlevertex_t* in, secretkey_t s);

#endif