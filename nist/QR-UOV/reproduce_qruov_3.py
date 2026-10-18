#!/usr/bin/env python3
"""Recover the QR-UOV oil space and forge after one RNG repetition.

The default 1q127L3 witness takes about a minute on a typical workstation.
Pass 3q127L3 to repeat the same end-to-end test at category 3; that larger
case takes several minutes. The public model supports SHAKE public expansion.
"""

import ctypes
import random
import sys
from pathlib import Path

from reproduce_qruov_3_model import QRUOV


ROOT = Path(__file__).resolve().parents[1]


class Metadata(ctypes.Structure):
    _fields_ = [
        ("abi", ctypes.c_uint32), ("scheme", ctypes.c_char_p),
        ("parameter", ctypes.c_char_p), ("implementation", ctypes.c_char_p),
        ("pk", ctypes.c_uint64), ("sk", ctypes.c_uint64),
        ("sig", ctypes.c_uint64),
    ]


def buffer(value):
    if isinstance(value, int):
        return (ctypes.c_ubyte * value)()
    return (ctypes.c_ubyte * len(value)).from_buffer_copy(value)


def rank(vectors, q):
    if not vectors:
        return 0
    matrix = [row[:] for row in vectors]
    row = 0
    for column in range(len(matrix[0])):
        pivot = next((i for i in range(row, len(matrix)) if matrix[i][column] % q), None)
        if pivot is None:
            continue
        matrix[row], matrix[pivot] = matrix[pivot], matrix[row]
        inverse = pow(matrix[row][column], q - 2, q)
        matrix[row] = [value * inverse % q for value in matrix[row]]
        for i in range(len(matrix)):
            if i != row and matrix[i][column]:
                scale = matrix[i][column]
                matrix[i] = [(a - scale * b) % q for a, b in zip(matrix[i], matrix[row])]
        row += 1
    return row


def solve(matrix, target, q):
    size = len(matrix)
    work = [row[:] + [value] for row, value in zip(matrix, target)]
    row, pivots = 0, []
    for column in range(size):
        pivot = next((i for i in range(row, size) if work[i][column] % q), None)
        if pivot is None:
            continue
        work[row], work[pivot] = work[pivot], work[row]
        inverse = pow(work[row][column], q - 2, q)
        work[row] = [value * inverse % q for value in work[row]]
        for i in range(size):
            if i != row and work[i][column]:
                scale = work[i][column]
                work[i] = [(a - scale * b) % q for a, b in zip(work[i], work[row])]
        pivots.append(column)
        row += 1
    if any(work[i][size] for i in range(row, size)):
        return None
    output = [0] * size
    for i, column in enumerate(pivots):
        output[column] = work[i][size]
    return output


def main():
    parameter = sys.argv[1] if len(sys.argv) > 1 else "1q127L3"
    if parameter not in ("1q127L3", "3q127L3"):
        raise SystemExit("usage: reproduce_qruov_3.py [1q127L3|3q127L3]")
    model = QRUOV(parameter)
    path = ROOT / "QR-UOV/lib" / f"{parameter}-shake.so"
    lib = ctypes.CDLL(str(path))
    bytep = ctypes.POINTER(ctypes.c_ubyte)
    lib.nist_sig_get_api.restype = ctypes.POINTER(Metadata)
    lib.nist_sig_seed.argtypes = [bytep]
    lib.nist_sig_keypair.argtypes = [bytep, bytep]
    lib.nist_sig_sign.argtypes = [bytep, ctypes.POINTER(ctypes.c_uint64), bytep, ctypes.c_uint64, bytep]
    lib.nist_sig_open.argtypes = [bytep, ctypes.POINTER(ctypes.c_uint64), bytep, ctypes.c_uint64, bytep]
    meta = lib.nist_sig_get_api().contents
    assert meta.pk == model.pk_bytes and meta.sig == model.sig_bytes

    def reseed(value):
        assert len(value) == 48
        assert lib.nist_sig_seed(buffer(value)) == 0

    reseed(bytes(range(100, 148)))
    pk_out, sk_out = buffer(meta.pk), buffer(meta.sk)
    assert lib.nist_sig_keypair(pk_out, sk_out) == 0
    pk, sk = bytes(pk_out), bytes(sk_out)

    def sign(seed, message):
        reseed(seed)
        signed = buffer(meta.sig + len(message))
        length = ctypes.c_uint64()
        assert lib.nist_sig_sign(signed, ctypes.byref(length), buffer(message), len(message), buffer(sk)) == 0
        assert length.value == meta.sig + len(message)
        return bytes(signed[:meta.sig])

    def library_open(signature, message):
        signed = signature + message
        output, length = buffer(len(signed)), ctypes.c_uint64()
        rc = lib.nist_sig_open(output, ctypes.byref(length), buffer(signed), len(signed), buffer(pk))
        return rc == 0 and bytes(output[:length.value]) == message

    seed_pk, p3 = model.parse_pk(pk)
    matrices = [model.expand_pk(seed_pk, i) for i in range(1, model.m + 1)]
    cache = {i + 1: matrices[i] for i in range(model.m)}
    zero = [0] * model.L
    one = [1] + [0] * (model.L - 1)

    def public_map(vector):
        return model.public_map(pk, vector, cache)

    def is_oil(vector):
        return not any(
            any(model.evaluate_extension(matrices[i][0], matrices[i][1], p3[i], vector))
            for i in range(model.m)
        )

    def blocks(vector):
        return [vector[i * model.L:(i + 1) * model.L] for i in range(model.N)]

    def field_power(value, exponent):
        result = one
        while exponent:
            if exponent & 1:
                result = model.multiply(result, value)
            value = model.multiply(value, value)
            exponent >>= 1
        return result

    def field_inverse(value):
        return field_power(value, model.q ** model.L - 2)

    def field_sub(left, right):
        return [(a - b) % model.q for a, b in zip(left, right)]

    def common_kernel(rows, columns):
        """Return rank and a kernel basis over F_(q^L)."""
        work = [[value[:] for value in row] for row in rows]
        pivots, current = [], 0
        for column in range(columns):
            pivot = next((i for i in range(current, len(work)) if any(work[i][column])), None)
            if pivot is None:
                continue
            work[current], work[pivot] = work[pivot], work[current]
            inverse = field_inverse(work[current][column])
            work[current] = [model.multiply(value, inverse) for value in work[current]]
            for i in range(len(work)):
                if i != current and any(work[i][column]):
                    factor = work[i][column]
                    work[i] = [
                        field_sub(a, model.multiply(factor, b))
                        for a, b in zip(work[i], work[current])
                    ]
            pivots.append(column)
            current += 1
        free = [column for column in range(columns) if column not in pivots]
        basis = []
        for free_column in free:
            vector = [zero] * columns
            vector[free_column] = one
            for i, column in enumerate(pivots):
                vector[column] = [(-value) % model.q for value in work[i][free_column]]
            basis.append(vector)
        return current, basis

    def symmetric_entry(equation, row, column):
        """Entry of the public lifted symmetric matrix over F_(q^L)."""
        p1, p2 = matrices[equation]
        if row < model.V and column < model.V:
            return p1[row][column]
        if row < model.V:
            return p2[row][column - model.V]
        if column < model.V:
            return p2[column][row - model.V]
        return p3[equation][row - model.V][column - model.V]

    control_a = sign(b"\x01" * 48, b"control message A")
    control_b = sign(b"\x02" * 48, b"control message B")
    vector_a = model.unpack(control_a[model.salt_bytes:], model.n)
    vector_b = model.unpack(control_b[model.salt_bytes:], model.n)
    control_oil = any(
        is_oil([(a - sign_b * b) % model.q for a, b in zip(vector_a, vector_b)])
        for sign_b in (1, -1)
    )
    print(f"control (distinct RNG seeds): oil vector found={control_oil}")

    repeated = b"\x55" * 48
    signature_a = sign(repeated, b"message A")
    signature_b = sign(repeated, b"message B")
    assert library_open(signature_a, b"message A")
    assert library_open(signature_b, b"message B")
    vector_a = model.unpack(signature_a[model.salt_bytes:], model.n)
    vector_b = model.unpack(signature_b[model.salt_bytes:], model.n)
    oil_vector = None
    for sign_b in (1, -1):
        candidate = [(a - sign_b * b) % model.q for a, b in zip(vector_a, vector_b)]
        if any(candidate) and is_oil(candidate):
            oil_vector = candidate
            break
    print(
        "one repeated-RNG pair: "
        f"salts equal={signature_a[:model.salt_bytes] == signature_b[:model.salt_bytes]}; "
        f"oil vector found={oil_vector is not None}"
    )
    if oil_vector is None or control_oil:
        return 1

    oil_blocks = blocks(oil_vector)
    polar_rows = []
    for equation in range(model.m):
        row = []
        for column in range(model.N):
            value = zero
            for source_row in range(model.N):
                value = model.add(
                    value,
                    model.multiply(oil_blocks[source_row], symmetric_entry(equation, source_row, column)),
                )
            row.append(value)
        polar_rows.append(row)
    polar_rank, extension_basis = common_kernel(polar_rows, model.N)
    if polar_rank != model.V or len(extension_basis) != model.M:
        raise RuntimeError(
            f"unexpected polar kernel: rank={polar_rank}, dimension={len(extension_basis)}"
        )

    oil_basis = []
    for vector in extension_basis:
        flat = sum(vector, [])
        assert is_oil(flat)
        for power in range(model.L):
            monomial = [int(i == power) for i in range(model.L)]
            oil_basis.append(sum((model.multiply(value, monomial) for value in vector), []))
    oil_rank = rank(oil_basis, model.q)
    print(
        f"public polar rank={polar_rank} (V={model.V}); "
        f"recovered oil-space rank={oil_rank} (m={model.m})"
    )
    if oil_rank != model.m:
        return 1

    rng = random.Random(7)
    message = b"fresh message never signed by the key owner"
    while True:
        salt = bytes(rng.randrange(256) for _ in range(model.salt_bytes))
        target = model.target(pk, message, salt)
        start = [rng.randrange(model.q) for _ in range(model.n)]
        start_map = public_map(start)
        columns = []
        for vector in oil_basis:
            shifted = [(a + b) % model.q for a, b in zip(start, vector)]
            columns.append([(a - b) % model.q for a, b in zip(public_map(shifted), start_map)])
        matrix = [[columns[column][row] for column in range(model.m)] for row in range(model.m)]
        solution = solve(matrix, [(a - b) % model.q for a, b in zip(target, start_map)], model.q)
        if solution is None:
            continue
        result = start[:]
        for scalar, vector in zip(solution, oil_basis):
            result = [(a + scalar * b) % model.q for a, b in zip(result, vector)]
        if not model.canonical(result):
            result = [(-value) % model.q for value in result]
        forgery = salt + model.pack(result)
        model_ok = model.verify(pk, message, forgery, cache)
        library_ok = library_open(forgery, message)
        print(f"fresh-message forgery: public model={model_ok}; submitted verifier={library_ok}")
        return 0 if model_ok and library_ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
