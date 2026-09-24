#!/usr/bin/env python3
"""Render Stage16 live performance tables."""

from __future__ import annotations

import csv
from pathlib import Path


AFS_BACKENDS = [
    ("Reference", "Reference"),
    ("Optimized opt64", "Optimized opt64"),
    ("Optimized AVX2 hybrid", "Optimized AVX2 hybrid"),
    ("Batch16 AVX512 norm", "Batch16 AVX512 norm"),
]
SHA3_LABELS = [
    "SHA3-512 Reference C (XKCP readable)",
    "SHA3-512 XKCP CompactFIPS202 (XKCP more-compact)",
    "SHA3-512 OpenSSL EVP",
]
REQUIRED_SIZES = (64, 192, 1024, 1536, 65536, 1048576)


def size_label(size: int) -> str:
    if size == 1024:
        return "1KB"
    if size == 65536:
        return "64KB"
    if size == 1048576:
        return "1MB"
    return f"{size}B"


def _display_implementation(value: str) -> str:
    if value == "Batch16 AVX512 true":
        return "Batch16 AVX512 norm"
    return value


def backend_label(row: dict[str, str]) -> str:
    if row["algorithm"].startswith("AFS-TrEDM-S6-"):
        instance = row["algorithm"].rsplit("-", 1)[-1]
        return f"AFS-TrEDM-S6 {_display_implementation(row['implementation'])} {instance}"
    if row["row"] == "ref-sha3":
        return "SHA3-512 Reference C (XKCP readable)"
    if row["row"] == "compact-sha3":
        return "SHA3-512 XKCP CompactFIPS202 (XKCP more-compact)"
    if row["row"] == "openssl-sha3":
        return "SHA3-512 OpenSSL EVP"
    return row["implementation"]


def internal_labels(instances: list[str]) -> list[str]:
    return [
        f"AFS-TrEDM-S6 {_display_implementation(implementation)} {instance}"
        for implementation, _short in AFS_BACKENDS
        for instance in instances
    ]


def _instance_labels(instance: str) -> list[str]:
    return [f"AFS-TrEDM-S6 {_display_implementation(implementation)} {instance}" for implementation, _short in AFS_BACKENDS]


def _as_float(value: str | None) -> float | None:
    if value is None or value == "NA":
        return None
    return float(value)


def _metric_value(raw: dict[str, str] | None, field: str, digits: int) -> str:
    if raw is None:
        return "NA"
    value = raw.get(field, "NA")
    if value == "NA":
        return "NA"
    return f"{float(value):.{digits}f}"


def _latency_key(label: str) -> str:
    if label.endswith("Batch16 AVX512 norm"):
        return f"{label}_latency_ms_normalized_per_message"
    return f"{label}_latency_ms"


def _sort_labels(labels: set[str], instances: list[str]) -> list[str]:
    ordered = internal_labels(instances)
    return [label for label in ordered if label in labels] + [label for label in SHA3_LABELS if label in labels]


def build_summary(
    raw_rows: list[dict[str, str]],
    sizes: list[int],
    instances: list[str] | None = None,
    *,
    include_speedup: bool = True,
) -> list[dict[str, str]]:
    instances = instances or sorted({row["instance"] for row in raw_rows if row["algorithm"].startswith("AFS-TrEDM-S6-")})
    by_key: dict[tuple[str, int], dict[str, str]] = {}
    labels: set[str] = set()
    for row in raw_rows:
        label = backend_label(row)
        labels.add(label)
        by_key[(label, int(row["msg_bytes"]))] = row

    ordered_labels = _sort_labels(labels, instances)
    summary: list[dict[str, str]] = []
    for size in sizes:
        item: dict[str, str] = {"message_size": size_label(size), "msg_bytes": str(size)}
        openssl_label = "SHA3-512 OpenSSL EVP"
        openssl_raw = by_key.get((openssl_label, size))
        openssl_mbps = _as_float(None if openssl_raw is None else openssl_raw.get("throughput_MBps"))
        for label in ordered_labels:
            raw = by_key.get((label, size))
            item[f"{label}_MBps"] = _metric_value(raw, "throughput_MBps", 3)
            item[_latency_key(label)] = _metric_value(raw, "latency_ms_per_hash", 9)
            item[f"{label}_cycles_per_hash"] = _metric_value(raw, "cycles_per_hash", 2)
            item[f"{label}_cycles_per_byte"] = _metric_value(raw, "cycles_per_byte", 2)
            mbps = _as_float(None if raw is None else raw.get("throughput_MBps"))
            if include_speedup:
                item[f"{label}_over_SHA3-512 OpenSSL EVP_throughput_ratio"] = (
                    "NA" if openssl_mbps in (None, 0.0) or mbps is None else f"{mbps / openssl_mbps:.3f}"
                )
        summary.append(item)
    return summary


def summary_columns(summary: list[dict[str, str]]) -> list[str]:
    fixed = ["message_size", "msg_bytes"]
    seen = set(fixed)
    columns = list(fixed)
    suffix_order = [
        "_MBps",
        "_latency_ms",
        "_latency_ms_normalized_per_message",
        "_cycles_per_hash",
        "_cycles_per_byte",
        "_over_SHA3-512 OpenSSL EVP_throughput_ratio",
    ]
    keys = [key for row in summary for key in row]
    for suffix in suffix_order:
        for key in keys:
            if key.endswith(suffix) and key not in seen:
                columns.append(key)
                seen.add(key)
    for key in keys:
        if key not in seen:
            columns.append(key)
            seen.add(key)
    return columns


def write_summary_csv(path: Path, summary: list[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    columns = summary_columns(summary)
    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=columns)
        writer.writeheader()
        for row in summary:
            writer.writerow({key: row.get(key, "NA") for key in columns})


def _plain_table(headers: list[str], rows: list[list[str]]) -> str:
    widths = [len(h) for h in headers]
    for row in rows:
        for i, value in enumerate(row):
            widths[i] = max(widths[i], len(value))
    out = ["  ".join(h.ljust(widths[i]) for i, h in enumerate(headers))]
    out.append("  ".join("-" * widths[i] for i in range(len(headers))))
    for row in rows:
        out.append("  ".join(value.rjust(widths[i]) if i else value.ljust(widths[i]) for i, value in enumerate(row)))
    return "\n".join(out)


def _plain_table_multi_header(header_rows: list[list[str]], rows: list[list[str]]) -> str:
    widths = [0 for _ in header_rows[0]]
    for header in header_rows:
        for i, value in enumerate(header):
            widths[i] = max(widths[i], len(value))
    for row in rows:
        for i, value in enumerate(row):
            widths[i] = max(widths[i], len(value))
    out = ["  ".join(value.ljust(widths[i]) for i, value in enumerate(header)) for header in header_rows]
    out.append("  ".join("-" * widths[i] for i in range(len(widths))))
    for row in rows:
        out.append("  ".join(value.rjust(widths[i]) if i else value.ljust(widths[i]) for i, value in enumerate(row)))
    return "\n".join(out)


def _md_table(headers: list[str], rows: list[list[str]]) -> str:
    out = ["| " + " | ".join(headers) + " |"]
    out.append("| " + " | ".join("---" for _ in headers) + " |")
    for row in rows:
        out.append("| " + " | ".join(row) + " |")
    return "\n".join(out)


def _metric_key(label: str, suffix: str) -> str:
    return _latency_key(label) if suffix == "latency" else f"{label}_{suffix}"


def _metric_rows(summary: list[dict[str, str]], labels: list[str], suffix: str) -> list[list[str]]:
    rows: list[list[str]] = []
    for row in summary:
        values = [row["message_size"]]
        for label in labels:
            values.append(row.get(_metric_key(label, suffix), "NA"))
        rows.append(values)
    return rows


def _stage14b_row_labels(instances: list[str], include_sha3: bool) -> list[str]:
    labels = internal_labels(instances)
    if include_sha3:
        labels.extend(SHA3_LABELS)
    return labels


def _summary_by_size(summary: list[dict[str, str]]) -> dict[int, dict[str, str]]:
    return {int(row["msg_bytes"]): row for row in summary}


def _stage14b_headers() -> list[str]:
    return ["Implementation", *[size_label(size) for size in REQUIRED_SIZES]]


def _stage14b_table_rows(summary: list[dict[str, str]], labels: list[str], suffix: str) -> list[list[str]]:
    by_size = _summary_by_size(summary)
    rows: list[list[str]] = []
    for label in labels:
        values = [label]
        for size in REQUIRED_SIZES:
            values.append(_value_for(by_size.get(size), label, suffix))
        rows.append(values)
    return rows


def _stage14b_table(title: str, summary: list[dict[str, str]], labels: list[str], suffix: str) -> str:
    return "\n".join(
        [
            title,
            _plain_table(_stage14b_headers(), _stage14b_table_rows(summary, labels, suffix)),
        ]
    )


def _summary_row(summary: list[dict[str, str]], msg_bytes: int) -> dict[str, str] | None:
    return next((row for row in summary if row["msg_bytes"] == str(msg_bytes)), None)


def _value_for(row: dict[str, str] | None, label: str, suffix: str) -> str:
    if row is None:
        return "NA"
    return row.get(_metric_key(label, suffix), "NA")


def _afs_instance_headers() -> list[str]:
    return ["Instance", "Reference", "opt64", "AVX2 hybrid", "Batch16 AVX512 norm"]


def _afs_instance_summary_rows(
    summary: list[dict[str, str]],
    *,
    msg_bytes: int,
    instances: list[str],
    suffix: str,
) -> list[list[str]]:
    row = _summary_row(summary, msg_bytes)
    rows: list[list[str]] = []
    for instance in instances:
        values = [f"AFS-S6-{instance}"]
        for implementation, _short in AFS_BACKENDS:
            values.append(_value_for(row, f"AFS-TrEDM-S6 {_display_implementation(implementation)} {instance}", suffix))
        rows.append(values)
    return rows


def _sha3_baseline_summary_rows(summary: list[dict[str, str]]) -> list[list[str]]:
    row_64 = _summary_row(summary, 64)
    row_1mib = _summary_row(summary, 1048576)
    labels = [
        ("SHA3-512 Reference C (XKCP readable)", "Reference C (XKCP readable)"),
        ("SHA3-512 XKCP CompactFIPS202 (XKCP more-compact)", "XKCP more-compact"),
        ("SHA3-512 OpenSSL EVP", "OpenSSL EVP"),
    ]
    rows: list[list[str]] = []
    for label, short in labels:
        rows.append(
            [
                short,
                _value_for(row_64, label, "MBps"),
                _value_for(row_1mib, label, "MBps"),
                _value_for(row_64, label, "cycles_per_byte"),
                _value_for(row_1mib, label, "cycles_per_byte"),
            ]
        )
    return rows


def _detail_columns(instance: str, include_sha3: bool) -> list[tuple[str, str, str]]:
    columns = [(label, f"AFS-S6-{instance}", short) for label, short in zip(_instance_labels(instance), [x[1] for x in AFS_BACKENDS])]
    if include_sha3:
        columns.extend(
            [
                ("SHA3-512 Reference C (XKCP readable)", "SHA3-512", "Reference C"),
                ("SHA3-512 XKCP CompactFIPS202 (XKCP more-compact)", "SHA3-512", "XKCP more-compact"),
                ("SHA3-512 OpenSSL EVP", "SHA3-512", "OpenSSL EVP"),
            ]
        )
    return columns


def _detail_header_rows(instance: str, include_sha3: bool) -> list[list[str]]:
    columns = _detail_columns(instance, include_sha3)
    return [
        ["Message", *[top for _label, top, _bottom in columns]],
        ["Size", *[bottom for _label, _top, bottom in columns]],
    ]


def _detail_md_headers(instance: str, include_sha3: bool) -> list[str]:
    return ["Message Size", *[label for label, _top, _bottom in _detail_columns(instance, include_sha3)]]


def _detail_rows(summary: list[dict[str, str]], instance: str, include_sha3: bool, suffix: str) -> list[list[str]]:
    rows: list[list[str]] = []
    columns = _detail_columns(instance, include_sha3)
    for row in summary:
        values = [row["message_size"]]
        for label, _top, _bottom in columns:
            values.append(row.get(_metric_key(label, suffix), "NA"))
        rows.append(values)
    return rows


def _detail_terminal_table(title: str, summary: list[dict[str, str]], instance: str, include_sha3: bool, suffix: str) -> str:
    return "\n".join(
        [
            title,
            _plain_table_multi_header(_detail_header_rows(instance, include_sha3), _detail_rows(summary, instance, include_sha3, suffix)),
        ]
    )


def _detail_title(metric: str, instance: str) -> str:
    return f"{metric} — AFS-TrEDM-S6-{instance} vs SHA3-512"


def _speedup_instance_header_rows(instance: str) -> list[list[str]]:
    return [
        ["Message", f"AFS-S6-{instance}", f"AFS-S6-{instance}", f"AFS-S6-{instance}", f"AFS-S6-{instance}", "SHA3-512", "SHA3-512"],
        ["Size", "Reference", "opt64", "AVX2 hybrid", "Batch16 AVX512 norm", "Reference C", "XKCP CompactFIPS202"],
    ]


def _speedup_instance_rows(summary: list[dict[str, str]], instance: str) -> list[list[str]]:
    labels = _instance_labels(instance) + ["SHA3-512 Reference C (XKCP readable)", "SHA3-512 XKCP CompactFIPS202 (XKCP more-compact)"]
    rows: list[list[str]] = []
    for row in summary:
        values = [row["message_size"]]
        for label in labels:
            values.append(row.get(f"{label}_over_SHA3-512 OpenSSL EVP_throughput_ratio", "NA"))
        rows.append(values)
    return rows


def _speedup_md_headers(instance: str) -> list[str]:
    return [
        "Message Size",
        f"AFS-TrEDM-S6-{instance} Reference / SHA3-512 OpenSSL EVP",
        f"AFS-TrEDM-S6-{instance} Optimized opt64 / SHA3-512 OpenSSL EVP",
        f"AFS-TrEDM-S6-{instance} Optimized AVX2 hybrid / SHA3-512 OpenSSL EVP",
        f"AFS-TrEDM-S6-{instance} Batch16 AVX512 norm / SHA3-512 OpenSSL EVP",
        "SHA3-512 Reference C (XKCP readable) / SHA3-512 OpenSSL EVP",
        "SHA3-512 XKCP CompactFIPS202 (XKCP more-compact) / SHA3-512 OpenSSL EVP",
    ]


def _combined_512_cpb_rows(summary: list[dict[str, str]], labels: list[str]) -> list[list[str]]:
    row_1mib = next((row for row in summary if row["msg_bytes"] == "1048576"), None)
    rows: list[list[str]] = []
    for label in labels:
        metric_class = (
            "multi-buffer normalized throughput"
            if "Batch16 AVX512 norm" in label
            else "single-message latency/throughput"
        )
        rows.append([label, "NA" if row_1mib is None else row_1mib.get(f"{label}_cycles_per_byte", "NA"), metric_class])
    return rows


def _latest_failed_backend_notes(metadata: dict[str, object]) -> list[str]:
    out: list[str] = []
    failed = metadata.get("failed_backends", [])
    if isinstance(failed, list):
        for item in failed:
            if isinstance(item, dict):
                out.append(f"- Unavailable backend: {item.get('backend', 'unknown')}: {item.get('error', 'unknown error')}")
    return out


def _metric_headers(labels: list[str]) -> list[str]:
    return ["Message Size", *labels]


def _speedup_rows(summary: list[dict[str, str]], labels: list[str]) -> list[list[str]]:
    rows: list[list[str]] = []
    for row in summary:
        values = [row["message_size"]]
        for label in labels:
            values.append(row.get(f"{label}_over_SHA3-512 OpenSSL EVP_throughput_ratio", "NA"))
        rows.append(values)
    return rows


def render_wide_terminal(
    summary: list[dict[str, str]],
    *,
    instances: list[str],
    include_sha3: bool,
    historical_h1: str | None = None,
    metadata: dict[str, object] | None = None,
) -> str:
    labels = _stage14b_row_labels(instances, include_sha3)
    parts = [
        _stage14b_table("Cycles per Byte Comparison (cpb)", summary, labels, "cycles_per_byte"),
        "",
        _stage14b_table("Cycles per Hash Comparison", summary, labels, "cycles_per_hash"),
        "",
        _stage14b_table("Throughput Comparison (MB/s)", summary, labels, "MBps"),
        "",
        _stage14b_table("Latency Comparison (milliseconds)", summary, labels, "latency"),
    ]
    parts.extend(_notes(metadata=metadata, historical_h1=None, detailed=False))
    return "\n".join(parts) + "\n"


def _notes(*, metadata: dict[str, object] | None, historical_h1: str | None, detailed: bool) -> list[str]:
    del historical_h1
    parts = [
        "",
        "Notes:",
        "- MB/s uses decimal bytes per second.",
        "- 1KB, 64KB, and 1MB labels correspond to binary byte counts in metadata: 1024, 65536, and 1048576 bytes.",
        "- AFS-TrEDM-S6 Batch16 AVX512 norm is a multi-buffer normalized throughput backend. Its latency, cycles/hash, and cpb are normalized per message across a 16-message batch and must not be interpreted as single-message latency.",
        "- SHA3-512 Reference C (XKCP readable) uses official XKCP Standalone/CompactFIPS202/C/Keccak-readable-and-compact.c unless otherwise noted.",
        "- SHA3-512 XKCP CompactFIPS202 (XKCP more-compact) is a separate compact XKCP baseline using official Keccak-more-compact.c.",
        "- SHA3-512 OpenSSL EVP is a single-message deployment baseline.",
        "- SHA3-512 Reference C and XKCP CompactFIPS202 are external baselines, not AFS-TrEDM implementations.",
        "- None of these SHA3 baselines is part of AFS-TrEDM's Reference_Implementation or Optimized_Implementation.",
    ]
    if detailed:
        parts.append("- Detailed comparison tables are emitted separately for each requested AFS-TrEDM-S6 instance.")
    if metadata:
        parts.extend(_latest_failed_backend_notes(metadata))
    return parts


def render_terminal(
    summary: list[dict[str, str]],
    *,
    instances: list[str],
    include_sha3: bool,
    historical_h1: str | None = None,
    metadata: dict[str, object] | None = None,
    wide: bool = False,
    summary_only: bool = False,
) -> str:
    del wide
    del summary_only
    del historical_h1
    labels = _stage14b_row_labels(instances, include_sha3)
    parts = [
        _stage14b_table("Cycles per Byte Comparison (cpb)", summary, labels, "cycles_per_byte"),
        "",
        _stage14b_table("Cycles per Hash Comparison", summary, labels, "cycles_per_hash"),
        "",
        _stage14b_table("Throughput Comparison (MB/s)", summary, labels, "MBps"),
        "",
        _stage14b_table("Latency Comparison (milliseconds)", summary, labels, "latency"),
    ]
    parts.extend(_notes(metadata=metadata, historical_h1=None, detailed=False))
    return "\n".join(parts) + "\n"


def render_internal_markdown(
    *,
    summary: list[dict[str, str]],
    run_id: str,
    metadata: dict[str, object],
    instances: list[str],
) -> str:
    internal = internal_labels(instances)
    lines = [
        "# AFS-TrEDM-S6 Internal Performance Tables",
        "",
        f"- run_id: `{run_id}`",
        f"- timestamp: `{metadata.get('timestamp', 'NA')}`",
        "",
        "## AFS-TrEDM-S6 Internal Throughput Comparison (MB/s)",
        "",
        _md_table(_metric_headers(internal), _metric_rows(summary, internal, "MBps")),
        "",
        "## AFS-TrEDM-S6 Internal Latency Comparison (ms per message)",
        "",
        _md_table(_metric_headers(internal), _metric_rows(summary, internal, "latency")),
        "",
        "## AFS-TrEDM-S6 Internal Cycles per Hash Comparison",
        "",
        _md_table(_metric_headers(internal), _metric_rows(summary, internal, "cycles_per_hash")),
        "",
        "## AFS-TrEDM-S6 Internal Cycles per Byte Comparison",
        "",
        _md_table(_metric_headers(internal), _metric_rows(summary, internal, "cycles_per_byte")),
        "",
        "AFS-TrEDM-S6 Batch16 AVX512 norm is a multi-buffer normalized throughput backend. Its latency, cycles/hash, and cpb are normalized per message across a 16-message batch and must not be interpreted as single-message latency.",
        "Optimized AVX2 hybrid is a single-message backend using an AVX2 AFS-64 nonlinear layer plus generated opt64 S6 linear layer.",
        "SHA3-512 Reference C (XKCP readable) uses official XKCP Standalone/CompactFIPS202/C/Keccak-readable-and-compact.c unless otherwise noted.",
        "SHA3-512 XKCP CompactFIPS202 (XKCP more-compact) is a separate compact XKCP baseline using official Keccak-more-compact.c.",
        "SHA3-512 OpenSSL EVP is a single-message deployment baseline.",
        "SHA3-512 Reference C and XKCP CompactFIPS202 are external baselines, not AFS-TrEDM implementations.",
        "None of these SHA3 baselines is part of AFS-TrEDM's Reference_Implementation or Optimized_Implementation.",
        "No true SHA3-512 batch baseline is claimed in this package unless a separate SHA3 batch backend is implemented and labeled.",
    ]
    return "\n".join(lines) + "\n"


def _markdown_detail_sections(summary: list[dict[str, str]], instances: list[str], include_sha3: bool) -> list[str]:
    lines: list[str] = []
    if not include_sha3:
        return lines
    for instance in instances:
        lines.extend(
            [
                "",
                f"## {_detail_title('Throughput Comparison (MB/s)', instance)}",
                "",
                _md_table(_detail_md_headers(instance, include_sha3), _detail_rows(summary, instance, include_sha3, "MBps")),
                "",
                f"## {_detail_title('Latency Comparison (milliseconds)', instance)}",
                "",
                _md_table(_detail_md_headers(instance, include_sha3), _detail_rows(summary, instance, include_sha3, "latency")),
                "",
                f"## {_detail_title('Cycles per Hash Comparison', instance)}",
                "",
                _md_table(_detail_md_headers(instance, include_sha3), _detail_rows(summary, instance, include_sha3, "cycles_per_hash")),
                "",
                f"## {_detail_title('Cycles per Byte Comparison (cpb)', instance)}",
                "",
                _md_table(_detail_md_headers(instance, include_sha3), _detail_rows(summary, instance, include_sha3, "cycles_per_byte")),
                "",
                f"## Speedup versus SHA3-512 OpenSSL EVP — AFS-TrEDM-S6-{instance}",
                "",
                _md_table(_speedup_md_headers(instance), _speedup_instance_rows(summary, instance)),
            ]
        )
    return lines


def render_markdown(
    *,
    summary: list[dict[str, str]],
    run_id: str,
    metadata: dict[str, object],
    instances: list[str],
    include_sha3: bool,
    historical_h1: str | None = None,
) -> str:
    internal = internal_labels(instances)
    sha3 = SHA3_LABELS if include_sha3 else []
    labels = _stage14b_row_labels(instances, include_sha3)
    combined_512 = [label for label in internal if label.endswith(" 512")] + sha3
    lines = [
        "# AFS-TrEDM-S6 Live Performance Tables",
        "",
        f"- run_id: `{run_id}`",
        f"- timestamp: `{metadata.get('timestamp', 'NA')}`",
        f"- hostname: `{metadata.get('hostname', 'NA')}`",
        f"- compiler: `{metadata.get('compiler', 'NA')}`",
        f"- OpenSSL: `{metadata.get('openssl_version', 'NA')}`",
        f"- CPU: `{metadata.get('cpu_model', 'NA')}`",
        f"- governor: `{metadata.get('governor', 'NA')}`",
        "",
        "## Cycles per Byte Comparison (cpb)",
        "",
        _md_table(_stage14b_headers(), _stage14b_table_rows(summary, labels, "cycles_per_byte")),
        "",
        "## Cycles per Hash Comparison",
        "",
        _md_table(_stage14b_headers(), _stage14b_table_rows(summary, labels, "cycles_per_hash")),
        "",
        "## Throughput Comparison (MB/s)",
        "",
        _md_table(_stage14b_headers(), _stage14b_table_rows(summary, labels, "MBps")),
        "",
        "## Latency Comparison (milliseconds)",
        "",
        _md_table(_stage14b_headers(), _stage14b_table_rows(summary, labels, "latency")),
        "",
        "## AFS-TrEDM-S6 Internal Throughput Comparison (MB/s)",
        "",
        _md_table(_metric_headers(internal), _metric_rows(summary, internal, "MBps")),
        "",
        "## AFS-TrEDM-S6 Internal Latency Comparison (ms per message)",
        "",
        _md_table(_metric_headers(internal), _metric_rows(summary, internal, "latency")),
        "",
        "## AFS-TrEDM-S6 Internal Cycles per Hash Comparison",
        "",
        _md_table(_metric_headers(internal), _metric_rows(summary, internal, "cycles_per_hash")),
        "",
        "## AFS-TrEDM-S6 Internal Cycles per Byte Comparison",
        "",
        _md_table(_metric_headers(internal), _metric_rows(summary, internal, "cycles_per_byte")),
    ]
    if include_sha3:
        lines.extend(
            [
                "",
                "## SHA3-512 Baseline Throughput Comparison (MB/s)",
                "",
                _md_table(_metric_headers(sha3), _metric_rows(summary, sha3, "MBps")),
                "",
                "## SHA3-512 Baseline Latency Comparison (ms per message)",
                "",
                _md_table(_metric_headers(sha3), _metric_rows(summary, sha3, "latency")),
                "",
                "## SHA3-512 Baseline Cycles per Hash Comparison",
                "",
                _md_table(_metric_headers(sha3), _metric_rows(summary, sha3, "cycles_per_hash")),
                "",
                "## SHA3-512 Baseline Cycles per Byte Comparison",
                "",
                _md_table(_metric_headers(sha3), _metric_rows(summary, sha3, "cycles_per_byte")),
                "",
                "## Combined 512 Cycles per Byte Summary",
                "",
                _md_table(["Backend", "1MiB cycles/byte", "Metric class"], _combined_512_cpb_rows(summary, combined_512)),
            ]
        )
    lines.extend(
        [
            "",
            "AFS-TrEDM-S6 Batch16 AVX512 norm is a multi-buffer normalized throughput backend. Its latency, cycles/hash, and cpb are normalized per message across a 16-message batch and must not be interpreted as single-message latency.",
            "",
            "AFS-TrEDM-S6 Optimized AVX2 hybrid is a single-message backend using an AVX2 AFS-64 nonlinear layer plus generated opt64 S6 linear layer.",
            "It is not a full AVX2 implementation of the S6 linear layer unless explicitly stated.",
            "",
            "## SHA3-512 baseline taxonomy",
            "",
            "- SHA3-512 Reference C (XKCP readable): official XKCP Standalone/CompactFIPS202/C/Keccak-readable-and-compact.c, reference-style baseline.",
            "- SHA3-512 XKCP CompactFIPS202 (XKCP more-compact): distinct official XKCP Standalone/CompactFIPS202/C/Keccak-more-compact.c compact baseline.",
            "- SHA3-512 OpenSSL EVP: single-message deployment baseline measured through OpenSSL EVP_sha3_512().",
            "- SHA3-512 Reference C and XKCP CompactFIPS202 are external baselines, not AFS-TrEDM implementations.",
            "- None of these SHA3 baselines is part of AFS-TrEDM's Reference_Implementation or Optimized_Implementation.",
            "- All SHA3-512 baselines are fresh single-message baselines in this package.",
            "- There is no true SHA3-512 Batch16 backend in this Stage16 package.",
            "Display labels 1KB, 64KB, and 1MB correspond to 1024, 65536, and 1048576 bytes in CSV metadata.",
        ]
    )
    if metadata:
        lines.extend(["", *_latest_failed_backend_notes(metadata)])
    return "\n".join(lines) + "\n"
