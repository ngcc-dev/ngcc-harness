#!/bin/sh
# kem-40-6: YuanYang's encapsulation ignores a failure of the hash that turns
# msg || Hash(pk) || 42 into K || seed (kem.c:163). On that path the
# encryption coins are Hash(pk) || 45 and the pre-key is msg, so anyone with
# pk and ct recovers msg and the returned shared secret.
#
# Checks the unchecked call in all six trees, then builds every reference and
# optimized instance with malloc wrapped so that exactly one allocation of the
# size that hash requests fails. Control: the attacker cannot reproduce an
# unfaulted ciphertext, which decapsulates correctly. Faulted: rc is 0 and the
# attacker's recomputed ciphertext and shared secret match.
set -u
here=$(cd "$(dirname "$0")" && pwd)
[ -d kem-40/Implementations ] || cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-40-6: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED kem-40-6: $1"; exit 1; }
ulimit -s unlimited 2>/dev/null || ulimit -s "$(ulimit -H -s)" 2>/dev/null

for tree in Reference Optimized; do
    for s in 512 1024 2048; do
        d=kem-40/Implementations/${tree}_Implementation/yuanyang-$s
        grep -Fq '	pseudohash(1024,tmphash,2*YY_SEC+8,tmphash); // tmphash=H(msg||Hash(pk)||42)=K||seed' "$d/kem.c" &&
            grep -Fq '	yy_encode_message(ct,ct_len_bytes,tmphash+YY_SEC/8,msg,h);' "$d/kem.c" &&
            grep -Fq 'return MEMORY_ALLOCATION_FAILED;' "$d/auxfunc.c" ||
            bad "$tree yuanyang-$s differs from the archive"
    done
done
echo 'source: all six trees discard the status of the K || seed hash in yy_encapsulate_API'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
for tree in Reference Optimized; do
    for s in 512 1024 2048; do
        d=kem-40/Implementations/${tree}_Implementation/yuanyang-$s
        (cd "$d" && gcc -O2 -w -I. -Wl,--wrap=malloc -o "$scratch/t" "$here/reproduce_public_coins.c" \
            prng.c auxfunc.c drng.c ntt.c codec.c poly_inv_ntt.c KEM_AlgorithmInstance.c kem-keygen.c -lm) \
            2>"$scratch/cc" || { cat "$scratch/cc"; bad "$tree yuanyang-$s build failed"; }
        out=$("$scratch/t" 2>/dev/null) || bad "$tree yuanyang-$s run failed"
        echo "$tree yuanyang-$s: $(echo "$out" | tr '\n' ' ')"
        case "$out" in *'control: rc=0 rd=0 dec_agrees=1 attacker_ok=0'*) ;; *) bad "$tree yuanyang-$s control differs" ;; esac
        case "$out" in *'faulted: alloc='*' rc=0 attacker_ok=1 attacker_matches=1 dec_agrees=0'*) ;; *) bad "$tree yuanyang-$s faulted run differs" ;; esac
    done
done
echo 'ATTACK kem-40-6: in all six YuanYang builds one failed hash allocation makes encapsulation return 0 under public coins; the shared secret is recomputed from pk and ct: CONFIRMED'
exit 0
