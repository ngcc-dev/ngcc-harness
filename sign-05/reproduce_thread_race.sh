#!/bin/sh
# sign-05-7: Chinith verification keeps per-call scratch buffers in the
# file-scope caches g_bavc_scratch (bavc.c) and g_vole_scratch (vole.c), so
# honest signatures are rejected when several threads verify at the same time.
#
# Checks that all 28 reference and optimized trees declare both caches, then
# builds two reference instances in a temporary directory with the flags of
# their submitted Makefile (-O3 -flto=auto -DNDEBUG, no sanitizer) plus
# -pthread for the driver. The driver makes four honest key pairs and one
# honest signature under each. Controls, which must not fail: one thread
# verifying the four signatures in turn, before and after the test, with
# unchanged inputs. Test: four threads, one per key, verify their own honest
# signature for a fixed number of rounds, and at least one verification must
# fail. The failure count depends on thread scheduling and varies from run to
# run.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-05-7: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-05-7: $1"; exit 1; }

top=sign-05/Implementations
trees=0
for d in "$top"/Reference_Implementation/*/ "$top"/Optimized_Implementation/*/; do
    grep -Fq 'static bavc_scratch_cache_t g_bavc_scratch = {0};' "$d/bavc.c" &&
        grep -Fq 'static vole_scratch_cache_t g_vole_scratch = {0};' "$d/vole.c" ||
        bad "$d differs from the archive"
    trees=$((trees + 1))
done
[ "$trees" = 28 ] || bad "expected 28 trees, found $trees"
echo 'source: bavc.c declares "static bavc_scratch_cache_t g_bavc_scratch" and vole.c "static vole_scratch_cache_t g_vole_scratch" in all 14 reference and all 14 optimized trees'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
driver=$PWD/sign-05/reproduce_thread_race.c
ref="$top/Reference_Implementation"
flags='CFLAGS = -O2 -std=c11 -pedantic -Wall -Wextra -I. -I./utils_sm4 -include fallbacks.h -DSM4_SBOX_TABLE'
for spec in sm4th_em_d2_128f_loose:25 sm4th_d3_128f_loose:10; do
    inst=${spec%:*}
    rounds=${spec#*:}
    n=$((4 * rounds))
    grep -Fxq "$flags" "$ref/$inst/Makefile" || bad "$inst Makefile differs from the archive"
    srcs=$(sed -n 's/^SOURCES = //p' "$ref/$inst/Makefile")
    cp -R "$ref/$inst" "$scratch/$inst" || bad "copy failed"
    (cd "$scratch/$inst" &&
        gcc -O2 -std=gnu11 -w -I. -pthread -c -o driver.o "$driver" &&
        gcc -O2 -std=c11 -pedantic -w -I. -I./utils_sm4 -include fallbacks.h -DSM4_SBOX_TABLE \
            -DNDEBUG -O3 -flto=auto -pthread -o t driver.o $srcs -lm) \
        2>"$scratch/cc" || { cat "$scratch/cc"; bad "$inst build failed"; }
    out=$("$scratch/$inst/t" "$rounds" 2>/dev/null) || bad "$inst run failed"
    echo "$inst: $out"
    pre="one thread 0/$n fail; four threads "
    post="/$n fail; one thread afterwards 0/4 fail, inputs unchanged=1"
    case "$out" in
        "$pre"*"$post") ;;
        *) bad "$inst a control failed" ;;
    esac
    failed=${out#"$pre"}
    failed=${failed%"$post"}
    case "$failed" in
        '' | *[!0-9]*) bad "$inst unexpected output" ;;
    esac
    [ "$failed" -gt 0 ] || bad "$inst no concurrent verification failed in $n"
done
echo 'CORRECTNESS sign-05-7 Chinith sm4th_em_d2_128f_loose, sm4th_d3_128f_loose: honest signatures are rejected when four threads verify at once, because verification keeps its scratch buffers in the file-scope caches g_bavc_scratch and g_vole_scratch; single-thread controls never fail; CONFIRMED'
exit 0
