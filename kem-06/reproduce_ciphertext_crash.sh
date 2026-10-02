#!/bin/sh
set -eu

COMMIT=e724a12a834bfc063dc0d2959d864842f269eb1e
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
TMP=$(mktemp -d /tmp/ngcc-bra-ct-crash.XXXXXX)
trap 'rm -rf -- "$TMP"' EXIT HUP INT TERM

git clone --quiet https://github.com/acprk/ngcc-round1-cryptanalysis.git "$TMP/audit"
git -C "$TMP/audit" checkout --quiet "$COMMIT"
test "$(git -C "$TMP/audit" rev-parse HEAD)" = "$COMMIT"

cd "$TMP/audit/bra-brqc-padding-malleability"
for level in 128 256 512; do
    echo "=== BRA-$level ==="
    ref="$HERE/Implementations/Reference_Implementation/BRA-$level"
    lib="$HERE/lib/libBRA-$level.so"
    [ -f "$lib" ] || { echo "build first: make -C $HERE" >&2; exit 77; }
    "${CC:-cc}" -O2 -std=gnu99 -DHDR="\"KEM_BRA-$level.h\"" \
        src/kem_audit.c -I"$ref" -L"$HERE/lib" -l"BRA-$level" \
        -Wl,-rpath,"$HERE/lib" -o audit
    out=$(./audit 1 0 5 2>&1)
    printf '%s\n' "$out"
    case "$level:$out" in
        128:*"random ct: 5 trials, crashes=5"*|256:*"random ct: 5 trials, crashes=5"*) ;;
        512:*"random ct: 5 trials, crashes=0"*) ;;
        *) echo "unexpected random-ciphertext result for BRA-$level" >&2; exit 1 ;;
    esac
done
