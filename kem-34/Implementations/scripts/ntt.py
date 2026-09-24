import argparse
import pdb
import string

from sage.all import *
from sage.arith.misc import carmichael_lambda


def convolve(a, b, n, R):
	res = vector([0] * (n * 2 - 1), R)
	for i in range(n):
		# perform a_i * b
		c_i_1 = R(0)
		c_i_2 = R(0)
		for j in range(n):
			c_i_1 += a[i] * b[j]
			c_i_2 += a[i] * b[j]
	return res


def basic_negacyclic_convolution(a, b, n, q, R):
	res = vector([0] * n, R)
	print(res)
	for k in range(n):
		c_k_1, c_k_2 = (R(0), R(0))
		for i in range(n):
			if i <= k:
				c_k_1 = c_k_1 + a[i] * b[k - i]
			else:
				c_k_2 = c_k_2 + a[i] * b[k + n - i]
		print(f"ck1: {c_k_1}, ck2: {c_k_2}")
		res[k] = c_k_1 - c_k_2
	return res


def bit_reverse(poly):
	n_bit = 7
	brv = []
	for i in range(2**n_bit):
		brv.append(Integer(bin(i + 2**n_bit)[:1:-1], 2) // 2)
		print(brv[i], end=", ")
	print("\n\n")
	new_poly = []
	for new_i in brv:
		new_poly.append(poly[new_i])
	return new_poly


"""Low-Complexity NTT Algorithm without Pre-Processing

This is a sagemath implementation of the algorithm described in the paper:
https://tches.iacr.org/index.php/TCHES/article/view/8544/8109.

Args:
	A: A polynomial with coefficients in Z/qZ
	gammas: A vector of 2n-th roots of unity

Returns:
	A: Polynomial A converted in place into the NTT domain
"""


def ntt_nopreproc(A, gamma, n, q):
	# breakpoint()
	m = 2
	p = 0
	while m <= n:
		# print(f"m: {m}")
		for j in range(int(m / 2)):
			# gamma_index = int((2 * j + 1) * n / m)
			omega = Mod(gamma ** ((2 * j + 1) * n / m), q)
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


def invntt_nopreproc(A, gamma, n, q):
	# breakpoint()
	m = n
	p = 0
	while m >= 2:
		# print(f"m: {m}")
		for j in range(int(m / 2)):
			# gamma_index = int((2 * j + 1) * n / m)
			omega = Mod(gamma ** ((2 * j + 1) * n / m), q)
			p += 1
			for k in range(int(n / m)):
				u = A[k * m + j]
				t = A[int(k * m + j + m / 2)]  # mod q
				A[k * m + j] = (u + t) / 2  # mod q
				A[int(k * m + j + m / 2)] = omega * (u - t)  # / 2 # mod q
		m = int(m / 2)
	return A


def ntt(poly, zeta, n, q):
	ntt_poly = []
	for j in range(n):
		s = 0
		for i in range(n):
			s += Mod(zeta ** (2 * i * j + i), q) * poly[i]
		ntt_poly.append(s)
	return ntt_poly


def invntt(ntt_poly, zeta, n, q):
	poly = []
	inv_n = inverse_mod(n, q)
	inv_zeta = inverse_mod(zeta, q)
	for j in range(n):
		s = 0
		for i in range(n):
			mat_entry = Mod(inv_zeta ** (2 * i * j + j), q)
			s = Mod(s + mat_entry * ntt_poly[i], q)
		poly.append(Mod(inv_n * s, q))
	return poly


def main():
	parser = argparse.ArgumentParser()

	parser.add_argument(
		"-q", help="Modulo of the ring, default is 3329", default=3329, type=int
	)
	parser.add_argument(
		"-n", help="Length of polynomial, default is 64", default=64, type=int
	)

	args = parser.parse_args()

	R = IntegerModRing(args.q)

	poly_a = vector(
		[
			2856,
			2554,
			313,
			2720,
			569,
			1390,
			1789,
			2415,
			1083,
			1287,
			2922,
			2636,
			655,
			176,
			2311,
			2656,
			957,
			469,
			2399,
			2858,
			968,
			658,
			369,
			190,
			2834,
			1853,
			843,
			1928,
			8,
			1237,
			366,
			872,
			2016,
			941,
			846,
			233,
			3155,
			320,
			3174,
			2482,
			0,
			2798,
			2560,
			2844,
			1920,
			713,
			1871,
			1723,
			1167,
			3095,
			1800,
			957,
			1020,
			562,
			1512,
			818,
			1116,
			772,
			1802,
			64,
			898,
			2471,
			1288,
			3270,
		],
		R,
	)
	poly_b = vector(
		[
			2841,
			1141,
			3206,
			2911,
			954,
			1638,
			147,
			864,
			1469,
			2716,
			1639,
			2449,
			918,
			648,
			2072,
			38,
			2159,
			1504,
			1455,
			1646,
			2406,
			2352,
			949,
			834,
			372,
			524,
			942,
			2010,
			2444,
			541,
			1002,
			1936,
			1235,
			2209,
			3181,
			141,
			578,
			1938,
			334,
			1941,
			2277,
			2806,
			598,
			1239,
			1117,
			1839,
			2582,
			1131,
			194,
			536,
			2708,
			1742,
			2594,
			2479,
			421,
			1609,
			404,
			1434,
			2816,
			17,
			3309,
			1551,
			1519,
			1050,
		],
		R,
	)
	res = basic_negacyclic_convolution(poly_a, poly_b, args.n, args.q, R)

	print(f"Basic negacyclic convolution: {res}")

	print("Calculating 2n-th primitive root")
	# 2n-th root
	zeta = 0
	zetas = []
	for i in R:
		if i.additive_order() == args.q:
			if i.multiplicative_order() == (2 * args.n):
				zetas.append(Mod(i, args.q))

	print(f"Found zetas {zetas}")
	zeta = zetas[int(input("Choose a zeta:"))]

	ntt_poly = vector(ntt(poly_a, zeta, args.n, args.q), R)
	rev_poly = vector(invntt(ntt_poly, zeta, args.n, args.q), R)
	if poly_a != rev_poly:
		print("polys don't match")
		print(poly_a)
		print(ntt_poly)
		print(rev_poly)

	ntt_poly_a = vector(ntt(poly_a, zeta, args.n, args.q), R)
	print(f"ntt poly a: {ntt_poly_a}")
	print(f"bit reverse ntt poly a: {bit_reverse(ntt_poly_a)}")
	ntt_poly_b = vector(ntt(poly_b, zeta, args.n, args.q), R)

	print(f"ntt poly b: {ntt_poly_b}")

	# print(f"ntt nopreproc poly a: {ntt_nopreproc(poly_a, zeta, args.n, args.q)}")
	# ntt_poly_a = vector(ntt_nopreproc(poly_a, zeta, args.n, args.q), R)
	# ntt_poly_b = vector(ntt(poly_b, zeta, args.n, args.q), R)

	# Test multiplication


if __name__ == "__main__":
	main()
