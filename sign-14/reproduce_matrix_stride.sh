#!/bin/sh
# sign-14-6: Lynx stores L0, L1, L2 with 8 words per row and reads them with
# ceil(lambda/64) words per row, so at 160 and 256 bits they lose rank.
#   Source: lynx_t holds uint64_t L[512][8] (lynx_matrices.h) in all eight
#   reference trees, and owf.c casts mats.L0..L3 to uint64_t (*)[3], [4],
#   [6] for lambda = 160, 256, 384.
#   Runtime: each reference tree for 160f, 256f, 384f and 512f is built from
#   its unmodified sources (default XOF) with reproduce_matrix_stride.c, which
#   measures the ranks for three OWF inputs and checks through the exported
#   lynx_extend_witness that adding a kernel vector of L0 (as read) to the
#   key leaves v2 = S2(L0 k + c) unchanged and changes v1.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-14-6: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-14-6: $1"; exit 1; }
ref=sign-14/Implementations/Reference_Implementation
[ -d "$ref/Lynxer-256f" ] || bad "tree not found"

n=0
for d in "$ref"/Lynxer-*; do
    grep -q 'uint64_t L0\[LYNX_MAX_CSP\]\[LYNX_MAX_WORDS\];' "$d/lynx_matrices.h" &&
        grep -q '#define LYNX_MAX_WORDS (LYNX_MAX_CSP / 64)' "$d/lynx_matrices.h" || bad "$d stores the matrices differently"
    for w in 3 4 6; do
        grep -q "const uint64_t (\*L0)\[$w\] = (const uint64_t (\*)\[$w\])mats.L0;" "$d/owf.c" || bad "$d/owf.c has no [$w] cast"
    done
    n=$((n + 1))
done
[ $n = 8 ] || bad "expected 8 reference trees, found $n"
grep -q 'lynx_gf2_matvec_padded_reference_stride' sign-14/Implementations/Optimized_Implementation/Lynxer-256f/owf_proof.inc ||
    bad "optimized tree differs"
echo 'source: 8/8 reference trees store 8 words per row and read L0..L3 with 3, 4 or 6 words per row; the optimized tree reads with the same stride'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
for s in 160f 256f 384f 512f; do
    P=LYNXER_$(echo $s | tr 'a-z' 'A-Z')
    (cd "$ref/Lynxer-$s" && gcc -O2 -std=gnu11 -w -DXOF_PSEUDO -DLYNX_PARAM=$P -I. -o "$scratch/r$s" \
        "$here/reproduce_matrix_stride.c" lynx_matrices.c owf.c fields.c instances.c utils.c auxfunc.c drng.c) \
        >"$scratch/cc" 2>&1 || { tail -20 "$scratch/cc"; bad "Lynxer-$s build failed"; }
    out=$("$scratch/r$s") || { echo "$out"; bad "Lynxer-$s driver failed"; }
    echo "$out" | grep -q '^RESULT ' || { echo "$out"; bad "Lynxer-$s: no result"; }
    echo "$out" | tr '|' '\n' | sed -n '2,$p' >"$scratch/rows"
    [ "$(wc -l <"$scratch/rows")" = 3 ] || { echo "$out"; bad "Lynxer-$s: expected three inputs"; }
    lam=${s%[sf]}
    while read -r row; do
        set -- $(echo "$row" | sed 's/[a-z0-9_]*=//g; s/[,/]/ /g')
        # $1..$3 stored ranks, $4..$6 read ranks, $7 kernel, $8/$9 v2 equal, $10/$11 v1 differs
        [ "$1" = "$lam" ] && [ "$2" = "$lam" ] && [ "$3" = "$lam" ] || bad "Lynxer-$s: a stored matrix is not full rank ($row)"
        [ "$8" = "$9" ] && [ "${10}" = "${11}" ] || bad "Lynxer-$s: v2 changed or v1 did not over the kernel ($row)"
        case $lam in
            160) [ "$4$5$6" = 100100100 ] && [ "$7" = 60 ] && [ "$9" -gt 0 ] || bad "Lynxer-$s: ranks differ from 100 ($row)" ;;
            256) [ "$4$5$6" = 128128128 ] && [ "$7" = 128 ] && [ "$9" -gt 0 ] || bad "Lynxer-$s: ranks differ from 128 ($row)" ;;
            512) [ "$4$5$6" = 512512512 ] && [ "$7" = 0 ] || bad "Lynxer-$s: control lost rank ($row)" ;;
        esac
        if [ "$9" -gt 0 ]; then
            echo "Lynxer-$s: L0/L1/L2 rank $4/$5/$6 as read ($1 as stored); kernel of L0 has dimension $7; key + kernel vector: v2 unchanged $8/$9, v1 changed ${10}/${11}"
        else
            echo "Lynxer-$s: L0/L1/L2 rank $4/$5/$6 as read ($1 as stored); kernel of L0 is trivial"
        fi
    done <"$scratch/rows"
done
echo 'ATTACK sign-14-6: the implemented L0, L1, L2 have rank 100 of 160 and 128 of 256, and v2 depends only on L0 k as read: CONFIRMED'
exit 0
