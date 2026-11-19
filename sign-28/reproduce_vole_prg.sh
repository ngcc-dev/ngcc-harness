#!/bin/sh
# sign-28-7: at lambda = 160 the VOLE tapes are not the PRG output of
# Algorithm 8.
#
# Algorithm 20 (spec physical p. 27) sets each leaf's tape r_0,i := PRG(sd_i, iv, twk;
# l_v), and Algorithm 8 (p. 19) returns s_0 || s_1 || ..., the first lambda =
# 160 bits of each Rijndael-192 output block. The reference code fills the tape
# in 128-bit rows: prg_output_parts returns 1 for a 20-byte lambda, so row r
# uses counter r and keeps only the first 128 bits of that block
# (src/vole_impl.inc). The tape departs from Algorithm 8 at bit 128.
#
# The KAT files are regenerated, not taken from the original archive: the
# shipped NGCC generator test/KAT_SIG.c is compiled against the unmodified
# reference sources, and its output is used only if its canonical SHA-256 (CR
# and trailing blanks stripped, empty lines dropped) equals the "strict" entry
# in the tracked sign-28/kat.sha256, the digest of the official file.
#
# The script builds the reference SYDO-160f and -160s twice: as shipped
# (control: must reproduce the KAT file) and with only the row function
# replaced by Algorithm 8's byte stream (reproduce_vole_prg.c). The KAT
# generator verifies every signature it writes, so both builds must complete;
# the keys must be unchanged and every signature must differ.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
id=sign-28
for t in gcc sha256sum awk; do
    command -v $t >/dev/null 2>&1 || { echo "SKIP $id-7: $t unavailable"; exit 77; }
done
bad() { echo "NOT CONFIRMED $id-7: $1"; exit 1; }
man=$id/kat.sha256
[ -f "$man" ] || bad "$man (tracked KAT manifest) missing"
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
ref=$id/Implementations/Reference_Implementation

canon_sha() { awk '{ sub(/[\r \t]+$/, ""); if ($0 != "") print }' "$1" | sha256sum | cut -c1-64; }
# the archived Makefile's flags (identical in both trees); checked against the
# Makefile when one is present (the harness copy has none)
mcflags='-O3 -DNDEBUG -std=c11 -Wall -Wextra -Werror=implicit-function-declaration'
mcpp1='CPPFLAGS += -I. -Isrc -Ilib -Ilib/blake2/ref \'
mcpp2='	-DHAVE_SYS_RANDOM_H -DHAVE_GETRANDOM -DSYDO_REF_NGCC'
flags='-O3 -DNDEBUG -std=c11 -I. -Isrc -Ilib -Ilib/blake2/ref -DHAVE_SYS_RANDOM_H -DHAVE_GETRANDOM -DSYDO_REF_NGCC'
srcs='SIG_AlgorithmInstance.c src/aes.c src/bavc.c src/compat.c src/fields.c src/randomness.c src/sydo.c src/field.c src/hash.c src/macs.c src/primitives.c src/quicksilver.c src/rsd.c src/vole_check.c src/universal_hashing.c src/utils.c src/vole.c lib/auxfunc.c lib/drng.c lib/blake2/ref/blake2b-ref.c test/KAT_SIG.c'
call='    eval_vole_round_key(params, raw[i], &round_keys[key_index], iv, tweak, counter);'
anchor='static bool process_output_parts_round_keys(const sydo_ref_paramset_t* params, bool ignore_0,'

# gen DIR: build kat_sig in DIR and write DIR/output/KAT_SIG_<set>.txt
gen() {
    (cd "$1" && gcc $flags -w -o kat_sig $srcs && ./kat_sig) >"$1.log" 2>&1
}
# field FILE NAME: the NAME lines of FILE, in order
field() { sed -n "s/^$2 = //p" "$1"; }

for v in 160f 160s; do
    d=$ref/sydo_$v
    [ -d "$d" ] || bad "$d not found"
    if [ -f "$d/Makefile" ]; then
        grep -Fqx "CFLAGS += $mcflags" "$d/Makefile" && grep -Fqx "$mcpp1" "$d/Makefile" &&
            grep -Fqx "$mcpp2" "$d/Makefile" || bad "sydo_$v Makefile flags differ from the archived ones"
    fi
    inc=$d/src/vole_impl.inc
    grep -Fqx '  if (lambda_bytes > SYDO_REF_VOLE_BLOCK_BYTES &&' "$inc" &&
        grep -Fqx '        const uint32_t counter = (uint32_t)(row / parts);' "$inc" &&
        [ "$(grep -Fxc "$call" "$inc")" = 1 ] && [ "$(grep -Fxc "$anchor" "$inc")" = 1 ] ||
        bad "sydo_$v src/vole_impl.inc differs from the archive"

    cp -R "$d" "$scratch/ship$v" && cp -R "$d" "$scratch/spec$v" || exit 2
    awk -v f="$here/reproduce_vole_prg.c" -v a="$anchor" -v c="$call" '
        $0 == a { while ((getline l < f) > 0) print l }
        $0 == c { sub(/eval_vole_round_key/, "spec_eval_vole_round_key") }
        { print }' "$inc" > "$scratch/spec$v/src/vole_impl.inc" || exit 2
    [ "$(grep -c 'spec_eval_vole_round_key(params, raw' "$scratch/spec$v/src/vole_impl.inc")" = 1 ] ||
        bad "sydo_$v patch did not apply"

    gen "$scratch/ship$v" || { tail -5 "$scratch/ship$v.log"; bad "shipped sydo_$v KAT generation failed"; }
    gen "$scratch/spec$v" || { tail -5 "$scratch/spec$v.log"; bad "Algorithm 8 sydo_$v KAT generation failed"; }
    ship=$scratch/ship$v/output/KAT_SIG_sydo_$v.txt
    spec=$scratch/spec$v/output/KAT_SIG_sydo_$v.txt
    kwant=$(awk -v n="sydo_$v/KAT_SIG_sydo_$v.txt" '$2 == "strict" && $3 == n { c++; h = $1 } END { if (c == 1) print h }' "$man")
    [ -n "$kwant" ] || bad "no unique strict entry for sydo_$v in $man"
    kgot=$(canon_sha "$ship")
    [ "$kgot" = "$kwant" ] || bad "shipped sydo_$v KAT has canonical SHA-256 $kgot, $man records $kwant"
    echo "  shipped sydo_$v: regenerated KAT matches $man ($kwant)"

    n=$(field "$ship" Sn | wc -l)
    for f in PK SK Sn; do
        field "$ship" $f > "$scratch/a" && field "$spec" $f > "$scratch/b" || exit 2
        eval "same_$f=\$(paste -d' ' \"\$scratch/a\" \"\$scratch/b\" | awk '\$1 == \$2' | wc -l)"
    done
    echo "  Algorithm 8 sydo_$v: $n of $n signatures verified by the generator; keys equal in $same_PK/$n (PK) and $same_SK/$n (SK); signatures equal in $same_Sn/$n"
    [ "$n" = 10 ] && [ "$same_PK" = 10 ] && [ "$same_SK" = 10 ] && [ "$same_Sn" = 0 ] ||
        bad "sydo_$v Algorithm 8 build behaves differently"
done
echo "the shipped lambda = 160 reference reproduces both official KAT files; with the VOLE tape set to Algorithm 8's PRG output, keys are unchanged and none of the 20 signatures matches"
echo "ATTACK $id-7: at lambda = 160 the VOLE tapes keep 128 of every 160 PRG bits, so the implementation differs from Algorithm 8: CONFIRMED"
exit 0
