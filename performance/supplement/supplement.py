#!/usr/bin/env python3
"""Supplementary measurements next to a published benchmark campaign.

    python3 performance/supplement/supplement.py check   --cpu 2
    python3 performance/supplement/supplement.py all     --cpu 2 [--run-dir DIR]
    python3 performance/supplement/supplement.py publish --run-dir DIR
    python3 performance/supplement/supplement.py report  [--system arm_1]

Two sets of measurements that the published datasets (performance/data/<ID>)
do not contain:

  reference  ML-KEM-512/768/1024 (FIPS 203) and ML-DSA-44/65/87 (FIPS 204):
             the pq-crystals reference implementations (upstream/, see
             PROVENANCE.md) behind the ICCS API, built with the guide's
             reference flags, checked against the upstream test vectors,
             timed like a candidate, and profiled for their SHA-3 and DRNG
             shares (hashprof with HASHPROF_SHA3=1).
  xof        The output-length test. Each submitted XOF instance is called
             through CryptHash with a short seed, its security level in bytes,
             and asks for 32 B ... 64 KiB of output (the hash test's S1-S8
             lengths, used as output lengths). Reference rows: ICCS pseudoXOF,
             SHAKE128 and SHAKE256 at every seed length used, and C Hash's
             LDXOF (submitted code, outside its ICCS interface).

The planning (trial target, operation budget) is that of the published
campaign of the same system, and the measuring phases refuse to run unless the
host state matches the published records for the same CPU (clock, governor,
boost, SMT, counter access, ...). Run directories go under performance/runs/
(Git-ignored); publish copies the evidence to performance/supplement/data/<ID>/
and report writes summary_<ID>.md, reference_<ID>.csv and xof_<ID>.csv.
The published campaign, its datasets and its pages are not touched.
"""

from __future__ import annotations

import argparse
import csv
import ctypes
import datetime as dt
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import platform
import socket
import subprocess
import sys

SUPP = Path(__file__).resolve().parent
sys.path.insert(0, str(SUPP.parent))
import campaign as C  # noqa: E402  (performance/campaign.py)

ROOT, PERF = C.ROOT, C.PERF
OUT_SIZES = C.HASH_SIZES

# candidate id -> package directory, operation kind, upstream tree, label -> upstream test-vector name
REFERENCE = {
    "ml-kem": ("ml-kem", "kem", "kyber",
               {"ML-KEM-512": "tvecs512", "ML-KEM-768": "tvecs768", "ML-KEM-1024": "tvecs1024"}),
    "ml-dsa": ("ml-dsa", "sign", "dilithium",
               {"ML-DSA-44": "tvecs2", "ML-DSA-65": "tvecs3", "ML-DSA-87": "tvecs5"}),
}
PKC_OPS = {"kem": [("keygen", 0), ("enc", 0), ("dec", 0)], "sign": [("keygen", 0), ("sign", 0), ("verify", 0)]}
# FIPS 203 (ek, ct) and FIPS 204 (pk, sig) sizes in bytes
FIPS_SIZES = {"ML-KEM-512": (800, 768), "ML-KEM-768": (1184, 1088), "ML-KEM-1024": (1568, 1568),
              "ML-DSA-44": (1312, 2420), "ML-DSA-65": (1952, 3309), "ML-DSA-87": (2592, 4627)}

XOF_REF = "xof-ref"
XOF_REF_DIR = "xof"
# Submitted XOF instances and their seed length: the security level in bytes.
# The level is the number in the instance name, except that MoFang names the
# digest length of the underlying hash (MoFang-256-XOF is a 128-bit and
# MoFang-768-XOF a 384-bit function); Laurus-XOF and litchi_xof use their
# assigned comparison targets (security_targets.csv).
XOF_ROWS = {
    ("hash-07", "Dragon-XOF-256"): 32, ("hash-07", "Dragon-XOF-384"): 48, ("hash-07", "Dragon-XOF-512"): 64,
    ("hash-14", "Laurus-XOF"): 32,
    ("hash-15", "litchi_xof"): 64,
    **{("hash-18", f"MEGASCON-{s}-XOF-{w}"): s // 8 for s in (384, 512) for w in (256, 1024, 2048)},
    ("hash-19", "MoFang-256-XOF"): 16, ("hash-19", "MoFang-768-XOF"): 48,
    **{("hash-20", f"MOZI-{s}-XOF-{w}"): s // 8 for s in (384, 512) for w in (256, 1024, 2048)},
    ("hash-33", "Thunder-XOF-256"): 32, ("hash-33", "Thunder-XOF-384"): 48, ("hash-33", "Thunder-XOF-512"): 64,
    (XOF_REF, "CHash-LDXOF-512"): 32, (XOF_REF, "CHash-LDXOF-1024"): 64,
    (XOF_REF, "Laurus-XOF-adapted"): 32, (XOF_REF, "litchi_xof-adapted"): 64,
}
# Submitted XOF code reached through an adapter (xof/Makefile): label -> the
# submitted instance whose output it must reproduce at that instance's width
# (None: no submitted instance exposes the function)
ADAPTED = {"CHash-LDXOF-512": None, "CHash-LDXOF-1024": None,
           "Laurus-XOF-adapted": ("hash-14", "Laurus-XOF", 2048),
           "litchi_xof-adapted": ("hash-15", "litchi_xof", 1024)}
XOF_REFS = ("pseudoXOF", "SHAKE128", "SHAKE256")
REF_SEEDS = sorted(set(XOF_ROWS.values()))
CPU_LOCK = None


def rel(p: Path) -> str:
    return p.relative_to(ROOT).as_posix()


def guide_flags() -> str:
    return f"{C.ARCH[platform.machine()]['reference']} {C.HARNESS_ADDITIONS}"


def seed_message(n: int) -> bytes:
    """The driver's deterministic input (ngcc_perf.c, op xof-SEED)."""
    return bytes((i * 131 + n) & 0xFF for i in range(n))


# ---------------------------------------------------------------- host state

def published(sid: str) -> Path:
    return PERF / "data" / sid


def published_env(sid: str, cpu: int) -> tuple[dict | None, set]:
    cpus = set()
    for f in sorted((published(sid) / "records").glob("*/*.json")):
        r = C.load(f) or {}
        env = r.get("environment") or {}
        cpus.add(env.get("cpu_number"))
        if r.get("status") == "complete" and env.get("cpu_number") == cpu:
            return env, cpus
    return None, cpus


def host_differences(sid: str, cpu: int) -> list[str]:
    ref, cpus = published_env(sid, cpu)
    if not ref:
        return [f"no published {sid} record was measured on CPU {cpu} (published CPUs: "
                f"{sorted(c for c in cpus if c is not None)})"]
    env = C.environment(cpu)
    return [f"{k}: published {ref.get(k)!r}, now {env.get(k)!r}" for k in C.FIXED_ENV
            if k in ref and ref.get(k) != env.get(k)]


# ---------------------------------------------------------------- self-tests (run in a child process)

def _ull(v=0):
    return ctypes.c_ulonglong(v)


def child_api_test(lib: str, kind: str, label: str) -> int:
    """KEM: sizes, round trip, implicit rejection. SIG: sizes, verify, rejection of
    a modified signature and of a modified message."""
    L = ctypes.CDLL(lib, mode=ctypes.RTLD_LOCAL)
    L.ngcc_seed(bytes(range(48)), _ull(48))
    ok = True
    if kind == "kem":
        for f in ("kem_get_pk_len_bytes", "kem_get_sk_len_bytes", "kem_get_ss_len_bytes", "kem_get_ct_len_bytes"):
            getattr(L, f).restype = ctypes.c_ulonglong
        pkl, skl, ssl, ctl = (L.kem_get_pk_len_bytes(), L.kem_get_sk_len_bytes(),
                              L.kem_get_ss_len_bytes(), L.kem_get_ct_len_bytes())
        print(f"sizes pk={pkl} sk={skl} ss={ssl} ct={ctl}")
        ok &= (pkl, ctl) == FIPS_SIZES[label] and ssl == 32
        pk, sk, ct = (ctypes.create_string_buffer(n) for n in (pkl, skl, ctl))
        ss, ss2, ss3 = (ctypes.create_string_buffer(ssl) for _ in range(3))
        a, b, c, d, e = (_ull() for _ in range(5))
        r1 = L.kem_keygen(pk, ctypes.byref(a), sk, ctypes.byref(b))
        r2 = L.kem_enc(pk, _ull(pkl), ss, ctypes.byref(c), ct, ctypes.byref(d))
        r3 = L.kem_dec(sk, _ull(skl), ct, _ull(ctl), ss2, ctypes.byref(e))
        print(f"keygen={r1} enc={r2} dec={r3} shared secrets equal={ss.raw == ss2.raw}")
        ok &= r1 == r2 == r3 == 0 and ss.raw == ss2.raw
        bad = bytearray(ct.raw)
        bad[0] ^= 1
        r4 = L.kem_dec(sk, _ull(skl), bytes(bad), _ull(ctl), ss3, ctypes.byref(e))
        print(f"modified ciphertext: dec={r4} implicit rejection gives a different key={ss3.raw != ss.raw}")
        ok &= r4 == 0 and ss3.raw != ss.raw
    else:
        for f in ("sig_get_pk_len_bytes", "sig_get_sk_len_bytes", "sig_get_sn_len_bytes"):
            getattr(L, f).restype = ctypes.c_ulonglong
        pkl, skl, snl = L.sig_get_pk_len_bytes(), L.sig_get_sk_len_bytes(), L.sig_get_sn_len_bytes()
        print(f"sizes pk={pkl} sk={skl} sig={snl}")
        ok &= (pkl, snl) == FIPS_SIZES[label]
        pk, sk, sn = (ctypes.create_string_buffer(n) for n in (pkl, skl, snl))
        msg = bytes(range(64))
        a, b, c = _ull(), _ull(), _ull()
        r1 = L.sig_keygen(pk, ctypes.byref(a), sk, ctypes.byref(b))
        r2 = L.sig_sign(sk, _ull(skl), msg, _ull(64), sn, ctypes.byref(c))
        r3 = L.sig_verify(pk, _ull(pkl), sn, c, msg, _ull(64))
        bad = bytearray(sn.raw)
        bad[snl // 2] ^= 1
        r4 = L.sig_verify(pk, _ull(pkl), bytes(bad), c, msg, _ull(64))
        r5 = L.sig_verify(pk, _ull(pkl), sn, c, bytes([1]) + msg[1:], _ull(64))
        print(f"keygen={r1} sign={r2} (length {c.value}) verify={r3} "
              f"modified signature={r4} modified message={r5}")
        ok &= r1 == r2 == r3 == 0 and c.value == snl and r4 == -1 and r5 == -1
    print("API", "OK" if ok else "FAIL")
    return 0 if ok else 1


GUARD = 4096    # bytes after the requested output, to catch an implementation that writes more


def crypt_hash(L, out_bytes: int, msg: bytes, fill: int) -> tuple[int, bytes]:
    """CryptHash(8*out_bytes, msg); returns the output followed by 64 guard bytes."""
    buf = ctypes.create_string_buffer(bytes([fill]) * (out_bytes + GUARD), out_bytes + GUARD)
    rc = L.CryptHash(ctypes.c_int(8 * out_bytes), msg, _ull(8 * len(msg)), buf)
    raw = buf.raw
    overrun = raw[out_bytes:] != bytes([fill]) * GUARD
    return rc, raw[:out_bytes] + (bytes([fill ^ 0x5A]) * 64 if overrun else bytes([fill]) * 64)


def child_probe(lib: str, seed: int) -> int:
    """For each output length: return code, whether the whole output and nothing
    beyond it was written (two calls over different fills), and the output's
    SHA-256; then whether shorter outputs are prefixes of the longest."""
    L = ctypes.CDLL(lib, mode=ctypes.RTLD_LOCAL)
    msg = seed_message(seed)
    rows, outputs = [], {}
    for n in OUT_SIZES:
        rc0, b0 = crypt_hash(L, n, msg, 0x00)
        rc1, b1 = crypt_hash(L, n, msg, 0xFF)
        full = rc0 == rc1 == 0 and b0[:n] == b1[:n]
        clean = b0[n:] == bytes(64) and b1[n:] == b"\xff" * 64
        if full:
            outputs[n] = b0[:n]
        rows.append({"output_bytes": n, "rc": rc0, "full_output": full, "no_overrun": clean,
                     "sha256": hashlib.sha256(b0[:n]).hexdigest() if full else None})
    longest = outputs.get(max(OUT_SIZES))
    for r in rows:
        r["prefix_of_longest"] = (longest is not None and r["output_bytes"] in outputs
                                  and longest[:r["output_bytes"]] == outputs[r["output_bytes"]])
    print(json.dumps({"seed_bytes": seed, "sizes": rows}))
    return 0


def iccs_model():
    spec = importlib.util.spec_from_file_location("iccs_selftest", PERF / "iccs" / "selftest.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def child_xof_test(lib: str, label: str, submitted: str = "", width: str = "0") -> int:
    """SHAKE against hashlib, pseudoXOF against the SM3 model of iccs/selftest.py.
    An adapted submitted XOF must reproduce the submitted (KAT-checked) library at
    that library's fixed width. LDXOF has no reference vectors: deterministic,
    complete output, no overrun (NOKAT). Prefix consistency is only reported."""
    L = ctypes.CDLL(lib, mode=ctypes.RTLD_LOCAL)
    lengths = (1, 16, 32, 136, 137, 168, 169, 1024, 4096, 65536)
    if label in ADAPTED:
        ok, prefix = True, True
        for seed in (16, 32, 64, 128):
            msg = seed_message(seed)
            rc, big = crypt_hash(L, 65536, msg, 0)
            for n in lengths:
                r0, b0 = crypt_hash(L, n, msg, 0x00)
                r1, b1 = crypt_hash(L, n, msg, 0xFF)
                ok &= rc == r0 == r1 == 0 and b0[:n] == b1[:n] and b0[n:] == bytes(64) and b1[n:] == b"\xff" * 64
                prefix &= b0[:n] == big[:n]
        print(f"deterministic, complete output, no overrun: {ok}; shorter outputs are prefixes: {prefix}")
        if submitted:
            S = ctypes.CDLL(submitted, mode=ctypes.RTLD_LOCAL)
            w = int(width) // 8
            same = True
            for seed in (0, 1, 16, 32, 64, 128, 136, 1000):
                msg = seed_message(seed) if seed else b""
                _, a = crypt_hash(L, w, msg, 0)
                _, s = crypt_hash(S, w, msg, 0)
                same &= a[:w] == s[:w]
            print(f"equals the submitted library at its {width}-bit width: {same}")
            ok &= same
        else:
            print("no submitted instance or reference vectors for this function")
        return 0 if ok else 1
    if label == "pseudoXOF":
        model = iccs_model()
        ref = lambda n, m: model.pseudoxof(8 * n, m)  # noqa: E731
    else:
        h = hashlib.shake_128 if label == "SHAKE128" else hashlib.shake_256
        ref = lambda n, m: h(m).digest(n)  # noqa: E731
    bad = 0
    for seed in (16, 32, 48, 64, 96):
        msg = seed_message(seed)
        for n in lengths:
            rc, b = crypt_hash(L, n, msg, 0xA5)
            if rc != 0 or b[:n] != ref(n, msg) or b[n:] != b"\xa5" * 64:
                bad += 1
                print(f"MISMATCH seed={seed} output={n} rc={rc}")
    print(f"{label}: {5 * len(lengths) - bad}/{5 * len(lengths)} outputs match the independent model")
    return 0 if bad == 0 else 1


def run_child(args: list[str]) -> tuple[int, str]:
    p = subprocess.run([sys.executable, str(Path(__file__).resolve()), *args],
                       capture_output=True, text=True, timeout=3600)
    return p.returncode, p.stdout + p.stderr


# ---------------------------------------------------------------- build

def entry_for(run: Path, cand: str, label: str, lib: Path, kat: str, logf: Path, variant: str,
              ops: list[tuple[str, int]], **extra) -> dict:
    return {"candidate": cand, "label": label, "instance": label, "variant": variant, "flags": "guide",
            "kat": kat, "library": rel(lib), "library_sha256": C.sha256(lib),
            "elf_load_bytes": C.elf_load_bytes(lib), "kat_log": logf.relative_to(run).as_posix(),
            "kat_log_sha256": C.sha256(logf), "operations": [[o, n] for o, n in ops], **extra}


def build_package(run: Path, args, name: str) -> int:
    return C.run_make(["make", "-B", f"-j{args.jobs}", "-C", rel(SUPP / name), "libs",
                      f"NGCC_CFLAGS={guide_flags()}"], run / "build" / f"{name}.log")


def phase_build(run: Path, args) -> None:
    state = C.load(run / "build.json", {"instances": {}})
    flags = guide_flags()

    # ML-KEM and ML-DSA: upstream test vectors with the same flags, then the API
    for cand, (pkg, kind, upstream, tvecs) in REFERENCE.items():
        rc = build_package(run, args, pkg)
        sums = dict(reversed(l.split()) for l in (SUPP / "upstream" / upstream / "SHA256SUMS").read_text().splitlines() if l.strip())
        for label, tv in tvecs.items():
            lib = SUPP / pkg / "lib" / f"lib{label}.so"
            logf = run / "kat" / cand / f"{label}.log"
            logf.parent.mkdir(parents=True, exist_ok=True)
            lines = [f"instance = {label}", f"flags = {flags}"]
            status = "BUILDFAIL"
            if rc == 0 and lib.is_file():
                trc = C.run_make(["make", "-C", rel(SUPP / pkg), f"tvecs/{label}", f"NGCC_CFLAGS={flags}"],
                                 run / "build" / f"{pkg}.log")
                out = subprocess.run([str(SUPP / pkg / "tvecs" / label)], capture_output=True).stdout if trc == 0 else b""
                got = hashlib.sha256(out).hexdigest()
                tv_ok = trc == 0 and got == sums.get(tv)
                lines += [f"upstream test vectors {tv}: sha256 {got}, expected {sums.get(tv)}: "
                          f"{'MATCH' if tv_ok else 'DIFFERENT'}"]
                arc, text = run_child(["_apitest", str(lib), kind, label])
                lines += [text.rstrip()]
                status = "PASS" if tv_ok and arc == 0 else "FAIL"
            lines.append(f"RESULT {cand} {label} {status}")
            logf.write_text("\n".join(lines) + "\n", encoding="utf-8")
            if status in ("PASS", "FAIL") and lib.is_file():
                state["instances"][f"{cand}/{label}"] = entry_for(
                    run, cand, label, lib, status, logf, "supplement-reference", PKC_OPS[kind],
                    dir=rel(SUPP / pkg))
            C.log(run, f"build {cand} {label}: {status}")
            C.save(run / "build.json", state)

    # the submitted XOFs: the candidates' own harness builds and KATs
    probes = C.load(run / "xofprobe.json", {})
    pub = (C.load(published(args.sid) / "build.json") or {}).get("instances", {})
    xof_cands = sorted({c for c, _ in XOF_ROWS if c != XOF_REF})
    if args.probe_all_hashes:
        xof_cands = sorted(set(xof_cands) | set(c for c in C.candidates(r"^hash-")))
    for cand in xof_cands:
        built = C.build_candidate(run, args, cand, args.jobs)
        groups: dict[tuple, list[str]] = {}
        for key, e in sorted(built.items(), key=lambda kv: kv[0]):
            label = e["label"]
            row = (cand, label) in XOF_ROWS
            if not row and not args.probe_all_hashes:
                continue
            if e.get("kat") != "PASS" or not e.get("library"):
                C.log(run, f"xof {key}: KAT {e.get('kat')}; not probed")
                continue
            seed = XOF_ROWS.get((cand, label), 32)
            probe = probe_library(ROOT / e["library"], seed)
            pe = pub.get(key, {})
            probe.update(xof_instance=row, published_library_sha256=pe.get("library_sha256"),
                         same_binary_as_published=pe.get("library_sha256") == e.get("library_sha256"))
            probes[f"{cand}/{label}/{seed}"] = probe
            if not row:
                continue
            adapted = next((a for a, s in ADAPTED.items() if s and s[:2] == (cand, label)), None)
            if adapted:
                probe["timed_as"] = adapted
                C.log(run, f"xof {key}: CryptHash ignores the requested length; timed as {XOF_REF}/{adapted}")
                continue
            sizes = probe.get("sizes", [])
            ok = [r["output_bytes"] for r in sizes if r["full_output"] and r["no_overrun"]]
            # instances that differ only in their default output width are one function:
            # identical outputs for the same seed at every accepted length
            sig = (seed, tuple((r["output_bytes"], r["sha256"]) for r in sizes))
            groups.setdefault(sig, []).append(label)
            if len(groups[sig]) > 1:
                probe["same_function_as"] = groups[sig][0]
                C.log(run, f"xof {key}: same outputs as {groups[sig][0]}; not timed separately")
                continue
            if not ok:
                why = "probe failed: " + probe["error"][-300:] if "error" in probe else \
                    "CryptHash accepted no requested output length"
                C.log(run, f"xof {key}: {why}; not timed")
                continue
            state["instances"][key] = {**e, "operations": [[f"xof-{seed}", n] for n in ok],
                                       "xof_seed_bytes": seed}
            C.log(run, f"xof {key}: seed {seed} B, {len(ok)}/{len(OUT_SIZES)} output lengths accepted, "
                       f"same binary as published: {probe['same_binary_as_published']}")
        C.save(run / "xofprobe.json", probes)
        C.save(run / "build.json", state)

    # reference rows (pseudoXOF, SHAKE128, SHAKE256 at every seed length) and the
    # submitted XOF functions reached through an adapter
    rc = build_package(run, args, XOF_REF_DIR)
    for label in (*XOF_REFS, *ADAPTED):
        lib = SUPP / XOF_REF_DIR / "lib" / f"lib{label}.so"
        logf = run / "kat" / XOF_REF / f"{label}.log"
        logf.parent.mkdir(parents=True, exist_ok=True)
        if rc != 0 or not lib.is_file():
            logf.write_text(f"RESULT {XOF_REF} {label} BUILDFAIL\n", encoding="utf-8")
            C.log(run, f"build {XOF_REF} {label}: BUILDFAIL")
            continue
        extra = []
        if ADAPTED.get(label):
            cand, sub, width = ADAPTED[label]
            sublib = ROOT / cand / "lib" / f"lib{sub}.so"
            if C.kat_status(cand, sub)[0] != "PASS" or not sublib.is_file():
                logf.write_text(f"submitted {cand}/{sub} did not build or pass its KATs\n"
                                f"RESULT {XOF_REF} {label} FAIL\n", encoding="utf-8")
                C.log(run, f"build {XOF_REF} {label}: FAIL (submitted {cand}/{sub} not KAT-passing)")
                continue
            extra = [str(sublib), str(width)]
        trc, text = run_child(["_xoftest", str(lib), label, *extra])
        status = "PASS" if trc == 0 else "FAIL"
        if trc == 0 and label in ADAPTED and not ADAPTED[label]:
            status = "NOKAT"     # timed, but flagged: no reference vectors exist
        logf.write_text(f"instance = {label}\nflags = {flags}\n{text.rstrip()}\nRESULT {XOF_REF} {label} {status}\n",
                        encoding="utf-8")
        seeds = REF_SEEDS if label in XOF_REFS else [XOF_ROWS[(XOF_REF, label)]]
        ops = []
        for seed in seeds:
            probe = probe_library(lib, seed)
            probes[f"{XOF_REF}/{label}/{seed}"] = probe
            ops += [(f"xof-{seed}", r["output_bytes"]) for r in probe.get("sizes", []) if r["full_output"] and r["no_overrun"]]
        if status in ("PASS", "NOKAT"):
            state["instances"][f"{XOF_REF}/{label}"] = entry_for(
                run, XOF_REF, label, lib, status, logf, "supplement-reference", ops, dir=rel(SUPP / XOF_REF_DIR),
                **({"adapts": f"{ADAPTED[label][0]}/{ADAPTED[label][1]}"} if ADAPTED.get(label) else {}))
        C.log(run, f"build {XOF_REF} {label}: {status}, {len(ops)} output-length operations")
    C.save(run / "xofprobe.json", probes)
    C.save(run / "build.json", state)


def probe_library(lib: Path, seed: int) -> dict:
    """The output-length probe in a child process (a library that writes past the
    requested length must not corrupt this one); its JSON even if it then crashed."""
    prc, text = run_child(["_probe", str(lib), str(seed)])
    first = text.splitlines()[0] if text else ""
    probe = json.loads(first) if first.startswith("{") else {"error": text[-2000:]}
    probe["child_exit"] = prc
    return probe


# ---------------------------------------------------------------- measure and profile

def namespace(args) -> argparse.Namespace:
    return argparse.Namespace(cpu=args.cpu, only=None, defer=None, deferred_only=False, shard=None)


def phase_measure(run: Path, args) -> None:
    C.phase_calibrate(run, namespace(args))
    C.phase_measure(run, namespace(args))


def phase_profile(run: Path, args) -> None:
    """SHA-3 and DRNG shares of the ML-KEM and ML-DSA operations (hashprof)."""
    state = C.load(run / "build.json", {"instances": {}})
    for e in state["instances"].values():
        cand, label = e["candidate"], e["label"]
        if cand not in REFERENCE or e.get("kat") != "PASS":
            continue
        p = subprocess.run([str(PERF / "hashprof/relink.sh"), e["dir"], label], capture_output=True, text=True,
                           env={**os.environ, "HASHPROF_SHA3": "1"})
        if p.returncode:
            C.save(run / "profile" / cand / f"{label}__RELINK_FAILED.json", {"stderr": p.stderr[-2000:]})
            C.log(run, f"profile {cand} {label}: relink failed")
            continue
        lib = p.stdout.strip()
        for op, size in e["operations"]:
            out = run / "profile" / cand / f"{label}__{C.op_key(op, size)}.json"
            if out.is_file():
                continue
            cal = C.load(run / "calibrate" / cand / f"{label}__{C.op_key(op, size)}.json") or {}
            t1 = cal.get("t1_s") or 1.0
            iters = 1 if t1 > 60 else min(1000, max(3, math.ceil(1.0 / t1)))
            cmd = ["taskset", "-c", str(args.cpu), "performance/hashprof/hashprof",
                   os.path.relpath(lib, ROOT), op, str(size), str(iters), "1"]
            pr = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, timeout=3 * (iters + 1) * t1 + 300)
            meta, calls, total = {}, [], None
            for line in pr.stdout.splitlines():
                f = line.split("\t")
                if f[0] == "META" and len(f) == 3:
                    meta[f[1]] = f[2]
                elif f[0] == "TOTAL":
                    total = {"operations": int(f[1]), "ticks": int(f[2])}
                elif f[0] == "CALL":
                    calls.append({"fn": f[1], "in_bits": int(f[2]), "out_bits": int(f[3]),
                                  "calls": int(f[4]), "ticks": int(f[5])})
            rec = {"schema": 2, "candidate": cand, "label": label, "operation": op, "input_bytes": size,
                   "status": "complete" if pr.returncode == 0 and total else "failed", "command": cmd,
                   "stderr": pr.stderr[-2000:], "meta": meta, "total": total, "calls": calls}
            if total and total["ticks"]:
                by_fn: dict[str, int] = {}
                for c in calls:
                    by_fn[c["fn"]] = by_fn.get(c["fn"], 0) + c["ticks"]
                rec["share"] = {fn: t / total["ticks"] for fn, t in by_fn.items()}
            C.save(out, rec)
            sh = rec.get("share", {})
            C.log(run, f"profile {cand} {label} {op}: sha3={100 * sh.get('sha3', 0):.1f}% "
                       f"drng={100 * sh.get('drng', 0):.1f}%")


# ---------------------------------------------------------------- publish and report

def phase_publish(run: Path) -> int:
    meta = C.load(run / "campaign.json") or {}
    sid = meta.get("system_id")
    if not sid:
        print(f"{run}: campaign.json has no system_id", file=sys.stderr)
        return 2
    files = [run / "campaign.json", run / "build.json", run / "xofprobe.json"]
    files += sorted((run / "records").rglob("*.json")) + sorted((run / "kat").rglob("*.log"))
    files += sorted(f for f in (run / "profile").rglob("*__*.json") if "RELINK" not in f.name)
    staged = {}
    for f in files:
        if not f.is_file():
            continue
        text = f.read_text(encoding="utf-8", errors="replace").replace(str(ROOT) + "/", "").replace(str(ROOT), ".")
        for bad in ("/home/", "/tmp/", "/root/", "/Users/"):
            if bad in text:
                print(f"{f.relative_to(run)}: absolute path {bad!r} remains; not publishing", file=sys.stderr)
                return 2
        staged[f.relative_to(run).as_posix()] = text
    dest = SUPP / "data" / sid
    if dest.exists():
        import shutil
        shutil.rmtree(dest)
    for name, text in staged.items():
        (dest / name).parent.mkdir(parents=True, exist_ok=True)
        (dest / name).write_text(text, encoding="utf-8")
    print(f"publish: {len(staged)} files -> {rel(dest)}/")
    return 0


DERIVED: set = set()     # records whose cycles were derived from time x clock


def cycles_of(r: dict) -> float | None:
    if r.get("status") != "complete":
        return None
    if r.get("mean_cycles"):
        return r["mean_cycles"]
    hz = C.fixed_clock(r.get("environment") or {})
    if hz and r.get("mean_seconds"):
        DERIVED.add((r["candidate"], r["label"], r["operation"], r["input_bytes"]))
        return r["mean_seconds"] * hz
    return None


def fmt(v: float | None) -> str:
    if v is None:
        return "–"
    for unit, scale in (("G", 1e9), ("M", 1e6), ("k", 1e3)):
        if v >= scale:
            return f"{v / scale:.3g} {unit}"
    return f"{v:.0f}"


def phase_report(sid: str) -> int:
    ds = SUPP / "data" / sid
    build = (C.load(ds / "build.json") or {}).get("instances", {})
    camp = C.load(ds / "campaign.json") or {}
    probes = C.load(ds / "xofprobe.json") or {}
    if not build:
        print(f"no published supplementary dataset for {sid}", file=sys.stderr)
        return 2
    recs: dict[tuple, dict] = {}
    for f in sorted((ds / "records").rglob("*.json")):
        r = C.load(f)
        recs[(r["candidate"], r["label"], r["operation"], r["input_bytes"])] = r
    profs = {(p["candidate"], p["label"], p["operation"]): p
             for p in (C.load(f) for f in sorted((ds / "profile").rglob("*.json")))}
    env = camp.get("environment_at_start", {})
    lines = [f"# Supplementary measurements ({sid})", "",
             f"Host `{env.get('hostname')}` ({env.get('cpu_model')}), CPU {env.get('cpu_number')}, "
             f"compiler `{env.get('compiler')}`; reference flags `{C.ARCH.get(env.get('architecture'), {}).get('reference')}`; "
             f"planning {camp.get('planning')}. Generated by `performance/supplement/supplement.py report` "
             f"from `performance/supplement/data/{sid}/`. Mean cycles per call.", "",
             "## ML-KEM and ML-DSA reference implementations", "",
             "pq-crystals reference code (see `PROVENANCE.md`) behind the ICCS API, same driver, flags, "
             "seeded DRNG and 64-byte message as the candidates. SHA-3 and DRNG are the shares of the "
             "operation's time inside the SHA-3 entry points and the ICCS DRNG (hashprof).", "",
             "| instance | operation | cycles | SHA-3 | DRNG | cycles excl. SHA-3 and DRNG | public key | ciphertext / signature | self-test |",
             "|---|---|---|---|---|---|---|---|---|"]
    rows_ref = []
    for cand in REFERENCE:
        for label in REFERENCE[cand][3]:
            e = build.get(f"{cand}/{label}")
            if not e:
                continue
            for op, size in e["operations"]:
                r = recs.get((cand, label, op, size), {})
                cyc = cycles_of(r) if r else None
                sh = profs.get((cand, label, op), {}).get("share")
                s3, dr = (sh.get("sha3", 0.0), sh.get("drng", 0.0)) if sh is not None else (None, None)
                rest = cyc * (1 - (s3 or 0) - (dr or 0)) if cyc and sh else None
                sizes = r.get("sizes", {})
                pk = sizes.get("pk_bytes")
                second = sizes.get("ct_bytes") or sizes.get("signature_bytes")
                lines.append(f"| {label} | {op} | {fmt(cyc)} | {'' if s3 is None else f'{100 * s3:.1f}%'} | "
                             f"{'' if dr is None else f'{100 * dr:.1f}%'} | {fmt(rest)} | {pk or ''} | {second or ''} | {e['kat']} |")
                rows_ref.append({"candidate": cand, "label": label, "operation": op,
                                 "mean_cycles": round(cyc) if cyc else "", "sha3_share": s3 if s3 is not None else "",
                                 "drng_share": dr if dr is not None else "", "pk_bytes": pk or "",
                                 "ct_or_signature_bytes": second or "", "self_test": e["kat"]})
    lines += ["", "## XOF output-length test", "",
              "CryptHash with a short seed (the security level in bytes) and the requested output length; "
              "`c/B` is cycles per output byte. Instances that differ only in their default output width "
              "and give identical outputs are timed once. C Hash's LDXOF is submitted code outside its "
              "ICCS interface and has no reference vectors (NOKAT).", "",
              "| instance | seed | " + " | ".join(f"{n} B" if n < 1024 else f"{n // 1024} KiB" for n in OUT_SIZES)
              + " | c/B 1 KiB | c/B 64 KiB | × SHAKE256 64 KiB | × pseudoXOF 64 KiB | prefix | notes |",
              "|---|---|" + "---|" * len(OUT_SIZES) + "---|---|---|---|---|---|"]
    rows_xof = []

    def xof_cycles(cand, label, seed, n):
        r = recs.get((cand, label, f"xof-{seed}", n))
        return cycles_of(r) if r else None
    keys = [(c, l, s) for (c, l), s in XOF_ROWS.items() if f"{c}/{l}" in build]
    keys += [(XOF_REF, l, s) for l in XOF_REFS for s in REF_SEEDS if f"{XOF_REF}/{l}" in build]
    big = max(OUT_SIZES)
    for cand, label, seed in keys:
        cyc = [xof_cycles(cand, label, seed, n) for n in OUT_SIZES]
        if not any(cyc):
            continue
        shake = xof_cycles(XOF_REF, "SHAKE256", seed, big)
        pxof = xof_cycles(XOF_REF, "pseudoXOF", seed, big)
        probe = probes.get(f"{cand}/{label}/{seed}", {})
        prefix = all(r.get("prefix_of_longest") for r in probe.get("sizes", []) if r.get("full_output"))
        same = sorted(k.split("/")[1] for k, p in probes.items()
                      if p.get("same_function_as") == label and k.startswith(cand + "/"))
        notes = []
        if same:
            notes.append("same outputs: " + ", ".join(same))
        if build.get(f"{cand}/{label}", {}).get("adapts"):
            notes.append(f"`{build[f'{cand}/{label}']['adapts']}` function, requested length passed by an "
                         "adapter (its CryptHash fixes the width); equals the submitted library at that width")
        if label.startswith("CHash-LDXOF"):
            notes.append("`hash-03` LDXOF (spec. App. B), outside the ICCS interface; no reference vectors")
        c1k, cbig = cyc[OUT_SIZES.index(1024)], cyc[-1]
        lines.append(f"| {label} | {seed} B | " + " | ".join(fmt(c) for c in cyc)
                     + f" | {'' if not c1k else f'{c1k / 1024:.1f}'} | {'' if not cbig else f'{cbig / big:.1f}'}"
                     + f" | {'' if not (cbig and shake) else f'{cbig / shake:.2f}'}"
                     + f" | {'' if not (cbig and pxof) else f'{cbig / pxof:.2f}'}"
                     + f" | {'yes' if prefix else 'no'} | {'; '.join(notes)} |")
        for n, c in zip(OUT_SIZES, cyc):
            rows_xof.append({"candidate": cand, "label": label, "seed_bytes": seed, "output_bytes": n,
                             "mean_cycles": round(c) if c else "",
                             "cycles_per_output_byte": round(c / n, 3) if c else "",
                             "prefix_consistent": prefix, "self_test": build[f"{cand}/{label}"]["kat"]})
    if DERIVED:
        lines += ["", f"**{len(DERIVED)} measurements had no hardware cycle count; their cycles are the "
                  "measured time multiplied by the fixed clock.**"]
    other = {k.rsplit("/", 1)[0]: max(r["output_bytes"] for r in p["sizes"] if r.get("full_output") and r.get("no_overrun"))
             for k, p in probes.items() if not p.get("xof_instance", True)
             and sum(1 for r in p.get("sizes", []) if r.get("full_output") and r.get("no_overrun")) > 1}
    if other:
        lines += ["", "Fixed-length hash instances whose CryptHash honoured more than one requested output "
                  "length (largest shown; not timed, and not the XOF mode of a specification): "
                  + ", ".join(f"`{k}` ({n} B)" for k, n in sorted(other.items())) + "."]
    (SUPP / f"summary_{sid}.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    for name, rows in ((f"reference_{sid}.csv", rows_ref), (f"xof_{sid}.csv", rows_xof)):
        if rows:
            with (SUPP / name).open("w", newline="", encoding="utf-8") as f:
                w = csv.DictWriter(f, fieldnames=list(rows[0]))
                w.writeheader()
                w.writerows(rows)
    print(f"report: {rel(SUPP / f'summary_{sid}.md')}, {len(rows_ref)} reference rows, {len(rows_xof)} XOF rows")
    return 0


# ---------------------------------------------------------------- main

def main() -> int:
    if len(sys.argv) > 1 and sys.argv[1].startswith("_"):
        cmd, rest = sys.argv[1], sys.argv[2:]
        if cmd == "_apitest":
            return child_api_test(*rest)
        if cmd == "_probe":
            return child_probe(rest[0], int(rest[1]))
        if cmd == "_xoftest":
            return child_xof_test(*rest)
        return 2
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("phase", choices=["check", "build", "measure", "profile", "all", "publish", "report"])
    ap.add_argument("--cpu", type=int, default=2)
    ap.add_argument("--run-dir", type=Path)
    ap.add_argument("--system", help="system ID from performance/systems.csv (default: by hostname)")
    ap.add_argument("--jobs", type=int, default=8)
    ap.add_argument("--probe-all-hashes", action="store_true",
                    help="also build and probe every hash candidate's instances (not timed)")
    ap.add_argument("--allow-unfixed-host", action="store_true",
                    help="tests only: run on a host whose state differs from the published records")
    args = ap.parse_args()
    args.sid = C.system_id(args.system)
    if args.phase == "report":
        if not args.sid:
            ap.error("unknown system; pass --system")
        return phase_report(args.sid)
    if args.phase == "publish":
        if not args.run_dir:
            ap.error("publish needs --run-dir")
        return phase_publish(args.run_dir if args.run_dir.is_absolute() else ROOT / args.run_dir)
    if not args.sid or not published(args.sid).is_dir():
        print("this machine has no published dataset (performance/data/<ID>); pass --system", file=sys.stderr)
        return 2
    problems = C.host_problems(args.cpu) + host_differences(args.sid, args.cpu)
    if args.phase == "check":
        for p in problems:
            print("HOST:", p)
        print("host ready" if not problems else f"{len(problems)} problem(s)")
        return 1 if problems else 0
    if problems and args.phase != "build" and not args.allow_unfixed_host:
        for p in problems:
            print("HOST:", p, file=sys.stderr)
        print("refusing to measure: the host state differs from the published records", file=sys.stderr)
        return 2
    # the published campaign's planning, so iteration counts follow the same rules
    planning = (C.load(published(args.sid) / "campaign.json") or {}).get("planning") or {}
    C.TRIAL_TARGET_S = planning.get("trial_target_s", C.TRIAL_TARGET_S)
    C.OP_BUDGET_S = planning.get("op_budget_s", C.OP_BUDGET_S)
    if not args.run_dir:
        stamp = dt.datetime.now(dt.UTC).strftime("%Y%m%dT%H%M%SZ")
        args.run_dir = PERF / "runs" / f"{socket.gethostname()}-supplement-{stamp}"
    run = args.run_dir if args.run_dir.is_absolute() else ROOT / args.run_dir
    run.mkdir(parents=True, exist_ok=True)
    meta = C.load(run / "campaign.json", {})
    if meta.get("system_id") not in (None, args.sid):
        print(f"run directory belongs to {meta['system_id']}", file=sys.stderr)
        return 2
    meta.setdefault("system_id", args.sid)
    meta.setdefault("kind", "supplement")
    meta.setdefault("started_utc", dt.datetime.now(dt.UTC).isoformat())
    meta.setdefault("environment_at_start", C.environment(args.cpu))
    meta["planning"] = {**planning, "source": f"performance/data/{args.sid}/campaign.json"}
    meta["arch_config"] = C.ARCH.get(platform.machine())
    meta["harness_additions"] = C.HARNESS_ADDITIONS
    meta.setdefault("host_checks", []).append({"phase": args.phase, "cpu": args.cpu, "problems": problems,
                                               "utc": dt.datetime.now(dt.UTC).isoformat()})
    C.save(run / "campaign.json", meta)
    subprocess.run(["make", "-s", "-C", "performance"], cwd=ROOT, check=True)
    subprocess.run(["make", "-s", "-C", "api", "harness"], cwd=ROOT, check=True)
    import fcntl
    global CPU_LOCK
    (PERF / "runs").mkdir(exist_ok=True)
    CPU_LOCK = open(PERF / "runs" / f".cpu{args.cpu}.lock", "w")
    fcntl.flock(CPU_LOCK, fcntl.LOCK_EX)
    C.log(run, f"supplement {args.phase} in {rel(run) if run.is_relative_to(ROOT) else run} on CPU {args.cpu}"
               + (f" (HOST NOT AS PUBLISHED: {'; '.join(problems)})" if problems else ""))
    phases = {"build": phase_build, "measure": phase_measure, "profile": phase_profile}
    for name in (["build", "measure", "profile"] if args.phase == "all" else [args.phase]):
        phases[name](run, args)
    meta = C.load(run / "campaign.json", {})
    meta["ended_utc"] = dt.datetime.now(dt.UTC).isoformat()
    meta["environment_at_end"] = C.environment(args.cpu)
    C.save(run / "campaign.json", meta)
    C.log(run, f"supplement {args.phase} finished")
    return 0


if __name__ == "__main__":
    sys.exit(main())
