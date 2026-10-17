#!/bin/sh
# sign-06-6: the shake256 wrapper compresses long message-representative
# inputs to 256 bits; a reduced-width collision transfers a signature.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-06-6: gcc unavailable'; exit 77; }

base=sign-06/Implementations
ref=$base/Reference_Implementation
opt=$base/Optimized_Implementation
id=sign-06-6

# The same wrapper ships in all eight per-set reference and optimized trees.
for tree in "$ref" "$opt"; do
    for n in 128 256 384 512; do
        f=$tree/COMPASS-SIG-$n/symmetric-shake.c
        cmp "$ref/COMPASS-SIG-256/symmetric-shake.c" "$f" >/dev/null ||
            { echo "NOT CONFIRMED $id: $f differs"; exit 1; }
    done
done
sed -n '151,167p' "$ref/COMPASS-SIG-256/symmetric-shake.c" | tr -s ' ' |
    grep -q 'pseudoXOF(256, state->absorb_buf, state->absorb_len \* 8, tmp);' ||
    { echo "NOT CONFIRMED $id: compression call not found in shake256_absorb"; exit 1; }
grep -q 'uint8_t  absorb_buf\[SYMMETRIC_BUFSZ\];' "$ref/COMPASS-SIG-256/symmetric.h" &&
grep -q '#define SYMMETRIC_BUFSZ 512' "$ref/COMPASS-SIG-256/symmetric.h" ||
    { echo "NOT CONFIRMED $id: 512-byte absorb buffer not found"; exit 1; }
echo "source: shake256_absorb replaces a full 512-byte buffer by pseudoXOF(256, ...) (symmetric-shake.c:160-165, 8/8 trees)"

scratch=$(mktemp -d) || exit 2
trap 'rm -rf "$scratch"' EXIT HUP INT TERM
cp sign-06/reproduce_absorb_compression.c "$scratch/" || exit 2

fail=0
common="sign.c packing.c polyvec.c poly.c ntt.c reduce.c rounding.c drng.c auxfunc.c"
build() { # mode
    s=$scratch/src$1
    cp -rL "$ref/COMPASS-SIG-$1" "$s" || exit 2
    (cd "$s" && gcc -O2 -std=gnu11 -w -DCOMPASS_SIG_MODE="$1" -I. \
        -o "$scratch/full$1" ../reproduce_absorb_compression.c $common symmetric-shake.c) || exit 1
    (cd "$s" && gcc -O2 -std=gnu11 -w -DCOMPASS_SIG_MODE="$1" -I. -DpseudoXOF=wrap_pseudoXOF \
        -c -o "$scratch/ss$1.o" symmetric-shake.c &&
     gcc -O2 -std=gnu11 -w -DCOMPASS_SIG_MODE="$1" -DREDUCED -DRBITS=24 -I. \
        -o "$scratch/red$1" ../reproduce_absorb_compression.c $common "$scratch/ss$1.o") || exit 1
}

for n in 128 256 384 512; do
    build "$n"
    out=$("$scratch/full$n") || { echo "NOT CONFIRMED $id: mode $n run failed"; exit 1; }
    printf '%s\n' "$out"
    printf '%s\n' "$out" | grep -q '): yes$' || fail=1
    printf '%s\n' "$out" | grep -q 'prefix bit flip changes mu: yes' || fail=1
done

for n in 256 512; do
    out=$(timeout 300 "$scratch/red$n") || { echo "NOT CONFIRMED $id: reduced mode $n run failed"; exit 1; }
    printf 'reduced build, mode %s:\n%s\n' "$n" "$out"
    printf '%s\n' "$out" | grep -q '^verify(sig, m1) = 0$' || fail=1
    printf '%s\n' "$out" | grep -q '^verify(sig, m2) = 0$' || fail=1
    printf '%s\n' "$out" | grep -q 'suffix bit flipped) = 0$' && fail=1
    pair=$(printf '%s\n' "$out" | sed -n 's/.*prefix counters \([0-9]*\) \([0-9]*\)$/\1 \2/p')
    # shellcheck disable=SC2086
    ctl=$("$scratch/full$n" $pair) || fail=1
    printf 'full-width build, mode %s, same message pair (control):\n%s\n' "$n" "$ctl"
    printf '%s\n' "$ctl" | grep -q '^verify(sig, m1) = 0$' || fail=1
    printf '%s\n' "$ctl" | grep -q '^verify(sig, m2) = 0$' && fail=1
done

if [ "$fail" -eq 0 ]; then
    echo "CONFIRMED $id: mu of a >446-byte message depends on it only through a 256-bit pseudoXOF value (modes 128-512); a 24-bit-reduced collision transfers a signature at 256 and 512, controls reject"
    exit 0
fi
echo "NOT CONFIRMED $id: a check or control failed"
exit 1
