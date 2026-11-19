#!/bin/sh
# sign-08: DARTS public-key reads and the key-generation N(s) test.
#   (a) AVX2 poly_q_unpack loads 32 bytes per 17 (DARTS128/256) or 18
#       (DARTS512) consumed, so verification reads 15 or 14 bytes past a
#       valid, exactly sized pk: SIGSEGV when pk ends at a PROT_NONE page,
#       rc=0 when that page is readable or when 15/14 bytes of slack are left.
#       Six AVX2 trees by source (DARTS_AVX2 and the FIPS202 optimized copy);
#       the three DARTS_AVX2 sets at run time with the candidate's flags.
#   (b) sig_verify discards pk_len_bytes (all nine SIG wrappers): a valid pk
#       in a heap buffer one byte short, declared at that length, is read past
#       the buffer under AddressSanitizer (reference); full length is clean.
#   (c) polyveckl_sqsing_value sums N(s)*1024 in int32_t res (all 15 trees).
#       DARTS256/512 values exceed 2^31, wrap, and always pass the
#       N(s) <= GAMMA^2*N test of Algorithm 6, so sig_keygen returns keys that
#       the specification rejects. Checked on 100 reference keys per set by
#       recomputing N(s) from the unpacked secret with 64-bit integers.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-08: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-08: $1"; exit 1; }
drv=$here/reproduce_pk_reads.c
impl=sign-08/Implementations
[ -d "$impl/Reference_Implementation/DARTS128" ] || bad "tree not found"

n=0
for f in $(find "$impl/" -name polyvec.c | sort); do
    sed -n '/polyveckl_sqsing_value(const poly/,/^}/p' "$f" | grep -q '^    int32_t res = 0;$' || bad "$f differs"
    n=$((n + 1))
done
[ $n = 15 ] || bad "expected 15 polyvec.c, found $n"
n=0
for f in $(grep -l '_mm256_loadu_si256((__m256i \*)&a\[17 \* i\]);' $(find "$impl/" -name poly.c)); do
    grep -q '_mm256_loadu_si256((__m256i \*)&a\[18 \* i\]);' "$f" || bad "$f differs"
    n=$((n + 1))
done
[ $n = 6 ] || bad "expected 6 AVX2 poly.c, found $n"
n=0
for f in $(find "$impl/" -name SIG_AlgorithmInstance.c); do
    tr -d '\r' < "$f" | sed -n '/^int sig_verify/,/^}/p' | grep -q '(void)pk_len_bytes;' || bad "$f checks pk_len_bytes"
    n=$((n + 1))
done
[ $n = 9 ] || bad "expected 9 wrappers, found $n"
echo 'source: 15/15 trees sum N(s) in int32_t; 6/6 AVX2 trees use 32-byte loads in poly_q_unpack; 9/9 sig_verify wrappers drop pk_len_bytes'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
ulimit -c 0 2>/dev/null
refsrc='sign.c packing.c polyvec.c poly.c ntt.c reduce.c sampler.c encoding.c polyfix.c polymat.c fft.c fixpoint.c symmetric.c drng.c auxfunc.c SIG_AlgorithmInstance.c'
avxsrc='sign.c packing.c polyvec.c poly.c ntt.c consts.c sampler.c samplerx4.c encoding.c polyfix.c polymat.c fft.c fixpoint.c symmetric.c drng.c auxfunc.c SIG_AlgorithmInstance.c'

# (c) and (b) on the reference sets
echo 'int main(void){return 0;}' >"$scratch/p.c"
asan=1; gcc -fsanitize=address -o "$scratch/p" "$scratch/p.c" 2>/dev/null && "$scratch/p" || asan=0
ASAN_OPTIONS=detect_leaks=0; export ASAN_OPTIONS
for s in 128 256 512; do
    d=$impl/Reference_Implementation/DARTS$s
    (cd "$d" && gcc -O2 -std=gnu99 -fcommon -w -DSQSING -I. -o "$scratch/q$s" "$drv" $refsrc -lm) >"$scratch/cc" 2>&1 ||
        { cat "$scratch/cc"; bad "DARTS$s build failed"; }
    out=$("$scratch/q$s" sqsing 100) || bad "DARTS$s keygen run failed"
    echo "$out"
    above=$(echo "$out" | sed -n 's/.* exact_N(s)_above_bound=\([0-9]*\) .*/\1/p')
    echo "$out" | grep -q 'code_value_below_bound=100 ' || bad "DARTS$s: code rejects some keys"
    if [ $s = 128 ]; then [ "$above" = 0 ] || bad "DARTS128 keys above bound"
    else [ "${above:-0}" -gt 0 ] || bad "DARTS$s: no key above bound"; fi
    [ $asan = 1 ] || continue
    (cd "$d" && gcc -O1 -g -std=gnu99 -fcommon -w -fsanitize=address -fno-omit-frame-pointer -I. -o "$scratch/a$s" "$drv" $refsrc -lm) \
        >"$scratch/cc" 2>&1 || { cat "$scratch/cc"; bad "DARTS$s ASan build failed"; }
    full=$("$scratch/a$s" full 2>&1)
    echo "$full" | grep -q 'verify rc=0' && ! echo "$full" | grep -q AddressSanitizer || { echo "$full"; bad "DARTS$s full-length control failed"; }
    short=$("$scratch/a$s" short 2>&1)
    echo "$short" | grep -q 'heap-buffer-overflow' && echo "$short" | grep -q 'unpack_pk .*packing\.c' &&
        echo "$short" | grep -q 'located 0 bytes after' || { echo "$short" | head -20; bad "DARTS$s short pk not read past"; }
    echo "DARTS$s: full pk clean; $(echo "$short" | grep -m1 -o 'READ of size [0-9]*') in unpack_pk ($(echo "$short" | grep -A4 'READ of' | grep -m1 -o 'packing\.c:[0-9]*')), $(echo "$short" | grep -m1 -o 'located 0 bytes after [0-9]*-byte region')"
done
echo 'ATTACK sign-08-7: the int32_t N(s) sum wraps and DARTS256/512 sig_keygen returns keys that fail the specified N(s) <= GAMMA^2*N test: CONFIRMED'
if [ $asan = 1 ]; then
    echo 'sign-08-8 (b): sig_verify ignores pk_len_bytes and reads past a one-byte-short public key'
else
    echo 'SKIP sign-08-8: AddressSanitizer unavailable'; exit 77
fi

# (a) AVX2 guard page
grep -qw avx2 /proc/cpuinfo 2>/dev/null || { echo 'SKIP sign-08-8: CPU lacks AVX2'; exit 77; }
for s in 128:15 256:15 512:14; do
    v=${s%%:*}; over=${s#*:}
    d=$impl/Optimized_Implementation/DARTS_AVX2/DARTS$v
    (cd "$d" && gcc -O3 -march=native -mtune=native -std=gnu99 -fcommon -w -I. -o "$scratch/v$v" "$drv" $avxsrc -lm) \
        >"$scratch/cc" 2>&1 || { cat "$scratch/cc"; bad "AVX2 DARTS$v build failed"; }
    "$scratch/v$v" ctrl >"$scratch/o" 2>&1 && grep -q 'ctrl: verify rc=0' "$scratch/o" || bad "AVX2 DARTS$v readable control failed"
    "$scratch/v$v" guard $over >"$scratch/o" 2>&1 && grep -q 'guard: verify rc=0' "$scratch/o" || bad "AVX2 DARTS$v $over-byte slack failed"
    "$scratch/v$v" guard $((over - 1)) >"$scratch/o" 2>&1; r1=$?
    "$scratch/v$v" guard 0 >"$scratch/o" 2>&1; r0=$?
    [ $r0 -ge 128 ] && [ $r1 -ge 128 ] || bad "AVX2 DARTS$v did not fault at the guard page (rc=$r0/$r1)"
    echo "AVX2 DARTS$v: exact pk before PROT_NONE page: signal $((r0 - 128)); $((over - 1)) bytes slack: signal $((r1 - 128)); $over bytes slack or readable page: rc=0"
done
echo 'ATTACK sign-08-8: sig_verify reads past a one-byte-short public key, and AVX2 verification reads 15 (DARTS128/256) or 14 (DARTS512) bytes past an exactly sized one: CONFIRMED'
exit 0
