import argparse
import pdb
import string

from sage.all import *
from sage.arith.misc import carmichael_lambda


def print_c_array(arr):
	str_arr = str(arr)
	str_arr = str_arr.replace("[", "{")
	return str_arr.replace("]", "}")


def redc(a, q, q_inv):
	u = a * q_inv
	u = (a + u * q) >> 16
	return u


def bitrev_order(n):
	bitrev = [0] * n
	bitrev_size = len("{:b}".format(n))
	fmt_str = "{:0" + str(bitrev_size - 1) + "b}"
	print(fmt_str)
	for i in range(n):
		bitstring = fmt_str.format(i)
		revbitstr = "".join(reversed(bitstring))
		bitrev[i] = int(revbitstr, 2)
	return bitrev


"""Low-Complexity NTT Algorithm without Pre-Processing

This is a sagemath implementation of the algorithm described in the paper:
https://tches.iacr.org/index.php/TCHES/article/view/8544/8109.

Args:
	A: A polynomial with coefficients in Z/qZ
	gammas: A vector of 2n-th roots of unity

Returns:
	A: Polynomial A converted in place into the NTT domain
"""


def ntt(A, gammas, n):
	# breakpoint()
	m = 2
	p = 0
	while m <= n:
		# print(f"m: {m}")
		for j in range(int(m / 2)):
			# gamma_index = int((2 * j + 1) * n / m)
			omega = gammas[p]
			p += 1
			for k in range(int(n / m)):
				u = A[k * m + j]
				t = omega * A[int(k * m + j + m / 2)]  # mod q
				A[k * m + j] = u + t  # mod q
				A[int(k * m + j + m / 2)] = u - t  # mod q
				if (k * m + j) == 1 or (k * m + j + m / 2) == 1:
					print(A[1])
					print("j: ", int(k * m + j + m / 2))
					print("A[j]: ", A[int(k * m + j + m / 2)])
					print("t: ", t)
					print("u: ", u)
		m *= 2
	return A


def invntt(A, inv_gammas, n):
	# breakpoint()
	m = n
	p = 0
	while m >= 2:
		# print(f"m: {m}")
		for j in range(int(m / 2)):
			# gamma_index = int((2 * j + 1) * n / m)
			omega = inv_gammas[p]
			p += 1
			for k in range(int(n / m)):
				u = A[k * m + j]
				t = A[int(k * m + j + m / 2)]  # mod q
				A[k * m + j] = (u + t) / 2  # mod q
				A[int(k * m + j + m / 2)] = omega * (u - t)  # / 2 # mod q
		m = int(m / 2)
	return A


def loop_order_rudraksh(n):
	print("rudraksh loop")
	k = 1
	l = int(n / 2)
	j = 0
	while l >= 1:
		st = 0
		# print(f"len: {l}")
		while st < n:
			# print(f"st: {st}, k: {k}")
			k += 1
			j = st
			while j < st + l:
				# print(f"j: {j}")
				print(f"Access polynomial at j+len: {j + l}")
				j += 1
			st = j + l
		l = l >> 1


def loop_order_orig(n):
	print("loop order orig")
	print("ntt")
	m = 2
	j = 0
	while m <= n:
		# print(f"m: {m}")
		for j in range(int(m / 2)):
			# print(f"zeta order (2j+1)N/m: {(2 * j + 1) * n / m}")
			for k in range(int(n / m)):
				print(f"Access polynomial at km+j+m/2: {k * m + j + m / 2}")
				print(f"A other order km+j: {k * m + j}")
		m *= 2
	print("invntt")
	m = n
	j = 0
	while m >= 2:
		# print(f"m: {m}")
		for j in range(int(m / 2)):
			# print(f"zeta order (2j+1)N/m: {(2 * j + 1) * n / m}")
			print("new zeta")
			for k in range(int(n / m)):
				print(f"Access polynomial at km+j+m/2: {k * m + j + m / 2}")
				print(f"A other order km+j: {k * m + j}")
		m /= 2


def loop_order_new(n):
	print("loop order new")
	k = 1
	start = 1
	m_2 = 1
	m = 2
	while m <= n:
		while start < m:
			print(f"zeta access start: {start}")
			for j in range(start, n, m):
				print(f"A order new j: {j}")
			start += 1
		m *= 2
		m_2 *= 2


def calculate_zetas(root_of_unity, polynomial_degree, ring):
	zetas = []
	newhope_zetas = []
	inv_zetas = []
	newhope_inv_zetas = []
	conversion_zetas = []
	conversion_inv_zetas = []
	m = 2
	while m <= polynomial_degree:
		for j in range(int(m / 2)):
			power = (2 * j + 1) * polynomial_degree / m
			print(f"power {power}")
			# For classic ntt
			zetas.append(root_of_unity**power)
			# For sped up montgomery from https://eprint.iacr.org/2015/1092.pdf
			newhope_zetas.append((2**16) * (root_of_unity**power))
			# For converting to montgomery as part of ntt
			conversion_zetas.append((2**32) * (root_of_unity**power))
		m *= 2
	m = int(m / 2)
	while m >= 2:
		for j in range(int(m / 2)):
			power = -int((2 * j + 1) * polynomial_degree / m)
			# For classic ntt
			inv_zetas.append(root_of_unity**power)
			# For sped up montgomery from https://eprint.iacr.org/2015/1092.pdf
			# 2^15 because we divide by 2 in invntt
			newhope_inv_zetas.append((2**15) * (root_of_unity**power))
			# For converting to montgomery as part of ntt
			# (2^16)^-1 / 2 mod q
			conversion_inv_zetas.append((1749) * (root_of_unity**power))
		m = int(m / 2)
	print(f"ZETAS length {len(zetas)}: {print_c_array(zetas)}")
	print(f"NEWHOPE ZETAS length {len(newhope_zetas)}: {print_c_array(newhope_zetas)}")
	print(
		f"CONVERSION ZETAS length {len(conversion_zetas)}: {print_c_array(conversion_zetas)}"
	)
	print(f"INV ZETAS length {len(inv_zetas)}: {print_c_array(inv_zetas)}")
	print(
		f"NEWHOPE INV ZETAS length {len(newhope_inv_zetas)}: {print_c_array(newhope_inv_zetas)}"
	)
	print(
		f"CONVERSION INV ZETAS length {len(conversion_inv_zetas)}: {print_c_array(conversion_inv_zetas)}"
	)
	return (
		zetas,
		newhope_zetas,
		conversion_zetas,
		inv_zetas,
		newhope_inv_zetas,
		conversion_inv_zetas,
	)


def calculate_param_dict(args, zeta, zetas, inv_zetas):
	params = {
		"KEM_L": args.l,
		"KEM_N": args.n,
		"KEM_Q": args.q,
		"LOG2Q": 1,
		"LOG2P": args.log2p,
		"LOG2T": args.log2t,
		"KEM_Q_2": floor(args.q / 2),
		"KEM_Q_DENOM": 2 ** (32 - args.log2t) // args.q,
		"LOG2T_BITMASK": "0x" + format((2**args.log2t) - 1, "x"),
		"LOG2P_BITMASK": "0x" + format((2**args.log2p) - 1, "x"),
		"KEM_Q_232": (2**32 // args.q) + 1,
		"KEM_Q_230": (2**30 // args.q) + 1,
		"KEM_Q_INV": -inverse_mod(args.q, 2**16) % 2**16,
		"RLOG": 16,
		"RMASK": "0x" + format((2**16) - 1, "x"),
		"INV_2_Q": (2**15) % args.q,
		"MONT": 2**16 % args.q,
		"MONT_2": 2**16 * 2**16 % args.q,
		"ROOT_OF_UNITY": zeta,
		"KEM_ETA": 2,
		"KEM_SYMBYTES": 16,
		"KEM_SSBYTES": 16,
		"ZETAS": f"{print_c_array(zetas)}",
		"INV_ZETAS": f"{print_c_array(inv_zetas)}",
	}

	# log2q
	while 2 ** params["LOG2Q"] < args.q:
		params["LOG2Q"] += 1

	return params


def main():
	parser = argparse.ArgumentParser()

	parser.add_argument(
		"-l", help="Number of polynomial vectors, default: 9", default=9, type=int
	)
	parser.add_argument(
		"-n", help="Polynomial degree, default: 64", default=64, type=int
	)
	parser.add_argument(
		"-q", help="The ring modulus, default: 3329", default=3329, type=int
	)
	parser.add_argument(
		"-z", help="Primitive 2nth root of unity, default: 33", default=33, type=int
	)
	parser.add_argument(
		"--log2p", help="Bit-size of p, default: 12", default=12, type=int
	)
	parser.add_argument(
		"--log2t", help="Bit-size of t, default: 6", default=6, type=int
	)

	args = parser.parse_args()

	R = IntegerModRing(args.q)

	zeta = 0
	for i in R:
		if i.additive_order() == args.q:
			if i.multiplicative_order() == (2 * args.n):
				zeta = Mod(i, args.q)
				break

	# Put root of unity in ring
	# zeta = R(args.z)

	# loop_order_rudraksh(args.n)
	# loop_order_orig(args.n)
	# loop_order_new(args.n)

	print(f"bit reversed order {bitrev_order(args.n)}")

	zetas, nh_zetas, conv_zetas, inv_zetas, inv_nh_zetas, conv_inv_zetas = (
		calculate_zetas(zeta, args.n, R)
	)

	param_dict = calculate_param_dict(args, zeta, nh_zetas, inv_nh_zetas)

	print(param_dict)
	params = None
	with open("param-template.h", "r") as f:
		params = string.Template(f.read())

	with open("params.h", "w") as f:
		f.write(params.substitute(param_dict))


if __name__ == "__main__":
	main()
