#!/usr/bin/env python3
"""Render a campaign run directory into Markdown reports.

    python3 performance/report.py RUN_DIR [--system ID] [--out DIR]

The system ID (performance/systems.csv, e.g. x86_1) comes from the run's
campaign.json or its hostname; it keeps reports from different architectures
and machines apart. Writes, next to the other per-candidate reports:
  <id>/perf_<ID>.md      one report per candidate, following the guide's report
                         items (1)-(7), plus the share of each operation spent in
                         the ICCS placeholder hash functions and DRNG
and, in --out (default performance/):
  summary_<ID>.md        cycles per operation and ICCS hash share, all candidates
  method_<ID>.md         method, host settings and limitations
  symmetric-survey.md    which candidates use the ICCS helpers (source analysis,
                         shared by all systems), with this system's measured share

Numbers are taken only from records in RUN_DIR. The summary compares each
hash candidate with the ICCS `pseudoXOF` of the same output width at the same
message length, timed directly as candidate "iccs" (campaign.py baseline); a run
without those records falls back to the median profiled `pseudoXOF` call of that
shape, for 32-byte messages only. Neither estimates the cost
of substituting that hash into a public-key scheme; several hash submissions
are not yet constant-time, so their timings are not production figures. The
hash-cost data stays in RUN_DIR/hashcost/.
"""

from __future__ import annotations

import argparse
import csv
import sys
import json
import os
import re
import statistics
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CATS = [("sign", "Digital signatures"), ("kem", "Key encapsulation"), ("kex", "Key exchange"),
        ("hash", "Hash functions")]
ORDER = ["keygen", "enc", "dec", "sign", "verify", "exchange", "init_a", "init_b"] + \
    [f"pass{k}" for k in range(1, 17)] + ["derive_a", "derive_b"]


def op_order(key: str):
    base, _, size = key.partition("_") if key.startswith("hash") else (key, "", "")
    return (ORDER.index(base) if base in ORDER else len(ORDER), int(size or 0), key)


BASELINE = "iccs"        # the ICCS helpers timed as hash instances (performance/iccs)
HASH_SIZES = (32, 128, 512, 1024, 4096, 8192, 16384, 65536)   # the guide's S1-S8
# bandwidth columns of the summary: (size field, header); sizes are the library's
# own API constants, so they do not depend on the system
SIZE_COLS = {"kem": [("pk_bytes", "public key (B)"), ("ct_bytes", "ciphertext (B)")],
             "sign": [("pk_bytes", "public key (B)"), ("signature_bytes", "signature (B)")],
             "kex": [("total_msg_bytes", "transferred (B)")]}
OPS = {"kem": ["keygen", "enc", "dec"], "sign": ["keygen", "sign", "verify"], "kex": ["exchange"],
       "hash": ["hash_32", "hash_1024", "hash_65536"]}


def load(p: Path):
    return json.loads(p.read_text(encoding="utf-8")) if p.is_file() else None


def fmt_int(x):
    if x is None:
        return "–"
    if x >= 1e9:
        return f"{x / 1e9:.2f} G"
    if x >= 1e6:
        return f"{x / 1e6:.2f} M"
    if x >= 1e4:
        return f"{x / 1e3:.1f} k"
    return f"{x:.0f}"


def fmt_time(s):
    if s is None:
        return "–"
    for unit, f in (("s", 1), ("ms", 1e3), ("µs", 1e6), ("ns", 1e9)):
        if s * f >= 1:
            return f"{s * f:.3g} {unit}"
    return f"{s * 1e9:.3g} ns"


def pct(x):
    return "–" if x is None else f"{100 * x:.0f}%" if x >= 0.095 else f"{100 * x:.1f}%"


class Run:
    def __init__(self, run: Path):
        self.dir = run
        self.campaign = load(run / "campaign.json") or {}
        self.build = (load(run / "build.json") or {}).get("instances", {})
        self.records = defaultdict(dict)   # (cand, label) -> op_key -> record
        for f in sorted((run / "records").rglob("*.json")):
            r = load(f)
            key = r["operation"] + (f"_{r['input_bytes']}" if r.get("input_bytes") else "")
            r["_path"] = f.relative_to(run).as_posix()
            self.records[(r["candidate"], r["label"])][key] = r
        # the measuring host as recorded in the timing records (campaign.json's
        # start environment may predate the fixed host settings, e.g. the build phase)
        envs = [r["environment"] for recs in self.records.values() for r in recs.values()
                if r.get("status") == "complete" and r.get("environment")]
        main = [e for e in envs if str(e.get("cpu_number")) == "2"] or envs
        self.env = main[0] if main else self.campaign.get("environment_at_start", {})
        self.cpus = sorted({e.get("cpu_number") for e in envs})
        self.profiles = defaultdict(dict)
        for f in sorted((run / "profile").rglob("*__*.json")):
            r = load(f)
            if "operation" not in r:
                continue
            key = r["operation"] + (f"_{r['input_bytes']}" if r.get("input_bytes") else "")
            r["_path"] = f.relative_to(run).as_posix()
            self.profiles[(r["candidate"], r["label"])][key] = r
        self.names, self.pages = {}, {}
        with (ROOT / "downloads.csv").open(encoding="utf-8") as f:
            for row in csv.DictReader(f, delimiter=";"):
                self.names[row["ID"]] = row["Algorithm"]
                self.pages[row["ID"]] = row.get("PageURL", "")
        self.survey = {}
        with (ROOT / "performance/symmetric_survey.csv").open(encoding="utf-8") as f:
            for row in csv.DictReader((l for l in f if not l.startswith("#")), delimiter=";"):
                self.survey[row["ID"]] = row
        self.kat_issues = []
        with (ROOT / "performance/kat_issues.csv").open(encoding="utf-8") as f:
            self.kat_issues = list(csv.DictReader((l for l in f if not l.startswith("#")), delimiter=";"))
        for row in self.kat_issues:
            # Cause is the last column and may itself contain ";"
            if None in row:
                row["Cause"] = ";".join([row["Cause"], *row.pop(None)])
        self.summary_notes = []
        with (ROOT / "performance/summary_notes.csv").open(encoding="utf-8") as f:
            self.summary_notes = list(csv.DictReader((l for l in f if not l.startswith("#")), delimiter=";"))
        for row in self.summary_notes:
            if None in row or row.get("ID") not in self.names:
                raise ValueError(f"invalid performance/summary_notes.csv row: {row}")
            re.compile(row.get("Label", ""))
        # The 32-byte hash comparison is derived entirely from the published
        # profiles: use the median per-call cost of pseudoXOF with the same
        # input and output widths.  Each aggregate profile contributes one
        # observation, so no single high-iteration operation dominates it.
        costs = defaultdict(list)
        core_hz = 1000 * float(self.env.get("cpufreq_max_khz") or 0)
        for profiles in self.profiles.values():
            for profile in profiles.values():
                tick_hz = float(profile.get("meta", {}).get("tick_hz") or 0)
                to_core_cycles = core_hz / tick_hz if core_hz and tick_hz else 1.0
                for call in profile.get("calls", []):
                    if call.get("fn") == "pseudoXOF" and call.get("calls"):
                        costs[(call.get("in_bits"), call.get("out_bits"))].append(
                            call["ticks"] / call["calls"] * to_core_cycles)
        self.pseudoxof_cost = {shape: statistics.median(values) for shape, values in costs.items()}
        # the ICCS helpers timed directly at every hash message length
        self.baseline = {l: recs for (c, l), recs in self.records.items() if c == BASELINE}
        self.params = defaultdict(list)
        with (ROOT / "data/parameters.csv").open(encoding="utf-8") as f:
            for row in csv.DictReader(f, delimiter=";"):
                self.params[row["ID"]].append(row)


OUT = ROOT / "performance"
SYSTEM = "x86_1"          # set in main() from --system / the run
SYSTEM_DESC = ""


def load_systems() -> dict:
    with (ROOT / "performance/systems.csv").open(encoding="utf-8") as f:
        return {r["ID"]: r for r in csv.DictReader((l for l in f if not l.startswith("#")), delimiter=";")}


CHECK = False            # --check: compare with the files on disk instead of writing
DIFFERENT: list[str] = []


def emit(path: Path, text: str) -> None:
    if CHECK:
        if not path.is_file() or path.read_text(encoding="utf-8") != text:
            DIFFERENT.append(path.relative_to(ROOT).as_posix() if path.is_relative_to(ROOT) else str(path))
        return
    path.write_text(text, encoding="utf-8")


def evidence_dir(run) -> str:
    return run.dir.relative_to(ROOT).as_posix() if run.dir.is_relative_to(ROOT) else str(run.dir)


def record_host_state(run) -> tuple[int, dict]:
    """(complete timing records, {deviation: count}) from each record's stored environment."""
    import collections
    n, issues = 0, collections.Counter()
    for recs in run.records.values():
        for r in recs.values():
            if r.get("status") != "complete":
                continue
            n += 1
            e = r.get("environment") or {}
            if e.get("intel_pstate_no_turbo") == "0" or e.get("cpufreq_boost") == "1":
                issues["turbo enabled"] += 1
            if e.get("cpufreq_governor") not in (None, "performance"):
                issues[f"governor {e.get('cpufreq_governor')}"] += 1
            if e.get("smt_control") not in (None, "off", "forceoff", "notsupported", "notimplemented"):
                issues[f"SMT {e.get('smt_control')}"] += 1
            if not r.get("mean_cycles"):
                issues["no hardware cycle count"] += 1
            if run.env.get("core_clock_mhz") and not e.get("cpufreq_governor") \
                    and e.get("core_clock_mhz") != run.env.get("core_clock_mhz"):
                issues[f"clock {e.get('core_clock_mhz')} MHz"] += 1
    return n, dict(issues)


def cand_page(cand: str) -> Path:
    return ROOT / cand / f"perf_{SYSTEM}.md"


def summary_page(out: Path) -> Path:
    return out / f"summary_{SYSTEM}.md"


def method_page_path(out: Path) -> Path:
    return out / f"method_{SYSTEM}.md"


def link(frm: Path, to: Path) -> str:
    """Relative Markdown link target from the page at frm to the page at to."""
    return os.path.relpath(to, frm.parent).replace(os.sep, "/")


def value(rec, unit="cycles"):
    if not rec or rec.get("status") != "complete":
        return None
    return rec.get("mean_cycles") if unit == "cycles" else rec.get("mean_seconds")


def smt_text(env: dict) -> str:
    return {"notsupported": "none", "notimplemented": "none"}.get(env.get("smt_control"), env.get("smt_control"))


def clock_text(env: dict) -> str:
    if not env.get("cpufreq_driver") and env.get("core_clock_hz"):
        # no cpufreq: the clock is set by firmware; campaign.py checked it was steady
        return (f"measured {int(env.get('core_clock_mhz') or 0) / 1000:.2f} GHz, set by firmware (no OS frequency "
                f"scaling or boost), SMT {smt_text(env)}")
    turbo = {"1": "off", "0": "on"}.get(env.get("intel_pstate_no_turbo") or "",
                                         {"0": "off", "1": "on"}.get(env.get("cpufreq_boost") or "", "unknown"))
    mhz = int(env.get("cpufreq_max_khz") or 0) / 1e6
    if env.get("architecture") == "aarch64" and env.get("cpufreq_min_khz") == env.get("cpufreq_max_khz"):
        return (f"fixed {mhz:.2f} GHz (governor {env.get('cpufreq_governor')}, minimum = maximum), "
                f"boost {turbo}, SMT {smt_text(env)}")
    return f"max {mhz:.2f} GHz, governor {env.get('cpufreq_governor')}, turbo {turbo}, SMT {env.get('smt_control')}"


def cell(rec):
    if not rec:
        return "–"
    if rec.get("status") != "complete":
        return "failed"
    c = rec.get("mean_cycles")
    s = fmt_int(c) if c else fmt_time(rec.get("mean_seconds"))
    return s + ("" if rec.get("guide_100_measurements_met") else f" (n={rec.get('measurements')})")


def pk_cell(rec, share):
    """Cycles with the symmetric share and any short count in one parenthesis:
    `3.27 G (99%)`, `48.20 G (99%, n=35)`."""
    text = cell(rec)
    if not rec or rec.get("status") != "complete":
        return text
    notes = [share] if share else []
    if not rec.get("guide_100_measurements_met"):
        text = text.rsplit(" (n=", 1)[0]
        notes.append(f"n={rec.get('measurements')}")
    return f"{text} ({', '.join(notes)})" if notes else text


def size_text(n: int) -> str:
    return f"{n // 1024} KiB" if n >= 1024 and n % 1024 == 0 else f"{n} B"


def xof_cycles(run: Run, input_bytes: int, out_bits: int):
    """Cycles of pseudoXOF with this message length and output width: the direct
    baseline measurement or, in a run without one, the median profiled call of
    that shape (32-byte messages only)."""
    if run.baseline:
        return value(run.baseline.get(f"pseudoXOF-{out_bits}", {}).get(f"hash_{input_bytes}"))
    return run.pseudoxof_cost.get((8 * input_bytes, out_bits)) if input_bytes == 32 else None


def hash_cell(run: Run, rec, input_bytes: int):
    """Cycle count, plus the ratio to pseudoXOF of the same shape."""
    rendered = cell(rec)
    if not rec or rec.get("status") != "complete" or (input_bytes != 32 and not run.baseline):
        return rendered
    out_bits = 8 * int(rec.get("sizes", {}).get("digest_bytes", 0))
    baseline = xof_cycles(run, input_bytes, out_bits)
    cycles = rec.get("mean_cycles")
    if not baseline or not cycles:
        return f"{rendered} (ratio –)"
    return f"{rendered} ({cycles / baseline:.2f}×)"


def compact_notes(run: Run, cand: str, label: str, entry: dict) -> str:
    notes = []
    verdict = run.survey.get(cand, {}).get("Verdict", "")
    if verdict == "bypass":
        notes.append("own symmetric primitives; sym % excludes them")
    elif verdict == "mixed":
        notes.append("mixed own/ICCS primitives")
    elif verdict == "instance-dependent":
        notes.append("hash backend varies by instance")
    status = entry.get("kat")
    if status and status != "PASS":
        notes.append("KAT/output caveat; see candidate page")
    for row in run.summary_notes:
        if row["ID"] == cand and re.search(row["Label"], label):
            notes.append(row["Note"])
    return "; ".join(dict.fromkeys(notes)) or "–"


def omit_non_iccs_zero_row(run: Run, cand: str, label: str, cat: str) -> bool:
    """Hide non-ICCS variants that never enter a placeholder hash helper.

    ICCS-only implementations are retained even when the hash share is zero,
    because some use the separately-accounted ICCS DRNG as their XOF.  The
    candidate detail pages always retain every measured implementation.
    """
    if cat == "hash" or run.survey.get(cand, {}).get("Verdict") == "ICCS-only":
        return False
    profiles = run.profiles.get((cand, label), {})
    shares = [profiles[op].get("hash_share") for op in OPS[cat]
              if op in profiles and profiles[op].get("hash_share") is not None]
    return bool(shares) and all(share == 0 for share in shares)


def baseline_table(run: Run) -> list[str]:
    """The ICCS helpers at every hash message length (records/iccs/)."""
    def order(label):
        fn, bits = label.rsplit("-", 1)
        return (["sm3hash", "pseudohash", "pseudoXOF"].index(fn), int(bits))
    rows = []
    for label in sorted(run.baseline, key=order):
        recs = run.baseline[label]
        fn, bits = label.rsplit("-", 1)
        big = recs.get(f"hash_{HASH_SIZES[-1]}")
        cpb = value(big) / HASH_SIZES[-1] if value(big) else None
        status = run.build.get(f"{BASELINE}/{label}", {}).get("kat")
        mark = "" if status == "PASS" else f" ⚠ self-test {status}"
        rows.append(f"| `{fn}`{mark} | {bits} | " + " | ".join(cell(recs.get(f"hash_{n}")) for n in HASH_SIZES)
                    + f" | {'–' if cpb is None else f'{cpb:.1f}'} |")
    return ["| ICCS helper | output bits | " + " | ".join(size_text(n) for n in HASH_SIZES)
            + f" | cycles/byte ({size_text(HASH_SIZES[-1])}) |",
            "|---|---|" + "---|" * (len(HASH_SIZES) + 1)] + rows


def instances_of(run: Run, cand: str):
    labels = sorted({l for (c, l) in run.records if c == cand} |
                    {e["label"] for e in run.build.values() if e["candidate"] == cand})
    def natural(label):
        return [int(t) if t.isdigit() else t for t in re.split(r"(\d+)", label)]
    return sorted(labels, key=natural)


def summary(run: Run, out: Path, arch: str):
    env = run.env
    lines = [f"# Performance — {arch}, system {SYSTEM}", "",
             "Independent measurements of the NGCC Round 1 implementations on one "
             f"{env.get('cpu_model', 'x86-64')} core ({clock_text(env)}).", "",
             "- **Cycles** are the mean of all timed calls in five trials, normally with each trial in a fresh process.",
             "- **Symmetric %**, in parentheses after each public-key cycle count, is the measured share of an operation spent specifically in the ICCS placeholder "
             "functions (`pseudohash`, `pseudoXOF`, `sm3hash`). It excludes the ICCS DRNG and candidates' own "
             "hash primitives, so a low value does not necessarily mean little symmetric-cryptography work; see "
             "the [symmetric cryptography survey](symmetric-survey.md).",
             "- Each instance links to its **performance report** with KAT status, all measured implementations, "
             "sizes, memory proxies, primitive profiles, and raw-evidence references.",
             f"- See [method and limitations]({method_page_path(out).name}). These are independent measurements, "
             "not submitter self-assessments or NICCS results.",
             "- **Notation:** `–` means not measured, `n=` marks fewer than 100 timed calls, and ⚠ marks an instance "
             "whose submitted KAT vectors are not reproduced by the submitted code.",
             "- **Sizes** in bytes (public key, ciphertext, signature; for key exchange, the total transferred "
             "in all protocol messages) are the implementation's own API constants and do not depend on the system.",
             ("- **Hash rows** give three message sizes; each parenthesized value is the candidate's cycles divided "
              "by those of the ICCS `pseudoXOF` with the same output width and message length, timed the same way "
              "([ICCS helpers](#iccs-hash-helpers)). It is a relative speed, not an estimate of a production "
              "replacement." if run.baseline else
              "- **Hash rows** give three message sizes; the parenthesized 32-byte value is relative to an "
              "exact-shape measured `pseudoXOF` call, not an estimate of a production replacement."),
             "- **Scope:** the table keeps ICCS-facing reference parameter sets. Notes flag important caveats; "
             "additional measured variants remain on the linked instance reports.", ""]
    for cat, title in CATS:
        rows = []
        for cand in sorted({c for (c, l) in run.records if c.startswith(cat)} |
                           {e["candidate"] for e in run.build.values() if e["candidate"].startswith(cat)}):
            for label in instances_of(run, cand):
                recs = run.records.get((cand, label), {})
                entry = run.build.get(f"{cand}/{label}", {})
                if entry.get("variant", "reference") != "reference":
                    continue
                if omit_non_iccs_zero_row(run, cand, label, cat):
                    continue
                cells = []
                for op in OPS[cat]:
                    if cat == "hash":
                        cells.append(hash_cell(run, recs.get(op), int(op.split("_")[1])))
                    else:
                        p = run.profiles.get((cand, label), {}).get(op)
                        share = pct(p["hash_share"]) if p and "hash_share" in p else None
                        cells.append(pk_cell(recs.get(op), share))
                sizes = next((r["sizes"] for r in recs.values() if r.get("sizes")), {})
                cells += [str(sizes[k]) if sizes.get(k) else "–" for k, _ in SIZE_COLS.get(cat, [])]
                variant = " (AVX2)" if label.endswith("-avx2") else ""
                status = entry.get("kat")
                if status and status != "PASS":
                    variant += f" ⚠ KAT {status}"
                spec = f"https://github.com/ngcc-dev/ngcc-harness/blob/main/{cand}/{cand}-spec.pdf"
                algorithm = f"{run.names.get(cand, '')} [PDF]({spec})"
                notes = compact_notes(run, cand, label, entry)
                rows.append(f"| [{cand}]({link(summary_page(out), cand_page(cand))}) | {algorithm} | "
                            f"`{label}`{variant} | " + " | ".join(cells) + f" | {notes} |")
        if not rows:
            continue
        if cat == "hash" and run.baseline:
            lines += ["## ICCS hash helpers", "",
                      "The ICCS placeholder functions from `api/auxfunc.c`, built with the reference flags and "
                      "timed as hash instances at the guide's S1–S8 message lengths (mean cycles per call). "
                      "`pseudohash` exists only with 512-, 768- and 1024-bit output; `pseudoXOF` is given at every "
                      "digest width of a hash candidate.", ""] + baseline_table(run) + [""]
        lines += [f"## {title}", ""]
        if cat == "hash" and run.baseline:
            lines += ["| id | algorithm | instance performance report | 32 B cycles (× pseudoXOF) | "
                      "1 KiB cycles (× pseudoXOF) | 64 KiB cycles (× pseudoXOF) | notes |",
                      "|---|---|---|---|---|---|---|"]
        elif cat == "hash":
            lines += ["| id | algorithm | instance performance report | 32 B cycles (vs pseudoXOF) | 1 KiB cycles | 64 KiB cycles | notes |",
                      "|---|---|---|---|---|---|---|"]
        else:
            hdr = " | ".join([f"{op} cycles (sym %)" for op in OPS[cat]] + [h for _, h in SIZE_COLS[cat]])
            lines += [f"| id | algorithm | instance performance report | {hdr} | notes |",
                      "|---|---|---|" + "---|" * (len(OPS[cat]) + len(SIZE_COLS[cat]) + 1)]
        lines += rows + [""]
    emit(summary_page(out), "\n".join(lines) + "\n")


def guide_name(arch: str) -> str:
    return {"x86-64": "x86", "AArch64": "ARM"}.get(arch, arch)


def candidate_page(run: Run, cand: str, out: Path, arch: str):
    cat = cand.split("-")[0]
    name = run.names.get(cand, "")
    env = run.env
    labels = instances_of(run, cand)
    L = [f"# {cand} {name} — performance on {arch} (system {SYSTEM})", "",
         f"[Performance {SYSTEM}]({link(cand_page(cand), summary_page(out))}) › `{cand}` · "
         f"[method]({link(cand_page(cand), method_page_path(out))}) · [NICCS page]({run.pages.get(cand, '')})", "",
         f"Independent measurement following the structure of the NICCS {guide_name(arch)} self-assessment "
         "guide, §3.5 (1)–(7). Not a submitter self-assessment.", ""]
    # (1)
    fn = {"kem": "key encapsulation", "sign": "digital signature", "kex": "key exchange", "hash": "hash"}[cat]
    L += ["## 1. Basic information", "",
          f"- Category: {'public-key' if cat != 'hash' else 'cryptographic hash'} algorithm; function: {fn}",
          f"- Algorithm: {name}",
          "- Implementation versions measured: " + ", ".join(sorted({
              "optimized (AVX2)" if l.endswith("-avx2") else "reference" for l in labels})),
          "- Parameter sets: " + ", ".join(f"`{l}`" for l in labels), ""]
    # (2)
    L += ["## 2. Assessment environment", "",
          "| item | value |", "|---|---|",
          f"| processor | {env.get('cpu_model')} (CPU {env.get('cpu_number')}, one core) |",
          *([f"| machine | {env['machine']} |"] if env.get("machine") else []),
          f"| clock | {clock_text(env)} |",
          f"| memory | {int(env.get('memory_total_kib') or 0) // 1024} MiB |",
          f"| OS / kernel | {env.get('os')} / {env.get('kernel')} |",
          f"| compiler / build tool | {env.get('compiler')} / {env.get('cmake')} |",
          f"| campaign start / end (UTC) | {run.campaign.get('started_utc', '')[:19]} / {run.campaign.get('ended_utc', '')[:19]} |", ""]
    # (3)
    L += ["## 3. Functional testing (KAT)", "",
          "Each library was checked against the submitted KAT vectors (SHA-256 manifest "
          f"`{cand}/kat.sha256` in the harness) before timing.", "",
          "| instance | build flags | KAT |", "|---|---|---|"]
    notes = []
    for l in labels:
        e = run.build.get(f"{cand}/{l}", {})
        # the issue list is shared by all systems: a KAT timeout on a slower machine
        # is no issue where the same KATs pass within the limit
        issue = next((i for i in run.kat_issues if i["ID"] == cand and re.search(i["Label"], l)
                      and not (e.get("kat") == "PASS" and i["Cause"].startswith("KAT TIMEOUT"))), None)
        mark = ""
        if issue:
            if issue not in notes:
                notes.append(issue)
            mark = f" [{notes.index(issue) + 1}]"
            if issue["Action"] == "exclude":
                mark += " (not timed)"
        L.append(f"| `{l}` | {e.get('flags', '–')} | {e.get('kat', '–')}{mark} |")
    L.append("")
    for n, issue in enumerate(notes, 1):
        L.append(f"[{n}] {issue['Cause']}" + (" These instances are timed anyway; their output is not validated."
                                              if issue["Action"] == "measure" else ""))
        L.append("")
    # (4)
    L += ["## 4. Performance", ""]
    if cat == "hash":
        L += ["| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |", "|---|---|---|---|---|---|---|"]
    else:
        L += ["| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |",
              "|---|---|---|---|---|---|---|"]
    for l in labels:
        for key, r in sorted(run.records.get((cand, l), {}).items(), key=lambda kv: op_order(kv[0])):
            if r.get("status") != "complete":
                L.append(f"| `{l}` | {key} | failed | | | | |")
                continue
            n = f"{r['measurements']} ({r['plan']['trials']} × {r['plan']['iterations_per_trial']})"
            if cat == "hash":
                c = r.get("mean_cycles")
                L.append(f"| `{l}` | {r['input_bytes']} B | {fmt_int(c)} | {c / r['input_bytes']:.1f} | "
                         f"{fmt_time(r['mean_seconds'])} | {r.get('throughput_MB_per_second') or 0:.1f} | {n} |"
                         if c else f"| `{l}` | {r['input_bytes']} B | – | – | {fmt_time(r['mean_seconds'])} | "
                         f"{r.get('throughput_MB_per_second') or 0:.1f} | {n} |")
            else:
                retries = sum(int(pr.get("meta", {}).get("sign_failures_retried", 0) or 0)
                              for pr in r.get("processes", []))
                if retries:
                    n += f"; {retries} failed signing attempts retried"
                L.append(f"| `{l}` | {key} | {fmt_int(r.get('mean_cycles'))} | {fmt_time(r['mean_seconds'])} | "
                         f"{r['operations_per_second']:.3g} | {fmt_time(r['median_trial_seconds'])} | {n} |")
    L.append("")
    # (5)
    L += ["## 5. Resource consumption", "",
          "Static memory is approximated by the library's loadable ELF segments; peak memory is "
          "the process high-water mark (VmHWM) including the benchmark driver and libc, so both are "
          "upper-bound proxies rather than isolated algorithm memory.", "",
          "| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |", "|---|---|---|---|---|"]
    for l in labels:
        elf = run.build.get(f"{cand}/{l}", {}).get("elf_load_bytes")
        for key, r in sorted(run.records.get((cand, l), {}).items(), key=lambda kv: op_order(kv[0])):
            if r.get("peak_rss_bytes"):
                L.append(f"| `{l}` | {key} | {elf if elf is not None else '–'} | "
                         f"{r['baseline_rss_bytes'] // 1024} KiB | {r['peak_rss_bytes'] // 1024} KiB |")
    L.append("")
    # (6)
    if cat != "hash":
        L += ["## 6. Transmission and storage overhead", ""]
        if cat == "kex":
            L += ["| instance | passes | messages (bytes) | total | long-term pk / sk | shared secret |",
                  "|---|---|---|---|---|---|"]
        else:
            L += ["| instance | public key | secret key | " + ("ciphertext | shared secret |" if cat == "kem" else "signature |"),
                  "|---|---|---|---|" + ("---|" if cat == "kem" else "")]
        for l in labels:
            recs = run.records.get((cand, l), {})
            r = next((x for x in recs.values() if x.get("sizes")), None)
            if not r:
                continue
            s = r["sizes"]
            if cat == "kem":
                L.append(f"| `{l}` | {s.get('pk_bytes')} | {s.get('sk_bytes')} | {s.get('ct_bytes')} | {s.get('ss_bytes')} |")
            elif cat == "sign":
                L.append(f"| `{l}` | {s.get('pk_bytes')} | {s.get('sk_bytes')} | {s.get('signature_bytes')} |")
            else:
                msgs = [s[k] for k in sorted((k for k in s if re.match(r"msg\d+_bytes", k)), key=lambda k: int(k[3:-6]))]
                L.append(f"| `{l}` | {r.get('kex', {}).get('kex_passes')} | {' / '.join(map(str, msgs))} | "
                         f"{s.get('total_msg_bytes')} | {s.get('pk_bytes')} / {s.get('sk_bytes')} | {s.get('ss_actual_bytes', s.get('ss_bytes'))} |")
        L.append("")
    # share of the ICCS placeholder functions
    if cat != "hash":
        sv = run.survey.get(cand, {})
        L += ["## Symmetric primitives", "",
              f"[Survey]({link(cand_page(cand), out / 'symmetric-survey.md')}) verdict: "
              f"**{sv.get('Verdict', 'not surveyed')}**" + (f" — {sv['Notes']}" if sv.get("Notes") else ""), "",
              "Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG "
              "(reference build, measured in the same run with link-time wrappers):", "",
              "| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |", "|---|---|---|---|---|"]
        for l in labels:
            for key, p in sorted(run.profiles.get((cand, l), {}).items(), key=lambda kv: op_order(kv[0])):
                if p.get("status") != "complete":
                    continue
                ops = p["total"]["operations"]
                calls = defaultdict(float)
                for c in p["calls"]:
                    calls[c["fn"]] += c["calls"] / ops
                cstr = ", ".join(f"{fn} {v:.3g}" for fn, v in sorted(calls.items()))
                L.append(f"| `{l}` | {key} | {pct(p.get('hash_share'))} | {pct(p.get('share', {}).get('drng', 0))} | {cstr or '–'} |")
        L.append("")
    # (7)
    L += ["## 7. Raw evidence index", "",
          f"Paths are relative to `{evidence_dir(run)}/` in the "
          "[harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, "
          "environment and trials.", "",
          "| instance | item | file |", "|---|---|---|"]
    for l in labels:
        e = run.build.get(f"{cand}/{l}", {})
        if e.get("kat_log"):
            L.append(f"| `{l}` | KAT log (sha256 `{e['kat_log_sha256'][:16]}…`) | `{e['kat_log']}` |")
        for key, r in sorted(run.records.get((cand, l), {}).items()):
            L.append(f"| `{l}` | timing {key} | `{r['_path']}` |")
        for key, p in sorted(run.profiles.get((cand, l), {}).items()):
            L.append(f"| `{l}` | hash profile {key} | `{p['_path']}` |")
    L += ["", "Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` "
          "in the [harness](https://github.com/ngcc-dev/ngcc-harness).", ""]
    emit(cand_page(cand), "\n".join(L) + "\n")


def survey_page(run: Run, out: Path):
    counts = defaultdict(int)
    for r in run.survey.values():
        counts[r["Verdict"]] += 1
    L = ["# Symmetric cryptography survey", "",
         "The NGCC public-key submissions were asked to use the ICCS placeholder functions "
         "`pseudohash` (SM3/HMAC-SM3), `pseudoXOF` (KDF-SM3) and `sm3hash` for hashing, and the "
         "ICCS SM3 DRNG for randomness, so that the selected NGCC hash can later be substituted. "
         "This survey records which reference implementations do so, which bypass the helpers with "
         "their own primitives, and how much of each operation the helpers take.", "",
         "Totals: " + ", ".join(f"{v} {k}" for k, v in sorted(counts.items(), key=lambda kv: -kv[1])) + ".", "",
         "Evidence: call-graph reachability from the exported API in the harness-built libraries, "
         "object-level constant scans (Keccak, SHA-2, SM3, AES, SM4, ChaCha), and source reading for "
         "candidates without a harness build. A primitive that is compiled but unreachable is not "
         "counted.", "",
         f"The last column is measured on system {SYSTEM} ({SYSTEM_DESC}); see its "
         f"[summary]({summary_page(out).name}).", "",
         f"| id | algorithm | verdict | notes | largest hash share on {SYSTEM} (op) |", "|---|---|---|---|---|"]
    for cand in sorted(run.survey, key=lambda c: (c.split("-")[0], c)):
        best = None
        for (c, l), profs in run.profiles.items():
            if c != cand:
                continue
            for key, p in profs.items():
                if "hash_share" in p and (best is None or p["hash_share"] > best[0]):
                    best = (p["hash_share"], f"{l} {key}")
        r = run.survey[cand]
        ref = f"[{cand}]({link(out / 'symmetric-survey.md', cand_page(cand))})" if cand_page(cand).is_file() else cand
        L.append(f"| {ref} | {run.names.get(cand, '')} | {r['Verdict']} | {r['Notes']} | "
                 f"{pct(best[0]) + ' (' + best[1] + ')' if best else '–'} |")
    emit(out / "symmetric-survey.md", "\n".join(L) + "\n")


def host_state_text(run) -> str:
    n, issues = record_host_state(run)
    if not issues and not run.env.get("cpufreq_driver") and run.env.get("core_clock_hz"):
        return (f"All {n} timing records were taken in this state (no cpufreq driver; the firmware-set "
                f"clock, measured on the timing core with the cycle counter before each phase, was steady at "
                f"{int(run.env['core_clock_mhz']) / 1000:.2f} GHz; SMT {smt_text(run.env)}; hardware cycle "
                "counter available), as stored in each record. A trial that ran below 95% of that clock "
                "was repeated, up to twice.")
    if not issues and run.env.get("architecture") == "aarch64" and run.env.get("cpufreq_driver"):
        return (f"All {n} timing records were taken in this state (clock pinned at "
                f"{int(run.env.get('cpufreq_max_khz') or 0) / 1e6:.2f} GHz on the clusters of the timing cores: `performance` "
                "governor with minimum = maximum frequency, below the level at which the firmware's thermal "
                "limiter intervenes; boost off; no SMT; hardware cycle counter available), as stored in each "
                "record. A trial that ran below 95% of that clock was repeated, up to twice.")
    if not issues:
        return (f"All {n} timing records were taken in this state (turbo off, `performance` governor, "
                "SMT off, hardware cycle counter available), as stored in each record.")
    return (f"Of {n} timing records, some deviate from the fixed host state: "
            + "; ".join(f"{k} ({v})" for k, v in sorted(issues.items())) + ".")


def secondary_core_text(run) -> str:
    main = run.env.get("cpu_number")
    other = sorted({(c, l) for (c, l), recs in run.records.items() for r in recs.values()
                    if r.get("status") == "complete" and (r.get("environment") or {}).get("cpu_number") != main})
    cores = ", ".join(str(c) for c in run.cpus)
    if run.campaign.get("shards", 1) > 1 and len(run.cpus) > 1:
        shard_cpus = set((run.campaign.get("shard_cpus") or {}).values())
        slow = sorted({(c, l) for (c, l), recs in run.records.items() for r in recs.values()
                       if r.get("status") == "complete" and shard_cpus
                       and (r.get("environment") or {}).get("cpu_number") not in shard_cpus})
        return (f"- Timing ran in parallel on cores {cores}. The instances were divided among cores "
                f"{', '.join(str(c) for c in sorted(shard_cpus)) or cores} by expected run time"
                + (f"; the slowest instances ({', '.join(f'{c} {l}' for c, l in slow)}) ran on a further core"
                   if slow else "")
                + ". All operations of one instance ran on the same core, and each record states its CPU. "
                "Cycle counts are comparable across these identical cores.")
    if not other:
        return f"- All timing ran on CPU {main}."
    names = ", ".join(f"{c} {l}" for c, l in other)
    kind = "performance " if run.env.get("architecture") == "x86_64" else ""
    return (f"- Timing ran on {kind}core(s) {cores}. The slowest instances ({names}) were timed on a "
            f"second {kind}core in parallel with the main run on CPU {main}, as was the hash "
            "profiling; each record states its CPU. Cycle counts are comparable across these identical cores.")


def tick_text(run) -> str:
    """The profile tick source, where it is coarser than the core clock."""
    metas = [p.get("meta", {}) for profs in run.profiles.values() for p in profs.values()]
    src = {m.get("tick_source") for m in metas} - {None, "rdtsc"}
    hz = [float(m["tick_hz"]) for m in metas if m.get("tick_hz")]
    if not src or not hz:
        return ""
    return (f" On this system the tick counter is `{'`, `'.join(sorted(src))}` at "
            f"{statistics.median(hz) / 1e6:.1f} MHz, so a single short call is resolved only to about "
            f"{1e9 / statistics.median(hz):.0f} ns; the rounding is unbiased and averages out over the many "
            "calls that make up a share.")


def method_page(run: Run, out: Path, arch: str):
    c = run.campaign
    pl = c.get("planning", {})
    cfg = c.get("arch_config") or {}
    env = run.env
    L = [f"# Performance method and limitations — system {SYSTEM}", "",
         f"System {SYSTEM}: {SYSTEM_DESC}. [Summary]({summary_page(out).name}).", "",
         "## Measurement", "",
         f"- One {env.get('cpu_model')} core (CPU {env.get('cpu_number')}; {clock_text(env)}). "
         "Cycles come from the hardware counter (`perf_event_open`, user mode); time from "
         "`CLOCK_MONOTONIC_RAW`. " + host_state_text(run),
         f"- Reference builds use the guide's flags `{cfg.get('reference')}` plus "
         f"`{c.get('harness_additions')}` for shared libraries and pre-C99 declarations; an instance "
         "that fails to build or to pass its KATs that way is rebuilt with the harness defaults, and "
         "its page says so. Optimized builds use the guide's performance flags.",
         "- Every library is checked against the submitted KAT vectors before timing. Instances "
         "whose vectors are not reproduced by the submitted code are still timed and are marked ⚠, "
         "with the identified cause on their candidate page (`performance/kat_issues.csv`); an "
         "instance without a reference source of its own is not timed. The DRNG is seeded with "
         "bytes 00..2f; signatures use a 64-byte message; hash inputs are the guide's S1–S8 lengths.",
         f"- Each operation is calibrated with one call, then measured in {pl.get('trials', 5)} trials. "
         f"Trials normally run in separate processes (fresh address-space layout); if setup takes "
         f"more than {pl.get('separate_process_setup_s', 60):.0f} s, the trials share one process. "
         f"Each trial runs a fixed number of calls, targeting {pl.get('trial_target_s', 1)} s and "
         f"at least {pl.get('min_total', 100)} calls in total; if that would exceed "
         f"{pl.get('op_budget_s', 1800) / 60:.0f} minutes, fewer calls are used, but never fewer than "
         "one per trial. No operation is cut short by a time limit.",
         "- Reported cycles are the arithmetic mean over all timed calls (the guide's metric); the "
         "median of the trial means is also recorded as a robustness check.",
         "- Key exchange: `exchange` covers both initialisations, every pass and both key "
         "derivations of one protocol run (no network time); single steps are timed call by call.", "",
         "## Share of the ICCS placeholder functions", "",
         "Each reference library is relinked with link-time wrappers (`-Wl,--wrap`) around "
         "`pseudohash`, `pseudoXOF`, `sm3hash` and the DRNG's `get_random_number`. Every call that "
         "crosses an object-file boundary is timed with the CPU tick counter and recorded with its "
         "input and output length; nested calls are not counted twice. The reported share is the "
         "time inside these functions divided by the time of the whole operation, measured in the "
         "same process on the same inputs as the benchmark. The wrappers cost a few tens of cycles "
         "per call." + tick_text(run), "",
         (f"The ICCS helpers `sm3hash`, `pseudohash` and `pseudoXOF` are also timed directly, as hash "
          f"instances of `api/auxfunc.c` (`performance/iccs`, records under `{BASELINE}/`): same reference "
          "flags, driver, message lengths and planning as the hash candidates. Before timing, each is checked "
          "against an independent model on OpenSSL's SM3, and `sm3hash` against the GB/T 32905 examples. "
          "The summary divides each hash candidate's cycles by those of `pseudoXOF` with the same output "
          "width and message length. "
          if run.baseline else
          "The summary compares each hash candidate's 32-byte timing with the median measured "
          "`pseudoXOF` call having exactly the same input and output widths in the published profiles. "
          "Profile TSC ticks are converted to fixed-frequency core cycles using each profile's recorded "
          "tick calibration and the campaign's fixed CPU frequency. ") +
         "This is a direct, self-contained relative measurement, not an estimate of substituting "
         "that hash into a public-key scheme. Several hash submissions are not yet constant-time "
         "(e.g. table-based S-boxes), so their current timings are not production figures; the "
         "summary flags known cases.", "",
         "## Limitations", "",
         "- Static and peak memory are process-level proxies (ELF image, VmHWM), not isolated "
         "algorithm memory.",
         "- The ICCS share covers only the three helper functions; candidates that implement their "
         "own SHAKE/AES/SM3 show that time as non-symmetric (see the survey).",
         "- Link-time wrapping is not reliable with LTO, so shares are measured on reference builds.",
         "- The complete functional test vectors are referenced by digest, not embedded.",
         "- Very slow operations have fewer than 100 timed calls; their pages say how many. "
         "Operations too slow for more than one call use their calibration call as the measurement.",
         secondary_core_text(run),
         "- A single key-exchange step is timed around each call, so its wall time includes the "
         "counter start/stop system calls (a floor of roughly a microsecond); its user-mode cycle "
         "count does not. The `exchange` figure has no such overhead.", ""]
    emit(method_page_path(out), "\n".join(L) + "\n")


def main():
    global SYSTEM, SYSTEM_DESC, CHECK
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("run_dir", type=Path)
    ap.add_argument("--system", help="system ID from performance/systems.csv (default: from the run)")
    ap.add_argument("--out", type=Path, default=OUT, help="directory for the shared pages")
    ap.add_argument("--check", action="store_true",
                    help="regenerate in memory and compare with the committed pages; exit 1 on any difference")
    a = ap.parse_args()
    run = Run(a.run_dir.resolve())
    systems = load_systems()
    host = (run.env or {}).get("hostname") or run.campaign.get("environment_at_start", {}).get("hostname")
    system = a.system or run.campaign.get("system_id") or next(
        (sid for sid, r in systems.items() if r["Hostname"] == host), None)
    if system not in systems:
        ap.error(f"unknown system {system!r} (host {host!r}); add it to performance/systems.csv or pass --system")
    SYSTEM, SYSTEM_DESC = system, systems[system]["Description"]
    CHECK = a.check
    arch = {"x86_64": "x86-64", "aarch64": "AArch64"}.get(systems[system]["Arch"], systems[system]["Arch"])
    out = a.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    cands = sorted(({c for (c, l) in run.records} | {e["candidate"] for e in run.build.values()}) - {BASELINE})
    for cand in cands:
        candidate_page(run, cand, out, arch)
    summary(run, out, arch)
    survey = SYSTEM == next(iter(systems))
    if survey:
        survey_page(run, out)
    method_page(run, out, arch)
    if CHECK:
        for path in DIFFERENT:
            print(f"differs from regenerated output: {path}")
        print(f"report check: system {SYSTEM}: {len(cands) + 2 + survey} pages, {len(DIFFERENT)} differ")
        return 1 if DIFFERENT else 0
    print(f"report: system {SYSTEM}: {len(cands)} candidate reports (<id>/perf_{SYSTEM}.md), "
          f"{summary_page(out).name}, {method_page_path(out).name}"
          f"{', symmetric-survey.md' if survey else ''} in {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
