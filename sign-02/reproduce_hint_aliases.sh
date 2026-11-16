#!/bin/sh
# sign-02-4: BiT-512 hint bytes admit several encodings that the reference
# verifier accepts as the same signature.
#
# Checks the hint packer and unpacker in the reference and AVX2 BiT-512 trees,
# then signs one message with the reference build and verifies the final
# hint byte of each polynomial rewritten with each value of its unused base-3
# digit, and one complete-group byte at most 12 rewritten by +243. All must be
# accepted for the signed message and rejected for a different message.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-02-4: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-02-4: $1"; exit 1; }

for t in Reference_Implementation/BiT-512 Optimized_Implementation/BiT-512-avx2; do
    f=sign-02/Implementations/$t/packing.c
    grep -Fq 'static const uint8_t h3_pow[5] = {1, 3, 9, 27, 81};' "$f" &&
        grep -Fq 'if (idx >= BIT_N) break;' "$f" || bad "$t hint packing differs from the archive"
done
grep -Eq '^#define BIT_N +1024' sign-02/Implementations/Reference_Implementation/BiT-512/params.h ||
    bad 'BiT-512 BIT_N differs'
echo 'source: reference and AVX2 BiT-512 pack five ternary digits per byte and stop decoding at coefficient 1024 without checking the rest'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
d=sign-02/Implementations/Reference_Implementation/BiT-512
(cd "$d" && gcc -O2 -std=c99 -w -I. -o "$scratch/a" "$OLDPWD/sign-02/reproduce_hint_aliases.c" \
    $(ls *.c | grep -v -e KAT_SIG.c -e SIG_AlgorithmInstance.c)) 2>"$scratch/cc" ||
    { cat "$scratch/cc"; bad 'build failed'; }
out=$("$scratch/a") || bad 'run failed'
echo "BiT-512: $out"
[ "$out" = 'honest=0 final_byte_variants_accepted=9 distinct=6 complete_group_alias_accepted=1 other_message_rejected=1' ] ||
    bad 'behaviour differs'
echo 'CONFIRMED sign-02-4: six byte-distinct final-byte variants and one complete-group alias of an honest BiT-512 signature verify for the signed message and are rejected for another'
exit 0
