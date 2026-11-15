#!/bin/sh
# sign-19-7: Phoenix's H2 index sampler can exhaust its fixed 512-byte pool,
# and its caller ignores the failure.
#
# Checks the pool, the failure return and the unchecked call in the reference
# SHAKE and SM3 384s trees, then builds the archived Phoenix-SHAKE-384s sampler
# and replays one digest for which it returns -1, next to a control digest.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-19-7: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-19-7: $1"; exit 1; }

ref='sign-19/Implementations and Test_Vectors/Implementations/Reference_Implementation'
for v in SHAKE:hash_shake.c SM3:hash_sm3.c; do
    d="$ref/Phoenix-${v%%:*}-384s"; f="$d/${v#*:}"
    grep -Fq 'unsigned char rnd[512];' "$f" &&
        grep -Fq 'if (rnd_ptr + 8 > sizeof(rnd)) return -1;' "$f" &&
        grep -Eq '^ +h2_generate_indices\(indices1, m, ctx\);' "$d/tfors.c" ||
        bad "Phoenix-${v%%:*}-384s sampler or caller differs from the archive"
done
echo 'source: the sampler draws from a 512-byte pool and returns -1 when it is used up; message_to_indices ignores the result'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
d="$ref/Phoenix-SHAKE-384s"
gcc -O2 -w -DPARAMS=phoenix-shake-384s -I"$d" -o "$scratch/h2" sign-19/reproduce_h2_exhaustion.c \
    "$d/hash_shake.c" "$d/fips202.c" "$d/address.c" "$d/utils.c" "$d/thash_shake_simple.c" \
    2>"$scratch/cc" || { cat "$scratch/cc"; bad 'build failed'; }
out=$("$scratch/h2") || bad 'run failed'
echo "Phoenix-SHAKE-384s: $out"
[ "$out" = 'exhausting_digest=-1 control_digest=0 k=37 k_prime=64' ] || bad 'behaviour differs'
echo 'CONFIRMED sign-19-7: the archived sampler fails on a concrete digest, and its caller would continue with unwritten indices'
exit 0
