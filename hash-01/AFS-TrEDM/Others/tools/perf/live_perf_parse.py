#!/usr/bin/env python3
"""Parsers for S6 live benchmark output."""

from __future__ import annotations

import csv
import socket
from pathlib import Path
from typing import Iterable


RAW_COLUMNS = [
    "row",
    "algorithm",
    "implementation",
    "implementation_kind",
    "backend",
    "metric_class",
    "message_label",
    "msg_bytes",
    "message_bytes",
    "repetitions",
    "elapsed_seconds",
    "ns_per_hash",
    "cycles_per_hash",
    "cycles_per_byte",
    "throughput_MBps",
    "throughput_MiBps",
    "latency_ms_per_hash",
    "checksum",
    "digest_prefix_32_bytes",
    "source_kind",
    "row_label",
    "fresh_log",
    "log_path",
    "run_id",
    "timestamp_utc",
    "instance",
    "mode",
    "command",
    "binary_path",
    "sha256_binary",
    "binary_sha256",
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
    "start_time_utc",
    "start_time",
    "end_time_utc",
    "end_time",
    "started_at_utc",
    "ended_at_utc",
    "host",
    "cpu_model",
    "hot_path_messages",
    "fallback_messages",
    "hot_path_blocks",
    "fallback_blocks",
]


def _csv_rows(text: str) -> Iterable[list[str]]:
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith("#") or line.startswith("COMMAND:"):
            continue
        yield next(csv.reader([line]))


def _float_or_na(value: str) -> str:
    return "NA" if value == "NA" else f"{float(value):.6f}"


def _mbps_from_mibps(value: str) -> str:
    if value == "NA":
        return "NA"
    return f"{float(value) * 1.048576:.6f}"


def _size_label(size: int) -> str:
    if size == 1024:
        return "1KB"
    if size == 65536:
        return "64KB"
    if size == 1048576:
        return "1MB"
    return f"{size}B"


def parse_single_message_output(
    *,
    text: str,
    row: str,
    algorithm: str,
    implementation: str,
    implementation_kind: str,
    backend: str,
    metric_class: str,
    fresh_log: Path,
    run_id: str,
    instance: str,
    mode: str,
    command: str,
    binary_path: str,
    sha256_binary: str,
    start_time_utc: str,
    end_time_utc: str,
    start_time: str | None = None,
    end_time: str | None = None,
    binary_sha256: str | None = None,
    source_origin: str = "NA",
    source_file: str = "NA",
    source_sha256: str = "NA",
    backend_class: str = "NA",
    simd: str = "NA",
    api: str = "NA",
    kat_status: str = "NA",
    kat_log: str = "NA",
    notes: str = "NA",
    host: str | None = None,
) -> list[dict[str, str]]:
    out: list[dict[str, str]] = []
    host_value = host or socket.gethostname()
    start_time_value = start_time or start_time_utc
    end_time_value = end_time or end_time_utc
    for fields in _csv_rows(text):
        if len(fields) >= 12 and fields[0] == backend:
            _, msg_bits, msg_bytes, reps, elapsed, ns_hash = fields[0:6]
            cycles_hash, cycles_byte, mbps, mibps, checksum, digest_prefix = fields[6:12]
        elif len(fields) >= 10:
            msg_bits, msg_bytes, reps, elapsed, ns_hash = fields[0:5]
            cycles_hash, cycles_byte, mibps, checksum, digest_prefix = fields[5:10]
            mbps = _mbps_from_mibps(mibps)
        else:
            continue
        msg_bytes_i = int(msg_bytes)
        out.append(
            {
                "row": row,
                "algorithm": algorithm,
                "implementation": implementation,
                "implementation_kind": implementation_kind,
                "backend": backend,
                "metric_class": metric_class,
                "message_label": _size_label(msg_bytes_i),
                "msg_bytes": str(msg_bytes_i),
                "repetitions": reps,
                "elapsed_seconds": f"{float(elapsed):.6f}",
                "ns_per_hash": f"{float(ns_hash):.6f}",
                "cycles_per_hash": _float_or_na(cycles_hash),
                "cycles_per_byte": _float_or_na(cycles_byte),
                "throughput_MBps": _float_or_na(mbps),
                "throughput_MiBps": _float_or_na(mibps),
                "latency_ms_per_hash": f"{float(ns_hash) / 1_000_000.0:.9f}",
                "checksum": checksum,
                "digest_prefix_32_bytes": digest_prefix,
                "source_kind": "fresh",
                "row_label": "NA",
                "fresh_log": str(fresh_log),
                "log_path": str(fresh_log),
                "run_id": run_id,
                "timestamp_utc": end_time_utc,
                "instance": instance,
                "mode": mode,
                "command": command,
                "binary_path": binary_path,
                "sha256_binary": sha256_binary,
                "binary_sha256": binary_sha256 or sha256_binary,
                "source_origin": source_origin,
                "upstream_source": source_origin,
                "source_file": source_file,
                "source_path": source_file,
                "source_sha256": source_sha256,
                "backend_class": backend_class,
                "simd": simd,
                "api": api,
                "multi_buffer_normalized": "false",
                "kat_status": kat_status,
                "kat_log": kat_log,
                "notes": notes,
                "start_time_utc": start_time_utc,
                "start_time": start_time_value,
                "end_time_utc": end_time_utc,
                "end_time": end_time_value,
                "started_at_utc": start_time_utc,
                "ended_at_utc": end_time_utc,
                "host": host_value,
                "hot_path_messages": "NA",
                "fallback_messages": "NA",
                "hot_path_blocks": "NA",
                "fallback_blocks": "NA",
            }
        )
        if int(msg_bits) != msg_bytes_i * 8:
            raise ValueError(f"non-byte-aligned row in live parser: {fields}")
    return out


def parse_batch16_output(
    *,
    text: str,
    row: str,
    algorithm: str,
    implementation: str,
    implementation_kind: str,
    fresh_log: Path,
    run_id: str,
    instance: str,
    mode: str,
    command: str,
    binary_path: str,
    sha256_binary: str,
    start_time_utc: str,
    end_time_utc: str,
    start_time: str | None = None,
    end_time: str | None = None,
    binary_sha256: str | None = None,
    source_origin: str = "NA",
    source_file: str = "NA",
    source_sha256: str = "NA",
    backend_class: str = "NA",
    simd: str = "NA",
    api: str = "NA",
    kat_status: str = "NA",
    kat_log: str = "NA",
    notes: str = "NA",
    host: str | None = None,
) -> list[dict[str, str]]:
    out: list[dict[str, str]] = []
    host_value = host or socket.gethostname()
    start_time_value = start_time or start_time_utc
    end_time_value = end_time or end_time_utc
    for fields in _csv_rows(text):
        if len(fields) < 16 or fields[0] != "batch16":
            continue
        (
            batch_mode,
            backend,
            msg_bits,
            msg_bytes,
            batches,
            elapsed,
            _cycles_per_batch,
            cycles_per_hash,
            cycles_per_byte,
            mibps,
            hot_blocks,
            hot_messages,
            fallback_messages,
            fallback_blocks,
            checksum,
            digest_prefix,
        ) = fields[:16]
        del batch_mode
        msg_bytes_i = int(msg_bytes)
        batches_i = int(batches)
        normalized_hashes = batches_i * 16
        elapsed_f = float(elapsed)
        ns_hash = elapsed_f * 1_000_000_000.0 / float(normalized_hashes)
        out.append(
            {
                "row": row,
                "algorithm": algorithm,
                "implementation": implementation,
                "implementation_kind": implementation_kind,
                "backend": backend,
                "metric_class": "multi-buffer normalized throughput",
                "message_label": _size_label(msg_bytes_i),
                "msg_bytes": str(msg_bytes_i),
                "repetitions": str(normalized_hashes),
                "elapsed_seconds": f"{elapsed_f:.6f}",
                "ns_per_hash": f"{ns_hash:.6f}",
                "cycles_per_hash": _float_or_na(cycles_per_hash),
                "cycles_per_byte": _float_or_na(cycles_per_byte),
                "throughput_MBps": _mbps_from_mibps(mibps),
                "throughput_MiBps": _float_or_na(mibps),
                "latency_ms_per_hash": f"{ns_hash / 1_000_000.0:.9f}",
                "checksum": checksum,
                "digest_prefix_32_bytes": digest_prefix,
                "source_kind": "fresh",
                "row_label": "NA",
                "fresh_log": str(fresh_log),
                "log_path": str(fresh_log),
                "run_id": run_id,
                "timestamp_utc": end_time_utc,
                "instance": instance,
                "mode": mode,
                "command": command,
                "binary_path": binary_path,
                "sha256_binary": sha256_binary,
                "binary_sha256": binary_sha256 or sha256_binary,
                "source_origin": source_origin,
                "upstream_source": source_origin,
                "source_file": source_file,
                "source_path": source_file,
                "source_sha256": source_sha256,
                "backend_class": backend_class,
                "simd": simd,
                "api": api,
                "multi_buffer_normalized": "true",
                "kat_status": kat_status,
                "kat_log": kat_log,
                "notes": notes,
                "start_time_utc": start_time_utc,
                "start_time": start_time_value,
                "end_time_utc": end_time_utc,
                "end_time": end_time_value,
                "started_at_utc": start_time_utc,
                "ended_at_utc": end_time_utc,
                "host": host_value,
                "hot_path_messages": hot_messages,
                "fallback_messages": fallback_messages,
                "hot_path_blocks": hot_blocks,
                "fallback_blocks": fallback_blocks,
            }
        )
        if int(msg_bits) != msg_bytes_i * 8:
            raise ValueError(f"non-byte-aligned batch row in live parser: {fields}")
    return out


def write_raw_csv(path: Path, rows: list[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=RAW_COLUMNS)
        writer.writeheader()
        for row in rows:
            writer.writerow({key: row.get(key, "NA") for key in RAW_COLUMNS})
