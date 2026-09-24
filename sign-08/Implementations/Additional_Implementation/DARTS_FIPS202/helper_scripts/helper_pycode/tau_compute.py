# 输入：整数 n 和目标熵 target_entropy
# 输出：满足 log2(C(n, tau)) >= target_entropy 的最小整数 tau
# C(n, tau) 为组合数，即从 n 个元素中选择 tau 个元素的方式数

import math

def tau_computer(n, target_entropy):
    tau = 0
    while(math.log(math.comb(n, tau), 2)) < target_entropy and tau < n:
        tau += 1
    if tau == n:
        print("Warning, overflow!")
    return tau