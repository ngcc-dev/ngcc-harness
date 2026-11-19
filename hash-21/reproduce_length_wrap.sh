#!/bin/sh
# hash-21-5: Neulaser returns success without writing a digest for message
# lengths just below 2^64 - 64.
#
# CryptHash computes total_bits = msg_len_bits + 1 + zero_bits + 64 and
# block_count = total_bits / k in 64-bit arithmetic (reference
# CryptHash_AlgorithmInstance.c:362-364, optimized :392-394); the only write
# to digest is inside the block loop (:375-378, :405-408). For
# msg_len_bits in [2^64 - 64 - (2^64 mod k), 2^64 - 65] the sum wraps below k,
# no block is processed and the function returns 0 with digest untouched.
# The driver passes a PROT_NONE page as the message and a 0xcc-filled digest
# buffer; control: the empty message writes the digest. All six instance
# directories are built with their Makefile's optimisation flags.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP hash-21-5: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED hash-21-5: $1"; exit 1; }
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
impl="hash-21/Neulaser/Implementations and Test_Vectors/API_CryptHash/Implementations"
n=0
for tree in Reference Optimized; do
    flags="-std=c99 -O2"
    [ "$tree" = Optimized ] && flags="-std=c99 -O3 -flto -DNDEBUG"
    for p in 512:960 768:1216 1024:1472; do
        bits=${p%:*}; k=${p#*:}
        d="$impl/${tree}_Implementation/Neulaser-$bits"
        grep -q 'total_bits = msg_len_bits + 1ULL + zero_bits + NL_LENGTH_FIELD_BITS;' "$d/CryptHash_AlgorithmInstance.c" ||
            bad "$tree Neulaser-$bits source differs from the archive"
        gcc $flags -w -DDIG="$bits" -DWRAP_K="$k" -I"$d" -o "$scratch/t" "$here/reproduce_length_wrap.c" \
            "$d/CryptHash_AlgorithmInstance.c" 2>"$scratch/cc" || { cat "$scratch/cc"; bad "$tree Neulaser-$bits build failed"; }
        "$scratch/t" >"$scratch/out" || bad "$tree Neulaser-$bits run failed (message read?)"
        sed "s/^/$tree:/" "$scratch/out"
        grep -q 'unwritten_success=1 control_written=1$' "$scratch/out" || bad "$tree Neulaser-$bits behaves differently"
        n=$((n + 1))
    done
done
echo "$n instance directories return 0 and leave the digest buffer unwritten for lengths just below 2^64 - 64"
echo "ATTACK hash-21-5: msg_len_bits near 2^64 wraps total_bits and CryptHash returns success without writing the digest: CONFIRMED"
exit 0
