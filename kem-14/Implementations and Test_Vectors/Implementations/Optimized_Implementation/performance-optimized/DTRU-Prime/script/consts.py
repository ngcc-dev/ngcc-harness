import random
import math
from typing import List, Optional


def get_prime_factors(n: int) -> List[int]:
    """Helper function to find all unique prime factors of a number."""
    factors = set()
    d = 2
    temp = n
    while d * d <= temp:
        if temp % d == 0:
            factors.add(d)
            while temp % d == 0:
                temp //= d
        d += 1
    if temp > 1:
        factors.add(temp)
    return list(factors)


def is_prime(n: int, k: int = 10) -> bool:
    """
    Check if a number is likely prime using the Miller-Rabin primality test.
    """
    if n <= 1:
        return False
    if n <= 3:
        return True
    if n % 2 == 0:
        return False

    r, d = 0, n - 1
    while d % 2 == 0:
        r += 1
        d //= 2

    for _ in range(k):
        a = random.randrange(2, n - 1)
        x = pow(a, d, n)
        if x == 1 or x == n - 1:
            continue
        for _ in range(r - 1):
            x = pow(x, 2, n)
            if x == n - 1:
                break
        else:
            return False
    return True


def find_smallest_primitive_kth_root(k: int, q: int) -> Optional[int]:
    """
    Finds the smallest positive integer x that is a primitive k-th root of unity modulo q.
    """
    if (q - 1) % k != 0:
        raise ValueError(f"Order {k} does not divide q-1 ({q-1}).")

    k_factors = get_prime_factors(k)

    for x in range(2, q):
        if pow(x, k, q) != 1:
            continue

        is_primitive = True
        for factor in k_factors:
            if pow(x, k // factor, q) == 1:
                is_primitive = False
                break

        if is_primitive:
            return x

    return None


def bit_reverse(n: int, num_bits: int) -> int:
    """Reverses the bits of a number."""
    rev = 0
    for i in range(num_bits):
        if (n >> i) & 1:
            rev |= 1 << (num_bits - 1 - i)
    return rev


def generate_ntt_tree(n: int, pr: int, q: int) -> List[int]:
    """
    Generates the pre-ordered twiddle factors for a standard Cooley-Tukey NTT.
    """
    tree = []
    num_bits = (n // 2).bit_length() - 1

    for i in range(n // 2):
        rev_i = bit_reverse(i, num_bits)
        root = pow(pr, rev_i, q)
        tree.append(root)

    return tree


def precompute_montgomery_factors(tree: List[int], beta: int, q: int) -> List[int]:
    """
    Applies Montgomery pre-computation to a list of NTT twiddle factors.
    Calculates (zeta * 2^beta) mod q for each zeta in the tree and centers the result.
    """
    mont = pow(2, beta, q)
    print(f"Montgomery Constant (MONT = 2^{beta} mod {q}): {mont}")

    precomputed_tree = []
    for zeta in tree:
        mont_zeta = (zeta * mont) % q
        if mont_zeta > q // 2:
            mont_zeta -= q
        precomputed_tree.append(mont_zeta)

    return precomputed_tree

def precompute_final_factors(mont_tree: List[int], beta: int, q: int) -> List[int]:
    """
    Applies the final QINV multiplication for optimized Montgomery reduction.
    Calculates (mont_zeta * QINV) mod 2^beta where QINV = q^-1 mod 2^beta.
    """
    mod_beta = 1 << beta
    qinv = pow(q, -1, mod_beta)
    print(f"Inverse Modulo (QINV = {q}^-1 mod {mod_beta}): {qinv}")

    final_tree = []
    for mont_zeta in mont_tree:
        centered_zeta = mont_zeta if mont_zeta >= 0 else mont_zeta + mod_beta
        final_val = (centered_zeta * qinv) % mod_beta
        
        if final_val >= mod_beta // 2:
            final_val -= mod_beta
        final_tree.append(final_val)

    return final_tree


def generate_invntt_tree(n: int, pr: int, q: int) -> List[int]:
    inv_pr = pow(pr, -1, q)
    m = n // 2
    l = int((m).bit_length() - 1)
    out: List[int] = []
    for k in range(l, 0, -1):
        for j in range(1 << (k - 1)):
            idx = (m + 2 * m * bit_reverse(j, k - 1)) // (1 << k)
            out.append(pow(inv_pr, idx, q))
    return out

def compute_barrett_shift(q: int, beta: int) -> int:
    return math.floor(math.log2(q) - 1) + beta

def compute_barret_v(q: int, shift: int) -> int:
    return ((1 << shift) + q // 2) // q

def compute_f_avx(beta: int, n: int, ntt_layer_interval: int, q: int) -> int:
    mont = pow(2, beta, q)
    denom = n // ntt_layer_interval
    inv_d = pow(denom, -1, q)
    s = (mont * mont * inv_d) % q
    return s - q if s > q // 2 else s

def main() -> None:
    """Main execution function to generate and print NTT parameters."""
    q = 33550337
    n = 1536
    root_n = 512 # root_unity^root_n mod q = 1
    beta = 32
    ntt_layer_interval = 3

    print("--- NTT Parameter Generation ---")
    print(f"Modulus q = {q}")
    print(f"Transform Length n = {n}")
    print(f"Montgomery Beta = {beta}\n")

    barrett_shift = compute_barrett_shift(q, beta)
    barrett_shift_avx = barrett_shift - beta
    barret_v = compute_barret_v(q, barrett_shift)
    f_avx = compute_f_avx(beta, n, ntt_layer_interval, q)

    print("--- Additional Constants ---")
    print(f"#define Barrett_SHIFT {barrett_shift} // floor(log_2(Q))+16")
    print(f"#define Barrett_SHIFT_AVX {barrett_shift_avx} // floor(log_2(Q))")
    print(f"#define Barret_V {barret_v} // v = ((1<<Barrett_SHIFT) + Q/2) / Q")
    print(f"#define F_AVX {f_avx} // f = mont^2/(2048/NTT_LAYER_INTERVAL) mod q\n")

    print("start generate ntt tree")
    print(f"Step 1: Finding smallest primitive {root_n}-th root of unity...")
    try:
        smallest_root = find_smallest_primitive_kth_root(root_n, q)
        if not smallest_root:
            print(f"Error: Could not find a primitive {root_n}-th root of unity.")
            return
        print(f"Success: Found smallest primitive {root_n}-th root: {smallest_root}\n")

        print(f"Step 2: Generating raw NTT tree...")
        ntt_tree = generate_ntt_tree(root_n, smallest_root, q)
        print(f"Success: Generated {len(ntt_tree)} raw twiddle factors.\n")

        print(f"Step 3: Pre-computing Montgomery factors...")
        montgomery_tree = precompute_montgomery_factors(ntt_tree, beta, q)
        print(f"\nGenerated {len(montgomery_tree)} Montgomery-precomputed factors:")
        for i, root in enumerate(montgomery_tree):
            print(f"{root:5d},", end="")
            if (i + 1) % 16 == 0:
                print("")
        print("\n")

        print(f"Step 4: Pre-computing final QINV factors...")
        final_tree = precompute_final_factors(montgomery_tree, beta, q)
        print(f"\nFinal Result: {len(final_tree)} factors for C code (zeta * MONT * QINV):")
        for i, root in enumerate(final_tree):
            print(f"{root:6d},", end="")
            if (i + 1) % 16 == 0:
                print("")
        print("\n")

        print("--- Inverse NTT Parameters ---")
        inv_tree = generate_invntt_tree(root_n, smallest_root, q)
        inv_mont = precompute_montgomery_factors(inv_tree, beta, q)
        print(f"Generated {len(inv_mont)} inverse twiddle factors (zeta^-1 * MONT mod q) in zeta_table order:")
        for i, root in enumerate(inv_mont):
            print(f"{root:6d},", end="")
            if (i + 1) % 16 == 0:
                print("")
        print("\n")
        inv_final = precompute_final_factors(inv_mont, beta, q)
        print(f"Generated {len(inv_final)} inverse twiddle factors (zeta^-1 * MONT * QINV):")
        for i, root in enumerate(inv_final):
            print(f"{root:6d},", end="")
            if (i + 1) % 16 == 0:
                print("")

    except ValueError as e:
        print(f"Error: {e}")


if __name__ == "__main__":
    try:
        main()
    except BrokenPipeError:
        pass
