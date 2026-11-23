#!/bin/sh
# sign-24-6: Sigurd verification keeps its Reed-Solomon workspace (reference
# trees) and its whole verifier workspace (optimized trees) in static storage,
# so honest signatures are rejected when several threads verify different
# signatures at the same time.
#
# Checks that the three reference trees declare the static RS workspace and the
# three optimized trees the static RS and verifier workspaces, then builds the
# reference Sigurd-128 in a temporary directory with the CFLAGS of the
# submitted Implementations/Makefile (-std=c99 -O2, no sanitizer) plus -pthread
# for the driver. The driver makes four honest key pairs and one honest
# signature under each. Controls, which must not fail: one thread verifying
# the four signatures in turn, before and after the test, with unchanged
# inputs. Test: four threads, one per key, verify their own honest signature
# for a fixed number of rounds, and at least one verification must fail. The
# failure count depends on thread scheduling and varies from run to run.
#
# Then, unless the CPU lacks AVX2, builds the optimized Sigurd-128 in the same
# temporary directory with the OPT_CFLAGS of the submitted Makefile (-O3
# -mavx2 -march=native -flto ..., no sanitizer) and runs the driver with
# "accept": one honest key pair and one honest signature on a message, and the
# same message with one bit flipped. Controls, which must never accept: one
# thread verifying the genuine message three times and the altered message
# once in each of 50 rounds, and one thread again afterwards with unchanged
# inputs. Test: 32 child processes of 500 rounds each, one at a time per four
# CPUs and at most four at a time; in each, four threads share the key pair
# and signature, three verify the genuine message and one the altered message,
# and the altered message must be accepted at least once. The children run
# apart from the witness, so an abort or segfault in the optimized tree is
# counted as an abnormal exit; calls made before it still count. Both counts
# depend on scheduling. The whole witness took about 20 s on a 28-CPU x86-64
# machine and about 60 s when limited to four of its CPUs.
#
# Exit status: 0 both parts confirmed; 1 a check, build or control failed;
# 3 the altered message was never accepted (NOT REPRODUCED); 77 gcc missing,
# or the optimized part skipped for lack of AVX2 after the reference part.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-24-6: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-24-6: $1"; exit 1; }

top=sign-24/Implementations
for l in 128 256 512; do
    grep -Fq 'static RSWorkspace g_rs_workspace;' "$top/Reference_Implementation/Sigurd-$l/sig_core.c" ||
        bad "reference Sigurd-$l differs from the archive"
    grep -Fq 'static RSWorkspace g_rs_workspaces[2];' "$top/Optimized_Implementation/Sigurd-$l/sig_core.c" &&
        grep -Fq 'static VerifierWorkspace g_cached_verifier_workspace;' \
            "$top/Optimized_Implementation/Sigurd-$l/SIG_AlgorithmInstance.c" ||
        bad "optimized Sigurd-$l differs from the archive"
done
grep -Fxq 'CFLAGS ?= -std=c99 -O2 -Wall -Wextra' "$top/Makefile" || bad "Makefile differs from the archive"
echo 'source: sig_core.c declares "static RSWorkspace g_rs_workspace" in the three reference trees; the three optimized trees declare "static RSWorkspace g_rs_workspaces[2]" and "static VerifierWorkspace g_cached_verifier_workspace"'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
driver=$PWD/sign-24/reproduce_thread_race.c
level=128
rounds=100
n=$((4 * rounds))
cp -R "$top/Reference_Implementation/Sigurd-$level" "$scratch/$level" || bad "copy failed"
(cd "$scratch/$level" &&
    gcc -O2 -std=gnu11 -w -I. -pthread -c -o driver.o "$driver" &&
    gcc -std=c99 -O2 -w -I. -pthread -o t driver.o SIG_AlgorithmInstance.c sig_core.c rsencode.c \
        rsencode_common.c auxfunc.c drng.c -lm) \
    2>"$scratch/cc" || { cat "$scratch/cc"; bad "Sigurd-$level build failed"; }
out=$("$scratch/$level/t" "$rounds" 2>/dev/null) || bad "Sigurd-$level run failed"
echo "Sigurd-$level: $out"
pre="one thread 0/$n fail; four threads "
post="/$n fail; one thread afterwards 0/4 fail, inputs unchanged=1"
case "$out" in
    "$pre"*"$post") ;;
    *) bad "Sigurd-$level a control failed" ;;
esac
failed=${out#"$pre"}
failed=${failed%"$post"}
case "$failed" in
    '' | *[!0-9]*) bad "Sigurd-$level unexpected output" ;;
esac
[ "$failed" -gt 0 ] || bad "Sigurd-$level no concurrent verification failed in $n"
echo 'CORRECTNESS sign-24-6 Sigurd-128: honest signatures are rejected when four threads verify different signatures at once, because RS_encode uses one static workspace; single-thread controls never fail; CONFIRMED'

optflags='-std=c99 -O3 -DNDEBUG -Wall -Wextra -mavx2 -march=native -mtune=native -flto -fomit-frame-pointer -funroll-loops -falign-functions=32 -falign-loops=32 -fno-semantic-interposition -pthread'
grep -Fxq "OPT_CFLAGS := $optflags" "$top/Makefile" || bad "Makefile differs from the archive"
if ! gcc -march=native -dM -E -x c /dev/null 2>/dev/null | grep -q '__AVX2__'; then
    echo "SKIP sign-24-6 Sigurd-$level optimized: the CPU lacks AVX2, which the submission's optimized flags require; the reference result above stands"
    exit 77
fi
cp -R "$top/Optimized_Implementation/Sigurd-$level" "$scratch/opt$level" || bad "copy failed"
(cd "$scratch/opt$level" &&
    gcc -O2 -std=gnu11 -w -I. -pthread -c -o driver.o "$driver" &&
    gcc $optflags -w -I. -o t driver.o SIG_AlgorithmInstance.c sig_core.c rsencode.c \
        rsencode_common.c auxfunc.c drng.c -lm) \
    2>"$scratch/cc" || { cat "$scratch/cc"; bad "optimized Sigurd-$level build failed"; }
procs=32
rounds=500
cpus=$(nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null)
case "$cpus" in '' | *[!0-9]*) cpus=4 ;; esac
par=$((cpus / 4))
[ "$par" -ge 1 ] || par=1
[ "$par" -le 4 ] || par=4
out=$("$scratch/opt$level/t" accept "$procs" "$rounds" "$par" 2>/dev/null) || bad "optimized Sigurd-$level run failed"
printf '%s\n' "$out" | sed "s/^/Sigurd-$level optimized, /"
[ "$(printf '%s\n' "$out" | sed -n 1p)" = 'one thread: genuine message rejected 0/150, altered message accepted 0/50' ] &&
    [ "$(printf '%s\n' "$out" | sed -n 3p)" = 'one thread afterwards: genuine message rejected 0/1, altered message accepted 0/1, inputs unchanged=1' ] ||
    bad "optimized Sigurd-$level a control failed"
set -- $(printf '%s\n' "$out" | sed -n "2s/^four threads, three on the genuine message and one on the altered message, $procs processes of $rounds rounds: altered message accepted \\([0-9]*\\)\\/\\([0-9]*\\) .*/\\1 \\2/p")
[ $# = 2 ] || bad "optimized Sigurd-$level unexpected output"
if [ "$1" -eq 0 ]; then
    echo "NOT REPRODUCED sign-24-6 Sigurd-$level optimized: the altered message was not accepted in $2 concurrent verifications; the count depends on thread scheduling"
    exit 3
fi
echo 'FAIL-OPEN sign-24-6 Sigurd-128 optimized: sig_verify accepts a genuine signature for a message with one bit flipped, rejected when verified alone, while three other threads verify that signature on the genuine message under the same key; single-thread controls never accept; CONFIRMED'
exit 0
