#!/bin/sh
# sign-31-9: TSUOV sig_verify keeps the expanded public key in a static buffer,
# so honest signatures are rejected when threads verify under different public
# keys at the same time, and a signature that is invalid for the victim's key
# is accepted under it while other threads verify under an attacker's key.
#
# Checks that the three reference and three optimized trees (each shipped in
# both packages) declare the static buffer in sig_verify, then builds each
# reference level with the submission's flags (-O2 -std=c99, no sanitizer) plus
# -pthread for the driver. The driver makes two honest key pairs and one honest
# signature under each. Controls, which must not fail: one thread verifying
# both signatures in turn, four threads all verifying under one key, and one
# thread again afterwards with unchanged inputs. Test: four threads, two per
# key, verify the same honest signatures for a fixed number of rounds, and at
# least one verification must fail. The failure count depends on thread
# scheduling and varies from run to run.
#
# Then, at TSUOV-128, the same driver with "forge": from pk_V's seed only, it
# picks a random signature value S on a fresh message and solves the
# verifier's equations, which are affine in the P3 entries, for a P3_X that it
# stores behind arbitrary seed bytes as pk_X (the attacker's model of the
# verifier is first checked against both honest signatures). Single thread:
# the unmodified TSUOV_Verify(seed_V, P3_X, m, S) must accept and
# sig_verify(pk_V, S, m) must reject. Controls, which must never accept: one
# thread verifying (pk_V, S, m); four threads where three verify the honest
# signature under the second key; one thread again afterwards with unchanged
# inputs. Test: four threads where three call sig_verify(pk_X, junk, junk),
# and sig_verify(pk_V, S, m) must return 0 at least once in a fixed number of
# rounds. The count also depends on scheduling.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-31-9: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-31-9: $1"; exit 1; }

top=sign-31/Implementations
trees=0
for pkg in Reference_Implementation Performance_Optimized_Implementation; do
    impl="$top/Digital_Signature-TSUOV-x86-$pkg/API_PKC/Implementations"
    for d in "$impl"/Reference_Implementation/TSUOV_*; do
        [ "$(grep -Fxc '    static TSUOV_P3 P3;' "$d/SIG_AlgorithmInstance.c")" = 2 ] &&
            grep -Fxq '    restore_TSUOV_P3(pk, &pk_pool_bits, P3);' "$d/SIG_AlgorithmInstance.c" ||
            bad "$d differs from the archive"
        trees=$((trees + 1))
    done
    for d in "$impl"/Optimized_Implementation/TSUOV_*; do
        grep -Fq 'static TSUOV_P3 P3      ; // to avoid huge array in stack' "$d/SIG_AlgorithmInstance.c" &&
            grep -Fq '// (but problematic in mult-thread environment)' "$d/SIG_AlgorithmInstance.c" ||
            bad "$d differs from the archive"
        trees=$((trees + 1))
    done
done
[ "$trees" = 12 ] || bad "expected 12 trees, found $trees"
echo 'source: sig_verify declares "static TSUOV_P3 P3" in all three reference and all three optimized trees; the optimized trees comment "(but problematic in mult-thread environment)"'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
driver=$PWD/sign-31/reproduce_thread_race.c
ref="$top/Digital_Signature-TSUOV-x86-Reference_Implementation/API_PKC/Implementations/Reference_Implementation"
for spec in 128:100 256:20 512:5; do
    level=${spec%:*}
    rounds=${spec#*:}
    n=$((4 * rounds))
    mkdir "$scratch/$level" && cp "$ref/TSUOV_$level"/*.c "$ref/TSUOV_$level"/*.h "$scratch/$level/" || bad "copy failed"
    (cd "$scratch/$level" && gcc -O2 -std=c99 -w -I. -DTSUOV_VARIANT=$level -pthread \
        -o t "$driver" SIG_AlgorithmInstance.c auxfunc.c drng.c tsuov_core.c mgf.c Fql.c -lm) \
        2>"$scratch/cc" || { cat "$scratch/cc"; bad "TSUOV-$level build failed"; }
    out=$("$scratch/$level/t" "$rounds" 2>/dev/null) || bad "TSUOV-$level run failed"
    echo "TSUOV-$level: $out"
    pre="one thread 0/$n fail; four threads one key 0/$n fail; four threads two keys "
    post="/$n fail; one thread afterwards 0/2 fail, inputs unchanged=1"
    case "$out" in
        "$pre"*"$post") ;;
        *) bad "TSUOV-$level a control failed" ;;
    esac
    failed=${out#"$pre"}
    failed=${failed%"$post"}
    case "$failed" in
        '' | *[!0-9]*) bad "TSUOV-$level unexpected output" ;;
    esac
    [ "$failed" -gt 0 ] || bad "TSUOV-$level no concurrent verification failed in $n"
done
echo 'CORRECTNESS sign-31-9 TSUOV-128/256/512: honest signatures under two public keys are rejected when four threads verify at once, because sig_verify keeps the public key in a static buffer; single-thread and one-key controls never fail; CONFIRMED'

rounds=1000
out=$("$scratch/128/t" "$rounds" forge 2>/dev/null) || bad "TSUOV-128 forge run failed"
echo "TSUOV-128 forge: $out"
pre="attacker model matches 2/2 honest signatures; random S on a fresh message, P3_X solved from 88 equations in 1220 unknowns (rank 88); single thread TSUOV_Verify(seed_V, P3_X, m, S)=1, sig_verify(pk_V, S, m)=-1; sig_verify(pk_V, S, m) accepted: one thread 0/$rounds; four threads, three under honest key 1, 0/$rounds; four threads, three under pk_X, "
post="/$rounds; one thread afterwards 0/1, inputs unchanged=1"
case "$out" in
    "$pre"*"$post") ;;
    *) bad "TSUOV-128 forge: a check or control failed" ;;
esac
hit=${out#"$pre"}
hit=${hit%"$post"}
case "$hit" in
    '' | *[!0-9]*) bad "TSUOV-128 forge: unexpected output" ;;
esac
[ "$hit" -gt 0 ] || bad "TSUOV-128 forge: no concurrent verification accepted S in $rounds"
echo 'FAIL-OPEN sign-31-9 TSUOV-128: sig_verify accepts, under the victim public key, a signature made without its secret key and rejected when verified alone, while other threads verify junk under an attacker public key whose P3 was solved for it; single-thread and honest-key controls never accept; CONFIRMED'
exit 0
