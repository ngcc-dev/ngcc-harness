#!/usr/bin/env python3
"""Check sign-30-3: TRINE-512's message-first SHAKE256 transcript in normal builds."""

from pathlib import Path


ROOT = Path(__file__).resolve().parent / "Implementations/Reference_Implementation"


def require(condition: bool, detail: str) -> None:
    if not condition:
        raise AssertionError(detail)


for variant in ("balanced", "ShortSig"):
    instance = ROOT / f"TRINE-512-{variant}"
    build = (instance / "Makefile").read_text()
    params = (instance / "params.h").read_text()
    source = (instance / "trine.c").read_text()
    hashes = (instance / "hashkdf.c").read_text()

    require("NORMAL_CFLAGS := $(COMMON_CFLAGS) -DUSE_SHA3" in build, f"{variant}: normal backend")
    require(params.count("#define TRINE_lambda 512") >= 1, f"{variant}: claimed level")
    init = "trine_hash_init(&transcript)"
    message = "trine_hash_absorb(&transcript, m, message_len)"
    commitment = "trine_absorb_canonical_form(&transcript, encoded_psi, psi)"
    require(source.count(init) == 2, f"{variant}: signer/verifier transcripts")
    for i, start in enumerate(pos for pos in range(len(source)) if source.startswith(init, pos)):
        end = source.find(init, start + 1)
        section = source[start:end if end >= 0 else None]
        require(section.find(message) > 0, f"{variant}: transcript {i} message")
        require(section.find(commitment) > section.find(message),
                f"{variant}: transcript {i} message-first order")
    hash_section = hashes[hashes.index("int trine_hash_init("):hashes.index("void trine_hash_release(")]
    require("shake256_init(&state->shake)" in hash_section, f"{variant}: SHAKE init")
    require("shake256_absorb(&state->shake, input, input_len)" in hash_section,
            f"{variant}: SHAKE absorb")
    require("shake256_finalize(&state->shake)" in hash_section, f"{variant}: SHAKE finalize")
    print(f"PASS TRINE-512-{variant}: message precedes commitments in SHAKE256 transcript")

print("Bound: about 2^256 SHAKE256 calls, one signing query; search not executed")
