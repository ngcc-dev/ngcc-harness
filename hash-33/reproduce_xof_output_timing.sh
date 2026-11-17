#!/bin/sh
# hash-33-1: Thunder's XOF copies its output with a branch on each output bit.
#
# Builds reference Thunder-XOF-256 with gcc -O0, gcc -O2 and, if a native clang
# is found, clang -O2, and checks three things.
#   A. Taint: with the message marked undefined, Valgrind memcheck must report
#      a branch in copy_bits_msb for gcc -O0 (and clang -O2), and nothing for
#      gcc -O2.
#   B. Instruction count (callgrind): for gcc -O0 the count inside CryptHash
#      must be an exact affine function of the output's Hamming weight, with nonzero slope; for
#      gcc -O2 it must not vary.
#   C. Timing: the regression of time on the output's Hamming weight is
#      printed for each build. It is informational, because it depends on the
#      host and its load.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
id=hash-33-1
src='hash-33/Thunder/Implementations and Test_vector/Thunder-X86/Implementations/Reference_Implementation/Thunder-XOF-256'
drv=$here/reproduce_xof_output_timing.c
for t in gcc valgrind; do
    command -v "$t" >/dev/null 2>&1 || { echo "SKIP $id: $t unavailable"; exit 77; }
done
echo '#include <valgrind/memcheck.h>' | gcc -E -x c - >/dev/null 2>&1 ||
    { echo "SKIP $id: valgrind/memcheck.h unavailable"; exit 77; }
bad() { echo "NOT CONFIRMED $id: $1"; exit 1; }
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM

build() {   # compiler, optimization, name
    "$1" "$2" -g -w -I"$src" -c -o "$scratch/$3.o" "$src/CryptHash_AlgorithmInstance.c" 2>/dev/null &&
        gcc -O2 -w -o "$scratch/$3" "$drv" "$scratch/$3.o" -lm 2>/dev/null &&
        "$scratch/$3" one 1 >/dev/null 2>&1
}
taint() {   # name -> prints "branch" or "clean"
    if valgrind -q --error-exitcode=9 "$scratch/$1" taint >/dev/null 2>"$scratch/err"; then
        echo clean
    elif grep -q 'copy_bits_msb' "$scratch/err"; then
        echo branch
    else
        echo other
    fi
}
counts() {  # name -> prints "hw ir" for six messages
    for seed in 1 2 3 4 5 6; do
        hw=$("$scratch/$1" one $seed)
        ir=$(valgrind --tool=callgrind --toggle-collect=CryptHash --callgrind-out-file=/dev/null \
            "$scratch/$1" one $seed 2>&1 >/dev/null | sed -n 's/.*Collected : \([0-9][0-9]*\).*/\1/p')
        echo "$hw $ir"
    done
}

build gcc -O0 g0 || bad 'gcc -O0 build failed'
build gcc -O2 g2 || bad 'gcc -O2 build failed'
clang=
for c in clang clang-21 clang-20 clang-19 clang-18 clang-17; do
    if command -v "$c" >/dev/null 2>&1 && build "$c" -O2 c2; then clang=$c; break; fi
done

a0=$(taint g0); a2=$(taint g2)
echo "A. taint: gcc -O0 $a0, gcc -O2 $a2"
[ "$a0" = branch ] || bad 'gcc -O0 shows no branch in copy_bits_msb'
[ "$a2" = clean ] || bad 'gcc -O2 is not clean'
if [ -n "$clang" ]; then
    ac=$(taint c2)
    echo "A. taint: $clang -O2 $ac"
fi

c0=$(counts g0); c2=$(counts g2)
echo "$c0" | awk '{h[NR]=$1; v[NR]=$2} END{j=0; for(i=2;i<=NR;i++) if(h[i]!=h[1]){j=i;break}
    if(!j||v[j]==v[1]) exit 1; for(i=1;i<=NR;i++) if((v[i]-v[1])*(h[j]-h[1])!=(v[j]-v[1])*(h[i]-h[1])) exit 1
    printf "B. gcc -O0: instructions = constant %+d per output 1-bit, exactly, for all six messages\n",(v[j]-v[1])/(h[j]-h[1])}' ||
    bad 'gcc -O0 instruction count is not an exact affine function of the Hamming weight'
echo "$c2" | awk 'NR==1{k=$2} {if($2!=k) bad=1} END{printf "B. gcc -O2: %d instructions for all six messages\n",k; exit bad}' ||
    bad 'gcc -O2 instruction count varies'

echo "C. timing, gcc -O0: $("$scratch/g0" time 60000)"
echo "C. timing, gcc -O2: $("$scratch/g2" time 60000)"
[ -n "$clang" ] && echo "C. timing, $clang -O2: $("$scratch/c2" time 60000)"

echo "ATTACK $id: the XOF output copy branches on each output bit, and the instruction count of an unoptimized build follows the output's Hamming weight: CONFIRMED"
exit 0
