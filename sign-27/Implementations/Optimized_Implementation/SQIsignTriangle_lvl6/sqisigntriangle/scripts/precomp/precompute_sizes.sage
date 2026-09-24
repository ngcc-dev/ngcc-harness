#!/usr/bin/env sage
proof.all(False)  # faster

from sage.misc.banner import require_version
if not require_version(9, 8, print_message=True):
    exit('')

################################################################

from parameters import lvl, f, p

################################################################

logp = ceil(log(p, 2))
tors2val = (p+1).valuation(2)
tors2part = (p+1).p_primary_part(2)
tors3part = (p+1).p_primary_part(3)

defs = dict()

TORSION_2POWER_BYTES = (tors2part.bit_length() + 7) // 8
SECURITY_BITS = round(p.bit_length() / 128) * 64
# lvl2 (lambda=160) is not a multiple of 64, so the rounding formula above yields
# 128; override it to the intended security level. (lvl6 -> 512 is correct already.)
if lvl == 2:
    SECURITY_BITS = 160
RESPONSE_LENGTH = ceil(p.bit_length()/2)
RESPONSE_BYTES = (RESPONSE_LENGTH + 9) // 8

fpsz = (logp + 63)//64*8
fp2sz = 2 * fpsz
defs['SECURITY_BITS'] = SECURITY_BITS
defs['SQIsign_response_length'] = ceil(logp/2)
defs['HASH_ITERATIONS'] = 2**(32 * ceil( logp/64 ) - (tors2val - ceil(logp/2)))
defs['FP_ENCODED_BYTES'] = fpsz
defs['FP2_ENCODED_BYTES'] = fp2sz
defs['EC_CURVE_ENCODED_BYTES'] = fp2sz  # just the A
defs['EC_POINT_ENCODED_BYTES'] = fp2sz  # just the x
defs['EC_BASIS_ENCODED_BYTES'] = 3 * defs['EC_POINT_ENCODED_BYTES']

# SQIsignTriangle-specific encoding sizes. The signature is (challenge curve A) +
# a compressed torsion basis of 4 scalar coefficients + a scalar q. Each scalar is
# encoded in 7 bytes per 64-bit limb of the field, i.e. 7 * (number of 64-bit limbs
# needed for p). This reproduces the committed lvl1/3/5 values exactly:
#   lvl1 (4 limbs): COEFF=28, COMPRESSED=112, Q=28, NEW_SIGNATURE=204
#   lvl3 (6 limbs): COEFF=42, COMPRESSED=168, Q=42, NEW_SIGNATURE=306
#   lvl5 (8 limbs): COEFF=56, COMPRESSED=224, Q=56, NEW_SIGNATURE=408
NWORDS_FIELD_SAT = (logp + 63)//64
defs['SQISIGNTRIANGLE_BASIS_COEFF_BYTES'] = 7 * NWORDS_FIELD_SAT
defs['SQISIGNTRIANGLE_COMPRESSED_BASIS_BYTES'] = 4 * defs['SQISIGNTRIANGLE_BASIS_COEFF_BYTES']
defs['SQISIGNTRIANGLE_Q_BYTES'] = defs['SQISIGNTRIANGLE_BASIS_COEFF_BYTES']

defs['PUBLICKEY_BYTES'] = defs['EC_CURVE_ENCODED_BYTES'] + 1  # extra byte for hint
defs['SECRETKEY_BYTES'] = defs['PUBLICKEY_BYTES'] + 5*defs['FP_ENCODED_BYTES'] + 4*TORSION_2POWER_BYTES
defs['NEW_SIGNATURE_BYTES'] = defs['EC_CURVE_ENCODED_BYTES'] \
    + defs['SQISIGNTRIANGLE_COMPRESSED_BASIS_BYTES'] + defs['SQISIGNTRIANGLE_Q_BYTES']

size_privkey = defs['SECRETKEY_BYTES']
size_pubkey = defs['PUBLICKEY_BYTES']
size_signature = defs['NEW_SIGNATURE_BYTES']

algname = f'SQIsign_lvl{lvl}'

################################################################

with open('include/encoded_sizes.h','w') as hfile:
    for k,v in defs.items():
        v = ZZ(v)
        print(f'#define {k} {v}', file=hfile)

################################################################

api = f'''
// SPDX-License-Identifier: Apache-2.0

#ifndef api_h
#define api_h

#include <sqisign_namespace.h>

#define CRYPTO_SECRETKEYBYTES {size_privkey}
#define CRYPTO_PUBLICKEYBYTES {size_pubkey}
#define CRYPTO_BYTES {size_signature}

#define CRYPTO_ALGNAME "{algname}"

#if defined(ENABLE_SIGN)
SQISIGN_API
int
crypto_sign_keypair(unsigned char *pk, unsigned char *sk);

SQISIGN_API
int
crypto_sign(unsigned char *sm, unsigned long long *smlen,
            const unsigned char *m, unsigned long long mlen,
            const unsigned char *sk);
#endif

SQISIGN_API
int
crypto_sign_open(unsigned char *m, unsigned long long *mlen,
                 const unsigned char *sm, unsigned long long smlen,
                 const unsigned char *pk);

#endif /* api_h */
'''.strip()

with open(f'../../../nistapi/lvl{lvl}/api.h', 'w') as f:
    print(api, file=f)

