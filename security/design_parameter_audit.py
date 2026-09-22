#!/usr/bin/env python3
"""Cheap, reproducible design/parameter checks for the NGCC submissions.

This is deliberately not a cryptanalytic estimator.  It records elementary
ceilings and deterministic relations which can be checked without trusting a
candidate's KATs: digest birthday bounds, cross-variant output prefixes, and a
small registry of specification/source parameter mismatches.
"""
from __future__ import annotations

import argparse
import ctypes
import itertools
import json
import math
import re
import subprocess
from dataclasses import asdict, dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MESSAGES = (
    ("empty", b"", 0),
    ("a", b"a", 8),
    ("abc", b"abc", 24),
    ("partial-777", bytes(range(128)), 777),
)


@dataclass
class Finding:
    check: str
    candidate: str
    classification: str
    status: str
    detail: str
    evidence: list[str]
    report_id: str = ""


REPORT_IDS = {
    "vdoo-level5-salt-proof-bound": "sign-33-3",
    "compass512-seed-and-shared-secret-capacity": "kem-11-1",
    "hep-qc-7-seed-and-shared-secret-capacity": "kem-17-2",
    "ctl512-public-key-seed-capacity": "kem-12-1",
    "ctl512-output-capacity": "kem-12-2",
    "vdoo-level5-message-prehash": "sign-33-2",
    "atlas192-challenge-cardinality": "sign-15-3",
    "bit512-message-representative-collision": "sign-02-1",
    "origami512-message-prehash-collision": "sign-18-1",
    "tsuov512-message-prehash-collision": "sign-31-1",
    "compass-sig512-message-representative-collision": "sign-06-1",
    "compass-sig512-key-seed-capacity": "sign-06-2",
    "darts512-message-representative-collision": "sign-08-1",
    "rhyme512-message-representative-collision": "sign-22-1",
    "rhyme512-key-seed-capacity": "sign-22-2",
    "mamba-nike512-secret-seed-capacity": "kex-06-1",
    "mito-e-erasure-decoder-discarded": "kem-23-1",
    "trike-specified-max-decoder-nonfunctional": "kem-36-1",
    "uvw512-h1-seed-capacity": "kem-38-1",
    "uvw-distinct-decapsulation-failure-oracle": "kem-38-2",
}

# Cross-variant prefix relations are sampled by hash_checks() below and also
# have runtime witnesses in tools/reproduce.sh.
CROSS_VARIANT_REPORT_IDS = ("hash-18-1", "hash-20-1")


def metadata() -> list[dict[str, str]]:
    rows: list[dict[str, str]] = []
    for library in sorted(ROOT.glob("*/lib/*.so")):
        proc = subprocess.run(
            [str(ROOT / "bin/ngcc_kat"), "--meta-only", str(library)],
            cwd=ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            check=False,
        )
        if proc.returncode:
            continue
        parsed: dict[str, str] = {"library": str(library.relative_to(ROOT))}
        for line in proc.stdout.splitlines():
            if "=" in line:
                key, value = line.split("=", 1)
                parsed[key.strip()] = value.strip()
        kind = parsed.get("type", "")
        row = {
            "candidate": library.parts[-3],
            "type": kind,
            "algorithm": parsed.get("algorithm", ""),
            "instance": parsed.get("instance", ""),
            "digest_bits": parsed.get("digest_bits", ""),
            "digest_len": parsed.get("digest_len", ""),
            "pk_len": parsed.get("pk_len", ""),
            "sk_len": parsed.get("sk_len", ""),
            "ct_len": parsed.get("ct_len", ""),
            "ss_len": parsed.get("ss_len", ""),
            "sn_len": parsed.get("sn_len", ""),
            "library": str(library.relative_to(ROOT)),
        }
        rows.append(row)
    return rows


def hash_once(row: dict[str, str], message: bytes, message_bits: int) -> bytes:
    library = ctypes.CDLL(str(ROOT / row["library"]))
    crypt_hash = library.CryptHash
    crypt_hash.argtypes = (
        ctypes.c_int,
        ctypes.POINTER(ctypes.c_ubyte),
        ctypes.c_ulonglong,
        ctypes.POINTER(ctypes.c_ubyte),
    )
    crypt_hash.restype = ctypes.c_int
    source = (ctypes.c_ubyte * max(1, len(message)))()
    for i, value in enumerate(message):
        source[i] = value
    output = (ctypes.c_ubyte * int(row["digest_len"]))()
    result = crypt_hash(int(row["digest_bits"]), source, message_bits, output)
    if result:
        raise RuntimeError(f"CryptHash returned {result}")
    return bytes(output)


def hash_checks(rows: list[dict[str, str]]) -> tuple[list[dict], list[Finding]]:
    ceilings: list[dict] = []
    groups: dict[str, list[dict[str, str]]] = {}
    for row in rows:
        if row["type"] != "hash":
            continue
        bits = int(row["digest_bits"])
        ceilings.append(
            {
                "candidate": row["candidate"],
                "instance": row["instance"],
                "digest_bits": bits,
                "generic_collision_bits_at_most": bits / 2,
                "generic_preimage_bits_at_most": bits,
            }
        )
        groups.setdefault(row["candidate"], []).append(row)

    findings: list[Finding] = []
    for candidate, variants in sorted(groups.items()):
        outputs: dict[str, list[bytes]] = {}
        try:
            for variant in variants:
                outputs[variant["instance"]] = [
                    hash_once(variant, message, bits)
                    for _, message, bits in MESSAGES
                ]
        except (OSError, RuntimeError) as exc:
            findings.append(
                Finding(
                    "hash-cross-variant-prefix",
                    candidate,
                    "test_error",
                    "error",
                    str(exc),
                    [v["library"] for v in variants],
                )
            )
            continue

        for left, right in itertools.combinations(variants, 2):
            left_len, right_len = int(left["digest_len"]), int(right["digest_len"])
            if left_len <= right_len:
                short, long = left, right
            else:
                short, long = right, left
            prefix_len = min(left_len, right_len)
            if all(
                outputs[short["instance"]][i]
                == outputs[long["instance"]][i][:prefix_len]
                for i in range(len(MESSAGES))
            ):
                relation = "equality" if left_len == right_len else "prefix"
                findings.append(
                    Finding(
                        "hash-cross-variant-prefix",
                        candidate,
                        "deterministic_relation",
                        "confirmed_on_samples",
                        f"{short['instance']} is a {prefix_len}-byte {relation} "
                        f"of {long['instance']} for all {len(MESSAGES)} test messages",
                        [short["library"], long["library"]],
                    )
                )
    return ceilings, findings


def shared_secret_inventory(rows: list[dict[str, str]]) -> list[dict]:
    """Record delivered-key capacity without treating it as an IND-CCA break."""
    inventory: list[dict] = []
    for row in rows:
        if row["type"] not in ("kem", "kex") or not row["ss_len"]:
            continue
        size = int(row["ss_len"])
        inventory.append(
            {
                "candidate": row["candidate"],
                "type": row["type"],
                "instance": row["instance"],
                "shared_secret_bytes": size,
                "delivered_key_capacity_bits_at_most": 8 * size,
                "library": row["library"],
            }
        )
    return inventory


def extract_pdf(path: Path) -> str:
    proc = subprocess.run(
        ["pdftotext", "-layout", str(path), "-"],
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        check=False,
    )
    return proc.stdout.decode("utf-8", "replace")


def registered_checks() -> list[Finding]:
    findings: list[Finding] = []

    # VDOO: the salt and proof term are both normative specification text.
    vdoo_spec_path = ROOT / "sign-33/sign-33-spec.pdf"
    vdoo_spec = extract_pdf(vdoo_spec_path)
    has_levels = all(
        re.search(pattern, vdoo_spec, re.I | re.S)
        for pattern in (r"VDOO-256.*256-bit classical", r"VDOO-512.*512-bit classical")
    )
    has_salt = bool(re.search(r"fix the salt length at 16 bytes", vdoo_spec, re.I))
    has_bound = "2−|salt|" in vdoo_spec or "2-|salt|" in vdoo_spec
    findings.append(
        Finding(
            "vdoo-level5-salt-proof-bound",
            "sign-33",
            "specification_parameter_and_proof_gap",
            "confirmed" if has_levels and has_salt and has_bound else "not_reproduced",
            "VDOO-256 and VDOO-512 claim 256 and 512 classical bits, but the "
            "specification fixes a 128-bit salt and its own EUF-CMA bound contains "
            "(q_s+q_h)q_s*2^-128.  At q_s=q_h=1 this proof term is "
            "2^-127; at q_s=2^64 it is already order one.  This makes the "
            "stated proof incapable of substantiating either EUF-CMA claim; "
            "it is not by itself a concrete forgery.",
            ["sign-33/sign-33-spec.pdf (physical pages 19-20)"],
        )
    )

    # COMPASS-KEM-512: its specification scales all n-bit values to 512 bits,
    # but the implementation retains the 32-byte constants from lower levels.
    compass_params_path = ROOT / (
        "kem-11/Implementations and Test_Vectors/Implementations/"
        "Reference_Implementation/COMPASS-KEM-512/params.h"
    )
    compass384_dir = compass_params_path.parent.parent / "COMPASS-KEM-384"
    compass384_params_path = compass384_dir / "params.h"
    compass384_indcpa_path = compass384_dir / "indcpa.c"
    compass384_kem_path = compass384_dir / "kem.c"
    compass_indcpa_path = compass_params_path.with_name("indcpa.c")
    compass_kem_path = compass_params_path.with_name("kem.c")
    compass_params = compass_params_path.read_text(encoding="utf-8", errors="replace")
    compass_indcpa = compass_indcpa_path.read_text(encoding="utf-8", errors="replace")
    compass_kem = compass_kem_path.read_text(encoding="utf-8", errors="replace")
    compass384_params = compass384_params_path.read_text(encoding="utf-8", errors="replace")
    compass384_indcpa = compass384_indcpa_path.read_text(encoding="utf-8", errors="replace")
    compass384_kem = compass384_kem_path.read_text(encoding="utf-8", errors="replace")
    compass_reproduced = all(
        (
            re.search(r"#define\s+COMPASS_KEM_N\s+512\b", compass_params),
            re.search(r"#define\s+COMPASS_KEM_SYMBYTES\s+32\b", compass_params),
            re.search(r"#define\s+COMPASS_KEM_SSBYTES\s+32\b", compass_params),
            "indcpa_keypair_derand(pk, sk, coins)" in compass_kem,
            "memcpy(buf, coins, COMPASS_KEM_SYMBYTES)" in compass_indcpa,
            re.search(r"#define\s+COMPASS_KEM_SYMBYTES\s+32\b", compass384_params),
            re.search(r"#define\s+COMPASS_KEM_SSBYTES\s+32\b", compass384_params),
            "indcpa_keypair_derand(pk, sk, coins)" in compass384_kem,
            "memcpy(buf, coins, COMPASS_KEM_SYMBYTES)" in compass384_indcpa,
            re.search(r"32-byte core seed", extract_pdf(ROOT / "kem-11/kem-11-spec.pdf"), re.I),
        )
    )
    findings.append(
        Finding(
            "compass512-seed-and-shared-secret-capacity",
            "kem-11",
            "implementation_specification_conformance_break",
            "confirmed" if compass_reproduced else "not_reproduced",
            "The PDF specifies an n-bit initial key-generation seed and K in "
            "B^(n/8), but its implementation notes also mention a 32-byte core "
            "seed.  The 384- and 512-bit implementations set both SYMBYTES and "
            "SSBYTES to 32.  Each public/secret IND-CPA "
            "keypair is a deterministic function of the first 256-bit coins "
            "value, so exhaustive seed enumeration recovers it in at most "
            "2^256 trials; the delivered shared key is also only 256 bits.",
            [
                "kem-11/kem-11-spec.pdf (physical pages 9, 12, 16)",
                str(compass_params_path.relative_to(ROOT)),
                str(compass_indcpa_path.relative_to(ROOT)),
                str(compass_kem_path.relative_to(ROOT)),
                str(compass384_params_path.relative_to(ROOT)),
                str(compass384_indcpa_path.relative_to(ROOT)),
                str(compass384_kem_path.relative_to(ROOT)),
            ],
        )
    )

    # HEP-QC-7: unlike COMPASS, both short values are explicit in the PDF.
    hep_spec_path = ROOT / "kem-17/kem-17-spec.pdf"
    hep_spec = extract_pdf(hep_spec_path)
    hep_reproduced = all(
        re.search(pattern, hep_spec, re.I | re.S)
        for pattern in (
            r"HEP-QC-7\s+512",
            r"\|seed\|\s*=\s*32\s*B",
            r"\|K\|\s*=\s*32\s*B",
            r"seedKEM\s*\)\s*.*?B\s*\|seed\|",
        )
    )
    findings.append(
        Finding(
            "hep-qc-7-seed-and-shared-secret-capacity",
            "kem-17",
            "specification_design_break",
            "confirmed" if hep_reproduced else "not_reproduced",
            "The specification claims 512-bit classical security for HEP-QC-7 "
            "but fixes every seed and the shared key K at 32 bytes.  KEM.KeyGen "
            "samples one 256-bit seedKEM and deterministically derives seedPKE "
            "and the complete PKE keypair.  Thus there are at most 2^256 public "
            "keys: enumerate seedKEM, regenerate ekKEM, and match the target "
            "public key to recover its decapsulation key.  The delivered key "
            "capacity is independently at most 256 bits.",
            ["kem-17/kem-17-spec.pdf (physical pages 12-14)"],
        )
    )

    # CTL-512: the compact-key format makes the public-key support explicit.
    ctl_spec_path = ROOT / "kem-12/kem-12-spec.pdf"
    ctl_spec = extract_pdf(ctl_spec_path)
    ctl_source_dir = ROOT / (
        "kem-12/Implementations and Test_Vectors/"
        "KEM-CTL-x86-Reference_Implementation/crypto_kem/Implementations/"
        "CTL-3329-2048"
    )
    ctl_header_path = ctl_source_dir / "ctl.h"
    ctl_api_path = ctl_source_dir / "api.c"
    ctl_header = ctl_header_path.read_text(encoding="utf-8", errors="replace")
    ctl_api = ctl_api_path.read_text(encoding="utf-8", errors="replace")
    ctl_reproduced = all(
        (
            re.search(r"CTL[_-]512\s+2048", ctl_spec, re.I),
            re.search(r"Only the 32-byte key generation random seed", ctl_spec, re.I),
            re.search(r"Regenerated from the 32-byte seed", ctl_spec, re.I),
            re.search(r"Public key.*Re-computed according", ctl_spec, re.I | re.S),
            re.search(r"uint8_t seed\[32\]", ctl_header),
            "ctl_keygen_make_fg(sk->f, sk->g" in ctl_api,
        )
    )
    findings.append(
        Finding(
            "ctl512-public-key-seed-capacity",
            "kem-12",
            "specification_design_break",
            "confirmed" if ctl_reproduced else "not_reproduced",
            "The specification claims 512-bit security for CTL-512, but its "
            "short private-key format stores a 32-byte key-generation seed and "
            "requires f and g to be regenerated deterministically from it; the "
            "public key is then recomputed as h=f^-1*g mod q.  Consequently the "
            "public-key support has size at most 2^256.  Enumerating seeds and "
            "matching h identifies f and g for the target public key in at most "
            "2^256 trials, irrespective of the nominal lattice parameters.  The "
            "submitted source implements the same 32-byte seed construction.",
            [
                "kem-12/kem-12-spec.pdf (physical pages 24, 28-30, 35-36)",
                str(ctl_header_path.relative_to(ROOT)),
                str(ctl_api_path.relative_to(ROOT)),
            ],
        )
    )

    ctl_adapter_path = ctl_source_dir / "KEM_AlgorithmInstance.c"
    ctl_adapter = ctl_adapter_path.read_text(encoding="utf-8", errors="replace")
    ctl_output_reproduced = all(
        (
            re.search(r"CTL_MK\(3329,\s*2048,\s*48,", ctl_header),
            "*ss_len_bytes = 48;" in ctl_adapter,
            re.search(r"64 bytes \(512-\s*bit security\) for CTL-512", ctl_spec, re.I),
        )
    )
    findings.append(
        Finding(
            "ctl512-output-capacity",
            "kem-12",
            "implementation_specification_conformance_break",
            "confirmed" if ctl_output_reproduced else "not_reproduced",
            "The CTL-512 adapter returns a 48-byte shared secret, so its "
            "delivered-key capacity is at most 384 bits.  It also instantiates "
            "the ciphertext hash component c2 at 48 bytes, whereas the PDF "
            "explicitly assigns 64 bytes to CTL-512.  Output length alone is "
            "not an IND-CCA attack, but it cannot deliver the claimed 512-bit "
            "key capacity and it contradicts the specified ciphertext format.",
            [
                "kem-12/kem-12-spec.pdf (physical pages 27-30)",
                str(ctl_header_path.relative_to(ROOT)),
                str(ctl_adapter_path.relative_to(ROOT)),
            ],
        )
    )

    # VDOO: unlike the salt, the 32-byte inner digest is a source-level choice.
    config_path = ROOT / "sign-33/Implementation/Reference_Implementation/vdoo_512/vdoo_config.h"
    api_path = ROOT / "sign-33/Implementation/Reference_Implementation/vdoo_512/api.c"
    config256_path = ROOT / "sign-33/Implementation/Reference_Implementation/vdoo_256/vdoo_config.h"
    api256_path = ROOT / "sign-33/Implementation/Reference_Implementation/vdoo_256/api.c"
    config = config_path.read_text(encoding="utf-8", errors="replace")
    api = api_path.read_text(encoding="utf-8", errors="replace")
    config256 = config256_path.read_text(encoding="utf-8", errors="replace")
    api256 = api256_path.read_text(encoding="utf-8", errors="replace")
    prehash_reproduced = all(
        (
            re.search(r"#define\s+HASH_LEN\s+32\b", config),
            re.search(r"#define\s+HASH_LEN\s+32\b", config256),
            all(token in api for token in ("digest[HASH_LEN]", "hash_msg(digest, HASH_LEN")),
            all(token in api256 for token in ("digest[HASH_LEN]", "hash_msg(digest, HASH_LEN")),
        )
    )
    findings.append(
        Finding(
            "vdoo-level5-message-prehash",
            "sign-33",
            "implementation_specification_conformance_break",
            "confirmed" if prehash_reproduced else "not_reproduced",
            "VDOO-256 and VDOO-512 reduce each message to a 256-bit inner digest before "
            "signing.  A generic collision in that digest (about 2^128 work) "
            "transfers a signature between the colliding messages.  Algorithm 4 "
            "instead types H as {0,1}* -> F_q^m, so the 32-byte truncation is "
            "not classified as a clean specification-level design parameter.",
            [
                str(config_path.relative_to(ROOT)),
                str(api_path.relative_to(ROOT)),
                str(config256_path.relative_to(ROOT)),
                str(api256_path.relative_to(ROOT)),
                "sign-33/sign-33-spec.pdf (physical pages 8-9, 21-23)",
            ],
        )
    )

    # MORNING-ATLAS-192: an elementary exact challenge-space calculation.
    atlas_path = ROOT / "sign-15/Implementation/Reference_Implementation/lwrdsa192/params.h"
    atlas_source = atlas_path.read_text(encoding="utf-8", errors="replace")
    atlas_bits = math.log2(math.comb(128, 64)) + 64
    atlas_reproduced = bool(re.search(r"#define\s+KAPPA\s+64U?\b", atlas_source))
    findings.append(
        Finding(
            "atlas192-challenge-cardinality",
            "sign-15",
            "implementation_specification_conformance_break",
            "confirmed" if atlas_reproduced and atlas_bits < 192 else "not_reproduced",
            f"The implementation uses n=128 and kappa=64, giving "
            f"log2(C(128,64)*2^64)={atlas_bits:.5f} challenge bits.  The "
            "specification selects kappa=69 (about 192.61 bits), so this is "
            "implementation-only rather than a design break.",
            [
                str(atlas_path.relative_to(ROOT)),
                "sign-15/sign-15-spec.pdf (physical pages 21-22)",
            ],
        )
    )

    # A plain, unsalted n-bit message representative provides only n/2 bits of
    # collision resistance: collide two messages, request a signature on one,
    # and transfer it to the other.  These 512-bit parameter sets make
    # that 512-bit representative explicit while claiming 512 classical bits.
    bit_spec_path = ROOT / "sign-02/sign-02-spec.pdf"
    bit_spec = extract_pdf(bit_spec_path)
    bit_params_path = ROOT / "sign-02/Implementations/Reference_Implementation/BiT-512/params.h"
    bit_sign_path = bit_params_path.with_name("sign.c")
    bit_params = bit_params_path.read_text(encoding="utf-8", errors="replace")
    bit_sign = bit_sign_path.read_text(encoding="utf-8", errors="replace")
    bit_reproduced = all(
        (
            re.search(r"Security Level.*512", bit_spec, re.I | re.S),
            re.search(r"µ\s*∈\s*\{0,\s*1\}κ\s*:=\s*H\(tr\|\|M", bit_spec),
            re.search(r"#define\s+BIT_MESSAGEBYTES\s+64\b", bit_params),
            "bit_h512_2(message_hash" in bit_sign,
        )
    )
    findings.append(
        Finding(
            "bit512-message-representative-collision",
            "sign-02",
            "specification_design_break",
            "confirmed" if bit_reproduced else "not_reproduced",
            "BiT-512 claims 512-bit classical security and specifies the "
            "message representative mu=H(tr||M) as a kappa-bit value, with "
            "kappa=512.  A generic collision in this unsalted 512-bit "
            "representative costs about 2^256 evaluations.  Requesting a "
            "signature on one colliding message yields a valid signature on "
            "the other.  The source implements the representative as 64 bytes.",
            [
                "sign-02/sign-02-spec.pdf (physical pages 18, 33-34)",
                str(bit_params_path.relative_to(ROOT)),
                str(bit_sign_path.relative_to(ROOT)),
            ],
        )
    )

    origami_spec_path = ROOT / "sign-18/sign-18-spec.pdf"
    origami_spec = extract_pdf(origami_spec_path)
    origami_dir = ROOT / (
        "sign-18/Implementations and Test_Vectors/Implementations/"
        "Reference_Implementation/Origami-512"
    )
    origami_header_path = origami_dir / "origami.h"
    origami_adapter_path = origami_dir / "SIG_AlgorithmInstance.c"
    origami384_dir = origami_dir.parent / "Origami-384"
    origami384_header_path = origami384_dir / "origami.h"
    origami384_adapter_path = origami384_dir / "SIG_AlgorithmInstance.c"
    origami_header = origami_header_path.read_text(encoding="utf-8", errors="replace")
    origami_adapter = origami_adapter_path.read_text(encoding="utf-8", errors="replace")
    origami384_header = origami384_header_path.read_text(encoding="utf-8", errors="replace")
    origami384_adapter = origami384_adapter_path.read_text(encoding="utf-8", errors="replace")
    origami_reproduced = all(
        (
            re.search(r"Origami-512.*512-bit", origami_spec, re.I | re.S),
            re.search(r"fixed-length message\s+digest", origami_spec, re.I),
            re.search(r"Hmsg\s*\(µ\).*?64-byte", origami_spec, re.I | re.S),
            re.search(r"target.*hµ.*salt", origami_spec, re.I),
            re.search(r"#define\s+BYTES_DIGEST\s+64\b", origami_header),
            "pseudohash(BYTES_DIGEST * 8, m" in origami_adapter,
            re.search(r"Origami-384.*384", origami_spec, re.I | re.S),
            re.search(r"#define\s+BYTES_DIGEST\s+64\b", origami384_header),
            "pseudohash(BYTES_DIGEST * 8, m" in origami384_adapter,
        )
    )
    findings.append(
        Finding(
            "origami512-message-prehash-collision",
            "sign-18",
            "specification_design_break",
            "confirmed" if origami_reproduced else "not_reproduced",
            "Origami-384 and Origami-512 claim 384- and 512-bit classical "
            "security but first compress each message with the same fixed "
            "64-byte H_msg, then derive the salted "
            "target from target||H_msg(message)||salt.  A roughly 2^256 generic "
            "collision in H_msg therefore transfers a requested signature from "
            "one colliding message to the other; the later salt does not repair "
            "an already-colliding prehash.  This construction and length are "
            "explicit in both the PDF and source.",
            [
                "sign-18/sign-18-spec.pdf (physical pages 14-16, 50-52)",
                str(origami_header_path.relative_to(ROOT)),
                str(origami_adapter_path.relative_to(ROOT)),
                str(origami384_header_path.relative_to(ROOT)),
                str(origami384_adapter_path.relative_to(ROOT)),
            ],
        )
    )

    tsuov_spec_path = ROOT / "sign-31/sign-31-spec.pdf"
    tsuov_spec = extract_pdf(tsuov_spec_path)
    tsuov_dir = ROOT / (
        "sign-31/Implementations/Digital_Signature-TSUOV-x86-Reference_Implementation/"
        "API_PKC/Implementations/Reference_Implementation/TSUOV_512"
    )
    tsuov_params_path = tsuov_dir / "tsuov_params.h"
    tsuov_params = tsuov_params_path.read_text(encoding="utf-8", errors="replace")
    tsuov_reproduced = all(
        (
            re.search(r"TSUOV\s+512.*ICCS level.*512", tsuov_spec, re.I | re.S),
            re.search(r"Expandµ.*pseudohash\(512", tsuov_spec, re.I | re.S),
            re.search(r"Hash\(µ\|\|salt\)", tsuov_spec),
            re.search(r"#define\s+TSUOV_MU_LEN\s+64\b", tsuov_params),
        )
    )
    findings.append(
        Finding(
            "tsuov512-message-prehash-collision",
            "sign-31",
            "specification_design_break",
            "confirmed" if tsuov_reproduced else "not_reproduced",
            "TSUOV-512 specifies mu=Expand_mu(seed_pk||M) as a 512-bit "
            "pseudohash and only afterward hashes mu||salt to the MQ target.  "
            "A generic collision in Expand_mu costs about 2^256 and transfers "
            "a signature across the colliding messages for the signature's "
            "same salt.  The PDF's own EUF-CMA bound includes a digest-"
            "collision term but does not account for this concrete birthday "
            "ceiling when claiming 512-bit classical security.",
            [
                "sign-31/sign-31-spec.pdf (physical pages 18-21, 27-29)",
                str(tsuov_params_path.relative_to(ROOT)),
            ],
        )
    )

    # COMPASS-SIG has both the same 512-bit message-binding ceiling in the
    # normative design and a separate, shorter implementation root seed.
    compass_sig_spec_path = ROOT / "sign-06/sign-06-spec.pdf"
    compass_sig_spec = extract_pdf(compass_sig_spec_path)
    compass_sig_dir = ROOT / "sign-06/Implementations/Reference_Implementation/COMPASS-SIG-512"
    compass_sig_params_path = compass_sig_dir / "params.h"
    compass_sig_sign_path = compass_sig_dir / "sign.c"
    compass_sig384_dir = compass_sig_dir.parent / "COMPASS-SIG-384"
    compass_sig384_params_path = compass_sig384_dir / "params.h"
    compass_sig384_sign_path = compass_sig384_dir / "sign.c"
    compass_sig_params = compass_sig_params_path.read_text(encoding="utf-8", errors="replace")
    compass_sig_sign = compass_sig_sign_path.read_text(encoding="utf-8", errors="replace")
    compass_sig384_params = compass_sig384_params_path.read_text(encoding="utf-8", errors="replace")
    compass_sig384_sign = compass_sig384_sign_path.read_text(encoding="utf-8", errors="replace")
    compass_sig_hash_reproduced = all(
        (
            re.search(r"512-bit.*security", compass_sig_spec, re.I | re.S),
            re.search(r"µ\s*=\s*H\(pk.*?m\)", compass_sig_spec, re.I | re.S),
            re.search(r"#define\s+CRHBYTES\s+64\b", compass_sig_params),
            "shake256_squeeze(mu, CRHBYTES" in compass_sig_sign,
            re.search(r"#define\s+CRHBYTES\s+64\b", compass_sig384_params),
            "shake256_squeeze(mu, CRHBYTES" in compass_sig384_sign,
        )
    )
    findings.append(
        Finding(
            "compass-sig512-message-representative-collision",
            "sign-06",
            "implementation_security_ceiling_spec_length_underspecified",
            "confirmed" if compass_sig_hash_reproduced else "not_reproduced",
            "COMPASS-SIG specifies the unsalted value mu=H(pk||m) but not H's "
            "output length.  The 384- and 512-bit implementations instantiate "
            "mu as 64 bytes.  Thus a generic 2^256 "
            "collision-and-signature-transfer attack bounds classical EUF-CMA "
            "security below both classical claims, independent of the lattice "
            "parameters.  This is an implementation ceiling and specification omission.",
            [
                "sign-06/sign-06-spec.pdf (physical pages 5, 8-10, 13)",
                str(compass_sig_params_path.relative_to(ROOT)),
                str(compass_sig_sign_path.relative_to(ROOT)),
                str(compass_sig384_params_path.relative_to(ROOT)),
                str(compass_sig384_sign_path.relative_to(ROOT)),
            ],
        )
    )
    compass_sig_seed_reproduced = all(
        (
            re.search(r"KeyGen takes a n-bit.*random\s+seed", compass_sig_spec, re.I | re.S),
            re.search(r"#define\s+N\s+512\b", compass_sig_params),
            re.search(r"#define\s+SEEDBYTES\s+32\b", compass_sig_params),
            "get_random_number(&drng_algorithm, seed, SEEDBYTES * 8)" in compass_sig_sign,
            "shake256(seedbuf, 2*SEEDBYTES + CRHBYTES, seed, SEEDBYTES)" in compass_sig_sign,
            re.search(r"#define\s+SEEDBYTES\s+32\b", compass_sig384_params),
            "get_random_number(&drng_algorithm, seed, SEEDBYTES * 8)" in compass_sig384_sign,
            "shake256(seedbuf, 2*SEEDBYTES + CRHBYTES, seed, SEEDBYTES)" in compass_sig384_sign,
        )
    )
    findings.append(
        Finding(
            "compass-sig512-key-seed-capacity",
            "sign-06",
            "implementation_specification_conformance_break",
            "confirmed" if compass_sig_seed_reproduced else "not_reproduced",
            "The COMPASS-SIG PDF requires an n-bit KeyGen seed.  The 384- and "
            "512-bit implementations instead draw one 32-byte root "
            "and deterministically expand the entire keypair from it, limiting "
            "the public-key support to at most 2^256 and enabling generic seed "
            "enumeration in that many trials.",
            [
                "sign-06/sign-06-spec.pdf (physical pages 7, 13)",
                str(compass_sig_params_path.relative_to(ROOT)),
                str(compass_sig_sign_path.relative_to(ROOT)),
                str(compass_sig384_params_path.relative_to(ROOT)),
                str(compass_sig384_sign_path.relative_to(ROOT)),
            ],
        )
    )

    # The DARTS PDF names H1 but does not give its output type/length.  The
    # implementation makes the resulting DARTS-512 ceiling unambiguous.
    darts_spec_path = ROOT / "sign-08/sign-08-spec.pdf"
    darts_spec = extract_pdf(darts_spec_path)
    darts_dir = ROOT / "sign-08/Implementations/Reference_Implementation/DARTS512"
    darts_params_path = darts_dir / "params.h"
    darts_sign_path = darts_dir / "sign.c"
    darts_params = darts_params_path.read_text(encoding="utf-8", errors="replace")
    darts_sign = darts_sign_path.read_text(encoding="utf-8", errors="replace")
    darts_reproduced = all(
        (
            re.search(r"Security level.*512 bits", darts_spec, re.I | re.S),
            re.search(r"µ\s*←\s*H1\s*\(pk,\s*M", darts_spec),
            re.search(r"#define\s+CRHBYTES\s+64\b", darts_params),
            "sm3_xof_squeeze(mu, CRHBYTES" in darts_sign,
        )
    )
    findings.append(
        Finding(
            "darts512-message-representative-collision",
            "sign-08",
            "implementation_security_ceiling_spec_length_underspecified",
            "confirmed" if darts_reproduced else "not_reproduced",
            "DARTS-512 claims 512 classical bits and binds messages through "
            "mu=H1(pk,M), but its implementation emits only a 64-byte mu.  A "
            "generic collision and signature transfer therefore costs about "
            "2^256.  The PDF does not type H1's output length, so the concrete "
            "ceiling is confirmed while the short length is not classified as "
            "a clean normative parameter.",
            [
                "sign-08/sign-08-spec.pdf (physical pages 4, 8, 12-13)",
                str(darts_params_path.relative_to(ROOT)),
                str(darts_sign_path.relative_to(ROOT)),
            ],
        )
    )

    rhyme_spec_path = ROOT / "sign-22/sign-22-spec.pdf"
    rhyme_spec = extract_pdf(rhyme_spec_path)
    rhyme_dir = ROOT / (
        "sign-22/Implementations and Test_Vectors/Implementations/"
        "Reference_Implementation/Rhyme-SHAKE/Rhyme-SHAKE-512"
    )
    rhyme_params_path = rhyme_dir / "include/params.h"
    rhyme_sign_path = rhyme_dir / "src/sign.c"
    rhyme_params = rhyme_params_path.read_text(encoding="utf-8", errors="replace")
    rhyme_sign = rhyme_sign_path.read_text(encoding="utf-8", errors="replace")
    rhyme_root = rhyme_dir.parents[1]
    rhyme_extra_variants = [
        rhyme_root / family / f"{family}-{bits}"
        for family in ("Rhyme-SHAKE", "Rhyme-SM3")
        for bits in (384, 512)
        if (family, bits) != ("Rhyme-SHAKE", 512)
    ]
    rhyme_variant_sources = [
        (
            variant / "include/params.h",
            variant / "src/sign.c",
            (variant / "include/params.h").read_text(encoding="utf-8", errors="replace"),
            (variant / "src/sign.c").read_text(encoding="utf-8", errors="replace"),
        )
        for variant in rhyme_extra_variants
    ]
    rhyme_hash_reproduced = all(
        (
            re.search(r"512-bit classical security", rhyme_spec, re.I),
            re.search(r"µ\s*←\s*Hgen\s*\(seedA\s*,\s*bgen\s*,\s*M", rhyme_spec),
            re.search(r"#define\s+CRHBYTES\s+64\b", rhyme_params),
            "rhyme_shake256_squeeze(mu, CRHBYTES" in rhyme_sign,
            all(
                re.search(r"#define\s+CRHBYTES\s+64\b", params)
                and "rhyme_shake256_squeeze(mu, CRHBYTES" in source
                for _, _, params, source in rhyme_variant_sources
            ),
        )
    )
    findings.append(
        Finding(
            "rhyme512-message-representative-collision",
            "sign-22",
            "implementation_security_ceiling_spec_length_underspecified",
            "confirmed" if rhyme_hash_reproduced else "not_reproduced",
            "Rhyme's unsalted message binding is mu=H_gen(pk,M).  The SHAKE "
            "and SM3 384- and 512-bit implementations fix mu at 64 bytes, "
            "enabling generic collision-and-signature transfer in about "
            "2^256 work.  The algorithm is normative but the PDF does "
            "not define H_gen's output length, so this is not labeled a clean "
            "specification parameter break.",
            [
                "sign-22/sign-22-spec.pdf (physical pages 29-32, 50-51)",
                str(rhyme_params_path.relative_to(ROOT)),
                str(rhyme_sign_path.relative_to(ROOT)),
            ] + [str(path.relative_to(ROOT)) for pair in rhyme_variant_sources for path in pair[:2]],
        )
    )
    rhyme_seed_reproduced = all(
        (
            re.search(r"seed\s*←\s*\{0,\s*1\}ρ0", rhyme_spec),
            re.search(r"#define\s+SEEDBYTES\s+32\b", rhyme_params),
            "randombytes(root, SEEDBYTES)" in rhyme_sign,
            "rhyme_shake256_hash(seedbuf, sizeof seedbuf, root, SEEDBYTES)" in rhyme_sign,
            all(
                re.search(r"#define\s+SEEDBYTES\s+32\b", params)
                and "randombytes(root, SEEDBYTES)" in source
                and "rhyme_shake256_hash(seedbuf, sizeof seedbuf, root, SEEDBYTES)" in source
                for _, _, params, source in rhyme_variant_sources
            ),
        )
    )
    findings.append(
        Finding(
            "rhyme512-key-seed-capacity",
            "sign-22",
            "implementation_security_ceiling_spec_length_underspecified",
            "confirmed" if rhyme_seed_reproduced else "not_reproduced",
            "The SHAKE and SM3 Rhyme-384 and Rhyme-512 implementations expand "
            "each entire keypair from one 32-byte root, so "
            "the public-key support is at most 2^256 and a generic seed search "
            "can recover a target signing key in that many trials.  Algorithm 5 "
            "uses an abstract rho_0-bit root but never assigns rho_0 in the "
            "parameter table, making this simultaneously an implementation "
            "ceiling and a specification omission.",
            [
                "sign-22/sign-22-spec.pdf (physical pages 29, 50-51)",
                str(rhyme_params_path.relative_to(ROOT)),
                str(rhyme_sign_path.relative_to(ROOT)),
            ] + [str(path.relative_to(ROOT)) for pair in rhyme_variant_sources for path in pair[:2]],
        )
    )

    # MAMBA-NIKE samples the normative high-entropy CBD secret through a fixed
    # 256-bit implementation seed.  The public dither seed is public, so it does
    # not add uncertainty to a seed-enumeration attack on a known target key.
    nike_spec_path = ROOT / "kex-06/kex-06-spec.pdf"
    nike_spec = extract_pdf(nike_spec_path)
    nike_dir = ROOT / "kex-06/Implementations/Reference_Implementation/MAMBA-NIKE-512"
    nike_params_path = nike_dir / "params.h"
    nike_source_path = nike_dir / "nike.c"
    nike384_dir = nike_dir.parent / "MAMBA-NIKE-384"
    nike384_params_path = nike384_dir / "params.h"
    nike384_source_path = nike384_dir / "nike.c"
    nike_params = nike_params_path.read_text(encoding="utf-8", errors="replace")
    nike_source = nike_source_path.read_text(encoding="utf-8", errors="replace")
    nike384_params = nike384_params_path.read_text(encoding="utf-8", errors="replace")
    nike384_source = nike384_source_path.read_text(encoding="utf-8", errors="replace")
    nike_reproduced = all(
        (
            re.search(r"MAMBA-NIKE-512.*512-bit Classical Security", nike_spec, re.I | re.S),
            re.search(r"s\s*←\s*Bηn", nike_spec),
            re.search(r"#define\s+NIKE_SECURITY_BITS\s+512\b", nike_params),
            re.search(r"unsigned char noiseseed\[32\]", nike_source),
            "nike_randombytes(noiseseed, 32)" in nike_source,
            "poly_getnoise(sk,noiseseed,0)" in nike_source,
            re.search(r"#define\s+NIKE_SECURITY_BITS\s+384\b", nike384_params),
            re.search(r"unsigned char noiseseed\[32\]", nike384_source),
            "nike_randombytes(noiseseed, 32)" in nike384_source,
            "poly_getnoise(sk,noiseseed,0)" in nike384_source,
        )
    )
    findings.append(
        Finding(
            "mamba-nike512-secret-seed-capacity",
            "kex-06",
            "implementation_specification_conformance_break",
            "confirmed" if nike_reproduced else "not_reproduced",
            "MAMBA-NIKE-384 and MAMBA-NIKE-512 claim 384- and 512-bit passive "
            "KE security and normatively sample each static secret polynomial "
            "from the full CBD.  Both implementations instead generate that "
            "polynomial deterministically "
            "from a 32-byte noise seed.  For the public rho contained in a target "
            "key, enumerate 2^256 noise seeds, regenerate s and b, and match b to "
            "recover the static secret.  The independent rho seed is public and "
            "does not increase the attack exponent.",
            [
                "kex-06/kex-06-spec.pdf (physical pages 7-10, 12-15)",
                str(nike_params_path.relative_to(ROOT)),
                str(nike_source_path.relative_to(ROOT)),
                str(nike384_params_path.relative_to(ROOT)),
                str(nike384_source_path.relative_to(ROOT)),
            ],
        )
    )
    # Mito-E: the inner decoder emits erasure positions, but every submitted E
    # variant drops them before calling the ordinary errors-only RS decoder.
    mito_e_dirs = sorted((ROOT / "kem-23/Implementations").glob("*_Implementation/Mito-*-E-*"))
    mito_bad = []
    for directory in mito_e_dirs:
        code = (directory / "code.c").read_text(encoding="utf-8", errors="replace")
        rs_header = (directory / "reed_solomon.h").read_text(encoding="utf-8", errors="replace")
        if all(token in code for token in (
            "t = reed_muller_decode(tmp, pos, em);",
            "reed_solomon_decode(m, tmp);",
        )) and "reed_solomon_decode(uint64_t* msg, uint64_t* cdw)" in rs_header:
            mito_bad.append(directory.name)
    findings.append(
        Finding(
            "mito-e-erasure-decoder-discarded",
            "kem-23",
            "implementation_specification_conformance_break",
            "confirmed" if len(mito_e_dirs) in (6, 12) and len(mito_bad) == len(mito_e_dirs) else "not_reproduced",
            "Every included Mito-E implementation tree asks the modified RM decoder "
            "for the erasure count and positions, then discard both values and call "
            "the ordinary errors-only Reed-Solomon decoder.  The PDF defines Mito-E "
            "by errors-and-erasures decoding with correctness condition 2*nu+t <= "
            "N-K and uses that decoder in its E-variant DFR analysis.  Consequently "
            "the claimed E-variant DFRs do not apply to the shipped decapsulator. "
            "The public harness retains the six reference trees; the private archive audit "
            "also checked their six optimized counterparts.",
            [
                "kem-23/kem-23-spec.pdf (physical pages 17-20, 47-48)",
                *[str((d / "code.c").relative_to(ROOT)) for d in mito_e_dirs],
                *[str((d / "reed_solomon.h").relative_to(ROOT)) for d in mito_e_dirs],
            ],
        )
    )

    # TRIKE: the branchless expression implements min(fs,t), contradicting the
    # normative max rule.  A paired whole-KEM build is provided separately.
    trike_dirs = sorted((ROOT / "kem-36/Implementations and Test_Vectors/Implementations/Reference_Implementation").glob("TRIKE-*"))
    trike_min = []
    for directory in trike_dirs:
        decoder = (directory / "src/decoder.c").read_text(encoding="utf-8", errors="replace")
        if "uint32_t mask = -(fs < t);" in decoder and "return (t & ~mask) | (fs & mask);" in decoder:
            trike_min.append(directory.name)
    trike_spec = extract_pdf(ROOT / "kem-36/kem-36-spec.pdf")
    trike_has_max = "T = max (ca w′ + cb , T ′ )" in trike_spec and "return max(Tnow , T ′ )" in trike_spec
    findings.append(
        Finding(
            "trike-specified-max-decoder-nonfunctional",
            "kem-36",
            "specification_implementation_contradiction",
            "confirmed" if len(trike_dirs) == 4 and len(trike_min) == 4 and trike_has_max else "not_reproduced",
            "The PDF normatively returns max(Tnow,T') and says to use the larger "
            "threshold, while all four implementations return min(Tnow,T').  In a "
            "paired TRIKE-2 whole-KEM sweep, the shipped min decoder recovered "
            "1000/1000 honest shared secrets and the literal PDF max decoder "
            "recovered 0/1000.  Thus the specified scheme is nonfunctional and "
            "the implementation/DFR target describes a different decoder.",
            [
                "kem-36/kem-36-spec.pdf (physical pages 8-10)",
                *[str((d / "src/decoder.c").relative_to(ROOT)) for d in trike_dirs],
                "security/kem_roundtrip_sweep.py",
            ],
        )
    )

    # UVW-512: H1 is specified as mapping directly to (r,e), but the submitted
    # instantiation factors it through a fixed 256-bit seed.
    uvw512_path = ROOT / "kem-38/Implementations/Reference_Implementation/UVW-KEM-512/src/KEM_AlgorithmInstance.c"
    uvw512 = uvw512_path.read_text(encoding="utf-8", errors="replace")
    uvw_spec = extract_pdf(ROOT / "kem-38/kem-38-spec.pdf")
    uvw_reproduced = all((
        re.search(r"UVW512.*512", uvw_spec, re.I | re.S),
        re.search(r"H1\s*:\s*\{0,\s*1\}.*?F", uvw_spec, re.S),
        "unsigned char seed_h1[32]" in uvw512,
        "256, seed_h1" in uvw512,
        "init_random_number(&drng_h1, seed_h1, 32)" in uvw512,
    ))
    findings.append(
        Finding(
            "uvw512-h1-seed-capacity",
            "kem-38",
            "implementation_specification_conformance_break",
            "confirmed" if uvw_reproduced else "not_reproduced",
            "UVW-512 claims 512-bit classical security and specifies H1 as a "
            "hash directly into the encryption pair (r,e).  The implementation "
            "instead hashes m to a 256-bit seed and deterministically expands "
            "that seed into (r,e).  Enumerate the at most 2^256 H1 seeds, expand "
            "each candidate, and test c1=rG+e against the public challenge; a "
            "match recovers m from c2 and hence the session key.  This caps the "
            "implemented UVW-512 confidentiality at 256 classical bits and "
            "128 quantum bits under generic search.",
            [
                "kem-38/kem-38-spec.pdf (physical pages 15-16)",
                str(uvw512_path.relative_to(ROOT)),
            ],
        )
    )

    uvw128_path = ROOT / "kem-38/Implementations/Reference_Implementation/UVW-KEM-128/src/KEM_AlgorithmInstance.c"
    uvw128 = uvw128_path.read_text(encoding="utf-8", errors="replace")
    uvw_oracle_reproduced = all(token in uvw128 for token in (
        "if (ret != 0)\n        return -2;",
        "return -1; // 验证失败",
        "memcmp(kct->d, d_prime, sizeof(d_prime))",
    ))
    findings.append(
        Finding(
            "uvw-distinct-decapsulation-failure-oracle",
            "kem-38",
            "implementation_side_channel",
            "confirmed" if uvw_oracle_reproduced else "not_reproduced",
            "UVW exposes list-decoder failure as -2 and later FO validation "
            "failure as -1.  On a deterministic UVW-128 key and ciphertext, "
            "one-bit mutations at positions 0 and 846 reproduce the two paths: "
            "about 49.3 seconds for -2 versus 0.81 seconds for -1 on the audit "
            "host.  The status and roughly 60-fold timing split provide a "
            "stable decryption-failure oracle; no end-to-end key recovery is "
            "claimed yet.",
            [str(uvw128_path.relative_to(ROOT)), "security/kem_mutation_oracle.py"],
        )
    )

    for finding in findings:
        finding.report_id = REPORT_IDS[finding.check]
    return findings


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--json", action="store_true", help="emit machine-readable JSON")
    parser.add_argument(
        "--report-id",
        action="append",
        default=[],
        help="emit only the named stable report ID (repeatable)",
    )
    args = parser.parse_args()

    rows = metadata()
    ceilings, prefix_findings = hash_checks(rows)
    shared_secrets = shared_secret_inventory(rows)
    registered = registered_checks()
    if args.report_id:
        requested = set(args.report_id)
        registered = [finding for finding in registered if finding.report_id in requested]
        prefix_findings = [finding for finding in prefix_findings if finding.report_id in requested]
        found = {finding.report_id for finding in registered + prefix_findings}
        missing = sorted(requested - found)
        if missing:
            parser.error(f"unknown report ID(s): {', '.join(missing)}")
        ceilings = []
        shared_secrets = []
    result = {
        "scope": {
            "built_instances": len(rows),
            "hash_instances": len(ceilings),
            "hash_messages": [name for name, _, _ in MESSAGES],
            "limitations": [
                "Digest ceilings are generic upper bounds, not attacks on their own.",
                "Sampled prefix relations become universal only after construction/source review.",
                "Registered checks are intentionally narrow and evidence-specific.",
            ],
        },
        "registered_findings": [asdict(finding) for finding in registered],
        "hash_prefix_findings": [asdict(finding) for finding in prefix_findings],
        "hash_generic_ceilings": ceilings,
        "shared_secret_capacity": shared_secrets,
    }
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        if args.report_id:
            print(f"selected report IDs: {', '.join(args.report_id)}")
        else:
            print(
                f"checked {len(rows)} built instances; "
                f"{len(ceilings)} are hash instances"
            )
        for finding in registered + prefix_findings:
            print(
                f"{finding.status:20} {finding.candidate:8} "
                f"{finding.check}: {finding.detail}"
            )
    selected = registered + prefix_findings
    return 0 if selected and all(f.status == "confirmed" for f in selected) else 1


if __name__ == "__main__":
    raise SystemExit(main())
