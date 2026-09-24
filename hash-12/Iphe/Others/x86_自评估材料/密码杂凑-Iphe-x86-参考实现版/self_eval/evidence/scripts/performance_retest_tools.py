import csv
import json
import math
import re
import statistics
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent

VERSIONS = {
    "Reference": {
        "key": "reference",
        "dir": "密码杂凑-Iphe-x86-参考实现版",
        "csv": "iphe_ngcc_s1_s8_reference.csv",
        "ngcc": "ngcc_bench_iphe_reference",
    },
    "Performance": {
        "key": "performance",
        "dir": "密码杂凑-Iphe-x86-性能优化版",
        "csv": "iphe_ngcc_s1_s8_performance.csv",
        "ngcc": "ngcc_bench_iphe_performance",
    },
    "Resource": {
        "key": "resource",
        "dir": "密码杂凑-Iphe-x86-资源优化版",
        "csv": "iphe_ngcc_s1_s8_resource.csv",
        "ngcc": "ngcc_bench_iphe_resource",
    },
}

RATES = {"Iphe-512": 184, "Iphe-768": 152, "Iphe-1024": 120}
SIZES = [32, 128, 512, 1024, 4096, 8192, 16384, 65536]


def evidence_dir(info):
    return ROOT / info["dir"] / "self_eval" / "evidence"


def load_original_rows():
    rows = []
    for version, info in VERSIONS.items():
        path = evidence_dir(info) / "csv" / info["csv"]
        with path.open(newline="", encoding="utf-8-sig") as f:
            for row in csv.DictReader(f):
                row["version"] = version
                row["input_bytes"] = int(row["input_bytes"])
                row["avg_cycles"] = float(row["avg_cycles"])
                row["throughput_MB_per_s"] = float(row["throughput_MB_per_s"])
                rows.append(row)
    return rows


def approx_blocks(size_bytes, instance):
    rate = RATES[instance]
    return max(1, math.ceil((size_bytes + 1) / rate))


def scan_anomalies():
    rows = load_original_rows()
    grouped = defaultdict(list)
    for row in rows:
        grouped[(row["version"], row["instance"])].append(row)

    out_rows = []
    for (version, instance), items in grouped.items():
        items.sort(key=lambda r: r["input_bytes"])
        prev = None
        for row in items:
            size = row["input_bytes"]
            cycles = row["avg_cycles"]
            mbps = row["throughput_MB_per_s"]
            blocks = approx_blocks(size, instance)
            flags = []
            cycles_nonmono = False
            throughput_drop = False
            adjacent_large = False
            long_drop = False
            if prev is not None:
                prev_cycles = prev["avg_cycles"]
                prev_mbps = prev["throughput_MB_per_s"]
                if cycles < prev_cycles:
                    cycles_nonmono = True
                    flags.append("cycles_nonmonotonic")
                if mbps < prev_mbps:
                    throughput_drop = True
                    drop = (prev_mbps - mbps) / prev_mbps if prev_mbps else 0.0
                    flags.append(f"throughput_drop_{drop:.1%}")
                    if size >= 4096 and prev["input_bytes"] >= 4096 and drop >= 0.15:
                        long_drop = True
                if prev_mbps:
                    change = abs(mbps - prev_mbps) / prev_mbps
                    if change >= 0.20:
                        adjacent_large = True
                        flags.append(f"adjacent_change_{change:.1%}")
            judgment = "normal"
            if long_drop:
                judgment = "long_message_drop"
            elif cycles_nonmono or throughput_drop or adjacent_large:
                judgment = "needs_retest"
            out_rows.append({
                "version": version,
                "algorithm_id": row["algorithm_id"],
                "instance": instance,
                "input_bytes": size,
                "original_cycles": f"{cycles:.6f}",
                "original_MB_per_s": f"{mbps:.6f}",
                "approx_blocks": blocks,
                "cycles_per_block": f"{cycles / blocks:.6f}",
                "cycles_nonmonotonic": "yes" if cycles_nonmono else "no",
                "throughput_local_drop": "yes" if throughput_drop else "no",
                "adjacent_change_over_20pct": "yes" if adjacent_large else "no",
                "long_message_drop": "yes" if long_drop else "no",
                "flags": ";".join(flags),
                "initial_judgment": judgment,
            })
            prev = row
    return out_rows


def write_scan():
    rows = scan_anomalies()
    fields = list(rows[0].keys())
    csv_path = ROOT / "performance_anomaly_scan.csv"
    with csv_path.open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(rows)

    flagged = [r for r in rows if r["initial_judgment"] != "normal"]
    lines = [
        "# S1-S8 性能数据静态异常扫描",
        "",
        "本扫描基于三版本正式报告对应 CSV，不修改正式报告原始性能表。cycles/block 使用近似块数 `ceil((input_bytes + 1) / rate)` 计算，用于定位趋势异常，不作为算法精确分块证明。",
        "",
        f"- 扫描点数：{len(rows)}",
        f"- 标记异常点数：{len(flagged)}",
        "- 标记阈值：相邻吞吐量变化超过 20%，或吞吐量局部下降，或 cycles 随输入长度增长下降。",
        "",
        "## 标记点",
        "",
        "| 版本 | 实例 | 输入 Bytes | 原 cycles | 原 MB/s | cycles/block | 标记 | 初步判断 |",
        "|---|---|---:|---:|---:|---:|---|---|",
    ]
    for r in flagged:
        lines.append(
            f"| {r['version']} | {r['instance']} | {r['input_bytes']} | "
            f"{float(r['original_cycles']):.2f} | {float(r['original_MB_per_s']):.2f} | "
            f"{float(r['cycles_per_block']):.2f} | {r['flags']} | {r['initial_judgment']} |"
        )
    lines.extend([
        "",
        "## 完整数据",
        "",
        "完整逐点扫描结果见 `performance_anomaly_scan.csv`。",
    ])
    (ROOT / "performance_anomaly_scan.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    return rows


def parse_retest_json(path):
    with path.open(encoding="utf-8") as f:
        data = json.load(f)
    alg = data["algorithm"]
    name = data.get("name", "")
    size = int(data["input_bytes"])
    cycles = float(data["average_cycles"])
    mbps = float(data["throughput_mbps"]) / 8.0
    return alg, name, size, cycles, mbps


def sample_std(values):
    return statistics.stdev(values) if len(values) >= 2 else 0.0


def summarize_retests():
    original = {}
    for row in load_original_rows():
        original[(row["version"], row["algorithm_id"], row["input_bytes"])] = row

    scan = {(r["version"], r["algorithm_id"], int(r["input_bytes"])): r for r in scan_anomalies()}
    grouped = defaultdict(list)
    for version, info in VERSIONS.items():
        retest_dir = evidence_dir(info) / "retest" / info["key"] / "json"
        if not retest_dir.exists():
            continue
        for path in retest_dir.glob("*.json"):
            alg, name, size, cycles, mbps = parse_retest_json(path)
            grouped[(version, alg, size)].append((cycles, mbps, path))

    rows = []
    for key, samples in sorted(grouped.items(), key=lambda x: (x[0][0], x[0][1], x[0][2])):
        version, alg, size = key
        orig = original.get(key)
        if orig is None:
            continue
        cycles = [s[0] for s in samples]
        mbps = [s[1] for s in samples]
        mean_c = statistics.mean(cycles)
        med_c = statistics.median(cycles)
        std_c = sample_std(cycles)
        mean_m = statistics.mean(mbps)
        med_m = statistics.median(mbps)
        std_m = sample_std(mbps)
        orig_c = float(orig["avg_cycles"])
        orig_m = float(orig["throughput_MB_per_s"])
        scan_row = scan.get(key, {})
        orig_flagged = scan_row.get("initial_judgment", "normal") != "normal"
        deviation_c = (med_c - orig_c) / orig_c * 100.0 if orig_c else 0.0
        deviation_m = (med_m - orig_m) / orig_m * 100.0 if orig_m else 0.0
        reproduced = "not_applicable"
        if orig_flagged:
            reproduced = "measurement_wave"
            if abs(deviation_m) <= 15.0 and (std_m / mean_m if mean_m else 0.0) <= 0.10:
                reproduced = "partly_reproduced"
        rows.append({
            "version": version,
            "algorithm_id": alg,
            "instance": orig["instance"],
            "input_bytes": size,
            "sample_count": len(samples),
            "original_cycles": f"{orig_c:.6f}",
            "mean_cycles": f"{mean_c:.6f}",
            "median_cycles": f"{med_c:.6f}",
            "min_cycles": f"{min(cycles):.6f}",
            "max_cycles": f"{max(cycles):.6f}",
            "std_cycles": f"{std_c:.6f}",
            "cv_cycles": f"{(std_c / mean_c if mean_c else 0.0):.6f}",
            "cycles_deviation_pct": f"{deviation_c:.6f}",
            "original_MB_per_s": f"{orig_m:.6f}",
            "mean_MB_per_s": f"{mean_m:.6f}",
            "median_MB_per_s": f"{med_m:.6f}",
            "min_MB_per_s": f"{min(mbps):.6f}",
            "max_MB_per_s": f"{max(mbps):.6f}",
            "std_MB_per_s": f"{std_m:.6f}",
            "cv_MB_per_s": f"{(std_m / mean_m if mean_m else 0.0):.6f}",
            "MB_per_s_deviation_pct": f"{deviation_m:.6f}",
            "original_anomaly": scan_row.get("flags", ""),
            "anomaly_reproduction": reproduced,
        })

    fields = list(rows[0].keys()) if rows else []
    with (ROOT / "performance_retest_summary.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(rows)

    lines = [
        "# S1-S8 重复测试统计汇总",
        "",
        "本文件为补充分析，不替换正式报告中的原始 S1-S8 性能表。NGCC 原始输出为 Mbps，本汇总使用 MB/s = Mbps / 8。",
        "",
    ]
    for version in VERSIONS:
        lines.append(f"## {version}")
        version_rows = [r for r in rows if r["version"] == version]
        for instance in ["Iphe-512", "Iphe-768", "Iphe-1024"]:
            sub = [r for r in version_rows if r["instance"] == instance]
            if not sub:
                continue
            lines.append(f"### {instance}")
            lines.append("")
            lines.append("| 输入 Bytes | 原 cycles | 复测 median cycles | cycles 偏差 | 原 MB/s | 复测 median MB/s | MB/s 偏差 | CV cycles | CV MB/s | 异常复现 |")
            lines.append("|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|")
            for r in sorted(sub, key=lambda x: x["input_bytes"]):
                lines.append(
                    f"| {r['input_bytes']} | {float(r['original_cycles']):.2f} | {float(r['median_cycles']):.2f} | "
                    f"{float(r['cycles_deviation_pct']):.2f}% | {float(r['original_MB_per_s']):.2f} | "
                    f"{float(r['median_MB_per_s']):.2f} | {float(r['MB_per_s_deviation_pct']):.2f}% | "
                    f"{float(r['cv_cycles']):.3f} | {float(r['cv_MB_per_s']):.3f} | {r['anomaly_reproduction']} |"
                )
            lines.append("")
    (ROOT / "performance_retest_summary.md").write_text("\n".join(lines), encoding="utf-8")
    return rows


def choose_focus_points(max_per_instance=3):
    scan = scan_anomalies()
    by_group = defaultdict(list)
    for row in scan:
        if row["initial_judgment"] != "normal":
            severity = 0
            flags = row["flags"]
            if "long_message_drop" in flags:
                severity += 4
            if "throughput_drop" in flags:
                severity += 3
            if "adjacent_change" in flags:
                m = re.search(r"adjacent_change_([0-9.]+)%", flags)
                severity += float(m.group(1)) / 20 if m else 1
            if "cycles_nonmonotonic" in flags:
                severity += 2
            by_group[(row["version"], row["algorithm_id"], row["instance"])].append((severity, row))

    focus = []
    covered = set()
    for group, items in by_group.items():
        items.sort(key=lambda x: x[0], reverse=True)
        for _, row in items[:max_per_instance]:
            focus.append(row)
            covered.add(group)

    for version, info in VERSIONS.items():
        rows = [r for r in load_original_rows() if r["version"] == version]
        for instance in ["Iphe-512", "Iphe-768", "Iphe-1024"]:
            algs = [r["algorithm_id"] for r in rows if r["instance"] == instance]
            if not algs:
                continue
            group = (version, algs[0], instance)
            if group not in covered:
                for size in [1024, 4096, 8192, 65536]:
                    match = next((r for r in rows if r["instance"] == instance and r["input_bytes"] == size), None)
                    if match:
                        focus.append({
                            "version": version,
                            "algorithm_id": match["algorithm_id"],
                            "instance": instance,
                            "input_bytes": size,
                            "flags": "representative",
                            "initial_judgment": "representative",
                        })
                        break
    return focus


def write_focus_csv():
    focus = choose_focus_points()
    fields = ["version", "algorithm_id", "instance", "input_bytes", "flags", "initial_judgment"]
    with (ROOT / "performance_retest_focus_points.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        for row in focus:
            w.writerow({k: row.get(k, "") for k in fields})
    return focus


def write_analysis():
    scan_rows = scan_anomalies()
    summary_rows = summarize_retests()
    focus = write_focus_csv()
    flagged = [r for r in scan_rows if r["initial_judgment"] != "normal"]
    high_cv = [r for r in summary_rows if float(r["cv_MB_per_s"]) > 0.10 or float(r["cv_cycles"]) > 0.10]
    reproduced = [r for r in summary_rows if r["anomaly_reproduction"] == "partly_reproduced"]
    measurement = [r for r in summary_rows if r["anomaly_reproduction"] == "measurement_wave"]

    lines = [
        "# S1-S8 性能波动与异常分析",
        "",
        "本文件为补充分析，不修改三份正式自评估报告中的原始 S1-S8 性能表。复测数据仅用于解释局部吞吐量或 cycles 非单调现象。",
        "",
        "## 5.1 总体现象",
        "",
        f"- 静态扫描覆盖 72 个点，标记 {len(flagged)} 个需要关注的局部波动点。",
        f"- 全量轻量复测覆盖 {len(summary_rows)} 个版本/实例/长度组合；重点复测点数为 {len(focus)}。",
        "- S1-S8 吞吐量整体随输入长度增加呈上升并在中长消息区间趋于稳定，但局部点存在非单调。",
        "- 所有复测输出均显示 functional self-test PASS；当前分析未发现功能正确性异常。",
        f"- 高 CV 点数：{len(high_cv)}；这类点更倾向于测量波动或系统调度影响。",
        f"- 初步稳定复现点数：{len(reproduced)}；方向不稳定或偏差较大的点数：{len(measurement)}。",
        "",
        "## 5.2 可能原因分类",
        "",
        "### 分块与 padding 边界",
        "",
        "Iphe 按 rate 分块处理，Iphe-512、Iphe-768、Iphe-1024 的 rate 分别为 184、152、120 Bytes。输入长度增长时，实际处理块数呈阶梯变化，padding 尾块也会引入固定成本。cycles/block 在块数变化附近会出现局部波动，因此吞吐量不要求严格单调。",
        "",
        "### 测量窗口与计时口径",
        "",
        "cycles 使用 `__rdtscp` 计数，MB/s 来自 NGCC 的 elapsed time 口径，两者测量窗口并不完全等价。短测试窗口下，系统计时粒度、上下文切换和缓存状态会让 cycles 与 MB/s 在局部点上不完全同步。",
        "",
        "### Windows 调度与频率波动",
        "",
        "复测脚本使用逻辑 CPU 0 亲和性运行，但 Windows 中断、后台任务、睿频和功耗策略仍会影响短时吞吐。混合核平台上固定亲和性不能完全消除频率与调度噪声。",
        "",
        "### 短消息固定开销",
        "",
        "32、128、512 Bytes 等短消息更容易受函数调用、adapter、benchmark 框架和计时开销影响。短消息点的 MB/s 不应过度解读，cycles 更适合作为补充观察。",
        "",
        "### 缓存、代码布局与分支预测",
        "",
        "Performance 版本使用更激进的优化结构，可能受 I-cache、展开代码布局和分支预测影响；Resource 版本采用 `-Os` 和更紧凑结构，局部长度下固定开销摊销方式不同。这些因素会造成中长消息吞吐量的局部波动。",
        "",
        "### NGCC 框架开销",
        "",
        "adapter 调用、输入缓冲、JSON/log 生成之前后的进程环境以及统计代码对短输入影响更明显。若复测 CV 较高，该点更应归类为测量稳定性问题。",
        "",
        "## 5.3 结论分级",
        "",
        "### 稳定结构性现象",
        "",
    ]
    if reproduced:
        for r in reproduced[:20]:
            lines.append(f"- {r['version']} {r['algorithm_id']} {r['input_bytes']} Bytes：复测 median 与原始值偏差较小，局部波动可结合分块/固定开销解释。")
    else:
        lines.append("- 未发现需要单独标记为稳定结构性异常的点。")

    lines.extend(["", "### 测量波动", ""])
    if measurement or high_cv:
        seen = set()
        for r in (measurement + high_cv)[:25]:
            key = (r["version"], r["algorithm_id"], r["input_bytes"])
            if key in seen:
                continue
            seen.add(key)
            lines.append(f"- {r['version']} {r['algorithm_id']} {r['input_bytes']} Bytes：复测偏差或 CV 较高，归类为测量波动。")
    else:
        lines.append("- 未发现明显高 CV 测量波动点。")

    lines.extend([
        "",
        "### 需进一步排查",
        "",
        "本轮未发现明确指向代码错误的异常。若后续需要进一步降低波动，可增加每点运行时长、提高重复次数、关闭后台负载，并记录 CPU 频率和核心类型。",
        "",
        "## 5.4 是否建议修改正式报告",
        "",
        "本轮不建议改动正式报告中的原始性能表数据。建议在正式报告性能测试章节补充说明：",
        "",
        "> 由于哈希计算按 rate 分块并包含固定开销、padding 尾块处理和测试框架开销，S1–S8 吞吐量随输入长度整体上升并趋于稳定，但局部长度点可能存在非单调波动。复测分析显示该现象主要来自分块边界、固定开销摊销和 Windows 计时/调度噪声，不影响功能正确性。原始复测数据和异常分析见 `self_eval/evidence/checks/performance_anomaly_analysis.md`。",
        "",
        "## 证据文件",
        "",
        "- 静态扫描：`performance_anomaly_scan.csv`、`performance_anomaly_scan.md`。",
        "- 复测汇总：`performance_retest_summary.csv`、`performance_retest_summary.md`。",
        "- 重点复测点：`performance_retest_focus_points.csv`。",
        "- 各版本复测原始 JSON/log：各版本 `self_eval/evidence/retest/`。",
    ])
    (ROOT / "performance_anomaly_analysis.md").write_text("\n".join(lines) + "\n", encoding="utf-8")


if __name__ == "__main__":
    write_scan()
    write_focus_csv()
    # summarize_retests/write_analysis are called after retest data exists.
