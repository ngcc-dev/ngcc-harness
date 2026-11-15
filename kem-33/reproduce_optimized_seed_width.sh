#!/bin/sh
# kem-33-6: the optimized QUBE-384/512 sets use 32-byte seeds where the
# specification and the reference implementation use lambda/8 bytes.
#
# Source certificate. Checks the reference definition, the optimized
# parameter files for the 384- and 512-bit sets, and the key-generation and
# encryption lines that store or consume SEED_BYTES of secret seed.
set -u
cd "$(dirname "$0")/.." || exit 2
bad() { echo "NOT CONFIRMED kem-33-6: $1"; exit 1; }

impl=kem-33/Implementations
for level in 384 512; do
    grep -q '^#define SEED_BYTES  *PARAM_SECURITY_BYTES' \
        "$impl/Reference_Implementation/qube-$level/src/ref/parameters.h" ||
        bad "reference qube-$level no longer ties SEED_BYTES to the security level"
done
echo 'reference: qube-384 and qube-512 define SEED_BYTES as PARAM_SECURITY_BYTES (48 and 64)'

opt=$impl/Optimized_Implementation/src
for set in 4:384 7:384 5:512 8:512; do
    n=${set%:*}; level=${set#*:}; p=$opt/x86/qube-$n/parameters.h
    grep -Eq "^#define PARAM_SECURITY  *$level( |\$)" "$p" || bad "qube-$n is not a $level-bit set"
    grep -Eq '^#define SEED_BYTES  *32( |$)' "$p" || bad "qube-$n SEED_BYTES is not 32"
    q=$opt/x86/qube-$n/qube.c
    grep -q 'uint8_t keypair_seed\[2 \* SEED_BYTES\]' "$q" &&
        grep -q 'memcpy(dk_pke, seed_dk, SEED_BYTES);' "$q" &&
        grep -q 'xof_init(&dk_xof_ctx, seed_dk, SEED_BYTES);' "$q" &&
        grep -q 'xof_init(&theta_xof_ctx, theta, SEED_BYTES);' "$q" ||
        bad "qube-$n key generation or encryption differs from the archived seed handling"
    echo "optimized qube-$n ($level-bit): SEED_BYTES = 32; decryption key and encryption XOF use 32-byte seeds"
done
k=$opt/common/kem.c
grep -q 'uint8_t seed_pke\[SEED_BYTES\]' "$k" && grep -q 'prng_get_bytes(seed_pke, SEED_BYTES);' "$k" &&
    grep -q 'memcpy(theta, K_theta + SHARED_SECRET_BYTES, SEED_BYTES);' "$k" ||
    bad 'optimized kem.c differs from the archived seed handling'

if command -v pdftotext >/dev/null 2>&1; then
    text=$(pdftotext -layout kem-33/kem-33-spec.pdf -)
    echo "$text" | grep -q 'Input: randomness ρ ∈ Bλ/8' && echo "$text" | grep -q 'ρc ∈ Bλ/8' ||
        bad 'specification text differs (seed widths in Figs. 1-2)'
    echo 'spec: key-generation randomness and encryption randomness are lambda/8 bytes'
fi
echo 'CONFIRMED kem-33-6: optimized QUBE-384 and QUBE-512 derive the decryption key and the encryption randomness from 256-bit seeds'
exit 0
