#!/usr/bin/env python3
"""Recover an MQOM key when two signatures repeat mseed and TweakSalt prefix.

This uses the submitted Python reference implementation, whose optional NumPy,
Numba, PyCryptodome and joblib dependencies are required.  Repeated signing
entropy models a stuck RNG or VM snapshot restore and is outside the ordinary
EUF-CMA experiment.
"""

import random
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PYREF = ROOT / "MQOM/submission/submission_package_v3/Reference_Implementation_Python"
REQUIREMENTS = "MQOM/submission/submission_package_v3/Reference_Implementation_Python/requirements.txt"
# Loading the submitted Python reference must not add generated files beneath
# the provenance-checked submission tree.
sys.dont_write_bytecode = True
sys.path.insert(0, str(PYREF))
try:
    from labels import get_instance_from_label
    from mqom import MQOM3
    from mqom.bits import pad_left
    from mqom.params import Variant
    from mqom.parsing import serialize
    from mqom.seeds import seed_expand
except ModuleNotFoundError as error:
    print(f"SKIP: submitted MQOM Python-reference dependency missing: {error.name}; "
          f"install -r {REQUIREMENTS}")
    raise SystemExit(77)


def main():
    label = sys.argv[1] if len(sys.argv) > 1 else "MQOM3-L1-gf16-fast-ct"
    try:
        params = get_instance_from_label(label)
    except ModuleNotFoundError as error:
        print(f"SKIP: submitted MQOM Python-reference dependency missing: {error.name}; "
              f"install -r {REQUIREMENTS}")
        return 77
    rng = random.Random(20261018)
    randombytes = lambda length: bytes(rng.randrange(256) for _ in range(length))
    scheme = MQOM3(params, randombytes)
    pk, sk = scheme.generate_keys()
    _, actual = scheme.sk_format.parse(sk)
    correlated = params.variant == Variant.CORRELATED_TREE

    def parse(signature):
        sig_id, salt, nonce, opening = scheme.sig_format.parse(signature)
        hidden = scheme._derive_challenge(sig_id, nonce)
        block = scheme.blc
        if correlated:
            chunks = block._opening_format.parse(opening)
            leaves, deltas = [], []
            for repetition in range(params.tau):
                path, _, delta, _ = block._rep_opening_format.parse(chunks[repetition])
                leaves.append(params.ggmtree.partially_expand(salt, path, repetition, hidden[repetition]))
                deltas.append(block.x_format.parse(pad_left(delta, params.lda)))
        else:
            path, _, deltas, _ = block._opening_format.parse(opening)
            indices = [hidden[e] * params.tau + e for e in range(params.tau)]
            expanded = params.ggmtree.partially_expand(salt, path, indices)
            leaves = [[expanded[params.tau * i + e] for i in range(params.N)]
                      for e in range(params.tau)]
            deltas = list(deltas)
        return salt, hidden, leaves, deltas

    mseed, salt = randombytes(params.lda), randombytes(params.lda)
    n_sep = params.domain_separation_bytesize
    second_salt = salt[:-n_sep] + bytes(byte ^ 0xA5 for byte in salt[-n_sep:])
    control_salt = bytes((salt[0] ^ 0x80,)) + salt[1:]
    message_one, message_two = b"message one", b"message two"
    signature_one = scheme.sign(sk, message_one, mseed=mseed, salt=salt)
    signature_two = scheme.sign(sk, message_two, mseed=mseed, salt=second_salt)
    control_signature = scheme.sign(sk, message_two, mseed=mseed, salt=control_salt)
    assert scheme.verify(pk, message_one, signature_one)
    assert scheme.verify(pk, message_two, signature_two)
    assert scheme.verify(pk, message_two, control_signature)
    salt_one, hidden_one, leaves_one, delta_one = parse(signature_one)
    salt_two, hidden_two, leaves_two, _ = parse(signature_two)
    control_parsed = parse(control_signature)

    def recover(other):
        _, hidden_other, leaves_other, _ = other
        for repetition in range(params.tau):
            if hidden_one[repetition] == hidden_other[repetition]:
                continue
            leaves = [
                leaves_one[repetition][i] if leaves_one[repetition][i] is not None
                else leaves_other[repetition][i]
                for i in range(params.N)
            ]
            if not all(leaf is not None for leaf in leaves):
                continue
            expanded = [
                scheme.blc.x_format.parse(
                    seed_expand(params, salt_one, repetition, leaf,
                                scheme.blc.x_format.get_bytesize() + scheme.blc.u_format.get_bytesize())
                    [:scheme.blc.x_format.get_bytesize()]
                )
                for leaf in leaves
            ]
            candidate = [
                delta_one[repetition][j] + sum(expanded[i][j] for i in range(params.N))
                for j in range(params.n)
            ]
            return candidate, repetition
        return None, None

    recovered, selected = recover((salt_two, hidden_two, leaves_two, None))
    control_recovered, control_selected = recover(control_parsed)
    if recovered is None:
        print("FAIL: no repetition had complementary openings")
        return 1
    replacement = serialize(pk, recovered)
    forgery_message = b"fresh message never signed by the key owner"
    forgery = scheme.sign(replacement, forgery_message)
    verified = scheme.verify(pk, forgery_message, forgery)
    print(f"instance: {label}; variant: {'CT' if correlated else 'OT'}")
    print(f"full salt repeated: {salt_one == salt_two}; "
          f"TweakSalt prefix repeated: {salt_one[:-n_sep] == salt_two[:-n_sep]}; "
          f"ignored suffix bytes: {n_sep}; complementary repetition: {selected}")
    print(f"different-prefix control repetition: {control_selected}; "
          f"secret recovered: {control_recovered == list(actual)}")
    print(f"secret x recovered: {recovered == list(actual)}; fresh-message signature verifies: {verified}")
    return 0 if recovered == list(actual) and verified and control_recovered != list(actual) else 1


if __name__ == "__main__":
    raise SystemExit(main())
