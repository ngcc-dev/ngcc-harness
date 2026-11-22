#!/bin/sh
# sign-18-8: a failed allocation while deriving the verification target makes
# the target zero, and the all-zero signature then verifies for any message.
#
# Checks that the squeeze helper zero-fills its output on failure and returns
# nothing, and that target derivation and verification use it unchecked, in
# all eight trees. Each tree is built with its Makefile flags and malloc
# wrapped. A fault-free verification locates target derivation's three
# consecutive allocations (squeeze buffer, then pseudoXOF's input copy and
# output buffer). Failing any one of them must make the all-zero signature
# verify for a never-signed message and for another message. Controls: without
# the fault the zero signature is rejected and an honest signature verifies;
# with the fault a nonzero pattern is rejected. Some keys fail to sign at
# Origami-384 (sign-18-6), so the driver takes the first seed that signs.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
id=sign-18-8
command -v gcc >/dev/null 2>&1 || { echo "SKIP $id: gcc unavailable"; exit 77; }
command -v timeout >/dev/null 2>&1 || { echo "SKIP $id: timeout unavailable"; exit 77; }
bad() { echo "NOT CONFIRMED $id: $1"; exit 1; }

impl='sign-18/Implementations and Test_Vectors/Implementations'
for t in Reference Optimized; do
    for l in 128 256 384 512; do
        d="$impl/${t}_Implementation/Origami-$l"
        c=origami_ref.c; [ $t = Optimized ] && c=origami_opt.c
        tr -d '\r' <"$d/symmetric_iccs.c" | grep -Fxq 'static void shake_do_squeeze(uint8_t *out, size_t outlen, shake_t *instance) {' &&
            [ "$(tr -d '\r' <"$d/symmetric_iccs.c" | grep -Fxc '        memset(out, 0, outlen);')" -ge 2 ] &&
            tr -d '\r' <"$d/$c" | grep -Fxq '    shake_squeeze(bytes, sizeof(bytes), &st);' &&
            tr -d '\r' <"$d/$c" | grep -Fxq '    hash_to_field(expected, digest, len_digest, salt);' &&
            tr -d '\r' <"$d/$c" | grep -Fxq '        if (result[i] != expected[i]) return -1;' ||
            bad "$t Origami-$l target derivation differs from the archive"
        if [ -f "$d/Makefile" ]; then
            grep -Fq -- '-std=c11 -O3' "$d/Makefile" || bad "$t Origami-$l Makefile flags differ from the archived ones"
        fi
    done
done
echo 'source: shake_do_squeeze zero-fills on allocation or pseudoXOF failure and returns void; hash_to_field and verify ignore it (8/8 trees)'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
for t in Reference Optimized; do
    for l in 128 256 384 512; do
        d="$impl/${t}_Implementation/Origami-$l"
        if [ $t = Reference ]; then c=origami_ref.c; x='-DORIGAMI_OPT=REF'
        else c=origami_opt.c; x='-DORIGAMI_OPT=OPT -DORIGAMI_OPTIMIZED_PROFILE'; fi
        (cd "$d" && gcc -std=c11 -O3 -w -DKAT_NUM_TESTS=1 $x -I. -Wl,--wrap=malloc -o "$scratch/$t-$l" \
            "$here/reproduce_zero_target.c" SIG_AlgorithmInstance.c drng.c auxfunc.c symmetric_iccs.c $c aes.c) \
            >"$scratch/$t-$l.log" 2>&1 &
    done
done
wait

for t in Reference Optimized; do
    for l in 128 256 384 512; do
        b="$scratch/$t-$l"
        [ -x "$b" ] || { cat "$b.log"; bad "$t Origami-$l build failed"; }
        (timeout 300 "$b" >"$b.out" 2>/dev/null; echo $? >"$b.rc") &
    done
done
wait
n=0
for t in Reference Optimized; do
    for l in 128 256 384 512; do
        b="$scratch/$t-$l"
        out=$(cat "$b.out")
        [ "$(cat "$b.rc")" = 0 ] || bad "$t Origami-$l run failed: $out"
        case "$out" in *'zero sig, message B, no fault: rc=-1'*'honest sig, message A, no fault: rc=0'*) ;;
            *) bad "$t Origami-$l fault-free controls: $out" ;; esac
        triple=$(echo "$out" | sed -n 's/^triple at allocation [0-9]*: //p')
        [ "$(echo "$triple" | wc -w)" = 3 ] || bad "$t Origami-$l: target-derivation allocations not found ($out)"
        [ "$(echo "$out" | grep -c '^fault [0-9]*:[0-9]*: zero/B rc=0 zero/A rc=0 pattern/B rc=-1$')" = 3 ] ||
            bad "$t Origami-$l faulted runs: $out"
        echo "$t Origami-$l: failing any of the target allocations [$triple] (size:occurrence) makes the zero signature verify for two messages; nonzero pattern rejected; fault-free controls hold"
        n=$((n + 1))
    done
done
echo "ATTACK $id: in $n Origami builds one failed allocation in target derivation makes the all-zero signature verify for any message: CONFIRMED"
exit 0
