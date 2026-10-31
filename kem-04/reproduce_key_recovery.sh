#!/bin/sh
# kem-04-2 BAG-Piglet: rejection-key plaintext-checking oracle -> recovery of
# the decryption secret x and the challenge key.  Builds kem-04/reproduce_key_recovery.c against the
# unmodified reference sources in a temporary directory.  The attack uses only
# the public key and ccakem_decaps outputs; the secret key is read only by the
# final VALIDATION line.  Default: BAG-Piglet-128 (about 5 minutes).
# NGCC_LONG=1 also runs BAG-Piglet-256 (about 1 hour).
set -eu
cd "$(dirname "$0")/.."
command -v cc >/dev/null 2>&1 || { echo 'SKIP kem-04-2: cc unavailable'; exit 77; }
tmp=$(mktemp -d "${TMPDIR:-/tmp}/ngcc-piglet.XXXXXX") || exit 77
trap 'rm -rf "$tmp"' EXIT
sets="128:1"
[ "${NGCC_LONG:-0}" = 1 ] && sets="128:1 256:2"
rc=0
for s in $sets; do
    l=${s%:*}; seed=${s#*:}
    D=kem-04/Implementations/Reference_Implementation/bag_piglet$l
    cc -O2 -w -I$D/src/common -I$D/src/scheme -I$D/src/ffi -I$D/src/rbc -I$D/src/kat \
        -I$D/lib/api_pkc -I$D/lib/randombytes -o "$tmp/attack$l" kem-04/reproduce_key_recovery.c \
        $D/src/common/aes256ctr.c $D/src/common/augabidulin.c $D/src/common/ccakem.c \
        $D/src/common/ccapke.c $D/src/common/gabidulin.c $D/src/common/parsing.c \
        $D/src/common/q_polynomial.c $D/src/common/tools.c $D/src/ffi/ffi_elt.c \
        $D/src/ffi/ffi_field.c $D/src/ffi/ffi_vec.c $D/src/rbc/rbc_elt.c $D/src/rbc/rbc_poly.c \
        $D/src/rbc/rbc_qre.c $D/src/rbc/rbc_vec.c $D/src/rbc/rbc_vspace.c \
        $D/src/scheme/bag_piglet.c $D/lib/api_pkc/auxfunc.c $D/lib/api_pkc/drng.c \
        $D/lib/randombytes/randombytes.c -lm
    st=0; "$tmp/attack$l" "$seed" > "$tmp/out$l" || st=$?
    grep -E '^(BAG-Piglet|RESULT|VALIDATION)' "$tmp/out$l" || true
    [ "$st" = 0 ] && grep -q 'challenge key recovered=YES' "$tmp/out$l" &&
        grep -q 'recovered x == true x: yes' "$tmp/out$l" || { echo "attack$l: exit $st"; rc=1; }
done
if [ "$rc" = 0 ]; then
    echo "CONFIRMED kem-04-2: decapsulation outputs recover x and the challenge key"
    exit 0
fi
echo "NOT CONFIRMED kem-04-2"
exit 1
