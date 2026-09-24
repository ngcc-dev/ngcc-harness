from math import log, sqrt
from .model_BKZ import *
from scipy.special import betainc
from .proba_util import gaussian_center_weight

log_infinity = 9999

STEPS_b = 1
STEPS_m = 5

class MSISParameterSet:
    def __init__(self, n, w, h, B, q, method):
        self.n = n           # 环 的维数
        self.w = w           # MSIS 的维数
        self.h = h           # 方程的个数
        self.B = B           # 范数上界（2范数或无穷范数）
        self.q = q           # 模数
        self.method = method # 评估参数的方法

def SIS_linf_cost(q, w, h, B, b, cost_svp=svp_classical, verbose=False):
    """ 
    Return the cost of finding a vector shorter than B in infinity norm, using BKZ-b, if it works.
    The equation is Ax = 0 mod q, where A has h rows, and w columns (h equations in dim w).
    """
    (i, j, L) = construct_BKZ_shape_randomized(q, h, w-h, b)
    
    l = exp(L[i])
    d = j - i + 1
    sigma = l / sqrt(j - i + 1)
    p_middle = gaussian_center_weight(sigma, B)
    p_head = 2.*B / q

    log2_eps = d * log(p_middle, 2) + i * log(p_head, 2)
    log2_R = max(0, - log2_eps - nvec_sieve(b)) 

    if verbose:
        print ("Attack uses block-size %d and %d dimensions, with %d q-vectors"%(b, w, i))
        print ("log2(epsilon) = %.2f, log2 nvector per run %.2f"%(log2_eps, nvec_sieve(b)))
        print ("shortest vector used has length l=%.2f, q=%d, `l<q'= %d"%(l, q, l<q))
    return cost_svp(b) + log2_R

def Dilithum_SIS_l2_cost(q, w, h, B, b, cost_svp=svp_classical, verbose=False):
    """ 
    Return the cost of finding a vector shorter than B with BKZ-b if it works.
    The equation is Ax = 0 mod q, where A has h rows, and w collumns (h equations in dim w).
    """
    if B>=q:
        if verbose:
            print ("B>=q")
    l = BKZ_first_length(q, h, w-h, b)
    if l > B:
        return log_infinity 
    if verbose:
        print ("Attack uses block-size %d and %d equations"%(b, h))
        print ("shortest vector used has length l=%.2f, q=%d, `l<q'= %d"%(l, q, l<q))
    return cost_svp(b)

def HAETAE_SIS_l2_cost(q, w, h, B, b, cost_svp=svp_classical, verbose=False):
    """ Return the cost of finding a vector shorter than B with BKZ-b if it works.
    The equation is Ax = 0 mod q, where A has h rows, and w collumns (h equations in dim w).
    """
    def volume(d, r):
        res = (2*r)**(d%2)
        for i in range(d//2):
            res = res*(2*pi)*r**2/(2*i+2+d%2)
        return res
    if B>=sqrt(w)*q/2:
        if verbose:
            print ("Norm too big. Trivial attack. Concluding 0 bits of security.")
        return 0
    l = BKZ_first_length(q, h, w-h, b)
    if l > B:
        if B<q:
            return log_infinity
        (i,_,L) = construct_BKZ_shape(q, h, w-h, b)
        l = exp(L[i])
        #Even if the first i-1 coordinates are 0, the vector is too long.
        if (l > B):
            return log_infinity
        r = sqrt(B**2-l**2)
        #Whatever the first coordinates, the vector will be short enough.
        if r>= sqrt(i-1)*floor(q/2):
            return cost_svp(b)
        #In other cases, we compute the probability that the vector is short enough
        h1 = r-floor(q/2)
        p_1 = volume(i-1,r/q)*(1-betainc(i/2.,1/2.,(2*r*h1-h1**2)/r**2))
        #print(p_1)
        if 1-(1-p_1)**(2**nvec_sieve(b))<=0:
            log_p_head = -log_infinity
        else:
            log_p_head = log(1-(1-p_1)**(2**nvec_sieve(b)),2)
        #if(erf(sqrt(10)*(3*(r/floor(q/2))**2-(i-1))/(4*sqrt(i-1))) + erf(sqrt(10*(i-1))/4))<=0:
        #    #print("Warning, accuracy too low")
        #    log_p_head = -log_infinity
        #else:
        #    log_p_head = log( 1-(1-p)**(exp(b*.2075))  ,2)
        #    log_p_head = (i-1)*log(2*floor(q/2)/q,2) + log(erf(sqrt(10)*(3*(r/floor(q/2))**2-(i-1))/(4*sqrt(i-1))) + erf(sqrt(10*(i-1))/4),2)-1
        return cost_svp(b) + max(0, - log_p_head)
        #COMPUTE THIS DIFFERENTLY
    if verbose:
        print ("Attack uses block-size %d and %d equations"%(b, h))
        print ("shortest vector used has length l=%.2f, q=%d, `l<q'= %d"%(l, q, l<q))
    return cost_svp(b)

def refined_SIS_l2_cost(q, w, h, B, b, cost_svp=svp_classical, verbose=False):
    """
    Numerically-stable high-precision version using mpmath.
    """
    import mpmath as mp
    from scipy.special import betainc as scipy_betainc
    mp.mp.dps = 50   # 设置 50 位小数精度，可按需加大

    def volume_mp(d, r):
        r = mp.mpf(r)
        return (mp.pi**(mp.mpf(d)/2) / mp.gamma(mp.mpf(d)/2 + 1)) * (r**mp.mpf(d))

    # 转化为 mp 数值
    q_mp = mp.mpf(q)
    w_mp = mp.mpf(w)
    B_mp = mp.mpf(B)

    
    if B_mp >= mp.sqrt(w_mp) * q_mp / 2:
        if verbose:
            print("Norm too big. Trivial attack. Concluding 0 bits of security.")
        return 0

    (i, _, L) = construct_BKZ_shape(q, h, w-h, b)

    L_i_mp = mp.mpf(L[i])
    l_mp = mp.e ** L_i_mp   # mp.exp(L_i_mp) also fine

    if l_mp > B_mp:
        return log_infinity

    diff = B_mp**2 - l_mp**2
    if diff <= 0:
        r_mp = mp.mpf('0')
    else:
        r_mp = mp.sqrt(diff)

    if i > 0 and r_mp >= mp.sqrt(mp.mpf(i-1)) * mp.floor(q_mp/2):
        return cost_svp(b)

    
    h1_mp = r_mp - mp.floor(q_mp/2)
    beta_arg = (2*r_mp*h1_mp - h1_mp**2) / (r_mp**2) if r_mp != 0 else mp.mpf(0)
    beta_term = mp.mpf(scipy_betainc(float(i)/2.0, 0.5, float(beta_arg)))
    vol = volume_mp(i-1, r_mp / q_mp)
    
    # 一个向量的成功概率
    p1_mp = vol * (1 - beta_term)


    if p1_mp <= 0:
        if verbose:
            print("p1 computed <= 0 -> impossible single-vector success.")
        return log_infinity
    if p1_mp >= 1:
        p = mp.mpf(1)
    else:
        # 计算总体成功概率：1 - (1 - p1) ^ n
        nvec = 2 ** nvec_sieve(b)   
        n_mp = mp.mpf(nvec)
        ln_term = n_mp * mp.log1p(-p1_mp)

        if ln_term <= mp.log(mp.mpf('1e-300')): 
            p = mp.mpf(1)
        else:
            p = - mp.expm1(ln_term)

    # numerical safeguards for p
    if p <= 0:
        if verbose:
            print("Final success probability p <= 0 (underflow).")
        return log_infinity
    if p >= 1:
        log_p_head_mp = mp.mpf(0)
    else:
        log_p_head_mp = mp.log(p, 2)   # base-2 log in mp

    if verbose:
        print("Attack uses block-size %d and %d equations" % (b, h))
        print("shortest vector used has length l=%.6g, q=%d, `l<q`= %d" % (float(l_mp), q, l_mp < q))

    # return cost (convert log part to positive float bits)
    return cost_svp(b) + max(0, - float(log_p_head_mp))

def SIS_optimize_attack(q, max_w, h, B, cost_attack=SIS_linf_cost, cost_svp=svp_classical, verbose=False):
    """ Find optimal parameters for a given attack
    """
    best_cost = log_infinity

    for b in range(50, max_w, STEPS_b):
        if cost_svp(b) > best_cost:
            break
        for w in [max_w]:    # No need to exhaust w here as the attack will auto-adjust anyway  range(max(h+1, b+1), max_w, STEPS_m):
            cost = cost_attack(q, w, h, B, b, cost_svp)
            if cost<=best_cost:
                best_cost = cost
                best_w = w
                best_b = b

    if verbose:
        cost_attack(q, best_w, h, B, best_b, cost_svp=cost_svp, verbose=verbose)

    return (best_w, best_b, best_cost)

def check_eq(m_pc, m_pq, m_pp):
    if (m_pc != m_pq):
        print("m and b not equals among the three models")
    if (m_pq != m_pp):
        print("m and b not equals among the three models")

def MSIS_summarize_attacks(ps):
    """ Create a report on the best primal and dual BKZ attacks on an l_oo - MSIS instance
    """
    q = ps.q
    h = ps.n * ps.h
    max_w = ps.n * ps.w
    B = ps.B

    if ps.method == "linf":
        attack = SIS_linf_cost
    elif ps.method == "l2":
        attack = Dilithum_SIS_l2_cost
    elif ps.method == "HAETAE_l2":
        attack = HAETAE_SIS_l2_cost
    elif ps.method == "refined_l2":
        attack = refined_SIS_l2_cost
    else:
        raise ValueError("Unknown method: " + ps.method)

    (m_pc, b_pc, c_pc) = SIS_optimize_attack(q, max_w, h, B, cost_attack=attack, cost_svp=svp_classical, verbose=False)
    (m_pq, b_pq, c_pq) = SIS_optimize_attack(q, max_w, h, B, cost_attack=attack, cost_svp=svp_quantum, verbose=False)
    (m_pp, b_pp, c_pp) = SIS_optimize_attack(q, max_w, h, B, cost_attack=attack, cost_svp=svp_plausible, verbose=False)

    check_eq(m_pc, m_pq, m_pp)
    check_eq(b_pc, b_pq, b_pp)

    print("SIS & %d & %d & %d & %d & %d"%(m_pq, b_pq, int(floor(c_pc)), int(floor(c_pq)), int(floor(c_pp))))

    return (b_pq, int(floor(c_pc)), int(floor(c_pq)), int(floor(c_pp)))