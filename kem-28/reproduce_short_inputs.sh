#!/bin/sh
# kem-28-3: kem_enc and kem_dec discard the declared public-key, secret-key
# and ciphertext lengths, so short inputs are read out of bounds.
#
# Checks that neither function uses its input lengths in all nine trees, then
# builds each with AddressSanitizer. An honest ciphertext copied into a heap
# buffer of exactly 16 bytes, or one byte short, must give a heap-buffer-
# overflow under kem_dec; an honest public key one byte short must give one
# under kem_enc. Controls: full-length inputs run cleanly and decapsulate to
# the encapsulated key. Source lists follow each tree's Makefile (checked when
# present; the harness copy has none). Appending bytes to a ciphertext is not
# tested: such aliases are not findings.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
id=kem-28-3
command -v gcc >/dev/null 2>&1 || { echo "SKIP $id: gcc unavailable"; exit 77; }
bad() { echo "NOT CONFIRMED $id: $1"; exit 1; }

impl=kem-28/Implementations
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
ASAN_OPTIONS=detect_leaks=0
export ASAN_OPTIONS
printf 'int main(void){return 0;}\n' >"$scratch/t.c"
gcc -fsanitize=address -o "$scratch/t" "$scratch/t.c" >/dev/null 2>&1 && "$scratch/t" ||
    { echo "SKIP $id: AddressSanitizer unavailable"; exit 77; }

body() { # file function: the function body after its parameter list
    awk -v f="$2" '$0 ~ "^int "f"\\(" {s=1} s && /^\{/ {b=1} b {print} b && /^\}/ {exit}' "$1"
}
srcs() { # tree set
    case "$1" in
        Optimized)
            s='auxfunc.c drng.c poly.c symmetric.c avx2.c ntt_asm_consts.c asm/ntt_full.s asm/invntt_full.s asm/baseinv_full.s asm/basemul_full.s asm/baseadd_full.s'
            [ "$2" = 648 ] || s="$(echo "$s" | sed 's/poly.c /poly.c ntt.c /')" ;;
        *) s='auxfunc.c drng.c poly.c ntt.c symmetric.c' ;;
    esac
    echo "$s"
}
trees='Reference Additional'
grep -qw avx2 /proc/cpuinfo 2>/dev/null && trees="$trees Optimized"
for t in Reference Additional Optimized; do
    for v in 648 1296 2592; do
        d="$impl/${t}_Implementation/OAEP-NTRU-$v"
        f="$d/KEM_AlgorithmInstance.c"
        body "$f" kem_dec | grep -q 'poly_frombytes(&p_c, ct);' && ! body "$f" kem_dec | grep -q 'ct_len_bytes\|sk_len_bytes' &&
            body "$f" kem_enc | grep -q 'pk' && ! body "$f" kem_enc | grep -q 'pk_len_bytes' ||
            bad "$t OAEP-NTRU-$v wrapper differs from the archive"
        if [ -f "$d/Makefile" ]; then
            [ "$(tr -d '\r' <"$d/Makefile" | sed -n 's/^SOURCES= *//p' | tr -s ' ' | sed 's/ *$//')" = "$(srcs $t $v)" ] ||
                bad "$t OAEP-NTRU-$v Makefile sources differ from the archived list"
        fi
    done
done
echo 'source: all nine kem_dec/kem_enc wrappers ignore ct_len_bytes, sk_len_bytes and pk_len_bytes'

for t in $trees; do
    for v in 648 1296 2592; do
        x=''
        [ $t = Optimized ] && x='-mavx2'
        [ $t = Optimized ] && [ $v = 2592 ] && x='-mavx2 -DOAEP2592_ASM_NTT'
        (cd "$impl/${t}_Implementation/OAEP-NTRU-$v" && gcc -O1 -g -fsanitize=address -fno-omit-frame-pointer -w $x -I. \
            -o "$scratch/$t-$v" "$here/reproduce_short_inputs.c" KEM_AlgorithmInstance.c $(srcs $t $v)) \
            >"$scratch/$t-$v.log" 2>&1 &
    done
done
wait

overflow() { # binary mode len api
    "$1" "$2" "$3" >"$scratch/out" 2>&1 && return 1
    # SIMD loads that straddle the end are reported as unknown-crash; both
    # reports place the first bad byte 0 bytes after the short buffer.
    grep -Eq 'ERROR: AddressSanitizer: (heap-buffer-overflow|unknown-crash)' "$scratch/out" &&
        grep -q "is located 0 bytes after $3-byte region" "$scratch/out" &&
        grep -q "in $4 .*KEM_AlgorithmInstance.c" "$scratch/out"
}
n=0
for t in $trees; do
    for v in 648 1296 2592; do
        b="$scratch/$t-$v"
        [ -x "$b" ] || { cat "$b.log"; bad "$t OAEP-NTRU-$v build failed"; }
        out=$("$b" ct full 2>&1) && ctl=$(echo "$out" | sed -n 's/^ct_len=\([0-9]*\) of .*/\1/p') &&
            case "$out" in *"dec=0 agrees=1") ;; *) false ;; esac || bad "$t OAEP-NTRU-$v ciphertext control: $out"
        out=$("$b" pk full 2>&1) && pkl=$(echo "$out" | sed -n 's/^pk_len=\([0-9]*\) of .*/\1/p') &&
            case "$out" in *"enc=0") ;; *) false ;; esac || bad "$t OAEP-NTRU-$v public-key control: $out"
        for len in 16 $((ctl - 1)); do
            overflow "$b" ct $len kem_dec || { cat "$scratch/out"; bad "$t OAEP-NTRU-$v: no overflow for a $len-byte ciphertext"; }
        done
        overflow "$b" pk $((pkl - 1)) kem_enc || { cat "$scratch/out"; bad "$t OAEP-NTRU-$v: no overflow for a $((pkl - 1))-byte public key"; }
        echo "$t OAEP-NTRU-$v: ct $ctl and pk $pkl bytes clean; 16 and $((ctl - 1))-byte ciphertexts and a $((pkl - 1))-byte public key read out of bounds"
        n=$((n + 1))
    done
done
case "$trees" in *Optimized*) ;; *) echo 'optimized trees not run: CPU lacks AVX2 (source checks above still cover them)' ;; esac
echo "$n builds: short ciphertexts and public keys are read out of bounds; full-length controls are clean"
echo "ATTACK $id: kem_enc and kem_dec ignore the declared input lengths and over-read short buffers: CONFIRMED"
exit 0
