import sys, math, time, os
sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from estimator import *
from estimator.reduction import MATZOV

PROFILES = {
    "NIKE-128": dict(n=512,  q=2**13, eta_s=2, eta_r=2, p_pk=2**9,  p_u=2**9,  p_v=2**6, target=128),
    "NIKE-192": dict(n=1024, q=2**13, eta_s=2, eta_r=2, p_pk=2**11, p_u=2**11, p_v=2**6, target=192),
    "NIKE-256": dict(n=1024, q=2**13, eta_s=2, eta_r=2, p_pk=2**10, p_u=2**10, p_v=2**6, target=256),
    "NIKE-384": dict(n=2048, q=2**13, eta_s=2, eta_r=2, p_pk=2**12, p_u=2**12, p_v=2**6, target=384),
    "NIKE-512": dict(n=2048, q=2**13, eta_s=2, eta_r=2, p_pk=2**10, p_u=2**10, p_v=2**6, target=512),
}

def qerr_support(delta):
    h = delta // 2
    return -h + 1, h

for name, p in PROFILES.items():
    n, q, target = p['n'], p['q'], p['target']
    delta_pk = q // p['p_pk']
    delta_u  = q // p['p_u']
    
    Xs = ND.CenteredBinomial(p['eta_s'])
    Xr = ND.CenteredBinomial(p['eta_r'])
    a1, b1 = qerr_support(delta_pk)
    a2, b2 = qerr_support(delta_u)
    Xe_pk = ND.Uniform(a1, b1)
    Xe_u  = ND.Uniform(a2, b2)
    
    pk_params = LWE.Parameters(n=n, q=q, Xs=Xs, Xe=Xe_pk, m=n, tag=f'{name}-pk')
    u_params  = LWE.Parameters(n=n, q=q, Xs=Xr, Xe=Xe_u,  m=n, tag=f'{name}-u')
    
    sep = '=' * 60
    print(f'\n{sep}')
    print(f'  {name}: n={n}, q={q}, delta_pk={delta_pk}, delta_u={delta_u}')
    print(sep)
    
    t0 = time.time()
    pk_r = LWE.estimate(pk_params, red_cost_model=MATZOV)
    u_r  = LWE.estimate(u_params,  red_cost_model=MATZOV)
    dt = time.time() - t0
    
    best_rop = 1e300
    best_beta = None
    best_attack = ''
    for label, r in [('pk', pk_r), ('u ', u_r)]:
        for k, v in r.items():
            if hasattr(v, '__contains__') and 'rop' in v:
                rop = math.log2(v['rop'])
                if rop < best_rop:
                    best_rop = rop
                    best_attack = f'{label}/{k}'
            if hasattr(v, '__contains__') and 'beta' in v:
                if best_beta is None or v['beta'] < best_beta:
                    best_beta = v['beta']
    
    csvp_q = 0.292 * best_beta if best_beta else 0
    margin_c = best_rop - target
    margin_q = csvp_q - target
    
    print(f'  MATZOV C: {best_rop:.1f}  |  MATZOV Q: {csvp_q:.1f}  (beta={best_beta})')
    print(f'  Margin C: {margin_c:+.1f}  |  Margin Q: {margin_q:+.1f}')
    print(f'  Attack: {best_attack}  ({dt:.0f}s)')
