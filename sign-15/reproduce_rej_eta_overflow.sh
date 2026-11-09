#!/bin/sh
# sign-15-17: rej_eta bounds its second write by N rather than by the
# remaining length, so a resumed call can write one coefficient past the
# polynomial. For the last polynomial of s1 this is past the stack object.
#
# Checks the bound in all eight trees, then runs unmodified reference key
# generation under AddressSanitizer for 40 seeds per set. ATLAS-256 and -512
# need a second XOF block and overflow on some seeds; ATLAS-128 and -192 fill
# each polynomial from the first block and are expected not to.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-15-17: gcc unavailable'; exit 77; }

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
printf 'int main(void){return 0;}\n' > "$scratch/probe.c"
gcc -fsanitize=address -o "$scratch/probe" "$scratch/probe.c" 2>/dev/null ||
    { echo 'SKIP sign-15-17: gcc cannot build with AddressSanitizer'; exit 77; }

for tree in Reference_Implementation Optimized_Implementation; do
    for l in 128 192 256 512; do
        f=sign-15/Implementation/$tree/lwrdsa$l/poly.c
        grep -q 'if(t1 <= 2\*ETA && ctr < N)' "$f" ||
            { echo "NOT CONFIRMED sign-15-17: $f differs"; exit 1; }
    done
done
echo 'source: all eight rej_eta copies bound the second write by N, not len'

export ASAN_OPTIONS=detect_leaks=0:exitcode=99
hits256=0; hits512=0
for l in 128 192 256 512; do
    src=sign-15/Implementation/Reference_Implementation/lwrdsa$l
    gcc -O1 -g -std=gnu11 -fsanitize=address -fno-omit-frame-pointer -D_GNU_SOURCE \
        -DNGCC_BUILD_SIG '-DNGCC_ID="sign-15"' '-DNGCC_ALG="ATLAS"' '-DNGCC_VARIANT="ref"' \
        -I "$src" -I api -o "$scratch/k$l" sign-15/reproduce_rej_eta_overflow.c \
        $(ls "$src"/*.c | grep -v KAT_SIG.c) api/link_shim.c -lm 2>"$scratch/cc$l" ||
        { cat "$scratch/cc$l"; echo "NOT CONFIRMED sign-15-17: ATLAS-$l build failed"; exit 1; }
    hit=0; s=0
    while [ "$s" -lt 40 ]; do
        "$scratch/k$l" "$s" >/dev/null 2>"$scratch/e"
        [ $? -eq 99 ] && grep -q 'stack-buffer-overflow' "$scratch/e" &&
            grep -q 'in rej_eta' "$scratch/e" && hit=$((hit + 1))
        s=$((s + 1))
    done
    echo "ATLAS-$l: $hit/40 key generations write past s1 in rej_eta"
    [ "$l" = 256 ] && hits256=$hit
    [ "$l" = 512 ] && hits512=$hit
done
if [ "$hits256" -gt 0 ] && [ "$hits512" -gt 0 ]; then
    echo 'CONFIRMED sign-15-17: honest ATLAS-256 and -512 key generation writes one coefficient past s1'
    exit 0
fi
echo 'NOT CONFIRMED sign-15-17: no overflow observed'
exit 1
