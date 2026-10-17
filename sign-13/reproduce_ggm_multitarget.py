#!/usr/bin/env python3
"""GreatWall: unindexed GGM nodes and leaf commitments give multi-target seed recovery.

    python3 sign-13/reproduce_ggm_multitarget.py tree    # sign-13-3, internal GGM nodes
    python3 sign-13/reproduce_ggm_multitarget.py leaf    # sign-13-4, leaf commitments
    NGCC_LONG=1 python3 sign-13/reproduce_ggm_multitarget.py tree   # more signatures and profiles

Three parts:

1. Static certificate.  All sixteen reference and optimized vector_com.c files
   (byte-identical within each tree) expand every internal node with the same
   PRG(K || iv) map and commit every leaf with the same H0(K || iv); neither
   call takes a node, leaf or vector index.  The specification text of
   Algorithms 4 and 14 is checked for the same unindexed calls when pdftotext
   is available.
2. Real signatures.  Signatures from the prebuilt sign-13/lib/*.so libraries are
   parsed with the submitted layout.  The opened internal-node seeds and the tau
   published hidden-leaf commitments are counted, cross-checked against an
   independent recomputation from chall3, and turned into the multi-target
   search costs 2^lambda/(T+1) (internal nodes) and 2^lambda/tau (leaves).
3. Reduced-width recovery.  A temporary copy of the GreatWall128s reference
   source is built with one inserted call that keeps the low B bits of every PRG
   key.  The unmodified batch_vector_commit/batch_vector_open produce the
   opening; the attacker sees only (opening, chall3, iv) and searches all
   published targets at once with the submitted expand_chunk/write_leaf.
   Controls: a single-target search, a wrong-IV search, and ground truth.
"""

from __future__ import annotations

import ctypes
import hashlib
import math
import os
import random
import re
import shutil
import statistics
import subprocess
import sys
import tempfile
import time
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CAND = ROOT / "sign-13"
IMPL = CAND / "Implementations"
TREES = ("Reference_Implementation", "Optimized_Implementation")
PROFILES = ("128s", "128f", "192s", "192f", "256s", "256f", "512s", "512f")
# Classical security claimed in spec Table 3 for each variant.
CLAIM = {128: 128, 192: 192, 256: 256, 512: 512}
NGCC_FLOOR = 128
# Exponents computed from the median opened-target counts of the submitted KAT
# signatures (100 per profile); the sampled signatures below must agree.
KAT_EXPONENT = {
    "128s": 121.37, "128f": 121.25, "192s": 184.56, "192f": 184.64,
    "256s": 248.13, "256f": 248.21, "512s": 503.11, "512f": 503.16,
}
LONG = os.environ.get("NGCC_LONG", "0") == "1"
B_GRIND = 16  # VOLE-consistency hash extra bits (spec B)


def fail(msg: str, code: int = 1) -> None:
    print(f"NOT CONFIRMED {ID}: {msg}")
    sys.exit(code)


# --------------------------------------------------------------------------
# 1. Static certificate
# --------------------------------------------------------------------------

TREE_NEEDLES = [
    ("expand_chunk(\n\tbool leaf, size_t n, uint32_t stretch, block128 iv,",
     "the expansion routine takes a seed and the signature IV, and no node index"),
    ("\t\tmemcpy(&ivs_tree[i], &iv, sizeof(ivs_tree[i]));",
     "every expanded node uses the same IV block"),
    ("expand_chunk_switch(TREE_CHUNK_SIZE, iv, &fixed_key_tree, &tree[ next_to_expand_from ], &tree[next_to_expand_to]);",
     "signer: (K_2a+1, K_2a+2) = PRG(K_a, iv)"),
    ("expand_chunk(0, 1, 2, iv, &fixed_key_tree, &fixed_key_leaf, &tree[ node ], &tree[2*node + 1]);",
     "verifier: every revealed seed is expanded by the same map"),
]
LEAF_NEEDLES = [
    ("\texpand_chunk(1, 1, 3, iv, NULL, fixed_key_leaf, seed, prg_output);",
     "write_leaf: (sd, leafcom) = H0(K, iv), no leaf or vector index"),
    ("expand_chunk_leaf_n_leaf_chunk_size(iv, &fixed_key_leaf, tree + BATCH_VECTOR_COMMIT_LEAVES - 1 + i, prg_output + 3*i);",
     "signer: every leaf position is committed with the same H0"),
    ("write_leaf(iv, &fixed_key_leaf, tree + pos , leaves",
     "verifier: same H0 for every leaf position"),
]
HASH_NEEDLES = [
    "\t\thash_update(&hasher, &keys[i], sizeof(keys[i]));\n"
    "\t\thash_update(&hasher, &ivs[i], sizeof(ivs[i]));\n"
    "\t\thash_update_byte(&hasher, 0);\n",
]
SPEC_TREE = [
    ("(K2a+1,K2a+2):=PRG(Ka∥ivvole;2λ)", "Algorithm 4 line 15"),
    ("(K2p+1,K2p+2):=PRG(Kp,ivvole;2λ)", "Algorithm 14 line 27"),
]
SPEC_LEAF = [
    ("(sdi,j,leafcomi,j):=H0(KL−1+t∥ivvole)", "Algorithms 4 line 20 and 14 line 36"),
    ("H0:{0,1}λ+128→{0,1}λ×{0,1}2λ", "Section 2.3.4 domain of H0"),
]


def line_of(text: str, needle: str) -> int:
    return text[: text.index(needle)].count("\n") + 1


def static_certificate(mode: str) -> None:
    for tree in TREES:
        digests = set()
        for prof in PROFILES:
            d = IMPL / tree / f"GreatWall{prof}"
            vc = (d / "vector_com.c").read_text()
            digests.add(hashlib.sha256(vc.encode()).hexdigest())
            cfg = (d / "config.h").read_text()
            for macro in ("TREE_PRG_AES_CTR", "LEAF_PRG_SHAKE"):
                if f"#define {macro}" not in cfg:
                    fail(f"{tree}/{prof}: config.h lacks {macro}")
            thr = re.search(r"#if (\d+) == -1", cfg)
            if not thr:
                fail(f"{tree}/{prof}: batch vector commitment not selected")
            for needle, _ in TREE_NEEDLES + LEAF_NEEDLES:
                if vc.count(needle) < 1:
                    fail(f"{tree}/{prof}: missing source line {needle!r}")
            hh = (d / "hash.h").read_text()
            for needle in HASH_NEEDLES:
                if needle not in hh:
                    fail(f"{tree}/{prof}: shake_prg absorbs more than key||iv||0")
        if len(digests) != 1:
            fail(f"{tree}: vector_com.c differs between profiles")
    ref = IMPL / TREES[0] / "GreatWall128s"
    vc = (ref / "vector_com.c").read_text()
    hh = (ref / "hash.h").read_text()
    needles = TREE_NEEDLES if mode == "tree" else LEAF_NEEDLES
    for needle, what in needles:
        print(f"  source  vector_com.c:{line_of(vc, needle)}: {what}")
    if mode == "leaf":
        print(f"  source  hash.h:{line_of(hh, HASH_NEEDLES[0])}-"
              f"{line_of(hh, HASH_NEEDLES[0]) + 2}: H0 = SHAKE(K || iv || 0x00), no index")
    print("  source  vector_com.c is byte-identical across all 8 profiles in both trees "
          "(16 files checked)")

    pdftotext = shutil.which("pdftotext")
    if not pdftotext:
        print("  spec    SKIP: pdftotext unavailable; Algorithms 4 and 14 not re-read")
        return
    out = subprocess.run([pdftotext, "-f", "17", "-l", "27", str(CAND / "sign-13-spec.pdf"), "-"],
                         capture_output=True, text=True)
    flat = re.sub(r"\s+", "", out.stdout)
    for needle, where in (SPEC_TREE if mode == "tree" else SPEC_LEAF):
        if needle not in flat:
            fail(f"specification text {needle!r} ({where}) not found")
        print(f"  spec    {where}: {needle}")


# --------------------------------------------------------------------------
# 2. Real signatures from lib/*.so
# --------------------------------------------------------------------------

def profile_params(prof: str) -> dict:
    cfg = (IMPL / TREES[0] / f"GreatWall{prof}" / "config.h").read_text()
    get = lambda name: int(re.search(rf"#define {name} (\d+)", cfg).group(1))
    lam = get("SECURITY_PARAM")
    tau = get("BITS_PER_WITNESS")
    w = get("ZERO_BITS_IN_CHALLENGE_3")
    t_open = int(re.search(r"#if (\d+) == -1", cfg).group(1))
    nch = lam - w
    return {"lam": lam, "tau": tau, "w": w, "t_open": t_open, "nch": nch,
            "kmax": -(-nch // tau), "kmin": nch // tau, "nmax": nch % tau}


def count_revealed(p: dict, chall3: bytes) -> tuple[int, list[int]]:
    """Recompute the opened-node count from chall3 (Algorithms 12/14)."""
    tau, kmax, kmin, nmax = p["tau"], p["kmax"], p["kmin"], p["nmax"]
    nmin = tau - nmax
    short = 1 << kmin
    leaves = (nmin << kmin) + (nmax << kmax)
    nodes = 2 * leaves - 1
    bits = [(chall3[i // 8] >> (i % 8)) & 1 for i in range(p["lam"])]
    hidden = bytearray(nodes)
    off = 0
    positions = []
    for v in range(tau):
        depth = kmax if v < nmax else kmin
        idx = sum(bits[off + s] << s for s in range(depth))
        off += depth
        if idx < short:
            pos = leaves - 1 + tau * idx + v
        else:
            pos = leaves - 1 + nmin * short + nmax * idx + v
        hidden[pos] = 1
        positions.append(pos)
    revealed = 0
    for i in range(leaves - 2, -1, -1):
        hidden[i] = hidden[2 * i + 1] | hidden[2 * i + 2]
        if hidden[2 * i + 1] ^ hidden[2 * i + 2]:
            revealed += 1
    return revealed, positions


def sample_profile(args: tuple[str, int, int]) -> dict:
    prof, count, seed = args
    p = profile_params(prof)
    lib = ctypes.CDLL(str(CAND / "lib" / f"libGreatWall{prof}.so"))
    for name in ("sig_get_pk_len_bytes", "sig_get_sk_len_bytes", "sig_get_sn_len_bytes"):
        getattr(lib, name).restype = ctypes.c_ulonglong
    pkl, skl, snl = (lib.sig_get_pk_len_bytes(), lib.sig_get_sk_len_bytes(),
                     lib.sig_get_sn_len_bytes())
    lam, tau, t_open = p["lam"], p["tau"], p["t_open"]
    lb = lam // 8
    ell = 8 * pkl                         # witness bits: (pt, x1), each ceil(n/8) bytes
    ellhat = ell + 2 * lam + B_GRIND
    off_open = (tau - 1) * ellhat // 8 + (lam + B_GRIND) // 8 + ell // 8 + lb
    open_len = (2 * tau + t_open) * lb
    layout = off_open + open_len + lb + 16 + 4
    if layout != snl:
        return {"prof": prof, "error": f"layout {layout} != CRYPTO_BYTES {snl}"}
    pk = (ctypes.c_ubyte * pkl)()
    sk = (ctypes.c_ubyte * skl)()
    a, b = ctypes.c_ulonglong(), ctypes.c_ulonglong()
    if lib.sig_keygen(pk, ctypes.byref(a), sk, ctypes.byref(b)) != 0:
        return {"prof": prof, "error": "keygen failed"}
    rng = random.Random(seed)
    counts, leaf_counts = [], []
    controls = []
    for k in range(count):
        msg = rng.randbytes(33)
        sig = (ctypes.c_ubyte * snl)()
        sl = ctypes.c_ulonglong()
        if lib.sig_sign(sk, skl, msg, len(msg), sig, ctypes.byref(sl)) != 0 or sl.value != snl:
            return {"prof": prof, "error": "sign failed"}
        s = bytes(sig)
        opening = s[off_open: off_open + open_len]
        chall3 = s[off_open + open_len: off_open + open_len + lb]
        if p["w"] and int.from_bytes(chall3, "little") >> (lam - p["w"]):
            return {"prof": prof, "error": "grinding bits not zero"}
        leafcoms = [opening[2 * lb * v: 2 * lb * (v + 1)] for v in range(tau)]
        seeds = [opening[2 * lb * tau + lb * j: 2 * lb * tau + lb * (j + 1)] for j in range(t_open)]
        nonzero = sum(1 for x in seeds if any(x))
        exact, _ = count_revealed(p, chall3)
        if exact != nonzero or exact > t_open or any(any(x) for x in seeds[exact:]):
            return {"prof": prof, "error": f"opening parse mismatch ({exact} vs {nonzero})"}
        if sum(1 for x in leafcoms if any(x)) != tau or len(set(leafcoms)) != tau:
            return {"prof": prof, "error": "leaf commitments not tau distinct values"}
        counts.append(exact)
        leaf_counts.append(tau)
        if k == 0:
            ok = lib.sig_verify(pk, pkl, sig, snl, msg, len(msg))
            bad = bytearray(s)
            bad[off_open + 2 * lb * tau] ^= 1          # first opened seed
            bad_buf = (ctypes.c_ubyte * snl).from_buffer(bad)
            rej = lib.sig_verify(pk, pkl, bad_buf, snl, msg, len(msg))
            bad2 = bytearray(s)
            bad2[off_open] ^= 1                       # first hidden-leaf commitment
            bad2_buf = (ctypes.c_ubyte * snl).from_buffer(bad2)
            rej2 = lib.sig_verify(pk, pkl, bad2_buf, snl, msg, len(msg))
            controls = [ok, rej, rej2]
    return {"prof": prof, "p": p, "counts": counts, "controls": controls, "snl": snl}


def real_signatures(mode: str) -> dict:
    if mode == "tree":
        n_small, n_512 = (100, 20) if LONG else (12, 2)
    else:
        n_small, n_512 = (4, 1) if LONG else (1, 1)
    jobs = [(prof, n_512 if prof.startswith("512") else n_small, 20261017 + i)
            for i, prof in enumerate(PROFILES)]
    missing = [j[0] for j in jobs if not (CAND / "lib" / f"libGreatWall{j[0]}.so").is_file()]
    if missing:
        print(f"SKIP {ID}: missing sign-13/lib/libGreatWall{missing[0]}.so (run: make -C sign-13)")
        sys.exit(77)
    with ProcessPoolExecutor(max_workers=len(jobs)) as ex:
        results = {r["prof"]: r for r in ex.map(sample_profile, jobs)}
    for prof in PROFILES:
        if "error" in results[prof]:
            fail(f"GreatWall{prof}: {results[prof]['error']}")
    return results


def report_costs(mode: str, results: dict) -> list[str]:
    below = []
    print(f"  {'profile':9} {'lambda':>6} {'tau':>4} {'T_open':>6} {'sigs':>5} "
          f"{'T median':>8} {'T range':>9} {'cost (log2)':>11}  vs claim")
    for prof in PROFILES:
        r = results[prof]
        p, counts = r["p"], r["counts"]
        ok, rej, rej2 = r["controls"]
        if ok != 0 or rej == 0 or rej2 == 0:
            fail(f"GreatWall{prof}: controls failed (honest={ok}, seed flip={rej}, leafcom flip={rej2})")
        lam, tau = p["lam"], p["tau"]
        med = statistics.median(counts)
        if mode == "tree":
            cost = lam - math.log2(med + 1)
            if abs(cost - KAT_EXPONENT[prof]) > 0.25:
                fail(f"GreatWall{prof}: sampled exponent {cost:.2f} far from KAT value "
                     f"{KAT_EXPONENT[prof]}")
            rng_s = f"{min(counts)}-{max(counts)}"
        else:
            cost = lam - math.log2(tau)
            rng_s = f"{tau}"
            med = tau
        claim = CLAIM[lam]
        status = "BELOW claim" if cost < claim else "meets claim"
        if lam == 192:
            status += f", above the {NGCC_FLOOR}-bit NGCC floor" if cost >= NGCC_FLOOR else ", BELOW floor"
        if cost < claim:
            below.append(prof)
        print(f"  {prof:9} {lam:6} {tau:4} {p['t_open']:6} {len(counts):5} "
              f"{med:8g} {rng_s:>9} {cost:11.2f}  {status}")
    print("  controls: honest signature verifies and a one-bit flip of the first opened seed "
          "and of the first leaf commitment rejects, for all 8 profiles")
    return below


# --------------------------------------------------------------------------
# 3. Reduced-width recovery with the submitted code
# --------------------------------------------------------------------------

COMMON_SRCS = ["aes.c", "aes_impl.c", "block.c", "block_impl.c", "hash.c", "KeccakHash.c",
               "KeccakP-1600-reference.c", "KeccakSponge.c", "prgs.c", "util.c", "small_vole.c"]
CFLAGS = ["-O2", "-std=gnu11", "-D_GNU_SOURCE", "-fwrapv", "-fno-strict-aliasing", "-w",
          "-Wno-error=implicit-function-declaration", "-Wno-error=incompatible-pointer-types",
          "-Wno-error=int-conversion", "-Wno-error=implicit-int"]
PATCH_OLD = "\tmemcpy(&keys[0], input, n * sizeof(block_secpar));\n"
PATCH_NEW = PATCH_OLD + "\tgw_trunc_keys(&keys[0], n);\n"


def build_witness(work: Path, prof: str) -> Path:
    src = work / prof
    shutil.copytree(IMPL / TREES[0] / f"GreatWall{prof}", src, symlinks=False)
    vc = (src / "vector_com.c").read_text()
    if vc.count(PATCH_OLD) != 1:
        fail("could not locate the PRG key load in vector_com.c")
    (src / "vector_com.c").write_text(vc.replace(PATCH_OLD, PATCH_NEW))
    shutil.copyfile(CAND / "reproduce_ggm_multitarget.c", src / "witness.c")
    exe = src / "witness"
    r = subprocess.run(["gcc", *CFLAGS, "-I.", "witness.c", *COMMON_SRCS, "-o", "witness"],
                       cwd=src, capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stderr[-2000:])
        fail(f"witness build failed for GreatWall{prof}")
    return exe


def run_recovery(mode: str, exe: Path, prof: str, bits: int, trials: int, singles: int) -> dict:
    r = subprocess.run([str(exe), mode, str(bits), str(trials), str(singles), "20261017"],
                       capture_output=True, text=True)
    if r.returncode != 0:
        fail(f"witness failed: {r.stderr.strip()}")
    rows = []
    for line in r.stdout.splitlines():
        if line.startswith("TRIAL"):
            rows.append(dict(kv.split("=") for kv in line.split()[2:]))
    if len(rows) != trials:
        fail("witness output truncated")
    space = 1 << bits
    multi_norm, single_norm, multi, single, targets = [], [], [], [], []
    for k, row in enumerate(rows):
        t = int(row["targets"])
        m = int(row["multi"])
        targets.append(t)
        if m == 0 or row["truth"] != "1":
            fail(f"GreatWall{prof} trial {k}: no verified recovery ({row})")
        if mode == "tree" and int(row["leaves_confirmed"]) < 1:
            fail(f"GreatWall{prof} trial {k}: recovered parent not confirmed by a leaf commitment")
        multi.append(m)
        multi_norm.append(m * (t + 1) / space)
        if k < singles:
            s = int(row["single"])
            if s == 0:
                fail(f"GreatWall{prof} trial {k}: single-target control found nothing")
            single.append(s)
            single_norm.append(s * 2 / space)
    if rows[0]["wrong_iv"] != "none":
        fail(f"GreatWall{prof}: wrong-IV control produced a recovery")
    return {"multi": multi, "single": single, "targets": targets,
            "multi_norm": statistics.mean(multi_norm), "single_norm": statistics.mean(single_norm),
            "space": space}


def reduced_recovery(mode: str) -> list[str]:
    if not shutil.which("gcc"):
        print(f"SKIP {ID}: gcc unavailable for the reduced-width witness")
        sys.exit(77)
    if LONG:
        plan = [("128s", 22, 64, 16), ("128f", 22, 64, 16), ("192f", 20, 32, 8), ("256f", 20, 32, 8)]
    else:
        plan = [("128s", 20, 32, 8)]
    lines = []
    work = Path(tempfile.mkdtemp(prefix="tmp.gw."))
    try:
        for prof, bits, trials, singles in plan:
            exe = build_witness(work, prof)
            t0 = time.time()
            res = run_recovery(mode, exe, prof, bits, trials, singles)
            dt = time.time() - t0
            tbar = statistics.mean(res["targets"])
            speed = statistics.mean(res["single"]) / statistics.mean(res["multi"])
            what = "opened internal-node seeds" if mode == "tree" else "hidden-leaf commitments"
            print(f"  GreatWall{prof}, B={bits}: {trials}/{trials} openings, every one recovered "
                  f"by searching all {tbar:.1f} {what} at once")
            print(f"    mean guesses multi-target {statistics.mean(res['multi']):.0f}, "
                  f"model 2^B/(T+1) = {res['space'] / (tbar + 1):.0f} "
                  f"(normalized {res['multi_norm']:.2f})")
            print(f"    control single target ({len(res['single'])} trials): mean "
                  f"{statistics.mean(res['single']):.0f}, model 2^B/2 = {res['space'] // 2} "
                  f"(normalized {res['single_norm']:.2f}); speedup {speed:.1f}x, "
                  f"model {(tbar + 1) / 2:.1f}x")
            print(f"    control wrong IV: 0 recoveries in 2^{bits} guesses; ground truth matched "
                  f"in every trial; {dt:.1f}s")
            if not (0.5 < res["multi_norm"] < 2.0 and 0.35 < res["single_norm"] < 2.5):
                fail(f"GreatWall{prof}: search cost does not follow the multi-target model")
            if speed < (tbar + 1) / 6:
                fail(f"GreatWall{prof}: multi-target speedup {speed:.1f}x too small")
            lines.append(f"{prof} B={bits} {trials}/{trials} recovered, speedup {speed:.0f}x")
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return lines


def main() -> int:
    global ID
    mode = sys.argv[1] if len(sys.argv) > 1 else "tree"
    if mode not in ("tree", "leaf"):
        print("usage: reproduce_ggm_multitarget.py tree|leaf")
        return 2
    ID = "sign-13-3" if mode == "tree" else "sign-13-4"
    t0 = time.time()
    print(f"== {ID}: static certificate")
    static_certificate(mode)
    print(f"== {ID}: signatures from sign-13/lib/*.so")
    results = real_signatures(mode)
    below = report_costs(mode, results)
    print(f"== {ID}: reduced-width seed recovery (submitted code, truncated PRG keys)")
    rec = reduced_recovery(mode)
    route = ("unindexed internal GGM nodes: one PRG(K||iv) map for every node, "
             "searched against all opened seeds of a signature")
    if mode == "leaf":
        route = ("unindexed leaf commitments H0(K||iv), searched against all tau "
                 "published hidden-leaf commitments of a signature")
    if len(below) != len(PROFILES):
        fail(f"only {below} fall below their claimed level")
    print(f"LIMITATION {ID}: no full-width search or end-to-end forgery; recovery runs at "
          f"reduced seed width only; 192s/192f stay above the 128-bit NGCC floor")
    print(f"CONFIRMED {ID}: {route}; all 8 profiles below their Table 3 claim; "
          f"{'; '.join(rec)} ({time.time() - t0:.0f}s)")
    return 0


ID = "sign-13-3"
if __name__ == "__main__":
    sys.exit(main())
