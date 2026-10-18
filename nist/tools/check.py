#!/usr/bin/env python3
"""Validate the checked-in NIST subsite data without a source checkout."""
from __future__ import annotations

import csv
import hashlib
import json
import re
import sys
from collections import defaultdict
from pathlib import Path
from urllib.parse import urlparse


ROOT = Path(__file__).resolve().parent.parent
SCHEMES = ("SQIsign", "HAWK", "MQOM", "SDitH", "MAYO", "QR-UOV", "SNOVA", "UOV", "FAEST")
SEVERITIES = {"Critical", "High", "Medium", "Low", "Informational"}
STATUSES = {"Confirmed", "Probable", "Lead", "Proof gap", "Withdrawn"}
LAYERS = {"Design", "Implementation", "Side-channel", "Evaluation"}
DISCOVERY = {"Trivial", "Moderate", "Non-trivial", "Hard"}
REMEDIATION = {"None", "Local", "Moderate", "Architectural"}
FINDING_FIELDS = (
    "ID", "Scheme", "Severity", "Status", "Layer", "Discovery", "Exploitation",
    "Remediation", "Title", "Credit", "Report", "Verification", "Reproducer", "Scope", "SourceIDs",
)
REPRODUCER_FIELDS = (
    "ID", "ScriptPath", "ScriptSHA256", "NeedsOfficialSource",
    "BuildDirectory", "BuildTarget", "Prerequisites",
)
CANDIDATE_FIELDS = (
    "order", "family", "scheme", "candidate_status", "package_round", "candidate_page",
    "spec_label", "spec_url", "spec_sha256", "archive_label", "archive_url",
    "archive_bytes", "archive_sha256",
)
ID = re.compile(r"(FAEST|HAWK|MAYO|MQOM|QR-UOV|SDitH|SNOVA|SQIsign|UOV)-([1-9][0-9]*)")
HASH = re.compile(r"[0-9a-f]{64}")
PRIVATE_PROJECT_NAME = "nist" + "-sig3"
PRIVATE_HTTP = re.compile(r"https?://[^\s)\]>'\"]*" + re.escape(PRIVATE_PROJECT_NAME), re.IGNORECASE)
TRUSTED_PYTHON = {
    "reproduce.py",
    "tools/check.py",
    "tools/fetch_submissions.py",
    "tools/sync.py",
    "tools/test_reproduce.py",
}
IGNORED_PYTHON_PARTS = {
    ".cache", "__pycache__", "build", "lib", "results", "submission",
}
REQUIRED_PREREQUISITES = {
    "FAEST-1": ("pycryptodome",),
    "MQOM-2": ("numpy", "numba", "pycryptodome", "joblib"),
    "SDitH-2": ("cmake", "pdftotext"),
}


def safe_package_path(relative: str) -> bool:
    """Accept only canonical relative paths contained by this package."""
    path = Path(relative)
    if (not relative or "\\" in relative or path.is_absolute() or ".." in path.parts
            or "." in path.parts or path.as_posix() != relative):
        return False
    try:
        (ROOT / path).resolve().relative_to(ROOT.resolve())
    except (OSError, ValueError):
        return False
    return True


def expected_script(identifier: str, scheme: str) -> str:
    if identifier == "HAWK-1":
        return "security/reproduce_hawk_1.py"
    stems = {"QR-UOV": "qruov", "SDitH": "sdith", "SQIsign": "sqisign"}
    suffix = identifier.rsplit("-", 1)[1]
    return f"{scheme}/reproduce_{stems.get(scheme, scheme.lower())}_{suffix}.py"


def rows(path: Path, fields: tuple[str, ...], problems: list[str]) -> list[dict[str, str]]:
    if not path.is_file():
        problems.append(f"missing {path.name}")
        return []
    with path.open(encoding="utf-8", newline="") as source:
        reader = csv.DictReader(source)
        if tuple(reader.fieldnames or ()) != fields:
            problems.append(f"{path.name}: invalid header")
        result = []
        for line, raw in enumerate(reader, 2):
            if None in raw:
                problems.append(f"{path.name}:{line}: too many columns")
            if any(value is None for key, value in raw.items() if key is not None):
                problems.append(f"{path.name}:{line}: too few columns")
            result.append({key: value or "" for key, value in raw.items() if key is not None})
        return result


def official(url: str) -> bool:
    parsed = urlparse(url)
    return parsed.scheme == "https" and parsed.hostname == "csrc.nist.gov"


def field(text: str, name: str) -> str:
    match = re.search(rf"(?m)^\*{{0,2}}{re.escape(name)}:\*{{0,2}}\s*(.+?)\s*$", text)
    return match.group(1) if match else ""


def main() -> int:
    problems: list[str] = []
    candidate_rows = rows(ROOT / "candidates.csv", CANDIDATE_FIELDS, problems)
    if tuple(row.get("scheme") for row in candidate_rows) != SCHEMES:
        problems.append("candidates.csv: expected the canonical nine-scheme order")
    for line, row in enumerate(candidate_rows, 2):
        scheme = row.get("scheme", "")
        if row.get("order") != str(line - 1):
            problems.append(f"candidates.csv:{line}: invalid order")
        for column in ("candidate_page", "spec_url", "archive_url"):
            if not official(row.get(column, "")):
                problems.append(f"candidates.csv:{line}: {column} is not an official NIST URL")
        if not row.get("archive_bytes", "").isdigit():
            problems.append(f"candidates.csv:{line}: invalid archive_bytes")
        for column in ("archive_sha256", "spec_sha256"):
            if not HASH.fullmatch(row.get(column, "")):
                problems.append(f"candidates.csv:{line}: invalid {column}")
        if scheme == "HAWK":
            if (row.get("candidate_status") != "withdrawn" or row.get("package_round") != "2"
                    or row.get("spec_label") != "Round 2 PDF" or row.get("archive_label") != "Round 2 ZIP"):
                problems.append("candidates.csv: HAWK must identify its withdrawn Round 2 fallback")
        elif (row.get("candidate_status") != "On-Ramp Round 3 candidate" or row.get("package_round") != "3"
              or row.get("spec_label") != "PDF" or row.get("archive_label") != "ZIP"):
            problems.append(f"candidates.csv:{line}: {scheme} must be a On-Ramp Round 3 candidate")

    finding_rows = rows(ROOT / "findings.csv", FINDING_FIELDS, problems)
    reproducer_rows = rows(ROOT / "reproducers.csv", REPRODUCER_FIELDS, problems)
    reproducers = {row.get("ID", ""): row for row in reproducer_rows}
    if len(reproducers) != len(reproducer_rows):
        problems.append("reproducers.csv: duplicate IDs")
    seen: set[str] = set()
    numbers: dict[str, list[int]] = defaultdict(list)
    source_ids = set()
    try:
        sources = json.loads((ROOT / "sources.json").read_text(encoding="utf-8"))
        source_ids = {entry["id"] for entry in sources}
        if len(source_ids) != len(sources):
            problems.append("sources.json: duplicate source IDs")
    except (OSError, ValueError, KeyError) as exc:
        problems.append(f"sources.json: {exc}")

    for line, row in enumerate(finding_rows, 2):
        identifier = row.get("ID", "")
        match = ID.fullmatch(identifier)
        if not match:
            problems.append(f"findings.csv:{line}: invalid ID {identifier!r}")
            continue
        scheme, number = match.groups()
        if row.get("Scheme") != scheme:
            problems.append(
                f"findings.csv:{line}: Scheme {row.get('Scheme')!r} differs from ID {identifier!r}"
            )
        expected_report = f"{scheme}/{identifier}.md"
        if row.get("Report") != expected_report:
            problems.append(f"findings.csv:{line}: Report must be {expected_report!r}")
        numbers[scheme].append(int(number))
        if identifier in seen:
            problems.append(f"findings.csv:{line}: duplicate {identifier}")
        seen.add(identifier)
        reproducer = reproducers.get(identifier)
        if reproducer is None:
            problems.append(f"reproducers.csv: missing {identifier}")
        elif row.get("Reproducer"):
            expected = expected_script(identifier, scheme)
            if row["Reproducer"] != expected:
                problems.append(f"findings.csv:{line}: Reproducer must be {expected!r}")
            if reproducer.get("ScriptPath") != row["Reproducer"]:
                problems.append(f"reproducers.csv: script path differs for {identifier}")
            if (not safe_package_path(reproducer.get("ScriptPath", ""))
                    or not reproducer.get("ScriptPath", "").endswith(".py")):
                problems.append(f"reproducers.csv: unsafe script path for {identifier}")
            if not HASH.fullmatch(reproducer.get("ScriptSHA256", "")):
                problems.append(f"reproducers.csv: invalid script hash for {identifier}")
            if reproducer.get("NeedsOfficialSource") not in {"yes", "no"}:
                problems.append(f"reproducers.csv: invalid source requirement for {identifier}")
            if bool(reproducer.get("BuildDirectory")) != bool(reproducer.get("BuildTarget")):
                problems.append(f"reproducers.csv: incomplete build recipe for {identifier}")
            if reproducer.get("BuildDirectory") not in {"", scheme}:
                problems.append(f"reproducers.csv: invalid build directory for {identifier}")
            if reproducer.get("BuildTarget") not in {"", "libs"}:
                problems.append(f"reproducers.csv: invalid build target for {identifier}")
            if not reproducer.get("Prerequisites"):
                problems.append(f"reproducers.csv: missing prerequisites for {identifier}")
            prerequisites = reproducer.get("Prerequisites", "").lower()
            for required in REQUIRED_PREREQUISITES.get(identifier, ()):
                if required not in prerequisites:
                    problems.append(
                        f"reproducers.csv: {identifier} omits prerequisite {required}"
                    )
        elif reproducer.get("ScriptPath") or reproducer.get("ScriptSHA256"):
            problems.append(f"reproducers.csv: unexpected script for {identifier}")
        for column, allowed in (
            ("Severity", SEVERITIES), ("Status", STATUSES), ("Layer", LAYERS),
            ("Discovery", DISCOVERY), ("Remediation", REMEDIATION),
        ):
            if row.get(column) not in allowed:
                problems.append(f"findings.csv:{line}: invalid {column}")
        for source_id in row.get("SourceIDs", "").split(";"):
            if source_id not in source_ids:
                problems.append(f"findings.csv:{line}: unknown source ID {source_id!r}")
        report = ROOT / "reports" / f"{identifier}.md"
        if not report.is_file():
            problems.append(f"missing reports/{identifier}.md")
            continue
        text = report.read_text(encoding="utf-8")
        heading = text.splitlines()[0] if text else ""
        if not heading.startswith(f"# {identifier} — {row['Severity']}:"):
            problems.append(f"reports/{identifier}.md: heading differs from inventory")
        for column in ("Severity", "Status", "Layer", "Discovery", "Remediation", "Credit"):
            if field(text, column) != row[column]:
                problems.append(f"reports/{identifier}.md: {column} differs from inventory")
        for report_field, csv_field in (("Affected", "Scope"), ("Exploitation", "Exploitation")):
            if field(text, report_field) != row[csv_field]:
                problems.append(
                    f"reports/{identifier}.md: {report_field} differs from inventory {csv_field}"
                )
        if (not field(text, "Affected") or not field(text, "Exploitation")
                or not field(text, "Credit") or not field(text, "Date")):
            problems.append(f"reports/{identifier}.md: incomplete metadata")
        if f"python3 nist/reproduce.py {identifier}" not in text:
            problems.append(f"reports/{identifier}.md: missing stable harness reproduction command")
        for target in re.findall(r"\[[^\]]*\]\(([^)]+)\)", text):
            if not target.startswith(("https://", "http://", "mailto:", "#", "/nist/")):
                problems.append(f"reports/{identifier}.md: unresolved relative link {target!r}")
        for command_path in re.findall(
            r"(?m)\bpython3\s+([A-Za-z0-9_.-]+(?:/[A-Za-z0-9_.-]+)+\.py)\b", text,
        ):
            relative = command_path.removeprefix("nist/")
            path = Path(relative)
            if path.is_absolute() or ".." in path.parts or not (ROOT / path).is_file():
                problems.append(
                    f"reports/{identifier}.md: command references missing script {command_path}"
                )
    for scheme in SCHEMES:
        expected = list(range(1, len(numbers[scheme]) + 1))
        if sorted(numbers[scheme]) != expected:
            problems.append(f"{scheme}: finding numbers are not sequential")
    report_ids = {path.stem for path in (ROOT / "reports").glob("*.md")}
    for identifier in sorted(report_ids - seen):
        problems.append(f"reports/{identifier}.md: absent from findings.csv")

    classification = (ROOT / "classification.md")
    if not classification.is_file():
        problems.append("missing classification.md")
    else:
        policy = classification.read_text(encoding="utf-8")
        for needle in ("NIST additional-signature call", "EUF-CMA", "2^64", "random-number-generator malfunction"):
            if needle not in policy:
                problems.append(f"classification.md: missing {needle!r}")
        if "https://csrc.nist.gov/" not in policy:
            problems.append("classification.md: missing official NIST call link")
        if "https://csrc.nist.gov/Projects/pqc-dig-sig/standardization/call-for-proposals" not in policy:
            problems.append("classification.md: missing official NIST call page")

    public_classification = ROOT / "classification-public.md"
    if not public_classification.is_file():
        problems.append("missing classification-public.md")
    else:
        public_policy = public_classification.read_text(encoding="utf-8")
        for needle in (
            "NIST additional-signature report classification", "EUF-CMA", "2^64",
            "## Constant-time classification", "## Entropy and malleability classification",
            "Severity", "Status", "Layer", "Discovery", "Remediation",
            "independent project assessments, not ratings issued by NIST",
            "https://csrc.nist.gov/Projects/pqc-dig-sig/standardization/call-for-proposals",
            "https://csrc.nist.gov/csrc/media/Projects/pqc-dig-sig/documents/call-for-proposals-dig-sig-sept-2022.pdf",
        ):
            if needle not in public_policy:
                problems.append(f"classification-public.md: missing {needle!r}")
        for forbidden in (
            "Reports live beside each scheme", "Seed replay in this repository",
            "Reproduction records separate", "harvest.py", "harvests.jsonl",
            "cache outside Git", "checker", "reproduction command",
        ):
            if forbidden in public_policy:
                problems.append(f"classification-public.md: contains internal workflow prose {forbidden!r}")

    manifest_rows = rows(
        ROOT / "source-manifest.csv",
        ("SourcePath", "SourceSHA256", "OutputPath", "OutputSHA256"),
        problems,
    )
    outputs: dict[str, str] = {}
    for line, row in enumerate(manifest_rows, 2):
        relative = row.get("OutputPath", "")
        path = Path(relative)
        if not safe_package_path(relative):
            problems.append(f"source-manifest.csv:{line}: unsafe output path")
            continue
        if not HASH.fullmatch(row.get("SourceSHA256", "")) or not HASH.fullmatch(row.get("OutputSHA256", "")):
            problems.append(f"source-manifest.csv:{line}: invalid hash")
            continue
        if relative in outputs and outputs[relative] != row["OutputSHA256"]:
            problems.append(f"source-manifest.csv:{line}: conflicting output hash for {relative}")
            continue
        outputs[relative] = row["OutputSHA256"]
    for relative, expected in outputs.items():
        path = ROOT / relative
        if not path.is_file():
            problems.append(f"source-manifest.csv: missing output {relative}")
        elif hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            problems.append(f"source-manifest.csv: digest differs for {relative}")
    try:
        snapshot = json.loads((ROOT / "snapshot.json").read_text(encoding="utf-8"))
        if snapshot.get("format") != 1:
            problems.append("snapshot.json: invalid format")
        if not HASH.fullmatch(snapshot.get("input_tree_sha256", "")):
            problems.append("snapshot.json: invalid input tree digest")
        if snapshot.get("candidates") != len(candidate_rows) or snapshot.get("findings") != len(finding_rows):
            problems.append("snapshot.json: inventory counts differ")
    except (OSError, ValueError) as exc:
        problems.append(f"snapshot.json: {exc}")

    if (ROOT / "source.json").exists():
        problems.append("source.json: private-repository metadata must not be published")

    expected_outputs = {
        "candidates.csv", "findings.csv", "classification.md", "classification-public.md",
        "reproducers.csv", "snapshot.json", "sources.json",
        "FAEST/survey_runtime.py", "HAWK/survey_runtime.py", "MAYO/survey_runtime.py",
        "SDitH/kat/cat1_fast.tsv",
    }
    expected_outputs.update(f"reports/{identifier}.md" for identifier in seen)
    missing_outputs = expected_outputs - set(outputs)
    if missing_outputs:
        problems.append("source-manifest.csv: missing publication outputs: " + ", ".join(sorted(missing_outputs)))

    license_path = ROOT / "LICENSE.md"
    if not license_path.is_file():
        problems.append("missing LICENSE.md rights-status notice")
    else:
        license_text = license_path.read_text(encoding="utf-8")
        for needle in (
            "currently grants no additional licence",
            "upstream notices and licence terms",
            "SDitH/kat/cat1_fast.tsv",
            "PQCsignKAT_147.rsp",
        ):
            if needle not in license_text:
                problems.append(f"LICENSE.md: missing rights-status text {needle!r}")

    for identifier, row in reproducers.items():
        relative = row.get("ScriptPath", "")
        if not relative:
            continue
        script = ROOT / relative
        if not script.is_file():
            problems.append(f"reproducers.csv: missing public script for {identifier}: {relative}")
        elif hashlib.sha256(script.read_bytes()).hexdigest() != row.get("ScriptSHA256"):
            problems.append(f"reproducers.csv: public script hash differs for {identifier}")

    allowed_python = {path for path in outputs if path.endswith(".py")} | TRUSTED_PYTHON
    for path in ROOT.rglob("*.py"):
        relative_path = path.relative_to(ROOT)
        if any(part in IGNORED_PYTHON_PARTS for part in relative_path.parts):
            continue
        relative = relative_path.as_posix()
        if relative not in allowed_python:
            problems.append(f"unlisted Python file could affect isolated imports: {relative}")

    for path in ROOT.rglob("*"):
        if not path.is_file() or any(part in {"__pycache__", ".cache"} for part in path.parts):
            continue
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue
        relative = path.relative_to(ROOT)
        if PRIVATE_HTTP.search(text):
            problems.append(f"{relative}: URL exposes private analysis repository")

    for path in [ROOT / "data/submissions.json", *(ROOT / scheme / "provenance.json" for scheme in SCHEMES)]:
        try:
            value = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, ValueError) as exc:
            problems.append(f"{path.relative_to(ROOT)}: {exc}")
            continue
        records = value if isinstance(value, list) else [value]
        if any("retrieved_utc" in record for record in records):
            problems.append(f"{path.relative_to(ROOT)}: contains a nondeterministic retrieval timestamp")

    for problem in problems:
        print(f"FAIL {problem}")
    print(f"NIST publication data: {len(candidate_rows)} candidates, {len(finding_rows)} findings, {len(problems)} problem(s)")
    return int(bool(problems))


if __name__ == "__main__":
    sys.exit(main())
