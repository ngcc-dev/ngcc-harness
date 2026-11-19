#!/bin/sh
# sign-07: two CS verifier memory defects.
#   (a) CS_Verify allocates buf = malloc(len + HBYTES) (cs.c:157) and frees it
#       only on the accepting path; the four "return false" exits (sigDecode
#       failure, z norm bound, challenge mismatch, w norm bound) leak it. All
#       six trees by source; the three reference sets at run time with malloc
#       and free wrapped: 100 rejected verifications of a 28-byte message
#       (challenge mismatch, and a corrupted rANS length that fails sigDecode)
#       leave 100 blocks of 28 + HBYTES bytes; 100 accepted ones leave none.
#   (b) sig_verify passes the signature to CS_Verify without its length
#       (reference SIG_AlgorithmInstance.c; the optimized tree has no API
#       wrapper). A valid signature in a heap buffer one byte shorter than the
#       bytes its rANS fields occupy, declared at that length, is read past
#       the buffer by sigDecode under AddressSanitizer; control: full length.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-07: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-07: $1"; exit 1; }
drv=$here/reproduce_verify_memory.c
impl=sign-07/Implementations
[ -d "$impl/Reference_Implementation/CS-128" ] && [ -d sign-07/patches/common ] || bad "tree not found"

for d in "$impl"/Reference_Implementation/CS-* "$impl"/Optimized_Implementation/CS-*; do
    v=$(sed -n '/^bool CS_Verify/,/^}/p' "$d/cs.c")
    [ "$(echo "$v" | grep -c 'uint8_t\* buf = malloc(len + HBYTES);')" = 1 ] &&
        [ "$(echo "$v" | grep -c 'return false')" = 4 ] &&
        [ "$(echo "$v" | grep -c 'free(buf)')" = 1 ] &&
        [ "$(echo "$v" | sed -n '/free(buf)/,$p' | grep -c 'return false')" = 0 ] || bad "$d/cs.c differs from the archive"
done
for d in "$impl"/Reference_Implementation/CS-*; do
    tr -d '\r' < "$d/SIG_AlgorithmInstance.c" | grep -Fq 'return CS_Verify(pk, m, m_len_bytes, sn) != true;' ||
        bad "$d wrapper differs from the archive"
done
echo 'source: all six CS_Verify copies return false 4 times before their only free(buf); the reference sig_verify drops sn_len_bytes'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
echo 'int main(void){return 0;}' >"$scratch/p.c"
gcc -fsanitize=address -o "$scratch/p" "$scratch/p.c" 2>/dev/null && "$scratch/p" ||
    { echo 'SKIP sign-07: AddressSanitizer unavailable'; exit 77; }
flags="-w -fpermissive -fcommon -I. -I$PWD/sign-07/patches/common"
src='SIG_AlgorithmInstance.c auxfunc.c bits-hints.c conversion.c cs.c drng.c encodings.c ntt.c sampling.c'
ASAN_OPTIONS=detect_leaks=0; export ASAN_OPTIONS
for s in 128:32 256:64 512:128; do
    n=${s%%:*}; hb=${s#*:}
    d=$impl/Reference_Implementation/CS-$n
    z2=$(cd "$d" && printf '#include "params.h"\nPOLYZ2_PACKEDBYTES\n' | gcc -E -P -I. - | tail -n 1)
    (cd "$d" && gcc -O2 $flags -DPOLYZ2="($z2)" -DLEAK -Wl,--wrap=malloc,--wrap=free -o "$scratch/l$n" "$drv" $src -lm &&
        gcc -O1 -g $flags -DPOLYZ2="($z2)" -fsanitize=address -fno-omit-frame-pointer -o "$scratch/a$n" "$drv" $src -lm) \
        >"$scratch/cc" 2>&1 || { cat "$scratch/cc"; bad "CS-$n build failed"; }
    out=$("$scratch/l$n" "$scratch/sig$n") || bad "CS-$n leak run failed"
    echo "CS-$n leak: $out"
    b=$((100 * (28 + hb)))
    [ "$out" = "accepted=100 leaked_blocks=0 leaked_bytes=0; challenge_reject=100 leaked_blocks=100 leaked_bytes=$b; decode_reject=100 leaked_blocks=100 leaked_bytes=$b" ] ||
        bad "CS-$n leak differs"
    full=$("$scratch/a$n" "$scratch/sig$n" full 2>&1) || { echo "$full"; bad "CS-$n full-length control failed"; }
    echo "$full" | grep -q 'verify rc=0$' && ! echo "$full" | grep -q AddressSanitizer || { echo "$full"; bad "CS-$n control differs"; }
    short=$("$scratch/a$n" "$scratch/sig$n" short 2>&1)
    echo "$short" | grep -q 'AddressSanitizer: heap-buffer-overflow' && echo "$short" | grep -q 'in sigDecode' &&
        echo "$short" | grep -q 'in CS_Verify' && echo "$short" | grep -q 'located 0 bytes after' ||
        { echo "$short" | head -20; bad "CS-$n short signature not read past"; }
    echo "CS-$n short: $(echo "$full" | sed -n 's/^siglen=\([0-9]*\) used=\([0-9]*\) .*/siglen \1, decoder uses \2 bytes/p'); full-length clean; $(echo "$short" | grep -m1 -o 'READ of size [0-9]*') in sigDecode, $(echo "$short" | grep -m1 -o 'located 0 bytes after [0-9]*-byte region')"
done
echo 'ATTACK sign-07-9: every rejected CS verification leaks its len + HBYTES message buffer, and a signature buffer shorter than its encoded fields is read out of bounds: CONFIRMED'
exit 0
