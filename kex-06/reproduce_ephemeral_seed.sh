#!/bin/sh
# kex-06-1 extension: MAMBA-NIKE-384/-512 session secrets come from a 32-byte
# noiseseed, so a correct 2^256 guess recovers the session key from (pk_B, M1).
# Builds the unmodified nike.c with a fed nike_randombytes in a temporary copy.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kex-06-1: gcc unavailable'; exit 77; }

ref=kex-06/Implementations/Reference_Implementation
scratch=$(mktemp -d) || exit 2
case "$scratch" in /tmp/tmp.*) ;; *) echo 'unexpected temporary path' >&2; exit 2 ;; esac
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM

fail=0
for level in 384 512; do
    dir=$ref/MAMBA-NIKE-$level
    # Source certificate: nike_sharedb draws mu and a 32-byte noiseseed, and
    # expands r and the HelpRec bits from that noiseseed alone.
    body=$(sed -n '/^int nike_sharedb(/,/^}/p' "$dir/nike.c")
    for pat in 'unsigned char noiseseed\[32\];' 'nike_randombytes(mu, NIKE_SEEDBYTES)' \
               'nike_randombytes(noiseseed, 32)' 'poly_getnoise(&sp,noiseseed,0);' \
               'helprec(&c, &v, noiseseed, 3);'; do
        printf '%s\n' "$body" | grep -q "$pat" || {
            echo "NOT CONFIRMED kex-06-1: MAMBA-NIKE-$level nike_sharedb lacks '$pat'"; exit 1; }
    done
    grep -q "^#define NIKE_SEEDBYTES 32" "$dir/params.h" || {
        echo "NOT CONFIRMED kex-06-1: MAMBA-NIKE-$level NIKE_SEEDBYTES is not 32"; exit 1; }

    work=$scratch/$level
    mkdir -p "$work" && cp -rL "$dir" "$work/src" || exit 2
    s=$work/src
    gcc -O2 -w -DKAT_BUILD -I"$s" -o "$work/witness" kex-06/reproduce_ephemeral_seed.c \
        "$s/nike.c" "$s/poly.c" "$s/toom.c" "$s/error_correction.c" "$s/reduce.c" \
        "$s/fips202.c" "$s/crypto_stream_chacha20.c" -lm || exit 1
    if "$work/witness"; then
        echo "CONFIRMED kex-06-1: MAMBA-NIKE-$level session key recovered from (pk_B, M1) and the 32-byte noiseseed"
    else
        echo "NOT CONFIRMED kex-06-1: MAMBA-NIKE-$level"
        fail=1
    fi
done
exit $fail
