#!/bin/sh
# sign-01-11: two Aigis-Sig+ verifier memory defects.
#   (a) msig_verf leaks its CRHBYTES + mlen message buffer on both reject
#       returns (challenge mismatch, norm bound); all 12 trees by source,
#       the three reference sets at run time (200 rejects -> 200 blocks;
#       200 accepts -> 0).
#   (b) the declared signature length is only range-checked, so a valid
#       signature held in a heap buffer of its length minus one byte is read
#       past the buffer under AddressSanitizer; control: full-length buffer.
# Run from the repository root: sh sign-01/reproduce_verify_memory.sh
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
driver=$here/reproduce_verify_memory.c
[ -d sign-01/Implementations ] || { echo 'run from the repository root'; exit 2; }
[ -f "$driver" ] || { echo "missing $driver"; exit 2; }
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-01-11: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-01-11: $1"; exit 1; }

impl=sign-01/Implementations/Implementations
for d in "$impl"/Reference_Implementation/Aigis-Sig+-* "$impl"/Optimized_Implementation/avx2/Aigis-Sig+-* \
         "$impl"/Additional_Implementation/neon/Aigis-Sig+-* "$impl"/Additional_Implementation/aarch64/Aigis-Sig+-*; do
    f=$d/sign.c
    # in msig_verf: one malloc of buf, two bare "return 1;" after it, one free(buf)
    v=$(tr -d '\r' < "$f" | sed -n '/msig_verf/,$p')
    [ "$(echo "$v" | grep -c 'malloc(CRHBYTES + mlen)')" = 1 ] &&
        [ "$(echo "$v" | sed -n '/malloc(CRHBYTES + mlen)/,$p' | grep -c 'free(buf)')" = 1 ] &&
        [ "$(echo "$v" | sed -n '/challenge(tcseed/,/free(buf)/p' | grep -c 'return 1;')" = 2 ] &&
        echo "$v" | grep -q 'smlen < SIG_MIN_SIZE_PACKED || smlen > SIG_MAX_SIZE_PACKED' ||
        bad "$f differs from the archive"
done
echo 'source: all 12 msig_verf copies return 1 twice after malloc(CRHBYTES + mlen) without free(buf), and only range-check smlen'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
flags='-std=gnu99 -w -Wno-incompatible-pointer-types -Wno-int-conversion -DUSE_ICCS -I.'
src='sign.c polyvec.c packing.c poly.c reduce.c ntt.c rounding.c fips202.c hashkdf.c auxfunc.c drng.c pspm.c rng.c'
for s in 1:I 2:II 3:III; do
    p=${s%%:*}; n=${s#*:}
    cp -R "$impl/Reference_Implementation/Aigis-Sig+-$n" "$scratch/$n" || bad "copy failed"
    (cd "$scratch/$n" &&
        gcc -O2 $flags -DPARAMS=$p -DLEAK -Wl,--wrap=malloc -Wl,--wrap=free -o "$scratch/leak$n" "$driver" $src &&
        gcc -O1 -g $flags -DPARAMS=$p -fsanitize=address -fno-omit-frame-pointer -o "$scratch/asan$n" "$driver" $src) \
        >"$scratch/cc" 2>&1 || { cat "$scratch/cc"; bad "Aigis-Sig+-$n build failed"; }
    out=$("$scratch/leak$n" "$scratch/sig$n") || bad "Aigis-Sig+-$n leak run failed"
    echo "Aigis-Sig+-$n leak: $out"
    [ "$out" = 'accepted=200 leaked_blocks=0; rejected=200 leaked_blocks=200' ] || bad "Aigis-Sig+-$n leak differs"
    full=$("$scratch/asan$n" "$scratch/sig$n" full 2>&1) || { echo "$full"; bad "Aigis-Sig+-$n full-length control failed"; }
    echo "$full" | grep -q '^verify rc=0$' || bad "Aigis-Sig+-$n full-length control not accepted"
    short=$("$scratch/asan$n" "$scratch/sig$n" 2>&1)
    echo "Aigis-Sig+-$n short: $(echo "$short" | grep '^siglen=')"
    echo "$short" | grep -q 'AddressSanitizer: heap-buffer-overflow' &&
        echo "$short" | grep -q 'in unpack_h' &&
        echo "$short" | grep -q '0 bytes after' ||
        { echo "$short" | head -20; bad "Aigis-Sig+-$n short buffer not read out of bounds"; }
    echo "  ASan: $(echo "$short" | grep -m1 'READ of size') in unpack_h, $(echo "$short" | grep -m1 'bytes after' | sed 's/^0x[0-9a-f]* is //')"
done
echo 'ATTACK sign-01-11 CONFIRMED: every rejected verification leaks CRHBYTES + mlen bytes (one block) at all three levels, and a valid signature in a heap buffer one byte shorter than its length, declared at that length, is read past the buffer by unpack_h under AddressSanitizer'
exit 0
