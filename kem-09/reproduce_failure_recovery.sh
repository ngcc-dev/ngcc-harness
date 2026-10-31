#!/bin/sh
# kem-09-5 CheetahKEM: decryption-failure key recovery.
# 1. Measures the decryption noise of honest Cheetah256 encapsulations through
#    the unmodified reference KEM_Cheetah.c (indcpa_decrypt copied with its
#    final mod-2 step replaced by a copy-out) for comparison with the model
#    sigma printed by kem-09/reproduce_failure_rate.py (245.5).
# 2. Draws 3,000 Cheetah256 failures from the exact failure-conditioned noise
#    law for a fresh key and recovers the secret from the attacker-known
#    ciphertext data and the failure bit alone (about 1 minute).
# NGCC_LONG=1 adds the norm-boosted Cheetah128 recovery (1,500 failures).
set -eu
cd "$(dirname "$0")/.."
command -v cc >/dev/null 2>&1 || { echo 'SKIP kem-09-5: cc unavailable'; exit 77; }
command -v python3 >/dev/null 2>&1 || { echo 'SKIP kem-09-5: python3 unavailable'; exit 77; }
tmp=$(mktemp -d "${TMPDIR:-/tmp}/ngcc-cheetah.XXXXXX") || exit 77
trap 'rm -rf "$tmp"' EXIT
D=kem-09/Implementations/Reference_Implementation/Cheetah256
python3 - "$D/KEM_Cheetah.c" "$tmp/noise_fn.h" <<'PY'
import re, sys
src = open(sys.argv[1], encoding="utf-8-sig").read()
f = src[src.index("static void indcpa_decrypt"):src.index("int kem_keygen")]
f = f.replace("static void indcpa_decrypt", "static void noise_vals").replace("uint8_t m[MSG_BYTES]", "int16_t *outv")
f, n = re.subn(r"vector_mod_2\(v1, SHARED_KEY_BYTES\*8\);\s*encode_msg\(v1, m\);",
               "memcpy(outv, v1, sizeof(int16_t)*SHARED_KEY_BYTES*8);", f)
assert n == 1, "indcpa_decrypt layout changed"
open(sys.argv[2], "w").write("#include <string.h>\n" + f)
PY
cc -O2 -w -I"$D" -I"$tmp" -o "$tmp/noise" kem-09/reproduce_failure_noise.c \
    $D/poly.c $D/ntt.c $D/mod.c $D/auxfunc.c $D/drng.c -lm
"$tmp/noise" 20000 3 > "$tmp/noise.out"
head -1 "$tmp/noise.out"
cc -O2 -o "$tmp/fail256" kem-09/reproduce_failure_recovery.c -lm
rc=0
st=0; "$tmp/fail256" 3000 41 3 0.3 > "$tmp/o256" || st=$?
grep -E '^(key:|sampled|RECOV|cleanup)' "$tmp/o256" || true
[ "$st" = 0 ] || rc=1
grep -q -E 'RECOVERED s exactly|RECOVERED s \(public-key check passes; s-errors vs truth = 0\)' "$tmp/o256" || rc=1
if [ "${NGCC_LONG:-0}" = 1 ]; then
    cc -O2 -DK=1 -DETA=5 -DLAM=128 -o "$tmp/fail128" kem-09/reproduce_failure_recovery.c -lm
    st=0; "$tmp/fail128" 1500 31 4 0.3 1.17 > "$tmp/o128" || st=$?
    grep -E '^(norm boost|sampled|RECOV|cleanup)' "$tmp/o128" || true
    [ "$st" = 0 ] || rc=1
    grep -q -E 'RECOVERED s exactly|RECOVERED s \(public-key check passes; s-errors vs truth = 0\)' "$tmp/o128" || rc=1
fi
if [ "$rc" = 0 ]; then
    echo "CONFIRMED kem-09-5: simulated natural failures recover the Cheetah secret exactly"
    exit 0
fi
echo "NOT CONFIRMED kem-09-5"
exit 1
