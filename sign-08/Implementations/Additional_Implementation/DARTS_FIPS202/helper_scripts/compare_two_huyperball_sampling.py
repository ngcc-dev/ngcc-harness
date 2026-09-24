# 我们优化了 HAETAE 的双峰拒绝采样，并对两种方法得到的拒绝采样次数进行对比

# 数值示例
B0  =  5688.207384237302
B1  =  5886.17117117636
B   =  5889.615987644186
t   =  201.40860340688386
m = 6  
n = 256

from helper_pycode.repetitions_compute import M_reject_compute
from math import log, exp


def HAETAE_M_reject(B1, B, m, n):
    """
    HAETAE中拒绝采样次数的估计
        :param B1: 中球半径
        :param B:  大球半径
        :param m:  m = k + l
        :param n:  环的维数
    """
    # 直接计算即可，公式参考HAAETAE lemma 5
    return exp(log(2)+m*n*log(B)-m*n*log(B1))

print("改进后的拒绝采样次数： ",M_reject_compute(B1, B0, B, m, n))
print("改进前的拒绝采样次数：", HAETAE_M_reject(B1, B, m, n))
    
# 但由于 mn = 1546， 改进前后差别很小

"""
改进后的拒绝采样次数：  4.91263875946136
改进前的拒绝采样次数： 4.912638759462206
"""