#!/bin/sh
# hash-02-6: AXIS returns a message-independent digest for byte-aligned
# lengths 2^64-128 ... 2^64-72.
#
# The byte-aligned fast path computes
#   padded_bits = message_bits + 2 + zero_count + 64
# in 64-bit arithmetic (reference axis_core.c:1306-1307, optimized
# :1966-1967 for AXIS-512, and the same in the 768/1024 paths). For the
# eight byte-aligned lengths in [2^64-128, 2^64-72] it wraps to 0, the
# absorption loop does not run and the digest is the squeeze of the initial
# state. The driver passes a PROT_NONE page as the message, so any read would
# fault, and requires one digest for all eight lengths that differs from the
# empty-message digest. All six instance directories are built with gcc -O2.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP hash-02-6: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED hash-02-6: $1"; exit 1; }
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
n=0
for tree in Reference Optimized; do
    for bits in 512 768 1024; do
        d="hash-02/AXIS/Implementations/${tree}_Implementation/AXIS-$bits"
        grep -q 'padded_bits = message_bits + 2ULL + zero_count + AXIS_LENGTH_BITS;' "$d/axis_core.c" ||
            bad "$tree AXIS-$bits source differs from the archive"
        gcc -std=c99 -O2 -w -DDIG="$bits" -I"$d" -o "$scratch/t" "$here/reproduce_length_wrap.c" \
            "$d/CryptHash_AlgorithmInstance.c" "$d/axis_core.c" 2>"$scratch/cc" || { cat "$scratch/cc"; bad "$tree AXIS-$bits build failed"; }
        out=$("$scratch/t") || bad "$tree AXIS-$bits run failed (message read?)"
        echo "  $tree: $out"
        case "$out" in *'rc=0 identical_for_8_lengths=1 differs_from_empty=1') ;; *) bad "$tree AXIS-$bits: $out" ;; esac
        n=$((n + 1))
    done
done
echo "$n instance directories return 0 with one constant digest for 8 lengths near 2^64 without reading the message"
echo "ATTACK hash-02-6: byte-aligned lengths 2^64-128 to 2^64-72 wrap padded_bits to 0 and give a constant digest: CONFIRMED"
exit 0
