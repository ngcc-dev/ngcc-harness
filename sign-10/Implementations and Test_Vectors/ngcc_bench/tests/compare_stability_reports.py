#!/usr/bin/env python3
import argparse
import json
import math
import sys


STATUS_RANK = {
    "STABLE": 0,
    "PASS": 0,
    "WARNING": 1,
    "UNSTABLE": 2,
    "FAIL": 3,
    "STOPPED": 4,
}


def to_num(v):
    if isinstance(v, bool):
        return None
    if isinstance(v, (int, float)) and math.isfinite(float(v)):
        return float(v)
    return None


def load_json(path):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def get_test_node(report, algo):
    try:
        return report["tests"][algo]
    except Exception:
        return None


def get_metric(report, algo, key):
    test = get_test_node(report, algo)
    if not isinstance(test, dict):
        return None
    metrics = test.get("stability_metrics")
    if not isinstance(metrics, dict):
        return None
    return to_num(metrics.get(key))


def get_status(report, algo):
    test = get_test_node(report, algo)
    metrics = None
    top_level = None
    raw_status = None

    if not isinstance(test, dict):
        return None

    top_level = test.get("stability")
    metrics = test.get("stability_metrics")
    if isinstance(metrics, dict):
        raw_status = metrics.get("status")

    if top_level in ("STABLE", "WARNING", "UNSTABLE", "FAIL", "STOPPED", "SKIPPED"):
        return top_level
    if raw_status in ("STABLE", "WARNING", "UNSTABLE"):
        return raw_status
    if isinstance(top_level, str):
        return top_level
    return None


def get_status_rank(status):
    if status in (None, "SKIPPED"):
        return None
    return STATUS_RANK.get(status)


def fail(msg):
    print(f"compare failed: {msg}", file=sys.stderr)
    return False


def check_higher_is_worse(ok, algo, key, baseline, candidate, allow_ratio):
    limit = baseline * (1.0 + allow_ratio)
    if candidate > limit:
        return fail(f"{algo}.{key}: {candidate:.6f} > {limit:.6f} (baseline {baseline:.6f})") and ok
    return ok


def check_lower_is_worse(ok, algo, key, baseline, candidate, allow_ratio):
    limit = baseline * (1.0 - allow_ratio)
    if candidate < limit:
        return fail(f"{algo}.{key}: {candidate:.6f} < {limit:.6f} (baseline {baseline:.6f})") and ok
    return ok


def main():
    p = argparse.ArgumentParser(description="Compare stability reports (baseline vs candidate).")
    p.add_argument("baseline")
    p.add_argument("candidate")
    p.add_argument("--allow-cv-regress-ratio", type=float, default=0.20,
                   help="Allowed relative regression ratio for CV metrics (default 0.20 = +20%%)")
    p.add_argument("--allow-throughput-mean-regress-ratio", type=float, default=0.10,
                   help="Allowed relative regression for throughput_mean_ops decrease (default 0.10 = -10%%)")
    p.add_argument("--allow-time-mean-regress-ratio", type=float, default=0.10,
                   help="Allowed relative regression for time_mean_ms increase (default 0.10 = +10%%)")
    p.add_argument("--allow-cycles-mean-regress-ratio", type=float, default=0.10,
                   help="Allowed relative regression for cycles_mean increase (default 0.10 = +10%%)")
    p.add_argument("--allow-memory-growth-abs", type=float, default=1.0,
                   help="Allowed absolute regression for heap_growth_percent (default +1.0)")
    p.add_argument("--allow-error-rate-abs", type=float, default=0.0,
                   help="Allowed absolute regression for error_rate_percent (default +0.0)")
    args = p.parse_args()

    base = load_json(args.baseline)
    cur = load_json(args.candidate)

    ok = True
    algos = ("hash", "sig", "kem", "kex")
    cv_keys = ("throughput_cv_percent", "time_cv_percent", "cycles_cv_percent")

    for algo in algos:
        base_status = get_status(base, algo)
        cur_status = get_status(cur, algo)
        base_rank = get_status_rank(base_status)
        cur_rank = get_status_rank(cur_status)

        if base_rank is not None and cur_rank is not None and cur_rank > base_rank:
            ok = fail(f"{algo}.status: candidate {cur_status} worse than baseline {base_status}") and ok

        for k in cv_keys:
            b = get_metric(base, algo, k)
            c = get_metric(cur, algo, k)
            if b is None or c is None or b < 0.0:
                continue
            ok = check_higher_is_worse(ok, algo, k, b, c, args.allow_cv_regress_ratio)

        b_thr = get_metric(base, algo, "throughput_mean_ops")
        c_thr = get_metric(cur, algo, "throughput_mean_ops")
        if b_thr is not None and c_thr is not None and b_thr > 0.0:
            ok = check_lower_is_worse(ok, algo, "throughput_mean_ops", b_thr, c_thr,
                                      args.allow_throughput_mean_regress_ratio)

        b_time = get_metric(base, algo, "time_mean_ms")
        c_time = get_metric(cur, algo, "time_mean_ms")
        if b_time is not None and c_time is not None and b_time > 0.0:
            ok = check_higher_is_worse(ok, algo, "time_mean_ms", b_time, c_time,
                                       args.allow_time_mean_regress_ratio)

        b_cycles = get_metric(base, algo, "cycles_mean")
        c_cycles = get_metric(cur, algo, "cycles_mean")
        if b_cycles is not None and c_cycles is not None and b_cycles > 0.0:
            ok = check_higher_is_worse(ok, algo, "cycles_mean", b_cycles, c_cycles,
                                       args.allow_cycles_mean_regress_ratio)

        b_heap = get_metric(base, algo, "heap_growth_percent")
        c_heap = get_metric(cur, algo, "heap_growth_percent")
        if b_heap is not None and c_heap is not None:
            if c_heap > b_heap + args.allow_memory_growth_abs:
                ok = fail(f"{algo}.heap_growth_percent: {c_heap:.6f} > {b_heap + args.allow_memory_growth_abs:.6f}") and ok

        b_heap_abs = get_metric(base, algo, "heap_growth_abs_bytes")
        c_heap_abs = get_metric(cur, algo, "heap_growth_abs_bytes")
        if b_heap_abs is not None and c_heap_abs is not None:
            if c_heap_abs > b_heap_abs + args.allow_memory_growth_abs * 1024:
                ok = fail(f"{algo}.heap_growth_abs_bytes: {c_heap_abs} > {b_heap_abs + int(args.allow_memory_growth_abs * 1024)}") and ok

        b_rss = get_metric(base, algo, "rss_growth_percent")
        c_rss = get_metric(cur, algo, "rss_growth_percent")
        if b_rss is not None and c_rss is not None:
            if c_rss > b_rss + args.allow_memory_growth_abs:
                ok = fail(f"{algo}.rss_growth_percent: {c_rss:.6f} > {b_rss + args.allow_memory_growth_abs:.6f}") and ok

        b_rss_abs = get_metric(base, algo, "rss_growth_abs_bytes")
        c_rss_abs = get_metric(cur, algo, "rss_growth_abs_bytes")
        if b_rss_abs is not None and c_rss_abs is not None:
            if c_rss_abs > b_rss_abs + args.allow_memory_growth_abs * 1024:
                ok = fail(f"{algo}.rss_growth_abs_bytes: {c_rss_abs} > {b_rss_abs + int(args.allow_memory_growth_abs * 1024)}") and ok

        b_err = get_metric(base, algo, "error_rate_percent")
        c_err = get_metric(cur, algo, "error_rate_percent")
        if b_err is not None and c_err is not None:
            if c_err > b_err + args.allow_error_rate_abs:
                ok = fail(f"{algo}.error_rate_percent: {c_err:.6f} > {b_err + args.allow_error_rate_abs:.6f}") and ok

    if not ok:
        return 1

    print("stability compare passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
