#!/bin/sh
# hash-11-3: Garnet truncates 64-bit byte counts to 32-bit int, so messages
# of 4 GiB or more collide and messages of 2 GiB crash.
#
# Garnet-512: pad_message keeps the byte count in an int and garnet_512
# compares its loop counters with (int)byte_num (Garnet_512_896.c:577-603,
# :664, :682; the same in all five 512-bit rate files). At 128 + 2^35 bits
# (2^32 + 16 bytes) the count wraps to 16, so only bytes 0..15 are absorbed;
# the 64-bit length block is still absorbed, so the collision is between
# messages of the same length. Two such messages that differ in bytes 16 and
# 79, in a middle byte and in the last byte must hash alike, for the 896-bit
# and the 512-bit rate files; the 128-bit prefix alone must not (length
# control), and two 1 KiB messages differing in byte 16 must not.
# Garnet-1024: pad_message (Garnet_1024.c:686-698) writes its 0x80 marker at
# the wrapped offset 16 of the message copy, so two 128 + 2^35-bit messages
# that differ only in byte 16 must hash alike; a 1 KiB control must not.
# Finally a 2^34 + 128-bit (2 GiB + 16 B) message must crash Garnet-512,
# -768 and -1024: the int byte count is negative and pad_message writes
# 2 GiB before the buffer.
#
# Each Garnet call copies the message into a calloc'ed buffer, so the run
# needs about 9 GiB of free memory. Runtime is about 1.5 minutes, most of it
# the two Garnet-1024 hashes of 4 GiB, which run in parallel.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP hash-11-3: gcc unavailable'; exit 77; }
avail=$(awk '/^MemAvailable:/ {print int($2 / 1048576)}' /proc/meminfo 2>/dev/null)
[ "${avail:-0}" -ge 10 ] || { echo 'SKIP hash-11-3: needs about 10 GiB of free memory'; exit 77; }
bad() { echo "NOT CONFIRMED hash-11-3: $1"; exit 1; }
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
G=hash-11/Garnet/Implementations/API_CryptHash/Implementations/Reference_Implementation/Garnet
[ -f "$G/Garnet_512_896.c" ] || bad "reference tree not found"
grep -q 'for (i = 0; (i + 112) <= (int)byte_num; i += 112)' "$G/Garnet_512_896.c" &&
    grep -q 'for (i = 0; (i + 64) <= (int)byte_num; i += 64)' "$G/Garnet_512_512.c" &&
    grep -q 'byte_num = (int)(bit_len / 8);' "$G/Garnet_512_896.c" ||
    bad "Garnet-512 source differs from the archive"

L=$(( (1 << 35) + 128 ))        # 34359738496 bits = 2^32 + 16 bytes
last=$(( (1 << 32) + 15 ))
mid=$(( (1 << 31) + 100 ))
for v in 512_896 512_512; do
    gcc -O2 -w -I"$G" -o "$scratch/t$v" "$here/reproduce_long_message_collision.c" "$G/CryptHash_Garnet.c" \
        "$G/Garnet_$v.c" "$G/Garnet_768.c" "$G/Garnet_1024.c" 2>"$scratch/cc" || { cat "$scratch/cc"; bad "build with Garnet_$v.c failed"; }
    t="$scratch/t$v"
    a=$("$t" hash 512 $L)
    [ $? -eq 77 ] && { echo 'SKIP hash-11-3: sparse mapping refused'; exit 77; }
    b=$("$t" hash 512 $L 16=0x5a 79=0x5a $mid=0xff $last=0x01)
    s=$("$t" hash 512 128)
    c1=$("$t" hash 512 8192)
    c2=$("$t" hash 512 8192 16=0x5a)
    echo "Garnet_$v.c, Garnet-512, $L-bit messages:"
    echo "  H(A) = $a"
    echo "  H(B) = $b"
    echo "  H(128-bit prefix) = $s"
    case "$a" in 'rc=0 '*) ;; *) bad "Garnet_$v.c: $a" ;; esac
    [ "$a" = "$b" ] || bad "Garnet_$v.c: no collision"
    [ "$a" != "$s" ] || bad "Garnet_$v.c: length not absorbed"
    [ "$c1" != "$c2" ] || bad "Garnet_$v.c: 1 KiB control collides"
done

t="$scratch/t512_896"
"$t" hash 1024 $L >"$scratch/a1024" &
"$t" hash 1024 $L 16=0x5a >"$scratch/b1024" &
wait
a=$(cat "$scratch/a1024"); b=$(cat "$scratch/b1024")
c1=$("$t" hash 1024 8192); c2=$("$t" hash 1024 8192 16=0x5a)
echo "Garnet-1024, $L-bit messages differing only in byte 16:"
echo "  H(A) = $a"
echo "  H(A with byte 16 = 5a) = $b"
case "$a" in 'rc=0 '*) ;; *) bad "Garnet-1024: $a" ;; esac
[ "$a" = "$b" ] || bad "Garnet-1024: no collision"
[ "$c1" != "$c2" ] || bad "Garnet-1024: 1 KiB control collides"

C=$(( (1 << 34) + 128 ))
for bits in 512 768 1024; do
    out=$("$t" crash $bits $C)
    echo "Garnet-$bits, $C-bit all-zero message: $out"
    [ "$out" = "killed by SIGSEGV" ] || bad "Garnet-$bits did not crash at 2 GiB"
done
echo "same-length 4 GiB collisions in Garnet-512 (two rate files) and Garnet-1024; 1 KiB and length controls differ; 2 GiB messages crash all three sizes"
echo "ATTACK hash-11-3: 32-bit byte counts give same-length collisions for 4 GiB messages and crash on 2 GiB messages: CONFIRMED"
exit 0
