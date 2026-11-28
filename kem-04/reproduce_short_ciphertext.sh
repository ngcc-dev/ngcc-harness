#!/bin/sh
# kem-04-5: BAG-Piglet decapsulation ignores the declared ciphertext length.
#   Source: kem_dec casts ct_len_bytes and sk_len_bytes to void in all eight
#   trees, and ccakem_decaps reads CCAKEM_CT_SIZE bytes of ct.
#   Runtime: each reference set is built from its unmodified sources with
#   reproduce_short_ciphertext.c. An honest ciphertext whose declared bytes
#   end at a PROT_NONE page decapsulates correctly at full length, and faults
#   when declared one byte short or as a single byte.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v cc >/dev/null 2>&1 || { echo 'SKIP kem-04-5: cc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED kem-04-5: $1"; exit 1; }
impl=kem-04/Implementations
[ -d "$impl/Reference_Implementation/bag_piglet128" ] || bad "tree not found"

n=0
for f in $(find "$impl" -path '*src/kat/KEM_AlgorithmInstance.c' | sort); do
    tr -d '\r' <"$f" | sed -n '/^int kem_dec(/,/^}/p' | grep -q '(void) ct_len_bytes;' || bad "$f uses ct_len_bytes"
    n=$((n + 1))
done
[ $n = 8 ] || bad "expected 8 wrappers, found $n"
echo "source: 8/8 kem_dec wrappers discard ct_len_bytes"

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
ulimit -c 0 2>/dev/null
for l in 128 256 384 512; do
    D=$impl/Reference_Implementation/bag_piglet$l
    cc -O2 -w -DPIGLET_LABEL="\"BAG-Piglet-$l\"" -I$D/src/common -I$D/src/scheme -I$D/src/ffi -I$D/src/rbc -I$D/src/kat \
        -I$D/lib/api_pkc -I$D/lib/randombytes -o "$scratch/p$l" "$here/reproduce_short_ciphertext.c" \
        $D/src/kat/KEM_AlgorithmInstance.c \
        $D/src/common/aes256ctr.c $D/src/common/augabidulin.c $D/src/common/ccakem.c \
        $D/src/common/ccapke.c $D/src/common/gabidulin.c $D/src/common/parsing.c \
        $D/src/common/q_polynomial.c $D/src/common/tools.c $D/src/ffi/ffi_elt.c \
        $D/src/ffi/ffi_field.c $D/src/ffi/ffi_vec.c $D/src/rbc/rbc_elt.c $D/src/rbc/rbc_poly.c \
        $D/src/rbc/rbc_qre.c $D/src/rbc/rbc_vec.c $D/src/rbc/rbc_vspace.c \
        $D/src/scheme/bag_piglet.c $D/lib/api_pkc/auxfunc.c $D/lib/api_pkc/drng.c \
        $D/lib/randombytes/randombytes.c -lm >"$scratch/cc" 2>&1 || { tail -20 "$scratch/cc"; bad "BAG-Piglet-$l build failed"; }
    out=$("$scratch/p$l") || { echo "$out"; bad "BAG-Piglet-$l driver failed"; }
    line=$(echo "$out" | grep '^RESULT ') || { echo "$out"; bad "BAG-Piglet-$l: no result"; }
    echo "$line" | grep -q ' full=ok ' || { echo "$line"; bad "BAG-Piglet-$l full-length control failed"; }
    echo "$line" | grep -Eq ' minus1=sig[0-9]+ one=sig[0-9]+$' || { echo "$line"; bad "BAG-Piglet-$l short ciphertexts were not read past"; }
    ctb=$(echo "$line" | sed 's/.* ct=\([0-9]*\) .*/\1/')
    echo "BAG-Piglet-$l: $ctb-byte ciphertext before a PROT_NONE page decapsulates correctly; declared $((ctb - 1)) bytes or 1 byte, kem_dec reads past the buffer and faults ($(echo "$line" | sed 's/.* minus1=\(sig[0-9]*\).*/\1/'))"
done
echo 'ATTACK kem-04-5: kem_dec ignores the declared ciphertext length and reads past a short buffer: CONFIRMED'
exit 0
