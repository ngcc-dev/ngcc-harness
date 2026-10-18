#!/usr/bin/env python3
"""Reproduce the source-level evidence for SDitH-1."""

import argparse
import hashlib
import json
from pathlib import Path


HERE = Path(__file__).resolve().parent
REF = HERE / "submission/submission_package/Reference_Implementation"
OPT = HERE / "submission/submission_package/Optimized_Implementation"
TABLE_SHA256 = "20e926127ce2b6ee0e53edbe8efdbd8f744ba8a1fdaaff3d778218e0a3e46607"
CT_UTILS_SHA256 = "eebc30587162374c4070be707ab6d0f07e2af5d3f23ae673144659728b5b43ed"
PRNG_SHA256 = "aa8b9d804759c70cf2f26391daa02b6a97e8c235b81af208cd6c86e473758322"
VOLE_SHA256 = "b863e22da0eade81b8184425d9b658fe4afc6dd890766a184ce2d7dfdc33b1f3"
SIGNATURE_SHA256 = "99685ad55272b44d5cc6f7ae20b4822d168a1bf75a8f9fcbb2e0e1f2d8dd7aec"


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    tables = sorted(REF.glob("*/lib/aes/aes_ansi_littleendian_ref.c"))
    utils = sorted(REF.glob("*/src/ct_utils.c"))
    prngs = sorted(REF.glob("*/src/sdith_prng.c"))
    voles = sorted(REF.glob("*/src/vole_expansion.c"))
    signatures = sorted(REF.glob("*/src/sdith_signature.c"))
    makefile = (HERE / "Makefile").read_text()
    checks = {
        "12_reference_variants": len(tables) == len(utils) == len(prngs) == len(voles) == len(signatures) == 12,
        "table_sources_pinned": bool(tables) and all(sha(path) == TABLE_SHA256 for path in tables),
        "secret_indexed_cipher_tables": bool(tables) and all(
            "Te0[s0 & 0xff]" in path.read_text() and "Te4[(temp >> 24)" in path.read_text()
            for path in tables
        ),
        "secret_seed_used_as_cipher_key": bool(prngs) and all(
            "aes128_key_schedule_x1_ref(s->rk, sk_seed)" in path.read_text()
            and "rijndael256_key_schedule_x1_ref(s->rk, sk_seed)" in path.read_text()
            for path in prngs
        ),
        "signing_root_seed_enters_prover_tree": bool(signatures) and all(
            sha(path) == SIGNATURE_SHA256
            and "memcpy(root_seed, ((uint8_t*)entropy) + par.lambda_bytes" in path.read_text()
            and "prover_generate_midsize_grey_vole_from_seeds_bfs_ct_ref" in path.read_text()
            for path in signatures
        ),
        "signing_tree_schedules_secret_nodes_and_leaves": bool(prngs) and bool(voles) and all(
            sha(path) == PRNG_SHA256
            and "aes128_key_schedule_x1_ref(extended_seed_out, seed128)" in path.read_text()
            and "ggm_seed_rng_lr_ext_cat1_aes128_ref" in path.read_text()
            for path in prngs
        ) and all(
            sha(path) == VOLE_SHA256
            and "vole_params->extend_leaf_seed_4x(out_ext_seed" in path.read_text()
            and "vole_params->ggm_extseed_rng_commit(out_commit" in path.read_text()
            and "vole_params->vole_extseed_rng(tmp_r" in path.read_text()
            for path in voles
        ),
        "secret_division_branch": bool(utils) and all(
            sha(path) == CT_UTILS_SHA256 and "if (rem >= public_d)" in path.read_text()
            for path in utils
        ),
        "harness_forces_reference_backend": makefile.count("-DONLY_REF_IMPLEMENTATION") == 12,
        "optimized_hardened_helpers_present": len(list(OPT.glob("*/src/ct_utils_avx2.c"))) == 12,
    }
    result = {
        "finding": "SDitH-1",
        "evidence": "source-level root-cause witness; no timing measurement or key recovery",
        "table_source_sha256": TABLE_SHA256,
        "ct_utils_sha256": CT_UTILS_SHA256,
        "prng_sha256": PRNG_SHA256,
        "vole_expansion_sha256": VOLE_SHA256,
        "signature_sha256": SIGNATURE_SHA256,
        "checks": checks,
        "status": "PASS" if all(checks.values()) else "FAIL",
    }
    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(encoded)
    print(encoded, end="")
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
