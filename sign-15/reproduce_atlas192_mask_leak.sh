#!/bin/sh
# sign-15-7 ATLAS-192: finite mask support makes honest signatures leak s1.
#
# Builds a temporary copy of the ATLAS-192 reference implementation whose
# mask decoder (rej_gamma1m1) takes the two independent 20-bit values the
# specification describes, signs chosen messages and correlates a public
# boundary-half-space estimate with the true s1 and with a control key.
#
#   sh sign-15/reproduce_atlas192_mask_leak.sh             # 40,000 signatures
#   NGCC_LONG=1 sh sign-15/reproduce_atlas192_mask_leak.sh # adds the 240,000-
#       signature Chebyshev-centre LP run (NumPy and SciPy; set
#       NGCC_SAGE_PYTHON to a suitable Python)
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-15-7: gcc unavailable'; exit 77; }
command -v python3 >/dev/null 2>&1 || { echo 'SKIP sign-15-7: python3 unavailable'; exit 77; }

src=sign-15/Implementation/Reference_Implementation/lwrdsa192
[ -f "$src/poly.c" ] || { echo "NOT CONFIRMED sign-15-7: $src missing"; exit 1; }
nsig=${ATLAS192_SIGNATURES:-40000}
jobs=$(nproc 2>/dev/null || echo 4)
[ "$jobs" -gt 8 ] && jobs=8

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
cp -rL "$src" "$scratch/lwrdsa192" || exit 2
cp sign-15/reproduce_atlas192_mask_leak.c "$scratch/lwrdsa192/" || exit 2

# Replace the submitted duplicate decode by the specified independent decode.
python3 - "$scratch/lwrdsa192/poly.c" <<'EOF' || { echo 'NOT CONFIRMED sign-15-7: mask decoder not as submitted'; exit 1; }
import sys
p = sys.argv[1]
s = open(p).read()
old = """    t  = buf[pos + 2] >> 4;
    t |= (uint32_t)buf[pos + 3] << 4;
    t |= (uint32_t)buf[pos + 4] << 12;

    pos += 5;

    if(t <= 2*GAMMA1 - 2)
      a[ctr++] = Q + GAMMA1 - 1 - t;
    if(t <= 2*GAMMA1 - 2 && ctr < len)
      a[ctr++] = Q + GAMMA1 - 1 - t;
"""
new = """    if(t <= 2*GAMMA1 - 2)
      a[ctr++] = Q + GAMMA1 - 1 - t;

    t  = buf[pos + 2] >> 4;
    t |= (uint32_t)buf[pos + 3] << 4;
    t |= (uint32_t)buf[pos + 4] << 12;

    pos += 5;

    if(t <= 2*GAMMA1 - 2 && ctr < len)
      a[ctr++] = Q + GAMMA1 - 1 - t;
"""
assert s.count(old) == 1
open(p, "w").write(s.replace(old, new))
EOF
echo "patched: rej_gamma1m1 now decodes buf[pos..pos+2] and buf[pos+2..pos+4] as two coefficients"

(
    cd "$scratch/lwrdsa192" || exit 2
    gcc -O2 -std=gnu11 -fwrapv -I. -o ../atlas reproduce_atlas192_mask_leak.c \
        SIG_lwrdsa192.c polyvec.c packing.c poly.c rounding.c auxfunc.c drng.c -lm
) || { echo 'NOT CONFIRMED sign-15-7: build failed'; exit 1; }
bin=$scratch/atlas

per=$(( (nsig + jobs - 1) / jobs ))
start=$(date +%s)
w=0
while [ "$w" -lt "$jobs" ]; do
    "$bin" acc keyA $((w * per)) "$per" "$scratch/acc$w.bin" &
    w=$((w + 1))
done
wait
out=$("$bin" corr keyA keyB "$scratch"/acc*.bin) || { echo 'NOT CONFIRMED sign-15-7: analysis failed'; exit 1; }
printf '%s\n' "$out"
echo "reduced run: $((per * jobs)) signatures, $jobs workers, $(( $(date +%s) - start )) s"

verdict=$(printf '%s\n' "$out" | python3 -c '
import sys, re
t = dict(re.findall(r"(\w+)=(-?[0-9.]+)", sys.stdin.read()))
ok = (int(t["halfspace_violations"]) == 0 and int(t["verify_failures"]) == 0
      and int(t["mutated_accepted"]) == 0 and float(t["corr_true"]) >= 0.15
      and abs(float(t["corr_control"])) < 3 * float(t["one_over_sqrt_n"]))
print("ok" if ok else "fail")')
echo "control: honest signatures verify, changed messages reject, true key satisfies every public half-space, control-key correlation within 3/sqrt(n)"
if [ "$verdict" != ok ]; then
    echo 'NOT CONFIRMED sign-15-7: public estimate does not separate the true key from the control'
    exit 1
fi

if [ "${NGCC_LONG:-0}" = 1 ]; then
    probe='import numpy, scipy.optimize'
    PY=
    for cand in "${NGCC_SAGE_PYTHON:-}" python3; do
        [ -n "$cand" ] && "$cand" -c "$probe" >/dev/null 2>&1 && { PY=$cand; break; }
    done
    [ -n "$PY" ] || { echo 'SKIP sign-15-7 long run: NumPy/SciPy unavailable (set NGCC_SAGE_PYTHON)'; exit 77; }
    mkdir "$scratch/run192" || exit 2
    start=$(date +%s)
    w=0
    while [ "$w" -lt 12 ]; do
        "$bin" events keyA $((w * 20000)) 20000 200 "$scratch/run192/part$w.bin" &
        w=$((w + 1))
    done
    wait
    "$bin" s1 keyA "$scratch/run192/s1.bin" && "$bin" s1 keyB "$scratch/run192/s1_keyB.bin" || exit 2
    echo "long run: 240000 signatures generated in $(( $(date +%s) - start )) s"
    cat > "$scratch/lp.py" <<'EOF'
import glob, sys, time
import numpy as np
from scipy.optimize import linprog
d, N, L, ETA, G1 = sys.argv[1], 128, 10, 8, 1 << 19
rows = [[] for _ in range(L)]; rhs = [[] for _ in range(L)]; nsig = 0
def negrow(c, i):
    r = np.zeros(N, dtype=np.int64)
    for k in np.nonzero(c)[0]:
        if k <= i: r[i - k] += c[k]
        else: r[i + N - k] -= c[k]
    return r
for fn in sorted(glob.glob(d + "/part*.bin")):
    buf = open(fn, "rb").read(); p = 0
    while p < len(buf):
        c = np.frombuffer(buf, np.int8, N, p).astype(np.int64); p += N
        ne = int.from_bytes(buf[p:p + 2], "little"); p += 2
        for _ in range(ne):
            j, i = buf[p], buf[p + 1]
            z = int.from_bytes(buf[p + 2:p + 6], "little", signed=True); p += 6
            r = negrow(c, i)
            if z > 0: rows[j].append(-r); rhs[j].append(G1 - 1 - z)
            else: rows[j].append(r); rhs[j].append(z + G1 - 1)
        nsig += 1
st = np.fromfile(d + "/s1.bin", np.int32).reshape(L, N)
sc = np.fromfile(d + "/s1_keyB.bin", np.int32).reshape(L, N)
X = []; ok = True; t0 = time.time()
for j in range(L):
    A = np.array(rows[j]); b = np.array(rhs[j]); ok &= bool(np.all(A @ st[j] <= b))
    res = linprog(np.r_[np.zeros(N), -1.0], A_ub=np.hstack([A, np.ones((len(b), 1))]), b_ub=b,
                  bounds=[(-ETA, ETA)] * N + [(None, None)], method="highs")
    X.append(res.x[:N])
X = np.array(X)
ct = np.corrcoef(X.ravel(), st.ravel())[0, 1]; cc = np.corrcoef(X.ravel(), sc.ravel())[0, 1]
print("LP signatures=%d constraints=%d true_key_feasible=%s lp_seconds=%.0f" % (nsig, sum(map(len, rhs)), ok, time.time() - t0))
print("LP corr_true=%.4f corr_control=%.4f one_over_sqrt_n=%.4f" % (ct, cc, 1 / np.sqrt(X.size)))
sys.exit(0 if ok and ct >= 0.3 and abs(cc) < 3 / np.sqrt(X.size) else 1)
EOF
    "$PY" -I "$scratch/lp.py" "$scratch/run192" || { echo 'NOT CONFIRMED sign-15-7: long LP run did not separate the keys'; exit 1; }
fi

echo 'CONFIRMED sign-15-7: ATLAS-192 honest signatures under the specified mask decoder leak s1 through public boundary half-spaces (control key uncorrelated)'
exit 0
