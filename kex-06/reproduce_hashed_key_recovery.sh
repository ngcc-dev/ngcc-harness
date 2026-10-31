#!/bin/sh
# kex-06-2 MAMBA-NIKE: static-key recovery through the deployed hashed-key
# responder.  Builds kex-06/reproduce_hashed_key_recovery.c against the
# unmodified reference sources (without STATISTICAL_TEST) in a temporary
# directory.  The oracle is kex_derive_ss_b on attacker-chosen one-pass
# messages; the attacker learns the derived key of those non-test sessions.
# Default: MAMBA-NIKE-128 (under a minute).  NGCC_LONG=1 runs all five sets
# (about 30 minutes).
set -eu
cd "$(dirname "$0")/.."
command -v cc >/dev/null 2>&1 || { echo 'SKIP kex-06-2: cc unavailable'; exit 77; }
tmp=$(mktemp -d "${TMPDIR:-/tmp}/ngcc-mamba.XXXXXX") || exit 77
trap 'rm -rf "$tmp"' EXIT
levels=128
[ "${NGCC_LONG:-0}" = 1 ] && levels="128 192 256 384 512"
rc=0
for l in $levels; do
    D=kex-06/Implementations/Reference_Implementation/MAMBA-NIKE-$l
    cc -O2 -std=gnu99 -w -I$D -o "$tmp/attack$l" kex-06/reproduce_hashed_key_recovery.c \
        $D/KEX_AlgorithmInstance.c $D/nike.c $D/poly.c $D/toom.c $D/error_correction.c \
        $D/reduce.c $D/fips202.c $D/crypto_stream_chacha20.c -lm
    st=0; "$tmp/attack$l" -s 1 > "$tmp/out$l" || st=$?
    grep '^RESULT' "$tmp/out$l" || true
    [ "$st" = 0 ] && grep -q 'pk_verified=1 honest_session_key_match=1 perturbed_control_rejected=1' "$tmp/out$l" ||
        { echo "MAMBA-NIKE-$l: exit $st"; rc=1; }
done
if [ "$rc" = 0 ]; then
    echo "CONFIRMED kex-06-2: hashed session keys recover the static secret; the recovered key derives an honest session key"
    exit 0
fi
echo "NOT CONFIRMED kex-06-2"
exit 1
