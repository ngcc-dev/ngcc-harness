import argparse
import math


def approximation(x, t, p, factor):
	u = x % p
	print(f"u: {u}")
	d0 = u * (2**t)
	print(f"d0: {d0}")
	d0 += round(p / 2.0)
	print(f"d0: {d0}")
	d0 *= round((2**factor) / p)
	print(f"d0: {d0}")
	d0 /= 2**factor
	print(f"d0: {d0}")
	return math.floor(d0)


def ground_truth(x, t, p):
	return round(((2**t) / p) * x)


def check_bitlength(t, p, factor):
	for i in range(p):
		approx = approximation(float(i), float(t), float(p), factor)
		truth = ground_truth(float(i), t, p)
		if approx != truth:
			print(
				f"Factor {factor} failed for {i}, got approx {approx}, expected {truth}"
			)
			return False
	return True


def main():
	parser = argparse.ArgumentParser()

	parser.add_argument("-t", "--log2t", type=int)
	parser.add_argument("-p", "--prime", type=int)
	parser.add_argument("-c", "--check-factor", type=int, default=-1)
	parser.add_argument("--compress", type=int, default=-1)

	args = parser.parse_args()

	if args.check_factor > 0:
		if args.compress > 0:
			print(
				f"Approximation of {args.compress}: {approximation(args.compress, args.log2t, args.prime, args.check_factor)}, truth: {ground_truth(args.compress, args.log2t, args.prime)}"
			)
		else:
			if check_bitlength(args.log2t, args.prime, float(args.check_factor)):
				print(f"{args.check_factor} works!")
				exit(0)
	else:
		for i in range(64):
			print(f"Checking {i}...")
			if check_bitlength(args.log2t, args.prime, float(i)):
				print(f"{i} works!")
				exit(0)

	print(f"Could not find a suitable bit length :(")
	exit(-1)


if __name__ == "__main__":
	main()
