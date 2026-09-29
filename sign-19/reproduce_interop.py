#!/usr/bin/env python3
"""Reproduce the Phoenix-SHAKE-128f reference/AVX2 incompatibility."""

from __future__ import annotations

import ctypes
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parent
BASE = ROOT / "Implementations and Test_Vectors/Implementations"
REF = BASE / "Reference_Implementation/Phoenix-SHAKE-128f"
OPT = BASE / "Optimized_Implementation/avx2/Phoenix-SHAKE-128f-avx2"
REF_SOURCES = [
    "address.c", "counter.c", "hash_shake.c", "fips202.c", "merkle.c",
    "octopus.c", "randombytes.c", "sign.c", "tfors.c",
    "thash_shake_simple.c", "utils.c", "utilsx1.c", "gwots.c",
    "gwotsx1.c", "SIG_AlgorithmInstance.c", "drng.c",
]
OPT_SOURCES = [
    "address.c", "counter.c", "fips202.c", "fips202x4.c", "hash_shake.c",
    "hash_shake256x4.c", "keccak4x/KeccakP-1600-times4-SIMD256.c",
    "merkle.c", "octopus.c", "randombytes.c", "tfors.c",
    "thash_shake_simple.c", "thash_shake_simplex4.c", "utils.c",
    "utilsx1.c", "utilsx4.c", "gwots.c", "gwotsx1.c", "gwotsx4.c",
    "SIG_AlgorithmInstance.c", "drng.c",
]


def build(source: Path, output: Path, optimized: bool) -> None:
    shim = output.with_suffix(".c")
    shim.write_text('#include "drng.h"\nDRNG_ctx drng_algorithm;\n')
    flags = [
        "-O3", "-std=c99", "-fPIC", "-shared", "-DPARAMS=phoenix-shake-128f",
        "-DALLOW_DEEP_TREES", "-DSUBMISSION_DRNG", "-I", str(source),
    ]
    if optimized:
        flags += ["-mavx2", "-march=native", "-DAVX2_OPTIMIZED"]
    sources = OPT_SOURCES if optimized else REF_SOURCES
    subprocess.run(["cc", *flags, "-o", str(output),
                    *(str(source / name) for name in sources), str(shim)], check=True)


class Sig:
    def __init__(self, path: Path):
        self.lib = ctypes.CDLL(str(path), mode=ctypes.RTLD_LOCAL)
        for name in ("sig_get_pk_len_bytes", "sig_get_sk_len_bytes",
                     "sig_get_sn_len_bytes"):
            getattr(self.lib, name).restype = ctypes.c_ulonglong
        self.pk_n = self.lib.sig_get_pk_len_bytes()
        self.sk_n = self.lib.sig_get_sk_len_bytes()
        self.sn_n = self.lib.sig_get_sn_len_bytes()
        self.state = (ctypes.c_ubyte * 165).in_dll(self.lib, "drng_algorithm")
        self.lib.init_random_number.argtypes = [ctypes.c_void_p, ctypes.c_void_p,
                                                ctypes.c_ulonglong]

    def seed(self, value: bytes) -> None:
        seed = ctypes.create_string_buffer(value)
        assert self.lib.init_random_number(self.state, seed, len(value)) == 0

    def keygen(self) -> tuple[bytes, bytes]:
        pk, sk = ctypes.create_string_buffer(self.pk_n), ctypes.create_string_buffer(self.sk_n)
        pn, sn = ctypes.c_ulonglong(), ctypes.c_ulonglong()
        assert self.lib.sig_keygen(pk, ctypes.byref(pn), sk, ctypes.byref(sn)) == 0
        return pk.raw[:pn.value], sk.raw[:sn.value]

    def sign(self, sk_value: bytes, message: bytes) -> bytes:
        sk, msg = ctypes.create_string_buffer(sk_value), ctypes.create_string_buffer(message)
        signature, length = ctypes.create_string_buffer(self.sn_n + 16), ctypes.c_ulonglong()
        assert self.lib.sig_sign(sk, len(sk_value), msg, len(message),
                                 signature, ctypes.byref(length)) == 0
        return signature.raw[:length.value]

    def verify(self, pk_value: bytes, signature: bytes, message: bytes) -> int:
        pk = ctypes.create_string_buffer(pk_value)
        sn = ctypes.create_string_buffer(signature)
        msg = ctypes.create_string_buffer(message)
        return self.lib.sig_verify(pk, len(pk_value), sn, len(signature), msg, len(message))


def main() -> None:
    seed = bytes.fromhex(
        "FAD9DABF7CEDAD251A188BC5CD2820888411443E90050E6A32C096FF6C1EA2F"
        "363FA4C083D31AA319F40F302F389DD695E9E42F6E638951AF0E69427E5D141C8"
    )
    message = bytes.fromhex(
        "F04A2770E801BA2633D149EDBBCDF4B935BE1508C4ECCB5CAC5E83C8AAD57CD1"
        "504D6DA1367C454AABA9544B2134762A12B7D825CCACC64A"
    )
    with tempfile.TemporaryDirectory(prefix="phoenix-interop-") as tmp_name:
        tmp = Path(tmp_name)
        build(REF, tmp / "ref.so", False)
        build(OPT, tmp / "opt.so", True)
        ref, opt = Sig(tmp / "ref.so"), Sig(tmp / "opt.so")
        ref.seed(seed)
        opt.seed(seed)
        rpk, rsk = ref.keygen()
        opk, osk = opt.keygen()
        assert rpk == opk and rsk == osk
        rsn, osn = ref.sign(rsk, message), opt.sign(osk, message)
        assert ref.verify(rpk, rsn, message) == 0
        assert opt.verify(opk, osn, message) == 0
        opt_ref = opt.verify(opk, rsn, message)
        ref_opt = ref.verify(rpk, osn, message)
        assert opt_ref == 0 and ref_opt == -1
        print("CONFIRMED sign-19-1 Phoenix-SHAKE-128f implementation incompatibility")
        print(f"reference_signature_bytes={len(rsn)} optimized_signature_bytes={len(osn)}")
        print(f"optimized_verifies_reference={opt_ref} reference_verifies_optimized={ref_opt}")


if __name__ == "__main__":
    main()
