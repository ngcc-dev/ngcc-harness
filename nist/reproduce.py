#!/usr/bin/env python3
"""Verify and run one published NIST finding reproducer."""
from __future__ import annotations

import argparse
import csv
import hashlib
import re
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent
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
FINDING_ID = re.compile(r"(FAEST|HAWK|MAYO|MQOM|QR-UOV|SDitH|SNOVA|SQIsign|UOV)-([1-9][0-9]*)")
REPRODUCER_FIELDS = (
    "ID", "ScriptPath", "ScriptSHA256", "NeedsOfficialSource",
    "BuildDirectory", "BuildTarget", "Prerequisites",
)


def load() -> dict[str, dict[str, str]]:
    with (ROOT / "reproducers.csv").open(encoding="utf-8", newline="") as source:
        reader = csv.DictReader(source)
        if tuple(reader.fieldnames or ()) != REPRODUCER_FIELDS:
            raise ValueError("reproducers.csv has an unsupported schema")
        rows = list(reader)
    if any(None in row or any(value is None for value in row.values()) for row in rows):
        raise ValueError("reproducers.csv contains a ragged row")
    result: dict[str, dict[str, str]] = {}
    for row in rows:
        identifier = row["ID"]
        match = FINDING_ID.fullmatch(identifier)
        if not match:
            raise ValueError(f"reproducers.csv has invalid finding ID: {identifier!r}")
        if identifier in result:
            raise ValueError(f"reproducers.csv has duplicate finding ID: {identifier}")
        scheme = match.group(1)
        relative = row["ScriptPath"]
        if relative:
            path = Path(relative)
            if (path.is_absolute() or ".." in path.parts or "." in path.parts
                    or path.as_posix() != relative or path.suffix != ".py"):
                raise ValueError(f"reproducers.csv has unsafe ScriptPath: {relative!r}")
            allowed = (
                len(path.parts) == 2
                and path.parts[0] == scheme
                and path.name.startswith("reproduce_")
            ) or (identifier == "HAWK-1" and relative == "security/reproduce_hawk_1.py")
            if not allowed:
                raise ValueError(f"reproducers.csv has unexpected ScriptPath for {identifier}")
            if not row["ScriptSHA256"] or not row["Prerequisites"]:
                raise ValueError(f"reproducers.csv has incomplete entry for {identifier}")
        elif row["ScriptSHA256"]:
            raise ValueError(f"reproducers.csv has hash without script for {identifier}")
        if row["NeedsOfficialSource"] not in {"yes", "no"}:
            raise ValueError(f"reproducers.csv has invalid source requirement for {identifier}")
        if bool(row["BuildDirectory"]) != bool(row["BuildTarget"]):
            raise ValueError(f"reproducers.csv has incomplete build recipe for {identifier}")
        if row["BuildDirectory"] not in {"", scheme} or row["BuildTarget"] not in {"", "libs"}:
            raise ValueError(f"reproducers.csv has unsafe build recipe for {identifier}")
        result[identifier] = row
    return result


def verify_public_inputs() -> set[str]:
    """Authenticate the complete generated witness-input tree before execution."""
    manifest = ROOT / "source-manifest.csv"
    with manifest.open(encoding="utf-8", newline="") as source:
        reader = csv.DictReader(source)
        if tuple(reader.fieldnames or ()) != (
            "SourcePath", "SourceSHA256", "OutputPath", "OutputSHA256",
        ):
            raise ValueError("source-manifest.csv has an unsupported schema")
        rows = list(reader)
    expected: dict[str, str] = {}
    for row in rows:
        if None in row or any(value is None for value in row.values()):
            raise ValueError("source-manifest.csv contains a ragged row")
        relative = row["OutputPath"]
        path = Path(relative)
        if not relative or path.is_absolute() or ".." in path.parts:
            raise ValueError(f"unsafe manifest output path: {relative!r}")
        previous = expected.setdefault(relative, row["OutputSHA256"])
        if previous != row["OutputSHA256"]:
            raise ValueError(f"conflicting hashes for manifest output: {relative}")
    for relative, wanted in expected.items():
        path = ROOT / relative
        try:
            actual = hashlib.sha256(path.read_bytes()).hexdigest()
        except OSError as exc:
            raise ValueError(f"cannot read manifest input {relative}: {exc}") from exc
        if actual != wanted:
            raise ValueError(f"manifest hash differs for {relative}")
    allowed_python = {path for path in expected if path.endswith(".py")} | TRUSTED_PYTHON
    unexpected = []
    for path in ROOT.rglob("*.py"):
        relative_path = path.relative_to(ROOT)
        if any(part in IGNORED_PYTHON_PARTS for part in relative_path.parts):
            continue
        relative = relative_path.as_posix()
        if relative not in allowed_python:
            unexpected.append(relative)
    if unexpected:
        raise ValueError(
            "unlisted Python file(s) could affect isolated imports: "
            + ", ".join(sorted(unexpected))
        )
    return set(expected)


def isolated_command(script: Path, forwarded: list[str]) -> list[str]:
    bootstrap = (
        "import sys;"
        "script=sys.argv[1];"
        "sys.path.extend(sys.argv[2:4]);"
        "sys.argv=[script]+sys.argv[4:];"
        "source=sys.stdin.buffer.read();"
        "scope={'__name__':'__main__','__file__':script,'__package__':None,"
        "'__cached__':None};"
        "exec(compile(source,script,'exec'),scope,scope)"
    )
    return [
        sys.executable, "-I", "-c", bootstrap,
        str(script), str(script.parent), str(ROOT), *forwarded,
    ]


def main() -> int:
    parser = argparse.ArgumentParser(
        description="hash-check and run a numbered NIST signature reproducer",
    )
    parser.add_argument("finding", nargs="?", help="stable finding ID, for example QR-UOV-3")
    parser.add_argument("--list", action="store_true", help="list finding IDs and retained scripts")
    args, forwarded = parser.parse_known_args()
    try:
        entries = load()
        verify_public_inputs()
    except (OSError, ValueError) as exc:
        print(f"FAIL public harness integrity: {exc}", file=sys.stderr)
        return 1
    if args.list:
        for identifier, row in entries.items():
            print(f"{identifier}\t{row['ScriptPath'] or '-'}")
        return 0
    if not args.finding:
        parser.error("finding is required unless --list is used")
    if args.finding not in entries:
        parser.error(f"unknown finding ID: {args.finding}")
    row = entries[args.finding]
    if not row["ScriptPath"]:
        print(f"SKIP {args.finding}: the report has no retained automated reproducer")
        return 77
    if row["NeedsOfficialSource"] == "yes" and not (ROOT / row["ID"].rsplit("-", 1)[0] / "submission").is_dir():
        print(
            f"SKIP {args.finding}: official submitted source is absent; "
            "run `make -C nist fetch` from the ngcc-harness parent",
            file=sys.stderr,
        )
        return 77
    script = ROOT / row["ScriptPath"]
    try:
        content = script.read_bytes()
    except OSError as exc:
        print(f"FAIL {args.finding}: cannot read {script}: {exc}", file=sys.stderr)
        return 1
    actual = hashlib.sha256(content).hexdigest()
    if actual != row["ScriptSHA256"]:
        print(
            f"FAIL {args.finding}: source script hash differs\n"
            f"  expected {row['ScriptSHA256']}\n  actual   {actual}\n  path     {script}",
            file=sys.stderr,
        )
        return 1
    forwarded = list(forwarded)
    if forwarded and forwarded[0] == "--":
        forwarded.pop(0)
    if row["BuildTarget"]:
        build = ["make", "-C", str(ROOT / row["BuildDirectory"]), row["BuildTarget"]]
        print(f"BUILD {args.finding}: {' '.join(build)}", flush=True)
        result = subprocess.run(build, cwd=ROOT, check=False)
        if result.returncode:
            print(
                f"FAIL {args.finding}: build failed; prerequisites: {row['Prerequisites']}",
                file=sys.stderr,
            )
            return 1
    command = isolated_command(script, forwarded)
    print(f"RUN {args.finding}: {row['ScriptPath']} ({actual})", flush=True)
    # Feed the already authenticated bytes to the isolated interpreter. The
    # child never reopens the entry script, closing the hash/execute race.
    return subprocess.run(command, cwd=ROOT, input=content, check=False).returncode


if __name__ == "__main__":
    sys.exit(main())
