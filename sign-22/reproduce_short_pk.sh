#!/bin/sh
# sign-22-9: sig_verify discards pk_len_bytes, so a short public key is read
# out of bounds by unpack_pk.
#
# Checks the wrapper in all sixteen trees, then builds each with
# AddressSanitizer and passes sig_verify a heap public-key buffer allocated at
# exactly the declared length. The signature is a well-formed (c_tilde, z = 0)
# encoding from the submitted pack_sig, which passes decoding and the norm
# checks without any key. A 16-byte key must give an out-of-bounds READ of
# size 32 in unpack_pk (src/packing.c:40); a key one byte short must give a
# heap-buffer-overflow in unpack_pk as well (src/packing.c:44). Controls: the
# full-length buffer runs cleanly; at the 128-bit level an honest key pair and
# signature verify with the full-length key and overflow with the key one byte
# short.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-22-9: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-22-9: $1"; exit 1; }

impl='sign-22/Implementations and Test_Vectors/Implementations'
[ -d "$impl" ] || bad "implementation tree not found"
driver="$here/reproduce_short_pk.c"
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
ASAN_OPTIONS=detect_leaks=0
export ASAN_OPTIONS

printf 'int main(void){return 0;}\n' >"$scratch/t.c"
gcc -fsanitize=address -o "$scratch/t" "$scratch/t.c" >/dev/null 2>&1 && "$scratch/t" ||
    { echo 'SKIP sign-22-9: AddressSanitizer unavailable'; exit 77; }

avx2=0
grep -qw avx2 /proc/cpuinfo 2>/dev/null && avx2=1
trees='Reference'
[ $avx2 = 1 ] && trees='Reference Optimized'

for t in Reference Optimized; do
    for f in SM3 SHAKE; do
        for l in 128 256 384 512; do
            d="$impl/${t}_Implementation/Rhyme-$f/Rhyme-$f-$l"
            tr -d '\r' <"$d/SIG_AlgorithmInstance.c" | grep -Fxq '	(void)pk_len_bytes;' &&
                tr -d '\r' <"$d/src/packing.c" | grep -Fxq '    memcpy(seedA, pk, SEEDBYTES);' ||
                bad "$t $f-$l wrapper or unpack_pk differs from the archive"
        done
    done
done
echo 'source: all 16 sig_verify wrappers discard pk_len_bytes; unpack_pk reads CRYPTO_PUBLICKEYBYTES'

core='SIG_AlgorithmInstance.c src/poly.c src/ntt.c src/ntt_tables.c src/sampler.c src/encoding.c
      src/packing.c src/sign.c src/zpntt.c src/symmetric-shake.c rhyme_xof.c src/keygen/kg_main.c
      src/keygen/kg_solver.c src/keygen/kg_zint.c src/keygen/kg_ntt.c src/keygen/kg_primes.c
      randombytes.c auxfunc.c drng.c'
build() { # dir tree family level output
    extra=''; flags=''
    case $3 in SM3) extra='sm3.c sm3_xof.c' ;; SHAKE) extra='src/fips202.c' ;; esac
    case "$2$3" in
        OptimizedSM3) extra="$extra simd/sm3_avx2.c"
            # sm3.c calls rhyme_sm3_hash_avx2 without a prototype (an error since GCC 14)
            flags='-mavx2 -DUSE_AVX2_SM3 -Wno-error=implicit-function-declaration' ;;
        OptimizedSHAKE) flags='-march=native -DUSE_DP1_CHECK' ;;
    esac
    (cd "$1" && gcc -O1 -g -fsanitize=address -fno-omit-frame-pointer -std=c99 -w -I. -Iinclude \
        -Isrc/keygen -DRHYME_NO_AES -DRHYME_MODE="$4" -DOUTPUT_BLANK_TEST_VECTORS=0 $flags \
        -o "$5" "$driver" $core $extra -lm) >"$5.log" 2>&1
}
for t in $trees; do
    for f in SM3 SHAKE; do
        for l in 128 256 384 512; do
            build "$impl/${t}_Implementation/Rhyme-$f/Rhyme-$f-$l" "$t" "$f" "$l" "$scratch/$t-$f-$l" &
        done
    done
done
wait

pklen() { case $1 in 128) echo 800 ;; 256) echo 1824 ;; 384) echo 2720 ;; 512) echo 3872 ;; esac; }
overflow() { # binary mode len line-pattern
    "$1" "$2" "$3" >"$scratch/out" 2>&1 && return 1
    # -march=native builds may report the 32-byte memcpy as "unknown-crash";
    # both reports place the access 0 bytes after the short heap buffer.
    grep -Eq 'ERROR: AddressSanitizer: (heap-buffer-overflow|unknown-crash)' "$scratch/out" &&
        grep -Eq "is located 0 bytes after $3-byte region" "$scratch/out" &&
        grep -q "in rhyme[0-9]*_unpack_pk src/packing.c:$4" "$scratch/out" &&
        grep -q 'in sig_verify .*SIG_AlgorithmInstance.c:63' "$scratch/out"
}
n=0
for t in $trees; do
    for f in SM3 SHAKE; do
        for l in 128 256 384 512; do
            b="$scratch/$t-$f-$l"
            [ -x "$b" ] || { cat "$b.log"; bad "$t $f-$l build failed"; }
            full=$(pklen $l); short=$((full - 1))
            out=$("$b" keyless full 2>&1) || bad "$t $f-$l full-length control: $out"
            case "$out" in "pk_len=$full of $full sig_len="*"
verify=-1") ;; *) bad "$t $f-$l full-length control: $out" ;; esac
            overflow "$b" keyless 16 40 && grep -q 'READ of size 32' "$scratch/out" ||
                { cat "$scratch/out"; bad "$t $f-$l: no overflow for a 16-byte key"; }
            overflow "$b" keyless $short 44 ||
                { cat "$scratch/out"; bad "$t $f-$l: no overflow for a $short-byte key"; }
            msg="full=$full: verify=-1 clean; 16 bytes: READ of size 32 at packing.c:40; $short bytes: overflow in unpack_pk (packing.c:44)"
            if [ $l = 128 ]; then
                out=$("$b" honest full 2>&1) || bad "$t $f-$l honest control: $out"
                case "$out" in *"verify=0") ;; *) bad "$t $f-$l honest control: $out" ;; esac
                overflow "$b" honest $short 44 ||
                    { cat "$scratch/out"; bad "$t $f-$l: no overflow for an honest $short-byte key"; }
                msg="$msg; honest key: full verify=0, $short bytes overflow"
            fi
            echo "$t Rhyme-$f-$l: $msg"
            n=$((n + 1))
        done
    done
done
[ $avx2 = 1 ] || echo 'optimized trees not run: CPU lacks AVX2 (source checks above still cover them)'
echo "$n builds: an exact-length short public key is read out of bounds; full-length controls are clean"
echo "ATTACK sign-22-9: sig_verify ignores pk_len_bytes and unpack_pk over-reads a short heap public key: CONFIRMED"
exit 0
