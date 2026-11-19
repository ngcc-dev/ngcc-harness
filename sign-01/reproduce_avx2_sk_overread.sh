#!/bin/sh
# sign-01: Aigis-Sig+ AVX2 key unpacking reads past exactly sized keys.
#   polyt0_unpack (packing.c) issues _mm256_loadu_si256 at a[pos] and
#   advances pos by PARAM_D (15 at sets I/II, 13 at set III); the t0
#   polynomials end the secret key, so signing reads 17 or 19 bytes past sk.
#   Run with the candidate's AVX2 flags: an exactly sized sk ending at a
#   PROT_NONE page faults; with the page readable, or with 17/19 bytes of
#   slack, signing returns 0 and the signature verifies; one byte less slack
#   still faults. The same check on pk during verification measures the
#   polyt1_unpack over-read (1 byte at I/II, 23 at III).
#   The NEON tree compiles the same packing.c through neon_compat.h, whose
#   _mm256_loadu_si256 is two 16-byte vld1q loads (checked by source only);
#   the reference and aarch64 trees unpack t0 with scalar code.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-01: gcc unavailable'; exit 77; }
for f in avx2 bmi2 popcnt; do
    grep -qw $f /proc/cpuinfo 2>/dev/null || { echo "SKIP sign-01: CPU lacks $f"; exit 77; }
done
bad() { echo "NOT CONFIRMED sign-01: $1"; exit 1; }
drv=$here/reproduce_avx2_sk_overread.c
impl=sign-01/Implementations/Implementations
for d in "$impl"/Optimized_Implementation/avx2/Aigis-Sig+-* "$impl"/Additional_Implementation/neon/Aigis-Sig+-*; do
    v=$(tr -d '\r' < "$d/packing.c" | sed -n '/^void polyt0_unpack/,/^}/p')
    echo "$v" | grep -q 'tmp = _mm256_loadu_si256((__m256i \*)&a\[pos\]);' &&
        echo "$v" | grep -q 'pos += PARAM_D;' || bad "$d/packing.c differs from the archive"
done
grep -q 'r.hi = vld1q_s32(q + 4);' "$impl"/Additional_Implementation/neon/Aigis-Sig+-I/neon_compat.h || bad "neon_compat.h differs"
echo 'source: AVX2 and NEON polyt0_unpack load 32 bytes per PARAM_D-byte step (NEON via neon_compat.h)'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
ulimit -c 0 2>/dev/null
src='sign.c polyvec.c packing.c poly.c ntt.c fips202.c hashkdf.c pspm.c fips202x4.c keccak4x/KeccakP-1600-times4-SIMD256.c auxfunc.c SIG_AlgorithmInstance.c drng.c sample.c rng.c'
for s in 1:I:17:1 2:II:17:1 3:III:19:23; do
    p=${s%%:*}; r=${s#*:}; n=${r%%:*}; r=${r#*:}; over=${r%%:*}; pkover=${r#*:}
    (cd "$impl/Optimized_Implementation/avx2/Aigis-Sig+-$n" &&
        gcc -O3 -march=native -mtune=native -fomit-frame-pointer -mpopcnt -std=gnu99 -w -Wno-incompatible-pointer-types \
            -DPARAMS=$p -DUSE_ICCS -I. -o "$scratch/a$n" "$drv" $src) >"$scratch/cc" 2>&1 ||
        { cat "$scratch/cc"; bad "AVX2 Aigis-Sig+-$n build failed"; }
    x=$scratch/a$n
    "$x" ctrl >"$scratch/o" 2>&1 && grep -q 'ctrl: sign rc=0 verify rc=0' "$scratch/o" || { cat "$scratch/o"; bad "set $n readable control failed"; }
    "$x" guard $over >"$scratch/o" 2>&1 && grep -q 'guard: sign rc=0 verify rc=0' "$scratch/o" || bad "set $n $over-byte slack failed"
    "$x" guard $((over - 1)) >/dev/null 2>&1; r1=$?
    "$x" guard 0 >/dev/null 2>&1; r0=$?
    [ $r0 -ge 128 ] && [ $r1 -ge 128 ] || bad "set $n did not fault (rc=$r0/$r1)"
    echo "AVX2 Aigis-Sig+-$n sign: sk at PROT_NONE page: signal $((r0 - 128)); $((over - 1)) bytes slack: signal $((r1 - 128)); $over bytes slack or readable page: sign 0, verify 0"
    "$x" pkctrl >"$scratch/o" 2>&1 && grep -q 'pkctrl: verify rc=0' "$scratch/o" || bad "set $n pk control failed"
    "$x" pkguard $pkover >"$scratch/o" 2>&1 && grep -q 'pkguard: verify rc=0' "$scratch/o" || bad "set $n pk $pkover-byte slack failed"
    "$x" pkguard $((pkover - 1)) >/dev/null 2>&1; r1=$?
    [ $r1 -ge 128 ] || bad "set $n pk did not fault"
    echo "AVX2 Aigis-Sig+-$n verify: pk with $((pkover - 1)) bytes slack before PROT_NONE page: signal $((r1 - 128)); $pkover bytes or readable page: verify 0"
done
echo 'ATTACK sign-01-12: AVX2 signing reads 17 (sets I/II) or 19 (set III) bytes past an exactly sized secret key, and AVX2 verification 1 or 23 bytes past an exactly sized public key: CONFIRMED'
exit 0
