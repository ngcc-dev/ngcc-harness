#!/bin/sh
# sign-06-2 (signing path): COMPASS-SIG-384/-512 signing is deterministic from
# the 32-byte secret-key field K, so one signature tests a guess for K and a
# correct guess yields c*s1 = z - y.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-06-2: gcc unavailable'; exit 77; }

base=sign-06/Implementations
ref=$base/Reference_Implementation
opt=$base/Optimized_Implementation
id=sign-06-2

for n in 384 512; do
    for tree in "$ref" "$opt"; do
        d=$tree/COMPASS-SIG-$n
        grep -q '^#define SEEDBYTES 32$' "$d/params.h" &&
        grep -q '^//#define COMPASS_SIG_RANDOMIZED_SIGNING' "$d/config.h" &&
        grep -q 'shake256_absorb(&state, key, SEEDBYTES);' "$d/sign.c" ||
            { echo "NOT CONFIRMED $id: $d does not have the 32-byte deterministic signing key"; exit 1; }
    done
done
echo "source: SEEDBYTES = 32, rnd = 0^32 (COMPASS_SIG_RANDOMIZED_SIGNING unset), rhoprime = H(K || rnd || mu) in 4/4 trees"

scratch=$(mktemp -d) || exit 2
trap 'rm -rf "$scratch"' EXIT HUP INT TERM
cp sign-06/reproduce_signing_key_root.c "$scratch/" || exit 2

fail=0
for n in 384 512; do
    s=$scratch/src$n
    cp -rL "$ref/COMPASS-SIG-$n" "$s" || exit 2
    (cd "$s" && gcc -O2 -std=gnu11 -w -DCOMPASS_SIG_MODE="$n" -I. -o "$scratch/wit$n" \
        ../reproduce_signing_key_root.c sign.c packing.c polyvec.c poly.c ntt.c reduce.c \
        rounding.c symmetric-shake.c drng.c auxfunc.c) || exit 1
    out=$(timeout 120 "$scratch/wit$n") || { echo "NOT CONFIRMED $id: mode $n run failed"; exit 1; }
    printf '%s\n' "$out"
    printf '%s\n' "$out" | grep -q 'SEEDBYTES=32 (K field)' || fail=1
    printf '%s\n' "$out" | grep -q 're-sign after DRBG reseed identical: yes' || fail=1
    printf '%s\n' "$out" | grep -q 'z - y == c\*s1: yes' || fail=1
    printf '%s\n' "$out" | grep -q 'no match (control ok)' || fail=1
done

if [ "$fail" -eq 0 ]; then
    echo "CONFIRMED $id: 384/512 signatures are a deterministic function of the 32-byte K and mu; the true K reproduces the challenge and gives z - y = c*s1, a one-bit-wrong K does not"
    exit 0
fi
echo "NOT CONFIRMED $id: a check or control failed"
exit 1
