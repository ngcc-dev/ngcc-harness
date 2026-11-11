#!/bin/sh
# hash-23-1: CryptHash derives its rate from an unvalidated digest_len_bits.
#
# Checks the rate computation in all six wrappers, then builds each unmodified
# wrapper with AddressSanitizer. Supported 512/768/1024-bit requests must run
# cleanly; a 256-bit request must write 328 bytes into the 265-byte final-block
# buffer; a 1568-bit request must not return from the reference wrappers.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP hash-23-1: gcc unavailable'; exit 77; }
command -v timeout >/dev/null 2>&1 || { echo 'SKIP hash-23-1: timeout unavailable'; exit 77; }

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
printf 'int main(void){return 0;}\n' > "$scratch/probe.c"
gcc -fsanitize=address -o "$scratch/probe" "$scratch/probe.c" 2>/dev/null ||
    { echo 'SKIP hash-23-1: gcc cannot build with AddressSanitizer'; exit 77; }

base=hash-23/QILIN/Implementations
export ASAN_OPTIONS=detect_leaks=0:exitcode=99
ok=0
for tree in Reference_Implementation Optimized_Implementation; do
    for l in 512 768 1024; do
        src=$base/$tree/QILIN-$l
        f=$src/CryptHash_AlgorithmInstance.c
        grep -q 'rate_bits = 3136 - 2 \* digest_len_bits;' "$f" &&
            grep -q 'uint8_t last_block\[265\]' "$f" ||
            { echo "NOT CONFIRMED hash-23-1: $f differs"; exit 1; }
        bin=$scratch/${tree%%_*}$l
        gcc -O1 -g -fsanitize=address -fno-omit-frame-pointer -I "$src" -o "$bin" \
            hash-23/reproduce_digest_length.c "$f" 2>"$scratch/cc" ||
            { cat "$scratch/cc"; echo "NOT CONFIRMED hash-23-1: $tree/QILIN-$l build failed"; exit 1; }
        for d in 512 768 1024; do
            "$bin" "$d" >/dev/null 2>"$scratch/e" ||
                { cat "$scratch/e"; echo "NOT CONFIRMED hash-23-1: $tree/QILIN-$l control $d failed"; exit 1; }
        done
        "$bin" 256 >/dev/null 2>"$scratch/e"; st=$?
        if [ "$st" -ne 99 ] || ! grep -q 'WRITE of size 328' "$scratch/e" ||
            ! grep -q 'in CryptHash' "$scratch/e"; then
            cat "$scratch/e"; echo "NOT CONFIRMED hash-23-1: $tree/QILIN-$l 256-bit request"; exit 1
        fi
        line="$tree/QILIN-$l: 512/768/1024 clean; 256 gives WRITE of size 328 in CryptHash"
        if [ "$tree" = Reference_Implementation ]; then
            timeout 5 "$bin" 1568 0 >/dev/null 2>&1; st=$?
            [ "$st" -eq 124 ] ||
                { echo "NOT CONFIRMED hash-23-1: $tree/QILIN-$l 1568-bit request returned"; exit 1; }
            line="$line; 1568 still running after 5 s"
        fi
        echo "$line"
        ok=$((ok + 1))
    done
done
echo "CONFIRMED hash-23-1: $ok/6 wrappers overrun last_block[265] on a 256-bit request; the reference wrappers hang on 1568"
exit 0
