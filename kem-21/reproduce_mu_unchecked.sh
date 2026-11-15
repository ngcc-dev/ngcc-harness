#!/bin/sh
# kem-21-4: Viper decapsulation never checks the ciphertext's mu field.
#
# Checks the self-comparison in all ten reference and optimized trees, then
# builds each reference level and decapsulates 50 ciphertexts that are honest
# except for an arbitrary mu. All must return the valid-path key; the same
# ciphertexts with one flipped bit of u are the control and must not.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-21-4: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED kem-21-4: $1"; exit 1; }

impl=kem-21/Implementations
for tree in Reference Optimized; do
    for level in 128 192 256 384 512; do
        f=$impl/${tree}_Implementation/MAMBA-Viper-$level/viper.c
        grep -q 'const unsigned char \*mu = ct + VIPER_PACKED_U_BYTES + VIPER_PACKED_V_BYTES;' "$f" &&
            grep -q 'diff |= diff_bytes(mu, ct + VIPER_PACKED_U_BYTES + VIPER_PACKED_V_BYTES, VIPER_MU_BYTES);' "$f" ||
            bad "$tree MAMBA-Viper-$level differs from the archived re-encryption check"
    done
done
echo 'source: in all ten trees the re-encryption check compares the received mu with itself'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
for level in 128 192 256 384 512; do
    d=$impl/Reference_Implementation/MAMBA-Viper-$level
    gcc -O2 -w -I"$d" -o "$scratch/t$level" kem-21/reproduce_mu_unchecked.c "$d/viper.c" \
        "$d/viper_arith.c" "$d/viper_e8.c" "$d/viper_message_codec.c" "$d/verify.c" \
        "$d/viper_api_hash.c" "$d/auxfunc.c" "$d/drng.c" 2>"$scratch/cc" ||
        { cat "$scratch/cc"; bad "Viper-$level build failed"; }
    out=$("$scratch/t$level") || bad "Viper-$level run failed"
    echo "Viper-$level: $out"
    [ "$out" = 'arbitrary_mu_accepted=50 control_accepted=0' ] || bad "Viper-$level behaviour differs"
done
echo 'CONFIRMED kem-21-4: at all five levels decapsulation accepts ciphertexts whose mu is arbitrary; a flipped bit of u is rejected'
exit 0
