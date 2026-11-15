#!/bin/sh
# kem-34-6: Rudraksh2 decryption hands the Minal decoder coefficients at or
# above q, which the submitted DFR tool's decoder reduces first.
#
# Checks the two decoders and poly_sub in source, then builds the archived
# reference decoder at each level and compares it on every reachable
# non-canonical pair with the same decoder on reduced inputs. Rudraksh2-128 and
# -256 (q = 3329) must differ on 987 pairs, none closer than 429.1 to any
# codeword (half the minimum distance is 430.3) and 50 of them strictly inside
# that half distance. Rudraksh2-512 (q = 7681) is the control and must not
# differ.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-34-6: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED kem-34-6: $1"; exit 1; }

impl=kem-34/Implementations
dfr=$impl/Additional_Implementations/Security_and_DFR_Analysis/DFR_Computation/dfr_analysis/minal.c
grep -Fq 'target[0] = (target[0] % KEM_Q);' "$dfr" && grep -Fq 'target[1] = (target[1] % KEM_Q);' "$dfr" ||
    bad 'DFR tool decoder no longer reduces its input'
for tree in Reference Optimized; do
    for level in 128 256 512; do
        d=$impl/${tree}_Implementation/lwekem$level
        grep -Fq 'target[0] % KEM_Q' "$d/minal.c" && bad "$tree lwekem$level decoder now reduces its input"
        grep -Fq 'r->coeffs[i] = barrett_reduce (a->coeffs[i] + 3 * KEM_Q - b->coeffs[i]);' "$d/poly.c" ||
            bad "$tree lwekem$level poly_sub differs from the archive"
    done
done
echo 'source: the DFR tool decoder reduces modulo q first; the six reference and optimized decoders do not, and poly_sub applies only barrett_reduce'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
want='max_output=3689 noncanonical_pairs=2359857 differing=987 nearest_codeword=429.1 half_min_distance=430.3 inside=50 shipped_nearest=779 reduced_nearest=207 witness=10/0'
for level in 128 256 512; do
    d=$impl/Reference_Implementation/lwekem$level
    gcc -O2 -w -I"$d" -o "$scratch/g$level" kem-34/reproduce_decoder_representatives.c \
        "$d/minal.c" "$d/reduce.c" -lm 2>"$scratch/cc" || { cat "$scratch/cc"; bad "lwekem$level build failed"; }
    out=$("$scratch/g$level") || bad "lwekem$level run failed"
    echo "lwekem$level: $out"
    case "$level:$out" in
        512:*' differing=0 '*) ;;
        128:*"$want") ;;
        256:*"$want") ;;
        *) bad "lwekem$level result differs" ;;
    esac
done
echo 'CONFIRMED kem-34-6: at q = 3329 the submitted decoder and its reduced form differ on 987 reachable pairs, all at least 429.1 from every codeword against a half minimum distance of 430.3 (50 lie inside it); no pair differs at q = 7681'
exit 0
