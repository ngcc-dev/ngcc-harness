#!/bin/sh
# kem-34-7: Rudraksh2's kem_enc discards the status of hash_g and copies the
# uninitialised stack buffer kr into the shared secret and the coins. Called
# again from the same place, it returns the previous session's key.
#
# Checks the uninitialised kr and the unchecked hash_g in all nine trees, then
# builds every reference and optimized set with malloc wrapped so that exactly
# one allocation of the size hash_g's pseudoXOF requests fails. Alice
# encapsulates to Mallory, then to Bob with the failure: Mallory's own
# decapsulated key must equal the Alice-Bob key. Controls: no failure, and the
# failure after the stack is overwritten between the sessions.
set -u
here=$(cd "$(dirname "$0")" && pwd)
[ -d kem-34/Implementations ] || cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-34-7: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED kem-34-7: $1"; exit 1; }

impl=kem-34/Implementations
for d in "$impl"/Reference_Implementation/lwekem* "$impl"/Optimized_Implementation/lwekem* \
         "$impl"/Additional_Implementations/Cortex_M4/lwekem*/crypto_kem/Rudraksh2/Rudraksh2; do
    k=$(ls "$d"/KEM_lwekem*.c)
    grep -q '^	uint8_t kr\[2 \* KEM_SYMBYTES\];' "$k" &&
        grep -Fq '	hash_g (kr, buf, 2 * KEM_SYMBYTES);' "$k" &&
        grep -Fq '	memcpy (ss, kr, KEM_SYMBYTES);' "$k" &&
        grep -Fq 'pseudoXOF (2 * KEM_SSBYTES * 8, IN, INBYTES * 8, OUT)' "$d/symmetric.h" ||
        bad "$d differs from the archive"
done
echo 'source: all nine trees (reference, optimized, Cortex-M4) leave kr uninitialised and ignore hash_g failures in kem_enc'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
for tree in Reference Optimized; do
    for s in 128 256 512; do
        d=$impl/${tree}_Implementation/lwekem$s
        x=
        [ $tree = Optimized ] && x="$(cd "$d" && ls *.S) -mavx2 -mbmi2 -mpopcnt"
        (cd "$d" && gcc -O2 -w -I. -DKEM_HEADER="\"KEM_lwekem$s.h\"" -Wl,--wrap=malloc -o "$scratch/t" \
            "$here/reproduce_stale_encaps_key.c" auxfunc.c drng.c cbd.c indcpa.c minal.c ntt.c poly.c \
            polyvec.c reduce.c verify.c KEM_lwekem$s.c $x) 2>"$scratch/cc" ||
            { cat "$scratch/cc"; bad "$tree lwekem$s build failed"; }
        out=$("$scratch/t" 2>/dev/null) || bad "$tree lwekem$s run failed"
        echo "$tree lwekem$s: $(echo "$out" | tr '\n' ' ')"
        case "$out" in *'control: rc=0,0 alloc=0 mallory_has_M=1 mallory_predicts_AB=0 bob_agrees=1'*) ;; *) bad "$tree lwekem$s control differs" ;; esac
        case "$out" in *'faulted: rc=0,0 alloc=3 mallory_has_M=1 mallory_predicts_AB=1 bob_agrees=0'*) ;; *) bad "$tree lwekem$s faulted run differs" ;; esac
        case "$out" in *'scrubbed: rc=0,0 alloc=3 mallory_has_M=1 mallory_predicts_AB=0 bob_agrees=0'*) ;; *) bad "$tree lwekem$s scrubbed run differs" ;; esac
    done
done
echo 'ATTACK kem-34-7: in all six x86 Rudraksh2 builds a failed hash_g allocation makes kem_enc return 0 with the previous session key, which an earlier recipient already holds: CONFIRMED'
exit 0
