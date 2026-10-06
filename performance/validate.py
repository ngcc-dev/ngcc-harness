#!/usr/bin/env python3
"""Check the published performance evidence and the reports generated from it.

    python3 performance/validate.py          (also: make -C performance check)

For the source catalog: every submission is covered in order, with the archive
digest recorded in SOURCE_ARCHIVES.md. For every published campaign dataset
performance/data/<system>/ (see performance/systems.csv):

  - the system is registered and matches campaign.json;
  - no file contains a local absolute path;
  - every KAT log exists, matches the digest in build.json and ends with the
    recorded result;
  - every timing record belongs to a built instance and its statistics
    (measurement count, mean time and cycles, median trial, throughput) are
    recomputed from the raw trials;
  - every hash profile's shares are recomputed from its call records;
  - every measured instance has one explicit comparison-target row;
  - every key-exchange instance has one bandwidth row, checked against its
    source key sizes and raw passes (accounting for AFS-KEX's API mapping);
  - every measured KEM/signature instance has a separate external-size row;
  - performance/report.py --check regenerates every report of that system and
    finds them byte-for-byte identical to the committed pages.
"""

from __future__ import annotations

import csv
import hashlib
import json
import math
from pathlib import Path
import re
import statistics
import subprocess
import sys

from import_sources import expected_sha256


ROOT = Path(__file__).resolve().parent.parent
DATA = ROOT / "performance/data"
ABSOLUTE = re.compile(r"(?<![\w.])/(home|tmp|root|Users)/")


def check_source_catalog() -> list[str]:
    problems = []
    with (ROOT / "downloads.csv").open(newline="", encoding="utf-8") as source:
        expected_ids = [row["ID"] for row in csv.DictReader(source, delimiter=";")]
    with (ROOT / "performance/source_catalog.csv").open(newline="", encoding="utf-8") as source:
        rows = list(csv.DictReader(source))
    if [row["id"] for row in rows] != expected_ids:
        problems.append("source catalog does not cover downloads.csv in submission order")
    for row in rows:
        if row["archive_sha256"] != expected_sha256(row["id"]):
            problems.append(f"{row['id']}: source catalog archive digest differs from provenance")
        if int(row["source_files"]) < 1 or int(row["source_bytes"]) < 1:
            problems.append(f"{row['id']}: no software source recorded")
    return problems


def systems() -> dict:
    with (ROOT / "performance/systems.csv").open(encoding="utf-8") as f:
        return {r["ID"]: r for r in csv.DictReader((l for l in f if not l.startswith("#")), delimiter=";")}


def check_security_targets(datasets: list[Path]) -> list[str]:
    path = ROOT / "performance/security_targets.csv"
    problems = []
    with path.open(encoding="utf-8", newline="") as source:
        reader = csv.DictReader(source, delimiter=";")
        if tuple(reader.fieldnames or ()) != ("ID", "Instance", "TargetBits"):
            return ["performance/security_targets.csv: invalid header"]
        rows = list(reader)
    found = set()
    for line_number, row in enumerate(rows, 2):
        key = row["ID"], row["Instance"]
        if key in found:
            problems.append(f"performance/security_targets.csv:{line_number}: duplicate {key[0]}/{key[1]}")
        found.add(key)
        if row["TargetBits"] not in ("", "128", "256", "512"):
            problems.append(
                f"performance/security_targets.csv:{line_number}: invalid target {row['TargetBits']!r}")
    expected = set()
    for dataset in datasets:
        build = json.loads((dataset / "build.json").read_text(encoding="utf-8"))["instances"]
        for entry in build.values():
            if entry["candidate"] != "iccs":
                expected.add((entry["candidate"], entry["label"]))
    for candidate, label in sorted(expected - found):
        problems.append(f"performance/security_targets.csv: missing {candidate}/{label}")
    for candidate, label in sorted(found - expected):
        problems.append(f"performance/security_targets.csv: stale {candidate}/{label}")
    return problems


def check_kex_bandwidth(datasets: list[Path]) -> list[str]:
    path = ROOT / "performance/kex_bandwidth.csv"
    problems = []
    with path.open(encoding="utf-8", newline="") as source:
        lines = [(number, line) for number, line in enumerate(source, 1)
                 if not line.startswith("#")]
        reader = csv.DictReader((line for _, line in lines), delimiter=";")
        if tuple(reader.fieldnames or ()) != (
                "ID", "Instance", "PkABytes", "PkBBytes", "ProtocolMessageBytes"):
            return ["performance/kex_bandwidth.csv: invalid header"]
        rows = list(zip((number for number, _ in lines[1:]), reader))
    with (ROOT / "data/parameters.csv").open(encoding="utf-8", newline="") as source:
        source_pks = {(row["ID"], row["Instance"]): int(row["PublicKeyBytes"])
                      for row in csv.DictReader(source, delimiter=";")
                      if row["ID"].startswith(("kex-", "kem-", "sign-"))}

    def expected_keys(key: tuple[str, str], cap: int) -> tuple[int, int]:
        candidate, label = key
        if candidate in ("kex-01", "kex-06"):
            return 0, cap  # Only the responder has a required static key.
        if candidate == "kex-02":
            return cap // 2, cap // 2  # Base keys; fresh composites are protocol messages.
        if candidate != "kex-03":
            return cap, cap
        mode = label.split("-")[1]
        kem_match = re.search(r"(PLAC|ZEN)(128|256|512)(Star)?", label)
        sig_match = re.search(r"BiT(128|256|512)", label)
        kem_pk = None
        if kem_match:
            family, level, star = kem_match.groups()
            if family == "PLAC":
                instance = f"POLARLAC-{level}" + ("-Star" if star else "")
                kem_pk = source_pks["kem-30", instance]
            else:
                kem_pk = source_pks["kem-41", f"ZEN_{level}"]
        sig_pk = source_pks["sign-02", f"BiT-{sig_match.group(1)}"] if sig_match else None
        if mode == "K2K":
            return kem_pk, kem_pk
        if mode == "K2S":
            return kem_pk, sig_pk
        if mode == "S2K":
            return sig_pk, kem_pk
        if mode == "S2S":
            return sig_pk, sig_pk
        raise ValueError(f"unrecognized CreTAKE instance: {label}")
    bandwidth = {}
    for line_number, row in rows:
        key = row.get("ID"), row.get("Instance")
        if not all(key):
            problems.append(f"performance/kex_bandwidth.csv:{line_number}: missing ID or instance")
            continue
        if key in bandwidth:
            problems.append(f"performance/kex_bandwidth.csv:{line_number}: duplicate {key}")
            continue
        try:
            values = tuple(int(row.get(field)) for field in ("PkABytes", "PkBBytes", "ProtocolMessageBytes"))
        except (TypeError, ValueError):
            problems.append(f"performance/kex_bandwidth.csv:{line_number}: invalid byte count")
            continue
        if min(values) < 0:
            problems.append(f"performance/kex_bandwidth.csv:{line_number}: negative byte count")
        bandwidth[key] = values
    expected = set()
    for ds in datasets:
        for record_path in sorted((ds / "records").glob("kex-*/*__exchange.json")):
            r = json.loads(record_path.read_text(encoding="utf-8"))
            key = r["candidate"], r["label"]
            expected.add(key)
            if key not in bandwidth:
                continue
            pk_a, pk_b, messages = bandwidth[key]
            sizes = r["sizes"]
            cap = int(sizes["pk_bytes"])
            if (pk_a, pk_b) != expected_keys(key, cap):
                problems.append(f"{ds.name}: {key}: required public keys differ from source sizes")
            raw = int(sizes["total_msg_bytes"])
            if key[0] == "kex-02":
                # The frozen pass-4 function never sets its no-output length.
                raw -= int(sizes["msg4_bytes"])
                # Figure 3 sends two fresh composites that the API instead
                # places in its pre-distributed public-key buffers.
                raw += pk_a + pk_b
            if messages != raw:
                problems.append(f"{ds.name}: {key}: protocol messages {messages} != corrected raw {raw}")
            if messages + pk_a + pk_b == 0:
                problems.append(f"{ds.name}: {key}: zero bandwidth")
    for key in sorted(expected - bandwidth.keys()):
        problems.append(f"performance/kex_bandwidth.csv: missing {key}")
    for key in sorted(bandwidth.keys() - expected):
        problems.append(f"performance/kex_bandwidth.csv: stale {key}")
    return problems


def check_external_sizes(datasets: list[Path]) -> list[str]:
    path = ROOT / "performance/external_sizes.csv"
    problems = []
    with path.open(encoding="utf-8", newline="") as source:
        lines = [(number, line) for number, line in enumerate(source, 1)
                 if not line.startswith("#")]
        reader = csv.DictReader((line for _, line in lines), delimiter=";")
        if tuple(reader.fieldnames or ()) != (
                "ID", "Instance", "PublicKeyBytes", "CiphertextBytes", "SignatureBytes", "Basis"):
            return ["performance/external_sizes.csv: invalid header"]
        rows = list(zip((number for number, _ in lines[1:]), reader))
    sizes = {}
    for line_number, row in rows:
        key = row.get("ID"), row.get("Instance")
        if not all(key):
            problems.append(f"performance/external_sizes.csv:{line_number}: missing ID or instance")
            continue
        if key in sizes:
            problems.append(f"performance/external_sizes.csv:{line_number}: duplicate {key}")
            continue
        try:
            pk, ct, sig = (int(row.get(field) or 0) for field in
                           ("PublicKeyBytes", "CiphertextBytes", "SignatureBytes"))
        except (TypeError, ValueError):
            problems.append(f"performance/external_sizes.csv:{line_number}: invalid byte count")
            continue
        basis = row.get("Basis")
        if pk <= 0 or min(ct, sig) < 0 or (ct == 0) == (sig == 0):
            problems.append(f"performance/external_sizes.csv:{line_number}: invalid external sizes")
        if basis not in ("encoded", "maximum-variable", "nominal-variable"):
            problems.append(f"performance/external_sizes.csv:{line_number}: invalid basis {basis!r}")
        sizes[key] = pk, ct, sig, basis
    expected = set()
    for ds in datasets:
        for record_path in sorted((ds / "records").glob("*/*__keygen.json")):
            r = json.loads(record_path.read_text(encoding="utf-8"))
            key = r["candidate"], r["label"]
            if key[0].split("-", 1)[0] not in ("kem", "sign"):
                continue
            expected.add(key)
            if key not in sizes:
                continue
            pk, ct, sig, basis = sizes[key]
            raw = r["sizes"]
            if pk != int(raw["pk_bytes"]):
                problems.append(f"{ds.name}: {key}: public key disagrees with API encoding")
            if key[0].startswith("kem-"):
                if sig != 0 or ct != int(raw["ct_bytes"]):
                    problems.append(f"{ds.name}: {key}: ciphertext disagrees with API encoding")
            elif ct != 0:
                problems.append(f"{ds.name}: {key}: signature row has a ciphertext size")
            elif basis == "nominal-variable":
                if not 0 < sig < int(raw["signature_bytes"]):
                    problems.append(f"{ds.name}: {key}: nominal signature is not below API cap")
            elif sig != int(raw["signature_bytes"]):
                problems.append(f"{ds.name}: {key}: signature disagrees with API encoding")
    for key in sorted(expected - sizes.keys()):
        problems.append(f"performance/external_sizes.csv: missing {key}")
    for key in sorted(sizes.keys() - expected):
        problems.append(f"performance/external_sizes.csv: stale {key}")
    # The independently inventoried submission API constants are a second
    # source check, not the source of the catalog itself. Some benchmark labels
    # denote optimized variants and have no exact row in this reference index.
    with (ROOT / "data/parameters.csv").open(encoding="utf-8", newline="") as source:
        parameters = {(row["ID"], row["Instance"]): row for row in
                      csv.DictReader(source, delimiter=";")
                      if row["Type"] in ("kem", "sig")}
    for key, (pk, ct, sig, basis) in sizes.items():
        if not (ROOT / key[0] / "pseudocode.md").is_file():
            problems.append(f"performance/external_sizes.csv: no specification comparison for {key}")
        params = parameters.get(key)
        if not params:
            continue
        if pk != int(params["PublicKeyBytes"]):
            problems.append(f"performance/external_sizes.csv: {key}: public key differs from source catalog")
        if ct and ct != int(params["CiphertextBytes"]):
            problems.append(f"performance/external_sizes.csv: {key}: ciphertext differs from source catalog")
        if sig and basis != "nominal-variable" and sig != int(params["SignatureBytes"]):
            problems.append(f"performance/external_sizes.csv: {key}: signature differs from source catalog")
    return problems


def close(a, b) -> bool:
    return (a is None and b is None) or (a is not None and b is not None and math.isclose(a, b, rel_tol=1e-9))


def check_record(name: str, r: dict, build: dict) -> list[str]:
    problems = []
    key = f"{r.get('candidate')}/{r.get('label')}"
    entry = build.get(key)
    if not entry:
        return [f"{name}: {key} is not in build.json"]
    if r.get("library") != entry.get("library") or r.get("kat_log") != entry.get("kat_log"):
        problems.append(f"{name}: library or KAT log differs from build.json")
    if r.get("status") != "complete":
        return problems
    trials = r.get("trials", [])
    if len(trials) != r["plan"]["trials"]:
        problems.append(f"{name}: {len(trials)} trials, plan says {r['plan']['trials']}")
    n = sum(t["measurements"] for t in trials)
    secs = sum(t["seconds"] for t in trials)
    per = [t["seconds"] / t["measurements"] for t in trials]
    cycles_ok = bool(trials) and all(t["cycles"] > 0 for t in trials)
    expect = {
        "measurements": n,
        "mean_seconds": secs / n if n else None,
        "median_trial_seconds": statistics.median(per) if per else None,
        "mean_cycles": sum(t["cycles"] for t in trials) / n if cycles_ok and n else None,
        "operations_per_second": n / secs if secs else None,
    }
    for field, value in expect.items():
        if field == "measurements":
            if r.get(field) != value:
                problems.append(f"{name}: measurements {r.get(field)} != sum of trials {value}")
        elif not close(r.get(field), value):
            problems.append(f"{name}: {field} {r.get(field)} does not follow from the trials ({value})")
    if r.get("guide_100_measurements_met") != (n >= 100):
        problems.append(f"{name}: guide_100_measurements_met inconsistent with {n} measurements")
    if r.get("input_bytes") and not close(r.get("throughput_MB_per_second"), r["input_bytes"] * n / secs / 1e6):
        problems.append(f"{name}: throughput does not follow from the trials")
    if not r.get("environment"):
        problems.append(f"{name}: no environment recorded")
    return problems


def check_profile(name: str, p: dict) -> list[str]:
    if p.get("status") != "complete" or not p.get("total"):
        return []
    total = p["total"]["ticks"]
    by_fn = {}
    for c in p.get("calls", []):
        by_fn[c["fn"]] = by_fn.get(c["fn"], 0) + c["ticks"]
    problems = []
    for fn, ticks in by_fn.items():
        if not close(p.get("share", {}).get(fn), ticks / total if total else None):
            problems.append(f"{name}: share of {fn} does not follow from its calls")
    hash_share = sum(t for fn, t in by_fn.items() if fn != "drng") / total if total else None
    if by_fn and not close(p.get("hash_share"), hash_share):
        problems.append(f"{name}: hash_share does not follow from its calls")
    return problems


def check_dataset(ds: Path, known: dict) -> tuple[list[str], dict]:
    problems, counts = [], {"records": 0, "profiles": 0, "kat_logs": 0}
    sid = ds.name
    campaign = json.loads((ds / "campaign.json").read_text(encoding="utf-8"))
    if sid not in known:
        problems.append(f"{sid}: not registered in performance/systems.csv")
    if campaign.get("system_id") != sid:
        problems.append(f"{sid}: campaign.json system_id is {campaign.get('system_id')!r}")
    for f in sorted(ds.rglob("*")):
        if f.is_file() and ABSOLUTE.search(f.read_text(encoding="utf-8", errors="replace")):
            problems.append(f"{f.relative_to(ROOT)}: contains a local absolute path")
    build = json.loads((ds / "build.json").read_text(encoding="utf-8"))["instances"]
    for key, e in sorted(build.items()):
        if not e.get("kat_log"):
            continue
        log = ds / e["kat_log"]
        if not log.is_file():
            problems.append(f"{sid}: {key} KAT log {e['kat_log']} missing")
            continue
        counts["kat_logs"] += 1
        if hashlib.sha256(log.read_bytes()).hexdigest() != e.get("kat_log_sha256"):
            problems.append(f"{sid}: {e['kat_log']} does not match its digest in build.json")
        results = re.findall(r"^RESULT \S+ \S+ (\S+)", log.read_text(encoding="utf-8", errors="replace"), re.M)
        if not results or results[-1] != e.get("kat"):
            problems.append(f"{sid}: {e['kat_log']} result {results[-1:] or '-'} != build.json {e.get('kat')}")
    for f in sorted((ds / "records").rglob("*.json")):
        counts["records"] += 1
        problems += check_record(f.relative_to(ds).as_posix(), json.loads(f.read_text(encoding="utf-8")), build)
    for f in sorted((ds / "profile").rglob("*.json")):
        counts["profiles"] += 1
        problems += check_profile(f.relative_to(ds).as_posix(), json.loads(f.read_text(encoding="utf-8")))
    check = subprocess.run([sys.executable, str(ROOT / "performance/report.py"), str(ds), "--system", sid, "--check"],
                           capture_output=True, text=True)
    if check.returncode:
        problems += [line for line in check.stdout.splitlines() if line.startswith("differs")] or \
                    [f"{sid}: report check failed: {check.stderr.strip()[-300:]}"]
    counts["report_check"] = check.stdout.strip().splitlines()[-1] if check.stdout.strip() else "failed"
    return problems, counts


def main() -> int:
    problems = check_source_catalog()
    print(f"performance source catalog: 119 submissions, {len(problems)} problem(s)")
    known = systems()
    datasets = sorted(d for d in DATA.iterdir() if (d / "campaign.json").is_file()) if DATA.is_dir() else []
    target_problems = check_security_targets(datasets)
    for problem in target_problems:
        print(problem)
    print(f"performance security targets: {len(target_problems)} problem(s)")
    problems += target_problems
    bandwidth_problems = check_kex_bandwidth(datasets)
    for problem in bandwidth_problems:
        print(problem)
    print(f"performance KEX bandwidth: {len(bandwidth_problems)} problem(s)")
    problems += bandwidth_problems
    size_problems = check_external_sizes(datasets)
    for problem in size_problems:
        print(problem)
    print(f"performance KEM/signature external sizes: {len(size_problems)} problem(s)")
    problems += size_problems
    for ds in datasets:
        found, counts = check_dataset(ds, known)
        for problem in found:
            print(problem)
        print(f"performance data {ds.name}: {counts['records']} timing record(s), {counts['profiles']} "
              f"profile(s), {counts['kat_logs']} KAT log(s), {len(found)} problem(s); {counts['report_check']}")
        problems += found
    stray = sorted(p.name for p in DATA.glob("*.json")) if DATA.is_dir() else []
    if stray:
        problems.append(f"performance/data has files outside a system dataset: {', '.join(stray)}")
        print(problems[-1])
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
