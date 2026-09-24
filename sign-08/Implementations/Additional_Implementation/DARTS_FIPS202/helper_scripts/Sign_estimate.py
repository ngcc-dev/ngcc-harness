from security_estimate.LWE_estimate import *
from security_estimate.SIS_estimate import *
from helper_pycode.repetitions_compute import from_B11_to_BSIS, from_B11_to_B1, from_B11_to_B0
from helper_pycode.entropy_coordinate_refined_hyperball import *

class SignParameters:
    def __init__(self, n, k, l, q, p, tau, d, B11, t):
        self.n = n      # 多项式维数
        self.k = k      # 行数
        self.l = l      # 列数
        self.q = q      # 模数
        self.p = p      # 三元分布取 1 (-1) 的概率
        self.tau = tau  # 挑战 c 中非零系数的数量
        self.d = d      # 压缩位数
        self.B11 = B11  # 重复采样参数 B''
        self.t = t      # sc 的上界 t = gamma * sqrt(tau)


def Sign_to_MLWE(ps):
    """
    将签名参数转换为 MLWE 参数集
    输入:
        ps: 签名参数
    输出:
        n: 环维数
        d: MLWE 的维数
        m: MLWE 的样本数
    """
    d = ps.l             # MLWE 维数
    m = ps.k             # MLWE 样本数
    return MLWEParameterSet(ps.n, d, m, ps.q, ps.p)


def Sign_to_MSIS(ps):
    """
    将签名将签名参数转换为 MSIS 参数集
    输入:
        ps: 签名参数
    输出:
        n: 环维数
        h: MSIS 的行数，即方程数
        w: MSIS 的列数，即变量数
        B: MSIS 的范数上界（采用2范数估计安全性）
    """
    B = from_B11_to_BSIS(ps.B11, ps.n, ps.k, ps.l, ps.q, ps.d)
    h = ps.k
    w = ps.k + ps.l
    return MSISParameterSet(ps.n, w, h, B, ps.q, method="refined_l2")


def compute_sign_size(ps):
    '''
    计算签名大小（字节），估计 rANS 编码后的签名尺寸，会比实际的签名尺寸小
    输入:
        ps: 签名参数
    输出:
        sign_size_byte: 签名尺寸的估计值
    '''
    c_size = ps.n   # 原始的 c 
    m = ps.k + ps.l # 环的维数
    
    B1 = from_B11_to_B1(ps.B11, ps.n, ps.k, ps.l, ps.q, ps.d)
    B0 = from_B11_to_B0(ps.B11, ps.n, ps.k, ps.l, ps.q, ps.d, ps.t)
    
    # 计算 h 的尺寸
    (high_prob, _) = refined_compute_f_table(ps.n, m, B1, B0, ps.d, ps.q)
    entropy = sum([-(p + 1e-16) * log(p + 1e-16, 2) for p in high_prob])
    h_size = entropy * ps.n * ps.k
    
    # 计算 z1 尺寸
    # 由于对 z1 压缩后分别取高位和低位，我们对高位 rANS 编码，并且低位同均匀分布统计距离较小
    # 因此在估计尺寸时，高位我们使用 rANS 编码，低位直接使用定长编码
    hb_z1_size = entropy * ps.n * ps.l
    lb_z1_size = ps.n * ps.l * ceil(log(2*round(ps.q / (2 ** (ps.d+1)))+1, 2))  # 定长编码
    z1_size = hb_z1_size + lb_z1_size
    
    # 计算签名尺寸
    sign_size_bit = z1_size + c_size + h_size
    sign_size_byte = sign_size_bit/8 + 2
    return sign_size_byte
    

def compute_pk_size(ps):
    """
    计算公钥大小（字节）
    公钥包括 seed 和矩阵 A_0， 其中seed 大小为32字节，矩阵 A 为 k*n*ceil(log(q))/8 字节
    输入:
        ps: 签名参数
        
    输出:
        公钥大小（字节）
    """
    seed_size = 32                                       # 字节
    A_size = ps.k * ps.n * ( (ps.q).bit_length() ) / 8   # 字节
    return seed_size + A_size


def summarize_sign(ps):
    print("Sign parameters:", ps.__dict__)
    print("Bound for SIS:", from_B11_to_BSIS(ps.B11, ps.n, ps.k, ps.l, ps.q, ps.d))
    print("Public key size (bytes): ", int(compute_pk_size(ps)))   # 返回整数值
    print("Signature size (bytes):  ", int(compute_sign_size(ps))) # 返回整数值
    print("Sum size (bytes):        ", int(compute_pk_size(ps) + compute_sign_size(ps)))
    print("MLWE security:")
    MLWE_summarize_attacks(Sign_to_MLWE(ps))
    print("MSIS security:")
    MSIS_summarize_attacks(Sign_to_MSIS(ps))
