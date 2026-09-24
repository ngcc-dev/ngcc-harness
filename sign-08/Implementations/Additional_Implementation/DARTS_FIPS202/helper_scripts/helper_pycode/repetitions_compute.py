from math import sqrt, log, exp


def from_B11_to_BSIS(B11, n, k, l, q, d):
    """
    从签名参数中的B''计算SIS问题中的B_SIS
    输入:
        B11: 签名参数中的B''
        n: 多项式的次数
        k: 行数
        l: 列数
        q: 模数
        d: 压缩位数
    输出:
        B_SIS
    """
    B_SIS = B11 + sqrt(n * k)*(q/(2**d) + 2)
    return B_SIS

def from_B11_to_B1(B11, n, k, l, q, d):
    """
    从签名参数中的B''计算中间参数B1
    输入:
        B11: 签名参数中的B''
        n: 多项式的次数
        k: 行数
        l: 列数
        q: 模数
        d: 压缩位数
    输出:
        B1
    """
    m = k + l  # SIS问题的列数
    B1 = B11 - sqrt(n*m/2) - sqrt(n*k) * (q//(2**(d+1)))
    return B1

def from_B11_to_B0(B11, n, k, l, q, d, t):
    """
    B0 = B - t
    从签名参数中的B''计算中间参数B0
    输入:
        B11: 签名参数中的B''
        n: 多项式的次数
        k: 行数
        l: 列数
        q: 模数
        d: 压缩位数
        t: Sc的界
    输出:
        B0: 双峰拒绝采样时中间小球的半径
    """
    B1 = from_B11_to_B1(B11, n, k, l, q, d)
    B  = sqrt(B1**2 + t**2)
    B0 = B - t
    return B0

def from_B11_to_B(B11, n, k, l, q, d, t):
    """
    从签名参数中的B''计算中间参数B
    输入:
        B11: 签名参数中的B''
        n: 多项式的次数
        k: 行数
        l: 列数
        q: 模数
        d: 压缩位数
        t: Sc的界
    输出:
        B
    """
    B1 = from_B11_to_B1(B11, n, k, l, q, d)
    B  = sqrt(B1**2 + t**2)
    return B


def M_reject_compute(B1, B0, B, m, n):
    x = m*n  

    u = x * log(B)
    v = x * log(B1)
    w = x * log(B0)

    M = max(v, w)

    num = 2 * exp(u - M)
    den = exp(v - M) + exp(w - M)

    return num / den

def from_B11_to_M(B11, n, k, l, q, d, tau, t):
    """
    从签名参数中的B''计算总的重复采样次数M
    输入:
        B11: 签名参数中的B''
        n: 多项式的次数
        k: 行数
        l: 列数
        q: 模数
        d: 压缩位数
        tau: c的汉明重量
        t: Sc的界
    输出:
        M
    """
    m = k + l  # SIS问题的列数
    B1 = from_B11_to_B1(B11, n, k, l, q, d)
    B  = sqrt(B1**2 + t**2)
    B0 = min(B1, B - t)

    M_reject = M_reject_compute(B1, B0, B, m, n)
    M_compress = 1/(0.5 + 0.5 * ((q - 2**d)/q)**tau)

    return M_reject, M_compress, M_reject * M_compress


def repetitions_compute(B11, n, k, l, q, d, tau, t, verbose=False):
    """
    计算重复采样的参数，一次性返回所有相关参数
    输入:
        B11: 设定的B''参数
        n: 多项式的次数
        k: 行数
        l: 列数
        q: 模数
        d: 压缩位数
        tau: c的汉明重量
        t: Sc的界
        verbose: 是否打印中间计算结果
    输出:
        (B0, B, B1, B11, B_SIS, M_reject, M_compress, M)
    """

    # 计算参数 m, 即SIS问题的列数
    m = k + l 
    B_SIS = B11 + sqrt(n * k)*(q/(2**d) + 2)
    B1 = B11 - sqrt(n*m/2) - sqrt(n*k) * (q//(2**(d+1)))
    B = sqrt(B1**2 + t**2)
    B0 = min(B1, B-t)

    M_reject = M_reject_compute(B1, B0, B, m, n)
    M_compress = 1/(0.5 + 0.5 * ((q - 2**d)/q)**tau)

    # 总的重复采样次数
    M = M_reject * M_compress
    
    if verbose:
        print("B_SIS: {B_SIS}, B11, {B11}, B1: {B1}, B0: {B0}, B: {B}".format(B_SIS=B_SIS, B11=B11, B1=B1, B0=B0, B=B))
        print("M_reject: {M_reject}, M_compress: {M_compress}, M: {M}".format(M_reject=M_reject, M_compress=M_compress, M=M))
    
    return (B0, B, B1, B11, B_SIS, M_reject, M_compress, M)