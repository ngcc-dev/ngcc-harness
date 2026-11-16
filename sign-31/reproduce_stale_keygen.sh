#!/bin/sh
# sign-31-8: TSUOV key generation ignores a failed oil-matrix expansion, so the
# published key is built from stale stack bytes that hold public data, and
# anyone can sign under it.
#
# Checks that all six trees share the void TSUOV_KeyGen and the unchecked
# RejSampPRG_sm3, then builds each reference level with malloc and pseudoXOF
# wrapped so that exactly one allocation (pseudoXOF's first, inside Expand_S)
# fails. "prev": after an earlier keygen, an attacker who knows only the earlier
# public key and the new one must forge a signature under the new key at all
# three levels. "fresh": the faulted keygen is the process's first call and the
# attacker assumes zeros; required at 256 and 512 (at 128 the slot is not all
# zero and the forgery must fail). Controls without the fault must not forge,
# and the holder's own signature must verify only without the fault.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-31-8: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-31-8: $1"; exit 1; }

top=sign-31/Implementations
ref="$top/Digital_Signature-TSUOV-x86-Reference_Implementation/API_PKC/Implementations/Reference_Implementation"
opt="$top/Digital_Signature-TSUOV-x86-Performance_Optimized_Implementation/API_PKC/Implementations/Reference_Implementation"
for d in "$ref"/TSUOV_* "$opt"/TSUOV_*; do
    grep -Fq 'void TSUOV_KeyGen(const TSUOV_SEED seed_sk, const TSUOV_SEED seed_pk, TSUOV_P3 P3)' "$d/tsuov_core.c" &&
        grep -Fq '    pseudoXOF((unsigned long long)tau * 8, msg,' "$d/tsuov_core.c" &&
        grep -Fq '    TSUOV_KeyGen(seed_sk, seed_pk, P3);' "$d/SIG_AlgorithmInstance.c" &&
        grep -Fq 'return MEMORY_ALLOCATION_FAILED;' "$d/auxfunc.c" ||
        bad "$d differs from the archive"
done
echo 'source: all six trees discard pseudoXOF status in RejSampPRG_sm3; TSUOV_KeyGen is void and sig_keygen returns 0'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
driver=$PWD/sign-31/reproduce_stale_keygen.c
for level in 128 256 512; do
    mkdir "$scratch/$level" && cp "$ref/TSUOV_$level"/*.c "$ref/TSUOV_$level"/*.h "$scratch/$level/" || bad "copy failed"
    (cd "$scratch/$level" && gcc -O2 -std=c99 -w -I. -DTSUOV_VARIANT=$level \
        -Wl,--wrap=malloc -Wl,--wrap=pseudoXOF -o g "$driver" \
        SIG_AlgorithmInstance.c auxfunc.c drng.c tsuov_core.c mgf.c Fql.c -lm) \
        2>"$scratch/cc" || { cat "$scratch/cc"; bad "TSUOV-$level build failed"; }
    if [ "$level" = 128 ]; then fresh=0; else fresh=1; fi
    for mode in 'prev' 'prev control' 'fresh' 'fresh control'; do
        out=$("$scratch/$level/g" $mode 2>/dev/null) || bad "TSUOV-$level $mode run failed"
        echo "TSUOV-$level $out"
        case "$mode" in
            *control) want="$mode: keygen=0 holder-signature-verifies=1 attacker-forgery(m3)-verifies=0" ;;
            prev) want='prev: keygen=0 holder-signature-verifies=0 attacker-forgery(m3)-verifies=1' ;;
            fresh) want="fresh: keygen=0 holder-signature-verifies=0 attacker-forgery(m3)-verifies=$fresh" ;;
        esac
        [ "$out" = "$want" ] || bad "TSUOV-$level $mode behaviour differs"
    done
done
echo 'ATTACK sign-31-8 TSUOV-128/256/512: keygen returns 0 after the oil-matrix allocation fails, and an attacker with only public keys forges under the new key; CONFIRMED'
exit 0
