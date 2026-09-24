#############################################################
#   我们对HAETAE的双峰拒绝采样进行了改进：
#   目标分布不再是半径为 B1 的超球上的均匀分布， 
#   而是在半径为 B1 的超球中又选定一半径为 B_0 的小球，
#   选取半径为 B_0 的小球中的点的概率是大球其余部分概率的 2 倍，
#   且小球中概率分布均匀，大球其余部分概率分布也均匀
#   目标分布与 s 无关
#############################################################


import scipy.special as sc
from math import log, pi, gamma, ceil, floor
import numpy as np


def vol_ball(m, r):
    # m 维 半径 r 的超球的体积
    return (pi ** (m/2)) / gamma(m/2 + 1) * (r ** m)


def prob_slab(n, m, B1, a, b):
    """
    HAETAE中的目标分布 (m 维) 的每个坐标在 [a,b] 中的概率
        m:  m = k + l
        n:  环的维度
        B1: 中球的半径
    """
    dim = n * m  # 超球的维数
    B1 = floor(B1) # 取整
    
    if a > b or abs(a) > B1 or abs(b) > B1: 
        raise ValueError('Invalid interval')
    
    # 归一化
    a = a / B1
    b = b / B1
    
    # S(t) = Pr[X > t]，其中 X 为 m 维超球的其中一个坐标 
    def S(t:float) -> float:
        if t >= 0:
            return 0.5 * sc.betainc((dim+1)/2, 0.5, 1 - t*t)
        else:
            # 当 t < 0 时利用分布的对称性求解概率
            return 1 - S(-t)
    return S(a) - S(b)


def refined_prob_slab(n, m, B1, B0, a, b):
    """
    计算目标分布 (m 维) 的每个坐标在 [a,b] 中的概率
    """
    dim = n * m  # 超球的维数
    
    if a > b or abs(a) > B1 or abs(b) > B1: 
        raise ValueError('Invalid interval')
    if B0 > B1:
        raise ValueError('B0 must be <= B1')

    # 归一化
    a = a / B1
    b = b / B1
    r = B0 / B1  # 小球半径归一化
    
    # S(t) = Pr[X > t]，其中 X 为 m 维超球的其中一个坐标 
    def S(t:float) -> float:
        if t >= 0:
            if t <= r:
                return 1/(2*(1+r**dim)) * (sc.betainc((dim+1)/2, 0.5, 1 - t*t) + (r**dim) * sc.betainc((dim+1)/2, 0.5, 1 - (t/r)*(t/r)))
            elif t > r:
                return 1/(2*(1+r**dim)) * sc.betainc((dim+1)/2, 0.5, 1 - t*t)
        else:
            # 当 t < 0 时利用分布的对称性求解概率
            return 1.0 - S(-t)
    return S(a) - S(b)


def plogp(p):
    """
    概率太小时信息熵过大我们不考虑这些极端情形，仅考虑概率 > 1e-16
    输入：
        p
    输出：
        p * log(p, 2)
    """
    return (p + 1e-16) * np.log2(p + 1e-16)


def compute_entropy_coordinate(n, m, B1):
    """
    HAETAE中计算 m 维向量中单个坐标的信息熵
    输入：
        m:  m = k + l
        n:  环的维度
        B1: 中球的半径
        B0: 小球的半径
    输出：
        单个坐标的信息熵
    """
    B1 = floor(B1) # 取整

    # 计算 rounding 后每个取值的概率
    prob = np.zeros(2*B1+1)
    count = 0
    
    # 左端点
    prob[count] = prob_slab(n, m, B1, -B1, -B1+1/2)
    count = count + 1
    for i in range(-B1+1,B1):
        prob[count] = prob_slab(n, m, B1, i-1/2, i+1/2)
        count = count + 1
        
    # 右端点
    prob[count] = prob_slab(n, m, B1, B1-1/2, B1)
    entropy = -np.sum(plogp(prob)) 
    # B1 不一定是整数，为什么左端点和右端点都是用 1/2，而中间是 1 ?
    # 因为我们最后是对采样后的 y 取 rounding 
    return entropy


def refined_compute_entropy_coordinate(n, m, B1, B0):
    """
    计算 m 维向量中单个坐标的信息熵
    调用 refined_prob_slab(n, m, B1, B0, a, b) 函数计算信息熵
    输入：
        m:  m = k + l
        n:  环的维度
        B1: 中球的半径
        B0: 小球的半径
    输出：
        单个坐标的信息熵
    """
    B1 = floor(B1) # 取整
    B0 = floor(B0) # 取整

    # 计算 rounding 后每个取值的概率
    prob = np.zeros(2*B1+1)
    
    # 左端点
    count = 0
    prob[count] = refined_prob_slab(n, m, B1, B0, -B1, -B1+1/2)
    count = count + 1

    # 中间部分
    for i in range(-B1+1,B1):
        prob[count] = refined_prob_slab(n, m, B1, B0, i-1/2, i+1/2)
        count = count + 1
        
    # 右端点
    prob[count] = refined_prob_slab(n, m, B1, B0, B1-1/2, B1)
    entropy = -np.sum(plogp(prob)) 
    # B1 不一定是整数，为什么左端点和右端点都是用 1/2，而中间是 1 ?
    # 因为我们最后是对采样后的 y 取 rounding 
    return entropy


def refined_compute_f_table(n, m, B1, B0, d, q):
    """
    计算rANS编码后的高位取值概率分布和低位取值概率分布
    我们采用的不是直接截断而是 Compress 和 Decompress 方法
    这对决定是否舍入的那一位需要单独讨论
    该代码考虑的是 B1 < q 的情形，此时不需要考虑模 q 对超球采样后分布的影响
    输入：
        :param n:  环的维度
        :param m:  m = k + l
        :param B1: 中球的半径
        :param B0: 小球的半径
        :param d:  压缩后的位数，即舍弃的低位比特数
        :param q:  模数
    输出：
        prob     : 高位概率分布
        low_prob : 低位概率分布
    """
    B1 = floor(B1) # 取整
    B0 = floor(B0) # 取整

    def compress(r, d, q):
        # 压缩函数
        r = r % q
        r = round(r*(2**d)/q) % (2**d)
        return r
    def decompress(r,d,q):
        # 解压缩函数
        r = round(r*q/(2**d)) % q
        return r 
    
    length = ceil(log(q,2))
    
    high_prob = [0 for _ in range(2**d)]
    low_prob  = [0 for _ in range(2**(length - d))]
    
    curr_p = refined_prob_slab(n, m, B1, B0, -B1, -B1+1/2)
    high_prob[compress(-B1, d, q)] += curr_p
    
    # 低位不压缩？
    # low_prob = []
    
    for i in range(-B1+1, B1):
        curr_p = refined_prob_slab(n, m, B1, B0, i-1/2, i+1/2)
        high_prob[compress(i, d, q)] += curr_p
        # low_prob = 1
    
    curr_p = refined_prob_slab(n, m, B1, B0, B1-1/2, B1)
    high_prob[compress(B1, d, q)] += curr_p
    # 精度会有一些损失导致概率和不等于1
    return (high_prob, low_prob)