#!/usr/bin/env python3
"""Check the 256-bit authentication-key search space in frozen DKEX-512 (kex-04-3).

This exercises a known seed, not the infeasible exhaustive search.
"""

from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parent
REF = ROOT / "Implementations/Reference_Implementation/DKEX-512"
OPT = ROOT / "Implementations/Optimized_Implementation/DKEX-512"


def main() -> None:
    header = (REF / "adkex_sig.h").read_text()
    params = (REF / "dilithium/params.h").read_text()
    sign_source = (REF / "dilithium/sign.c").read_text()
    wrapper = (REF / "adkex_derand.c").read_text()
    assert "#  define ADKEX_SIG_COINBITS    (8 * 32)" in header
    assert "#define SEEDBYTES 32" in params
    assert "randombytes(seedbuf, SEEDBYTES);" in sign_source
    assert "adkex_sig_sign(sigma_B, sk_B, T_st, ADKEX_SSBITS);" in wrapper
    assert "adkex_sig_verify(pk_B, sigma_B, T, ADKEX_SSBITS)" in wrapper

    # The optimized tree is shipped in the candidate archive, but is not
    # retained in every local checkout. Check the relevant copies if present.
    if OPT.is_dir():
        for name in ("adkex_sig.h", "adkex_sig_mldsa.c", "adkex_derand.c",
                     "dilithium/params.h", "dilithium/sign.c"):
            assert (REF / name).read_bytes() == (OPT / name).read_bytes(), name

    dilithium = REF / "dilithium"
    sources = [REF / "adkex_sig_mldsa.c", ROOT / "reproduce_signature_seed_cap.c"]
    sources += [dilithium / name for name in (
        "fips202.c", "ntt.c", "packing.c", "poly.c", "polyvec.c",
        "reduce.c", "rounding.c", "sign.c", "symmetric-shake.c")]
    with tempfile.TemporaryDirectory(prefix="ngcc-dkex-seed-") as temp_dir:
        executable = Path(temp_dir) / "witness"
        command = [
            "cc", "-std=c99", "-O2", "-fcommon", "-DADKEX_MODE=512",
            "-DDKE_MODE=512", "-DADKEX_SIG_BACKEND_MLDSA",
            "-DADKEX_SIG_MLDSA_LEVEL=5", "-DDILITHIUM_MODE=5",
            "-I", str(REF), "-I", str(dilithium), "-o", str(executable),
            *map(str, sources),
        ]
        subprocess.run(command, check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
