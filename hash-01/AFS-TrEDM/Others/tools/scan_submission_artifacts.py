#!/usr/bin/env python3
"""Scan an S6 submission tree for generated artifacts that should not ship."""

from __future__ import annotations

import argparse
import json
import stat
from pathlib import Path


FINALIZATION_ZIP_PREFIXES = (
    "AFS-TrEDM_Submission_S6-Stage06A_",
    "AFS-TrEDM_Submission_S6-Stage06B_",
    "AFS-TrEDM_Submission_S6-Stage06C_",
    "AFS-TrEDM_Submission_S6-Stage06D_",
    "AFS-TrEDM_Submission_S6-Stage06E_",
    "AFS-TrEDM_Submission_S6-Stage06F_",
    "AFS-TrEDM_Submission_S6-Stage06G_",
    "AFS-TrEDM_Submission_S6-Stage07A_",
    "AFS-TrEDM_Submission_S6-Stage07B_",
    "AFS-TrEDM-S6_Submission_DryRun_Stage06G",
    "AFS-TrEDM_Submission_S6-Stage11_SpecKATPerfFinal",
    "AFS-TrEDM_Submission_S6-Stage12_PerfTablesSpecSync",
    "AFS-TrEDM_Submission_S6-Stage16_StrictCleanupFinal",
)
CURRENT_FINAL_ZIP_RELATIVE_PATHS = set()

GENERATED_FILE_EXACT = {
    "kat",
    "kat_perf",
    "quick_test",
    "quick_test_perf",
    "quick_test_avx2",
    "quick_test_avx2_perf",
    "quick_test_dispatch",
    "quick_test_dispatch_perf",
    "hash_cli",
    "hash_cli_perf",
    "hash_cli_avx2",
    "hash_cli_avx2_perf",
    "hash_cli_dispatch",
    "hash_cli_dispatch_perf",
    "benchmark",
    "benchmark_perf",
    "benchmark_avx2",
    "benchmark_avx2_perf",
    "benchmark_dispatch",
    "benchmark_dispatch_perf",
    "profile_info",
    "profile_info_perf",
    "profile_info_avx2",
    "microbench_rounds",
    "microbench_rounds_opt64",
    "batch_quick_test",
    "batch_quick_test_perf",
    "batch_quick_test_fallback",
    "batch_quick_test_fallback_perf",
    "batch_benchmark",
    "batch_benchmark_perf",
    "batch_benchmark_fallback",
    "batch_benchmark_fallback_perf",
    "batchmany_benchmark",
    "batchmany_benchmark_perf",
    "batch16_avx512_selftest",
    "quick_test_batch16_avx512",
    "quick_test_batch16_avx512_perf",
    "benchmark_batch16_avx512",
    "benchmark_batch16_avx512_perf",
}

GENERATED_PREFIXES = (
    "batch_hardening_test_",
    "kat_avx2_",
    "quick_test_avx2_",
    "hash_cli_avx2_",
    "benchmark_avx2_",
    "kat_avx512_",
    "quick_test_avx512_",
    "hash_cli_avx512_",
    "benchmark_avx512_",
    "benchmark_native",
    "benchmark_avx2_native",
    "batch_microbench",
)

GENERATED_SUFFIXES = (".o", ".su", ".gcda", ".gcno", ".pyc")
GENERATED_DIR_NAMES = {
    "output",
    "output_performance",
    ".dispatch_build",
    ".dispatch_build_avx2",
    ".dispatch_build_portable",
    ".dispatch_perf_build",
    ".dispatch_perf_build_avx2",
    ".dispatch_perf_build_portable",
    "__pycache__",
}
ALLOWED_EXECUTABLE_SUFFIXES = {".sh", ".py"}


def rel(path: Path, root: Path) -> str:
    return str(path.relative_to(root))


def is_archive_path(path: Path, root: Path) -> bool:
    try:
        return "archive" in path.relative_to(root).parts
    except ValueError:
        return False


def is_finalization_zip(path: Path, root: Path) -> bool:
    try:
        rp = path.relative_to(root)
    except ValueError:
        return False
    rel_path = rp.as_posix()
    return (
        rel_path in CURRENT_FINAL_ZIP_RELATIVE_PATHS
        or len(rp.parts) == 1
        and any(path.name.startswith(prefix) for prefix in FINALIZATION_ZIP_PREFIXES)
    )


def generated_file_reason(path: Path) -> str | None:
    name = path.name
    if name in GENERATED_FILE_EXACT:
        return "generated executable"
    if any(name.startswith(prefix) for prefix in GENERATED_PREFIXES):
        return "generated executable"
    if any(name.endswith(suffix) for suffix in GENERATED_SUFFIXES):
        return "compiler/interpreter byproduct"
    if name.endswith(".zip"):
        return "zip file inside implementation tree"
    if name.startswith("LWC_HASH_KAT_") and name.endswith(".txt"):
        return "stale KAT vector in implementation tree"
    if name.startswith("KAT_") and name.endswith(".txt"):
        return "stale KAT vector in implementation tree"
    return None


def generated_dir_reason(path: Path) -> str | None:
    name = path.name
    if name in GENERATED_DIR_NAMES or name.startswith("output_") or name.startswith(".dispatch_build_") or name.startswith(".dispatch_perf_build_"):
        return "generated output/build directory"
    return None


def is_unexpected_executable(path: Path) -> bool:
    if path.suffix in ALLOWED_EXECUTABLE_SUFFIXES:
        return False
    mode = path.stat().st_mode
    return bool(mode & (stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", required=True)
    parser.add_argument("--json-out")
    args = parser.parse_args()

    root = Path(args.root).resolve()
    impl = root / "Implementations"
    findings: list[dict[str, str]] = []

    if not impl.exists():
        findings.append({"kind": "missing_tree", "path": "Implementations", "reason": "missing implementation tree"})
    else:
        for path in sorted(impl.rglob("*")):
            if is_archive_path(path, root):
                continue
            if path.is_dir():
                reason = generated_dir_reason(path)
                if reason is not None:
                    findings.append({"kind": "generated_dir", "path": rel(path, root), "reason": reason})
                continue
            if not path.is_file():
                continue
            reason = generated_file_reason(path)
            if reason is not None:
                findings.append({"kind": "generated_file", "path": rel(path, root), "reason": reason})
            elif is_unexpected_executable(path):
                findings.append({"kind": "unexpected_executable", "path": rel(path, root), "reason": "executable bit on non-script file"})

    for zip_path in sorted(root.rglob("*.zip")):
        if is_archive_path(zip_path, root):
            continue
        if is_finalization_zip(zip_path, root):
            continue
        findings.append({"kind": "historical_zip", "path": rel(zip_path, root), "reason": "non-finalization zip outside archive"})

    result = {
        "status": "PASS" if not findings else "FAIL",
        "finding_count": len(findings),
        "findings": findings,
    }
    text = json.dumps(result, indent=2, sort_keys=True)
    print(text)
    if args.json_out:
        Path(args.json_out).write_text(text + "\n", encoding="utf-8")
    return 0 if not findings else 1


if __name__ == "__main__":
    raise SystemExit(main())
