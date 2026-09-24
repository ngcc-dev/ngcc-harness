#!/usr/bin/env python3
"""Build, run, and render S6 live performance tables."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import secrets
import shutil
import socket
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

THIS_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(THIS_DIR))

from live_perf_parse import parse_batch16_output, parse_single_message_output, write_raw_csv
from live_perf_render import backend_label, build_summary, render_internal_markdown, render_markdown, render_terminal, write_summary_csv


INTERNAL_ROW_ORDER = ("ref", "opt64", "avx2", "batch16")
SHA3_ROW_ORDER = ("ref-sha3", "compact-sha3", "openssl-sha3")
ROW_ORDER = INTERNAL_ROW_ORDER + SHA3_ROW_ORDER
DEFAULT_SIZES = "64,192,1024,1536,65536,1048576"
SHA3_EMPTY_DIGEST_PREFIX_32 = "A69F73CCA23A9AC5C8B567DC185A756E97C982164FE25859E0D1DCC1475C80A61"
SHA3_EMPTY_DIGEST_FULL = (
    "A69F73CCA23A9AC5C8B567DC185A756E97C982164FE25859E0D1DCC1475C80A615B2123AF1F5F94C11E3E9402C3AC558F500199D95B6D3E301758586281DCD26"
)


class BackendUnavailable(RuntimeError):
    """Raised when a requested backend cannot run on the local platform."""


def run_command(
    command: list[str],
    *,
    cwd: Path,
    log_path: Path,
    command_log: list[dict[str, object]],
    env: dict[str, str] | None = None,
    check: bool = True,
) -> subprocess.CompletedProcess[str]:
    log_path.parent.mkdir(parents=True, exist_ok=True)
    start_time_utc = datetime.now(timezone.utc).isoformat(timespec="microseconds").replace("+00:00", "Z")
    result = subprocess.run(
        command,
        cwd=str(cwd),
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    end_time_utc = datetime.now(timezone.utc).isoformat(timespec="microseconds").replace("+00:00", "Z")
    command_line = " ".join(command)
    log_path.write_text(
        f"COMMAND: {command_line}\n"
        f"CWD: {cwd}\n"
        f"RETURN_CODE: {result.returncode}\n"
        f"{result.stdout}",
        encoding="utf-8",
    )
    command_log.append(
        {
            "command": command,
            "command_line": command_line,
            "cwd": str(cwd),
            "log": str(log_path),
            "returncode": result.returncode,
            "start_time_utc": start_time_utc,
            "end_time_utc": end_time_utc,
        }
    )
    if check and result.returncode != 0:
        raise RuntimeError(f"command failed ({result.returncode}): {command_line}; log={log_path}")
    return result


def command_metadata(entry: dict[str, object], *, instance: str, mode: str) -> dict[str, str]:
    start_time = str(entry.get("start_time_utc", "NA"))
    end_time = str(entry.get("end_time_utc", "NA"))
    return {
        "instance": instance,
        "mode": mode,
        "command": str(entry.get("command_line", " ".join(entry.get("command", [])))),
        "start_time_utc": start_time,
        "start_time": start_time,
        "end_time_utc": end_time,
        "end_time": end_time,
    }


def sha256_file(path: Path) -> str:
    h = __import__("hashlib").sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def binary_metadata(path: Path) -> dict[str, str]:
    if not path.exists():
        return {"binary_path": str(path), "sha256_binary": "NA", "binary_sha256": "NA"}
    digest = sha256_file(path)
    return {"binary_path": str(path), "sha256_binary": digest, "binary_sha256": digest}


def make_run_id() -> str:
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    return f"{stamp}_pid{os.getpid()}_{secrets.token_hex(4)}"


def command_output(command: list[str], cwd: Path) -> str:
    result = subprocess.run(
        command,
        cwd=str(cwd),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    return result.stdout.strip() if result.stdout else "unavailable"


def parse_sizes(value: str) -> list[int]:
    sizes = []
    for part in value.split(","):
        part = part.strip()
        if not part:
            raise ValueError("empty message size")
        size = int(part)
        if size <= 0:
            raise ValueError("message sizes must be positive byte counts")
        sizes.append(size)
    return sizes


def cpu_has_flag(flag: str) -> bool:
    cpuinfo = Path("/proc/cpuinfo")
    if not cpuinfo.exists():
        return False
    text = cpuinfo.read_text(encoding="utf-8", errors="ignore")
    return flag in text.split()


def cpu_model() -> str:
    cpuinfo = Path("/proc/cpuinfo")
    if not cpuinfo.exists():
        return "unavailable"
    for line in cpuinfo.read_text(encoding="utf-8", errors="ignore").splitlines():
        if line.startswith("model name"):
            return line.split(":", 1)[1].strip()
    return "unavailable"


def governor() -> str:
    values = []
    for path in sorted(Path("/sys/devices/system/cpu").glob("cpu*/cpufreq/scaling_governor")):
        try:
            values.append(path.read_text(encoding="utf-8").strip())
        except OSError:
            pass
    if not values:
        return "unavailable"
    unique = sorted(set(values))
    return ",".join(unique)


def mode_arg(mode: str) -> str:
    return "--quick" if mode == "quick" else "--full"


def build_and_run_ref(root: Path, instance: str, sizes: list[int], mode: str, log_dir: Path, run_id: str, commands: list[dict[str, object]]) -> list[dict[str, str]]:
    directory = root / f"Implementations/Reference_Implementation/AFS-TrEDM-{instance}"
    prefix = f"afs_{instance}_ref"
    run_command(["make", "-C", str(directory), "clean"], cwd=root, log_path=log_dir / f"{prefix}_build_clean.log", command_log=commands)
    run_command(["make", "-C", str(directory), "benchmark"], cwd=root, log_path=log_dir / f"{prefix}_build.log", command_log=commands)
    binary = directory / "benchmark"
    log_path = log_dir / f"{prefix}_benchmark.csvlog"
    result = run_command(
        [str(binary), mode_arg(mode), "--csv", "--sizes", ",".join(map(str, sizes))],
        cwd=root,
        log_path=log_path,
        command_log=commands,
    )
    return parse_single_message_output(
        text=result.stdout,
        row="ref",
        algorithm=f"AFS-TrEDM-S6-{instance}",
        implementation="Reference",
        implementation_kind="AFS-TrEDM-S6 internal single-message backend",
        backend="reference-o3",
        metric_class="single-message latency/throughput",
        fresh_log=log_path,
        run_id=run_id,
        **command_metadata(commands[-1], instance=instance, mode=mode),
        **binary_metadata(binary),
        source_origin="AFS-TrEDM-S6 submission source tree",
        source_file=f"Implementations/Reference_Implementation/AFS-TrEDM-{instance}/",
        backend_class="afs_internal_reference",
        simd="none",
        api="single-message",
        notes="freshly built and executed reference backend",
    )


def build_and_run_opt64(root: Path, instance: str, sizes: list[int], mode: str, log_dir: Path, run_id: str, commands: list[dict[str, object]]) -> list[dict[str, str]]:
    directory = root / f"Implementations/Optimized_Implementation/AFS-TrEDM-{instance}"
    prefix = f"afs_{instance}_opt64"
    run_command(["make", "-C", str(directory), "clean"], cwd=root, log_path=log_dir / f"{prefix}_build_clean.log", command_log=commands)
    run_command(["make", "-C", str(directory), "benchmark_perf"], cwd=root, log_path=log_dir / f"{prefix}_build.log", command_log=commands)
    binary = directory / "benchmark_perf"
    log_path = log_dir / f"{prefix}_benchmark.csvlog"
    result = run_command(
        [str(binary), mode_arg(mode), "--csv", "--sizes", ",".join(map(str, sizes))],
        cwd=root,
        log_path=log_path,
        command_log=commands,
    )
    return parse_single_message_output(
        text=result.stdout,
        row="opt64",
        algorithm=f"AFS-TrEDM-S6-{instance}",
        implementation="Optimized opt64",
        implementation_kind="AFS-TrEDM-S6 internal single-message backend",
        backend="opt64-s6",
        metric_class="single-message latency/throughput",
        fresh_log=log_path,
        run_id=run_id,
        **command_metadata(commands[-1], instance=instance, mode=mode),
        **binary_metadata(binary),
        source_origin="AFS-TrEDM-S6 submission source tree",
        source_file=f"Implementations/Optimized_Implementation/AFS-TrEDM-{instance}/",
        backend_class="afs_internal_optimized",
        simd="portable scalar opt64",
        api="single-message",
        notes="freshly built and executed opt64 backend",
    )


def build_and_run_avx2(root: Path, instance: str, sizes: list[int], mode: str, log_dir: Path, run_id: str, commands: list[dict[str, object]]) -> list[dict[str, str]]:
    if not cpu_has_flag("avx2"):
        raise BackendUnavailable(f"AFS-TrEDM-S6-{instance} Optimized AVX2 hybrid requires CPU flag avx2")
    directory = root / f"Implementations/Optimized_Implementation/AFS-TrEDM-{instance}"
    prefix = f"afs_{instance}_avx2"
    run_command(["make", "-C", str(directory), "benchmark_avx2_perf"], cwd=root, log_path=log_dir / f"{prefix}_build.log", command_log=commands)
    binary = directory / "benchmark_avx2_perf"
    log_path = log_dir / f"{prefix}_benchmark.csvlog"
    result = run_command(
        [str(binary), mode_arg(mode), "--csv", "--sizes", ",".join(map(str, sizes))],
        cwd=root,
        log_path=log_path,
        command_log=commands,
    )
    return parse_single_message_output(
        text=result.stdout,
        row="avx2",
        algorithm=f"AFS-TrEDM-S6-{instance}",
        implementation="Optimized AVX2 hybrid",
        implementation_kind="AFS-TrEDM-S6 internal single-message backend",
        backend="avx2-sbox + opt64-s6-linear",
        metric_class="single-message latency/throughput",
        fresh_log=log_path,
        run_id=run_id,
        **command_metadata(commands[-1], instance=instance, mode=mode),
        **binary_metadata(binary),
        source_origin="AFS-TrEDM-S6 submission source tree",
        source_file=f"Implementations/Optimized_Implementation/AFS-TrEDM-{instance}/",
        backend_class="afs_internal_optimized",
        simd="AVX2 nonlinear layer plus opt64 S6 linear layer",
        api="single-message",
        notes="freshly built and executed AVX2 hybrid backend",
    )


def build_and_run_batch16(root: Path, instance: str, sizes: list[int], mode: str, log_dir: Path, run_id: str, commands: list[dict[str, object]]) -> list[dict[str, str]]:
    if not cpu_has_flag("avx512f"):
        raise BackendUnavailable(f"AFS-TrEDM-S6-{instance} Batch16 AVX512 true requires CPU flag avx512f")
    directory = root / f"Implementations/Additional_Implementation/Batch_Multibuffer/AFS-TrEDM-{instance}"
    prefix = f"afs_{instance}_batch16"
    run_command(["make", "-C", str(directory), "clean"], cwd=root, log_path=log_dir / f"{prefix}_build_clean.log", command_log=commands)
    run_command(["make", "-C", str(directory), "benchmark_batch16_avx512_perf"], cwd=root, log_path=log_dir / f"{prefix}_build.log", command_log=commands)
    binary = directory / "benchmark_batch16_avx512_perf"
    rows: list[dict[str, str]] = []
    for size in sizes:
        bits = str(size * 8)
        log_path = log_dir / f"{prefix}_{size}B.csvlog"
        result = run_command(
            [str(binary), mode_arg(mode), "--batch16-only", f"--bits={bits}"],
            cwd=root,
            log_path=log_path,
            command_log=commands,
        )
        rows.extend(
            parse_batch16_output(
                text=result.stdout,
                row="batch16",
                algorithm=f"AFS-TrEDM-S6-{instance}",
                implementation="Batch16 AVX512 true",
                implementation_kind="AFS-TrEDM-S6 internal multi-buffer backend",
                fresh_log=log_path,
                run_id=run_id,
                **command_metadata(commands[-1], instance=instance, mode=mode),
                **binary_metadata(binary),
                source_origin="AFS-TrEDM-S6 submission source tree",
                source_file=f"Implementations/Additional_Implementation/Batch_Multibuffer/AFS-TrEDM-{instance}/",
                backend_class="afs_internal_multibuffer",
                simd="AVX512 batch16",
                api="16-way multi-buffer",
                notes="multi-buffer normalized throughput; not single-message latency",
            )
        )
    return rows


def sha3_source_metadata(root: Path, rel_source: str, *, source_origin: str, backend_class: str, simd: str, api: str, notes: str) -> dict[str, str]:
    source_path = root / rel_source
    return {
        "source_origin": source_origin,
        "source_file": rel_source,
        "source_sha256": sha256_file(source_path) if source_path.exists() else "NA",
        "backend_class": backend_class,
        "simd": simd,
        "api": api,
        "notes": notes,
    }


def run_sha3_kats(root: Path, log_dir: Path, commands: list[dict[str, object]]) -> Path:
    baseline_dir = root / "tools/perf/baselines"
    log_path = log_dir / "sha3_512_kat_and_crosscheck.log"
    run_command(["bash", "run_sha3_kat.sh"], cwd=baseline_dir, log_path=log_path, command_log=commands)
    return log_path


def build_and_run_sha3_ref_c(root: Path, sizes: list[int], mode: str, log_dir: Path, run_id: str, commands: list[dict[str, object]], kat_log: Path | None = None) -> list[dict[str, str]]:
    baseline_dir = root / "tools/perf/baselines"
    exe = baseline_dir / "sha3_512_reference_c"
    run_command(["make", "-C", str(baseline_dir), "sha3_512_reference_c"], cwd=root, log_path=log_dir / "sha3_ref_c_build.log", command_log=commands)
    log_path = log_dir / f"sha3_512_reference_c_{mode}.csv"
    result = run_command(
        [str(exe), "--mode", mode, "--csv", "--sizes", ",".join(map(str, sizes))],
        cwd=root,
        log_path=log_path,
        command_log=commands,
    )
    return parse_single_message_output(
        text=result.stdout,
        row="ref-sha3",
        algorithm="SHA3-512",
        implementation="SHA3-512 Reference C (XKCP readable)",
        implementation_kind="SHA3-512 official XKCP single-message baseline",
        backend="sha3-512-reference-c-xkcp-readable",
        metric_class="single-message latency/throughput",
        fresh_log=log_path,
        run_id=run_id,
        **command_metadata(commands[-1], instance="SHA3-512", mode=mode),
        **binary_metadata(exe),
        **sha3_source_metadata(
            root,
            "tools/perf/baselines/xkcp_readable/Keccak-readable-and-compact.c",
            source_origin="official XKCP Standalone/CompactFIPS202/C/Keccak-readable-and-compact.c",
            backend_class="external_sha3_reference",
            simd="none",
            api="single-message FIPS202_SHA3_512",
            notes="official readable-and-compact reference C baseline",
        ),
        kat_status="pass",
        kat_log=str(kat_log or "NA"),
    )


def build_and_run_openssl(root: Path, sizes: list[int], mode: str, log_dir: Path, run_id: str, commands: list[dict[str, object]], kat_log: Path | None = None) -> list[dict[str, str]]:
    baseline_dir = root / "tools/perf/baselines"
    exe = baseline_dir / "openssl_evp_sha3_512"
    run_command(["make", "-C", str(baseline_dir), "openssl_evp_sha3_512"], cwd=root, log_path=log_dir / "openssl_build.log", command_log=commands)
    run_command(["openssl", "version", "-a"], cwd=root, log_path=log_dir / "openssl_version_a.log", command_log=commands)
    log_path = log_dir / f"sha3_512_openssl_evp_{mode}.csv"
    result = run_command(
        [str(exe), "--mode", mode, "--csv", "--sizes", ",".join(map(str, sizes))],
        cwd=root,
        log_path=log_path,
        command_log=commands,
    )
    return parse_single_message_output(
        text=result.stdout,
        row="openssl-sha3",
        algorithm="SHA3-512",
        implementation="SHA3-512 OpenSSL EVP",
        implementation_kind="SHA3-512 single-message baseline",
        backend="openssl-evp-sha3-512",
        metric_class="single-message latency/throughput",
        fresh_log=log_path,
        run_id=run_id,
        **command_metadata(commands[-1], instance="SHA3-512", mode=mode),
        **binary_metadata(exe),
        **sha3_source_metadata(
            root,
            "tools/baselines/bench_openssl_sha3_512.c",
            source_origin="OpenSSL EVP_sha3_512",
            backend_class="external_sha3_deployment",
            simd="library-dependent",
            api="single-message EVP",
            notes="OpenSSL EVP SHA3-512 deployment baseline",
        ),
        kat_status="pass",
        kat_log=str(kat_log or "NA"),
    )


def build_and_run_compact(root: Path, sizes: list[int], mode: str, log_dir: Path, run_id: str, commands: list[dict[str, object]], kat_log: Path | None = None) -> list[dict[str, str]]:
    baseline_dir = root / "tools/perf/baselines"
    exe = baseline_dir / "xkcp_compactfips202_sha3_512"
    run_command(["make", "-C", str(baseline_dir), "xkcp_compactfips202_sha3_512"], cwd=root, log_path=log_dir / "xkcp_compact_build.log", command_log=commands)
    log_path = log_dir / f"sha3_512_xkcp_compact_{mode}.csv"
    result = run_command(
        [str(exe), "--mode", mode, "--csv", "--sizes", ",".join(map(str, sizes))],
        cwd=root,
        log_path=log_path,
        command_log=commands,
    )
    return parse_single_message_output(
        text=result.stdout,
        row="compact-sha3",
        algorithm="SHA3-512",
        implementation="SHA3-512 XKCP CompactFIPS202 (XKCP more-compact)",
        implementation_kind="SHA3-512 single-message baseline",
        backend="xkcp-compactfips202-sha3-512-more-compact",
        metric_class="single-message latency/throughput",
        fresh_log=log_path,
        run_id=run_id,
        **command_metadata(commands[-1], instance="SHA3-512", mode=mode),
        **binary_metadata(exe),
        **sha3_source_metadata(
            root,
            "tools/perf/baselines/xkcp_more_compact/Keccak-more-compact.c",
            source_origin="official XKCP Standalone/CompactFIPS202/C/Keccak-more-compact.c",
            backend_class="external_sha3_reference",
            simd="none",
            api="single-message FIPS202_SHA3_512",
            notes="distinct official compact XKCP source; not the same source as SHA3-512 Reference C (XKCP readable)",
        ),
        kat_status="pass",
        kat_log=str(kat_log or "NA"),
    )


def verify_sha3_digest_agreement(rows: list[dict[str, str]]) -> None:
    sha3_sets = {
        row_id: {(r["msg_bytes"], r["digest_prefix_32_bytes"]) for r in rows if r["row"] == row_id}
        for row_id in SHA3_ROW_ORDER
    }
    present = {row_id: values for row_id, values in sha3_sets.items() if values}
    if len(present) > 1:
        first_id, first_values = next(iter(present.items()))
        for row_id, values in present.items():
            if values != first_values:
                raise RuntimeError(f"{first_id} and {row_id} SHA3-512 digest prefixes disagree")
    for row in rows:
        if row.get("row") in SHA3_ROW_ORDER and row.get("kat_status") != "pass":
            raise RuntimeError(f"SHA3-512 baseline missing empty-message KAT pass: {row.get('row')}")


def enrich_stage16_row_metadata(rows: list[dict[str, str]]) -> None:
    """Add Stage16 canonical labels and provenance aliases before CSV/JSON output."""

    cpu = cpu_model()
    for row in rows:
        row["row_label"] = backend_label(row)
        row["message_bytes"] = row.get("msg_bytes", "NA")
        row["source_path"] = row.get("source_file", "NA")
        row["upstream_source"] = row.get("source_origin", "NA")
        row["start_time"] = row.get("start_time_utc", row.get("started_at_utc", "NA"))
        row["end_time"] = row.get("end_time_utc", row.get("ended_at_utc", "NA"))
        row["cpu_model"] = row.get("cpu_model") or cpu


def verify_fresh_rows(raw_rows: list[dict[str, str]], *, run_id: str, script_start_timestamp: float) -> None:
    if not raw_rows:
        raise RuntimeError("no fresh live benchmark rows were produced")
    for row in raw_rows:
        if row.get("source_kind") != "fresh":
            raise RuntimeError(f"non-fresh live row detected: {row.get('row', 'unknown')}")
        if row.get("run_id") != run_id:
            raise RuntimeError(f"live row run_id mismatch for {row.get('row', 'unknown')}")
        log_path = Path(row.get("log_path") or row.get("fresh_log") or "")
        if not log_path.exists():
            raise RuntimeError(f"fresh live row is missing log_path evidence: {log_path}")
        if log_path.stat().st_mtime < script_start_timestamp:
            raise RuntimeError(f"fresh live row log predates this invocation: {log_path}")
        for key in (
            "row_label",
            "backend",
            "binary_path",
            "binary_sha256",
            "command",
            "timestamp_utc",
            "source_origin",
            "upstream_source",
            "source_file",
            "source_path",
            "start_time",
            "end_time",
            "backend_class",
            "api",
            "multi_buffer_normalized",
        ):
            if not row.get(key) or row.get(key) == "NA":
                raise RuntimeError(f"fresh live row missing {key}: {row.get('row', 'unknown')}")


def fresh_row_metadata(rows: list[dict[str, str]]) -> list[dict[str, str]]:
    keys = [
        "source_kind",
        "row",
        "algorithm",
        "implementation",
        "implementation_kind",
        "backend",
        "metric_class",
        "message_label",
        "msg_bytes",
        "message_bytes",
        "binary_path",
        "sha256_binary",
        "binary_sha256",
        "command",
        "run_id",
        "timestamp_utc",
        "log_path",
        "instance",
        "mode",
        "start_time_utc",
        "end_time_utc",
        "started_at_utc",
        "ended_at_utc",
        "host",
        "cpu_model",
        "source_origin",
        "upstream_source",
        "source_file",
        "source_path",
        "source_sha256",
        "backend_class",
        "simd",
        "api",
        "multi_buffer_normalized",
        "kat_status",
        "kat_log",
        "notes",
    ]
    out = []
    for row in rows:
        item = {key: row.get(key, "NA") for key in keys}
        item["row_label"] = row.get("row_label", backend_label(row))
        item["source_path"] = row.get("source_path", row.get("source_file", "NA"))
        item["upstream_source"] = row.get("upstream_source", row.get("source_origin", "NA"))
        item["start_time"] = row.get("start_time", row.get("start_time_utc", "NA"))
        item["end_time"] = row.get("end_time", row.get("end_time_utc", "NA"))
        out.append(item)
    return out


def main_row_metadata(rows: list[dict[str, str]]) -> list[dict[str, object]]:
    optional_keys = [
        "row",
        "implementation_kind",
        "metric_class",
        "source_origin",
        "upstream_source",
        "source_file",
        "source_path",
        "source_sha256",
        "backend_class",
        "simd",
        "api",
        "kat_status",
        "kat_log",
        "run_id",
        "log_path",
        "timestamp_utc",
        "mode",
    ]
    seen: set[tuple[str, str, str]] = set()
    out: list[dict[str, object]] = []
    for row in rows:
        ident = (row.get("row", "NA"), row.get("algorithm", "NA"), row.get("implementation", "NA"))
        if ident in seen:
            continue
        seen.add(ident)
        instance: str | None = row.get("instance", "NA")
        if not row.get("algorithm", "").startswith("AFS-TrEDM-S6-"):
            instance = None
        item: dict[str, object] = {
            "row_label": row.get("row_label", backend_label(row)),
            "backend": row.get("backend", "NA"),
            "algorithm": row.get("algorithm", "NA"),
            "instance": instance,
            "implementation": row.get("implementation", "NA"),
            "source_kind": row.get("source_kind", "NA"),
            "binary_path": row.get("binary_path", "NA"),
            "binary_sha256": row.get("binary_sha256", row.get("sha256_binary", "NA")),
            "source_path": row.get("source_path", row.get("source_file", "NA")),
            "upstream_source": row.get("upstream_source", row.get("source_origin", "NA")),
            "source_sha256": row.get("source_sha256", "NA"),
            "command": row.get("command", "NA"),
            "start_time": row.get("start_time", row.get("start_time_utc", "NA")),
            "end_time": row.get("end_time", row.get("end_time_utc", "NA")),
            "started_at_utc": row.get("started_at_utc", row.get("start_time_utc", "NA")),
            "ended_at_utc": row.get("ended_at_utc", row.get("end_time_utc", "NA")),
            "host": row.get("host", socket.gethostname()),
            "notes": row.get("notes", "NA"),
        }
        item.update({key: row.get(key, "NA") for key in optional_keys})
        if row.get("row") == "batch16":
            item["multi_buffer_normalized"] = row.get("multi_buffer_normalized", "true")
        out.append(item)
    return out


def make_metadata(
    root: Path,
    run_id: str,
    log_dir: Path,
    out_dir: Path,
    commands: list[dict[str, object]],
    mode: str,
    sizes: list[int],
    rows: list[str],
    instances: list[str],
    historical_backends: list[str],
    failed_backends: list[dict[str, str]],
) -> dict[str, object]:
    timestamp = datetime.now(timezone.utc).isoformat(timespec="seconds").replace("+00:00", "Z")
    host = socket.gethostname()
    return {
        "run_id": run_id,
        "timestamp": timestamp,
        "hostname": host,
        "host": host,
        "compiler": command_output(["cc", "--version"], root).splitlines()[0],
        "openssl_version": command_output(["openssl", "version"], root),
        "cpu_model": cpu_model(),
        "governor": governor(),
        "mode": mode,
        "instances": instances,
        "message_sizes": sizes,
        "requested_message_sizes": sizes,
        "rows": rows,
        "requested_rows": rows,
        "fresh_backends": rows,
        "historical_backends": historical_backends,
        "failed_backends": failed_backends,
        "log_directory": str(log_dir),
        "output_directory": str(out_dir),
        "backend_command_lines": commands,
        "fresh_run_policy": "This run built and executed benchmark programs; old CSV files are not used for fresh live comparison tables.",
    }


def copy_latest(out_dir: Path, latest_dir: Path) -> None:
    if latest_dir.exists() or latest_dir.is_symlink():
        if latest_dir.is_symlink() or latest_dir.is_file():
            latest_dir.unlink()
        else:
            shutil.rmtree(latest_dir)
    shutil.copytree(out_dir, latest_dir)


def file_sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def write_output_sha256sums(out_dir: Path) -> None:
    entries = []
    for path in sorted(out_dir.iterdir(), key=lambda item: item.name):
        if not path.is_file() or path.name == "sha256sums.txt":
            continue
        entries.append(f"{file_sha256(path)}  {path.name}")
    (out_dir / "sha256sums.txt").write_text("\n".join(entries) + "\n", encoding="utf-8")


def cleanup_generated_artifacts(root: Path, log_dir: Path, commands: list[dict[str, object]]) -> None:
    cleanup_targets = []
    for instance in ("512", "768", "1024"):
        cleanup_targets.extend(
            [
                (f"cleanup_ref_{instance}_after.log", root / f"Implementations/Reference_Implementation/AFS-TrEDM-{instance}"),
                (f"cleanup_opt_{instance}_after.log", root / f"Implementations/Optimized_Implementation/AFS-TrEDM-{instance}"),
                (f"cleanup_batch16_{instance}_after.log", root / f"Implementations/Additional_Implementation/Batch_Multibuffer/AFS-TrEDM-{instance}"),
            ]
        )
    for log_name, directory in cleanup_targets:
        if directory.is_dir():
            run_command(["make", "-C", str(directory), "clean"], cwd=root, log_path=log_dir / log_name, command_log=commands)

    baseline_dir = root / "tools/perf/baselines"
    if baseline_dir.is_dir():
        run_command(["make", "-C", str(baseline_dir), "clean"], cwd=root, log_path=log_dir / "cleanup_sha3_baselines_after.log", command_log=commands)

    for rel in ("build/stage_s6_08_live_perf", "build/stage_s6_09_live_perf", "tools/perf/__pycache__"):
        target = root / rel
        removed = target.exists() or target.is_symlink()
        if target.is_symlink() or target.is_file():
            target.unlink()
        elif target.is_dir():
            shutil.rmtree(target)
        log_path = log_dir / ("cleanup_" + rel.replace("/", "_") + ".log")
        log_path.write_text(
            f"COMMAND: python cleanup generated path {rel}\n"
            f"CWD: {root}\n"
            f"RETURN_CODE: 0\n"
            f"removed={str(removed).lower()}\n",
            encoding="utf-8",
        )
        commands.append(
            {
                "command": ["python-cleanup-generated", rel],
                "cwd": str(root),
                "log": str(log_path),
                "returncode": 0,
            }
        )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", required=True)
    parser.add_argument("--mode", choices=("quick", "full"), default="quick")
    parser.add_argument("--message-sizes", default=DEFAULT_SIZES)
    parser.add_argument("--lengths", help="Alias for --message-sizes.")
    parser.add_argument("--instances", default="512")
    parser.add_argument("--rows", default=",".join(ROW_ORDER))
    parser.add_argument("--include-sha3", action="store_true")
    parser.add_argument("--include-sha3-reference", action="store_true")
    parser.add_argument("--internal-only", action="store_true")
    parser.add_argument("--sha3-only", action="store_true")
    parser.add_argument("--allow-missing-baseline", action="store_true")
    parser.add_argument("--fresh-only", action="store_true")
    parser.add_argument("--output-dir")
    parser.add_argument(
        "--log-dir",
        help=(
            "Directory for generated benchmark logs. Defaults to "
            "$AFS_TREDM_S6_LIVE_PERF_LOG_ROOT/<run_id>, or /tmp/afs_tredm_s6/live_perf/<run_id>."
        ),
    )
    parser.add_argument("--no-terminal-colors", action="store_true")
    parser.add_argument("--wide", action="store_true", help="Compatibility option; Stage16 terminal output remains the four required main tables.")
    parser.add_argument("--summary-only", action="store_true", help="Compatibility option; Stage16 terminal output remains the four required main tables.")
    args = parser.parse_args()

    script_start = datetime.now(timezone.utc)
    script_start_timestamp = script_start.timestamp()
    root = Path(args.root).resolve()
    sizes = parse_sizes(args.lengths or args.message_sizes)
    instances = [x.strip() for x in args.instances.split(",") if x.strip()]
    supported_instances = {"512", "768", "1024"}
    unknown_instances = sorted(set(instances) - supported_instances)
    if unknown_instances:
        raise SystemExit(f"unsupported instances: {','.join(unknown_instances)}")
    requested_rows = [x.strip() for x in args.rows.split(",") if x.strip()]
    if args.internal_only and args.sha3_only:
        raise SystemExit("--internal-only and --sha3-only cannot be combined")
    if args.include_sha3_reference:
        args.include_sha3 = True
    if args.internal_only:
        requested_rows = [row for row in requested_rows if row in INTERNAL_ROW_ORDER]
    elif args.sha3_only:
        requested_rows = [row for row in requested_rows if row in SHA3_ROW_ORDER]
    elif not args.include_sha3:
        requested_rows = [row for row in requested_rows if row in INTERNAL_ROW_ORDER]
    if args.include_sha3_reference and not args.internal_only and "ref-sha3" not in requested_rows:
        requested_rows.insert(len([row for row in requested_rows if row in INTERNAL_ROW_ORDER]), "ref-sha3")
    unknown = sorted(set(requested_rows) - set(ROW_ORDER))
    if unknown:
        raise SystemExit(f"unknown rows: {','.join(unknown)}")

    run_id = make_run_id()
    default_log_root = Path(os.environ.get("AFS_TREDM_S6_LIVE_PERF_LOG_ROOT", "/tmp/afs_tredm_s6/live_perf"))
    log_dir = Path(args.log_dir).resolve() if args.log_dir else (default_log_root / run_id).resolve()
    out_dir = Path(args.output_dir).resolve() if args.output_dir else root / "docs/perf/live" / run_id
    latest_dir = root / "docs/perf/live/latest"
    latest_log_dir = log_dir.parent / "latest"
    log_dir.mkdir(parents=True, exist_ok=True)
    out_dir.mkdir(parents=True, exist_ok=True)

    commands: list[dict[str, object]] = []
    raw_rows: list[dict[str, str]] = []
    failed_backends: list[dict[str, str]] = []
    builders = {
        "ref": build_and_run_ref,
        "opt64": build_and_run_opt64,
        "avx2": build_and_run_avx2,
        "batch16": build_and_run_batch16,
    }
    sha3_builders = {
        "ref-sha3": build_and_run_sha3_ref_c,
        "compact-sha3": build_and_run_compact,
        "openssl-sha3": build_and_run_openssl,
    }
    sha3_kat_log = run_sha3_kats(root, log_dir, commands) if any(row in SHA3_ROW_ORDER for row in requested_rows) else None
    for row_id in requested_rows:
        if row_id in INTERNAL_ROW_ORDER:
            for instance in instances:
                try:
                    raw_rows.extend(builders[row_id](root, instance, sizes, args.mode, log_dir, run_id, commands))
                except BackendUnavailable as exc:
                    failed_backends.append({"backend": f"AFS-TrEDM-S6-{instance} {row_id}", "error": str(exc)})
        else:
            try:
                raw_rows.extend(sha3_builders[row_id](root, sizes, args.mode, log_dir, run_id, commands, sha3_kat_log))
            except Exception as exc:
                failed_backends.append({"backend": row_id, "error": str(exc)})
                if not args.allow_missing_baseline:
                    raise

    enrich_stage16_row_metadata(raw_rows)
    verify_fresh_rows(raw_rows, run_id=run_id, script_start_timestamp=script_start_timestamp)

    if args.include_sha3 or args.sha3_only:
        verify_sha3_digest_agreement(raw_rows)
    include_sha3 = any(row["row"] in SHA3_ROW_ORDER for row in raw_rows)
    summary = build_summary(raw_rows, sizes, instances=instances, include_speedup=include_sha3)
    historical_backends: list[str] = []
    historical = None
    cleanup_generated_artifacts(root, log_dir, commands)
    metadata = make_metadata(root, run_id, log_dir, out_dir, commands, args.mode, sizes, requested_rows, instances, historical_backends, failed_backends)
    metadata["script_start_time_utc"] = script_start.isoformat(timespec="microseconds").replace("+00:00", "Z")
    metadata["script_started_at_utc"] = metadata["script_start_time_utc"]
    metadata["script_ended_at_utc"] = datetime.now(timezone.utc).isoformat(timespec="microseconds").replace("+00:00", "Z")
    metadata["cleanup_policy"] = "Generated benchmark binaries and the S6 live build directories are removed after benchmark logs are captured."
    metadata["generated_artifacts_cleaned"] = True
    metadata["terminal_output_mode"] = "stage16-four-main-tables"
    metadata["terminal_option_compatibility"] = {
        "wide_requested": bool(args.wide),
        "summary_only_requested": bool(args.summary_only),
        "behavior": "terminal output remains the four required Stage16 main tables; Markdown/CSV carry detailed data.",
    }
    metadata["fresh_row_metadata"] = fresh_row_metadata(raw_rows)
    metadata["main_row_metadata"] = main_row_metadata(raw_rows)
    metadata["main_backend_metadata"] = metadata["main_row_metadata"]
    metadata["backend_rows"] = metadata["main_row_metadata"]
    if sha3_kat_log is not None:
        metadata["sha3_512_empty_kat_log"] = str(sha3_kat_log)
        metadata["sha3_512_kat_log"] = str(sha3_kat_log)
        metadata["sha3_512_empty_kat_expected_prefix_32_bytes"] = SHA3_EMPTY_DIGEST_PREFIX_32
        metadata["sha3_512_empty_kat_expected"] = SHA3_EMPTY_DIGEST_FULL
        metadata["sha3_512_abc_kat_expected"] = "B751850B1A57168A5693CD924B6B096E08F621827444F70D884F5D0240D2712E10E116E9192AF3C91A7EC57647E3934057340B4CF408D5A56592F8274EEC53F0"
        metadata["sha3_512_generated_1mib_crosscheck"] = "Reference C (XKCP readable), XKCP CompactFIPS202 (XKCP more-compact), and OpenSSL EVP digest-generated=1048576 outputs matched in run_sha3_kat.sh."

    write_raw_csv(out_dir / "live_perf_raw.csv", raw_rows)
    write_summary_csv(out_dir / "live_perf_summary.csv", summary)
    internal_rows = [row for row in raw_rows if row["algorithm"].startswith("AFS-TrEDM-S6-")]
    internal_summary = build_summary(internal_rows, sizes, instances=instances, include_speedup=False)
    write_raw_csv(out_dir / "internal_afs_s6_raw.csv", internal_rows)
    write_summary_csv(out_dir / "internal_afs_s6_summary.csv", internal_summary)
    (out_dir / "run_metadata.json").write_text(json.dumps(metadata, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    terminal = render_terminal(
        summary,
        instances=instances,
        include_sha3=include_sha3,
        historical_h1=historical,
        metadata=metadata,
        wide=args.wide,
        summary_only=args.summary_only,
    )
    (out_dir / "terminal_output.txt").write_text(terminal, encoding="utf-8")
    (out_dir / "AFS-TrEDM-S6_Live_Performance_Tables.md").write_text(
        render_markdown(summary=summary, run_id=run_id, metadata=metadata, instances=instances, include_sha3=include_sha3, historical_h1=historical),
        encoding="utf-8",
    )
    (out_dir / "AFS-TrEDM-S6_Internal_Performance_Tables.md").write_text(
        render_internal_markdown(summary=internal_summary, run_id=run_id, metadata=metadata, instances=instances),
        encoding="utf-8",
    )
    write_output_sha256sums(out_dir)
    copy_latest(out_dir, latest_dir)
    copy_latest(log_dir, latest_log_dir)

    print(terminal, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
