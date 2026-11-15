#!/bin/sh
# kem-17-6: every HEP-QC seed expansion passes through a 208-bit XOF state.
#
# Checks that xof_init and xof_proc request CTX_LEN*8 = 208 bits for a context
# declared as 26 64-bit words, and that the decryption key is expanded through
# that context. Then, for each reference parameter set, runs the archived XOF
# and requires the whole stream to be reproduced from the first 26 context
# bytes. HEP-QC-1 and -3 (128 and 192 bits) are controls: same state, no
# shortfall.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-17-6: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED kem-17-6: $1"; exit 1; }

impl=kem-17/Implementations/Implementations
s=$impl/common/symmetric.c
grep -Fq 'pseudoXOF(CTX_LEN * 8, seed, seed_size * 8, (unsigned char *)xof_ctx->ctx);' "$s" &&
    grep -Fq 'pseudoXOF(CTX_LEN * 8, aux, sizeof(aux) * 8, (unsigned char *)ctx);' "$s" &&
    grep -Fq 'xof_init(&dk_xof_ctx, dk_pke, SEED_BYTES);' "$impl/common/parsing.c" ||
    bad 'XOF wrapper or key expansion differs from the archive'
for tree in ref x86_64; do
    for set in 1 3 5 7; do
        grep -Eq '^#define CTX_LEN +26( |$)' "$impl/$tree/hep-qc-$set/parameters.h" ||
            bad "$tree hep-qc-$set CTX_LEN is not 26"
    done
done
echo 'source: xof_init and xof_proc write CTX_LEN*8 = 208 bits of state; CTX_LEN is 26 in all eight parameter files'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
for set in 1 3 5 7; do
    gcc -std=c99 -O2 -w -DHEP_QC_ARCH_REF=1 -I"$impl/common" -I"$impl/common/hep-qc-$set" \
        -I"$impl/ref" -I"$impl/ref/hep-qc-$set" -Iapi -o "$scratch/x$set" \
        kem-17/reproduce_xof_state_width.c "$impl/common/symmetric.c" api/auxfunc.c api/drng.c \
        2>"$scratch/cc" || { cat "$scratch/cc"; bad "hep-qc-$set build failed"; }
    out=$("$scratch/x$set") || bad "hep-qc-$set run failed"
    echo "hep-qc-$set: $out"
    case "$out" in *'state_bits=208 tail_nonzero=0 stream_from_state=1') ;; *) bad "hep-qc-$set behaviour differs" ;; esac
done
echo 'CONFIRMED kem-17-6: all seed expansions are functions of a 208-bit state, below the 256-bit (HEP-QC-5) and 512-bit (HEP-QC-7) levels'
exit 0
