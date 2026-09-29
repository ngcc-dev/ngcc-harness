#!/usr/bin/env python3
"""Benchmark campaign over every harness-built candidate instance.

    python3 performance/campaign.py check                   host readiness only
    python3 performance/campaign.py all  --cpu 2            build, KAT, calibrate,
                                                            measure, profile, hashcost
    python3 performance/campaign.py measure --run-dir DIR   resume one phase

Phases (each resumable; finished work in the run directory is skipped):

  build      Build every instance with the guide's reference flags for this
             architecture; an instance that does not build or does not pass its
             KATs that way is rebuilt with the harness defaults and re-tested.
             Only KAT-passing libraries go further. KAT logs are copied into
             the run directory as evidence.
  calibrate  Time one call of every operation (no warm-up) to plan iterations.
  measure    Five trials per operation, normally each in its own process so
             every trial gets a fresh address-space layout. Iteration counts,
             not wall time, bound the work: fast operations get >= 100 timed
             calls, slow ones fewer but never fewer than one per trial. The
             cheapest operations run first.
  profile    Relink reference libraries with performance/hashprof/wrap.c and
             record the time and call shapes of pseudohash, pseudoXOF, sm3hash
             and the ICCS DRNG inside each operation.
  baseline   Build the ICCS helpers sm3hash, pseudohash and pseudoXOF as hash
             instances (performance/iccs, candidate id "iccs"), check them
             against an independent SM3-based model, then calibrate and measure
             them at the hash message lengths exactly like a hash candidate: the
             reference point for the hash comparison in the summary. Adding it to
             an existing run requires the same fixed host state as its records.
  hashcost   Price every recorded call shape with the official ICCS helpers and
             with each hash candidate (performance/hashprof/hashcost.c), for the
             hash-substitution estimate in performance/report.py.

Records go to performance/runs/<host>-campaign-<UTC>/ (Git-ignored); review
them before publishing anything.
"""

from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import re
import shlex
import socket
import statistics
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parent.parent
PERF = ROOT / "performance"
HASH_SIZES = (32, 128, 512, 1024, 4096, 8192, 16384, 65536)
PERMISSIVE = ("-Wno-error=implicit-function-declaration -Wno-error=incompatible-pointer-types "
              "-Wno-error=int-conversion -Wno-error=implicit-int")
# Guide build configurations per architecture (doc/perf-x86.pdf, doc/perf-arm.pdf,
# section 3.2). -fPIC is needed for the harness's shared libraries, -D_GNU_SOURCE
# only exposes POSIX declarations, and the permissive flags only turn GCC 14
# errors for pre-C99-style code back into warnings; none of these changes the
# language standard or optimisation level.
ARCH = {
    "x86_64": {
        "reference": "-std=c99 -Wpedantic -Wall -Wextra -O2",
        "performance": "-O3 -march=x86-64 -mavx2 -mtune=native -flto -fomit-frame-pointer -std=c99 -Wpedantic -Wall -Wextra",
        "optimized_families": ["mithril", "scloud", "zcdmc", "bit"],
    },
    "aarch64": {
        "reference": "-std=c99 -Wpedantic -Wall -Wextra -O2",
        "performance": "-O3 -march=armv8.2-a+sve -flto -fomit-frame-pointer -std=c99 -Wpedantic -Wall -Wextra",
        "optimized_families": [],
    },
}
HARNESS_ADDITIONS = f"-fPIC -D_GNU_SOURCE {PERMISSIVE}"

# Iteration planning. Work is bounded by iteration counts; the timeouts below
# only catch hung processes and scale with the calibrated cost.
TRIALS = 5
TRIAL_TARGET_S = 1.0          # aim for about this much timed work per trial
MIN_TOTAL = 100               # guide: at least 100 measurements where affordable
MAX_ITERS = 20000             # per trial
OP_BUDGET_S = 900.0           # above this, reduce to what fits (>= 1 per trial)
SEPARATE_PROCESS_SETUP_S = 60.0
CALIBRATION_TIMEOUT_S = 6 * 3600


def log(run: Path, msg: str) -> None:
    line = f"{dt.datetime.now().strftime('%Y-%m-%d %H:%M:%S')} {msg}"
    print(line, flush=True)
    with (run / "campaign.log").open("a", encoding="utf-8") as f:
        f.write(line + "\n")


def sha256(path: Path) -> str:
    with path.open("rb") as f:
        return hashlib.file_digest(f, "sha256").hexdigest()


def sysfs(path: str) -> str | None:
    try:
        return Path(path).read_text(encoding="utf-8").strip()
    except OSError:
        return None


def elf_load_bytes(path: Path) -> int | None:
    """Sum of the library's PT_LOAD memory sizes (code, constants, data, bss)."""
    try:
        listing = subprocess.check_output(["readelf", "-lW", str(path)], text=True)
        return sum(int(f[5], 16) for line in listing.splitlines()
                   if (f := line.split()) and f[0] == "LOAD")
    except (OSError, subprocess.CalledProcessError, ValueError, IndexError):
        return None


def load(path: Path, default=None):
    return json.loads(path.read_text(encoding="utf-8")) if path.is_file() else default


def save(path: Path, data) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_suffix(path.suffix + ".tmp")
    tmp.write_text(json.dumps(data, indent=1) + "\n", encoding="utf-8")
    tmp.replace(path)


# ---------------------------------------------------------------- host

def system_id(explicit: str | None) -> str | None:
    """Benchmark system ID from performance/systems.csv (by hostname and architecture)."""
    import csv
    with (PERF / "systems.csv").open(encoding="utf-8") as f:
        rows = list(csv.DictReader((l for l in f if not l.startswith("#")), delimiter=";"))
    if explicit:
        return explicit if any(r["ID"] == explicit for r in rows) else None
    host, arch = socket.gethostname(), platform.machine()
    return next((r["ID"] for r in rows if r["Hostname"] == host and r["Arch"] == arch), None)


def clock_probe(cpu: int) -> dict | None:
    """Measured core clock of CPU cpu (performance/clockprobe.c): median, lowest
    and highest cycles per second over short busy windows, and the interquartile
    spread. None if the cycle counter is unavailable."""
    probe = PERF / "clockprobe"
    if not probe.is_file():
        subprocess.run(["make", "-s", "-C", str(PERF), "clockprobe"], capture_output=True)
    try:
        p = subprocess.run(["taskset", "-c", str(cpu), str(probe), "21", "50"],
                           capture_output=True, text=True, timeout=120)
    except (OSError, subprocess.TimeoutExpired):
        return None
    out = dict(l.split("\t", 1) for l in p.stdout.splitlines() if "\t" in l)
    if p.returncode != 0 or not out.get("clock_hz"):
        return None
    return {k: int(float(v)) for k, v in out.items()}


def fixed_clock(env: dict) -> float:
    """The run's fixed core clock in Hz: the cpufreq limit where cpufreq exists,
    otherwise the measured clock (a firmware-controlled clock, e.g. on AArch64)."""
    return float(env.get("cpufreq_max_khz") or 0) * 1e3 or float(env.get("core_clock_hz") or 0)


def host_problems(cpu: int) -> list[str]:
    problems = []
    arch = platform.machine()
    if arch not in ARCH:
        problems.append(f"architecture {arch} has no guide configuration")
    if sysfs("/proc/sys/kernel/perf_event_paranoid") not in ("-1", "0", "1", "2"):
        problems.append("hardware cycle counter unavailable: set kernel.perf_event_paranoid=2")
    if sysfs("/sys/devices/system/cpu/intel_pstate/no_turbo") == "0" or sysfs("/sys/devices/system/cpu/cpufreq/boost") == "1":
        problems.append("turbo/boost is enabled")
    gov = sysfs(f"/sys/devices/system/cpu/cpu{cpu}/cpufreq/scaling_governor")
    if gov not in (None, "performance"):
        problems.append(f"CPU {cpu} governor is {gov}, not performance")
    if gov and arch == "aarch64":
        # no turbo switch to rely on: the clock is fixed by pinning min = max
        lo = sysfs(f"/sys/devices/system/cpu/cpu{cpu}/cpufreq/scaling_min_freq")
        hi = sysfs(f"/sys/devices/system/cpu/cpu{cpu}/cpufreq/scaling_max_freq")
        if lo != hi:
            problems.append(f"CPU {cpu} clock not pinned (scaling_min_freq {lo} != scaling_max_freq {hi})")
    siblings = sysfs(f"/sys/devices/system/cpu/cpu{cpu}/topology/thread_siblings_list")
    if siblings and siblings != str(cpu):
        problems.append(f"CPU {cpu} shares its core with CPUs {siblings} (SMT on)")
    core = sysfs("/sys/devices/cpu_core/cpus")
    if core and cpu not in expand_cpus(core):
        problems.append(f"CPU {cpu} is not a performance core (cpu_core: {core})")
    # the orchestrator itself may be kept off the benchmark cores
    # (campaign_parallel.sh); what matters is that taskset can pin there
    if subprocess.run(["taskset", "-c", str(cpu), "true"], capture_output=True).returncode != 0:
        problems.append(f"CPU {cpu} cannot be used by this process (cpuset)")
    elif gov is None and not problems:
        # no cpufreq: the clock is set by firmware and cannot be pinned here, so it
        # must be shown to be steady (the measuring phases then use it as the
        # reference clock for disturbed trials)
        # a busy moment elsewhere on the machine can steal part of a probe
        # window, so a verdict of "not steady" needs three probes in a row
        for attempt in range(3):
            clk = clock_probe(cpu)
            if clk is None or clk["clock_iqr_hz"] <= 0.005 * clk["clock_hz"]:
                break
            time.sleep(5)
        if clk is None:
            problems.append(f"CPU {cpu}: no cpufreq and no cycle counter to measure the clock")
        elif clk["clock_iqr_hz"] > 0.005 * clk["clock_hz"]:
            problems.append(f"CPU {cpu} clock is not steady without cpufreq control "
                            f"({clk['clock_min_hz'] / 1e9:.3f}-{clk['clock_max_hz'] / 1e9:.3f} GHz)")
    return problems


def expand_cpus(spec: str) -> set[int]:
    out = set()
    for part in spec.split(","):
        a, _, b = part.partition("-")
        out.update(range(int(a), int(b or a) + 1))
    return out


def environment(cpu: int) -> dict:
    cpuinfo = Path("/proc/cpuinfo").read_text(encoding="utf-8")
    model = next((l.split(":", 1)[1].strip() for l in cpuinfo.splitlines()
                  if l.startswith("model name")), None)
    if model is None:
        # arm64 /proc/cpuinfo has no model name; lscpu decodes implementer and part
        try:
            ls = dict(l.split(":", 1) for l in subprocess.check_output(
                ["lscpu"], text=True, env={**os.environ, "LC_ALL": "C"}).splitlines() if ":" in l)
            model = " ".join(ls[k].strip() for k in ("Vendor ID", "Model name") if k in ls) or None
        except (OSError, subprocess.CalledProcessError):
            pass
    if model is None:
        model = next((l.split(":", 1)[1].strip() for l in cpuinfo.splitlines()
                      if l.startswith(("Hardware", "CPU part"))), "unknown")
    meminfo = Path("/proc/meminfo").read_text(encoding="utf-8")
    os_release = Path("/etc/os-release").read_text(encoding="utf-8")

    def version(cmd):
        try:
            return subprocess.check_output(cmd, text=True, stderr=subprocess.DEVNULL).splitlines()[0]
        except (OSError, subprocess.CalledProcessError, IndexError):
            return None
    clk = clock_probe(cpu) or {}
    machine = sysfs("/sys/firmware/devicetree/base/model") or sysfs("/sys/class/dmi/id/product_name")
    return {
        "hostname": socket.gethostname(), "architecture": platform.machine(), "cpu_model": model,
        "machine": machine.rstrip("\x00 ") if machine else None,
        "cpu_number": cpu, "logical_cpus_online": sysfs("/sys/devices/system/cpu/online"),
        "virtualized": "hypervisor" in cpuinfo,
        "memory_total_kib": next((int(l.split()[1]) for l in meminfo.splitlines() if l.startswith("MemTotal:")), None),
        "os": next((l.split("=", 1)[1].strip('"') for l in os_release.splitlines() if l.startswith("PRETTY_NAME=")), None),
        "kernel": platform.release(), "compiler": version(["gcc", "--version"]),
        "cmake": version(["cmake", "--version"]), "python": sys.version.split()[0],
        "perf_event_paranoid": sysfs("/proc/sys/kernel/perf_event_paranoid"),
        "cpufreq_governor": sysfs(f"/sys/devices/system/cpu/cpu{cpu}/cpufreq/scaling_governor"),
        "cpufreq_driver": sysfs(f"/sys/devices/system/cpu/cpu{cpu}/cpufreq/scaling_driver"),
        "cpufreq_min_khz": sysfs(f"/sys/devices/system/cpu/cpu{cpu}/cpufreq/scaling_min_freq"),
        "cpufreq_max_khz": sysfs(f"/sys/devices/system/cpu/cpu{cpu}/cpufreq/scaling_max_freq"),
        # hardware-measured average clock where the kernel provides it (arm64 AMU)
        "cpufreq_avg_khz": sysfs(f"/sys/devices/system/cpu/cpu{cpu}/cpufreq/cpuinfo_avg_freq"),
        "intel_pstate_no_turbo": sysfs("/sys/devices/system/cpu/intel_pstate/no_turbo"),
        "cpufreq_boost": sysfs("/sys/devices/system/cpu/cpufreq/boost"),
        "smt_control": sysfs("/sys/devices/system/cpu/smt/control"),
        # measured clock (performance/clockprobe.c); where cpufreq is absent this is
        # the run's reference clock, and core_clock_mhz (to 50 MHz) must not change
        "core_clock_hz": clk.get("clock_hz"),
        "core_clock_min_hz": clk.get("clock_min_hz"), "core_clock_max_hz": clk.get("clock_max_hz"),
        "core_clock_mhz": 50 * round(clk["clock_hz"] / 50e6) if clk.get("clock_hz") else None,
    }


# ---------------------------------------------------------------- discovery

def candidates(only: str | None) -> list[str]:
    ids = sorted(p.parent.name for p in ROOT.glob("*-*/Makefile")
                 if re.match(r"(sign|kem|kex|hash)-\d\d$", p.parent.name))
    return [c for c in ids if not only or re.search(only, c)]


def instances(cand: str) -> list[str]:
    out = subprocess.run(["make", "-s", "-C", str(ROOT / cand), "list"], capture_output=True, text=True).stdout
    return [line.split()[0] for line in out.splitlines() if "->" in line]


def kat_status(cand: str, label: str) -> tuple[str, str | None]:
    """(status, instance) from the instance's KAT log; status is PASS or the failure."""
    logf = ROOT / cand / "results" / f"{label}.log"
    if not logf.is_file():
        return "NOLOG", None
    text = logf.read_text(encoding="utf-8", errors="replace")
    inst = re.search(r"^instance\s*=\s*(\S+)", text, re.M)
    res = re.findall(rf"^RESULT {re.escape(cand)} (\S+) (\S+)", text, re.M)
    if not res:
        return "NORESULT", inst.group(1) if inst else None
    return res[-1][1], inst.group(1) if inst else res[-1][0]


def run_make(args: list[str], logf: Path, timeout: float = 6 * 3600) -> int:
    logf.parent.mkdir(parents=True, exist_ok=True)
    with logf.open("a", encoding="utf-8") as f:
        f.write(f"$ {shlex.join(args)}\n")
        f.flush()
        try:
            return subprocess.run(args, cwd=ROOT, stdout=f, stderr=subprocess.STDOUT, timeout=timeout).returncode
        except subprocess.TimeoutExpired:
            f.write("TIMEOUT\n")
            return 124


# ---------------------------------------------------------------- phase: build

def build_candidate(run: Path, args, cand: str, jobs: int) -> dict:
    """Build and KAT-test one candidate; returns {cand/label: entry}."""
    arch = ARCH[platform.machine()]
    guide = f"{arch['reference']} {HARNESS_ADDITIONS}"
    labels = instances(cand)
    out = {}
    if not labels:
        log(run, f"build {cand}: no instances listed")
        return out
    blog = run / "build" / f"{cand}.log"
    t0 = time.time()
    rc = run_make(["make", "-B", f"-j{jobs}", "-C", cand, "libs", f"NGCC_CFLAGS={guide}"], blog)
    if rc == 0:
        run_make(["make", f"-j{jobs}", "-C", cand, "test"], blog)
    for label in labels:
        status, inst = kat_status(cand, label) if rc == 0 else ("BUILDFAIL", None)
        extra = {}
        flags = "guide"
        if status != "PASS":
            # fall back to the harness defaults for this instance only
            r2 = run_make(["make", "-B", "-C", cand, f"lib/lib{label}.so"], blog)
            if r2 == 0:
                run_make(["make", "-C", cand, f"test-{label}"], blog)
                status2, inst = kat_status(cand, label)
            else:
                status2 = "BUILDFAIL"
            extra = {"guide_result": status}
            flags, status = "harness-default", status2
        entry = {"candidate": cand, "label": label, "instance": inst, "variant": "reference",
                 "flags": flags, "kat": status, **extra}
        lib = ROOT / cand / "lib" / f"lib{label}.so"
        if status in ("PASS", *TIMED_WITHOUT_PASS) and lib.is_file():
            kat_log = ROOT / cand / "results" / f"{label}.log"
            dst = run / "kat" / cand / f"{label}.log"
            dst.parent.mkdir(parents=True, exist_ok=True)
            dst.write_bytes(kat_log.read_bytes())
            entry.update(library=lib.relative_to(ROOT).as_posix(), library_sha256=sha256(lib),
                         elf_load_bytes=elf_load_bytes(lib),
                         kat_log=dst.relative_to(run).as_posix(), kat_log_sha256=sha256(dst))
        out[f"{cand}/{label}"] = entry
    passed = sum(e["kat"] == "PASS" for e in out.values())
    fallback = sum(e["flags"] != "guide" for e in out.values())
    log(run, f"build {cand}: {passed}/{len(labels)} KAT PASS, {fallback} on harness defaults ({time.time() - t0:.0f}s)")
    return out


def phase_build(run: Path, args) -> None:
    import threading
    from concurrent.futures import ThreadPoolExecutor
    state = load(run / "build.json", {"instances": {}})
    lock = threading.Lock()
    todo = [c for c in candidates(args.only)
            if args.rebuild or not (instances(c) and all(f"{c}/{l}" in state["instances"] for l in instances(c)))]
    workers = max(1, args.build_workers)
    jobs = max(1, args.jobs // workers)

    def work(cand):
        try:
            res = build_candidate(run, args, cand, jobs)
        except Exception as exc:          # keep going; the log says what broke
            log(run, f"build {cand}: ERROR {exc!r}")
            return
        with lock:
            state["instances"].update(res)
            save(run / "build.json", state)
    with ThreadPoolExecutor(max_workers=workers) as pool:
        list(pool.map(work, todo))
    if platform.machine() == "x86_64" and not args.only:
        for fam in ARCH["x86_64"]["optimized_families"]:
            key = f"optimized/{fam}"
            if key in state.get("optimized", {}):
                continue
            rc = run_make(["make", "-C", "performance", f"build-{fam}-avx2"], run / "build" / f"optimized-{fam}.log")
            state.setdefault("optimized", {})[key] = rc
            fam_map = {"mithril": "kem-22", "scloud": "kem-35", "zcdmc": "hash-31", "bit": "sign-02"}
            cand = fam_map[fam]
            for lib in sorted((ROOT / cand / "lib").glob("*-avx2.so")):
                label = lib.stem.removeprefix("lib")
                status, inst = kat_status(cand, label)
                entry = {"candidate": cand, "label": label, "instance": inst, "variant": "optimized-avx2",
                         "flags": "guide-performance", "kat": status}
                if status == "PASS":
                    dst = run / "kat" / cand / f"{label}.log"
                    dst.parent.mkdir(parents=True, exist_ok=True)
                    dst.write_bytes((ROOT / cand / "results" / f"{label}.log").read_bytes())
                    entry.update(library=lib.relative_to(ROOT).as_posix(), library_sha256=sha256(lib),
                                 elf_load_bytes=elf_load_bytes(lib),
                                 kat_log=dst.relative_to(run).as_posix(), kat_log_sha256=sha256(dst))
                state["instances"][f"{cand}/{label}"] = entry
            save(run / "build.json", state)
            log(run, f"build optimized {fam}: make rc={rc}")


# ---------------------------------------------------------------- driver runs

def driver(args, lib: str, op: str, size: int, trials: int, iters: int, warmups: int,
           timeout: float) -> dict:
    cmd = ["taskset", "-c", str(args.cpu), "performance/ngcc_perf", lib, op, str(size),
           str(trials), "1", str(iters), str(warmups)]
    t0 = time.time()
    try:
        p = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, timeout=timeout)
    except subprocess.TimeoutExpired:
        return {"status": "hard_timeout", "timeout_s": timeout, "command": cmd, "wall_s": time.time() - t0}
    meta, trials_out, status = {}, [], "missing"
    for line in p.stdout.splitlines():
        f = line.split("\t")
        if f[0] == "META" and len(f) == 3:
            meta[f[1]] = f[2]
        elif f[0] == "TRIAL" and len(f) == 5:
            trials_out.append({"measurements": int(f[2]), "seconds": float(f[3]), "cycles": int(f[4])})
        elif f[0] == "STATUS":
            status = f[1]
    return {"status": status if p.returncode == 0 else "failed", "exit_code": p.returncode,
            "meta": meta, "trials": trials_out, "stderr": p.stderr.strip()[-2000:],
            "command": cmd, "wall_s": time.time() - t0}


def operations(entry: dict, args, run: Path) -> list[tuple[str, int]]:
    cand = entry["candidate"]
    kind = cand.split("-")[0]
    if kind == "kem":
        return [("keygen", 0), ("enc", 0), ("dec", 0)]
    if kind == "sign":
        return [("keygen", 0), ("sign", 0), ("verify", 0)]
    if kind in ("hash", BASELINE):
        return [("hash", n) for n in HASH_SIZES]
    # KEX: the full exchange, then each step that the protocol actually runs
    probe = load(run / "calibrate" / cand / f"{entry['label']}__exchange.json")
    meta = probe.get("meta", {}) if probe else {}
    # declared passes (a non-interactive protocol declares 0; the official driver
    # still calls empty passes, which are not timed individually)
    passes = min(int(meta.get("kex_passes", 0)), int(meta.get("kex_passes_run", 0)))
    ops = [("exchange", 0), ("init_a", 0), ("init_b", 0)]
    ops += [(f"pass{k}", 0) for k in range(1, passes + 1)]
    return ops + [("derive_a", 0), ("derive_b", 0)]


def op_key(op: str, size: int) -> str:
    return f"{op}_{size}" if size else op


def kat_issues() -> list[dict]:
    import csv
    path = PERF / "kat_issues.csv"
    if not path.is_file():
        return []
    with path.open(encoding="utf-8") as f:
        return list(csv.DictReader((l for l in f if not l.startswith("#")), delimiter=";"))


# Instances slow enough to hold up everything else (one operation takes tens of
# minutes). With --defer they are skipped; --deferred-only processes just them.
DEFER_DEFAULT = r"^(sign-30/TRINE-512-|sign-32/UVW-512$)"


def prior_costs() -> dict[str, float]:
    """Wall seconds per instance (cand/label) in the published datasets: a
    machine-independent ordering of the work, used to balance shards."""
    cost: dict[str, float] = {}
    for f in (PERF / "data").glob("*/records/*/*.json"):
        r = load(f) or {}
        key = f"{r.get('candidate')}/{r.get('label')}"
        cost[key] = cost.get(key, 0.0) + sum(p.get("wall_s", 0.0) for p in r.get("processes", []))
    return cost


SHARDS: dict[str, int] = {}


def shard_of(key: str, n: int, defer: str | None = None) -> int:
    """Shard (0-based) of instance key among n: greedy longest-first over the
    prior costs of all instances this run built (deferred ones excluded), so the
    assignment is the same in every shard process and every phase; unknown
    instances count as 60 s."""
    if SHARDS.get("__n__") != (n, defer):
        SHARDS.clear()
        prior = prior_costs()
        keys = {k for k in set(prior) | set(SHARDS_BUILT) if not (defer and re.search(defer, k))}
        loads = [0.0] * n
        for k in sorted(keys, key=lambda k: (-prior.get(k, 60.0), k)):
            i = loads.index(min(loads))
            SHARDS[k] = i
            loads[i] += prior.get(k, 60.0)
        SHARDS["__n__"] = (n, defer)
    return SHARDS.get(key, int(hashlib.sha256(key.encode()).hexdigest(), 16) % n)


SHARDS_BUILT: list[str] = []


def selected(entry: dict, args) -> bool:
    key = f"{entry['candidate']}/{entry['label']}"
    if getattr(args, "only", None) and not re.search(args.only, entry["candidate"]):
        return False
    deferred = bool(getattr(args, "defer", None)) and bool(re.search(args.defer, key))
    if getattr(args, "deferred_only", False):
        return bool(re.search(args.defer or DEFER_DEFAULT, key))
    if deferred:
        return False
    if getattr(args, "shard", None):
        return shard_of(key, args.shard[1], getattr(args, "defer", None)) == args.shard[0] - 1
    return True


# KAT outcomes whose library is still timed (flagged in every record and report)
TIMED_WITHOUT_PASS = ("MISMATCH", "NOKAT", "PARTIAL", "CRYPTOFAIL", "TIMEOUT", "OVERFLOW")


def measurable(run: Path) -> list[dict]:
    """KAT-passing libraries, plus built libraries whose KAT did not pass
    (e.g. a documented vector mismatch); the latter are flagged in every record
    and report as timing of unvalidated code."""
    state = load(run / "build.json", {"instances": {}})
    issues = kat_issues()
    out = []
    for e in state["instances"].values():
        issue = next((i for i in issues if i["ID"] == e["candidate"] and re.search(i["Label"], e["label"])), None)
        if issue and issue["Action"] == "exclude":
            continue
        if e.get("kat") == "PASS" and e.get("library"):
            out.append(e)
            continue
        lib = ROOT / e["candidate"] / "lib" / f"lib{e['label']}.so"
        if e.get("kat") in TIMED_WITHOUT_PASS and e.get("library"):
            out.append({**e, "kat_not_passed": True})
        elif e.get("kat") in TIMED_WITHOUT_PASS and lib.is_file():
            out.append({**e, "library": lib.relative_to(ROOT).as_posix(), "library_sha256": sha256(lib),
                        "elf_load_bytes": elf_load_bytes(lib), "kat_log": None, "kat_log_sha256": None,
                        "kat_not_passed": True})
    return out


# ---------------------------------------------------------------- phase: calibrate

def phase_calibrate(run: Path, args) -> None:
    for entry in measurable(run):
        if not selected(entry, args):
            continue
        cand, label = entry["candidate"], entry["label"]
        # KEX: exchange first, so the step list is known
        ops = operations(entry, args, run)
        if cand.startswith("kex") and not (run / "calibrate" / cand / f"{label}__exchange.json").is_file():
            ops = [("exchange", 0)]
        for op, size in ops:
            out = run / "calibrate" / cand / f"{label}__{op_key(op, size)}.json"
            if out.is_file():
                continue
            r = driver(args, entry["library"], op, size, 1, 1, 0, CALIBRATION_TIMEOUT_S)
            t1 = r["trials"][0]["seconds"] if r.get("trials") else None
            r.update(t1_s=t1, setup_s=(r["wall_s"] - t1) if t1 is not None else None)
            save(out, r)
            log(run, f"calibrate {cand} {label} {op_key(op, size)}: "
                     f"{'%.6fs' % t1 if t1 is not None else r['status']} (process {r['wall_s']:.1f}s)")
        if cand.startswith("kex") and len(ops) == 1:
            for op, size in operations(entry, args, run)[1:]:
                out = run / "calibrate" / cand / f"{label}__{op_key(op, size)}.json"
                if not out.is_file():
                    r = driver(args, entry["library"], op, size, 1, 1, 0, CALIBRATION_TIMEOUT_S)
                    t1 = r["trials"][0]["seconds"] if r.get("trials") else None
                    r.update(t1_s=t1, setup_s=(r["wall_s"] - t1) if t1 is not None else None)
                    save(out, r)
                    log(run, f"calibrate {cand} {label} {op}: {'%.6fs' % t1 if t1 is not None else r['status']}")


def plan(t1: float, setup: float, iter_cost: float | None = None) -> dict:
    """iter_cost: wall cost of one iteration when it exceeds the timed part (a
    KEX step is timed inside a complete protocol run)."""
    t1 = max(iter_cost or t1, t1, 1e-8)
    iters = max(math.ceil(TRIAL_TARGET_S / t1), math.ceil(MIN_TOTAL / TRIALS))
    iters = min(iters, MAX_ITERS)
    if TRIALS * iters * t1 > OP_BUDGET_S:
        iters = max(1, math.floor(OP_BUDGET_S / (TRIALS * t1)))
    # an operation slower than the budget allows for five calls gets fewer trials,
    # but always at least one timed call
    trials = TRIALS if TRIALS * t1 <= OP_BUDGET_S else max(1, math.floor(OP_BUDGET_S / t1))
    warmups = 3 if t1 < 0.05 else (1 if t1 < 5 else 0)
    separate = setup < SEPARATE_PROCESS_SETUP_S
    procs = trials if separate else 1
    est = trials * iters * t1 + procs * (warmups * t1 + setup)
    return {"iterations_per_trial": iters, "warmups": warmups, "trials": trials,
            "separate_processes": separate, "estimated_s": est,
            "timeout_per_process_s": 3 * ((iters * (1 if separate else trials)) + warmups) * t1 + 3 * setup + 300}


def all_plans(run: Path, args) -> list[tuple[float, dict, str, int, dict]]:
    todo = []
    for entry in measurable(run):
        if not selected(entry, args):
            continue
        for op, size in operations(entry, args, run):
            cal = load(run / "calibrate" / entry["candidate"] / f"{entry['label']}__{op_key(op, size)}.json")
            if not cal or cal.get("t1_s") is None:
                continue
            iter_cost = None
            if entry["candidate"].startswith("kex") and op != "exchange":
                ex = load(run / "calibrate" / entry["candidate"] / f"{entry['label']}__exchange.json") or {}
                iter_cost = ex.get("t1_s")
            p = plan(cal["t1_s"], cal["setup_s"] or 0.0, iter_cost)
            todo.append((p["estimated_s"], entry, op, size, p))
    todo.sort(key=lambda t: t[0])
    return todo


# ---------------------------------------------------------------- phase: measure

def phase_measure(run: Path, args) -> None:
    todo = all_plans(run, args)
    remaining = sum(t[0] for t in todo if not (run / "records" / t[1]["candidate"] /
                    f"{t[1]['label']}__{op_key(t[2], t[3])}.json").is_file())
    log(run, f"measure: {len(todo)} operations planned, about {remaining / 3600:.1f} h of work outstanding")
    env = environment(args.cpu)
    for est, entry, op, size, p in todo:
        cand, label = entry["candidate"], entry["label"]
        out = run / "records" / cand / f"{label}__{op_key(op, size)}.json"
        if out.is_file():
            continue
        start = dt.datetime.now(dt.UTC).isoformat()
        load_start = os.getloadavg()
        runs, retried = [], 0
        max_hz = fixed_clock(env)
        cal = load(run / "calibrate" / cand / f"{label}__{op_key(op, size)}.json") or {}
        reuse = (p["trials"] == 1 and p["iterations_per_trial"] == 1 and p["warmups"] == 0
                 and cal.get("status") == "complete" and cal.get("trials"))

        def disturbed(r):
            """A trial whose cycles/second is well below the fixed clock lost the CPU
            to another task (the core is not isolated). KEX steps are excluded: their
            wall time includes counter system calls by design."""
            if not max_hz or r.get("meta", {}).get("timing_scope") == "per_call_step":
                return False
            return any(t["cycles"] and t["seconds"] > 0.01 and t["cycles"] / t["seconds"] < 0.95 * max_hz
                       for t in r.get("trials", []))

        def one(trials):
            nonlocal retried
            for attempt in range(3):
                r = driver(args, entry["library"], op, size, trials, p["iterations_per_trial"],
                           p["warmups"], p["timeout_per_process_s"])
                if r["status"] != "complete" or not disturbed(r) or attempt == 2:
                    r["disturbed_retries"] = attempt
                    return r
                retried += 1
        if reuse:
            # an operation this slow is planned as one call without warm-up, which is
            # exactly what calibration ran; that run is the measurement
            runs.append({**cal, "reused_calibration": True, "disturbed_retries": 0})
        elif p["separate_processes"]:
            for _ in range(p["trials"]):
                runs.append(one(1))
                if runs[-1]["status"] != "complete":
                    break
        else:
            runs.append(one(p["trials"]))
        trials = [t for r in runs for t in r.get("trials", [])]
        ok = all(r["status"] == "complete" for r in runs) and len(trials) == p["trials"]
        rec = {"schema": 2, "status": "complete" if ok else "failed_or_partial",
               "candidate": cand, "label": label, "instance": entry["instance"], "variant": entry["variant"],
               "flags": entry["flags"], "operation": op, "input_bytes": size, "plan": p,
               "kat": entry.get("kat"), "kat_not_passed": bool(entry.get("kat_not_passed")),
               "disturbed_retries": retried,
               "library": entry["library"], "library_sha256": entry["library_sha256"],
               "kat_log": entry["kat_log"], "kat_log_sha256": entry["kat_log_sha256"],
               "started_utc": start, "ended_utc": dt.datetime.now(dt.UTC).isoformat(),
               "load_average_start": load_start, "load_average_end": os.getloadavg(),
               "environment": env, "processes": runs}
        if trials:
            n = sum(t["measurements"] for t in trials)
            secs = sum(t["seconds"] for t in trials)
            per_trial = [t["seconds"] / t["measurements"] for t in trials]
            cyc_ok = all(t["cycles"] > 0 for t in trials)
            per_trial_cyc = [t["cycles"] / t["measurements"] for t in trials] if cyc_ok else []
            meta = runs[0].get("meta", {})
            rec.update({
                "trials": trials, "measurements": n, "guide_100_measurements_met": n >= MIN_TOTAL,
                "mean_seconds": secs / n, "median_trial_seconds": statistics.median(per_trial),
                "min_trial_seconds": min(per_trial), "max_trial_seconds": max(per_trial),
                "mean_cycles": sum(t["cycles"] for t in trials) / n if cyc_ok else None,
                "median_trial_cycles": statistics.median(per_trial_cyc) if cyc_ok else None,
                "operations_per_second": n / secs if secs else None,
                "throughput_MB_per_second": size * n / secs / 1e6 if size and secs else None,
                "sizes": {k: int(v) for k, v in meta.items()
                          if k.endswith("_bytes") and k not in ("baseline_rss_bytes", "peak_rss_bytes")
                          and v.isdigit()},
                "kex": {k: int(v) for k, v in meta.items() if k.startswith("kex_") and v.isdigit()},
                "build_flags": meta.get("build_flags"),
                "baseline_rss_bytes": max(int(r["meta"].get("baseline_rss_bytes", 0)) for r in runs if r.get("meta")),
                "peak_rss_bytes": max(int(r["meta"].get("peak_rss_bytes", 0)) for r in runs if r.get("meta")),
                "timing_scope": meta.get("timing_scope"),
            })
        save(out, rec)
        remaining -= est
        mc = rec.get("mean_cycles")
        log(run, f"measure {cand} {label} {op_key(op, size)}: {rec['status']} n={rec.get('measurements', 0)} "
                 f"{'%.0f cycles' % mc if mc else '%.6fs' % rec['mean_seconds'] if trials else ''} "
                 f"(left ~{max(remaining, 0) / 3600:.1f} h)")


# ---------------------------------------------------------------- phase: baseline

BASELINE = "iccs"
BASELINE_DIR = "performance/iccs"
# host settings that every record of one run must share
FIXED_ENV = ("hostname", "cpu_model", "cpu_number", "cpufreq_governor", "cpufreq_max_khz",
             "intel_pstate_no_turbo", "cpufreq_boost", "smt_control", "perf_event_paranoid",
             "core_clock_mhz")


def phase_baseline(run: Path, args) -> None:
    if args.shard:
        log(run, "baseline: not sharded; run it without --shard")
        raise SystemExit(2)
    env = environment(args.cpu)
    ref = next((r["environment"] for f in sorted((run / "records").rglob("*.json"))
                if not f.parent.name == BASELINE and (r := load(f)) and r.get("status") == "complete"
                and r.get("environment", {}).get("cpu_number") == args.cpu), None)
    if ref:
        # a key that the run's records predate is not compared
        diff = [f"{k}: run {ref.get(k)!r}, now {env.get(k)!r}" for k in FIXED_ENV
                if k in ref and ref.get(k) != env.get(k)]
        if diff:
            log(run, "baseline: host state differs from the run's records; not measuring: " + "; ".join(diff))
            raise SystemExit(2)
        for k in ("kernel", "compiler"):
            if ref.get(k) != env.get(k):
                log(run, f"baseline: note {k} differs from the run's records ({ref.get(k)!r} -> {env.get(k)!r})")
    state = load(run / "build.json", {"instances": {}})
    labels = instances(BASELINE_DIR)
    if not all(f"{BASELINE}/{l}" in state["instances"] for l in labels):
        arch = ARCH[platform.machine()]
        blog = run / "build" / f"{BASELINE}.log"
        rc = run_make(["make", "-B", f"-j{args.jobs}", "-C", BASELINE_DIR, "libs",
                       f"NGCC_CFLAGS={arch['reference']} {HARNESS_ADDITIONS}"], blog)
        for label in labels:
            lib = ROOT / BASELINE_DIR / "lib" / f"lib{label}.so"
            entry = {"candidate": BASELINE, "label": label, "instance": label, "variant": "iccs-baseline",
                     "flags": "guide", "kat": "BUILDFAIL"}
            if rc == 0 and lib.is_file():
                dst = run / "kat" / BASELINE / f"{label}.log"
                dst.parent.mkdir(parents=True, exist_ok=True)
                p = subprocess.run([sys.executable, str(PERF / "iccs/selftest.py"),
                                    lib.relative_to(ROOT).as_posix(), label],
                                   cwd=ROOT, capture_output=True, text=True)
                dst.write_text(p.stdout + p.stderr, encoding="utf-8")
                res = re.findall(rf"^RESULT {BASELINE} \S+ (\S+)", p.stdout, re.M)
                entry.update(kat=res[-1] if res else "NORESULT", library=lib.relative_to(ROOT).as_posix(),
                             library_sha256=sha256(lib), elf_load_bytes=elf_load_bytes(lib),
                             kat_log=dst.relative_to(run).as_posix(), kat_log_sha256=sha256(dst))
            state["instances"][f"{BASELINE}/{label}"] = entry
        save(run / "build.json", state)
        passed = sum(state["instances"][f"{BASELINE}/{l}"]["kat"] == "PASS" for l in labels)
        log(run, f"baseline build: make rc={rc}, {passed}/{len(labels)} self-tests PASS")
    sub = argparse.Namespace(**{**vars(args), "only": rf"^{BASELINE}$", "defer": None, "deferred_only": False})
    phase_calibrate(run, sub)
    phase_measure(run, sub)


# ---------------------------------------------------------------- phase: profile

def phase_profile(run: Path, args) -> None:
    relinked = {}
    for entry in measurable(run):
        cand, label = entry["candidate"], entry["label"]
        if entry["variant"] != "reference" or cand.startswith("hash"):
            continue
        if not selected(entry, args):
            continue
        for op, size in operations(entry, args, run):
            if cand.startswith("kex") and op != "exchange":
                continue   # the summary needs the whole exchange; steps would each rerun it
            out = run / "profile" / cand / f"{label}__{op_key(op, size)}.json"
            if out.is_file():
                continue
            if (cand, label) not in relinked:
                mk = "Makefile" if (ROOT / cand / "Makefile").is_file() else "../performance/kem-35.mk"
                p = subprocess.run([str(PERF / "hashprof/relink.sh"), cand, label, mk],
                                   capture_output=True, text=True)
                relinked[(cand, label)] = p.stdout.strip() if p.returncode == 0 else None
                if p.returncode:
                    save(out.parent / f"{label}__RELINK_FAILED.json", {"stderr": p.stderr[-2000:]})
                    log(run, f"profile {cand} {label}: relink failed")
            lib = relinked[(cand, label)]
            if not lib:
                continue
            cal = load(run / "calibrate" / cand / f"{label}__{op_key(op, size)}.json") or {}
            t1 = cal.get("t1_s") or 1.0
            if cand.startswith("kex") and op != "exchange":
                ex = load(run / "calibrate" / cand / f"{label}__exchange.json") or {}
                t1 = max(t1, ex.get("t1_s") or t1)
            iters = 1 if t1 > 60 else min(1000, max(3, math.ceil(1.0 / t1)))
            warm = 1 if t1 < 5 else 0
            setup = cal.get("setup_s") or 0.0
            cmd = ["taskset", "-c", str(args.cpu), "performance/hashprof/hashprof",
                   os.path.relpath(lib, ROOT), op, str(size),
                   str(iters), str(warm)]
            try:
                pr = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True,
                                    timeout=3 * (iters + warm) * t1 + 3 * setup + 300)
                stdout, rc, err = pr.stdout, pr.returncode, pr.stderr[-2000:]
            except subprocess.TimeoutExpired:
                stdout, rc, err = "", 124, "hard timeout"
            meta, calls, total = {}, [], None
            for line in stdout.splitlines():
                f = line.split("\t")
                if f[0] == "META" and len(f) == 3:
                    meta[f[1]] = f[2]
                elif f[0] == "TOTAL":
                    total = {"operations": int(f[1]), "ticks": int(f[2])}
                elif f[0] == "CALL":
                    calls.append({"fn": f[1], "in_bits": int(f[2]), "out_bits": int(f[3]),
                                  "calls": int(f[4]), "ticks": int(f[5])})
            rec = {"schema": 2, "candidate": cand, "label": label, "operation": op, "input_bytes": size,
                   "status": "complete" if rc == 0 and total else "failed", "command": cmd, "stderr": err,
                   "meta": meta, "total": total, "calls": calls}
            if total and total["ticks"]:
                by_fn = {}
                for c in calls:
                    by_fn[c["fn"]] = by_fn.get(c["fn"], 0) + c["ticks"]
                rec["share"] = {fn: t / total["ticks"] for fn, t in by_fn.items()}
                rec["hash_share"] = sum(v for k, v in rec["share"].items() if k != "drng")
            save(out, rec)
            log(run, f"profile {cand} {label} {op_key(op, size)}: {rec['status']} "
                     f"hash={100 * rec.get('hash_share', 0):.1f}% drng={100 * rec.get('share', {}).get('drng', 0):.1f}%")


# ---------------------------------------------------------------- phase: hashcost

def read_tsv_keys(path: Path, width: int) -> set:
    if not path.is_file():
        return set()
    keys = set()
    for line in path.read_text(encoding="utf-8").splitlines():
        f = line.split("\t")
        if len(f) == width + 1 and not f[0].startswith("#"):
            keys.add(tuple(f[:width]))
    return keys


def append_tsv(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    lines = [l for l in text.splitlines() if l and (not l.startswith("#unit") or not path.is_file())]
    with path.open("a", encoding="utf-8") as f:
        f.write("".join(l + "\n" for l in lines))


def phase_hashcost(run: Path, args) -> None:
    """Price every profiled call shape; incremental (only missing shapes are measured)."""
    shapes = set()
    for f in (run / "profile").rglob("*.json"):
        for c in (load(f) or {}).get("calls", []):
            shapes.add((c["fn"], c["in_bits"], c["out_bits"]))
    if not shapes:
        log(run, "hashcost: no profiled call shapes")
        return
    out = run / "hashcost" / "iccs.tsv"
    have = read_tsv_keys(out, 3)
    todo = sorted(sh for sh in shapes if (sh[0], str(sh[1]), str(sh[2])) not in have)
    if todo:
        inp = "".join(f"{fn} {i} {o}\n" for fn, i, o in todo)
        p = subprocess.run(["taskset", "-c", str(args.cpu), str(PERF / "hashprof/hashcost"), "iccs"],
                           input=inp, capture_output=True, text=True)
        append_tsv(out, p.stdout)
        log(run, f"hashcost iccs: {len(todo)} new shapes (rc={p.returncode})")
    # hash candidates: every input length that a substitution could need
    need_bits = set()
    for fn, i, o in shapes:
        if fn != "drng":
            need_bits.update((i, i + 32))   # direct hash, and counter-mode block (msg || ctr)
    hash_libs = [e for e in measurable(run) if e["candidate"].startswith("hash") and e["variant"] == "reference"]
    for e in hash_libs:
        rec = load(run / "records" / e["candidate"] / f"{e['label']}__hash_32.json") or {}
        digest_bits = 8 * rec.get("sizes", {}).get("digest_bytes", 0)
        if not digest_bits:
            continue
        dst = run / "hashcost" / e["candidate"] / f"{e['label']}.tsv"
        have = read_tsv_keys(dst, 2)
        missing = sorted(b for b in need_bits if (str(digest_bits), str(b)) not in have)
        if not missing:
            continue
        inp = "".join(f"{digest_bits} {b}\n" for b in missing)
        p = subprocess.run(["taskset", "-c", str(args.cpu), str(PERF / "hashprof/hashcost"), "lib",
                            str(ROOT / e["library"])], input=inp, capture_output=True, text=True,
                           timeout=24 * 3600)
        if p.returncode == 0:
            append_tsv(dst, p.stdout)
        log(run, f"hashcost {e['candidate']} {e['label']} ({digest_bits}-bit): {len(missing)} lengths, rc={p.returncode}")


# ---------------------------------------------------------------- phase: evidence

def phase_evidence(run: Path) -> int:
    """Attach the library and KAT log of every instance that was timed although its
    KATs do not pass to build.json and to its timing records, as the build phase
    now does itself (runs built before that stored neither). A library is only
    linked when its SHA-256 equals the one each record measured. Idempotent."""
    state = load(run / "build.json", {"instances": {}})
    linked = problems = 0
    for key, e in state["instances"].items():
        if e.get("kat") not in TIMED_WITHOUT_PASS or e.get("kat_log"):
            continue
        cand, label = e["candidate"], e["label"]
        recs = sorted((run / "records" / cand).glob(f"{label}__*.json"))
        lib, logf = ROOT / cand / "lib" / f"lib{label}.so", ROOT / cand / "results" / f"{label}.log"
        if not recs or not lib.is_file() or not logf.is_file():
            continue
        digest = sha256(lib)
        if any((load(f) or {}).get("library_sha256") != digest for f in recs):
            print(f"evidence {key}: library changed since it was timed; not linked", file=sys.stderr)
            problems += 1
            continue
        dst = run / "kat" / cand / f"{label}.log"
        dst.parent.mkdir(parents=True, exist_ok=True)
        dst.write_bytes(logf.read_bytes())
        e.update(library=lib.relative_to(ROOT).as_posix(), library_sha256=digest,
                 elf_load_bytes=elf_load_bytes(lib), kat_log=dst.relative_to(run).as_posix(),
                 kat_log_sha256=sha256(dst))
        for f in recs:
            r = load(f)
            r.update(kat_log=e["kat_log"], kat_log_sha256=e["kat_log_sha256"])
            save(f, r)
        linked += 1
    save(run / "build.json", state)
    print(f"evidence: {linked} instance(s) linked, {problems} problem(s)")
    return 1 if problems else 0


# ---------------------------------------------------------------- phase: publish

def phase_publish(run: Path, exclude: str | None = None) -> int:
    """Copy the evidence that the reports cite into performance/data/<system>/.

    Published: campaign.json, build.json, records/, profile/ (the timing and
    hash-profile records), kat/ (the KAT logs of the timed libraries) and the
    katcheck/ logs. Not published: calibration runs, build logs, hash-cost
    tables and generated KAT text. Paths under the repository are rewritten to
    repository-relative form; any other absolute path aborts the publication.
    With exclude (a regex over cand/label, --defer), the timing records and
    profiles of matching instances are left out, e.g. for a preliminary release
    while they are still running; their build and KAT evidence stays.
    """
    import shutil
    meta = load(run / "campaign.json") or {}
    sid = meta.get("system_id")
    if not sid:
        print(f"{run}: campaign.json has no system_id", file=sys.stderr)
        return 2
    dest = PERF / "data" / sid
    files = [run / "campaign.json", run / "build.json"]
    def kept(f: Path) -> bool:
        return not exclude or not re.search(exclude, f"{f.parent.name}/{f.name.split('__')[0]}")
    files += sorted(f for f in (run / "records").rglob("*.json") if kept(f))
    files += sorted(f for f in (run / "profile").rglob("*__*.json") if "RELINK" not in f.name and kept(f))
    files += sorted((run / "kat").rglob("*.log")) + sorted((run / "katcheck").glob("*.log"))
    prefix = str(ROOT) + "/"
    staged = {}
    for f in files:
        text = f.read_text(encoding="utf-8", errors="replace").replace(prefix, "").replace(str(ROOT), ".")
        for bad in ("/home/", "/tmp/", "/root/", "/Users/"):
            if bad in text:
                print(f"{f.relative_to(run)}: absolute path {bad!r} remains; not publishing", file=sys.stderr)
                return 2
        staged[f.relative_to(run).as_posix()] = text
    if dest.exists():
        shutil.rmtree(dest)
    for rel, text in staged.items():
        out = dest / rel
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(text, encoding="utf-8")
    print(f"publish: {len(staged)} files for system {sid} -> {dest.relative_to(ROOT)}/")
    return 0


# ---------------------------------------------------------------- main

def parse_shard(text: str) -> tuple[int, int]:
    k, _, n = text.partition("/")
    if not (k.isdigit() and n.isdigit() and 1 <= int(k) <= int(n)):
        raise argparse.ArgumentTypeError("expected K/N with 1 <= K <= N")
    return int(k), int(n)


def update_campaign(run: Path, fn) -> dict:
    """Read-modify-write campaign.json under a lock (shard processes share it)."""
    import fcntl
    with open(run / ".campaign.lock", "w") as lk:
        fcntl.flock(lk, fcntl.LOCK_EX)
        meta = load(run / "campaign.json", {})
        fn(meta)
        save(run / "campaign.json", meta)
    return meta


def main() -> int:
    global TRIAL_TARGET_S, OP_BUDGET_S
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("phase", choices=["check", "build", "baseline", "calibrate", "measure", "profile", "hashcost", "all",
                                      "plan", "publish", "evidence"])
    ap.add_argument("--cpu", type=int, default=2)
    ap.add_argument("--run-dir", type=Path, help="existing or new run directory (default: new)")
    ap.add_argument("--only", help="regex over candidate ids (e.g. '^kem-2')")
    ap.add_argument("--jobs", type=int, default=8, help="parallel build jobs (build phase only)")
    ap.add_argument("--rebuild", action="store_true",
                    help="build phase: rebuild and re-test the --only candidates even if already recorded")
    ap.add_argument("--build-workers", type=int, default=3, help="candidates built/KAT-tested at once")
    ap.add_argument("--defer", nargs="?", const=DEFER_DEFAULT, default=None,
                    help=f"skip very slow instances (regex over cand/label, default {DEFER_DEFAULT!r})")
    ap.add_argument("--deferred-only", action="store_true", help="process only the deferred instances")
    ap.add_argument("--system", help="system ID from performance/systems.csv (default: by hostname)")
    ap.add_argument("--shard", type=parse_shard, metavar="K/N",
                    help="calibrate/measure/profile only shard K of N (1-based); instances are divided "
                         "by prior run time, all operations of an instance in one shard; run one process "
                         "per shard, each on its own --cpu, with the same run directory")
    ap.add_argument("--trial-target", type=float, default=TRIAL_TARGET_S,
                    help=f"seconds of timed work per trial (default {TRIAL_TARGET_S}); fixed per run")
    ap.add_argument("--op-budget", type=float, default=OP_BUDGET_S,
                    help=f"seconds per operation above which fewer calls are timed (default {OP_BUDGET_S:.0f}); "
                         "fixed per run")
    ap.add_argument("--allow-unfixed-host", action="store_true",
                    help="run even if turbo/governor/SMT/counter settings are not fixed (records say so)")
    args = ap.parse_args()
    if args.rebuild and not args.only:
        ap.error("--rebuild needs --only")
    TRIAL_TARGET_S, OP_BUDGET_S = args.trial_target, args.op_budget
    problems = host_problems(args.cpu)
    if args.phase == "check":
        for p in problems:
            print("HOST:", p)
        print("host ready" if not problems else f"{len(problems)} problem(s)")
        return 1 if problems else 0
    if args.phase in ("publish", "evidence"):
        if not args.run_dir:
            ap.error(f"{args.phase} needs --run-dir")
        run_dir = args.run_dir if args.run_dir.is_absolute() else ROOT / args.run_dir
        return phase_publish(run_dir, args.defer) if args.phase == "publish" else phase_evidence(run_dir)
    if problems and args.phase in ("baseline", "calibrate", "measure", "profile", "hashcost", "all") and not args.allow_unfixed_host:
        for p in problems:
            print("HOST:", p, file=sys.stderr)
        print("refusing to measure on an unfixed host (use --allow-unfixed-host for tests)", file=sys.stderr)
        return 2
    if not args.run_dir:
        stamp = dt.datetime.now(dt.UTC).strftime("%Y%m%dT%H%M%SZ")
        args.run_dir = PERF / "runs" / f"{socket.gethostname()}-campaign-{stamp}"
    run = args.run_dir if args.run_dir.is_absolute() else ROOT / args.run_dir
    run.mkdir(parents=True, exist_ok=True)
    meta = load(run / "campaign.json", {})
    planning = {"trials": TRIALS, "trial_target_s": TRIAL_TARGET_S, "min_total": MIN_TOTAL,
                "max_iters_per_trial": MAX_ITERS, "op_budget_s": OP_BUDGET_S,
                "separate_process_setup_s": SEPARATE_PROCESS_SETUP_S}
    if args.phase != "build" and meta.get("planning") not in (None, planning) \
            and any((run / "records").rglob("*.json")):
        print(f"run directory was measured with planning {meta['planning']}, not {planning}; "
              "pass the same --trial-target/--op-budget", file=sys.stderr)
        return 2
    sid = system_id(args.system)
    if not sid and args.phase != "build":
        print("this machine is not in performance/systems.csv; add it (ID;Arch;Hostname;Description) "
              "or pass --system", file=sys.stderr)
        return 2
    if meta.get("system_id") and sid and meta["system_id"] != sid:
        print(f"run directory belongs to system {meta['system_id']}, not {sid}", file=sys.stderr)
        return 2
    measuring = args.phase in ("baseline", "calibrate", "measure", "profile", "hashcost", "all")
    env_start = None if "environment_at_start" in meta else environment(args.cpu)

    def start(meta):
        if sid:
            meta["system_id"] = sid
        meta.setdefault("started_utc", dt.datetime.now(dt.UTC).isoformat())
        if env_start:
            meta.setdefault("environment_at_start", env_start)
        meta.setdefault("host_checks", []).append({"phase": args.phase, "cpu": args.cpu,
                                                   "utc": dt.datetime.now(dt.UTC).isoformat(),
                                                   "problems": problems,
                                                   **({"shard": "%d/%d" % args.shard} if args.shard else {})})
        if measuring:
            meta["host_problems"] = sorted(set(meta.get("host_problems", [])) | set(problems))
        if args.shard:
            meta["shards"] = max(meta.get("shards", 1), args.shard[1])
            meta.setdefault("shard_cpus", {})["%d/%d" % args.shard] = args.cpu
        meta["arch_config"] = ARCH.get(platform.machine())
        meta["harness_additions"] = HARNESS_ADDITIONS
        meta["planning"] = planning
    update_campaign(run, start)
    SHARDS_BUILT[:] = list((load(run / "build.json") or {}).get("instances", {}))
    subprocess.run(["make", "-s", "-C", "performance"], cwd=ROOT, check=True)
    subprocess.run(["make", "-s", "-C", "api", "harness"], cwd=ROOT, check=True)
    shown = run.relative_to(ROOT) if run.is_relative_to(ROOT) else run
    log(run, f"phase {args.phase} in {shown} on CPU {args.cpu}"
             + (f" (HOST NOT FIXED: {'; '.join(problems)})" if problems else ""))
    if args.phase in ("baseline", "calibrate", "measure", "profile", "hashcost", "all"):
        # one measuring process per benchmark CPU at a time; later phases wait here
        import fcntl
        lock = open(PERF / "runs" / f".cpu{args.cpu}.lock", "w")
        fcntl.flock(lock, fcntl.LOCK_EX)
        args._lock = lock
        if args.phase in ("profile", "hashcost"):
            # one worker per phase (per shard), whatever the CPU; a second one waits, then resumes
            tag = f"-{args.shard[0]}of{args.shard[1]}" if args.shard and args.phase == "profile" else ""
            plock = open(PERF / "runs" / f".{args.phase}{tag}.lock", "w")
            fcntl.flock(plock, fcntl.LOCK_EX)
            args._plock = plock
        if args.deferred_only:
            dlock = open(PERF / "runs" / ".deferred.lock", "w")
            fcntl.flock(dlock, fcntl.LOCK_EX)
            args._dlock = dlock
    phases = {"build": phase_build, "baseline": phase_baseline, "calibrate": phase_calibrate, "measure": phase_measure,
              "profile": phase_profile, "hashcost": phase_hashcost}
    if args.phase == "plan":
        todo = all_plans(run, args)
        print(f"{len(todo)} operations, estimated {sum(t[0] for t in todo) / 3600:.1f} h")
        for est, e, op, size, p in todo[-15:]:
            print(f"  {est / 60:8.1f} min  {e['candidate']} {e['label']} {op_key(op, size)}  "
                  f"iters/trial={p['iterations_per_trial']} separate={p['separate_processes']}")
        return 0
    for name in (["build", "baseline", "calibrate", "measure", "profile", "hashcost"] if args.phase == "all" else [args.phase]):
        phases[name](run, args)
    env_end = environment(args.cpu)

    def end(meta):
        meta["ended_utc"] = dt.datetime.now(dt.UTC).isoformat()
        meta["environment_at_end"] = env_end
    update_campaign(run, end)
    log(run, f"phase {args.phase} finished")
    return 0


if __name__ == "__main__":
    sys.exit(main())
