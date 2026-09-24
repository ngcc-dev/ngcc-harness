#!/usr/bin/env python3
import json
import math
import sys


def fail(msg: str) -> None:
    print(f"json validation failed: {msg}", file=sys.stderr)
    sys.exit(1)


def expect(cond: bool, msg: str) -> None:
    if not cond:
        fail(msg)


def is_number(v) -> bool:
    return isinstance(v, (int, float)) and not isinstance(v, bool) and math.isfinite(float(v))


def require_key(obj, key: str):
    expect(isinstance(obj, dict), f"object expected before key '{key}'")
    expect(key in obj, f"missing key '{key}'")
    return obj[key]


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: validate_json_report.py REPORT_JSON SCHEMA_JSON", file=sys.stderr)
        return 2

    report_path = sys.argv[1]
    schema_path = sys.argv[2]

    with open(report_path, "r", encoding="utf-8") as f:
        report = json.load(f)
    with open(schema_path, "r", encoding="utf-8") as f:
        schema = json.load(f)

    schema_version = schema.get("properties", {}).get("schema_version", {}).get("const")
    expect(isinstance(schema_version, int), "schema const schema_version must be integer")

    expect(require_key(report, "schema_version") == schema_version, f"schema_version must be {schema_version}")
    expect(isinstance(require_key(report, "timestamp"), str), "timestamp must be string")
    expect(isinstance(require_key(report, "library"), str), "library must be string")

    report_metadata = report.get("report_metadata")
    if report_metadata is not None:
        expect(isinstance(report_metadata, dict), "report_metadata must be object")
        expect(isinstance(require_key(report_metadata, "generator"), str), "report_metadata.generator must be string")
        expect(isinstance(require_key(report_metadata, "generator_version"), str), "report_metadata.generator_version must be string")
        expect(isinstance(require_key(report_metadata, "json_out_path"), str), "report_metadata.json_out_path must be string")

    environment = report.get("environment")
    if environment is not None:
        expect(isinstance(environment, dict), "environment must be object")
        for k in ("hostname", "cwd", "sysname", "release", "version", "machine"):
            v = environment.get(k)
            expect(v is None or isinstance(v, str), f"environment.{k} must be string|null")

    opts = require_key(report, "options")
    for k in ("test_mask", "mode_mask", "iterations", "stability_max_cases", "msg_len", "digest_len_bits"):
        expect(isinstance(require_key(opts, k), int), f"options.{k} must be integer")
    for k in ("duration_hours", "stability_sample_ms"):
        expect(is_number(require_key(opts, k)), f"options.{k} must be finite number")
        expect(float(opts[k]) > 0.0, f"options.{k} must be > 0")
    expect(require_key(opts, "cycles") in ("on", "off"), "options.cycles must be on|off")
    kat = require_key(opts, "kat")
    expect(kat is None or isinstance(kat, str), "options.kat must be string|null")

    thr = require_key(opts, "stability_thresholds")
    stable_keys = (
        "stable_throughput_cv_percent",
        "stable_cycles_cv_percent",
        "stable_time_cv_percent",
        "stable_heap_growth_percent",
        "stable_heap_growth_abs_bytes",
        "stable_rss_growth_percent",
        "stable_rss_growth_abs_bytes",
        "stable_error_rate_percent",
    )
    warning_keys = (
        "warning_throughput_cv_percent",
        "warning_cycles_cv_percent",
        "warning_time_cv_percent",
        "warning_heap_growth_percent",
        "warning_heap_growth_abs_bytes",
        "warning_rss_growth_percent",
        "warning_rss_growth_abs_bytes",
        "warning_error_rate_percent",
    )
    for k in stable_keys + warning_keys:
        expect(is_number(require_key(thr, k)), f"threshold {k} must be finite number")
        expect(float(thr[k]) >= 0.0, f"threshold {k} must be >= 0")
    for s, w in zip(stable_keys, warning_keys):
        expect(float(thr[w]) >= float(thr[s]), f"threshold pair invalid: {w} < {s}")

    tests = report.get("test_results")
    if tests is None:
        tests = require_key(report, "tests")
    run_statuses = {"PASS", "FAIL", "STOPPED", "SKIPPED"}
    stability_statuses = run_statuses if schema_version <= 3 else {
        "STABLE", "WARNING", "UNSTABLE", "FAIL", "STOPPED", "SKIPPED"
    }
    for name in ("hash", "sig", "kem", "kex"):
        t = require_key(tests, name)
        expect(isinstance(require_key(t, "selected"), bool), f"tests.{name}.selected must be bool")
        expect(require_key(t, "correctness") in run_statuses, f"tests.{name}.correctness invalid status")
        expect(require_key(t, "performance") in run_statuses, f"tests.{name}.performance invalid status")
        expect(require_key(t, "stability") in stability_statuses, f"tests.{name}.stability invalid status")
        expect("kat" in t, f"tests.{name}.kat missing")
        expect("performance_metrics" in t, f"tests.{name}.performance_metrics missing")
        expect("stability_metrics" in t, f"tests.{name}.stability_metrics missing")
        expect("memory_metrics" in t, f"tests.{name}.memory_metrics missing")

        metrics = t["stability_metrics"]
        if metrics is not None and schema_version >= 4:
            expect(isinstance(require_key(metrics, "sample_count"), int), f"tests.{name}.stability_metrics.sample_count must be integer")
            expect(require_key(metrics, "status") in {"STABLE", "WARNING", "UNSTABLE"},
                   f"tests.{name}.stability_metrics.status invalid")

        mem_metrics = t["memory_metrics"]
        if mem_metrics is not None:
            expect(require_key(mem_metrics, "status") in run_statuses, f"tests.{name}.memory_metrics.status invalid")
            expect(isinstance(require_key(mem_metrics, "static_memory_bytes"), int),
                   f"tests.{name}.memory_metrics.static_memory_bytes must be integer")
            expect(isinstance(require_key(mem_metrics, "peak_memory_bytes"), int),
                   f"tests.{name}.memory_metrics.peak_memory_bytes must be integer")

    overall = require_key(report, "overall")
    expect(require_key(overall, "status") in ("PASS", "FAIL"), "overall.status invalid")

    return 0


if __name__ == "__main__":
    sys.exit(main())
