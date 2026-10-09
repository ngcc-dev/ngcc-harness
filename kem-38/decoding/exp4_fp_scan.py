import sys
from pathlib import Path
sys.argv=[sys.argv[0]]
exec((Path(__file__).resolve().parent / 'exp3_counting.py').read_text(encoding='utf-8').split('# ISD cost for finding')[0].split('print("t     log2')[0])
from cryptographic_estimators.SDFqEstimator import SDFqEstimator
best=None
print("t  log2F(Stern,q-ary)  log2P  log2F/P")
for t in range(336, 432, 2):
    lnsol = max(0.0, max(lE1(t), lE2(t)))
    est = SDFqEstimator(n=n, k=k, w=t, q=q)
    res = est.estimate()
    ft = min(r['estimate']['time'] for r in res.values())
    fp = ft - lP(t)
    if best is None or fp < best[0]:
        best = (fp, t, ft)
    if t % 10 == 6 or t >= 400:
        print(f"{t:4d} {ft:8.2f} {lP(t):9.2f} {fp:9.2f}")
print("min log2 F/P =", best)
