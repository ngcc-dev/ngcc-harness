#!/usr/bin/env python3
"""Reproduce the FLIT512 reference/AVX2 ciphertext incompatibility."""

from __future__ import annotations

import ctypes
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parent
REF = ROOT / "Implementations/Reference_Implementation/FLIT512"
OPT = ROOT / "Implementations/Optimized_Implementation/FLIT512"
COMMON = [
    "kem.c", "ntt.c", "poly.c", "decode.c", "packing.c", "reduce.c",
    "verify.c", "auxfunc.c", "drng.c", "indcpa.c",
    "KEM_AlgorithmInstance.c",
]
OPT_EXTRA = ["ntt_avx.c", "basemul_avx.c", "consts.c"]


def build(source: Path, output: Path, optimized: bool) -> None:
    shim = output.with_suffix(".c")
    shim.write_text('#include "drng.h"\nDRNG_ctx drng_algorithm;\n')
    flags = ["-O2", "-std=c11", "-fPIC", "-shared", "-fcommon"]
    if optimized:
        flags += ["-mavx2", "-mbmi2", "-mpopcnt", "-mpclmul", "-march=native"]
    command = ["cc", *flags, "-I", str(source), "-o", str(output)]
    command += [str(source / name) for name in COMMON]
    if optimized:
        command += [str(source / name) for name in OPT_EXTRA]
    command.append(str(shim))
    subprocess.run(command, check=True)


class Kem:
    def __init__(self, path: Path):
        self.lib = ctypes.CDLL(str(path), mode=ctypes.RTLD_LOCAL)
        for name in ("kem_get_pk_len_bytes", "kem_get_sk_len_bytes",
                     "kem_get_ct_len_bytes", "kem_get_ss_len_bytes"):
            getattr(self.lib, name).restype = ctypes.c_ulonglong
        self.pk_n = self.lib.kem_get_pk_len_bytes()
        self.sk_n = self.lib.kem_get_sk_len_bytes()
        self.ct_n = self.lib.kem_get_ct_len_bytes()
        self.ss_n = self.lib.kem_get_ss_len_bytes()
        self.state = (ctypes.c_ubyte * 165).in_dll(self.lib, "drng_algorithm")
        self.lib.init_random_number.argtypes = [ctypes.c_void_p, ctypes.c_void_p,
                                                ctypes.c_ulonglong]

    def seed(self, value: bytes) -> None:
        seed = ctypes.create_string_buffer(value)
        assert self.lib.init_random_number(self.state, seed, len(value)) == 0

    def keygen(self) -> tuple[bytes, bytes]:
        pk, sk = ctypes.create_string_buffer(self.pk_n), ctypes.create_string_buffer(self.sk_n)
        pn, sn = ctypes.c_ulonglong(), ctypes.c_ulonglong()
        assert self.lib.kem_keygen(pk, ctypes.byref(pn), sk, ctypes.byref(sn)) == 0
        return pk.raw[:pn.value], sk.raw[:sn.value]

    def enc(self, pk_value: bytes) -> tuple[bytes, bytes]:
        pk = ctypes.create_string_buffer(pk_value)
        ct, ss = ctypes.create_string_buffer(self.ct_n), ctypes.create_string_buffer(self.ss_n)
        cn, sn = ctypes.c_ulonglong(), ctypes.c_ulonglong()
        assert self.lib.kem_enc(pk, len(pk_value), ss, ctypes.byref(sn),
                                ct, ctypes.byref(cn)) == 0
        return ct.raw[:cn.value], ss.raw[:sn.value]

    def dec(self, sk_value: bytes, ct_value: bytes) -> tuple[int, bytes]:
        sk, ct = ctypes.create_string_buffer(sk_value), ctypes.create_string_buffer(ct_value)
        ss, sn = ctypes.create_string_buffer(self.ss_n), ctypes.c_ulonglong()
        rc = self.lib.kem_dec(sk, len(sk_value), ct, len(ct_value), ss, ctypes.byref(sn))
        return rc, ss.raw[:sn.value]


def main() -> None:
    with tempfile.TemporaryDirectory(prefix="flit-interop-") as tmp_name:
        tmp = Path(tmp_name)
        build(REF, tmp / "ref.so", False)
        build(OPT, tmp / "opt.so", True)
        ref, opt = Kem(tmp / "ref.so"), Kem(tmp / "opt.so")
        seed = bytes(range(64))
        ref.seed(seed)
        opt.seed(seed)
        rpk, rsk = ref.keygen()
        opk, osk = opt.keygen()
        assert rpk == opk and rsk == osk
        rct, rss = ref.enc(rpk)
        oct_, oss = opt.enc(opk)
        rrc, rself = ref.dec(rsk, rct)
        orc, oself = opt.dec(osk, oct_)
        xrc1, cross1 = ref.dec(rsk, oct_)
        xrc2, cross2 = opt.dec(osk, rct)
        assert rrc == orc == 0 and rself == rss and oself == oss
        assert rct != oct_ and rss != oss
        assert cross1 != oss and cross2 != rss
        print("CONFIRMED kem-15-1 FLIT512 reference/optimized incompatibility")
        print(f"same_pk=yes same_sk=yes differing_ct_bytes={sum(a != b for a, b in zip(rct, oct_))}")
        print(f"ref_dec_opt_rc={xrc1} opt_dec_ref_rc={xrc2} cross_secret_match=no")


if __name__ == "__main__":
    main()
