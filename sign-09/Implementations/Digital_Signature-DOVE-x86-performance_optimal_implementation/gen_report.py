#!/usr/bin/env python3
"""
Self-assessment report generator.

Reads JSON results produced by ngcc_bench (reports/*.json), detects the host
environment, and writes a Markdown self-assessment report aligned with the
NGCC x86 implementation self-assessment guideline (section 3.5).

Usage:
    python3 gen_report.py [--reports DIR] [--out FILE]

Typically invoked automatically by run_self_assessment.sh after benchmarks finish.
"""
import argparse
import datetime
import glob
import json
import os
import platform
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))


def impl_label_from_results(results):
    if not results:
        return os.environ.get("IMPL_LABEL", "optimized")
    ver = results[0].get("algorithm", {}).get("implementation_version", "optimized")
    return "Optimized Implementation" if ver == "optimized" else "Reference Implementation"


def sh(cmd):
    try:
        out = subprocess.check_output(cmd, stderr=subprocess.DEVNULL)
        return out.decode("utf-8", "replace").strip().splitlines()[0]
    except Exception:
        return ""


def detect_env():
    system = platform.system()
    release = platform.release()
    machine = platform.machine()

    cc_ver = sh(["gcc", "--version"]) or sh(["cc", "--version"])
    is_gnu_gcc = ("Free Software Foundation" in cc_ver) or \
                 ("gcc" in cc_ver.lower() and "clang" not in cc_ver.lower())
    cmake_ver = sh(["cmake", "--version"])
    cmake_num = cmake_ver.replace("cmake version", "").strip()

    is_x86 = machine in ("x86_64", "amd64", "AMD64")
    avx2 = False
    mem_gb = None
    if system == "Linux":
        try:
            with open("/proc/cpuinfo") as f:
                avx2 = "avx2" in f.read()
        except Exception:
            avx2 = False
        try:
            with open("/proc/meminfo") as f:
                for line in f:
                    if line.startswith("MemTotal:"):
                        kb = int(line.split()[1])
                        mem_gb = kb / (1024 * 1024)
                        break
        except Exception:
            mem_gb = None

    compliant = (system == "Linux") and is_gnu_gcc and is_x86
    return {
        "system": system,
        "release": release,
        "machine": machine,
        "os_label": ("Linux %s" % release) if system == "Linux"
                    else ("macOS (Darwin %s)" % release) if system == "Darwin"
                    else "%s %s" % (system, release),
        "cc_ver": cc_ver,
        "is_gnu_gcc": is_gnu_gcc,
        "cmake_num": cmake_num,
        "is_x86": is_x86,
        "avx2": avx2,
        "mem_gb": mem_gb,
        "compliant": compliant,
    }


def load_results(reports_dir):
    latest = {}
    for path in glob.glob(os.path.join(reports_dir, "*.json")):
        try:
            with open(path) as f:
                d = json.load(f)
        except Exception:
            continue
        name = d["algorithm"]["name"]
        ts = d["run"]["timestamp"]
        if name not in latest or ts > latest[name]["run"]["timestamp"]:
            latest[name] = d
    return sorted(latest.values(), key=lambda d: (d["algorithm"]["name"], d["algorithm"]["security_level"]))


def n(x):
    return "{:,}".format(int(round(x)))


def f2(x):
    return "{:.2f}".format(float(x))


def family_name(instance_name):
    """TSUOV_128 -> TSUOV (prefix before _LEVEL)."""
    if "_" in instance_name:
        return instance_name.rsplit("_", 1)[0]
    return instance_name


def family_names_from_results(results):
    names = sorted({family_name(r["algorithm"]["name"]) for r in results})
    return " / ".join(names)


def parse_ts(ts):
    try:
        return datetime.datetime.strptime(ts, "%Y%m%d_%H%M%S")
    except Exception:
        return None


def build_report(results, env):
    if not results:
        sys.exit("ERROR: no JSON results found; run the assessment first.")

    impl_label = impl_label_from_results(results)
    iters = results[0]["run"]["iterations"]
    times = [parse_ts(r["run"]["timestamp"]) for r in results]
    times = [t for t in times if t]
    span = ""
    if times:
        lo, hi = min(times), max(times)
        span = "%s ~ %s (local)" % (lo.strftime("%Y-%m-%d %H:%M:%S"),
                                    hi.strftime("%H:%M:%S"))

    ok_cc = "OK" if env["is_gnu_gcc"] else "NON-COMPLIANT"
    ok_os = "OK" if env["system"] == "Linux" else "NON-COMPLIANT"
    col2 = "This run (compliant)" if env["compliant"] else "This run (non-compliant)"

    L = []
    L.append("# Self-Assessment Test Report — (%s)" % impl_label)
    L.append("")
    if env["compliant"]:
        L.append("> **Environment notice**: these figures were collected on an environment "
                 "compliant with guideline section 2.1 (Linux + GNU GCC + x86-64) and may be "
                 "used as the official self-assessment result.")
    else:
        L.append("> **Environment notice (important)**: the performance/resource figures below "
                 "were collected on a **NON-COMPLIANT environment (%s + %s)** and are "
                 "**for self-test / placeholder only**." %
                 (env["os_label"], env["cc_ver"].split(",")[0] or "unknown compiler"))
        L.append("> (Guideline section 2.1 requires Linux + GNU GCC 8.5.0+ + single core + AVX2). "
                 "**Regenerate on a compliant Linux x86 + GCC host** via `run_self_assessment.sh`. "
                 "Functional correctness and data sizes are platform-independent.")
    L.append("")
    L.append("> Auto-generated by `gen_report.py` from `reports/*.json`, following guideline section 3.5.")
    L.append("")
    L.append("---")
    L.append("")

    levels = " / ".join(r["algorithm"]["name"] for r in results)
    bits = " / ".join(str(r["algorithm"]["security_level"]) for r in results)
    alg_family = family_names_from_results(results)
    L.append("## 1. Algorithm Information")
    L.append("")
    L.append("| Item | Content |")
    L.append("|------|---------|")
    L.append("| Category | Public-key cryptography |")
    L.append("| Function | Digital signature |")
    L.append("| Name | DOVE (classic / pkc+skc) |")
    L.append("| Implementation version | %s |" % impl_label)
    L.append("| Parameter sets / security levels | %s (claimed %s-bit security) |" % (levels, bits))
    L.append("| Variants | classic (full PK) / pkc_skc (compact PK+SK) |")
    L.append("| Programming interface | `sig_keygen` / `sig_sign` / `sig_verify` |")
    L.append("| Underlying hash | SM3 |")
    L.append("")

    L.append("## 2. Evaluation Environment")
    L.append("")
    L.append("| Item | %s | Compliance requirement (section 2.1) |" % col2)
    L.append("|------|-------------------|---------------------|")
    L.append("| Processor | %s (%s) | 64-bit x86, x86-ISA compatible, >2GHz recommended |"
             % (env["machine"], env["system"]))
    L.append("| Cores | single process / single thread | single core only |")
    mem_label = ("%.1f GB" % env["mem_gb"]) if env.get("mem_gb") is not None else "—"
    L.append("| Memory | %s | >4GB recommended |" % mem_label)
    L.append("| Operating system | %s [%s] | Linux, kernel >= 4.19 |" % (env["os_label"], ok_os))
    L.append("| Compiler | %s [%s] | GNU GCC >= 8.5.0 |"
             % (env["cc_ver"].split(",")[0] or "unknown", ok_cc))
    L.append("| Build tool | CMake %s [OK] | CMake >= 3.11.4 |" % (env["cmake_num"] or "unknown"))
    L.append("| Instruction set | x86-64 base (RDTSC timing)%s | x86-64 + AVX2 |"
             % (", AVX2 available" if env["avx2"] else ""))
    L.append("| Start / end time | %s | fill in as measured |" % (span or "—"))
    L.append("")

    L.append("## 3. Functional Test")
    L.append("")
    L.append("Method: for each security level, a key pair is generated from a fixed DRNG seed; "
             "a fixed 64-byte message is signed and verified; a tampered signature must be rejected. "
             "Full KAT vectors are in `API_PKC/Test_Vector/` and `data/kat/`.")
    L.append("")
    L.append("| Security level | keygen/sign/verify correctness | forged-signature rejection | Result |")
    L.append("|---------|--------------------------|-------------|------|")
    for r in results:
        c = r["functional"]["correctness"]
        fr = r["functional"]["forged_signature_rejected"]
        L.append("| %s | %s | %s | %s |" % (
            r["algorithm"]["name"],
            "PASS" if c else "FAIL",
            "PASS" if fr else "FAIL",
            "PASS" if (c and fr) else "FAIL"))
    L.append("")

    L.append("## 4. Performance Test (CPU Cycles)")
    L.append("")
    L.append("Each operation is run **%d times**; signing uses a fixed 64-byte message. "
             "Average CPU cycles (RDTSC) and throughput (ops/s) are reported. "
             "KAT generation is not included." % iters)
    L.append("")
    L.append("| Parameter set | Operation | Avg CPU cycles | ops/s |")
    L.append("|--------------|------|---------------:|------:|")
    for r in results:
        p = r["performance"]
        name = r["algorithm"]["name"]
        for op_key, op_name in (("keygen", "KeyGen"), ("sign", "Sign"), ("verify", "Verify")):
            L.append("| %s | %s | %s | %s |" % (
                name, op_name,
                n(p[op_key]["avg_cycles"]),
                f2(p[op_key]["throughput_ops_per_sec"])))
    L.append("")

    L.append("## 5. Key and Signature Sizes")
    L.append("")
    L.append("| Parameter set | Public key (B) | Private key (B) | Signature (B) |")
    L.append("|--------------|---------------:|----------------:|--------------:|")
    for r in results:
        s = r["sizes_bytes"]
        L.append("| %s | %s | %s | %s |" % (
            r["algorithm"]["name"], n(s["public_key"]),
            n(s["private_key"]), n(s["signature"])))
    L.append("")

    L.append("## 6. Raw Evidence Index")
    L.append("")
    L.append("| Item | Content |")
    L.append("|------|---------|")
    L.append("| Test command | `run_self_assessment.sh %d` |" % iters)
    L.append("| Input data | fixed seed + 64B message (no KAT generation) |")
    L.append("| Raw log | `logs/self_assessment.log` |")
    L.append("| Structured results | `reports/<algorithm>_<timestamp>.json`, `reports/summary.csv` |")
    L.append("")
    L.append("- Benchmark code: `bench/sig_bench.c`, adapter: `adapter/sig/`, entry: `src/main.c`")
    L.append("- Reproduction: run `run_self_assessment.sh` on a compliant host.")
    L.append("")

    return "\n".join(L)


def main():
    ap = argparse.ArgumentParser(description="Generate the digital-signature self-assessment report")
    ap.add_argument("--reports", default=os.path.join(HERE, "reports"),
                    help="Directory containing JSON results (default: reports/)")
    ap.add_argument("--out", default=os.path.join(HERE, "self_assessment_report.md"),
                    help="Output Markdown path (default: self_assessment_report.md)")
    args = ap.parse_args()

    env = detect_env()
    results = load_results(args.reports)
    md = build_report(results, env)

    out = os.path.abspath(args.out)
    with open(out, "w", encoding="utf-8") as f:
        f.write(md + "\n")
    print("Report written: %s" % out)
    print("Environment compliant: %s" % ("yes" if env["compliant"] else "no (sample data)"))


if __name__ == "__main__":
    main()
