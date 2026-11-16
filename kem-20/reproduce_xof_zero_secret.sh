#!/bin/sh
# kem-20-3: MAMBA-Frost's void shake wrapper zeroes its output when pseudoXOF
# fails and the KEM carries on, so encapsulation and decapsulation return 0
# with a shared secret computable from the ciphertext alone.
#
# Checks the void api_pseudo_xof and the unchecked shake calls in all 20
# trees, then builds every reference and optimized instance with malloc
# wrapped so that exactly one pseudoXOF allocation fails. Control: an
# unfaulted round trip agrees. Faults: the G hash in encapsulation
# (ss = shake(ct || 0)), the final hash in encapsulation (ss = 0) and the
# final hash in decapsulation of an honest ciphertext (ss = 0).
set -u
here=$(cd "$(dirname "$0")" && pwd)
[ -d kem-20/Implementations ] || cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-20-3: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED kem-20-3: $1"; exit 1; }
ulimit -s unlimited 2>/dev/null || ulimit -s "$(ulimit -H -s)" 2>/dev/null

sets='128 192 256 384 512 CC-128 CC-192 CC-256 CC-384 CC-512'
for tree in Reference Optimized; do
    for s in $sets; do
        d=kem-20/Implementations/${tree}_Implementation/MAMBA-Frost-$s
        k=$(ls "$d"/Frost*/src/kem.c)
        grep -Fq 'static void api_pseudo_xof(' "$d/common/sha3/fips202.c" &&
            grep -Fq 'clear_output(output, outlen);' "$d/common/sha3/fips202.c" &&
            grep -Fq '    shake(G2out, BYTES_SEED_SE + CRYPTO_BYTES, G2in, BYTES_PKHASH + BYTES_MU + BYTES_SALT);' "$k" &&
            [ "$(grep -c '    shake(ss, CRYPTO_BYTES, Fin, CRYPTO_CIPHERTEXTBYTES + CRYPTO_BYTES);' "$k")" = 2 ] ||
            bad "$tree MAMBA-Frost-$s differs from the archive"
    done
done
echo 'source: all 20 trees route shake through a void api_pseudo_xof and ignore failures of the G and final hashes'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
for tree in Reference Optimized; do
    for s in $sets; do
        d=kem-20/Implementations/${tree}_Implementation/MAMBA-Frost-$s
        core=$(ls "$d"/Frost*/src/frost*[0-9].c)
        fam=$(dirname "$core")
        x=
        case $s in *512) [ $tree = Reference ] && x=-DFROST_U16_STREAMING_MATMUL ;; esac
        if [ $tree = Reference ]; then
            flags="-O2 -DNIX -D_AMD64_ -D_REFERENCE_ -D_AES128_FOR_A_ -DFROST_USE_E8_CODE $x"
            aes=common/aes/aes_c.c
        else
            flags="-O2 -march=x86-64 -mavx2 -maes -DNIX -D_AMD64_ -D_FAST_ -D_AES128_FOR_A_ -DFROST_USE_E8_CODE"
            aes=common/aes/aes_ni.c
        fi
        (cd "$d" && gcc $flags -w -I. -Wl,--wrap=malloc -o "$scratch/t" "$here/reproduce_xof_zero_secret.c" \
            "${core#$d/}" "${fam#$d/}/util.c" common/sha3/fips202.c $aes \
            KEM_AlgorithmInstance.c randombytes_adapter.c drng.c auxfunc.c -lm) 2>"$scratch/cc" ||
            { cat "$scratch/cc"; bad "$tree MAMBA-Frost-$s build failed"; }
        out=$("$scratch/t" 2>/dev/null) || bad "$tree MAMBA-Frost-$s run failed"
        echo "$tree MAMBA-Frost-$s: $(echo "$out" | tr '\n' ' ')"
        case "$out" in *'control: rc=0 rd=0 dec_agrees=1 ss_zero=0'*) ;; *) bad "$tree MAMBA-Frost-$s control differs" ;; esac
        case "$out" in *'G: alloc=3 '*' rc=0 attacker_matches=1 dec_agrees=0'*) ;; *) bad "$tree MAMBA-Frost-$s G fault differs" ;; esac
        case "$out" in *'Fenc: alloc='*' rc=0 attacker_matches=1 dec_agrees=0'*) ;; *) bad "$tree MAMBA-Frost-$s final-enc fault differs" ;; esac
        case "$out" in *'Fdec: alloc='*' rc=0 rd=0 attacker_matches=1 dec_agrees=0'*) ;; *) bad "$tree MAMBA-Frost-$s final-dec fault differs" ;; esac
    done
done
echo 'ATTACK kem-20-3: in all 20 MAMBA-Frost builds one failed pseudoXOF allocation makes encapsulation or decapsulation return 0 with a shared secret computed from the ciphertext alone: CONFIRMED'
exit 0
