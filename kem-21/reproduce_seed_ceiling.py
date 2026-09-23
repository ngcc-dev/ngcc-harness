#!/usr/bin/env python3
"""Static data-flow check for the Viper-384/512 secret-seed ceiling."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parent


def require(pattern: str, source: str, label: str) -> None:
    if not re.search(pattern, source, re.S):
        raise AssertionError(f"missing {label}: {pattern}")


for level in (384, 512):
    ref = ROOT / "Implementations" / "Reference_Implementation" / f"MAMBA-Viper-{level}"
    kem = (ref / "kem.c").read_text()
    core = (ref / "viper.c").read_text()
    require(r"unsigned char rho\[32\], sseed\[32\]", kem, f"{level} secret-seed declaration")
    require(r"randombytes\(sseed,\s*32\)", kem, f"{level} secret-seed draw")
    require(r"viper_pke_keypair\(pk,\s*sk,\s*rho,\s*sseed\)", kem, f"{level} keygen call")
    require(r"void viper_pke_keypair\([^)]*sseed\[32\]\).*?viper_sample_secret\(s,\s*sseed,\s*VIPER_ETA_S\)",
            core, f"{level} sole secret sampler")
    require(r"void viper_sample_secret\([^)]*seed\[32\].*?shake256\(buf,\s*sizeof\(buf\),\s*seed,\s*32\)",
            core, f"{level} 32-byte expansion")
    print(f"CONFIRMED kem-21-1 Viper-{level}: s is determined by a 256-bit seed and public rho")
