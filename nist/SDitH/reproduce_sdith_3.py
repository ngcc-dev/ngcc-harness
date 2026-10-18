#!/usr/bin/env python3
"""Recover the SDitH cat1_fast witness after repeated signing entropy.

The signer RNG is reset before two signatures on different messages.  This
models a stuck RNG or a VM snapshot restore; it is not an EUF-CMA capability.
The attack phase combines only the public key and the two signatures.
"""

import ctypes as C
import hashlib
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
LIBRARY = ROOT / "SDitH/lib/cat1_fast.so"
LAM, KAPPA, TAU, TOPEN, DEGREE, EXTRA = 128, 7, 18, 107, 4, 16
LB = LAM // 8
N = 1 << KAPPA
TOTAL = TAU * N
WITNESS_BITS = 616
VOLE_BITS = (LAM + EXTRA) + WITNESS_BITS + (DEGREE - 1) * LAM
VOLE_BYTES = (VOLE_BITS + 7) // 8
VOLE_SLICE = ((VOLE_BITS >> 3) + 31) & ~31
PK_BYTES, SK_BYTES, SIG_BYTES = 70, 147, 4914


def xor(left, right):
    return bytes(a ^ b for a, b in zip(left, right))


def main():
    lib = C.CDLL(str(LIBRARY))
    lib.nist_sig_seed.argtypes = [C.c_char_p]
    lib.nist_sig_keypair.argtypes = [C.c_char_p, C.c_char_p]
    lib.nist_sig_sign.argtypes = [C.c_char_p, C.POINTER(C.c_uint64), C.c_char_p, C.c_uint64, C.c_char_p]
    lib.nist_sig_open.argtypes = [C.c_char_p, C.POINTER(C.c_uint64), C.c_char_p, C.c_uint64, C.c_char_p]
    lib.hidden_leaves_indexes2.argtypes = [C.c_uint64, C.c_uint64, C.POINTER(C.c_uint32), C.c_char_p]
    lib.extend_leaf_seed_cat1_aes128_ref.argtypes = [C.c_char_p, C.c_char_p]
    lib.ggm_seed_rng_lr_ext_cat1_aes128_ref.argtypes = [C.c_char_p, C.c_char_p, C.c_char_p, C.c_uint64, C.c_uint32]
    lib.vole_rng_ext_cat1_aes128_ctrle_ref.argtypes = [C.c_char_p, C.c_uint64, C.c_char_p, C.c_char_p, C.c_uint64]

    def keypair(seed):
        pk, sk = C.create_string_buffer(PK_BYTES), C.create_string_buffer(SK_BYTES)
        assert lib.nist_sig_seed(seed) == 0
        assert lib.nist_sig_keypair(pk, sk) == 0
        return pk.raw, sk.raw

    def sign(seed, message, sk):
        signed = C.create_string_buffer(len(message) + SIG_BYTES)
        length = C.c_uint64()
        assert lib.nist_sig_seed(seed) == 0
        assert lib.nist_sig_sign(signed, C.byref(length), message, len(message), sk) == 0
        return signed.raw[:length.value]

    def opened(signed, pk):
        message = C.create_string_buffer(len(signed))
        length = C.c_uint64()
        return lib.nist_sig_open(message, C.byref(length), signed, len(signed), pk) == 0

    def parse(signature):
        offset, fields = 0, {}
        layout = [
            ("salt", LB), ("path", TOPEN * LB), ("hcmt", TAU * 2 * LB),
            ("corr", VOLE_BYTES * (TAU - 1)), ("cchk", (LAM + EXTRA) // 8),
            ("inpub", (WITNESS_BITS + 7) // 8), ("cz", (DEGREE - 1) * LB),
            ("hpiop", 2 * LB), ("ctr", 4),
        ]
        for name, length in layout:
            fields[name] = signature[offset:offset + length]
            offset += length
        assert offset == SIG_BYTES
        return fields

    def hidden_leaves(fields):
        count = (TAU * KAPPA + 4 + 7) // 8
        challenge = hashlib.shake_128(fields["hpiop"] + fields["ctr"]).digest(count)
        challenge = challenge[:(TAU * KAPPA + 7) // 8] + bytes(8)
        output = (C.c_uint32 * TAU)()
        lib.hidden_leaves_indexes2(KAPPA, TAU, output, challenge)
        return sorted(output)

    def siblings(hidden):
        queue, output, index = sorted(hidden, reverse=True), [], 0
        while len(queue) - index >= 2:
            left, right = queue[index], queue[index + 1]
            if left ^ right == 1:
                index += 2
            else:
                output.append(left ^ 1)
                index += 1
            queue.append(left >> 1)
        while queue[index] != 1:
            output.append(queue[index] ^ 1)
            queue.append(queue[index] >> 1)
            index += 1
        return output

    def tweak_salt(salt, ggm):
        low = int.from_bytes(salt[:8], "little") & ~((1 << 24) - 1)
        high = int.from_bytes(salt[8:], "little")
        high = high | (1 << 63) if ggm else high & ~(1 << 63)
        return low.to_bytes(8, "little") + high.to_bytes(8, "little")

    def extended(seed):
        output = C.create_string_buffer(16 * 11)
        lib.extend_leaf_seed_cat1_aes128_ref(output, seed)
        return output.raw

    def children(salt, seed, node):
        output = C.create_string_buffer(32)
        lib.ggm_seed_rng_lr_ext_cat1_aes128_ref(output, salt, extended(seed), 2 * node, 24)
        return output.raw[:16], output.raw[16:]

    def vole(salt, seed, repetition):
        output = C.create_string_buffer(VOLE_SLICE)
        lib.vole_rng_ext_cat1_aes128_ctrle_ref(output, VOLE_SLICE, salt, extended(seed), repetition)
        return output.raw[:VOLE_BYTES]

    def revealed(fields):
        hidden = hidden_leaves(fields)
        sibling_nodes = siblings(hidden)
        known = {
            node: fields["path"][i * LB:(i + 1) * LB]
            for i, node in enumerate(sibling_nodes)
        }
        assert all(byte == 0 for byte in fields["path"][len(sibling_nodes) * LB:])
        stack, leaves = list(known.items()), {}
        salt = tweak_salt(fields["salt"], True)
        while stack:
            node, seed = stack.pop()
            if node >= TOTAL:
                leaves[node] = seed
                continue
            left, right = children(salt, seed, node)
            stack.extend(((2 * node, left), (2 * node + 1, right)))
        return leaves

    def recover(first_fields, second_fields):
        first_leaves, second_leaves = revealed(first_fields), revealed(second_fields)
        all_leaves = dict(first_leaves)
        all_leaves.update(second_leaves)
        selected = None
        for repetition in range(TAU):
            nodes = [TOTAL + i * TAU + repetition for i in range(N)]
            if all(node in all_leaves for node in nodes):
                selected = repetition, nodes
                break
        if selected is None:
            return None, None, 0
        repetition, nodes = selected
        aggregate = bytes(VOLE_BYTES)
        salt = tweak_salt(first_fields["salt"], False)
        for node in nodes:
            aggregate = xor(aggregate, vole(salt, all_leaves[node], repetition))
        if repetition:
            start = (repetition - 1) * VOLE_BYTES
            aggregate = xor(aggregate, first_fields["corr"][start:start + VOLE_BYTES])
        offset = (LAM + EXTRA) // 8
        recovered = xor(first_fields["inpub"], aggregate[offset:offset + (WITNESS_BITS + 7) // 8])
        return recovered, repetition, len(nodes)

    pk, sk = keypair(bytes(range(48)))
    repeated = bytes([0xA5] * 48)
    first_message, second_message = b"message one", b"message two (different)"
    first = sign(repeated, first_message, sk)
    second = sign(repeated, second_message, sk)
    assert opened(first, pk) and opened(second, pk)
    first_fields = parse(first[len(first_message):])
    second_fields = parse(second[len(second_message):])
    recovered, repetition, leaf_count = recover(first_fields, second_fields)
    if recovered is None:
        print("FAIL: no repetition was fully opened")
        return 1
    actual = sk[LB + 54:]
    replacement = pk + recovered
    fresh_message = b"fresh message never signed by the key owner"
    forgery = sign(bytes([0x3C] * 48), fresh_message, replacement)
    forged = opened(forgery, pk)

    distinct = sign(bytes([0x5A] * 48), second_message, sk)
    assert opened(distinct, pk)
    distinct_fields = parse(distinct[len(second_message):])
    control_recovered, _, _ = recover(first_fields, distinct_fields)
    control_passed = (
        tweak_salt(first_fields["salt"], True) != tweak_salt(distinct_fields["salt"], True)
        and control_recovered != actual
    )
    print(f"salt equal: {first_fields['salt'] == second_fields['salt']}")
    print(f"fully opened repetition: {repetition}; leaves: {leaf_count}")
    print(f"recovered witness equals secret-key witness: {recovered == actual}")
    print(f"reconstructed-key fresh-message signature verifies: {forged}")
    print(f"distinct-seed control does not recover witness: {control_passed}")
    return 0 if recovered == actual and forged and control_passed else 1


if __name__ == "__main__":
    sys.exit(main())
