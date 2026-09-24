from math import *
from .model_BKZ import *

log_infinity = 9999

STEPS_m = 1 # 遍历 m 时的步长，大于 1 可能找不到最优的 b 或 m 

class MLWEParameterSet:
    def __init__(self, n, d, m, q, ternary_p):
        self.n = n          # 环维数
        self.d = d          # MLWE的维数 
        self.m = m          # 样本数
        self.q = q          # 模数
        self.p = ternary_p  # 三元分布取 1 和 -1 的概率参数

def LWE_primal_cost(q, n, m, s, b, cost_svp=svp_classical, verbose=False):
    """ Return the cost of the primal attack using m samples and blocksize b (infinity = fail)
    """
    d = n + m + 1
    if verbose:
        print("Primal attacks uses block-size %d and %d samples; dim d=%d"%(b, m, d))

    if s * sqrt(b) < BKZ_last_block_length(q, m, n + 1, b):
        return cost_svp(b)
    else:
        return log_infinity

def MNLWE_primal_cost(q, n, m, s, b, cost_svp=svp_classical, verbose=False):
    """ Return the cost of the primal attack using m samples and blocksize b (infinity = fail)
    """
    d = n + m
    if verbose:
        print("Primal attacks uses block-size %d and %d samples; dim d=%d"%(b, m, d))

    if s * sqrt(b) < BKZ_last_block_length(q, m, n, b):
        return cost_svp(b)
    else:
        return log_infinity

def LWE_dual_cost(q, n, m, s, b, cost_svp=svp_classical, verbose=False):
    """ Return the cost of the dual attack using m samples and blocksize b (infinity = fail)
    """
    d = n + m
    l = BKZ_first_length(q, n, m, b)
    tau = l * s / q
    log2_eps = - 2 * pi * pi * tau**2 / log(2)
    log2_R = max(0, - 2 * log2_eps - nvec_sieve(b))
    if verbose:
        print ("Dual attacks uses block-size %d and %d samples; dim d=%d"%(b, m, d))
        print ("shortest vector used has length l=%.2f, q=%d, `l<q'= %d"%(l, q, l<q))
        print ("log2(epsilon) = %.2f, log2 nvector per run %.2f"%(log2_eps, nvec_sieve(b)))
    return cost_svp(b) + log2_R


def MLWE_optimize_attack(q, n, max_m, s, cost_attack=LWE_primal_cost, cost_svp=svp_classical, verbose=True):
    """ Find optimal parameters for a given attack
    """
    best_cost = log_infinity
    best_b = None
    b_min, b_max = 50, n+max_m
    b_step = max(1, (b_max - b_min)//4)
    while b_step > 0:
        for b in range(b_min, b_max+1, b_step):
            if cost_svp(b) > best_cost:
                # cost_svp is monotone increasing with b, so we can truncate search space
                b_max = b-1
                break
            for m in range(max_m, max(0, b-n), -STEPS_m):
                cost = cost_attack(q, n, m, s, b, cost_svp)
                if cost == log_infinity:
                    break # Decreasing m will not improve cost.
                if cost <= best_cost: # "<=" instead of "<" since smaller m is better
                    (best_cost, best_m, best_b) = (cost, m, b)
                    # The optimal b is no smaller than b-b_step+1
                    b_min = max(b_min, b - b_step + 1)
        # Final b_step is 1 since x//2 >= 2 for x > 3 and 3//2 == 2//2 == 1
        b_step = b_step//2

    cost_attack(q, n, best_m, s, best_b, cost_svp=svp_classical, verbose=verbose)
    return (best_m, best_b, best_cost)


def check_eq(m_pc, m_pq, m_pp):
    if (m_pc != m_pq):
        print("m and b not equals among the three models")
    if (m_pq != m_pp):
        print("m and b not equals among the three models")


def MLWE_summarize_attacks(ps):
    """ Create a report on the best primal and dual BKZ attacks on an MLWE instance
    """
    q = ps.q
    n = ps.n * ps.d
    max_m = ps.n * ps.m
    p = ps.p
    
    # 三元分布的标准差
    s = sqrt(2*p)

    (m_pc, b_pc, c_pc) = MLWE_optimize_attack(q, n, max_m, s, cost_attack=LWE_primal_cost, cost_svp=svp_classical, verbose=False)
    (m_pq, b_pq, c_pq) = MLWE_optimize_attack(q, n, max_m, s, cost_attack=LWE_primal_cost, cost_svp=svp_quantum, verbose=False)
    (m_pp, b_pp, c_pp) = MLWE_optimize_attack(q, n, max_m, s, cost_attack=LWE_primal_cost, cost_svp=svp_plausible, verbose=False)

    check_eq(m_pc, m_pq, m_pp)
    check_eq(b_pc, b_pq, b_pp)

    print("Primal & %d & %d & %d & %d & %d"%(m_pq, b_pq, int(floor(c_pc)), int(floor(c_pq)), int(floor(c_pp))))

    (m_pc, b_pc, c_pc) = MLWE_optimize_attack(q, n, max_m, s, cost_attack=LWE_dual_cost, cost_svp=svp_classical, verbose=False)
    (m_pq, b_pq, c_pq) = MLWE_optimize_attack(q, n, max_m, s, cost_attack=LWE_dual_cost, cost_svp=svp_quantum, verbose=False)
    (m_pp, b_pp, c_pp) = MLWE_optimize_attack(q, n, max_m, s, cost_attack=LWE_dual_cost, cost_svp=svp_plausible, verbose=False)

    check_eq(m_pc, m_pq, m_pp)
    check_eq(b_pc, b_pq, b_pp)

    print("Dual & %d & %d & %d & %d & %d "%(m_pq, b_pq, int(floor(c_pc)), int(floor(c_pq)), int(floor(c_pp))))
    return (b_pq, int(floor(c_pc)), int(floor(c_pq)), int(floor(c_pp)))

def MNLWE_summarize_attacks(ps):
    """ Create a report on the best primal and dual BKZ attacks on an MLWE instance
    """
    q = ps.q
    n = ps.n * ps.d
    max_m = ps.n * ps.m
    p = ps.p
    
    # 三元分布的标准差
    s = sqrt(2*p)

    (m_pc, b_pc, c_pc) = MLWE_optimize_attack(q, n, max_m, s, cost_attack=MNLWE_primal_cost, cost_svp=svp_classical, verbose=False)
    (m_pq, b_pq, c_pq) = MLWE_optimize_attack(q, n, max_m, s, cost_attack=MNLWE_primal_cost, cost_svp=svp_quantum, verbose=False)
    (m_pp, b_pp, c_pp) = MLWE_optimize_attack(q, n, max_m, s, cost_attack=MNLWE_primal_cost, cost_svp=svp_plausible, verbose=False)

    check_eq(m_pc, m_pq, m_pp)
    check_eq(b_pc, b_pq, b_pp)

    print("Primal & %d & %d & %d & %d & %d"%(m_pq, b_pq, int(floor(c_pc)), int(floor(c_pq)), int(floor(c_pp))))

    #(m_pc, b_pc, c_pc) = MLWE_optimize_attack(q, n, max_m, s, cost_attack=LWE_dual_cost, cost_svp=svp_classical, verbose=False)
    #(m_pq, b_pq, c_pq) = MLWE_optimize_attack(q, n, max_m, s, cost_attack=LWE_dual_cost, cost_svp=svp_quantum, verbose=False)
    #(m_pp, b_pp, c_pp) = MLWE_optimize_attack(q, n, max_m, s, cost_attack=LWE_dual_cost, cost_svp=svp_plausible, verbose=False)

    #check_eq(m_pc, m_pq, m_pp)
    #check_eq(b_pc, b_pq, b_pp)

    #print("Dual & %d & %d & %d & %d & %d "%(m_pq, b_pq, int(floor(c_pc)), int(floor(c_pq)), int(floor(c_pp))))
    return (b_pq, int(floor(c_pc)), int(floor(c_pq)), int(floor(c_pp)))

