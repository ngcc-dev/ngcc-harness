#!/usr/bin/env python3
"""Fast certificate for the message-modification extension to three ZC Leads.

Checks the submitted C permutation against the independent model, solves the
three-round rate-only systems, and replays three reduced-round collision pairs.
It does not establish any full-round collision or the estimated 2^288 cost.
"""
import ctypes
import random
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MODEL = ROOT / "security/keccak_techniques/zc"
sys.path.insert(0, str(MODEL))
import mm  # noqa: E402
import mm_rate  # noqa: E402
from zc import RC_IMPL, ZC  # noqa: E402
from zcmodes import pad, xor_into, zc_hash  # noqa: E402

z = ZC(1536)
lib = ROOT / "hash-30/ZC-DM/Implementations/lib"
plain = lib / "low/ZuD-1536/plain/ZuD1536-plain.c"


def reduced_digest(mode, message):
    if mode != "EDMC":
        return zc_hash(mode, 1536, 768, message, rounds=4)
    # The submitted EDMC mode uses h(h(X) xor (0^r || X_c)); here h has two
    # reference last-round constants, giving two plus two reduced rounds.
    state = [0] * z.N
    for block in pad(message, 88):
        state = xor_into(z, state, block, 0)
        old = list(state)
        state = z.perm(state, RC_IMPL[-2:])
        state = xor_into(z, state, z.to_bytes(old)[88:], 88)
        state = z.perm(state, RC_IMPL[-2:])
    output = b""
    while len(output) < 96:
        output += z.to_bytes(state)[:88]
        if len(output) < 96:
            state = z.perm(state, RC_IMPL[-4:])
    return output[:96]


with tempfile.TemporaryDirectory(prefix="ngcc-zc-mm-") as temporary:
    library = Path(temporary) / "libzud.so"
    subprocess.run(
        ["cc", "-O2", "-shared", "-fPIC", "-I", str(lib / "common"),
         "-o", str(library), str(plain)],
        check=True, stdout=subprocess.DEVNULL,
    )
    ref = ctypes.CDLL(str(library))
    rng = random.Random(9)
    for rounds in (2, 4, 12):
        for _ in range(4):
            state = z.rand(rng)
            c_state = ctypes.create_string_buffer(z.to_bytes(state), 192)
            ref.ZuD1536_plain_Permute_Nrounds(c_state, rounds)
            assert z.from_bytes(c_state.raw) == z.perm(state, RC_IMPL[-rounds:])
    for cid, name, mode in ((30, "ZC-DM", "DM"),
                            (31, "ZC-DMC", "DMC"),
                            (32, "ZC-EDMC", "EDMC")):
        candidate = ROOT / f"hash-{cid}/{name}/Implementations"
        candidate_lib = candidate / "lib"
        implementation = candidate / f"Implementations/Reference_Implementation/{name}-1536-512"
        shared = Path(temporary) / f"lib{mode}.so"
        subprocess.run(
            ["cc", "-O2", "-shared", "-fPIC", "-I", str(implementation),
             "-I", str(candidate_lib / "common"), "-o", str(shared),
             str(implementation / "CryptHash_AlgorithmInstance.c"),
             str(candidate_lib / "low/ZuD-1536/plain/ZuD1536-plain.c")],
            check=True, stdout=subprocess.DEVNULL,
        )
        submitted = ctypes.CDLL(str(shared))
        for message in (b"", b"ngcc model check"):
            digest = ctypes.create_string_buffer(96)
            assert submitted.CryptHash(768, message, ctypes.c_ulonglong(8 * len(message)), digest) == 0
            assert digest.raw == zc_hash(mode, 1536, 768, message)

for rate, lane in ((704, 1), (448, 0)):
    for trial in range(4):
        state = z.rand(random.Random(100 + rate + trial))
        for bit, value in mm.cond_bits(lane, 0):
            state[bit // 64] = (state[bit // 64] & ~(1 << (bit % 64))) | (value << (bit % 64))
        result, error = mm_rate.build_rate(state, rate, lane, 0, RC_IMPL, seed=trial)
        assert result is not None, (rate, error)
        particular, basis, variables, info = result
        assert len(basis) == info["dim"] and len(basis) >= 100
        for value in (particular, particular ^ basis[0]):
            x = mm_rate.x_from_vars(state, variables, value)
            assert mm.trail_rounds(x, mm.delta(lane, 0), RC_IMPL) >= 3

for mode in ("DM", "DMC", "EDMC"):
    log = (MODEL / f"mm_hash_demo_{mode}.log").read_text()
    messages = []
    for index in (1, 2):
        match = re.search(rf"\[{mode}\] message {index} \(\d+ bytes\): ([0-9a-f]+)", log)
        assert match is not None
        messages.append(bytes.fromhex(match.group(1)))
    assert messages[0] != messages[1]
    assert reduced_digest(mode, messages[0]) == reduced_digest(mode, messages[1])
    assert zc_hash(mode, 1536, 768, messages[0]) != zc_hash(mode, 1536, 768, messages[1])

print("LEAD hash-30-1/hash-31-2/hash-32-2 VERIFIED: rate-only three-round systems and reduced-round collisions; full-round estimate unproved")
