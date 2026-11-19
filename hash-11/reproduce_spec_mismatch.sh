#!/bin/sh
# hash-11-4: two places (block count and length injection) where
# the Garnet reference code computes a different function from the
# specification's definition.
#
# 1. Block count. Eq. (1) (section 2.2) pads M to m = floor(mlength/128) + 1
#    128-bit words and eq. (2) (section 2.5) groups them into w-bit blocks, the
#    last one holding the last padded word; so ceil(128m/w) blocks are absorbed.
#    The code counts byte_num = ceil(mlength/8) message bytes, absorbs
#    floor(byte_num/W) full blocks and then ALWAYS absorbs one more block. When
#    mlength mod r is in {r-7..r-1} the padding bit already sits in the last
#    full block and the extra block is all zero (one extra P^b call).
#    Spec reading: skip that final block exactly when the padded message is
#    already complete ((mlength % 8) != 0 and byte_num % W == 0).
# 2. Length injection. Section 2.5: "mlength is injected into the rate part in
#    the same serialized convention" (section 2.2: the first 128-bit word goes
#    to the lowest-numbered rate word). The code XORs the same 128-bit word
#    (low half = mlength) into S0, S5, S10 and S15 (absorb_message_4x4).
#    Spec reading: XOR it into S0 only.
#
# Every reference variant (Garnet-512 at rates 512/640/768/896/1024,
# Garnet-768, Garnet-1024a) is built three times: as shipped (control: all
# 4097 KAT_2_12 vectors), with reading 1 (must fail exactly at the boundary
# lengths) and with reading 2 (must match only Msg_Len = 0).
#
# The KAT_2_12 reference files are regenerated, not taken from the original
# archive: gen_KAT_2_12() of the shipped NGCC generator KAT_CryptHash.c is
# compiled against the unmodified reference sources of each variant, and its
# output is used only if its canonical SHA-256 (api/README.md: CR and trailing
# blanks stripped, empty lines dropped) equals the "strict" entry for that
# file in the tracked hash-11/kat.sha256, the digest of the official reference
# file. The single Garnet tree defines ALGORITHM_INSTANCE "Garnet" and
# DIGEST_BIT_LENGTH 512 for every variant, so the KAT name and digest length
# are passed to gen_KAT_2_12() explicitly, as hash-11/Makefile does.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
id=hash-11
for t in gcc sha256sum awk; do
    command -v $t >/dev/null 2>&1 || { echo "SKIP $id-4: $t unavailable"; exit 77; }
done
bad() { echo "NOT CONFIRMED $id-4: $1"; exit 1; }
man=$id/kat.sha256
[ -f "$man" ] || bad "$man (tracked KAT manifest) missing"
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
G=$id/Garnet/Implementations/API_CryptHash/Implementations/Reference_Implementation/Garnet
mkdir -p "$scratch/src" "$scratch/v" || exit 2
cp "$G"/*.c "$G"/*.h "$scratch/src/" || bad "sources missing"
S=$scratch/src

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
# the shipped generator, reduced to gen_KAT_2_12(NAME, BITS); it writes
# output/KAT_2_12_NAME.txt in the working directory
cat > "$scratch/katgen.c" <<'EOF'
#define main ngcc_kat_main
#include "KAT_CryptHash.c"
#undef main
int main(int argc, char **argv) { return argc != 3 || gen_KAT_2_12(argv[1], atoi(argv[2])) != KAT_SUCCESS; }
EOF

# reading 1: insert the guard before the final-block absorption and a label
# before the length injection; nothing else changes
count_variant() { # file W out
    awk -v W="$2" '
    NR == FNR {
        if (index($0, "msglen.v[1] = 0")) L = FNR
        if (index($0, "zero128(msg + j)")) Z[++nz] = FNR
        if ((index($0, "absorb_message") && index($0, "(state, msg);")) || index($0, "keep_capacity5_4x4_st(state, cap);")) A[++na] = FNR
        next
    }
    FNR == 1 {
        z = 0; for (i = 1; i <= nz; i++) if (Z[i] < L) z = Z[i]
        f = 0; for (i = 1; i <= na; i++) if (A[i] > z && f == 0) f = A[i]
    }
    FNR == f { print "    if ((mlength % 8) != 0 && byte_num > 0 && (byte_num % " W ") == 0) goto ngcc_skip;" }
    FNR == L { print "    ngcc_skip:;" }
    { print }' "$1" "$1" > "$3"
    [ "$(grep -c 'goto ngcc_skip;' "$3")" = 1 ] && [ "$(grep -c 'ngcc_skip:;' "$3")" = 1 ] &&
        [ "$(wc -l < "$3")" -le "$(($(wc -l < "$1") + 3))" ] || bad "$1: block-count guard not inserted"
}
# reading 2: the length word goes to S0 only
s0_variant() { # file out
    grep -c 'absorb_message_4x4\(_st\)\{0,1\}(state, msglen);' "$1" | grep -qx 1 || bad "$1: length-injection call differs from the expected shipped code"
    sed 's/absorb_message_4x4\(_st\)\{0,1\}(state, msglen);/state[0] = xor128(state[0], msglen);/' "$1" > "$2"
    [ "$(grep -c 'state\[0\] = xor128(state\[0\], msglen);' "$2")" = 1 ] || bad "$1: length-injection line not changed"
}
# Msg_Len values 0..4096 with mlength mod r in {r-7..r-1}
boundaries() { awk -v r="$1" 'BEGIN { s = ""; for (l = 0; l <= 4096; l++) if (l % r >= r - 7) s = s (s == "" ? "" : ",") l; print s }'; }

total9=0; total10=0
# label  patched-file  W  rate  KAT name  (512-file, 768-file, 1024-file roles)
for spec in Cap512:Garnet_512_512.c:64:512:Garnet_512_Cap512 Cap640:Garnet_512_640.c:80:640:Garnet_512_Cap640 \
            Cap768:Garnet_512_768.c:96:768:Garnet_512_Cap768 Cap896:Garnet_512_896.c:112:896:Garnet_512_Cap896 \
            Cap1024:Garnet_512.c:128:1024:Garnet_512_Cap1024 G768:Garnet_768.c:64:512:Garnet_768 \
            G1024:Garnet_1024.c:112:896:Garnet_1024; do
    IFS=: read -r label file W r kat <<EOT
$spec
EOT
    case $label in G768) bits=768 ;; G1024) bits=1024 ;; *) bits=512 ;; esac
    case $file in Garnet_768.c|Garnet_1024.c) g512=$S/Garnet_512.c ;; *) g512=$S/$file ;; esac
    g=$scratch/gen/$label
    mkdir -p "$g" || exit 2
    gcc -O2 -w -I"$S" -o "$g/katgen" "$scratch/katgen.c" "$S/CryptHash_Garnet.c" "$g512" "$S/Garnet_768.c" "$S/Garnet_1024.c" \
        "$S/drng.c" || bad "$label KAT generator build failed"
    (cd "$g" && ./katgen "$kat" "$bits") || bad "$label KAT_2_12 generation failed"
    use_kat "$g/output/KAT_2_12_$kat.txt" "$kat/KAT_2_12_$kat.txt" "$scratch/kat.txt"
    count_variant "$S/$file" "$W" "$scratch/v/count_$file"
    s0_variant "$S/$file" "$scratch/v/s0_$file"
    for v in shipped count s0; do
        f512=$S/Garnet_512.c; f768=$S/Garnet_768.c; f1024=$S/Garnet_1024.c
        if [ $v = shipped ]; then p=$S/$file; else p=$scratch/v/${v}_$file; fi
        case $file in
            Garnet_768.c) f768=$p ;;
            Garnet_1024.c) f1024=$p ;;
            *) f512=$p ;;
        esac
        gcc -O2 -w -I"$S" -o "$scratch/bin" "$S/CryptHash_Garnet.c" "$f512" "$f768" "$f1024" "$S/drng.c" \
            "$here/reproduce_spec_mismatch.c" || bad "$label $v build failed"
        out=$("$scratch/bin" "$scratch/kat.txt") || bad "$label $v run failed"
        echo "  $kat r=$r $v: $out"
        case $v in
            shipped) want="match=4097 total=4097 first_fail=-1 fails=none" ;;
            count)   b=$(boundaries "$r"); n=$(echo "$b" | tr ',' '\n' | wc -l | tr -d ' ')
                     want="match=$((4097 - n)) total=4097 first_fail=${b%%,*} fails=$b"; total9=$((total9 + n)) ;;
            s0)      want="match=1 total=4097 first_fail=1 fails=4096(list omitted)"; total10=$((total10 + 4096)) ;;
        esac
        [ "$out" = "$want" ] || bad "$label $v: expected '$want'"
    done
done
echo "as shipped all 7 reference variants reproduce their 4097 KAT_2_12 vectors"
echo "with the specified block count exactly the $total9 vectors with mlength mod r in {r-7..r-1} fail (all others match)"
echo "with the length injected into S0 only, $total10 of 7*4097 vectors fail (only Msg_Len = 0 matches)"
echo "ATTACK $id-4: Garnet absorbs an extra all-zero block when mlength mod r is in {r-7..r-1}, and injects mlength into S0, S5, S10 and S15 instead of the serialized rate position (each specified reading fails its KATs): CONFIRMED"
exit 0
