#!/bin/sh
# hash-35-5: Wish encodes the block-index tweak little-endian, while the
# specification defines it big-endian.
#
# Spec section 3.3.2 (p. 6, printed 5), Algorithm 2 line 5 and section 4.3:
# tau_i = enc64(i) || 0^64 with enc64 "the big-endian 64-bit encoding of the
# block index", XORed into lane L_{l/2} at the start of every step. The
# reference wish.c:177-179 (identical in Wish512_reference and
# Wish1024_reference) writes tweak[b] = (byte)(blk >> (b * 8)), i.e. the least
# significant byte of the index first.
#
# The KAT_2_12 reference files are regenerated, not taken from the original
# archive: gen_KAT_2_12() of the shipped NGCC generator KAT_CryptHash.c is
# compiled against the unmodified reference sources, and its output is used
# only if its canonical SHA-256 (api/README.md: CR and trailing blanks
# stripped, empty lines dropped) equals the "strict" entry for that file in
# the tracked hash-35/kat.sha256, the digest of the official reference file.
#
# The script builds Wish512 and Wish1024 twice: as shipped (control: all 4097
# KAT_2_12 vectors) and with only that line changed to the big-endian
# tweak[7 - b] (must fail from the first two-block message on, i.e. from
# Msg_Len = 512 resp. 1024, and still match every single-block message).
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
id=hash-35
for t in gcc sha256sum awk; do
    command -v $t >/dev/null 2>&1 || { echo "SKIP $id-5: $t unavailable"; exit 77; }
done
bad() { echo "NOT CONFIRMED $id-5: $1"; exit 1; }
man=$id/kat.sha256
[ -f "$man" ] || bad "$man (tracked KAT manifest) missing"
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM

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

for v in 512 1024; do
    src=$id/Wish/Implementations/Reference_Implementation/Wish${v}_reference
    d=$scratch/w$v
    mkdir -p "$d" || exit 2
    cp "$src/CryptHash_AlgorithmInstance.c" "$src/CryptHash_AlgorithmInstance.h" "$src/wish.c" "$src/wish.h" \
       "$src/drng.c" "$src/drng.h" "$src/KAT_CryptHash.c" "$d/" || bad "Wish$v sources missing"
    echo "Wish$v, KAT_2_12_Wish$v.txt:"
    gcc -O2 -w -I"$d" -o "$d/katgen" "$scratch/katgen.c" "$d/CryptHash_AlgorithmInstance.c" "$d/wish.c" "$d/drng.c" || bad "Wish$v KAT generator build failed"
    (cd "$d" && ./katgen) || bad "Wish$v KAT_2_12 generation failed"
    use_kat "$d/output/KAT_2_12_Wish$v.txt" "Wish$v/KAT_2_12_Wish$v.txt" "$scratch/kat$v.txt"
    grep -q 'tweak\[b\] = (byte)(blk >> (b \* 8));' "$d/wish.c" || bad "Wish$v tweak line differs from the expected shipped line"
    sed 's/tweak\[b\] = (byte)(blk >> (b \* 8));/tweak[7 - b] = (byte)(blk >> (b * 8));/' "$d/wish.c" > "$d/wish_be.c"
    n=$(diff "$d/wish.c" "$d/wish_be.c" | grep -c '^>')
    [ "$n" = 1 ] || bad "Wish$v: expected 1 changed line, got $n"
    gcc -O2 -w -I"$d" -o "$scratch/shipped" "$d/CryptHash_AlgorithmInstance.c" "$d/wish.c" "$d/drng.c" "$here/reproduce_spec_mismatch.c" || bad "Wish$v build failed"
    gcc -O2 -w -I"$d" -o "$scratch/spec" "$d/CryptHash_AlgorithmInstance.c" "$d/wish_be.c" "$d/drng.c" "$here/reproduce_spec_mismatch.c" || bad "Wish$v spec build failed"
    out=$("$scratch/shipped" "$scratch/kat$v.txt") || bad "Wish$v run failed"
    echo "  shipped (little-endian tweak): $out"
    [ "$out" = "match=4097 total=4097 first_fail=-1 fails=none" ] || bad "Wish$v shipped: $out"
    out=$("$scratch/spec" "$scratch/kat$v.txt") || bad "Wish$v spec run failed"
    echo "  spec (big-endian enc64 tweak): $out"
    [ "$out" = "match=$v total=4097 first_fail=$v fails=$((4097 - v))(list omitted)" ] || bad "Wish$v spec: $out"
done
echo "as shipped both instances reproduce all 4097 vectors; with the big-endian tweak of section 3.3.2 every multi-block vector fails (Msg_Len >= 512 / 1024) and every single-block vector still matches"
echo "ATTACK $id-5: Wish block-index tweak is encoded little-endian, not big-endian as specified (spec reading fails all multi-block KATs): CONFIRMED"
exit 0
