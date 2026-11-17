#!/bin/sh
# hash-02-2: AXIS executes a branch on secret bits, and the effect is measurable.
#
# Builds the reference AXIS-512 with the submission's flags (gcc -std=c99 -O2)
# and checks, for a 501-bit message:
#   A. Taint: with the message marked undefined, Valgrind memcheck reports a
#      branch in axis_flip_bit.
#   B. Instruction count (callgrind): the count inside CryptHash differs
#      between random messages of the same length.
#   Control: a 504-bit message is clean under taint and has a constant count.
#   C. Timing: printed for information; it depends on the host and its load.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
id=hash-02-2
src='hash-02/AXIS/Implementations/Reference_Implementation/AXIS-512'
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
for f in CryptHash_AlgorithmInstance.c axis_core.c; do
    gcc -std=c99 -O2 -g -w -I"$src" -c -o "$scratch/$f.o" "$src/$f" 2>"$scratch/cc" || { cat "$scratch/cc"; bad "build of $f failed"; }
    objs="$objs $scratch/$f.o"
done
gcc -O2 -w -o "$scratch/t" "$drv" $objs -lm 2>"$scratch/cc" || { cat "$scratch/cc"; bad 'driver build failed'; }

taint() {   # message bits -> "branch", "clean" or "other"
    if valgrind -q --error-exitcode=9 "$scratch/t" taint 512 "$1" >/dev/null 2>"$scratch/err"; then echo clean
    elif grep -q 'axis_flip_bit' "$scratch/err"; then echo branch
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

a=$(taint 501)
echo "A. taint, 501-bit message: $a"
[ "$a" = branch ] || bad 'no secret-dependent branch in axis_flip_bit'
counts 501 | awk '{if(NR==1){lo=$2;hi=$2} if($2<lo)lo=$2; if($2>hi)hi=$2} END{printf "B. 501-bit messages: %d to %d instructions over six random messages\n",lo,hi; exit (lo==hi)}' ||
    bad 'instruction count does not vary with the message'
c=$(taint 504)
[ "$c" = clean ] || bad '504-bit control is not clean under taint'
counts 504 | awk 'NR==1{k=$2} {if($2!=k) b=1} END{printf "Control, 504-bit messages: taint clean, %d instructions for all six\n",k; exit b}' ||
    bad '504-bit control instruction count varies'
echo "C. timing, 501-bit messages: $("$scratch/t" time 512 501 10000)"
echo "C. timing, 504-bit control: $("$scratch/t" time 512 504 10000)"
echo "ATTACK $id: the reference build branches on state bits for messages that are not byte-aligned, and its instruction count depends on the message: CONFIRMED"
exit 0
