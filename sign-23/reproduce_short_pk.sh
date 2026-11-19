#!/bin/sh
# sign-23: Shuttle sig_verify discards pk_len_bytes and unpack_pk reads the
# fixed-size public key. Each of the nine trees (reference, AVX2 optimized,
# AVX-512 additional; SHUTTLE-128/256/512) is built with its archived Makefile
# flags (without -flto) plus AddressSanitizer. A valid public key in a heap
# buffer one byte short, declared at that length, must be read past the
# buffer in unpack_pk; the full-length buffer must verify cleanly. The AVX2
# and AVX-512 trees are skipped on CPUs without those features.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-23: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-23: $1"; exit 1; }
drv=$here/reproduce_short_pk.c
impl="sign-23/Implementation codes and test vectors/Shuttle 算法实现源代码及测试向量/Implementations"
[ -d "$impl/Reference_Implementation/SHUTTLE-128" ] || bad "tree not found"

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
echo 'int main(void){return 0;}' >"$scratch/p.c"
gcc -fsanitize=address -o "$scratch/p" "$scratch/p.c" 2>/dev/null && "$scratch/p" ||
    { echo 'SKIP sign-23: AddressSanitizer unavailable'; exit 77; }
ASAN_OPTIONS=detect_leaks=0; export ASAN_OPTIONS
has() { for f in "$@"; do grep -qw "$f" /proc/cpuinfo 2>/dev/null || return 1; done; }
n=0
for t in Reference Optimized Additional; do
    case $t in
        Optimized) has avx2 bmi2 popcnt || { echo "SKIP $t trees: CPU lacks AVX2"; continue; } ;;
        Additional) has avx2 avx512f avx512bw avx512dq avx512vl avx512_vbmi2 || { echo "SKIP $t trees: CPU lacks AVX-512"; continue; } ;;
    esac
    for s in 128:q15361n256 256:q61441n512 512:q59393n1024; do
        v=${s%%:*}; q=${s#*:}
        d="$impl/${t}_Implementation/SHUTTLE-$v"
        tr -d '\r' < "$d/SIG_AlgorithmInstance.c" | sed -n '/^int sig_verify/,/^}/p' | tr -d ' \n' | grep -Fq '(void)pk_len_bytes;' &&
            grep -Fq 'memcpy(seedA, pk, SEEDBYTES);' "$d/packing.c" || bad "$t SHUTTLE-$v differs from the archive"
        # the archived Makefile's CFLAGS and SIMD_SRCS (identical at the three levels);
        # checked against the Makefile when one is present (the harness copy has none)
        case $t in
            Reference) mcf='-std=c99 -Wpedantic -Wall -Wextra -O2'; msimd='' ;;
            Optimized) mcf='-O3 -march=x86-64 -mavx2 -mtune=native -flto -fomit-frame-pointer -std=c99 -Wpedantic -Wall -Wextra -mbmi2 -mpopcnt -mno-avx512f -DUSE_AVX2_NTT -DUSE_AVX2_SAMPLER -DUSE_AVX2_SHAKE4X -DUSE_AVX2_XOF_NWAY'
                msimd='$(QSET)/ntt.S $(QSET)/ntt_consts.c symmetric_avx2.c drng_avx2.c auxfunc_avx2.c' ;;
            Additional) mcf='-O3 -march=x86-64 -mavx2 -mtune=native -flto -fomit-frame-pointer -std=c99 -Wpedantic -Wall -Wextra -mbmi2 -mpopcnt -mavx512f -mavx512bw -mavx512dq -mavx512vl -mavx512vbmi2 -DUSE_AVX512_NTT -DUSE_AVX512_SAMPLER -DUSE_AVX512_SHAKE8X -DUSE_AVX512_XOF_NWAY'
                msimd='$(QSET)/ntt_avx512.S $(QSET)/ntt_consts_avx512.c symmetric_avx512.c drng_avx512.c auxfunc_avx512.c' ;;
        esac
        if [ -f "$d/Makefile" ]; then
            [ "$(sed -n 's/^CFLAGS *= *//p' "$d/Makefile" | tr -s ' ' | sed 's/ *$//')" = "$mcf" ] &&
                [ "$(sed -n 's/^SIMD_SRCS *= *//p' "$d/Makefile" | tr -s ' ' | sed 's/ *$//')" = "$msimd" ] ||
                bad "$t SHUTTLE-$v Makefile flags differ from the archived ones"
        fi
        cf=$(echo "$mcf" | sed 's/-flto//; s/-Wpedantic -Wall -Wextra//')
        simd=$(echo "$msimd" | sed "s/\$(QSET)/$q/g")
        src="SIG_AlgorithmInstance.c sign.c polyvec.c sampler.c sampler_u.c irs.c rounding.c packing.c poly.c poly_ntt.c reduce.c rans.c approx_exp.c approx_log.c symmetric.c drng.c auxfunc.c ntt/$q/ntt_ref.c $simd"
        (cd "$d" && gcc $cf -w -g -fsanitize=address -fno-omit-frame-pointer -I. -Intt/$q -DSHUTTLE_MODE=$v \
            -o "$scratch/x" "$drv" $src -lm) >"$scratch/cc" 2>&1 || { cat "$scratch/cc"; bad "$t SHUTTLE-$v build failed"; }
        full=$("$scratch/x" full 2>&1)
        echo "$full" | grep -q '^verify rc=0$' && ! echo "$full" | grep -q AddressSanitizer || { echo "$full"; bad "$t SHUTTLE-$v full-length control failed"; }
        short=$("$scratch/x" short 2>&1)
        echo "$short" | grep -q 'heap-buffer-overflow' && echo "$short" | grep -q 'unpack_pk .*packing\.c' &&
            echo "$short" | grep -q 'located 0 bytes after' || { echo "$short" | head -20; bad "$t SHUTTLE-$v short pk not read past"; }
        echo "$t SHUTTLE-$v: full pk verifies cleanly; $(echo "$short" | grep -m1 -o 'READ of size [0-9]*') via unpack_pk ($(echo "$short" | grep 'unpack_pk' | grep -m1 -o 'packing\.c:[0-9]*')), $(echo "$short" | grep -m1 -o 'located 0 bytes after [0-9]*-byte region')"
        n=$((n + 1))
    done
done
[ $n -ge 3 ] || bad "no tree tested"
echo "ATTACK sign-23-4: sig_verify ignores pk_len_bytes and reads past a one-byte-short public key ($n of 9 trees run): CONFIRMED"
exit 0
