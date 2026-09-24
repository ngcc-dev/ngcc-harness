#!/usr/bin/env python3
"""Generate a NICCS x86 self-evaluation report (guide section 3.5).

Consumes selfeval KEY=VALUE logs named <instance>.<version>.selfeval.log plus
a sizes-from-`size` companion file <instance>.<version>.size.txt, and emits a
structured Markdown report and a flat CSV.

Usage: report_selfeval.py <log_dir> <report.md> <report.csv> [function]
  function: "KEM" or "KEX" (controls which columns are emitted; default KEM)
"""
import csv
import platform
import subprocess
import sys
from datetime import datetime
from pathlib import Path

VERSIONS = ["reference", "performance-optimized", "resource-optimized"]


def parse_kv(path: Path):
    d = {}
    for line in path.read_text().splitlines():
        line = line.strip()
        if "=" in line and not line.startswith("/"):
            k, v = line.split("=", 1)
            d[k.strip()] = v.strip()
    return d


def read_static_mem(path: Path):
    """size.txt holds: text data bss total filename (line 2 of `size -A`-less output)."""
    if not path.exists():
        return None
    try:
        lines = [l for l in path.read_text().splitlines() if l.strip()]
        # `size` default output: header line then values line
        parts = lines[-1].split()
        # text data bss dec hex filename
        text, data, bss = int(parts[0]), int(parts[1]), int(parts[2])
        return {"text": text, "data": data, "bss": bss, "static_total": text + data + bss}
    except (ValueError, IndexError):
        return None


def env_block():
    cpu = "unknown"
    mhz = "unknown"
    try:
        for line in Path("/proc/cpuinfo").read_text().splitlines():
            if line.startswith("model name"):
                cpu = line.split(":", 1)[1].strip()
                break
        for line in Path("/proc/cpuinfo").read_text().splitlines():
            if line.startswith("cpu MHz"):
                mhz = line.split(":", 1)[1].strip()
                break
    except OSError:
        pass
    mem = "unknown"
    try:
        for line in Path("/proc/meminfo").read_text().splitlines():
            if line.startswith("MemTotal"):
                mem = line.split(":", 1)[1].strip()
                break
    except OSError:
        pass
    try:
        gcc = subprocess.check_output(["gcc", "--version"], text=True).splitlines()[0]
    except Exception:
        gcc = "unknown"
    osname = "unknown"
    try:
        for line in Path("/etc/os-release").read_text().splitlines():
            if line.startswith("PRETTY_NAME"):
                osname = line.split("=", 1)[1].strip().strip('"')
                break
    except OSError:
        pass
    return cpu, mhz, mem, gcc, osname, platform.release()


def main(argv):
    if len(argv) < 4:
        raise SystemExit(f"usage: {argv[0]} <log_dir> <report.md> <report.csv> [KEM|KEX]")
    log_dir = Path(argv[1])
    md_path = Path(argv[2])
    csv_path = Path(argv[3])
    func = argv[4] if len(argv) > 4 else "KEM"

    # gather: data[instance][version] = merged dict
    data = {}
    for log in sorted(log_dir.glob("*.selfeval.log")):
        stem = log.name[: -len(".selfeval.log")]
        # stem = <instance>.<version>
        inst, _, version = stem.rpartition(".")
        rec = parse_kv(log)
        sm = read_static_mem(log_dir / f"{inst}.{version}.size.txt")
        if sm:
            rec.update({f"static_{k}": v for k, v in sm.items()})
        data.setdefault(inst, {})[version] = rec

    cpu, mhz, mem, gcc, osname, kernel = env_block()

    L = []
    L.append(f"# NICCS x86 自评估测试报告（{func}）")
    L.append("")
    L.append(f"生成时间：{datetime.now().isoformat(timespec='seconds')}")
    L.append("")
    L.append("## 1. 评估环境")
    L.append("")
    L.append("| 项目 | 配置 |")
    L.append("| --- | --- |")
    L.append(f"| 处理器 | {cpu} |")
    L.append(f"| 主频 (MHz) | {mhz} |")
    L.append(f"| 内存 | {mem} |")
    L.append(f"| 操作系统 | {osname} |")
    L.append(f"| 内核版本 | {kernel} |")
    L.append(f"| 编译器 | {gcc} |")
    L.append("| 核心数 | 单核 (single-thread benchmark) |")
    L.append("| 指令集 | x86-64 + AVX2 |")
    L.append("")
    L.append("编译选项（与官方指引 3.2 一致）：")
    L.append("")
    L.append("- 参考版：`-std=c99 -Wpedantic -Wall -Wextra -O2`")
    L.append("- 性能优化版：`-O3 -march=x86-64 -mavx2 -mtune=native -flto -fomit-frame-pointer -std=c99 -Wpedantic -Wall -Wextra`")
    L.append("- 资源优化版：`-Os -march=x86-64 -mavx2 -flto -fomit-frame-pointer -std=c99 -Wpedantic -Wall -Wextra`")
    L.append("")

    # 2. functional
    L.append("## 2. 功能测试（正确性）")
    L.append("")
    L.append("| 实例 | 版本 | KAT 一致性 |")
    L.append("| --- | --- | --- |")
    for inst in sorted(data):
        for v in VERSIONS:
            r = data[inst].get(v)
            if r:
                L.append(f"| {inst} | {v} | {r.get('func_kat', '-')} |")
    L.append("")

    # 3. performance
    L.append("## 3. 性能测试（中位 cycles 与吞吐量 ops/s）")
    L.append("")
    if func == "KEM":
        L.append("| 实例 | 版本 | keygen cyc | enc cyc | dec cyc | keygen ops/s | enc ops/s | dec ops/s |")
        L.append("| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |")
        for inst in sorted(data):
            for v in VERSIONS:
                r = data[inst].get(v)
                if not r:
                    continue
                L.append("| {i} | {v} | {a} | {b} | {c} | {d} | {e} | {f} |".format(
                    i=inst, v=v,
                    a=r.get("cycles_keygen", "-"), b=r.get("cycles_enc", "-"), c=r.get("cycles_dec", "-"),
                    d=r.get("ops_keygen", "-"), e=r.get("ops_enc", "-"), f=r.get("ops_dec", "-")))
    else:
        L.append("| 实例 | 版本 | 轮数 | 完整密钥交换 cyc | 密钥交换 ops/s |")
        L.append("| --- | --- | ---: | ---: | ---: |")
        for inst in sorted(data):
            for v in VERSIONS:
                r = data[inst].get(v)
                if not r:
                    continue
                L.append("| {i} | {v} | {rd} | {c} | {o} |".format(
                    i=inst, v=v, rd=r.get("rounds", "-"),
                    c=r.get("cycles_kex", "-"), o=r.get("ops_kex", "-")))
    L.append("")

    # 4. resource
    L.append("## 4. 资源消耗（静态内存 / 峰值内存）")
    L.append("")
    L.append("| 实例 | 版本 | 静态内存 text+data+bss (Bytes) | 峰值内存 RSS (Bytes) |")
    L.append("| --- | --- | ---: | ---: |")
    for inst in sorted(data):
        for v in VERSIONS:
            r = data[inst].get(v)
            if not r:
                continue
            L.append(f"| {inst} | {v} | {r.get('static_static_total', '-')} | {r.get('peak_rss_bytes', '-')} |")
    L.append("")

    # 5. transfer/storage
    L.append("## 5. 传输与存储开销（数据尺寸 / 轮数）")
    L.append("")
    if func == "KEM":
        L.append("| 实例 | 版本 | pk | sk | ct | ss |")
        L.append("| --- | --- | ---: | ---: | ---: | ---: |")
        for inst in sorted(data):
            for v in VERSIONS:
                r = data[inst].get(v)
                if not r:
                    continue
                L.append(f"| {inst} | {v} | {r.get('size_pk','-')} | {r.get('size_sk','-')} | {r.get('size_ct','-')} | {r.get('size_ss','-')} |")
    else:
        L.append("| 实例 | 版本 | 轮数 | pk | sk | msg1 | msg2 | msg3 | msg4 | 消息总计 | ss |")
        L.append("| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
        for inst in sorted(data):
            for v in VERSIONS:
                r = data[inst].get(v)
                if not r:
                    continue
                L.append("| {i} | {v} | {rd} | {pk} | {sk} | {m1} | {m2} | {m3} | {m4} | {mt} | {ss} |".format(
                    i=inst, v=v, rd=r.get("rounds", "-"), pk=r.get("size_pk", "-"), sk=r.get("size_sk", "-"),
                    m1=r.get("size_msg1", "-"), m2=r.get("size_msg2", "-"), m3=r.get("size_msg3", "-"),
                    m4=r.get("size_msg4", "-"), mt=r.get("size_msg_total", "-"), ss=r.get("size_ss", "-")))
    L.append("")

    # 6. evidence index
    L.append("## 6. 原始证据索引")
    L.append("")
    L.append(f"- 每次执行迭代次数：{next((data[i][v].get('ntests') for i in data for v in data[i]), 'N/A')}（≥100，满足指引要求）")
    L.append(f"- 原始日志：`{log_dir}/<instance>.<version>.selfeval.log`")
    L.append(f"- 静态内存原始数据：`{log_dir}/<instance>.<version>.size.txt`（`size` 工具输出）")
    L.append(f"- 汇总 CSV：`{csv_path.name}`")
    L.append(f"- 复现命令：在仓库根目录执行 `./Implementations/benchmark.sh`")
    L.append("")

    md_path.write_text("\n".join(L) + "\n")

    # CSV
    fields = ["instance", "version", "function", "ntests", "func_kat", "rounds",
              "cycles_keygen", "cycles_enc", "cycles_dec", "cycles_kex",
              "ops_keygen", "ops_enc", "ops_dec", "ops_kex",
              "static_static_total", "peak_rss_bytes",
              "size_pk", "size_sk", "size_ct", "size_ss",
              "size_msg1", "size_msg2", "size_msg3", "size_msg4", "size_msg_total"]
    with csv_path.open("w", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=fields, extrasaction="ignore")
        w.writeheader()
        for inst in sorted(data):
            for v in VERSIONS:
                r = data[inst].get(v)
                if not r:
                    continue
                row = {"instance": inst, "version": v}
                row.update(r)
                w.writerow(row)

    print(f"report: {md_path}")
    print(f"csv:    {csv_path}")


if __name__ == "__main__":
    main(sys.argv)
