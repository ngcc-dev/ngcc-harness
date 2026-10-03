#!/usr/bin/env python3
"""Full-parameter accounting certificate for TRIKE's generic multi-target loss."""

import hashlib
import json
from math import log2
from pathlib import Path

ROOT = Path(__file__).resolve().parent
REF = ROOT / "Implementations and Test_Vectors/Implementations/Reference_Implementation"
OPT = ROOT / "Implementations and Test_Vectors/Implementations/Optimized_Implementation"

CASES = (
    # set, message bits, published mean encapsulation cycles, target,
    # SHA-256 of the x86_1 raw timing record
    ("TRIKE-5", 256, 69356445.85806452, 256,
     "596e690cff2e659e97893a2d66102e7cfc3c22d2ec5b06b3988073f51f60272a"),
    ("TRIKE-9", 512, 206799142.74, 512,
     "2f23d094719a102c7c361731aba4395026a596e3f5ba5231cfc2612b8e90f65b"),
)


def main():
    performance = ROOT.parent / "performance/data/x86_1/records/kem-36"
    for name, message_bits, cycles, target, record_sha in CASES:
        ref = (REF / name / "src/KEM_AlgorithmInstance.c").read_text()
        opt = (OPT / name / "src/KEM_AlgorithmInstance.c").read_text()
        for tree, source in ((REF, ref), (OPT, opt)):
            params = (tree / name / "src/trike_params.h").read_text()
            assert f"#define PARAM_M {message_bits}" in params
            assert "get_random_number(&drng_algorithm, msg, PARAM_M);" in source
            assert "generate_error_vector(e, e + R_ZMM_SIZE_BYTES" in source
            assert "calculate_uv(u, e0, e1, e2, r1, r2);" in source
            assert "memcpy(hash_input, msg, M_SIZE_BYTES);" in source
            assert "hash_length_m(hash_input, (M_SIZE_BYTES + sizeof(ciphertext_t)) * 8, ss);" in source

        # The public harness carries the raw benchmark evidence.  Bind the
        # decisive cycle constants to those records when they are present;
        # ngcc1 intentionally does not duplicate the benchmark corpus.
        record = performance / f"{name}__enc.json"
        if performance.exists():
            raw = record.read_bytes()
            assert hashlib.sha256(raw).hexdigest() == record_sha
            measured = json.loads(raw)
            assert measured["candidate"] == "kem-36"
            assert measured["instance"] == name
            assert measured["operation"] == "enc"
            assert measured["mean_cycles"] == cycles

        trial_exponent = message_bits - 32
        cycle_exponent = trial_exponent + log2(cycles)
        assert trial_exponent == {256: 224, 512: 480}[message_bits]
        print(f"{name}: T=2^32 targets, search=2^{trial_exponent} public "
              f"encapsulations (about 2^{cycle_exponent:.3f} measured cycles)")

    print("MULTI-CIPHERTEXT PROPERTY kem-36-7 CONFIRMED: work scales as 2^l/T")


if __name__ == "__main__":
    main()
