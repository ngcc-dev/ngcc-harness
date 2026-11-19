#!/bin/sh
# hash-32-4: ZC-EDMC uses six of the twelve Table 1.1 round constants in a
# different order.
#
# Table 1.1 (shared by ZC-1280 and ZC-1536) gives c_-7, c_-6, c_-5 =
# 0x60, 0x120, 0x14 and c_-3, c_-2, c_-1 = 0xF0, 0x1A0, 0x380; iota XORs c_i
# into A0[0] in round i = -11..0 (Algorithms 1 and 2). The shared permutation
# headers lib/low/ZuD-1280/ZuD1280.h:25-36 and ZuD-1536/ZuD1536.h:25-36 define
# rc8, rc7, rc6 = 0x120, 0x14, 0x60 and rc4, rc3, rc2 = 0x380, 0xF0, 0x1A0
# (the Xoodoo order), and Permute_12rounds applies rc12 .. rc1, so rcN plays
# the role of c_(1-N).
#
# The KAT_2_12 reference files are regenerated, not taken from the original
# archive: gen_KAT_2_12() of the shipped NGCC generator KAT_CryptHash.c is
# compiled against the unmodified reference sources, and its output is used
# only if its canonical SHA-256 (api/README.md: CR and trailing blanks
# stripped, empty lines dropped) equals the "strict" entry for that file in
# the tracked hash-32/kat.sha256, the digest of the official reference file.
#
# The script builds all six reference instances twice: as shipped (control:
# all 4097 KAT_2_12 vectors) and with only those six #defines per header put
# in the Table 1.1 order (must reproduce none).
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
id=hash-32
name=ZC-EDMC
for t in gcc sha256sum awk; do
    command -v $t >/dev/null 2>&1 || { echo "SKIP $id-4: $t unavailable"; exit 77; }
done
bad() { echo "NOT CONFIRMED $id-4: $1"; exit 1; }
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

cp -R "$id/$name/Implementations" "$scratch/shipped" || bad "sources missing"
cp -R "$id/$name/Implementations" "$scratch/spec" || exit 2
for w in 1280 1536; do
    h=$scratch/spec/lib/low/ZuD-$w/ZuD$w.h
    for pair in rc8:0x0000000000000120ULL rc7:0x0000000000000014ULL rc6:0x0000000000000060ULL \
                rc4:0x0000000000000380ULL rc3:0x00000000000000F0ULL rc2:0x00000000000001A0ULL; do
        grep -q "^#define ZUD${w}_${pair%%:*}  *${pair#*:}" "$h" || bad "ZuD$w.h ${pair%%:*} differs from the expected shipped value"
    done
    sed -e "s/^\(#define ZUD${w}_rc8 *\)0x0000000000000120ULL/\10x0000000000000060ULL/" \
        -e "s/^\(#define ZUD${w}_rc7 *\)0x0000000000000014ULL/\10x0000000000000120ULL/" \
        -e "s/^\(#define ZUD${w}_rc6 *\)0x0000000000000060ULL/\10x0000000000000014ULL/" \
        -e "s/^\(#define ZUD${w}_rc4 *\)0x0000000000000380ULL/\10x00000000000000F0ULL/" \
        -e "s/^\(#define ZUD${w}_rc3 *\)0x00000000000000F0ULL/\10x00000000000001A0ULL/" \
        -e "s/^\(#define ZUD${w}_rc2 *\)0x00000000000001A0ULL/\10x0000000000000380ULL/" \
        "$scratch/shipped/lib/low/ZuD-$w/ZuD$w.h" > "$h"
    n=$(diff "$scratch/shipped/lib/low/ZuD-$w/ZuD$w.h" "$h" | grep -c '^>')
    [ "$n" = 6 ] || bad "ZuD$w.h: expected 6 changed constants, got $n"
done
echo "spec reading: c_-11..c_0 = 58 38 3C0 D0 60 120 14 2C F0 1A0 380 12 (Table 1.1); shipped: 58 38 3C0 D0 120 14 60 2C 380 F0 1A0 12"

ok=0
for w in 1280 1536; do
    for dl in 512 768 1024; do
        inst=$name-$w-$dl
        R=$scratch/shipped
        d=$R/Implementations/Reference_Implementation/$inst
        g=$scratch/gen/$inst
        mkdir -p "$g" || exit 2
        gcc -O2 -w -I"$R/lib/common" -I"$d" -o "$g/katgen" "$scratch/katgen.c" "$d/CryptHash_AlgorithmInstance.c" "$d/drng.c" \
            "$R/lib/low/ZuD-$w/plain/ZuD$w-plain.c" || bad "$inst KAT generator build failed"
        (cd "$g" && ./katgen) || bad "$inst KAT_2_12 generation failed"
        use_kat "$g/output/KAT_2_12_$inst.txt" "$inst/KAT_2_12_$inst.txt" "$scratch/kat.txt"
        for v in shipped spec; do
            R=$scratch/$v
            d=$R/Implementations/Reference_Implementation/$inst
            gcc -O2 -w -I"$R/lib/common" -I"$d" -o "$scratch/$v.bin" "$d/CryptHash_AlgorithmInstance.c" "$d/drng.c" \
                "$R/lib/low/ZuD-$w/plain/ZuD$w-plain.c" "$here/reproduce_spec_mismatch.c" || bad "$inst $v build failed"
            out=$("$scratch/$v.bin" "$scratch/kat.txt") || bad "$inst $v run failed"
            echo "  $inst $v: $out"
            if [ $v = shipped ]; then want="match=4097 total=4097 first_fail=-1 fails=none"; else want="match=0 total=4097 first_fail=0 fails=4097(list omitted)"; fi
            [ "$out" = "$want" ] || bad "$inst $v: expected '$want'"
        done
        ok=$((ok + 1))
    done
done
echo "all $ok reference instances reproduce their KAT_2_12 files as shipped and none of the 4097 vectors with the Table 1.1 constant order"
echo "ATTACK $id-4: $name round constants c_-7..c_-5 and c_-3..c_-1 are applied in an order different from Table 1.1 (spec reading fails 4097/4097 KATs): CONFIRMED"
exit 0
