import sys, math, time
sys.path.insert(0, '.')
from estimator import *
from estimator.reduction import MATZOV, ADPS16

# ============================================================
# NIKE-128: n=1024, q=2^13=8192, eta_s=eta_r=2, p_pk=p_u=4096
# ============================================================
n, q, eta_s, eta_r = 1024, 8192, 2, 2
p_pk, p_u = 4096, 4096
delta_pk = q // p_pk    # 2
delta_u  = q // p_u     # 2

def qerr_support(delta):
    h = delta // 2
    return -h + 1, h
def cbd_stddev(eta):
    return math.sqrt(float(eta) / 2.0)

Xs = ND.CenteredBinomial(eta_s)
Xr = ND.CenteredBinomial(eta_r)
a1, b1 = qerr_support(delta_pk)
a2, b2 = qerr_support(delta_u)
Xe_pk = ND.Uniform(a1, b1)
Xe_u  = ND.Uniform(a2, b2)

print('='*70)
print('  NIKE-128 FULL Security Estimation')
print(f'  n={n}, q={q}, eta_s={eta_s}, eta_r={eta_r}')
print(f'  p_pk={p_pk} (Delta={delta_pk}), p_u={p_u} (Delta={delta_u})')
print(f'  sigma_s={cbd_stddev(eta_s):.2f}, sigma_e_pk=0.50, sigma_e_u=0.50')
print('='*70)

# --- pk surface (ADPS16) ---
pk_params = LWE.Parameters(n=n, q=q, Xs=Xs, Xe=Xe_pk, m=n, tag='NIKE-128-pk')
print('\n--- pk surface (LWE: secret=s, error=chi_{q,p_pk}) ---')
t0 = time.time()
pk_results = LWE.estimate(pk_params)
print(f'  (elapsed: {time.time()-t0:.1f}s)')

# --- u surface (ADPS16) ---
u_params = LWE.Parameters(n=n, q=q, Xs=Xr, Xe=Xe_u, m=n, tag='NIKE-128-u')
print('\n--- u surface (LWE: secret=r, error=chi_{q,p_u}) ---')
t0 = time.time()
u_results = LWE.estimate(u_params)
print(f'  (elapsed: {time.time()-t0:.1f}s)')

# --- pk surface (MATZOV) ---
print('\n--- pk surface (MATZOV) ---')
t0 = time.time()
pk_matzov = LWE.estimate(pk_params, red_cost_model=MATZOV)
print(f'  (elapsed: {time.time()-t0:.1f}s)')

# --- u surface (MATZOV) ---
print('\n--- u surface (MATZOV) ---')
t0 = time.time()
u_matzov = LWE.estimate(u_params, red_cost_model=MATZOV)
print(f'  (elapsed: {time.time()-t0:.1f}s)')

# --- Extract min rop per attack across both surfaces ---
def min_rop_across(r1, r2):
    best = {}
    for results in [r1, r2]:
        for k, v in results.items():
            if hasattr(v, '__contains__') and 'rop' in v:
                rop_bits = math.log2(v['rop'])
                if k not in best or rop_bits < best[k]:
                    best[k] = rop_bits
    return best

def get_beta(results):
    for k, v in results.items():
        if hasattr(v, '__contains__') and 'beta' in v:
            return v['beta']
    return None

all_best = min_rop_across(pk_results, u_results)
beta_adps16 = get_beta(pk_results) or get_beta(u_results)

print(f'\n{"="*70}')
print(f'  NIKE-128 -- FULL SECURITY SUMMARY')
print(f'{"="*70}')

# ---- Core-SVP (ADPS16) ----
print(f'\n  {"Core-SVP (ADPS16)":-^50}')
print(f'  {"Attack":>18s}  {"log2(rop)":>10s}')
print(f'  {"-"*18}  {"-"*10}')
csvp_c = None
for attack in ['usvp', 'bdd', 'bdd_hybrid', 'dual', 'dual_hybrid', 'bkw']:
    if attack in all_best:
        print(f'  {attack:>18s}  {all_best[attack]:10.1f}')
        if csvp_c is None or all_best[attack] < csvp_c:
            csvp_c = all_best[attack]
print(f'  {"-"*18}  {"-"*10}')
if csvp_c:
    print(f'  {"Core-SVP Classical":>18s}  {csvp_c:10.1f}')
if beta_adps16:
    csvp_q_adps16 = 0.265 * beta_adps16
    print(f'  {"Core-SVP Quantum":>18s}  {csvp_q_adps16:10.1f}  (beta={beta_adps16})')

# ---- MATZOV ----
matzov_best = min_rop_across(pk_matzov, u_matzov)
beta_matzov = get_beta(pk_matzov) or get_beta(u_matzov)

print(f'\n  {"MATZOV":-^50}')
print(f'  {"Attack":>18s}  {"log2(rop)":>10s}')
print(f'  {"-"*18}  {"-"*10}')
matzov_c = None
for attack in ['usvp', 'bdd', 'bdd_hybrid']:
    if attack in matzov_best:
        print(f'  {attack:>18s}  {matzov_best[attack]:10.1f}')
        if matzov_c is None or matzov_best[attack] < matzov_c:
            matzov_c = matzov_best[attack]
print(f'  {"-"*18}  {"-"*10}')
if matzov_c:
    print(f'  {"MATZOV Classical":>18s}  {matzov_c:10.1f}')
if beta_matzov:
    matzov_q = 0.292 * beta_matzov
    print(f'  {"MATZOV Quantum":>18s}  {matzov_q:10.1f}  (beta={beta_matzov})')

# ---- Summary ----
print(f'\n{"="*70}')
print(f'  NIKE-128 :: SECURITY MARGINS vs 128-bit target')
print(f'{"="*70}')
if csvp_c:
    print(f'  Core-SVP Classical (ADPS16):  {csvp_c:6.1f}  |  margin: {csvp_c-128:+.1f}')
if beta_adps16:
    print(f'  Core-SVP Quantum   (ADPS16):  {csvp_q_adps16:6.1f}  |  margin: {csvp_q_adps16-128:+.1f}')
if matzov_c:
    print(f'  MATZOV   Classical          :  {matzov_c:6.1f}  |  margin: {matzov_c-128:+.1f}')
if beta_matzov:
    print(f'  MATZOV   Quantum  (0.292*b) :  {matzov_q:6.1f}  |  margin: {matzov_q-128:+.1f}')
