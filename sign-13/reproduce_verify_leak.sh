#!/bin/sh
# sign-13-6: GreatWall verification leaks the GGM commitment tree when a
# signature-length byte string sets a challenge bit that must be zero.
#   Source: batch_vector_verify returns before freeing `tree` in all 16 trees.
#   Runtime: each of the eight reference sets is built from its unmodified
#   sources with reproduce_verify_leak.c. For the six sets with grinding
#   bits, the heap must grow by about the tree size per call for the
#   "grind" and all-0xFF strings and stay flat otherwise. The 512-bit sets
#   have no grinding bits (checked in config.h); GreatWall512f is run as the
#   control and must stay flat in every case. GreatWall512s signs too slowly
#   for a routine run and is checked by source only.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-13-6: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-13-6: $1"; exit 1; }
impl=sign-13/Implementations
[ -d "$impl/Reference_Implementation/GreatWall128s" ] || bad "tree not found"

n=0
for f in $(find "$impl" -name vector_com.c | sort); do
    sed -n '/^bool batch_vector_verify(/,/^}/p' "$f" | tr -d '\r' |
        awk '/aligned_alloc/{a=1} a&&/if\( *delta\[i\] != 0\)/{getline; if ($0 ~ /return 0;/) ok=1} END{exit !ok}' ||
        bad "$f has no early return after the tree allocation"
    n=$((n + 1))
done
[ $n = 16 ] || bad "expected 16 vector_com.c, found $n"
echo "source: 16/16 trees return from batch_vector_verify before freeing the tree when a forbidden delta bit is set"
for s in 512s 512f; do
    grep -q '^#define ZERO_BITS_IN_CHALLENGE_3 0$' "$impl/Reference_Implementation/GreatWall$s/config.h" || bad "GreatWall$s has grinding bits"
done
echo "source: GreatWall512s and GreatWall512f have no grinding bits, so the early return never runs"

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
echo 'int main(void){return 0;}' >"$scratch/p.c"
gcc -o "$scratch/p" "$scratch/p.c" -lcrypto 2>/dev/null || { echo 'SKIP sign-13-6: libcrypto unavailable'; exit 77; }

# The source list of the per-set Makefiles, without KAT_SIG.c; only the matrix file differs.
common='SIG_AlgorithmInstance.c drng.c api.c aes.c aes_impl.c block.c block_impl.c faest.c greatwall.c greatwall_impl.c hash.c KeccakHash.c KeccakP-1600-reference.c KeccakSponge.c owf_proof.c polynomials.c polynomials_impl.c prgs.c quicksilver.c randomness_randombytes.c small_vole.c transpose.c universal_hash.c util.c vector_com.c vole_check.c vole_commit.c'
leaky=0
for s in 128s 128f 192s 192f 256s 256f 512f; do
    d=$impl/Reference_Implementation/GreatWall$s
    lvl=${s%[sf]}
    srcs="$common greatwall-$lvl-matrix.c"
    (cd "$d" && gcc -O2 -std=gnu11 -w -Wno-implicit-function-declaration -I. -o "$scratch/v$s" "$here/reproduce_verify_leak.c" $srcs -lcrypto) \
        >"$scratch/cc" 2>&1 || { tail -20 "$scratch/cc"; bad "GreatWall$s build failed"; }
    calls=3; [ "$s" = 512f ] && calls=1
    out=$("$scratch/v$s" $calls) || { echo "$out"; bad "GreatWall$s driver failed"; }
    line=$(echo "$out" | grep '^RESULT ') || { echo "$out"; bad "GreatWall$s: no result"; }
    set -- $(echo "$line" | sed 's/[a-z_]*=//g')
    zb=$3; tree=$4; grind=$5; ones=$6; others=$7
    [ "$others" = 0 ] || { echo "$out"; bad "GreatWall$s leaks on valid, flipped or zero inputs"; }
    if [ "$zb" -gt 0 ]; then
        [ "$grind" -ge "$tree" ] && [ "$grind" -lt $((tree + 4096)) ] &&
        [ "$ones" -ge "$tree" ] && [ "$ones" -lt $((tree + 4096)) ] ||
            { echo "$out"; bad "GreatWall$s: growth $grind/$ones is not the $tree-byte tree"; }
        leaky=$((leaky + 1))
        echo "GreatWall$s: $zb zero bits; grind and all-0xFF strings leak $grind and $ones bytes per call (tree $tree bytes); valid, flipped and zero strings leak 0"
    else
        [ "$grind" = 0 ] && [ "$ones" = 0 ] || { echo "$out"; bad "GreatWall$s leaks without grinding bits"; }
        echo "GreatWall$s: no grinding bits; no input leaks (control)"
    fi
done
[ $leaky = 6 ] || bad "expected 6 leaking sets, found $leaky"
echo 'ATTACK sign-13-6: any signature-length string with a forbidden challenge bit makes sig_verify leak the commitment tree, in six of eight sets: CONFIRMED'
exit 0
