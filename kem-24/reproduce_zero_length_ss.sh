#!/bin/sh
# kem-24-2 follow-up: a zero declared ciphertext length makes decapsulation of
# an all-zero buffer return a publicly predictable shared secret.
#
# For eight fresh keys at each reference level, predicts KDF(K(m0) || H(empty))
# from the public key alone and requires kem_dec(sk, zeros, 0) to return it.
# The same buffer with its true length is rejected implicitly and must not.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-24-2: gcc unavailable'; exit 77; }
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
driver=$PWD/kem-24/reproduce_zero_length_ss.c
for level in 128 256 512; do
    (cd "kem-24/Implementations/Reference_Implementation/scabbard$level" &&
        gcc -O2 -w -I. "-DKEM_HDR=\"KEM_scabbard$level.h\"" -o "$scratch/z$level" \
            auxfunc.c cbd.c drng.c indcpa.c packing.c poly.c polyvec.c verify.c minal.c \
            "KEM_scabbard$level.c" "$driver" -lcrypto) 2>"$scratch/cc" ||
        { cat "$scratch/cc"; echo "NOT CONFIRMED kem-24-2: scabbard$level build failed"; exit 1; }
    out=$("$scratch/z$level") || { echo "NOT CONFIRMED kem-24-2: scabbard$level run failed"; exit 1; }
    echo "scabbard$level: $out"
    echo "$out" | grep -q 'keys=8 zero_length_predicted=8 full_length_predicted=0$' ||
        { echo "NOT CONFIRMED kem-24-2: scabbard$level zero-length secret not predicted"; exit 1; }
done
echo 'CONFIRMED kem-24-2: with ct_len_bytes=0, all three levels return the publicly computed KDF(K(m0) || H(empty)) for an all-zero buffer; the full-length control is rejected'
exit 0
