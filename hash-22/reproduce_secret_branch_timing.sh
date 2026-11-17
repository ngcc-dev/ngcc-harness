#!/bin/sh
# hash-22-1: Pavelor executes a branch on secret bits, and the effect is measurable.
#
# Builds the reference Pavelor-512 with the submission's flags (gcc -std=c99 -O2)
# and checks, for a 504-bit message:
#   A. Taint: with the message marked undefined, Valgrind memcheck reports a
#      branch in xor_bit.
#   B. Instruction count (callgrind): the count inside CryptHash is an exact
#      affine function of the message's Hamming weight, with nonzero slope.

#   C. Timing: printed for information; it depends on the host and its load.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
id=hash-22-1
src='hash-22/Pavelor/Implementations and Test_Vectors/API_CryptHash/Implementations/Reference_Implementation/Pavelor-512'
drv=$here/reproduce_secret_branch_timing.c
for t in gcc valgrind; do
    command -v "$t" >/dev/null 2>&1 || { echo "SKIP $id: $t unavailable"; exit 77; }
done
echo '#include <valgrind/memcheck.h>' | gcc -E -x c - >/dev/null 2>&1 ||
    { echo "SKIP $id: valgrind/memcheck.h unavailable"; exit 77; }
bad() { echo "NOT CONFIRMED $id: $1"; exit 1; }
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
objs=
for f in CryptHash_AlgorithmInstance.c; do
    gcc -std=c99 -O2 -g -w -I"$src" -c -o "$scratch/$f.o" "$src/$f" 2>"$scratch/cc" || { cat "$scratch/cc"; bad "build of $f failed"; }
    objs="$objs $scratch/$f.o"
done
gcc -O2 -w -o "$scratch/t" "$drv" $objs -lm 2>"$scratch/cc" || { cat "$scratch/cc"; bad 'driver build failed'; }

taint() {   # message bits -> "branch", "clean" or "other"
    if valgrind -q --error-exitcode=9 "$scratch/t" taint 512 "$1" >/dev/null 2>"$scratch/err"; then echo clean
    elif grep -q 'xor_bit' "$scratch/err"; then echo branch
    else echo other; fi
}
counts() {  # message bits -> six lines "weight instructions"
    for seed in 1 2 3 4 5 6; do
        hw=$("$scratch/t" one 512 "$1" $seed)
        ir=$(valgrind --tool=callgrind --toggle-collect=CryptHash --callgrind-out-file=/dev/null \
            "$scratch/t" one 512 "$1" $seed 2>&1 >/dev/null | sed -n 's/.*Collected : \([0-9][0-9]*\).*/\1/p')
        echo "$hw $ir"
    done
}

a=$(taint 504)
echo "A. taint, 504-bit message: $a"
[ "$a" = branch ] || bad 'no secret-dependent branch in xor_bit'
counts 504 | awk '{h[NR]=$1; v[NR]=$2} END{j=0; for(i=2;i<=NR;i++) if(h[i]!=h[1]){j=i;break}
    if(!j||v[j]==v[1]) exit 1; for(i=1;i<=NR;i++) if((v[i]-v[1])*(h[j]-h[1])!=(v[j]-v[1])*(h[i]-h[1])) exit 1
    printf "B. 504-bit messages: instructions = constant %+d per message 1-bit, exactly, for all six\n",(v[j]-v[1])/(h[j]-h[1])}' ||
    bad 'instruction count is not an exact affine function of the Hamming weight'
echo "C. timing, 504-bit messages: $("$scratch/t" time 512 504 60000)"

echo "ATTACK $id: the reference build branches on each message bit of a partial final block, and its instruction count follows the Hamming weight of those bits: CONFIRMED"
exit 0
