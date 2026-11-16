#!/bin/sh
# sign-06-9: COMPASS-SIG's sig_verify and sig_sign discard the declared
# public- and secret-key lengths and read fixed-size keys. For each parameter
# set, a key copied into a heap buffer of the declared length (one byte short)
# must give an AddressSanitizer heap-buffer-overflow read in unpack_pk or
# unpack_sk; the full-length control must run cleanly.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "${NGCC_ROOT:-$here/..}" || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-06-9: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-06-9: $1"; exit 1; }

impl='sign-06/Implementations'
for d in "$impl"/Reference_Implementation/COMPASS-SIG-* "$impl"/Optimized_Implementation/COMPASS-SIG-*; do
    grep -Fq 'return crypto_sign_verify(sn, sn_len_bytes, m, m_len_bytes, NULL, 0, pk);' "$d/SIG_AlgorithmInstance.c" &&
        grep -Fq 'int ret = crypto_sign_signature(sn, &out_len, m, m_len_bytes, NULL, 0, sk);' "$d/SIG_AlgorithmInstance.c" ||
        bad "$d: API wrapper differs from the archive"
done
echo 'source: all eight wrappers drop pk_len_bytes and sk_len_bytes'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
driver=$here/reproduce_short_key_read.c
for level in 128 256 384 512; do
    d="$impl/Reference_Implementation/COMPASS-SIG-$level"
    (cd "$d" && gcc -g -O1 -w -fsanitize=address -I. -DCOMPASS_SIG_MODE=$level -o "$scratch/a$level" "$driver" \
        SIG_AlgorithmInstance.c sign.c packing.c polyvec.c poly.c ntt.c reduce.c rounding.c symmetric-shake.c drng.c auxfunc.c) \
        2>"$scratch/cc" || { cat "$scratch/cc"; bad "COMPASS-SIG-$level ASan build failed"; }
    ASAN_OPTIONS=detect_leaks=0 "$scratch/a$level" control >"$scratch/o" 2>&1 || { cat "$scratch/o"; bad "COMPASS-SIG-$level control failed"; }
    grep -q 'rc=0' "$scratch/o" || bad "COMPASS-SIG-$level control did not verify"
    for k in pk sk; do
        fn=unpack_$k
        ASAN_OPTIONS=detect_leaks=0 "$scratch/a$level" $k >"$scratch/o" 2>&1 && bad "COMPASS-SIG-$level short $k ran cleanly"
        grep -q 'ERROR: AddressSanitizer: heap-buffer-overflow' "$scratch/o" && grep -q "_$fn " "$scratch/o" ||
            { cat "$scratch/o"; bad "COMPASS-SIG-$level short $k: no overflow in $fn"; }
        echo "COMPASS-SIG-$level: one-byte-short $k -> heap-buffer-overflow READ in $fn"
    done
done
echo 'ATTACK sign-06-9: at all four levels a public or secret key one byte shorter than its declared length is read out of bounds by sig_verify or sig_sign (AddressSanitizer heap-buffer-overflow); full-length controls run cleanly CONFIRMED'
exit 0
