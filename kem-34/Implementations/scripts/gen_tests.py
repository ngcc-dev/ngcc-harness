import argparse
import pdb
import string

from sage.all import *
from sage.arith.misc import carmichael_lambda


def _negaconvolution_naive(L1, L2):
	"""
	Negacyclic convolution of L1 and L2, using naive algorithm.

	L1 and L2 must be the same length.

	EXAMPLES::

	    sage: from sage.rings.polynomial.convolution import _negaconvolution_naive
	    sage: from sage.rings.polynomial.convolution import _convolution_naive
	    sage: _negaconvolution_naive([2], [3])
	    [6]
	    sage: _convolution_naive([1, 2, 3], [3, 4, 5])
	    [3, 10, 22, 22, 15]
	    sage: _negaconvolution_naive([1, 2, 3], [3, 4, 5])
	    [-19, -5, 22]
	"""
	assert len(L1)
	assert len(L1) == len(L2)

	N = len(L1)
	return [
		sum([L1[i] * L2[j - i] for i in range(j + 1)])
		- sum([L1[i] * L2[N + j - i] for i in range(j + 1, N)])
		for j in range(N)
	]


def main():
	parser = argparse.ArgumentParser()

	parser.add_argument(
		"-n", help="Size of the polynomial, default 64", default=64, type=int
	)
	parser.add_argument(
		"-q", help="Size of the ring, default 3329", default=3329, type=int
	)

	args = parser.parse_args()

	# Ring
	R = IntegerModRing(args.q)

	# gen poly
	p, q = ([], [])
	for i in range(args.n):
		p.append(R.random_element())
		q.append(R.random_element())
	# p = vector(p)
	# q = vector(q)

	# Get multiplication
	res = _negaconvolution_naive(p, q)

	# Output
	print(f"p: {p}\nq: {q}\nres: {res}")


if __name__ == "__main__":
	main()
