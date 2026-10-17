#!/usr/bin/env python3
"""Certify SYDO's secret-dependent witness-position write in all reference trees."""

from pathlib import Path


ROOT = Path(__file__).resolve().parent
SETS = ("160s", "160f", "256s", "256f", "512s", "512f")


def require(text: str, needle: str, path: Path) -> None:
    if needle not in text:
        raise SystemExit(f"not confirmed: {path}: missing {needle!r}")


reference_sources: list[bytes] = []
for name in SETS:
    path = ROOT / "Implementations" / "Reference_Implementation" / f"sydo_{name}" / "src" / "sydo.c"
    raw = path.read_bytes()
    reference_sources.append(raw)
    text = raw.decode("utf-8")

    require(text, "static void set_bit_lsb_first", path)
    require(text, "if (bit) {", path)
    require(text, "set_bit_lsb_first(bytes, bit_offset + i, ((v >> i) & 1u) != 0);", path)
    require(text, "store_bits_lsb_first(x_pos_out, block_i * pos_bits, pos_bits, v % block_len);", path)
    require(text, "const uint8_t* seed_sk = sk + sk_seed_offset(params);", path)
    require(text, "sample_x_positions(x_positions, params, seed_sk)", path)
    require(text, "sydo_ref_derive_public_key(params, pk_check, params->public_key_size, sk, sk_len)", path)

if len(set(reference_sources)) != 1:
    raise SystemExit("not confirmed: the six reference sydo.c sources are not byte-identical")

optimized_keys: list[bytes] = []
optimized_signers: list[bytes] = []
for name in SETS:
    source_dir = ROOT / "Implementations" / "Optimized_Implementation" / f"sydo_{name}" / "src"
    keys_path = source_dir / "keys.inc"
    sign_path = source_dir / "sign.cpp"
    keys_raw = keys_path.read_bytes()
    sign_raw = sign_path.read_bytes()
    optimized_keys.append(keys_raw)
    optimized_signers.append(sign_raw)
    keys = keys_raw.decode("utf-8")
    signer = sign_raw.decode("utf-8")

    require(keys, "inline void set_bit_lsb_first", keys_path)
    require(keys, "if (bit)", keys_path)
    require(keys, "compute_witness(uint8_t* witness_extended_out, const uint8_t* witness_raw)", keys_path)
    require(keys, "get_bit_lsb_first(witness_raw, subvector_raw_off + raw_off + k);", keys_path)
    require(keys, "set_bit_lsb_first(witness_extended_out, subvector_ext_off + ext_off + k, bit);", keys_path)
    require(signer, "sydo_detail::compute_witness<P>(u_extended, sk.witness.data());", sign_path)

if len(set(optimized_keys)) != 1 or len(set(optimized_signers)) != 1:
    raise SystemExit("not confirmed: the optimized keys.inc/sign.cpp copies are not byte-identical")

print("SOURCE sign-28-6 CONFIRMED: secret-dependent witness branches in 6/6 reference and 6/6 optimized trees")
