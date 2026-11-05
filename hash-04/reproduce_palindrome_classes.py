#!/usr/bin/env python3
"""Check Slaughter's short commuting-palindrome collision construction.

This checks exact identities and reduced-prime collisions. It does not attempt
the square-root search at either submitted CHAMP prime.
"""

from fractions import Fraction
from random import Random

A = (-2, 1, 4, -3)
B = (-5, 2, -6, 2)
B_ALT = (6, 4, -2, -1)
S = (22, -14, -14, 9)
S_ALT = (10, 6, 6, 1)
PRIMES = (2**128 - 15449, 2**256 - 36113)


def mul(x, y, prime=None):
    z = (
        x[0] * y[0] + x[1] * y[2],
        x[0] * y[1] + x[1] * y[3],
        x[2] * y[0] + x[3] * y[2],
        x[2] * y[1] + x[3] * y[3],
    )
    return tuple(v % prime for v in z) if prime else z


def product(word, generators, prime):
    x = (1, 0, 0, 1)
    for bit in word:
        x = mul(x, generators[int(bit)], prime)
    return x


def class_key(x, prime):
    # For the two CHAMP forms s22 != 0, so these are projective coordinates
    # for the two-dimensional traceless subspace of the palindrome products.
    a, b = (x[0] - x[3]) % prime, x[1] % prime
    if b == 0:
        return ("scalar" if a == 0 else "infinity", 0)
    return ("finite", a * pow(b, -1, prime) % prime)


def check_small_search(generators, form, label):
    prime = 65537
    half_length = (prime + 2).bit_length()  # ceil(log2(p+3))
    rng = Random(2713)
    seen = {}
    used_words = set()
    for count in range(1, (1 << half_length) + 1):
        u = format(rng.randrange(1 << half_length), f"0{half_length}b")
        if u in used_words:
            continue
        used_words.add(u)
        palindrome = u + u[::-1]
        x = product(palindrome, generators, prime)
        sx = mul(form, x, prime)
        assert sx[1] == sx[2], "palindrome left the symmetric subspace"
        key = class_key(x, prime)
        if key in seen:
            previous, y = seen[key]
            if previous == palindrome:
                continue
            assert mul(x, y, prime) == mul(y, x, prime)
            left, right = palindrome + previous, previous + palindrome
            assert left != right and len(left) == len(right) == 4 * half_length
            assert product(left, generators, prime) == product(right, generators, prime)
            flipped = ("1" if left[0] == "0" else "0") + left[1:]
            assert product(flipped, generators, prime) != product(right, generators, prime)
            print(f"{label}: {len(left)}-bit toy collision at p={prime}, "
                  f"{len(used_words)} class evaluations")
            return
        seen[key] = (palindrome, x)
    raise AssertionError("no class collision found")


def check_short_recovery():
    # The paper's separate short-input observation applies to the main pair.
    rng = Random(2713)
    for prime, length in zip(PRIMES, (42, 84)):
        assert 8**length < prime // 2
        for _ in range(10):
            word = "".join(str(rng.randrange(2)) for _ in range(length))
            x = (1, 0, 0, 1)
            for bit in word:
                x = mul(x, (A, B)[int(bit)])
            assert all(2 * abs(v) < prime for v in x)
            # A digest can be inverted entrywise to obtain x mod p. Its
            # centered representatives then equal the integer product x.
            lifted = tuple((v % prime + prime // 2) % prime - prime // 2 for v in x)
            assert lifted == x
            recovered = []
            while lifted != (1, 0, 0, 1):
                slope = Fraction(lifted[2], lifted[0])
                bit = "0" if slope <= 1 else "1"
                recovered.append(bit)
                g = (A, B)[int(bit)]
                adj = (g[3], -g[1], -g[2], g[0])
                q = mul(adj, lifted)
                assert all(v % 2 == 0 for v in q)
                lifted = tuple(v // 2 for v in q)
            assert "".join(recovered) == word
        print(f"short recovery: 10 random {length}-bit messages at CHAMP prime")


def main():
    for generators, form, label in (((A, B), S, "main pair"),
                                    ((A, B_ALT), S_ALT, "alternative pair")):
        assert form[0] * form[3] - form[1] * form[2] in (2, -26)
        for g in generators:
            sg = mul(form, g)
            assert sg[1] == sg[2]
        for prime in PRIMES:
            assert (form[0] * form[3] - form[1] * form[2]) % prime != 0
            nclasses = prime + 2
            m = (nclasses).bit_length()
            m_plus = (2 * nclasses - 1).bit_length()
            assert 2**m >= nclasses + 1
            assert 2**m_plus >= 2 * nclasses and m_plus <= m + 1
            print(f"{label}: p has {prime.bit_length()} bits; heuristic {4*m}-bit, "
                  f"distribution-free {4*m_plus}-bit collisions")
        check_small_search(generators, form, label)

    prime = 54214950071
    a = "010110111001001110001101100110100101"
    b = "011101110001101111011101110110100000"
    p1, p2 = a + a[::-1], b + b[::-1]
    left, right = p1 + p2, p2 + p1
    assert left != right and len(left) == len(right) == 144
    x = product(left, (A, B), prime)
    assert x == product(right, (A, B), prime)
    assert x == (37275322184, 41540854233, 46507229653, 22313848993)
    flipped = ("1" if left[0] == "0" else "0") + left[1:]
    assert product(flipped, (A, B), prime) != x
    print("published 144-bit toy collision: verified with changed-bit control")
    check_short_recovery()
    print("CONFIRMED palindrome-class algebra and reduced-size witnesses")


if __name__ == "__main__":
    main()
