#!/usr/bin/env python3
import csv
import os
import platform
import statistics
import sys
from pathlib import Path


def read_status(path: Path):
    data = {}
    if path.exists():
        for line in path.read_text(errors="replace").splitlines():
            if "=" in line:
                k, v = line.split("=", 1)
                data[k] = v
    return data


def first_match(path: Path, prefix: str):
    if not path.exists():
        return "unknown"
    for line in path.read_text(errors="replace").splitlines():
        if line.startswith(prefix):
            if ":" in line:
                return line.split(":", 1)[1].strip()
            if "=" in line:
                return line.split("=", 1)[1].strip().strip('"')
            return line.strip()
    return "unknown"


def env_block(path: Path, marker: str):
    if not path.exists():
        return "unknown"
    lines = path.read_text(errors="replace").splitlines()
    for i, line in enumerate(lines):
        if line.startswith(marker):
            if i + 1 < len(lines):
                return lines[i + 1].strip()
    return "unknown"


def md_table(rows, headers):
    out = ["| " + " | ".join(headers) + " |", "|" + "|".join(["---"] * len(headers)) + "|"]
    for row in rows:
        out.append("| " + " | ".join(str(row.get(h, "")) for h in headers) + " |")
    return "\n".join(out)


def main():
    if len(sys.argv) != 2:
        print("usage: parse_results.py <result-dir>", file=sys.stderr)
        return 2
    result = Path(sys.argv[1])
    status = read_status(result / "status.env")
    env = result / "environment.txt"
    git = result / "git_status.txt"
    commit = "unknown"
    if git.exists():
        lines = [x.strip() for x in git.read_text(errors="replace").splitlines() if x.strip()]
        for line in lines:
            if len(line) == 40 and all(c in "0123456789abcdef" for c in line.lower()):
                commit = line
                break
    cpu_model = "unknown"
    if env.exists():
        for line in env.read_text(errors="replace").splitlines():
            if "Model name:" in line or "Model name" in line:
                cpu_model = line.split(":", 1)[1].strip()
                break
    gcc = env_block(env, "gcc --version:")
    os_name = first_match(env, "PRETTY_NAME")
    if os_name == "unknown":
        os_name = platform.platform()
    avx2 = "yes" if status.get("HAS_AVX2") == "1" else "no"
    aes = "yes" if status.get("HAS_AES") == "1" else "no"

    bench_rows = []
    bench_path = result / "benchmark.csv"
    if bench_path.exists():
        with bench_path.open(newline="") as f:
            bench_rows = list(csv.DictReader(f))
    size_rows = []
    sizes_path = result / "sizes.csv"
    if sizes_path.exists():
        with sizes_path.open(newline="") as f:
            size_rows = list(csv.DictReader(f))
    resource_rows = []
    resource_path = result / "resources.csv"
    if resource_path.exists():
        with resource_path.open(newline="") as f:
            resource_rows = list(csv.DictReader(f))

    median_rows = []
    for r in bench_rows:
        median_rows.append({
            "implementation": r.get("implementation", ""),
            "instance": r.get("instance", ""),
            "operation": r.get("operation", ""),
            "median_cycles": r.get("median_cycles", ""),
            "ops_per_sec": r.get("ops_per_sec", ""),
        })

    speed_rows = []
    ref = {}
    opt = {}
    for r in bench_rows:
        key = (r.get("instance"), r.get("operation"))
        if r.get("implementation") == "Reference":
            ref[key] = float(r.get("median_cycles") or 0)
        if r.get("implementation") == "Optimized":
            opt[key] = float(r.get("median_cycles") or 0)
    for key in sorted(set(ref) & set(opt)):
        rv, ov = ref[key], opt[key]
        speed_rows.append({
            "instance": key[0],
            "operation": key[1],
            "reference_median_cycles": f"{rv:.0f}",
            "optimized_median_cycles": f"{ov:.0f}",
            "speedup": f"{(rv / ov):.2f}" if ov > 0 else "NA",
        })

    summary = []
    summary.append(f"# API_PKC x86 self-evaluation summary")
    summary.append("")
    summary.append(f"* Result directory: `{result}`")
    summary.append(f"* Runs / warmup: `{status.get('RUNS', 'unknown')}` / `{status.get('WARMUP', 'unknown')}`")
    summary.append(f"* Git commit: `{commit}`")
    summary.append(f"* CPU model: {cpu_model}")
    summary.append(f"* OS: {os_name}")
    summary.append(f"* GCC version: {gcc}")
    summary.append(f"* AVX2 / AES-NI: {avx2} / {aes}")
    summary.append("")
    summary.append("## Test status")
    status_rows = [
        {"item": "Reference build", "status": status.get("reference_build", "not run")},
        {"item": "Reference smoke", "status": status.get("reference_smoke", "not run")},
        {"item": "Reference KAT", "status": status.get("reference_kat", "not run")},
        {"item": "Reference kat-repro", "status": status.get("reference_kat_repro", "not run")},
        {"item": "Reference full check", "status": status.get("reference_check", "not run")},
        {"item": "Optimized build", "status": status.get("optimized_build", "not run")},
        {"item": "Optimized smoke", "status": status.get("optimized_smoke", "not run")},
        {"item": "Optimized KAT", "status": status.get("optimized_kat", "not run")},
        {"item": "Optimized kat-repro", "status": status.get("optimized_kat_repro", "not run")},
        {"item": "Optimized full check", "status": status.get("optimized_check", "not run")},
        {"item": "Benchmark", "status": status.get("benchmark", "not run")},
        {"item": "Dependency scan", "status": status.get("dependency_scan", "not run")},
        {"item": "Sizes check", "status": status.get("sizes_check", "not run")},
        {"item": "Artifact scan", "status": status.get("artifact_scan", "not run")},
    ]
    summary.append(md_table(status_rows, ["item", "status"]))
    summary.append("")
    summary.append("## Sizes table")
    summary.append(md_table(size_rows, ["implementation", "instance", "pk_bytes", "sk_bytes", "ct_bytes", "ss_bytes"]) if size_rows else "not measured")
    summary.append("")
    summary.append("## Benchmark median cycles table")
    summary.append(md_table(median_rows, ["implementation", "instance", "operation", "median_cycles", "ops_per_sec"]) if median_rows else "not measured")
    summary.append("")
    summary.append("## Speedup table")
    summary.append(md_table(speed_rows, ["instance", "operation", "reference_median_cycles", "optimized_median_cycles", "speedup"]) if speed_rows else "not measured")
    summary.append("")
    summary.append("## Resource table")
    summary.append(md_table(resource_rows, ["implementation", "instance", "text", "data", "bss", "dec", "peak_rss_kb", "stack_usage_bytes", "status", "notes"]) if resource_rows else "not measured")
    summary.append("")
    summary.append("## Dependency and artifact notes")
    summary.append("* Dependency scan treats OpenSSL mentions without forbidden RNG calls as documentation-only; runtime forbidden randomness calls are FAIL.")
    summary.append("* DRNG and auxfunc scan logs are retained as `drng_scan.log` and `auxfunc_scan.log`.")
    summary.append("* `artifact_scan_after_clean.log` must be empty for PASS.")
    summary.append("")
    summary.append("## Remaining limitations")
    summary.append("* Full 1000-round correctness checks are marked NOT_RUN unless `--full-check` was supplied.")
    summary.append("* Stack usage is currently not measured and is recorded as NA.")
    summary.append("* Results are machine-specific and should be copied into the final LaTeX x86 self-evaluation report by the submitter.")
    (result / "summary.md").write_text("\n".join(summary) + "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
