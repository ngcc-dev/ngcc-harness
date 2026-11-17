#!/bin/sh
# hash-35-2 and hash-35-3: Wish CryptHash mishandles digest_len_bits.
#
# (1) The two reference trees pass digest_len_bits to Wish_hash as the digest
#     size and twice it as the state size, with no check. Built with
#     AddressSanitizer and called through a large sentinel buffer, an 8-bit
#     request overflows a heap allocation inside the function (memset of 2
#     bytes into the 0-byte state, wish.c:169), a 264-bit request writes 66
#     bytes into a 64-byte state (wish.c:169), and a 256-bit request reads
#     past the 64-byte state through the 2048-bit mixing table (wish.c:218).
#     Control: 512- and 1024-bit requests run clean and return the digest.
# (2) The optimized and additional trees for this host's architecture (x86 or
#     ARM) ignore digest_len_bits: 256- and 8-bit requests return 0 and write
#     the full 64/128-byte digest.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP hash-35-2 and hash-35-3: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED hash-35-2 and hash-35-3: $1"; exit 1; }
base='hash-35/Wish/Implementations'
scratch=$(mktemp -d) || exit 2
trap 'rm -f -- "$scratch"/*; rmdir -- "$scratch"' EXIT HUP INT TERM
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1; export ASAN_OPTIONS
asan='-O1 -g -fsanitize=address -fno-omit-frame-pointer'
n=0
for bits in 512 1024; do
    d="$base/Reference_Implementation/Wish${bits}_reference"
    gcc $asan -w -I"$d" -o "$scratch/t" "$here/reproduce_digest_length.c" \
        "$d/CryptHash_AlgorithmInstance.c" "$d/wish.c" 2>"$scratch/cc" ||
        { echo 'SKIP hash-35-2 and hash-35-3: AddressSanitizer build unavailable'; exit 77; }
    out=$("$scratch/t" "$bits" "$bits" 2>"$scratch/err") || { cat "$scratch/err"; bad "$d control run failed"; }
    [ "$out" = "requested=$bits rc=0 written_bytes=$((bits / 8)) equals_full=1" ] || bad "Wish$bits reference control: $out"
    for spec in 8:WRITE:2:169 264:WRITE:66:169 256:READ:1:218; do
        req=${spec%%:*}; rest=${spec#*:}; kind=${rest%%:*}; rest=${rest#*:}; size=${rest%%:*}; line=${rest#*:}
        "$scratch/t" "$bits" "$req" >/dev/null 2>"$scratch/err" && bad "Wish$bits reference: request $req ran clean"
        grep -q 'ERROR: AddressSanitizer: \(heap-buffer-overflow\|unknown-crash\)' "$scratch/err" &&
        grep -q "$kind of size $size " "$scratch/err" &&
        grep -q "in Wish_hash .*wish\.c:$line" "$scratch/err" &&
        grep -q 'bytes after [0-9]*-byte region' "$scratch/err" ||
            { head -20 "$scratch/err"; bad "Wish$bits reference: request $req, expected heap $kind of size $size at wish.c:$line"; }
    done
    n=$((n + 1))
done
echo "reference: in all $n trees an 8-bit request writes 2 bytes and a 264-bit request writes 66 bytes into"
echo '  a smaller heap state (wish.c:169); a 256-bit request reads past the state (wish.c:218)'
case $(uname -m) in
    x86_64|amd64|i?86) arch=x86; fl='-maes' ;;
    aarch64|arm64) arch=arm; fl='-march=armv8-a+aes' ;;
    *) arch= ;;
esac
m=0
if [ -n "$arch" ]; then
    for dir in "Optimized_Implementation/%s_${arch}_performance_optimized" "Additional_Implementation/%s_${arch}_resource_optimized"; do
        for bits in 512 1024; do
            d="$base/$(printf "$dir" "Wish$bits")"
            gcc -O2 $fl -w -I"$d" -o "$scratch/t" "$here/reproduce_digest_length.c" \
                "$d/CryptHash_AlgorithmInstance.c" "$d/wish.c" 2>"$scratch/cc" || { cat "$scratch/cc"; bad "$d build failed"; }
            for req in 256 8; do
                out=$("$scratch/t" "$bits" "$req") || bad "$d run failed"
                [ "$out" = "requested=$req rc=0 written_bytes=$((bits / 8)) equals_full=1" ] || bad "$d: $out"
            done
            m=$((m + 1))
        done
    done
    echo "optimized/additional: all $m $arch trees return 0 for 256- and 8-bit requests and write the full digest"
else
    echo 'optimized/additional: skipped (host is neither x86 nor ARM)'
fi
echo "ATTACK hash-35-2 and hash-35-3: Wish reference CryptHash overflows heap memory for unsupported digest lengths in all $n trees, and $m $arch wrappers ignore digest_len_bits and write the full digest: CONFIRMED"
exit 0
