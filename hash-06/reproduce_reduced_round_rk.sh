#!/bin/sh
# hash-06-1: Cuishen spec section 5.2.1 (Case 1/2/4) differential figures are
# contradicted by probability-1 message-difference characteristics.
#
# The spec reports, for zero chaining difference and a message-block
# difference, best differential probabilities 2^-2 / 2^-5 / 2^-13 for 4 / 5 / 6
# rounds of Octarx-256 and 2^-6 / 2^-16 for 4 / 5 rounds of Octarx-512.
# Because each key-schedule word reaches a round key only once, three rounds
# after it is created, a two-word message difference can be cancelled inside a
# single round (both addends differ only in bit 63) and then stays invisible
# until a later round. This gives probability-1 characteristics for 6 rounds of
# Octarx-256 and 5 rounds of Octarx-512, and so deterministic collisions of the
# whole reduced-round hash. The submitters' reference code is built with the
# round loops limited to NR rounds; the full 64-round function is NOT attacked.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP hash-06-1: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED hash-06-1: $1"; exit 1; }
ref='hash-06/Cuishen/Implementations/Reference_Implementation'
scratch=$(mktemp -d) || exit 2
trap 'rm -f -- "$scratch"/*.c "$scratch"/t* "$scratch"/cc; rmdir -- "$scratch"' EXIT HUP INT TERM
T=1000
run() { # inst NR variant -> prints "collisions=.. distinct=.. trials=.."
    sed 's/round < ROUNDS/round < NR/' "$ref/Cuishen-$1/CryptHash_Cuishen-$1.c" > "$scratch/ref$1.c" || exit 2
    gcc -O2 -w -DNR="$2" -DDLEN="$1" -I"$ref/Cuishen-$1" -o "$scratch/t" \
        "$scratch/ref$1.c" "$here/reproduce_reduced_round_rk.c" 2>"$scratch/cc" || { cat "$scratch/cc"; bad "build $1"; }
    "$scratch/t" "$3" "$T" || bad "run $1 NR=$2"
}
# sanity: the patched source with NR=64 is the unmodified reference function
for inst in 512 768 1024; do
    sed 's/round < ROUNDS/round < NR/' "$ref/Cuishen-$inst/CryptHash_Cuishen-$inst.c" > "$scratch/p.c"
    gcc -O2 -w -DNR=64 -DDLEN="$inst" -I"$ref/Cuishen-$inst" -o "$scratch/tp" "$scratch/p.c" "$here/reproduce_reduced_round_rk.c" || bad build
    gcc -O2 -w -DDLEN="$inst" -I"$ref/Cuishen-$inst" -o "$scratch/to" "$ref/Cuishen-$inst/CryptHash_Cuishen-$inst.c" "$here/reproduce_reduced_round_rk.c" || bad build
    [ "$("$scratch/tp" 0 50)" = "$("$scratch/to" 0 50)" ] || bad "NR=64 patch changes the $inst function"
done
echo 'sanity: NR=64 build behaves as the unmodified reference (full rounds: no collisions)'
check() { # label inst NR variant expected-min expected-max
    out=$(run "$2" "$3" "$4")
    c=${out#collisions=}; c=${c%% *}
    echo "$1: Cuishen-$2, $3-round Octarx: $out"
    [ "$c" -ge "$5" ] && [ "$c" -le "$6" ] || bad "$1 Cuishen-$2 NR=$3 collisions $c not in [$5,$6]"
}
check 'MSB trail      ' 512 6 0 $T $T
check 'MSB trail      ' 512 7 0 0 0
check 'bit-40 trail   ' 512 6 1 400 600
check 'MSB trail      ' 768 5 2 $T $T
check 'MSB trail      ' 768 6 2 0 0
check 'MSB trail      ' 1024 5 2 $T $T
check 'MSB trail      ' 1024 6 2 0 0
check 'full rounds    ' 512 64 0 0 0
echo 'spec 5.2.1 Case 1: Octarx-256 6 rounds 2^-13, Octarx-512 5 rounds 2^-16 (Cases 2/4 likewise below 1)'
echo "ATTACK hash-06-1: probability-1 message-difference characteristics (6 rounds Octarx-256, 5 rounds Octarx-512) give deterministic collisions of reduced-round Cuishen, contradicting spec 5.2.1: CONFIRMED"
exit 0
