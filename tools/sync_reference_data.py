#!/usr/bin/env python3
"""Import the public specification and parameter dataset from a source checkout."""
from __future__ import annotations

import argparse
import csv
import hashlib
import re
import shutil
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
ID = re.compile(r"(?:sign|kem|kex|hash)-\d{2}")
FIELDS = (
    "ID", "Type", "Algorithm", "Instance", "Variant", "SourceDirectory",
    "PublicKeyBytes", "SecretKeyBytes", "CiphertextBytes", "SharedSecretBytes",
    "SignatureBytes", "Passes", "InitiatorStateBytes", "ResponderStateBytes",
    "TotalMessageBytes", "DigestBits", "DigestBytes",
)
META_TO_CSV = {
    "id": "ID", "type": "Type", "algorithm": "Algorithm", "instance": "Instance",
    "variant": "Variant", "source_dir": "SourceDirectory", "pk_len": "PublicKeyBytes",
    "sk_len": "SecretKeyBytes", "ct_len": "CiphertextBytes", "ss_len": "SharedSecretBytes",
    "sn_len": "SignatureBytes", "passes": "Passes", "sta_len": "InitiatorStateBytes",
    "stb_len": "ResponderStateBytes", "total_msg": "TotalMessageBytes",
    "digest_bits": "DigestBits", "digest_len": "DigestBytes",
}
SPEC_FIELDS = ("ID", "Bytes", "SHA256", "ArchivePath")


def candidate_ids(downloads: Path) -> list[str]:
    with downloads.open(encoding="utf-8", newline="") as source:
        rows = list(csv.DictReader(source, delimiter=";"))
    ids = [row["ID"] for row in rows]
    if len(ids) != len(set(ids)) or any(not ID.fullmatch(cid) for cid in ids):
        raise SystemExit("invalid or duplicate IDs in downloads.csv")
    return ids


def extract_metadata(source: Path, ids: set[str]) -> list[dict[str, str]]:
    harness = source / "bin/ngcc_kat"
    if not harness.is_file():
        raise SystemExit(f"missing metadata harness: {harness}")
    rows: list[dict[str, str]] = []
    for library in sorted(source.glob("*/lib/*.so")):
        proc = subprocess.run(
            [str(harness), str(library), "--meta-only"], text=True,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False,
        )
        if proc.returncode:
            raise SystemExit(f"metadata extraction failed for {library}: {proc.stderr.strip()}")
        values: dict[str, str] = {}
        for line in proc.stdout.splitlines():
            if "=" in line:
                key, value = line.split("=", 1)
                values[key.strip()] = value.strip()
        row = {field: "" for field in FIELDS}
        for source_key, destination in META_TO_CSV.items():
            row[destination] = values.get(source_key, "")
        if row["ID"] not in ids or not row["Instance"]:
            raise SystemExit(f"invalid metadata from {library}")
        rows.append(row)
    rows.sort(key=lambda row: (row["ID"], row["Instance"], row["Variant"]))
    return rows


def write_csv(path: Path, rows: list[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as destination:
        writer = csv.DictWriter(destination, fieldnames=FIELDS, delimiter=";", lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def write_spec_manifest(source: Path, ids: list[str]) -> None:
    archive_paths = {}
    for line in (source / "spec-sources.tsv").read_text(encoding="utf-8").splitlines():
        cid, archive_path = line.split("\t", 1)
        archive_paths[cid] = archive_path
    rows = []
    for cid in ids:
        path = ROOT / cid / f"{cid}-spec.pdf"
        content = path.read_bytes()
        rows.append({
            "ID": cid,
            "Bytes": str(len(content)),
            "SHA256": hashlib.sha256(content).hexdigest(),
            "ArchivePath": archive_paths.get(cid, ""),
        })
    path = ROOT / "data/specifications.csv"
    with path.open("w", encoding="utf-8", newline="") as destination:
        writer = csv.DictWriter(destination, fieldnames=SPEC_FIELDS, delimiter=";", lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def validate(ids: list[str]) -> list[str]:
    problems: list[str] = []
    for cid in ids:
        for name in (f"{cid}-spec.pdf", "pseudocode.md"):
            path = ROOT / cid / name
            if not path.is_file() or path.stat().st_size == 0:
                problems.append(f"{path.relative_to(ROOT)}: missing or empty")
    parameters = ROOT / "data/parameters.csv"
    rows: list[dict[str, str]] = []
    if not parameters.is_file():
        problems.append("data/parameters.csv: missing")
    else:
        with parameters.open(encoding="utf-8", newline="") as source:
            reader = csv.DictReader(source, delimiter=";")
            if tuple(reader.fieldnames or ()) != FIELDS:
                problems.append("data/parameters.csv: invalid header")
            rows = list(reader)
        # Some submissions reuse an instance/variant label for multiple primitive
        # choices or parameter sets.  The submitted source directory disambiguates
        # those rows without inventing a public name that the submission did not use.
        seen: set[tuple[str, str, str, str]] = set()
        for line_number, row in enumerate(rows, 2):
            key = (
                row.get("ID", ""), row.get("Instance", ""),
                row.get("Variant", ""), row.get("SourceDirectory", ""),
            )
            if key in seen:
                problems.append(f"data/parameters.csv:{line_number}: duplicate instance {key}")
            seen.add(key)
            if key[0] not in ids or not key[1]:
                problems.append(f"data/parameters.csv:{line_number}: invalid candidate or instance")
        missing = set(ids) - {row.get("ID", "") for row in rows}
        for cid in sorted(missing):
            problems.append(f"data/parameters.csv: no instance for {cid}")
        required = {
            "kem": ("PublicKeyBytes", "SecretKeyBytes", "CiphertextBytes", "SharedSecretBytes"),
            "sig": ("PublicKeyBytes", "SecretKeyBytes", "SignatureBytes"),
            "kex": ("Passes", "PublicKeyBytes", "SecretKeyBytes", "InitiatorStateBytes",
                    "ResponderStateBytes", "SharedSecretBytes", "TotalMessageBytes"),
            "hash": ("DigestBits", "DigestBytes"),
        }
        for line_number, row in enumerate(rows, 2):
            for field in required.get(row.get("Type", ""), ()):
                if not row.get(field, "").isdigit():
                    problems.append(f"data/parameters.csv:{line_number}: invalid {field}")

    manifest = ROOT / "data/specifications.csv"
    if not manifest.is_file():
        problems.append("data/specifications.csv: missing")
    else:
        with manifest.open(encoding="utf-8", newline="") as source:
            reader = csv.DictReader(source, delimiter=";")
            if tuple(reader.fieldnames or ()) != SPEC_FIELDS:
                problems.append("data/specifications.csv: invalid header")
            specs = list(reader)
        if [row.get("ID", "") for row in specs] != ids:
            problems.append("data/specifications.csv: candidate order or coverage differs from downloads.csv")
        for line_number, row in enumerate(specs, 2):
            path = ROOT / row.get("ID", "") / f'{row.get("ID", "")}-spec.pdf'
            if not path.is_file():
                continue
            content = path.read_bytes()
            if row.get("Bytes") != str(len(content)):
                problems.append(f"data/specifications.csv:{line_number}: byte size differs")
            if row.get("SHA256") != hashlib.sha256(content).hexdigest():
                problems.append(f"data/specifications.csv:{line_number}: SHA-256 differs")
            if not row.get("ArchivePath", ""):
                problems.append(f"data/specifications.csv:{line_number}: missing archive path")
    print(
        f"reference data: {len(ids)} candidate(s), {len(rows)} instance(s), "
        f"{len(problems)} problem(s)"
    )
    return problems


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", nargs="?", type=Path, help="source checkout to import")
    parser.add_argument("--check", action="store_true", help="validate the checked-in dataset only")
    args = parser.parse_args()
    ids = candidate_ids(ROOT / "downloads.csv")

    if not args.check:
        if args.source is None:
            parser.error("source is required unless --check is used")
        source = args.source.resolve()
        source_ids = candidate_ids(source / "downloads.csv")
        if set(source_ids) != set(ids):
            raise SystemExit("candidate IDs differ between source and harness downloads.csv")
        for cid in ids:
            destination = ROOT / cid
            destination.mkdir(exist_ok=True)
            for name in (f"{cid}-spec.pdf", "pseudocode.md"):
                origin = source / cid / name
                if not origin.is_file():
                    raise SystemExit(f"missing source file: {origin}")
                shutil.copyfile(origin, destination / name)
        data = ROOT / "data"
        data.mkdir(exist_ok=True)
        for name in ("sign.csv", "kem.csv", "kex.csv", "hash.csv", "spec-sources.tsv"):
            shutil.copyfile(source / name, data / name)
        write_csv(data / "parameters.csv", extract_metadata(source, set(ids)))
        write_spec_manifest(source, ids)

    problems = validate(ids)
    for problem in problems:
        print(problem)
    return 1 if problems else 0


if __name__ == "__main__":
    raise SystemExit(main())
