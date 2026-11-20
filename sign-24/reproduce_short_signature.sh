#!/bin/sh
# sign-24-5: sig_deserialize_signature checks the signature length against a
# bound that omits most of the fixed-size fields it then reads, so a short
# signature is read out of bounds during verification.
#
# Checks the deserializer in all six trees, then builds each with
# AddressSanitizer and passes sig_verify a heap signature buffer allocated at
# exactly the declared length. The public key has the correct length; key and
# signature bytes are arbitrary, since the reads happen before any check of
# their content. A signature exactly at the entry bound must give a
# heap-buffer-overflow while unpacking the next polynomial; one byte short of
# the fixed prefix must give one while reading the proof-hash count. Controls:
# one byte below the entry bound, and the full fixed prefix, are rejected
# cleanly. At the 128-bit level an honest signature verifies at full length
# and overflows when cut to the entry bound.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-24-5: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-24-5: $1"; exit 1; }

impl='sign-24/Implementations'
[ -d "$impl" ] || bad "implementation tree not found"
driver="$here/reproduce_short_signature.c"
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
ASAN_OPTIONS=detect_leaks=0
export ASAN_OPTIONS

printf 'int main(void){return 0;}\n' >"$scratch/t.c"
gcc -fsanitize=address -o "$scratch/t" "$scratch/t.c" >/dev/null 2>&1 && "$scratch/t" ||
    { echo 'SKIP sign-24-5: AddressSanitizer unavailable'; exit 77; }

entry_check='    if (sn_len_bytes < (2ULL * RAW_POLY_BYTES) + (unsigned long long)(total_groups * HASH_DIGEST_LENGTH) + 4ULL)'
count_read='    *commitment_open_hash_count = (size_t)unpack_u32_le(sn + offset);'
for t in Reference Optimized; do
    for l in 128 256 512; do
        f="$impl/${t}_Implementation/Sigurd-$l/SIG_AlgorithmInstance.c"
        tr -d '\r' <"$f" | grep -Fxq "$entry_check" && tr -d '\r' <"$f" | grep -Fxq "$count_read" ||
            bad "$t Sigurd-$l deserializer differs from the archive"
    done
done
echo 'source: all 6 deserializers check only root hashes, two polynomials and 4 bytes before the fixed-size reads'

for t in Reference Optimized; do
    for l in 128 256 512; do
        (cd "$impl/${t}_Implementation/Sigurd-$l" &&
            gcc -O1 -g -fsanitize=address -fno-omit-frame-pointer -std=gnu11 -w -I. \
                -o "$scratch/$t-$l" "$driver" SIG_AlgorithmInstance.c sig_core.c rsencode.c \
                rsencode_common.c auxfunc.c drng.c) >"$scratch/$t-$l.log" 2>&1 &
    done
done
wait

overflow() { # binary mode len function
    "$1" "$2" "$3" >"$scratch/out" 2>&1 && return 1
    grep -q 'ERROR: AddressSanitizer: heap-buffer-overflow' "$scratch/out" &&
        grep -q "is located 0 bytes after $3-byte region" "$scratch/out" &&
        grep -q "in $4 .*SIG_AlgorithmInstance.c" "$scratch/out" &&
        grep -q 'in sig_deserialize_signature .*SIG_AlgorithmInstance.c' "$scratch/out" &&
        grep -q 'in sig_verify .*SIG_AlgorithmInstance.c' "$scratch/out"
}
clean() { # binary mode len expected-verify
    out=$("$1" "$2" "$3" 2>&1) && case "$out" in *"verify=$4") return 0 ;; esac
    return 1
}
n=0
for t in Reference Optimized; do
    for l in 128 256 512; do
        b="$scratch/$t-$l"
        [ -x "$b" ] || { cat "$b.log"; bad "$t Sigurd-$l build failed"; }
        set -- $("$b" bounds | tr '=' ' ')
        entry=$2 prefix=$4 max=$6
        poly=unpack_raw_poly
        clean "$b" keyless $((entry - 1)) -4 || bad "$t Sigurd-$l: $((entry - 1)) bytes not rejected cleanly"
        overflow "$b" keyless "$entry" $poly ||
            { cat "$scratch/out"; bad "$t Sigurd-$l: no overflow at the $entry-byte entry bound"; }
        overflow "$b" keyless $((prefix - 1)) unpack_u32_le ||
            { cat "$scratch/out"; bad "$t Sigurd-$l: no overflow at $((prefix - 1)) bytes"; }
        clean "$b" keyless "$prefix" -4 || bad "$t Sigurd-$l: $prefix bytes not rejected cleanly"
        msg="entry bound $entry, fixed prefix $prefix, maximum $max: $((entry - 1)) and $prefix bytes rejected cleanly; $entry and $((prefix - 1)) bytes read out of bounds"
        if [ "$l" = 128 ]; then
            clean "$b" honest full 0 || bad "$t Sigurd-$l honest signature does not verify"
            overflow "$b" honest "$entry" $poly ||
                { cat "$scratch/out"; bad "$t Sigurd-$l: no overflow for an honest signature cut to $entry bytes"; }
            msg="$msg; honest signature: full verify=0, cut to $entry bytes read out of bounds"
        fi
        echo "$t Sigurd-$l: $msg"
        n=$((n + 1))
    done
done
echo "$n builds: signatures between the entry bound and the fixed prefix are read out of bounds; controls are clean"
echo "ATTACK sign-24-5: sig_deserialize_signature under-checks the signature length and over-reads a short signature: CONFIRMED"
exit 0
