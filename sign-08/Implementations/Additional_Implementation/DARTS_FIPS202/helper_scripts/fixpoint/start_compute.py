from decimal import Decimal, getcontext, ROUND_HALF_UP
from math import sqrt

# start_cube = hex(round(2^76/(sqrt((K + L)*N + 2)^3)))
# start_times_threehalves = hex(round(2^76 * 3/(2 * sqrt((K + L)*N + 2))))

# 参数

K128 = 1
L128 = 2

K256 = 2
L256 = 3

K512 = 2
L512 = 3

N128 = 512
N256 = 512
N512 = 1024

def compute_constants(K, L, N, label):
    getcontext().prec = 200
    M = (K + L) * N + 2
    M_dec = Decimal(M)
    M_pow_3_2 = M_dec * M_dec.sqrt()

    val1 = (Decimal(2) ** 76) / M_pow_3_2
    rounded1 = int(val1.to_integral_value(rounding=ROUND_HALF_UP))

    val2 = (Decimal(2) ** 76 * 3) / (Decimal(2) * M_dec.sqrt())
    rounded2 = int(val2.to_integral_value(rounding=ROUND_HALF_UP))

    mask48 = (1 << 48) - 1
    def limbs(x):
        low = x & mask48
        high = x >> 48
        return low, high

    low1, high1 = limbs(rounded1)
    low2, high2 = limbs(rounded2)

    print(f"{label} parameters:")
    print("\n(high-precision Decimal):")
    print(" start_cube hex = 0x{:x}".format(rounded1))
    print("  limbs: low=0x{:x}, high=0x{:x}".format(low1, high1))
    print(" start_times_threehalves hex = 0x{:x}".format(rounded2))
    print("  limbs: low=0x{:x}, high=0x{:x}".format(low2, high2))
    
compute_constants(K128, L128, N128, "128-bit")
print("\n" + "="*40 + "\n")
compute_constants(K256, L256, N256, "256-bit")
print("\n" + "="*40 + "\n")
compute_constants(K512, L512, N512, "512-bit")

"""
128-bit parameters:

(high-precision Decimal):
 start_cube hex = 0x1162770077e2e41a
  limbs: low=0x770077e2e41a, high=0x1162
 start_times_threehalves hex = 0x9caa56693861ad937b
  limbs: low=0x693861ad937b, high=0x9caa56

========================================

256-bit parameters:

(high-precision Decimal):
 start_cube hex = 0x8160107f5256727
  limbs: low=0x107f5256727, high=0x816
 start_times_threehalves hex = 0x7962517a75107b7db5
  limbs: low=0x7a75107b7db5, high=0x796251

========================================

512-bit parameters:

(high-precision Decimal):
 start_cube hex = 0x2dc491fa9186aab
  limbs: low=0x491fa9186aab, high=0x2dc
 start_times_threehalves hex = 0x55d926912fd7c9421f
  limbs: low=0x912fd7c9421f, high=0x55d926
"""