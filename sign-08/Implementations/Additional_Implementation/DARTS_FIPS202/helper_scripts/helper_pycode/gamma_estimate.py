import numpy as np
import scipy.fft as dft
from math import sqrt


def build_ternary_array(n, num1, num_1):
    """ 字典表示三元分布的分布列
    :参数 n, num1, num_1: (integer)
    :return: 三元分布的分布列
    """
    # 构造三元分布的分布列
    random_array = np.random.choice([1, 0, -1], size=n, p=[num1/n, 1 - (num1 + num_1)/n, num_1/n])
    return random_array 


def gamma_estimate(N, n, num1, dim, tau, rate):
    """
    采样 N 次secret，计算每次的gamma值
    返回一个列表，百分比点由 rate 参数指定
 
    N: 采样次数
    n: 多项式的维数
    num1: secret 中 1 和 -1 的个数
    dim: secret 的维数
    tau: c 的汉明重量
    rate: 一个列表，表示要返回的百分比点，例如 rate = [10, 25, 50] 表示返回 10%、25%、50% 分位点

    例如：gamma_estimate(1000, 512, 64, 4, 16, [10,25,50,75,90])

        代表采样 1000 次 secret，
        多项式维数 512，secret 中 1 和 -1 的个数为 64，
        secret 维数为 4，
        c 的汉明重量为 16，
        返回 10%、25%、50%、75%、90% 分位点的 gamma 值
    """
    res = []
    i_max = n//tau
    leftover = n % tau

    for _ in range(N):

        # 构造 secret 的分布列
        s1 = [np.concatenate((build_ternary_array(n,num1,num1), np.array([0 for i in range(n)]))) for i in range(dim)]

        # 离散傅里叶变换，并取奇数项        
        y = [dft.fft(s1[i])[1::2] for i in range(len(s1))]

        # 计算每个维度的范数
        norm_y = [np.linalg.norm([abs(y[i][j]) for i in range(len(y))]) for j in range(len(y[0]))]

        # 排序
        sorted_y = sorted(norm_y, reverse = True)
        
        # 计算 gamma
        res.append(sqrt(tau**2*sum([x**2 for x in sorted_y[:i_max]])+(leftover*tau)*sorted_y[i_max]**2)/sqrt(n*tau))

    return([np.nanquantile(res, r/100) for r in rate])
