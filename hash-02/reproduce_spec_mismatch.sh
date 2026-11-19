#!/bin/sh
# hash-02-8: AXIS pads with 64 fewer zero bits than specified when
# len(M) mod 64 = 62.
#
# Spec section 2.2 (p. 8): Pad10^{s+1}1(M) = M || 1 || 0^{s+1} || 1 ||
# Binary64(len(M)) with s = (-len(M) - 3) mod 64, i.e. 1..64 zeros
# (Algorithm 1 line 2). The reference axis_core.c (identical in AXIS-512, -768
# and -1024) uses zero_count = (64 - ((len + 2) & 63)) & 63, i.e. 0..63 zeros,
# at :1050, :1096, :1306, :1401 and :1465. The counts differ only when
# len mod 64 = 62 (spec 64 zeros, code none).
#
# The KAT_2_12 reference files are regenerated, not taken from the original
# archive: gen_KAT_2_12() of the shipped NGCC generator KAT_CryptHash.c is
# compiled against the unmodified reference sources, and its output is used
# only if its canonical SHA-256 (api/README.md: CR and trailing blanks
# stripped, empty lines dropped) equals the "strict" entry for that file in
# the tracked hash-02/kat.sha256, the digest of the official reference file.
#
# The script builds the three reference instances twice: as shipped (control:
# all 4097 KAT_2_12 vectors) and with only the zero count set to s + 1 at the
# five sites (must fail exactly the 64 vectors with Msg_Len mod 64 = 62).
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
id=hash-02
for t in gcc sha256sum awk; do
    command -v $t >/dev/null 2>&1 || { echo "SKIP $id-8: $t unavailable"; exit 77; }
done
bad() { echo "NOT CONFIRMED $id-8: $1"; exit 1; }
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
expect62=$(awk 'BEGIN { s = ""; for (l = 62; l <= 4096; l += 64) s = s (s == "" ? "" : ",") l; print s }')

for v in 512 768 1024; do
    src=$id/AXIS/Implementations/Reference_Implementation/AXIS-$v
    d=$scratch/a$v
    mkdir -p "$d" || exit 2
    cp "$src"/*.c "$src"/*.h "$d/" || bad "AXIS-$v sources missing"
    echo "AXIS-$v, KAT_2_12_AXIS-$v.txt:"
    gcc -O2 -w -I"$d" -o "$d/katgen" "$scratch/katgen.c" "$d/CryptHash_AlgorithmInstance.c" "$d/axis_core.c" "$d/drng.c" || bad "AXIS-$v KAT generator build failed"
    (cd "$d" && ./katgen) || bad "AXIS-$v KAT_2_12 generation failed"
    use_kat "$d/output/KAT_2_12_AXIS-$v.txt" "AXIS-$v/KAT_2_12_AXIS-$v.txt" "$scratch/kat$v.txt"
    old='const uint64_t zero_count = (64ULL - ((message_bits + 2ULL) \& 63ULL)) \& 63ULL;'
    [ "$(grep -c "$old" "$d/axis_core.c")" = 5 ] || bad "AXIS-$v zero_count sites differ from the expected shipped code"
    sed "s/$old/const uint64_t zero_count = ((64ULL - ((message_bits + 3ULL) \& 63ULL)) \& 63ULL) + 1ULL;/" "$d/axis_core.c" > "$d/axis_core_spec.c"
    [ "$(grep -c 'message_bits + 3ULL) & 63ULL)) & 63ULL) + 1ULL;' "$d/axis_core_spec.c")" = 5 ] || bad "AXIS-$v spec zero count not applied"
    gcc -O2 -w -I"$d" -o "$scratch/shipped" "$d/CryptHash_AlgorithmInstance.c" "$d/axis_core.c" "$d/drng.c" "$here/reproduce_spec_mismatch.c" || bad "AXIS-$v build failed"
    gcc -O2 -w -I"$d" -o "$scratch/spec" "$d/CryptHash_AlgorithmInstance.c" "$d/axis_core_spec.c" "$d/drng.c" "$here/reproduce_spec_mismatch.c" || bad "AXIS-$v spec build failed"
    out=$("$scratch/shipped" "$scratch/kat$v.txt") || bad "AXIS-$v run failed"
    echo "  shipped ((-len-2) mod 64 zeros): $out"
    [ "$out" = "match=4097 total=4097 first_fail=-1 fails=none" ] || bad "AXIS-$v shipped: $out"
    out=$("$scratch/spec" "$scratch/kat$v.txt") || bad "AXIS-$v spec run failed"
    echo "  spec (s+1 zeros, s = (-len-3) mod 64): $out"
    [ "$out" = "match=4033 total=4097 first_fail=62 fails=$expect62" ] || bad "AXIS-$v spec: $out"
done
echo "as shipped all three instances reproduce all 4097 vectors; with the specified s+1 zeros exactly the 64 vectors with Msg_Len mod 64 = 62 fail in each file"
echo "ATTACK $id-8: AXIS padding omits the 64 specified zero bits when len(M) mod 64 = 62 (spec reading fails exactly those 64 KATs per instance): CONFIRMED"
exit 0
