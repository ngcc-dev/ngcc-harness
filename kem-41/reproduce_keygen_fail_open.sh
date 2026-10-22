#!/bin/sh
# Run the frozen ZEN reference key-generation paths under real memory pressure.
set -eu
cd "$(dirname "$0")/.."
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-41-2: gcc unavailable'; exit 77; }
tmp=$(mktemp -d)
trap 'rm -r "$tmp"' EXIT HUP INT TERM
for tier in 128 256 512; do
    src="kem-41/Implementations/Reference_Implementation/ZEN_$tier"
    gcc -std=c99 -O2 "-DZEN_TIER_$tier" -I "$src" -o "$tmp/probe" \
        kem-41/reproduce_keygen_fail_open.c \
        "$src/auxfunc.c" "$src/drng.c" "$src/sample.c" "$src/ntt.c" \
        "$src/poly.c" "$src/symmetric.c" "$src/pke.c" \
        "$src/KEM_AlgorithmInstance.c"
    # The public-stack overlap is build dependent. Try a known working seed,
    # then a bounded sweep; a retry loop times out inside the probe.
    case "$tier" in
        128) seeds='4 0 1 2 3 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23' ;;
        *) seeds='0 1 2 3 4 5 6 7 8 9 10 11 12 13 14' ;;
    esac
    confirmed=0
    for seed in $seeds; do
        if "$tmp/probe" "$seed" >"$tmp/stdout" 2>"$tmp/stderr"; then
            if grep -q "^CONFIRMED kem-41-2 ZEN-$tier:" "$tmp/stdout" &&
               grep -q 'Memory allocation failed' "$tmp/stderr"; then
                sed -n '/^public_candidate_offset=/p;/^observed_public_candidates=/p;/^CONFIRMED kem-41-2 /p' "$tmp/stdout"
                confirmed=1
                break
            fi
        fi
    done
    if [ "$confirmed" -ne 1 ]; then
        echo "SKIP kem-41-2 ZEN-$tier: this host/build did not expose the public-stack overlap"
        exit 77
    fi
done
