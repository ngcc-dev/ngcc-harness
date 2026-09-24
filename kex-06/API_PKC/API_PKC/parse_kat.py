#!/usr/bin/env python3
"""Parse NGCC KAT_KEX output into canonical MAMBA-NIKE .req/.rsp files."""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from tools.profile_manifest import load_profiles  # noqa: E402


def load_profile(profile: str) -> dict[str, int]:
    for p in load_profiles():
        if p.name == profile:
            pk = p.pk_bytes
            m1 = p.m1_bytes
            sk = p.sk_api_bytes
            ss = p.ss_bytes
            suffix = p.kat_suffix
            break
    else:
        raise ValueError(f"unknown profile {profile}")

    return {
        "pk": pk,
        "sk": sk,
        "m1": m1,
        "ss": ss,
        "suffix": suffix,
    }


def field_hex(data: str, label: str) -> str:
    m = re.search(rf"{label} = ([0-9A-Fa-f\n\r]+)", data)
    return m.group(1).replace("\n", "").replace("\r", "").strip() if m else ""


def parse_kat_file(filepath: str) -> list[tuple[int, str, str, str, str, str]]:
    """Return (count, seed, responder pk, responder sk, m1, ss) records."""
    text = Path(filepath).read_text()
    records = []
    blocks = re.split(r"Count = (\d+)", text)
    for i in range(1, len(blocks), 2):
        count = int(blocks[i])
        data = blocks[i + 1]
        seed = field_hex(data, "Seed")
        pk = field_hex(data, "PKb") or field_hex(data, "PKa")
        sk = field_hex(data, "SKb") or field_hex(data, "SKa")
        m1 = field_hex(data, "M1")
        ss = field_hex(data, "SS")
        if not all((seed, pk, sk, m1, ss)):
            raise ValueError(f"missing KAT field at Count={count}")
        records.append((count, seed, pk, sk, m1, ss))
    return records


def check_records(records: list[tuple[int, str, str, str, str, str]], expected: dict[str, int]) -> None:
    if not records:
        raise ValueError("no KAT records parsed")
    for count, seed, pk, sk, m1, ss in records:
        lengths = {
            "seed": len(seed) // 2,
            "pk": len(pk) // 2,
            "sk": len(sk) // 2,
            "m1": len(m1) // 2,
            "ss": len(ss) // 2,
        }
        if lengths["seed"] != 64:
            raise ValueError(f"Count={count}: seed length {lengths['seed']} != 64")
        for field in ("pk", "sk", "m1", "ss"):
            if lengths[field] != expected[field]:
                raise ValueError(
                    f"Count={count}: {field} length {lengths[field]} != {expected[field]}"
                )


def write_fprintstr(f, label: str, hexstr: str) -> None:
    f.write(f"{label}\n")
    for i in range(0, len(hexstr), 2000):
        f.write(hexstr[i : i + 2000] + "\n")


def write_kat_files(profile: str, suffix: int, records: list[tuple[int, str, str, str, str, str]],
                    expected: dict[str, int]) -> tuple[str, str]:
    kat_dir = Path("KAT") / profile
    kat_dir.mkdir(parents=True, exist_ok=True)

    req_path = kat_dir / f"PQCkexKAT_{suffix}.req"
    with req_path.open("w") as f:
        f.write(f"# {profile} KEX KAT Request File\n")
        f.write("# seed_len=64\n\n")
        for count, seed, _, _, _, _ in records:
            f.write(f"Count = {count}\n")
            f.write(f"Seed = {seed}\n\n")

    rsp_path = kat_dir / f"PQCkexKAT_{suffix}.rsp"
    with rsp_path.open("w") as f:
        f.write(f"# {profile} KEX KAT Response File\n")
        f.write(
            f"# pk_len={expected['pk']} sk_len={expected['sk']} "
            f"m1_len={expected['m1']} ss_len={expected['ss']}\n\n"
        )
        for count, seed, pk, sk, m1, ss in records:
            f.write(f"Count = {count}\n")
            f.write(f"Seed = {seed}\n")
            write_fprintstr(f, "pk = ", pk)
            write_fprintstr(f, "sk = ", sk)
            write_fprintstr(f, "m1 = ", m1)
            f.write(f"ss = {ss}\n\n")

    return str(req_path), str(rsp_path)


def main(argv: list[str]) -> int:
    if len(argv) != 4:
        print(f"Usage: {argv[0]} <profile> <kat_suffix_bytes> <input_kat.txt>", file=sys.stderr)
        return 1

    profile = argv[1]
    suffix = int(argv[2])
    input_file = argv[3]
    expected = load_profile(profile)
    if suffix != expected["suffix"]:
        raise ValueError(f"{profile}: suffix {suffix} != manifest {expected['suffix']}")

    records = parse_kat_file(input_file)
    check_records(records, expected)
    req, rsp = write_kat_files(profile, suffix, records, expected)
    print(f"  wrote {req} ({len(records)} vectors)")
    print(f"  wrote {rsp} ({len(records)} vectors)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
