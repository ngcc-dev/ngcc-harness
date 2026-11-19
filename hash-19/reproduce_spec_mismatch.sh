#!/bin/sh
# hash-19-4: the MoFang key expansion uses the two block transpositions of
# Algorithms 3 and 4 in exchanged positions.
#
# In the coordinates of Figure 5 (x = column, y = row; the convention the
# code's ShiftRow and key-rotation tables need for Tables 3 and 6), Algorithm 3
# line 7 sets C[x,y] = B[y, 2x+y] for the first block and line 18 sets
# C[x,y] = B[x+2y, x] for the second. The reference code stores word (x,y) as
# key[y][x] and does the opposite: line 138 tmp[i][j] = key0[j][(j+2*i)%3]
# (= B[x+2y, x]) and line 150 tmp[i][j] = key1[(i+2*j)%3][i] (= B[y, 2x+y]).
# MoFang-768 (Algorithm 4, lines 4/12/23) is exchanged on all three blocks.
#
# The KAT_2_12 reference files are regenerated, not taken from the original
# archive: gen_KAT_2_12() of the shipped NGCC generator KAT_CryptHash.c is
# compiled against the unmodified reference sources, and its output is used
# only if its canonical SHA-256 (api/README.md: CR and trailing blanks
# stripped, empty lines dropped) equals the "strict" entry for that file in
# the tracked hash-19/kat.sha256, the digest of the official reference file.
#
# The script builds the reference MoFang-256 and MoFang-768 twice: as shipped
# (control: must reproduce all 4097 KAT_2_12 vectors) and with only the
# transposition expressions set to the printed algorithm (must reproduce none).
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
id=hash-19
for t in gcc sha256sum awk; do
    command -v $t >/dev/null 2>&1 || { echo "SKIP $id-4: $t unavailable"; exit 77; }
done
bad() { echo "NOT CONFIRMED $id-4: $1"; exit 1; }
man=$id/kat.sha256
[ -f "$man" ] || bad "$man (tracked KAT manifest) missing"
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
ref=$id/MoFang/Implementations/Reference_Implementation

# canonical SHA-256 of a KAT text, as recorded in kat.sha256 (api/ngcc_kat.c)
canon_sha() { awk '{ sub(/[\r \t]+$/, ""); if ($0 != "") print }' "$1" | sha256sum | cut -c1-64; }
# use_kat FILE NAME DEST: accept the regenerated FILE only if its canonical
# digest equals the single "strict" manifest entry NAME; then move it to DEST
use_kat() {
    [ -s "$1" ] || bad "$2 was not regenerated"
    kwant=$(awk -v n="$2" '$2 == "strict" && $3 == n { c++; h = $1 } END { if (c == 1) print h }' "$man")
    [ -n "$kwant" ] || bad "no unique strict entry for $2 in $man"
    kgot=$(canon_sha "$1")
    [ "$kgot" = "$kwant" ] || bad "regenerated $2 has canonical SHA-256 $kgot, $man records $kwant"
    echo "  regenerated KAT_2_12 matches $man: $kwant strict $2"
    mv "$1" "$3" || exit 2
}
# the shipped generator, reduced to gen_KAT_2_12(); it writes
# output/KAT_2_12_<ALGORITHM_INSTANCE>.txt in the working directory
cat > "$scratch/katgen.c" <<'EOF'
#define main ngcc_kat_main
#include "KAT_CryptHash.c"
#undef main
int main(void) { return gen_KAT_2_12(ALGORITHM_INSTANCE, DIGEST_BIT_LENGTH) != KAT_SUCCESS; }
EOF

check() { # variant dir kat expected
    out=$("$scratch/$1" "$scratch/$2") || bad "$1 run failed"
    echo "  $1: $out"
    [ "$out" = "$3" ] || bad "$1: expected '$3'"
}

for v in 256 768; do
    d=$scratch/src$v
    mkdir -p "$d" || exit 2
    cp "$ref/MoFang-$v/CryptHash_AlgorithmInstance.c" "$ref/MoFang-$v/CryptHash_AlgorithmInstance.h" \
       "$ref/MoFang-$v/drng.c" "$ref/MoFang-$v/drng.h" "$ref/MoFang-$v/KAT_CryptHash.c" "$d/" || bad "MoFang-$v sources missing"
    c=$d/CryptHash_AlgorithmInstance.c
    gcc -O2 -w -I"$d" -o "$d/katgen" "$scratch/katgen.c" "$c" "$d/drng.c" || bad "MoFang-$v KAT generator build failed"
    (cd "$d" && ./katgen) || bad "MoFang-$v KAT_2_12 generation failed"
    if [ $v = 256 ]; then
        grep -q 'tmp\[i\]\[j\] = key0\[j\]\[(j+2\*i)%3\];' "$c" && grep -q 'tmp\[i\]\[j\] = key1\[(i+2\*j)%3\]\[i\];' "$c" || bad "MoFang-256 transposition lines differ from the expected shipped lines"
        sed -e 's/tmp\[i\]\[j\] = key0\[j\]\[(j+2\*i)%3\];/tmp[i][j] = key0[(i+2*j)%3][i];/' \
            -e 's/tmp\[i\]\[j\] = key1\[(i+2\*j)%3\]\[i\];/tmp[i][j] = key1[j][(j+2*i)%3];/' "$c" > "$d/spec.c"
    else
        grep -q 'key0\[j\]\[(2\*i+j)%3\];' "$c" && grep -q 'key1\[(i+2\*j)%3\]\[i\];' "$c" && grep -q 'key2\[j\]\[(2\*i+j)%3\];' "$c" || bad "MoFang-768 transposition lines differ from the expected shipped lines"
        sed -e 's/key0\[j\]\[(2\*i+j)%3\];/key0[(i+2*j)%3][i];/' \
            -e 's/key1\[(i+2\*j)%3\]\[i\];/key1[j][(2*i+j)%3];/' \
            -e 's/key2\[j\]\[(2\*i+j)%3\];/key2[(i+2*j)%3][i];/' "$c" > "$d/spec.c"
    fi
    n=$(diff "$c" "$d/spec.c" | grep -c '^>')
    if [ $v = 256 ]; then want=2; else want=3; fi
    [ "$n" = "$want" ] || bad "MoFang-$v: expected $want changed lines, got $n"
    gcc -O2 -w -I"$d" -o "$scratch/MoFang-$v-shipped" "$c" "$here/reproduce_spec_mismatch.c" || bad "MoFang-$v build failed"
    gcc -O2 -w -I"$d" -o "$scratch/MoFang-$v-spec" "$d/spec.c" "$here/reproduce_spec_mismatch.c" || bad "MoFang-$v spec build failed"
    echo "MoFang-$v, KAT_2_12_MoFang_$v.txt ($n transposition lines changed for the spec reading):"
    use_kat "$d/output/KAT_2_12_MoFang_$v.txt" "MoFang-$v/KAT_2_12_MoFang_$v.txt" "$scratch/kat$v.txt"
    check MoFang-$v-shipped kat$v.txt "match=4097 total=4097 first_fail=-1 fails=none"
    check MoFang-$v-spec kat$v.txt "match=0 total=4097 first_fail=0 fails=4097(list omitted)"
done
echo "the shipped key expansion reproduces all KAT_2_12 vectors of MoFang-256 and MoFang-768; Algorithms 3 and 4 as printed reproduce none"
echo "ATTACK $id-4: MoFang key expansion exchanges the block transpositions of Algorithms 3 and 4 (spec reading fails 4097/4097 KATs): CONFIRMED"
exit 0
