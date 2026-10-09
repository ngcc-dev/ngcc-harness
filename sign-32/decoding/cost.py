#!/usr/bin/env python3
"""Finite-size forgery costs for large-weight ternary SD (UVW / Wave points).

Models implemented from the papers (all log2):
  (S)  Spec formula as transcribed from UVW spec 3.3.2 (= Loyer Thm 7 =
       BCDL19 Prop. 4 smoothed Wagner in the PGE+SS framework):
         T = max( (3^l / 2^{(k+l)/2^{a-1}})^{1/(a-2)},  1/P_{p,l} )
  (W)  BCDL19 Thm 1 (plain Wagner, lists 3^{l/a}, constraint 3^{l/a} <= 2^{(k+l)/2^a})
  (N)  Narisada et al. 2026 generalized ISD tree (Thm 1 list model) with
       modes split/rep, q=3, shifted alphabet {0,1}; plus Prop. 5 bit-cost
       model (All-Split) with log2(memory) access factor.
"""
import math, itertools, sys
from math import lgamma, log2

LOG3 = log2(3)

def lb(n, k):
    """log2 binomial(n,k) (real n,k allowed), -inf if invalid."""
    if k < 0 or k > n or n < 0:
        return float('-inf')
    return (lgamma(n + 1) - lgamma(k + 1) - lgamma(n - k + 1)) / math.log(2)

def logP(n, k, w, l, p):
    """log2 P_{p,l} from BCDL19 Prop. 2 (q=3)."""
    num = lb(n - k - l, w - p) + (w - p)
    nsol = lb(n, w) + w - (n - k)          # log2 expected #solutions
    den = min((n - k - l) * LOG3, lb(n, w) + w - l * LOG3)
    return num - den

# ---------------------------------------------------------------- (S)
def spec_formula(n, k, w, a, l, p=None, leaves=None, constrained=True):
    """log2 of the spec's T_msg,class for given integer a and l."""
    if p is None:
        p = k + l
    if leaves is None:
        leaves = 2 ** (a - 1)             # as written: (k+l)/2^{a-1}
    if a < 3:
        return float('inf')
    if constrained:
        # BCDL19 Prop 4: a largest integer with 3^{l/(a-1)} < 2^{(k+l)/2^{a-1}}
        ok_a = l * LOG3 / (a - 1) < (k + l) / 2 ** (a - 1)
        ok_a1 = not (l * LOG3 / a < (k + l) / 2 ** a)
        if not (ok_a and ok_a1):
            return float('inf')
    lam = (l * LOG3 - (k + l) / leaves) / (a - 2)
    if lam < 0:
        return float('inf')
    return max(lam, -logP(n, k, w, l, p))

def best_spec(n, k, w, constrained=True, leaves_fn=None, amax=12):
    best = (float('inf'), None)
    for a in range(3, amax + 1):
        leaves = leaves_fn(a) if leaves_fn else None
        for l in range(1, n - k):
            t = spec_formula(n, k, w, a, l, leaves=leaves, constrained=constrained)
            if t < best[0]:
                best = (t, (a, l))
    return best

def best_spec_real_a(n, k, w):
    """Real-valued a (Sendrier-style smoothing) in the same formula."""
    best = (float('inf'), None)
    for l in range(1, n - k):
        for ai in range(300, 1200):
            a = ai / 100
            lam = (l * LOG3 - (k + l) / 2 ** (a - 1)) / (a - 2)
            if lam < 0:
                continue
            t = max(lam, -logP(n, k, w, l, k + l))
            if t < best[0]:
                best = (t, (a, l))
    return best


def best_spec_loyer(n, k, w, constraint='loyer', amax=12):
    """Loyer Prop.4/Thm.7 DOOM version: 2^a-1 search leaves of size 2^{(k+l)/(2^a-1)},
    lambda = (l log3 - 2(k+l)/(2^a-1))/(a-2).  constraint 'loyer': largest a with
    3^{l/a} < 2^{(k+l)/(2^a-1)}; 'bcdl': largest a with 3^{l/(a-1)} < 2^{(k+l)/(2^(a-1)-1)}."""
    best = (float('inf'), None)
    for l in range(1, n - k):
        if constraint == 'loyer':
            a = max([a for a in range(2, amax + 1) if l * LOG3 / a < (k + l) / (2 ** a - 1)] or [0])
        else:
            a = max([a for a in range(2, amax + 1) if l * LOG3 / (a - 1) < (k + l) / (2 ** (a - 1) - 1)] or [0])
        if a < 3:
            continue
        lam = (l * LOG3 - 2 * (k + l) / (2 ** a - 1)) / (a - 2)
        if lam < 0:
            continue
        t = max(lam, -logP(n, k, w, l, k + l))
        if t < best[0]:
            best = (t, (a, l))
    return best

def sendrier_asymptotic(n, k, w, doom=True):
    """Sendrier 2023 eq.(12): real a with 3^{l/a} = 2^{(k+l)/(2^a-1)} = 1/P_{k+l,l}; WF=3^{l/a}."""
    best = None
    prev = None
    for li in range(10, (n - k) * 10):
        l = li / 10
        T = -logP(n, k, w, l, k + l)            # log2(1/P)
        if T <= 0:
            continue
        a = l * LOG3 / T
        if a <= 1:
            continue
        leaves = (2 ** a - 1) if doom else 2 ** a
        g = (k + l) / leaves - T
        if prev is not None and (g > 0) != (prev[1] > 0):
            best = (T, a, l)
            break
        prev = (l, g)
    return best

def refine_tree_model(n, k, w, cfg):
    best = (float('inf'), None)
    a = cfg['a']; modes = cfg['modes']
    for pt in range(cfg['pt'] - 120, cfg['pt'] + 121, 6):
        for l in range(max(1, cfg['l'] - 90), cfg['l'] + 91, 3):
            N = k + l; p = pt
            for m in modes:
                N = N / 2 if m == 'split' else N
                p = p / 2
            La = lb(N, p)
            if La <= 0:
                continue
            for f in [x / 100 for x in range(40, 260, 3)]:
                lam = La * f
                ls = [0.0] * a
                ls[a - 1] = (2 * La - lam) / LOG3
                for i in range(a - 2, 0, -1):
                    ls[i] = lam / LOG3
                ls[0] = l - sum(ls[1:])
                if min(ls) < 0:
                    continue
                T, M, L0, L = narisada_total(n, k, w, a, modes, l, pt, ls)
                if T < best[0]:
                    best = (T, dict(a=a, modes=modes, l=l, pt=pt, lam=lam, M=M, L0=L0, L=L, ls=ls))
    return best

# ---------------------------------------------------------------- (W)
def best_wagner_plain(n, k, w, doom=False, amax=12):
    best = (float('inf'), None)
    for a in range(1, amax + 1):
        nleaf = 2 ** a - (1 if doom else 0)
        for l in range(1, n - k):
            L = l * LOG3 / a
            if L > (k + l) / nleaf:
                continue
            t = max(L, -logP(n, k, w, l, k + l))
            if t < best[0]:
                best = (t, (a, l))
    return best

# ---------------------------------------------------------------- (N)
def narisada_tree(n, k, w, a, modes, l, pt, ls):
    """Narisada Thm 1 list model, q=3, shift x=1: alphabet E1={1}, q-2=1.
    modes[i] for i=0..a-1 (modes[a-1] must be 'split'); ls[i]=l_i, sum=l.
    Returns (log2 Ttree, log2 Mtree, log2 L0, list of log2 L_i)."""
    N = [k + l]; p = [pt]
    for i in range(a):
        if modes[i] == 'split':
            N.append(N[-1] / 2); p.append(p[-1] / 2)
        else:
            N.append(N[-1]); p.append(p[-1] / 2)   # eps=0 (alpha_bar=0 for q=3)
    Delta = [lb(N[i], p[i]) for i in range(a + 1)]
    Lam = [sum(ls[i:]) for i in range(a)] + [0]
    L = [None] * (a + 1)
    L[a] = Delta[a]
    for i in range(a - 1, -1, -1):
        if modes[i] == 'split':
            th = 0.0
        else:
            r = lb(p[i], p[i + 1])
            th = r + Delta[i] - 2 * Delta[i + 1]
        Lhat = 2 * L[i + 1] - ls[i] * LOG3 + th
        L[i] = min(Delta[i] - Lam[i] * LOG3, Lhat)
    terms = [L[a]]
    for i in range(a):
        terms.append(L[i + 1])
        terms.append(2 * L[i + 1] - ls[i] * LOG3)
    Ttree = max(terms)      # up to poly factors (sum ~ max)
    Mtree = max(L[1:])
    return Ttree, Mtree, L[0], L

def narisada_total(n, k, w, a, modes, l, pt, ls):
    Ttree, Mtree, L0, L = narisada_tree(n, k, w, a, modes, l, pt, ls)
    lp = logP(n, k, w, l, k + l)
    return Ttree + max(0.0, -(L0 + lp)), Mtree, L0, L

def narisada_bits_allsplit(n, k, w, a, l, pt, ls):
    """Prop. 5 bit-cost model for the All-Split tree (q=3)."""
    N = [(k + l) / 2 ** i for i in range(a + 1)]
    p = [pt / 2 ** i for i in range(a + 1)]
    L = [None] * (a + 1)
    L[a] = lb(N[a], p[a])
    for i in range(a - 1, -1, -1):
        L[i] = 2 * L[i + 1] - ls[i] * LOG3
    def entry_bits(i):
        return p[i] * log2(N[i]) + ls[i - 1] * LOG3
    M = max(L[i] + log2(entry_bits(i)) for i in range(1, a + 1))
    C = max([L[i] + i + log2(entry_bits(i)) for i in range(1, a + 1)]
            + [L[0] + log2(p[0] * log2(N[0]))])
    # sum of terms rather than max:
    terms = [L[i] + i + log2(entry_bits(i)) for i in range(1, a + 1)] + [L[0] + log2(max(p[0] * log2(N[0]), 1))]
    mx = max(terms)
    Csum = mx + log2(sum(2 ** (t - mx) for t in terms))
    lp = logP(n, k, w, l, k + l)
    T = Csum + log2(M) + max(0.0, -(L[0] + lp))
    return T, M, L[0] + lp, L

def opt_allsplit_bits(n, k, w, amin=3, amax=8):
    """Optimise Prop.5 cost: a, l, pt, and balanced list sizes.
    l_i chosen so that every intermediate list has size 3^{lam} (lists
    L_{a-1}..L_1 equal), l_0 = l - sum."""
    best = (float('inf'), None)
    for a in range(amin, amax + 1):
        for pt in range(int(0.3 * (k)), int(0.7 * k), max(1, k // 80)):
            for l in range(50, n - k, max(1, (n - k) // 100)):
                Na = (k + l) / 2 ** a; pa = pt / 2 ** a
                La = lb(Na, pa)
                if La <= 0:
                    continue
                # intermediate list size lam (log2): choose l_{a-1} = (2La - lam)/LOG3
                # and l_i = lam/LOG3 for i=a-2..1, l_0 = l - rest (must be >=0)
                for lam in [La * f for f in (0.6, 0.8, 1.0, 1.2, 1.4, 1.6, 1.8, 2.0)]:
                    ls = [0.0] * a
                    if a >= 2:
                        ls[a - 1] = (2 * La - lam) / LOG3
                        for i in range(a - 2, 0, -1):
                            ls[i] = lam / LOG3
                    ls[0] = l - sum(ls[1:])
                    if min(ls) < 0:
                        continue
                    T, M, lsp, L = narisada_bits_allsplit(n, k, w, a, l, pt, ls)
                    if T < best[0]:
                        best = (T, dict(a=a, l=l, pt=pt, lam=lam, ls=ls, M=M, L0P=lsp, L=L))
    return best

def refine_allsplit_bits(n, k, w, cfg):
    """Local refinement around a coarse optimum with finer grid."""
    best = (float('inf'), None)
    a = cfg['a']
    for pt in range(cfg['pt'] - 40, cfg['pt'] + 41, 4):
        for l in range(max(1, cfg['l'] - 60), cfg['l'] + 61, 3):
            Na = (k + l) / 2 ** a; pa = pt / 2 ** a
            La = lb(Na, pa)
            if La <= 0:
                continue
            for f in [x / 100 for x in range(40, 260, 4)]:
                lam = La * f
                ls = [0.0] * a
                ls[a - 1] = (2 * La - lam) / LOG3
                for i in range(a - 2, 0, -1):
                    ls[i] = lam / LOG3
                ls[0] = l - sum(ls[1:])
                if min(ls) < 0:
                    continue
                T, M, lsp, L = narisada_bits_allsplit(n, k, w, a, l, pt, ls)
                if T < best[0]:
                    best = (T, dict(a=a, l=l, pt=pt, lam=lam, ls=ls, M=M, L0P=lsp, L=L))
    return best

def opt_tree_model(n, k, w, amax=7):
    """Narisada Thm 1 (poly-factor-free) over all mode patterns, balanced l_i."""
    best = (float('inf'), None)
    for a in range(2, amax + 1):
        for pat in itertools.product(['split', 'rep'], repeat=a - 1):
            modes = list(pat) + ['split']
            for pt in range(int(0.3 * k), int(0.7 * k), max(1, k // 50)):
                for l in range(40, n - k, max(1, (n - k) // 60)):
                    # leaf size
                    N = k + l; p = pt
                    for m in modes:
                        N = N / 2 if m == 'split' else N
                        p = p / 2
                    La = lb(N, p)
                    if La <= 0:
                        continue
                    for f in (0.6, 0.8, 1.0, 1.2, 1.4, 1.7, 2.0):
                        lam = La * f
                        ls = [0.0] * a
                        ls[a - 1] = (2 * La - lam) / LOG3
                        for i in range(a - 2, 0, -1):
                            ls[i] = lam / LOG3
                        ls[0] = l - sum(ls[1:])
                        if min(ls) < 0:
                            continue
                        T, M, L0, L = narisada_total(n, k, w, a, modes, l, pt, ls)
                        if T < best[0]:
                            best = (T, dict(a=a, modes=modes, l=l, pt=pt, lam=lam, M=M, L0=L0, L=L))
    return best

PARAMS = {
    'UVW128': (9700, 4850, 8633),
    'UVW256': (19200, 9600, 17088),
    'UVW512': (39000, 19500, 34710),
    'WaveI': (8576, 4288, 7668),
    'WaveIII': (12544, 6272, 11226),
    'WaveV': (16512, 8256, 14784),
}

if __name__ == '__main__':
    which = sys.argv[1:] or list(PARAMS)
    for name in which:
        n, k, w = PARAMS[name]
        print(f'== {name} n={n} k={k} w={w}  W={w/n:.4f} asymptotic 0.0151n={0.0151*n:.1f}')
        print('  #solutions log2 = %.1f' % (lb(n, w) + w - (n - k)))
        t, c = best_spec(n, k, w, constrained=True)
        print(f'  (S) spec formula, integer a with BCDL19 constraint : {t:.1f}  a,l={c}')
        t, c = best_spec(n, k, w, constrained=False)
        print(f'  (S) spec formula, integer a, no constraint          : {t:.1f}  a,l={c}')
        t, c = best_spec(n, k, w, constrained=False, leaves_fn=lambda a: 2 ** (a - 1) - 1)
        print(f'  (S) spec formula, DOOM leaves 2^(a-1)-1, no constr  : {t:.1f}  a,l={c}')
        t, c = best_spec_loyer(n, k, w, 'loyer')
        print(f'  (S) Loyer DOOM leaves 2^a-1, Loyer constraint       : {t:.1f}  a,l={c}')
        t, c = best_spec_loyer(n, k, w, 'bcdl')
        print(f'  (S) Loyer DOOM leaves 2^a-1, BCDL-type constraint   : {t:.1f}  a,l={c}')
        r = sendrier_asymptotic(n, k, w)
        print(f'  (A) Sendrier eq.(12) real-a GBA-DOOM                : {r[0]:.1f}  exponent={r[0]/n:.5f} a={r[1]:.2f} l={r[2]:.0f}')
        r = sendrier_asymptotic(n, k, w, doom=False)
        print(f'  (A) same without DOOM (2^a leaves)                  : {r[0]:.1f}  exponent={r[0]/n:.5f} a={r[1]:.2f} l={r[2]:.0f}')
        t, c = best_wagner_plain(n, k, w)
        print(f'  (W) BCDL19 Thm1 plain Wagner (no DOOM)              : {t:.1f}  a,l={c}')
        t, c = best_wagner_plain(n, k, w, doom=True)
        print(f'  (W) BCDL19 Thm1 plain Wagner (DOOM, 2^a-1 leaves)   : {t:.1f}  a,l={c}')
        if '--tree' in sys.argv or True:
            t, c = opt_tree_model(n, k, w, amax=7)
            t, c = refine_tree_model(n, k, w, c)
            print(f"  (N) Narisada Thm1 tree model best                   : {t:.1f}  a={c['a']} modes={''.join(m[0] for m in c['modes'])} l={c['l']} pt={c['pt']} L0={c['L0']:.1f} M={c['M']:.1f}")
        t, c = opt_allsplit_bits(n, k, w)
        for _ in range(2):
            t, c = refine_allsplit_bits(n, k, w, c)
        print(f"  (N) Narisada Prop5 All-Split bit cost               : {t:.1f}  a={c['a']} l={c['l']} pt={c['pt']} log2M={c['M']:.1f} log2(L0 P)={c['L0P']:.1f}")
        print('      lists log2:', ' '.join('%.1f' % x for x in c['L']), ' l_i:', ' '.join('%.0f' % x for x in c['ls']))
