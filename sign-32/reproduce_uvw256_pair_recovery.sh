#!/bin/sh
# sign-32-3 UVW-256: honest signatures keep the secret coordinate pairing.
#
# Default (about 1-2 minutes): sign with a temporary build of the frozen
# UVW-256 reference signer and measure, at the secret pair positions, the
# joint distribution of the public full error vectors against the analytic
# forced-pair model and against random coordinate pairs.  The secret
# permutation is used only to locate the pairs for this measurement.
#
# NGCC_LONG=1 (about 10-20 minutes on 28 cores; Python with NumPy, set
# NGCC_SAGE_PYTHON): key generation, 308 honest signatures, public pair
# scoring, recovery of every hidden pair, an equivalent signing key and a
# fresh-message forgery checked with the intended uvw_verify predicate, plus
# a uniform-vector control that must not recover anything.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-32-3: gcc unavailable'; exit 77; }
command -v python3 >/dev/null 2>&1 || { echo 'SKIP sign-32-3: python3 unavailable'; exit 77; }

ref=sign-32/Implementations/Reference_Implementation/UVW-256
[ -f "$ref/uvw.c" ] || { echo "NOT CONFIRMED sign-32-3: $ref missing"; exit 1; }
here=sign-32
jobs=$(nproc 2>/dev/null || echo 4)
[ "$jobs" -gt 8 ] && jobs=8

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
src=$scratch/src
cp -rL "$ref" "$src" || exit 2
for f in signonly keygen fastinv recover; do
    cp "$here/reproduce_uvw_pairs_$f.c" "$scratch/" || exit 2
done
cp "$here/reproduce_uvw_pairs_stats.py" "$scratch/" || exit 2

# Remove the submitted debug prints, capture the signer's full vector, and
# give key generation an output-identical fast inverse (scratch copy only).
sed -i -e '/size_t fe_hw = vf3_hamming_weight(fe);/d' -e '/size_t d_zero = 0;/d' \
    -e '/size_t p_dup = 0;/d' -e '/d_zero++/d' -e '/p_dup++/d' \
    -e '/fprintf(stderr, "DBG_/d' "$src/uvw.c"
sed 's/^bool inverse_matrix(/bool inverse_matrix_orig(/' "$src/gauss.c" > "$src/gauss_orig.c"
python3 - "$src" <<'EOF' || { echo 'NOT CONFIRMED sign-32-3: UVW-256 source not as submitted'; exit 1; }
import sys
d = sys.argv[1]
s = open(d + "/uvw.c").read()
a = "uvw_signature uvw_sign("
b = "    vf3_e *f = vf3_alloc(param.n - param.k);\n    sign.e"
assert s.count(a) == 1 and s.count(b) == 1
s = s.replace(a, "vf3_e *uvw_last_fe = NULL;\n" + a, 1)
s = s.replace(b, "    if (uvw_last_fe) vf3_free(uvw_last_fe);\n"
                 "    uvw_last_fe = vf3_alloc(param.n); vf3_copy(uvw_last_fe, fe);\n" + b, 1)
open(d + "/uvw_fe.c", "w").write(s)
s = open(d + "/SIG_AlgorithmInstance.c").read()
i = s.index("static bool row_reduce_to_identity(mf3_e *H, mf3_e *T) {")
j = s.index("static bool try_build_keypair(")
s = (s[:i] + "bool inverse_matrix(mf3_e *inv, mf3_e const *mat);\n"
     "static bool row_reduce_to_identity(mf3_e *H, mf3_e *T) { return inverse_matrix(T, H); }\n\n" + s[j:])
open(d + "/SIG_fast.c", "w").write(s)
EOF

common="auxfunc.c drng.c fq_arithmetic/vf3.c fq_arithmetic/mf3.c fq_arithmetic/dp.c gauss_orig.c ../reproduce_uvw_pairs_fastinv.c -lm"
cflags="-O3 -std=gnu11 -DUSE_API_PKC -I. -Ifq_arithmetic"
(
    cd "$src" || exit 2
    # shellcheck disable=SC2086
    gcc $cflags -DLEVEL=2 ../reproduce_uvw_pairs_signonly.c uvw_fe.c $common -o ../signonly 2>/dev/null || exit 1
    if [ "${NGCC_LONG:-0}" = 1 ]; then
        # shellcheck disable=SC2086
        gcc $cflags ../reproduce_uvw_pairs_keygen.c uvw.c $common -o ../keygen 2>/dev/null || exit 1
        # shellcheck disable=SC2086
        gcc $cflags ../reproduce_uvw_pairs_recover.c uvw.c $common -o ../recover 2>/dev/null || exit 1
    fi
) || { echo 'NOT CONFIRMED sign-32-3: build failed'; exit 1; }

# sign <keyarg> <outdir> <workers> <per-worker>
sign() {
    mkdir -p "$2" || exit 2
    i=0
    while [ "$i" -lt "$3" ]; do
        "$scratch/signonly" "$1" "w$i" "$4" "$2/w$i.vec" "$2/truth.bin" 2>>"$2/err.log" &
        i=$((i + 1))
    done
    wait
    cat "$2"/w*.vec > "$2/all.vec"
}

per=${UVW_SIG_PER_WORKER:-2}
t0=$(date +%s)
sign uvw256-sign-32-3 "$scratch/reduced" "$jobs" "$per"
echo "reduced run: $((jobs * per)) UVW-256 signatures from the frozen signer in $(( $(date +%s) - t0 )) s"
python3 - "$scratch/reduced/all.vec" "$scratch/reduced/truth.bin" <<'EOF' || { echo 'NOT CONFIRMED sign-32-3: UVW-256 hidden pairs not distinguishable from random pairs'; exit 1; }
import math, random, struct, sys
n, k1, k2, w = 19200, 6433, 3167, 17088
h = n // 2
raw = open(sys.argv[1], "rb").read()
V = [raw[i:i + n] for i in range(0, len(raw), n)]
tr = open(sys.argv[2], "rb").read()
p = [struct.unpack_from("<I", tr, 5 * i)[0] for i in range(n)]
d = [1 if tr[5 * i + 4] == 1 else -1 for i in range(n)]
sv = {0: 0, 1: 1, 2: -1}
assert all(sum(1 for x in v if x) == w for v in V), "weight check failed"
def cats(pairs):
    c = [0, 0, 0, 0]
    for v in V:
        for a, b, rel in pairs:
            x, y = sv[v[a]], sv[v[b]] * rel
            if x == 0 and y == 0: c[0] += 1
            elif (x == 0) != (y == 0): c[1] += 1
            elif x == y: c[2] += 1
            else: c[3] += 1
    t = float(sum(c))
    return [x / t for x in c]
hid = cats([(p[i], p[i + h], d[i] * d[i + h]) for i in range(h)])
rng = random.Random(7)
rnd = cats([(rng.randrange(n), rng.randrange(n), 1) for _ in range(h)])
rI = k1 / h; pi = (k2 / h) * 0.5 + (1 - k2 / h) * (2 / 3)
model = [(1 - rI) * (1 - pi) / 3, (1 - rI) * pi * 2 / 3,
         rI * (1 - pi) + (1 - rI) * (1 - pi) * 2 / 3, rI * pi + (1 - rI) * pi / 3]
rho = w / n
unif = [(1 - rho) ** 2, 2 * rho * (1 - rho), rho ** 2 / 2, rho ** 2 / 2]
print("UVW-256: %d signatures, %d hidden pairs; categories [both-zero, one-zero, equal, negated]" % (len(V), h))
for name, v in (("hidden pairs (measured)", hid), ("forced-pair model", model),
                ("control: random pairs", rnd), ("uniform weight-w", unif)):
    print("  %-26s %s" % (name, " ".join("%.4f" % x for x in v)))
kl = sum(a * math.log(a / b) for a, b in zip(hid, unif))
print("  KL(hidden || uniform) per signature per pair = %.4f nats" % kl)
ok = (max(abs(a - b) for a, b in zip(hid, model)) < 0.01
      and max(abs(a - b) for a, b in zip(rnd, unif)) < 0.01
      and hid[3] - unif[3] > 0.05)
sys.exit(0 if ok else 1)
EOF
echo 'control: every vector has weight w = 17088; random coordinate pairs match the uniform weight-w distribution'

if [ "${NGCC_LONG:-0}" = 1 ]; then
    PY=
    for cand in "${NGCC_SAGE_PYTHON:-}" python3; do
        [ -n "$cand" ] && "$cand" -c 'import numpy' >/dev/null 2>&1 && { PY=$cand; break; }
    done
    [ -n "$PY" ] || { echo 'SKIP sign-32-3 long run: NumPy unavailable (set NGCC_SAGE_PYTHON)'; exit 77; }
    t0=$(date +%s)
    python3 -c "import hashlib; open('$scratch/seed.bin','wb').write(hashlib.sha512(b'sign-32-3 UVW-256 key').digest()[:48])"
    "$scratch/keygen" "$scratch/seed.bin" "$scratch/pk.bin" "$scratch/sk.bin" || { echo 'NOT CONFIRMED sign-32-3: keygen failed'; exit 1; }
    echo "long run: UVW-256 key generated in $(( $(date +%s) - t0 )) s"
    lj=$(nproc 2>/dev/null || echo 4)
    [ "$lj" -gt 28 ] && lj=28
    t0=$(date +%s)
    sign "@$scratch/sk.bin" "$scratch/long" "$lj" $(( (308 + lj - 1) / lj ))
    nsig=$(( $(wc -c < "$scratch/long/all.vec") / 19200 ))
    echo "long run: $nsig signatures in $(( $(date +%s) - t0 )) s"
    "$PY" -I "$scratch/reproduce_uvw_pairs_stats.py" 256 "$scratch/long/all.vec" "$scratch/long/truth.bin" \
        --out "$scratch/long/cand.tsv" || exit 1
    out=$(cd "$scratch/long" && "$scratch/recover" "$scratch/pk.bin" cand.tsv 5000 truth.bin 2)
    rc=$?
    printf '%s\n' "$out"
    if [ "$rc" -ne 0 ] ||
       ! printf '%s\n' "$out" | grep -q 'truth check: 9600/9600 hidden pairs recovered, 9600 with correct relative scale' ||
       ! printf '%s\n' "$out" | grep -q 'intended uvw_verify on fresh message = 1, on changed message = 0'; then
        echo 'NOT CONFIRMED sign-32-3: UVW-256 recovery or forgery failed'
        exit 1
    fi
    echo 'control: uniformly random weight-w vectors through the same scoring and recovery'
    "$PY" -I "$scratch/reproduce_uvw_pairs_stats.py" 256 "$scratch/long/all.vec" "$scratch/long/truth.bin" \
        --control-uniform --out "$scratch/long/ctl.tsv" 2>/dev/null | grep 'mutual-best'
    cout=$(cd "$scratch/long" && "$scratch/recover" "$scratch/pk.bin" ctl.tsv 5000 truth.bin 2)
    crc=$?
    printf '%s\n' "$cout" | head -3
    if [ "$crc" -eq 0 ]; then
        echo 'NOT CONFIRMED sign-32-3: uniform control unexpectedly produced a forgery'
        exit 1
    fi
    echo 'CONFIRMED sign-32-3: 308 honest UVW-256 signatures recover all 9600 hidden pairs, an equivalent key and a fresh-message forgery accepted by uvw_verify (uniform control recovers nothing)'
    exit 0
fi

echo 'CONFIRMED sign-32-3: UVW-256 honest signatures preserve the secret coordinate pairing (forced-pair model, random-pair control uniform)'
exit 0
