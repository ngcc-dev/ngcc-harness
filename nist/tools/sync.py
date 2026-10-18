#!/usr/bin/env python3
"""Import reviewed NIST signature reports into the public harness package.

The import is intentionally deterministic: no network access, timestamps, or
Git metadata enter the output.  Use --check in CI to prove that the checked-in
publication snapshot still matches a private analysis checkout.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import io
import json
import posixpath
import re
import subprocess
import sys
from pathlib import Path
from urllib.parse import quote, unquote


PACKAGE = Path(__file__).resolve().parent.parent
SCHEMES = ("SQIsign", "HAWK", "MQOM", "SDitH", "MAYO", "QR-UOV", "SNOVA", "UOV", "FAEST")
FAMILIES = {
    "SQIsign": "Isogeny signatures",
    "HAWK": "Lattice-based signatures",
    "MQOM": "MPC-in-the-Head signatures",
    "SDitH": "MPC-in-the-Head signatures",
    "MAYO": "Multivariate signatures",
    "QR-UOV": "Multivariate signatures",
    "SNOVA": "Multivariate signatures",
    "UOV": "Multivariate signatures",
    "FAEST": "Symmetric-based signatures",
}
PUBLIC_REPOSITORY = "https://github.com/ngcc-dev/ngcc-harness"
PRIVATE_PROJECT_NAME = "nist" + "-sig3"
REPORT_ID = re.compile(r"(?:FAEST|HAWK|MAYO|MQOM|QR-UOV|SDitH|SNOVA|SQIsign|UOV)-[1-9][0-9]*")
MARKDOWN_LINK = re.compile(r"(\[[^\]]*\]\()((?:[^()\n]|\([^()\n]*\))+)(\))")
CANDIDATE_FIELDS = (
    "order", "family", "scheme", "candidate_status", "package_round", "candidate_page",
    "spec_label", "spec_url", "spec_sha256", "archive_label", "archive_url",
    "archive_bytes", "archive_sha256",
)
MANIFEST_FIELDS = ("SourcePath", "SourceSHA256", "OutputPath", "OutputSHA256")
REPRODUCER_FIELDS = (
    "ID", "ScriptPath", "ScriptSHA256", "NeedsOfficialSource",
    "BuildDirectory", "BuildTarget", "Prerequisites",
)
NO_OFFICIAL_SOURCE = {"HAWK-1", "UOV-2", "UOV-3", "UOV-4"}
BUILD = {
    "FAEST-1": ("FAEST", "libs"),
    "QR-UOV-3": ("QR-UOV", "libs"),
    "SDitH-3": ("SDitH", "libs"),
    "SNOVA-6": ("SNOVA", "libs"),
    "SQIsign-1": ("SQIsign", "libs"),
}
PREREQUISITES = {
    "FAEST-1": "Python 3; pycryptodome; C compiler; make; OpenSSL development files",
    "HAWK-1": "Python 3; harvested public evidence for complete verification",
    "MQOM-2": "Python 3; numpy; numba; pycryptodome; joblib",
    "QR-UOV-3": "C compiler; make; OpenSSL development files",
    "SDitH-2": "C compiler; make; cmake; pdftotext; OpenSSL development files",
    "SDitH-3": "C compiler; make; OpenSSL development files",
    "SNOVA-1": "C compiler",
    "SNOVA-6": "C compiler; make; OpenSSL development files",
    "SNOVA-7": "C compiler; OpenSSL development files",
    "SQIsign-1": "C compiler; make; GMP development files",
    "SQIsign-3": "C compiler; GMP development files",
    "SQIsign-4": "pdftotext",
}


def public_inputs(source: Path) -> list[str]:
    """Files needed to inspect, fetch, build, and run the public witnesses."""
    paths = {
        "api/Makefile",
        "api/nist_sig.c",
        "api/nist_sig_api.h",
        "api/nist_sig_shim.c",
        "doc/NIST-rules.md",
        "FAEST/survey_runtime.py",
        "HAWK/survey_runtime.py",
        "MAYO/survey_runtime.py",
        "SDitH/kat/cat1_fast.tsv",
        "security/artifacts/SNOVA-LPN.json",
        "security/artifacts/SNOVA-LPN.md",
        "security/harvest.py",
        "security/harvests.jsonl",
        "security/reproduce_hawk_1.py",
        "security/reproduce_snova_upstream.py",
        "security/reproductions/SNOVA-1.json",
        "security/sources.json",
    }
    for scheme in SCHEMES:
        directory = source / scheme
        for name in (
            "Makefile", "libraries.json", "internal-parameters.json",
            "parameters.csv", "alternative-parameters.csv", "provenance.json",
        ):
            if (directory / name).is_file():
                paths.add(f"{scheme}/{name}")
        for pattern in ("reproduce_*.py", "analysis/**/*.py", "analysis/**/*.sage", "analysis/**/README.md"):
            paths.update(path.relative_to(source).as_posix() for path in directory.glob(pattern) if path.is_file())
    missing = [path for path in sorted(paths) if not (source / path).is_file()]
    if missing:
        raise SystemExit("missing public harness input(s): " + ", ".join(missing))
    return sorted(paths)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def publication_text(data: bytes) -> bytes:
    """Normalize line endings and trailing whitespace in published text."""
    text = data.decode("utf-8")
    return ("\n".join(line.rstrip() for line in text.splitlines()) + "\n").encode("utf-8")


def public_wording(text: str) -> str:
    """Name the public harness directly instead of referring to a source checkout."""
    for old, new in (
        ("This repository's", "The public harness's"),
        ("this repository's", "the public harness's"),
        ("The repository's", "The public harness's"),
        ("the repository's", "the public harness's"),
        ("This repository", "The public harness"),
        ("this repository", "the public harness"),
        ("The repository", "The public harness"),
        ("the repository", "the public harness"),
    ):
        text = text.replace(old, new)
    return text


def csv_bytes(fields: tuple[str, ...], rows: list[dict[str, str]]) -> bytes:
    target = io.StringIO(newline="")
    writer = csv.DictWriter(target, fieldnames=fields, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    return target.getvalue().encode("utf-8")


def validate_source(source: Path) -> None:
    required = (
        source / "data/submissions.json",
        source / "security/findings.csv",
        source / "security/methodology.md",
        source / "security/sources.json",
    )
    missing = [str(path) for path in required if not path.is_file()]
    if missing:
        raise SystemExit("missing source input(s): " + ", ".join(missing))


def source_state(source: Path) -> tuple[str, str]:
    """Return source HEAD and exact porcelain state for before/after checks."""
    try:
        top = subprocess.run(
            ["git", "-C", str(source), "rev-parse", "--show-toplevel"],
            check=True, capture_output=True, text=True,
        ).stdout.strip()
        if Path(top).resolve() != source:
            raise SystemExit(f"source must be the Git worktree root: {source}")
        revision = subprocess.run(
            ["git", "-C", str(source), "rev-parse", "--verify", "HEAD"],
            check=True, capture_output=True, text=True,
        ).stdout.strip()
        status = subprocess.run(
            ["git", "-C", str(source), "status", "--porcelain=v1", "--untracked-files=all"],
            check=True, capture_output=True, text=True,
        ).stdout
    except (OSError, subprocess.CalledProcessError) as error:
        raise SystemExit(f"cannot inspect source Git state: {error}") from error
    return revision, status


def candidates(source: Path) -> bytes:
    submissions = json.loads((source / "data/submissions.json").read_text(encoding="utf-8"))
    by_scheme = {row["scheme"]: row for row in submissions}
    if set(by_scheme) != set(SCHEMES):
        raise SystemExit("data/submissions.json does not contain exactly the nine expected schemes")
    rows: list[dict[str, str]] = []
    round3_page = "https://csrc.nist.gov/Projects/pqc-dig-sig/round-3-additional-signatures"
    for order, scheme in enumerate(SCHEMES, 1):
        item = by_scheme[scheme]
        withdrawn = scheme == "HAWK"
        rows.append({
            "order": str(order),
            "family": FAMILIES[scheme],
            "scheme": scheme,
            "candidate_status": "withdrawn" if withdrawn else "round-3 candidate",
            "package_round": str(item["round"]),
            "candidate_page": round3_page,
            "spec_label": "Round 2 PDF" if withdrawn else "PDF",
            "spec_url": item["spec_url"],
            "spec_sha256": item["spec_sha256"],
            "archive_label": "Round 2 ZIP" if withdrawn else "ZIP",
            "archive_url": item["archive_url"],
            "archive_bytes": str(item["archive_bytes"]),
            "archive_sha256": item["archive_sha256"],
        })
    return csv_bytes(CANDIDATE_FIELDS, rows)


CLASSIFICATION_INTRODUCTION = """# NIST additional-signature report classification

This policy is specific to the NIST additional digital signature process. It
uses the required EUF-CMA model, claimed security categories, and evaluation
criteria on the official [NIST call page](https://csrc.nist.gov/Projects/pqc-dig-sig/standardization/call-for-proposals)
and in the [call PDF](https://csrc.nist.gov/csrc/media/Projects/pqc-dig-sig/documents/call-for-proposals-dig-sig-sept-2022.pdf).
The call permits security estimates to assume at most `2^64` chosen-message
signing queries. Ratings here are independent project assessments, not ratings issued by NIST.
NGCC findings may supply prior evidence, but their ratings do not transfer
automatically.
"""


def classification(source: Path) -> bytes:
    policy = (source / "security/methodology.md").read_text(encoding="utf-8")
    if policy.startswith("# Security reports and reproducibility\n"):
        policy = policy.removeprefix("# Security reports and reproducibility\n")
    introduction = CLASSIFICATION_INTRODUCTION
    return public_wording(introduction.rstrip() + "\n\n" + policy.lstrip()).encode("utf-8")


def public_classification(source: Path) -> bytes:
    """Extract stable public policy while excluding repository workflow prose."""
    policy = (source / "security/methodology.md").read_text(encoding="utf-8")
    start_marker = "Each numbered report carries"
    end_marker = "Reproduction records separate the following evidence:"
    start = policy.find(start_marker)
    end = policy.find(end_marker)
    if start < 0 or end < 0 or end <= start:
        raise SystemExit("security/methodology.md lacks public-policy boundaries")
    body = policy[start:end].rstrip()
    body = body.replace(
        "Seed replay\nin this repository is a KAT control, not an attacker capability.",
        "Seed replay during testing is a control, not an attacker capability.",
    )
    body = body.replace("this pinned version", "the evaluated submission version")
    body = body.replace("the pinned submission", "the evaluated submission")
    return public_wording(
        CLASSIFICATION_INTRODUCTION.rstrip() + "\n\n" + body + "\n"
    ).encode("utf-8")


def source_path_for_link(scheme: str, target: str) -> str:
    path, separator, fragment = target.partition("#")
    decoded = unquote(path)
    normalized = posixpath.normpath(posixpath.join(scheme, decoded))
    if normalized == ".." or normalized.startswith("../") or normalized.startswith("/"):
        raise ValueError(f"report link escapes the publication root: {target!r}")
    suffix = f"#{fragment}" if separator else ""
    return quote(normalized, safe="/") + suffix


def normalize_report(
    text: str, scheme: str, spec_url: str, identifier: str, has_reproducer: bool,
    published: set[str],
) -> str:
    """Resolve publication links without exposing the private analysis checkout."""
    # Public reports are new files in the harness repository, so Markdown hard
    # breaks copied from legacy metadata would make `git diff --check` fail on
    # every such line. Metadata is rendered as separate paragraphs/tables by
    # the site and does not need trailing spaces.
    text = "\n".join(line.rstrip() for line in text.splitlines()) + "\n"
    text = public_wording(text)

    def replace(match: re.Match[str]) -> str:
        target = match.group(2)
        if target.startswith(("https://", "http://", "mailto:", "#")):
            if re.search(r"https?://[^\s)]*" + re.escape(PRIVATE_PROJECT_NAME), target, re.IGNORECASE):
                return match.group(1).removesuffix("(") + match.group(3).removeprefix(")")
            return match.group(0)
        path = unquote(target.split("#", 1)[0])
        if REPORT_ID.fullmatch(Path(path).stem) and Path(path).suffix == ".md":
            return match.group(1) + f"/nist/reports/{Path(path).stem}.html" + match.group(3)
        if Path(path).name == f"{scheme}-spec.pdf":
            return match.group(1) + spec_url + match.group(3)
        repository_path = source_path_for_link(scheme, target)
        plain_path, separator, fragment = repository_path.partition("#")
        if unquote(plain_path) in published:
            return (
                match.group(1) + f"{PUBLIC_REPOSITORY}/blob/main/nist/{repository_path}"
                + match.group(3)
            )
        label = match.group(1)[1:-2]
        display = unquote(plain_path) + (f"#{fragment}" if separator else "")
        return f"{label} (`{display}`)"

    normalized = MARKDOWN_LINK.sub(replace, text)
    if has_reproducer:
        harness_note = (
            "\n## Harness reproduction\n\n"
            "From the `ngcc-harness` checkout, run the pinned source script through "
            "the harness-side dispatcher:\n\n"
            "```sh\n"
            f"python3 nist/reproduce.py {identifier}\n"
            "```\n"
        )
        if identifier == "HAWK-1":
            harness_note += (
                "\nThe retained script's algebra and cost-model subchecks can pass, but "
                "the dispatcher returns SKIP (77) because it does not execute the "
                "claimed end-to-end SVP key-recovery pipeline.\n"
            )
    else:
        harness_note = (
            "\n## Harness reproduction\n\n"
            "This historical or withdrawn-claim report has no retained automated "
            "reproducer. `python3 nist/reproduce.py " + identifier +
            "` reports that limit explicitly.\n"
        )
    normalized = normalized.rstrip() + "\n" + harness_note
    return normalized if normalized.endswith("\n") else normalized + "\n"


def desired_outputs(source: Path) -> tuple[dict[str, bytes], list[dict[str, str]]]:
    validate_source(source)
    raw_submissions = (source / "data/submissions.json").read_bytes()
    submissions = json.loads(raw_submissions)
    public_submissions = [
        {key: value for key, value in item.items() if key != "retrieved_utc"}
        for item in submissions
    ]
    specs = {item["scheme"]: item["spec_url"] for item in submissions}
    with (source / "security/findings.csv").open(encoding="utf-8", newline="") as handle:
        findings = list(csv.DictReader(handle))
    published_paths = public_inputs(source)
    published = set(published_paths)
    reproducer_rows: list[dict[str, str]] = []
    reproducer_inputs: list[tuple[str, bytes]] = []
    for row in findings:
        path = row["Reproducer"]
        if not path:
            reproducer_rows.append({
                "ID": row["ID"], "ScriptPath": "", "ScriptSHA256": "",
                "NeedsOfficialSource": "no", "BuildDirectory": "", "BuildTarget": "",
                "Prerequisites": "",
            })
            continue
        script = source / path
        if not script.is_file():
            raise SystemExit(f"missing source reproducer: {script}")
        content = script.read_bytes()
        build_directory, build_target = BUILD.get(row["ID"], ("", ""))
        reproducer_rows.append({
            "ID": row["ID"], "ScriptPath": path, "ScriptSHA256": digest(content),
            "NeedsOfficialSource": "no" if row["ID"] in NO_OFFICIAL_SOURCE else "yes",
            "BuildDirectory": build_directory, "BuildTarget": build_target,
            "Prerequisites": PREREQUISITES.get(row["ID"], "Python 3"),
        })
        reproducer_inputs.append((path, content))
    outputs: dict[str, bytes] = {
        "candidates.csv": candidates(source),
        "data/submissions.json": (
            json.dumps(public_submissions, indent=2, ensure_ascii=False) + "\n"
        ).encode("utf-8"),
        "findings.csv": (source / "security/findings.csv").read_bytes(),
        "classification.md": classification(source),
        "classification-public.md": public_classification(source),
        "reproducers.csv": csv_bytes(REPRODUCER_FIELDS, reproducer_rows),
        "sources.json": (
            json.dumps(
                json.loads((source / "security/sources.json").read_text(encoding="utf-8")),
                indent=2,
                ensure_ascii=False,
            ) + "\n"
        ).encode("utf-8"),
    }
    manifest: list[dict[str, str]] = [
        {
            "SourcePath": "data/submissions.json",
            "SourceSHA256": digest(raw_submissions),
            "OutputPath": "candidates.csv",
            "OutputSHA256": digest(outputs["candidates.csv"]),
        },
        {
            "SourcePath": "data/submissions.json",
            "SourceSHA256": digest(raw_submissions),
            "OutputPath": "data/submissions.json",
            "OutputSHA256": digest(outputs["data/submissions.json"]),
        },
        {
            "SourcePath": "security/findings.csv",
            "SourceSHA256": digest((source / "security/findings.csv").read_bytes()),
            "OutputPath": "findings.csv",
            "OutputSHA256": digest(outputs["findings.csv"]),
        },
        {
            "SourcePath": "security/methodology.md",
            "SourceSHA256": digest((source / "security/methodology.md").read_bytes()),
            "OutputPath": "classification.md",
            "OutputSHA256": digest(outputs["classification.md"]),
        },
        {
            "SourcePath": "security/methodology.md",
            "SourceSHA256": digest((source / "security/methodology.md").read_bytes()),
            "OutputPath": "classification-public.md",
            "OutputSHA256": digest(outputs["classification-public.md"]),
        },
        {
            "SourcePath": "security/sources.json",
            "SourceSHA256": digest((source / "security/sources.json").read_bytes()),
            "OutputPath": "sources.json",
            "OutputSHA256": digest(outputs["sources.json"]),
        },
    ]
    for path, content in reproducer_inputs:
        manifest.append({
            "SourcePath": path,
            "SourceSHA256": digest(content),
            "OutputPath": "reproducers.csv",
            "OutputSHA256": digest(outputs["reproducers.csv"]),
        })
    for path in published_paths:
        source_content = (source / path).read_bytes()
        content = source_content
        if path == "security/reproduce_snova_upstream.py":
            old_agent = b"nist" + b"-sig3-pinned-reproducer"
            content = content.replace(old_agent, b"ngcc-nist-pinned-reproducer")
        if path == "security/harvest.py":
            old_agent = b"nist" + b"-sig3-research-harvester/1.0"
            content = content.replace(old_agent, b"ngcc-nist-research-harvester/1.0")
            old_log = b'LOG = ROOT / "security/harvests.jsonl"\n'
            new_log = (
                b'BUNDLED_LOG = ROOT / "security/harvests.jsonl"\n'
                b'LOG = ROOT / ".cache/security/harvests.jsonl"\n'
            )
            if old_log not in content:
                raise SystemExit("cannot separate the public runtime harvest log")
            content = content.replace(old_log, new_log)
            old_records = (
                b'    records = [json.loads(line) for line in LOG.read_text().splitlines() if line] if LOG.exists() else []\n'
            )
            new_records = (
                b'    records = []\n'
                b'    for log in (BUNDLED_LOG, LOG):\n'
                b'        if log.exists():\n'
                b'            records.extend(json.loads(line) for line in log.read_text().splitlines() if line)\n'
            )
            if old_records not in content:
                raise SystemExit("cannot update public harvest-log verification")
            content = content.replace(old_records, new_records)
        if path == "security/reproduce_hawk_1.py":
            old_evidence = (
                b'    path = ROOT / "security/harvests.jsonl"\n'
                b'    records = [json.loads(line) for line in path.read_text().splitlines() if line] if path.exists() else []\n'
            )
            new_evidence = (
                b'    records = []\n'
                b'    for path in (\n'
                b'        ROOT / "security/harvests.jsonl",\n'
                b'        ROOT / ".cache/security/harvests.jsonl",\n'
                b'    ):\n'
                b'        if path.exists():\n'
                b'            records.extend(json.loads(line) for line in path.read_text().splitlines() if line)\n'
            )
            if old_evidence not in content:
                raise SystemExit("cannot add runtime harvest log to the HAWK-1 witness")
            content = content.replace(old_evidence, new_evidence)
            old_status = (
                b'    result["status"] = "FAIL" if failed else "SKIP" if skipped and args.require_cache else "PASS"\n'
                b'    print(json.dumps(result, indent=2))\n'
                b'    return 1 if failed else 77 if skipped and args.require_cache else 0\n'
            )
            new_status = (
                b'    result["status"] = "FAIL" if failed else "SKIP"\n'
                b'    print(json.dumps(result, indent=2))\n'
                b'    return 1 if failed else 77\n'
            )
            if old_status not in content:
                raise SystemExit("cannot enforce complete-witness HAWK-1 status")
            content = content.replace(old_status, new_status)
        if path.endswith("/provenance.json"):
            provenance = json.loads(content.decode("utf-8"))
            provenance.pop("retrieved_utc", None)
            content = (json.dumps(provenance, indent=2, ensure_ascii=False) + "\n").encode("utf-8")
        content = publication_text(content)
        outputs[path] = content
        manifest.append({
            "SourcePath": path,
            "SourceSHA256": digest(source_content),
            "OutputPath": path,
            "OutputSHA256": digest(content),
        })
    for row in reproducer_rows:
        if row["ScriptPath"]:
            row["ScriptSHA256"] = digest(outputs[row["ScriptPath"]])
    outputs["reproducers.csv"] = csv_bytes(REPRODUCER_FIELDS, reproducer_rows)
    for item in manifest:
        if item["OutputPath"] == "reproducers.csv":
            item["OutputSHA256"] = digest(outputs["reproducers.csv"])
    for row in findings:
        report_source = source / row["Report"]
        if not report_source.is_file():
            raise SystemExit(f"missing source report: {report_source}")
        output_path = f"reports/{row['ID']}.md"
        report = normalize_report(
            report_source.read_text(encoding="utf-8"), row["Scheme"], specs[row["Scheme"]],
            row["ID"], bool(row["Reproducer"]), published,
        )
        outputs[output_path] = report.encode("utf-8")
        manifest.append({
            "SourcePath": row["Report"],
            "SourceSHA256": digest(report_source.read_bytes()),
            "OutputPath": output_path,
            "OutputSHA256": digest(outputs[output_path]),
        })
    input_digest = digest(csv_bytes(
        ("SourcePath", "SourceSHA256"),
        [
            {"SourcePath": row["SourcePath"], "SourceSHA256": row["SourceSHA256"]}
            for row in sorted(manifest, key=lambda item: (item["SourcePath"], item["OutputPath"]))
        ],
    ))
    outputs["snapshot.json"] = (
        json.dumps({
            "format": 1,
            "input_tree_sha256": input_digest,
            "candidates": len(SCHEMES),
            "findings": len(findings),
        }, indent=2) + "\n"
    ).encode("utf-8")
    manifest.append({
        "SourcePath": "content-digest:inputs",
        "SourceSHA256": input_digest,
        "OutputPath": "snapshot.json",
        "OutputSHA256": digest(outputs["snapshot.json"]),
    })
    outputs["source-manifest.csv"] = csv_bytes(MANIFEST_FIELDS, manifest)
    return outputs, manifest


def check_outputs(outputs: dict[str, bytes]) -> int:
    problems: list[str] = []
    managed = {"candidates.csv", "findings.csv", "classification.md", "classification-public.md", "reproducers.csv", "snapshot.json", "sources.json", "source-manifest.csv"}
    managed.update(path for path in outputs if path.startswith("reports/"))
    for relative, expected in sorted(outputs.items()):
        path = PACKAGE / relative
        if not path.is_file():
            problems.append(f"missing {relative}")
        elif path.read_bytes() != expected:
            problems.append(f"stale {relative}")
    report_dir = PACKAGE / "reports"
    if report_dir.is_dir():
        for path in report_dir.glob("*.md"):
            relative = path.relative_to(PACKAGE).as_posix()
            if relative not in managed:
                problems.append(f"unexpected managed report {relative}")
    for problem in problems:
        print(f"FAIL {problem}")
    report_count = sum(path.startswith("reports/") for path in outputs)
    print(f"NIST publication sync: {report_count} reports, {len(problems)} problem(s)")
    return int(bool(problems))


def write_outputs(outputs: dict[str, bytes]) -> None:
    legacy = PACKAGE / "source.json"
    if legacy.exists():
        legacy.unlink()
    (PACKAGE / "reports").mkdir(parents=True, exist_ok=True)
    expected_reports = {path for path in outputs if path.startswith("reports/")}
    for path in (PACKAGE / "reports").glob("*.md"):
        if path.relative_to(PACKAGE).as_posix() not in expected_reports:
            path.unlink()
    for relative, content in sorted(outputs.items()):
        path = PACKAGE / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(content)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path, help="private analysis checkout")
    parser.add_argument("--check", action="store_true", help="compare without writing")
    parser.add_argument(
        "--allow-dirty", action="store_true",
        help="explicitly publish a reviewed dirty worktree; snapshot remains content-addressed",
    )
    args = parser.parse_args()
    source = args.source.resolve()
    revision_before, status_before = source_state(source)
    if status_before and not args.allow_dirty:
        raise SystemExit(
            "source worktree or index is dirty; commit it first or pass --allow-dirty "
            "only for an intentionally reviewed content-addressed snapshot"
        )
    outputs, _ = desired_outputs(source)
    revision_after, status_after = source_state(source)
    if revision_after != revision_before or status_after != status_before:
        raise SystemExit("source Git state changed while the publication snapshot was generated")
    if args.check:
        return check_outputs(outputs)
    write_outputs(outputs)
    report_count = sum(path.startswith("reports/") for path in outputs)
    print(f"NIST publication sync: wrote {report_count} reports from local analysis inputs")
    return 0


if __name__ == "__main__":
    sys.exit(main())
