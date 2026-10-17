#!/bin/sh
# YuanYang.DSA: the Delta2 rejection test is sign-reversed, one-sided and
# mis-scaled, so returned signatures depend on a secret quadratic form (sign-34-4).
#
# Builds isolated copies of the three reference trees with one read-only hook in
# delta2_probability_ntt(), checks that each copy's first signature is
# byte-identical to the prebuilt sign-34/lib/libyuanyang-*.so under the same DRNG
# seed, then signs N messages and reports the implemented Delta2 acceptance as a
# function of the sign of q = x^T (Sigma_delta/sigma^2) x, with the form G' of an
# unrelated key as the control.
#
# usage: sh sign-34/reproduce_delta2_selection.sh [N512] [N1024] [N2048]
#        default 2000 / 100 / 1000 signatures; NGCC_LONG=1 selects 6000 / 1000 / 3000.
set -u
cd "$(dirname "$0")/.." || exit 2
ID=sign-34-4
command -v gcc >/dev/null 2>&1 || { echo "SKIP $ID: gcc unavailable"; exit 77; }
command -v python3 >/dev/null 2>&1 || { echo "SKIP $ID: python3 unavailable"; exit 77; }
for d in 512 1024 2048; do
    [ -f sign-34/lib/libyuanyang-$d.so ] || { echo "SKIP $ID: sign-34/lib/libyuanyang-$d.so missing (make -C sign-34)"; exit 77; }
done
if [ "${NGCC_LONG:-0}" = 1 ]; then n1=6000; n2=1000; n3=3000; else n1=2000; n2=100; n3=1000; fi
N512=${1:-$n1}; N1024=${2:-$n2}; N2048=${3:-$n3}
SEED=21
root=$(pwd)
ref=sign-34/Implementations/Reference_Implementation
opt="sign-34/Implementations/Optimized_Implementation/SIG-YuanYang_DSA-x86-Performance Optimization Version"

# Source certificate: the four defects in all six trees.
for t in "$ref" "$opt"; do
    for d in 512 1024 2048; do
        f="$t/yuanyang-$d"
        grep -q 'if (dot_cmp <= 0) {' "$f/sign_ntt.c" &&
        grep -q 'fpr_expm_p62(exponent), fpr_q62_yuanyang_weak_inv_C)' "$f/sign.c" &&
        grep -q 'YUANYANG_LOGD - 1u' "$f/sign_ntt.c" || {
            echo "NOT CONFIRMED $ID: Delta2 source pattern missing in $f"; exit 1; }
        printf 'source %s: clamp at sign_ntt.c:%s, exp(-w) at sign.c:%s\n' "yuanyang-$d ($(basename "$t" | cut -c1-14))" \
            "$(grep -n '		return fpr_zero;' "$f/sign_ntt.c" | head -1 | cut -d: -f1)" \
            "$(grep -n 'fpr_expm_p62(exponent), fpr_q62_yuanyang_weak_inv_C)' "$f/sign.c" | head -1 | cut -d: -f1)"
    done
done

scratch=$(mktemp -d) || exit 2
case "$scratch" in /?*) [ -d "$scratch" ] ;; *) echo 'unexpected temporary path' >&2; exit 2 ;; esac
trap 'rm -rf "$scratch"' EXIT HUP INT TERM

build() {
    d=$1
    case $d in 512) tw=twiddles9.c; et=int32_t ;; 1024) tw=twiddles10.c; et=int32_t ;; 2048) tw=twiddles11.c; et=int64_t ;; esac
    cp -rL "$ref/yuanyang-$d" "$scratch/y$d" || return 1
    python3 - "$scratch/y$d/sign.c" "$et" <<'PY' || return 1
import sys
p, et = sys.argv[1], sys.argv[2]
s = open(p).read()
a = '#include "sign_ntt.h"\n'
b = '\treturn probability;\n'
assert s.count(a) == 1 and s.count(b) == 1, "patch anchors not unique"
assert s.index(b) > s.index("delta2_probability_ntt(")
s = s.replace(a, a + 'void yy_instr(const yuanyang_sign_ntt_precomp*,const %s*,const %s*,int64_t,int64_t,double,double);\n' % (et, et))
s = s.replace(b, '\tyy_instr(ntt_precomp, err0, err1, (int64_t)exponent.v, (int64_t)probability.v, '
              '(double)fpr_yuanyang_sigma_sig.v/8796093022208.0, '
              '(double)fpr_q62_yuanyang_weak_inv_C.v/4611686018427387904.0);\n' + b)
open(p, 'w').write(s)
PY
    (cd "$scratch/y$d" && gcc -O2 -std=gnu99 -DNGCC_KATS -I. -Ikeygen/ntrugen/src -DERR_T=$et \
        -o "$scratch/yyd2_$d" "$root/sign-34/reproduce_delta2_selection.c" \
        common.c prng.c fpr.c fft.c auxfunc.c drng.c ntt.c codec.c poly_inv_ntt.c \
        SIG_AlgorithmInstance.c sign.c sign_ntt.c vrfy.c sampler.c keygen/keygen.c \
        keygen/pairgen.c keygen/perturbation.c keygen/fft.c keygen/security_loss.c \
        keygen/ntrugen/src/ntrugen.c keygen/ntrugen/src/bigint.c keygen/ntrugen/src/bigintgcd.c \
        keygen/ntrugen/src/cfft.c keygen/ntrugen/src/complex.c keygen/ntrugen/src/fft.c \
        keygen/ntrugen/src/$tw -lm) >"$scratch/build$d.log" 2>&1
}
for d in 512 1024 2048; do build $d & done
wait
for d in 512 1024 2048; do
    [ -x "$scratch/yyd2_$d" ] || { echo "SKIP $ID: build of yuanyang-$d failed"; cat "$scratch/build$d.log"; exit 77; }
done

libsig() {
    python3 -I - "$1" "$SEED" <<'PY'
import ctypes, sys
L = ctypes.CDLL(sys.argv[1])
for f in ("sig_get_pk_len_bytes", "sig_get_sk_len_bytes", "sig_get_sn_len_bytes"):
    getattr(L, f).restype = ctypes.c_ulonglong
seed = (ctypes.c_ubyte * 48)(*([int(sys.argv[2]), 0x5a] + [0] * 46))
L.ngcc_seed(seed, ctypes.c_ulonglong(48))
P, S, N = L.sig_get_pk_len_bytes(), L.sig_get_sk_len_bytes(), L.sig_get_sn_len_bytes()
pk, sk, sn = (ctypes.c_ubyte * P)(), (ctypes.c_ubyte * S)(), (ctypes.c_ubyte * N)()
pl, sl, nl = ctypes.c_ulonglong(), ctypes.c_ulonglong(), ctypes.c_ulonglong()
m = (ctypes.c_ubyte * 16)(*b"control".ljust(16, b"\0"))
L.sig_keygen(pk, ctypes.byref(pl), sk, ctypes.byref(sl))
L.sig_sign(sk, sl, m, ctypes.c_ulonglong(16), sn, ctypes.byref(nl))
L.sig_keygen(pk, ctypes.byref(pl), sk, ctypes.byref(sl))
m = (ctypes.c_ubyte * 16)(*b"msg0".ljust(16, b"\0"))
L.sig_sign(sk, sl, m, ctypes.c_ulonglong(16), sn, ctypes.byref(nl))
h = 2166136261
for b in bytes(sn)[:nl.value]:
    h = ((h ^ b) * 16777619) & 0xffffffff
print("fnv1a=%08x len=%d" % (h, nl.value))
PY
}

for d in 512 1024 2048; do
    a=$("$scratch/yyd2_$d" 1 $SEED | sed -n 's/^FIRSTSIG //p')
    b=$(libsig "sign-34/lib/libyuanyang-$d.so")
    echo "identity yuanyang-$d: instrumented copy $a, prebuilt library $b"
    [ -n "$a" ] && [ "$a" = "$b" ] || { echo "NOT CONFIRMED $ID: instrumented copy differs from the prebuilt library"; exit 1; }
done

rc=0
for d in 512 1024 2048; do
    eval n=\$N$d
    echo "== YuanYang.DSA-$d reference, $n signatures"
    "$scratch/yyd2_$d" "$n" $SEED >"$scratch/out$d" 2>&1
    grep -v '^FIRSTSIG' "$scratch/out$d"
    # 512 and 2048 must show the selection; 1024 is reported only (its correction is a constant)
    case $d in
        512|2048) grep -q '^SELECTION SECRET-DEPENDENT controls=clean' "$scratch/out$d" || rc=1 ;;
        1024) grep -q 'controls=clean' "$scratch/out$d" || rc=1 ;;
    esac
done
if [ "$rc" -eq 0 ]; then
    echo "CONFIRMED $ID: YuanYang.DSA-512 and -2048 Delta2 acceptance depends on the sign of the secret quadratic form x^T G x and shifts returned signatures; unrelated-key control flat"
    exit 0
fi
echo "NOT CONFIRMED $ID: secret-dependent Delta2 selection not shown"
exit 1
