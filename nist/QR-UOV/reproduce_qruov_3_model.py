"""Public QR-UOV model used by reproduce_qruov_3.py.

This implements the public operations needed for the witness directly from the
On-Ramp Round 3 specification.  It is independently checked against the submitted
verifier by the reproducer's final open operation.
"""

import hashlib
import math


PARAMS = {
    "1q127L3": (1, 127, 3, 156, 54, 1, 1, 1),
    "1q127L10": (1, 127, 10, 540, 60, 2, 3, 1),
    "3q127L3": (3, 127, 3, 228, 78, 1, 1, 1),
    "3q127L10": (3, 127, 10, 820, 90, 2, 3, 1),
    "5q127L3": (5, 127, 3, 306, 105, 1, 1, 1),
    "5q127L10": (5, 127, 10, 1040, 110, 2, 3, 1),
    "1q31L3": (1, 31, 3, 165, 60, 1, 1, 1),
    "1q31L10": (1, 31, 10, 600, 70, 5, 3, 1),
    "1q7L10": (1, 7, 10, 740, 100, 2, 1, 1),
    "3q31L3": (3, 31, 3, 246, 87, 1, 1, 1),
    "3q31L10": (3, 31, 10, 890, 100, 5, 3, 1),
    "3q7L10": (3, 7, 10, 1100, 140, 2, 1, 1),
    "5q31L3": (5, 31, 3, 324, 114, 1, 1, 1),
    "5q31L10": (5, 31, 10, 1120, 120, 5, 3, 1),
    "5q7L10": (5, 7, 10, 1490, 190, 2, 1, 1),
}


def _failure_log(nprime, samples, q, bits):
    p = q / (1 << bits)
    terms, choose = [], 0.0
    for i in range(nprime):
        if i:
            choose += math.log(samples - i + 1) - math.log(i)
        terms.append(choose + i * math.log(p) + (samples - i) * math.log1p(-p))
    maximum = max(terms)
    return maximum + math.log(sum(math.exp(value - maximum) for value in terms))


def _tau(q, security, count):
    target = -security * math.log(2)
    low, high = count, count * 2 + 1000
    while _failure_log(count, high, q, q.bit_length()) > target:
        high *= 2
    while low < high:
        middle = (low + high) // 2
        if _failure_log(count, middle, q, q.bit_length()) <= target:
            high = middle
        else:
            low = middle + 1
    return low


class QRUOV:
    def __init__(self, name):
        self.cat, self.q, self.L, self.v, self.m, self.fc, self.fe, self.fc0 = PARAMS[name]
        self.name = name
        self.n = self.v + self.m
        self.V, self.M, self.N = self.v // self.L, self.m // self.L, self.n // self.L
        self.security = {1: 128, 3: 192, 5: 256}[self.cat]
        self.salt_bytes = self.security // 8
        self.qbits = self.q.bit_length()
        self.n1 = self.L * self.V * (self.V + 1) // 2
        self.n2 = self.L * self.V * self.M
        self.tau_n1 = _tau(self.q, self.security, self.n1)
        self.tau_n2 = _tau(self.q, self.security, self.n2)
        self.tau_m = _tau(self.q, self.security, self.m)
        self.phi_index = self.fe - 1
        self.p3_count = self.L * self.M * (self.M + 1) // 2
        self.pk_bytes = 16 + (self.qbits * self.m * self.p3_count + 7) // 8
        self.sig_bytes = self.salt_bytes + (self.qbits * self.n + 7) // 8

    def multiply(self, left, right):
        accum = [0] * (2 * self.L - 1)
        for i, a in enumerate(left):
            for j, b in enumerate(right):
                accum[i + j] += a * b
        for degree in range(2 * self.L - 2, self.L - 1, -1):
            value = accum[degree]
            accum[degree - self.L] += self.fc0 * value
            accum[degree - self.L + self.fe] += self.fc * value
        return [value % self.q for value in accum[:self.L]]

    def add(self, left, right):
        return [(a + b) % self.q for a, b in zip(left, right)]

    def sample(self, stream, count):
        values = [byte % (self.q + 1) for byte in stream]
        pointer = count
        while pointer < len(values) and values[pointer] == self.q:
            pointer += 1
        output = values[:count]
        for i in range(count):
            if output[i] == self.q:
                output[i] = values[pointer] if pointer < len(values) else 0
                pointer += 1
                while pointer < len(values) and values[pointer] == self.q:
                    pointer += 1
        return output

    def expand_pk(self, seed, index):
        first = hashlib.shake_128(seed + (2 * index - 2).to_bytes(2, "big")).digest(self.tau_n1)
        values = self.sample(first, self.n1)
        p1, pointer = [[None] * self.V for _ in range(self.V)], 0
        for row in range(self.V):
            for column in range(row, self.V):
                p1[row][column] = values[pointer:pointer + self.L]
                p1[column][row] = p1[row][column]
                pointer += self.L
        second = hashlib.shake_128(seed + (2 * index - 1).to_bytes(2, "big")).digest(self.tau_n2)
        values = self.sample(second, self.n2)
        p2 = [[values[(row * self.M + column) * self.L:(row * self.M + column + 1) * self.L]
               for column in range(self.M)] for row in range(self.V)]
        return p1, p2

    def unpack(self, data, count):
        value = int.from_bytes(data, "little")
        mask = (1 << self.qbits) - 1
        output = [(value >> (self.qbits * i)) & mask for i in range(count)]
        if value >> (self.qbits * count) or self.q in output:
            raise ValueError("non-canonical QR-UOV encoding")
        return output

    def pack(self, values):
        encoded = sum(value << (self.qbits * i) for i, value in enumerate(values))
        return encoded.to_bytes((self.qbits * len(values) + 7) // 8, "little")

    def parse_pk(self, pk):
        seed = pk[:16]
        coefficients = self.unpack(pk[16:], self.m * self.p3_count)
        matrices = []
        for equation in range(self.m):
            values = coefficients[equation * self.p3_count:(equation + 1) * self.p3_count]
            matrix, pointer = [[None] * self.M for _ in range(self.M)], 0
            for row in range(self.M):
                for column in range(row, self.M):
                    matrix[row][column] = values[pointer:pointer + self.L]
                    matrix[column][row] = matrix[row][column]
                    pointer += self.L
            matrices.append(matrix)
        return seed, matrices

    def evaluate_extension(self, p1, p2, p3, vector):
        blocks = [vector[i * self.L:(i + 1) * self.L] for i in range(self.N)]
        vinegar, oil = blocks[:self.V], blocks[self.V:]
        result = [0] * self.L
        for row in range(self.V):
            accum = [0] * self.L
            for column in range(self.V):
                accum = self.add(accum, self.multiply(p1[row][column], vinegar[column]))
            for column in range(self.M):
                term = self.multiply(p2[row][column], oil[column])
                accum = self.add(self.add(accum, term), term)
            result = self.add(result, self.multiply(vinegar[row], accum))
        for row in range(self.M):
            accum = [0] * self.L
            for column in range(self.M):
                accum = self.add(accum, self.multiply(p3[row][column], oil[column]))
            result = self.add(result, self.multiply(oil[row], accum))
        return result

    def public_map(self, pk, vector, cache):
        seed, p3 = self.parse_pk(pk)
        output = []
        for equation in range(1, self.m + 1):
            if equation not in cache:
                cache[equation] = self.expand_pk(seed, equation)
            p1, p2 = cache[equation]
            output.append(self.evaluate_extension(p1, p2, p3[equation - 1], vector)[self.phi_index])
        return output

    def target(self, pk, message, salt):
        digest = hashlib.shake_256(pk[:16] + message).digest(64)
        return self.sample(hashlib.shake_256(digest + salt).digest(self.tau_m), self.m)

    def canonical(self, vector):
        return next((value <= (self.q - 1) // 2 for value in vector if value), True)

    def verify(self, pk, message, signature, cache):
        salt = signature[:self.salt_bytes]
        vector = self.unpack(signature[self.salt_bytes:self.sig_bytes], self.n)
        return self.canonical(vector) and self.public_map(pk, vector, cache) == self.target(pk, message, salt)
