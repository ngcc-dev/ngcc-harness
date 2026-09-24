from math import sqrt,ceil

# 估计离散化系数 N 的值（下界）

# 默认 M0 和 c 的值
M0 = 1.001
c = 1.001 


def N_1_compute(m, n, B, M0=None):
    """
    利用连续的超球采样 实现 离散超球采样时对 N 的要求
    :param m:  m = k + l
    :param n:  环的维数
    :param B:  采样 y 时的超球的半径
    :param M0: 成功概率为 1/M0
    """
    if M0 == None:
        M0 = 1.001
    return (sqrt(m*n)/(2*B)) * (M0**(1/(m*n))+1)/(M0**(1/(m*n))-1)


def N_2_compute(m, n, B, B_candidate, c=None):
    """
    双峰拒绝采样时对 N 的要求
    :param m:  m = k + l
    :param n:  环的维数
    :param B:  采样 y 时的超球的半径
    :param B:  采样 y 时的超球的半径
    :param B:  采样 y 时的超球的半径
    :param c:  M' = cM，其中 M' 为离散超球的拒绝采样次数，M 为连续超球的拒绝采样次数
    """
    if c == None:
        c = 1.001
    return 1/(c**(1/(m*n))-1) * sqrt(m*n/4) * (c**(1/(m*n))/B + 1/B_candidate)

# Security 128
m = 3
n = 512
B = 6049.79
B_0 = 5858.47
B_1 = 6046.76
Bound1 = N_1_compute(m, n, B, M0)
Bound2_1 = N_2_compute(m, n, B, B_0, c)
Bound2_2 = N_2_compute(m, n, B, B_1, c)
print("Security 128:")
print("N lower bounds:", Bound1)
print("N lower bounds:", Bound2_1)
print("N lower bounds:", Bound2_2)
# Security 256
m = 5
n = 512
B = 18906.24
B_0 = 18429.40 
B_1 = 18900.22 
Bound1 = N_1_compute(m, n, B, M0)
Bound2_1 = N_2_compute(m, n, B, B_0, c)
Bound2_2 = N_2_compute(m, n, B, B_1, c)
print("Security 256:")
print("N lower bounds:", Bound1)
print("N lower bounds:", Bound2_1)
print("N lower bounds:", Bound2_2)
# Security 512
m = 6
n = 1024
B = 74206.14
B_0 = 73426.15 
B_1 = 74202.04
Bound1 = N_1_compute(m, n, B, M0)
Bound2_1 = N_2_compute(m, n, B, B_0, c)
Bound2_2 = N_2_compute(m, n, B, B_1, c)
print("Security 512:") 
print("N lower bounds:", Bound1)
print("N lower bounds:", Bound2_1)
print("N lower bounds:", Bound2_2)

"""
Security 128:
N lower bounds: 9955.511552372876
N lower bounds: 10118.070023312728
N lower bounds: 9958.005879102328
Security 256:
N lower bounds: 6854.437775306832
N lower bounds: 6943.113192594514
N lower bounds: 6855.529395069779
Security 512:
N lower bounds: 6493.128920227418
N lower bounds: 6527.616464614112
N lower bounds: 6493.308307656711

Thus we set N =16384 for all security levels, which is much larger than the lower bounds.
"""