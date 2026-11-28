#!/bin/sh
# sign-25-6: SQIsign2D2 verification ignores the declared signature length.
#   Source: in all 16 source directories sig_verify casts sn_len_bytes to
#   void and fp_decode copies FP_ENCODED_BYTES bytes from the signature.
#   Runtime: Level1-eff (reference) is built from its unmodified sources with
#   reproduce_short_signature.c, uncompressed and compressed, with the
#   candidate's -O2 -DNDEBUG. An honest signature whose declared bytes end at
#   a PROT_NONE page verifies at full length, and the verifier faults when it
#   is declared one byte short or as a single byte.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-25-6: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED sign-25-6: $1"; exit 1; }
impl=sign-25/Implementations
n=0
for d in "$impl"/Reference_Implementation/SQISign2Dsquare-* "$impl"/Optimized_Implementation/SQISign2Dsquare-*; do
    tr -d '\r' <"$d/sqisign/SIG_AlgorithmInstance.c" | sed -n '/^int sig_verify(/,/^}/p' |
        grep -q '(void)sn_len_bytes;' || bad "$d checks sn_len_bytes"
    tr -d '\r' <"$d/fp.c" | sed -n '/^fp_decode(/,/^}/p' | grep -q 'memcpy(buf, src, FP_ENCODED_BYTES);' ||
        bad "$d/fp.c differs from the archive"
    n=$((n + 1))
done
[ $n = 16 ] || bad "expected 16 instances, found $n"
echo 'source: 16/16 sig_verify wrappers discard sn_len_bytes; fp_decode copies FP_ENCODED_BYTES from the signature'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
ulimit -c 0 2>/dev/null
printf '#include <gmp.h>\nint main(void){mpz_t x; mpz_init(x); mpz_clear(x); return 0;}\n' >"$scratch/g.c"
gcc -o "$scratch/g" "$scratch/g.c" -lgmp 2>/dev/null || { echo 'SKIP sign-25-6: GMP unavailable'; exit 77; }
d=$impl/Reference_Implementation/SQISign2Dsquare-Level1-eff
src='algebra.c auxfunc.c basis.c biextension.c dim2.c dim2id2iso.c dim4.c drng.c ec.c finit.c fp.c fp2.c hd.c id2iso.c ideal.c intbig.c integers.c isog_chains.c klptx.c lattice.c lll.c matkermod.c mp.c pack.c printer.c theta_isogenies.c theta_structure.c tools.c xeval.c xisog.c ec_arithmetic/endomorphism_action.c ec_arithmetic/gf_constants.c ec_arithmetic/quaternion_data.c ec_arithmetic/torsion_constants.c sqisign/keygen.c sqisign/sign.c sqisign/SIG_AlgorithmInstance.c ec_arithmetic/fp_p255.c'
for c in 0 1; do
    v=uncompressed; [ $c = 1 ] && v=compressed
    (cd "$d" && gcc -std=c99 -O2 -DNDEBUG -DCOMPRESSED=$c -DSQI_LABEL="\"Level1-eff_$v\"" -w -Isqisign -I. -Iec_arithmetic/include \
        -o "$scratch/s$c" "$here/reproduce_short_signature.c" $src -lgmp) >"$scratch/cc" 2>&1 ||
        { tail -20 "$scratch/cc"; bad "Level1-eff $v build failed"; }
    out=$("$scratch/s$c") || { echo "$out"; bad "Level1-eff $v driver failed"; }
    line=$(echo "$out" | grep '^RESULT ') || { echo "$out"; bad "Level1-eff $v: no result"; }
    echo "$line" | grep -Eq ' full=acc minus1=sig[0-9]+ one=sig[0-9]+$' || { echo "$line"; bad "Level1-eff $v: expected acceptance at full length and faults when short"; }
    sb=$(echo "$line" | sed 's/.* sig=\([0-9]*\) .*/\1/')
    echo "Level1-eff $v: $sb-byte signature before a PROT_NONE page verifies; declared $((sb - 1)) bytes or 1 byte, sig_verify reads past the buffer and faults ($(echo "$line" | sed 's/.* one=\(sig[0-9]*\)$/\1/'))"
done
echo 'ATTACK sign-25-6: sig_verify ignores the declared signature length and reads past a short signature: CONFIRMED'
exit 0
