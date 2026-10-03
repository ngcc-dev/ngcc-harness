#!/usr/bin/env python3
"""Certify and exercise QUBE's unseeded optimized PRNG path."""

from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent / "Implementations" / "Optimized_Implementation"


def text(path):
    return (ROOT / path).read_text(errors="replace")


def main():
    symmetric = text("src/common/symmetric.c")
    kem = text("src/common/kem.c")
    wrapper = text("src/common/kem_qube.c")

    assert "shake256incctx shake256_prng_ctx;" in symmetric
    calls_in_code = [line for line in kem.splitlines()
                     if re.match(r"\s*prng_get_bytes\(", line)]
    assert len(calls_in_code) == 5
    assert "prng_get_bytes(seed_kem, SEED_BYTES)" in kem
    assert "prng_get_bytes(seed_pke, SEED_BYTES)" in kem
    assert "prng_get_bytes(sigma, PARAM_SECURITY_BYTES)" in kem
    assert "prng_get_bytes(m, PARAM_SECURITY_BYTES)" in kem
    assert "prng_get_bytes(c_kem_t.salt, SALT_BYTES)" in kem

    calls = []
    for path in ROOT.rglob("*.c"):
        source = path.read_text(errors="replace")
        for line_no, line in enumerate(source.splitlines(), 1):
            if re.match(r"\s*(?:void\s+)?prng_init\s*\(", line):
                calls.append((path.relative_to(ROOT).as_posix(), line_no, line.strip()))
    assert any(p == "src/common/symmetric.c" and line.startswith("void prng_init") for p, _, line in calls)
    actual_call_paths = {p for p, _, line in calls if not line.startswith("void prng_init")}
    assert actual_call_paths == {"benchmark/benchmark_kem.c", "tests/qube-test.c"}
    assert not any("prng_init" in line and not line.lstrip().startswith("//") for line in wrapper.splitlines())

    probe = r'''
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "symmetric.h"
int main(int argc, char **argv) {
    uint8_t out[32], seed[32];
    if (argc > 1) {
        memset(seed, argv[1][0], sizeof seed);
        prng_init(seed, NULL, sizeof seed, 0);
    }
    prng_get_bytes(out, sizeof out);
    for (size_t i = 0; i < sizeof out; i++) printf("%02x", out[i]);
    puts("");
    return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix="qube-prng-") as tmp:
        tmp = Path(tmp)
        src = tmp / "probe.c"
        exe = tmp / "probe"
        src.write_text(probe)
        subprocess.run([
            "cc", "-std=c99", "-O2", "-I", str(ROOT / "src/common"),
            "-I", str(ROOT / "src/x86"), "-I", str(ROOT / "src/x86/qube-1"),
            "-I", str(ROOT / "lib/fips202"), str(src),
            str(ROOT / "src/common/symmetric.c"),
            str(ROOT / "lib/fips202/fips202.c"), "-o", str(exe),
        ], check=True)
        raw1 = subprocess.check_output([exe], text=True)
        raw2 = subprocess.check_output([exe], text=True)
        seeded_a = subprocess.check_output([exe, "A"], text=True)
        seeded_b = subprocess.check_output([exe, "B"], text=True)
        assert raw1 == raw2
        assert seeded_a != seeded_b and raw1 not in (seeded_a, seeded_b)

    print("QUBE optimized keygen/encapsulation consume the zero-initialized, never-seeded SHAKE stream")
    print("control: explicit distinct seeds produce distinct PRNG output")
    print("ATTACK kem-33-3 CONFIRMED: optimized API randomness is publicly reproducible")


if __name__ == "__main__":
    main()
