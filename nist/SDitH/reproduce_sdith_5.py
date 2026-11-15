#!/usr/bin/env python3
"""Reproduce SDitH-5: five places where the SDitH v3.0 code and KATs depart
from the literal specification (interoperability only).

Checks (one per item of section C, H1-H5, of the 2026-11-15 review):
  H1  Algorithm 1 l.10-11 witness encoding (drop the last coordinate) against
      the mux convention the code consumes: keys re-encoded per Algorithm 1
      sign but do not verify (cat1_fast, cat3_fast, cat5_fast).
  H2  Delta_inv = psi(istar) printed without phi_Gray in Algorithms 11/12 l.4:
      a build of cat1_fast with the Gray map removed from
      delta1_from_delta0_ref (temporary copy) rejects its own signatures.
  H3  ExpandH as one sequential PRG stream against the code's per-column CTR
      block offsets: H' columns of the cat1_fast code are recomputed with a
      pure-Python AES-128 in counter mode (Algorithm 6).
  H4  ExpandEvaluationChallenge bit layout: the spec layout applied to code
      signatures (cat1_fast, hash grinding) gives vpow = 0 only at rate
      ~2^-w; the code layout holds for every signature. Cipher-grinding
      prefixes 5/6 and the counter-free first XOF are checked in the source.
  H5  Signature field order of Algorithm 3 l.9: hpiop is not after the salt
      but in the tail of the code's signature.

Each check cites the specification page it compares against (displayed page
number; the PDF page is two higher) and verifies the cited text on that PDF
page with pdftotext. Only the submitted Reference_Implementation sources are
compiled, in a temporary directory; no submission file in the repository is
modified. No forgery or security impact is claimed.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import subprocess
import tempfile
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path


HERE = Path(__file__).resolve().parent
SPEC = HERE / "SDitH-spec.pdf"
PKG = HERE / "submission/submission_package"
REF = PKG / "Reference_Implementation"

H1_SETS = ("cat1_fast", "cat3_fast", "cat5_fast")
H1_SEEDS = tuple(range(201, 209))      # 8 keys per category
H2_SEEDS = (301, 302, 303)
H3_SEED = 401
H4_SEED = 501
H4_SIGS = 400
PAGE_OFFSET = 2                        # PDF page = displayed page + 2

SPEC_SHA256 = "22dfed06d179afa759139487dca9c8f8d4a7cfea22bcedcb7736b50ce5b01c83"
SOURCE_SHA256 = {
    "Reference_Implementation/*/src/rsd.c": "82241402285599d2ad7e516819e6c7eca5a631b90dfd6bf455c8a22d2ea56ceb",
    "Reference_Implementation/*/src/piop_circuit.c": "7c3ede763f8556b7c9eedb1ea7908867945227a1fb1c2ad1a00c61468009c729",
    "Reference_Implementation/*/src/vole_expansion.c": "b863e22da0eade81b8184425d9b658fe4afc6dd890766a184ce2d7dfdc33b1f3",
    "Reference_Implementation/*/src/sdith_prng.c": "aa8b9d804759c70cf2f26391daa02b6a97e8c235b81af208cd6c86e473758322",
    "Reference_Implementation/*/src/sdith_signature.c": "99685ad55272b44d5cc6f7ae20b4822d168a1bf75a8f9fcbb2e0e1f2d8dd7aec",
}

# (displayed page, text that must occur on PDF page displayed+2, whitespace-normalised)
SPEC_ANCHORS = {
    "H1_alg1": (11, "wit ← wit ∥ (e0 , . . . , eµj −2 )"),
    "H1_mux": (34, "Muxµj (b, u) = u0 + X bj−1 · (uj − u0 )"),
    "H2_alg11": (29, "Algorithm 11 BLC.OpenRandomEvaluation"),
    "H2_alg11_l4": (29, "∆inv ← ψ([i∗ [0], . . . , i∗ [τ − 1]])"),
    "H2_alg12_l4": (30, "∆inv ← ψ([i∗ [0], . . . , i∗ [τ − 1]])"),
    "H2_alg12_l14": (30, "ϕGray (i∗ [e])"),
    "H3_prg_alg6": (17, "Enc(key := seed, ptx := twk salt ⊕ LittleEndianλ (i))"),
    "H3_expandh": (18, "colsH ′ [i] ← SampleFieldElements(prg, Fq , n − k)"),
    "H3_sample_f2": (18, "The byte Bi is interpreted as 8 field elements"),
    "H4_xof": (19, "xof ← XOF.Init(hpiop , ctr)"),
    "H4_istar": (19, "{i∗ [e]}e<τ ← SampleIntegers(xof, {0, . . . , 2κ − 1}, τ )"),
    "H4_vpow": (19, "vpow ← SampleFieldElements(xof, F2 , wpow )"),
    "H4_sampleintegers": (18, "v ← B0 + 256 · B1 + 2562 · B2 + 2563 · B3"),
    "H5_alg3_l9": (13, "σ = Serialize (salt ∥ hpiop ∥ aux ∥ pdecom ∥ (α2 , . . . , αd ))"),
}

LIB_SRC = (
    "src/commons.c src/sdith_prng.c src/gf256.c src/gf192.c src/ct_utils.c "
    "src/matrix_vector_products_f2.c src/matrix_transpose_f2.c src/ggm.c "
    "src/piop_circuit.c src/gf128.c src/rsd.c "
    "src/sdith_signature.c src/sdith_signature_parameters.c src/vole_to_piop.c "
    "src/vole_parameters.c generator/sign.c "
    "lib/aes/rijndael256.c lib/aes/aes_ansi_littleendian_ref.c "
    "lib/aes/aes128_ctrle_special_ref.c lib/aes/rijndael256_ctrle_special_ref.c "
    "lib/sha3/KeccakHash.c lib/sha3/KeccakSponge.c lib/sha3/KeccakSpongetimes4.c "
    "lib/sha3/KeccakHashtimes4.c lib/sha3/opt64/KeccakP-1600-opt64.c "
    "lib/sha3/opt64/KeccakP-1600-times4-on1.c"
).split()

NOGRAY_OLD = "xorto_kappabit_uint(kappa, i * kappa, delta1, pos ^ (pos >> 1));"
NOGRAY_NEW = "xorto_kappabit_uint(kappa, i * kappa, delta1, pos); /* REPRO: literal psi(istar), no phi_Gray */"

DRIVER = r"""
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "api.h"
#include "sdith_signature.h"
#include "vole_private.h"
#include "sdith_rsd.h"

static uint64_t rs;
static uint64_t sm64(void) {
  uint64_t z = (rs += UINT64_C(0x9e3779b97f4a7c15));
  z = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
  z = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
  return z ^ (z >> 31);
}
int randombytes(unsigned char* x, unsigned long long n) { while (n--) *x++ = (unsigned char)sm64(); return 0; }

static const signature_parameters* P = &SIGNATURE_PARAMS;
static unsigned char pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
static const unsigned char msg[32] = "SDitH SDitH-5 reproduction..";
static uint64_t mux_inputs(void) { uint64_t s = 0; for (uint64_t j = 0; j < P->mux_depth; j++) s += P->mux_arities[j] - 1; return s; }
static uint8_t* wit(unsigned char* s) { return s + CRYPTO_PUBLICKEYBYTES; }   /* sk = seedpk || y || wit */
static int getbit(const uint8_t* b, uint64_t i) { return (b[i >> 3] >> (i & 7)) & 1; }
static void setbit(uint8_t* b, uint64_t i, int v) { if (v) b[i >> 3] |= 1 << (i & 7); else b[i >> 3] &= ~(1 << (i & 7)); }
/* code / mux convention: bit k set iff p == k+1 (p == 0 is all-zero);
 * Algorithm 1 l.10-11: e = ElementaryVector(mu, p), keep e_0..e_{mu-2}: bit k set iff p == k */
static void enc_digit(uint8_t* w, uint64_t bp, uint64_t mu, uint64_t p, int alg1) {
  for (uint64_t k = 0; k + 1 < mu; k++) setbit(w, bp + k, alg1 ? (p == k) : (p == k + 1));
}
static int dec_chunk(const uint8_t* w, uint64_t i, uint64_t* d) {
  uint64_t bp = i * mux_inputs();
  for (uint64_t j = 0; j < P->mux_depth; j++) {
    uint64_t mu = P->mux_arities[j], cnt = 0, p = 0;
    for (uint64_t k = 0; k + 1 < mu; k++) if (getbit(w, bp + k)) { cnt++; p = k + 1; }
    if (cnt > 1) return -1;
    d[j] = p; bp += mu - 1;
  }
  return 0;
}
static void enc_chunk(uint8_t* w, uint64_t i, const uint64_t* d, int alg1) {
  uint64_t bp = i * mux_inputs();
  for (uint64_t j = 0; j < P->mux_depth; j++) { enc_digit(w, bp, P->mux_arities[j], d[j], alg1); bp += P->mux_arities[j] - 1; }
}
static int sign_verify(const unsigned char* skx) {
  unsigned char* sm = malloc(CRYPTO_BYTES + sizeof msg); unsigned char* m2 = malloc(CRYPTO_BYTES + sizeof msg);
  unsigned long long smlen = 0, m2len = 0;
  crypto_sign(sm, &smlen, msg, sizeof msg, skx);
  int r = crypto_sign_open(m2, &m2len, sm, smlen, pk);
  free(sm); free(m2);
  return r == 0;
}
static void dump(const char* fn, const void* p, size_t n) {
  FILE* f = fopen(fn, "wb"); if (!f || fwrite(p, 1, n, f) != n) { perror(fn); exit(3); } fclose(f);
}

int main(int argc, char** argv) {
  if (argc < 3) return 2;
  const char* mode = argv[1];
  rs = strtoull(argv[2], 0, 0);
  if (crypto_sign_keypair(pk, sk)) return 4;
  if (!strcmp(mode, "witenc")) {
    uint64_t w = P->rsd_w, d[8]; unsigned char sk2[sizeof sk];
    printf("control_accept %d\n", sign_verify(sk));
    memcpy(sk2, sk, sizeof sk);
    for (uint64_t i = 0; i < w; i++) { if (dec_chunk(wit(sk), i, d)) { puts("decode_fail"); return 5; } enc_chunk(wit(sk2), i, d, 0); }
    printf("code_reencode_identical %d\n", memcmp(sk, sk2, sizeof sk) == 0);
    memcpy(sk2, sk, sizeof sk);
    for (uint64_t i = 0; i < w; i++) { dec_chunk(wit(sk), i, d); enc_chunk(wit(sk2), i, d, 1); }
    printf("alg1_differs %d\n", memcmp(sk, sk2, sizeof sk) != 0);
    printf("alg1_accept %d\n", sign_verify(sk2));
    return 0;
  }
  if (!strcmp(mode, "selfverify")) {           /* honest sign + verify in this build */
    printf("accept %d\n", sign_verify(sk));
    return 0;
  }
  if (!strcmp(mode, "dumph")) {                 /* public key and the code's H' (public) */
    extended_parameters_t par; compute_extended_parameters(&par, P);
    uint64_t nmk = par.rsd_n - par.rsd_codim, slice = par.rsd_codim_slice, cb = (par.rsd_codim + 7) / 8;
    uint8_t* h = aligned_alloc(32, ((nmk * slice + 31) / 32) * 32);
    rsd_expand_public_key_ref(par.rsd_w, par.rsd_n, par.rsd_codim, &par.vole_params, (bitmat_t*)h, slice, (const seed_t*)pk);
    uint8_t* hc = malloc(nmk * cb);
    for (uint64_t i = 0; i < nmk; i++) memcpy(hc + i * cb, h + i * slice, cb);
    char fn[4096];
    snprintf(fn, sizeof fn, "%s/pk.bin", argv[3]); dump(fn, pk, sizeof pk);
    snprintf(fn, sizeof fn, "%s/H.bin", argv[3]); dump(fn, hc, nmk * cb);
    printf("lambda %lu n %lu codim %lu k %lu\n", (unsigned long)par.lambda, (unsigned long)par.rsd_n,
           (unsigned long)par.rsd_codim, (unsigned long)nmk);
    return 0;
  }
  if (!strcmp(mode, "sigs")) {                  /* N honest signatures, raw */
    int n = atoi(argv[3]), bad = 0;
    FILE* f = fopen(argv[4], "wb"); if (!f) return 3;
    unsigned char* sm = malloc(CRYPTO_BYTES + sizeof msg); unsigned char* m2 = malloc(CRYPTO_BYTES + sizeof msg);
    unsigned long long smlen, m2len;
    for (int i = 0; i < n; i++) {
      crypto_sign(sm, &smlen, msg, sizeof msg, sk);
      if (crypto_sign_open(m2, &m2len, sm, smlen, pk)) bad++;
      fwrite(sm + sizeof msg, 1, CRYPTO_BYTES, f);
    }
    fclose(f);
    printf("sigbytes %d lambda %lu kappa %lu tau %lu w %lu variant %d failed %d\n", CRYPTO_BYTES,
           (unsigned long)P->lambda, (unsigned long)P->kappa, (unsigned long)P->tau,
           (unsigned long)P->proofow_w, (int)P->proofow_variant, bad);
    return 0;
  }
  return 2;
}
"""


# ---------------------------------------------------------------- AES-128 (FIPS 197), pure Python
def _aes_tables():
    sbox = [0] * 256
    p = q = 1
    while True:
        p = p ^ ((p << 1) & 0xFF) ^ (0x1B if p & 0x80 else 0)
        q ^= q << 1
        q ^= q << 2
        q ^= q << 4
        q &= 0xFF
        if q & 0x80:
            q ^= 0x09
        x = q ^ (((q << 1) | (q >> 7)) & 0xFF) ^ (((q << 2) | (q >> 6)) & 0xFF) \
            ^ (((q << 3) | (q >> 5)) & 0xFF) ^ (((q << 4) | (q >> 4)) & 0xFF)
        sbox[p] = x ^ 0x63
        if p == 1:
            break
    sbox[0] = 0x63
    return sbox


SBOX = _aes_tables()


def _xt(a):
    return ((a << 1) ^ 0x1B) & 0xFF if a & 0x80 else a << 1


def aes128_expand(key: bytes):
    w = [list(key[4 * i:4 * i + 4]) for i in range(4)]
    rcon = 1
    for i in range(4, 44):
        t = list(w[i - 1])
        if i % 4 == 0:
            t = [SBOX[b] for b in t[1:] + t[:1]]
            t[0] ^= rcon
            rcon = _xt(rcon)
        w.append([a ^ b for a, b in zip(w[i - 4], t)])
    return [sum(w[4 * r:4 * r + 4], []) for r in range(11)]


def aes128_encrypt(rk, block: bytes) -> bytes:
    s = [a ^ b for a, b in zip(block, rk[0])]
    for r in range(1, 11):
        s = [SBOX[b] for b in s]
        s = [s[(i + 4 * (i % 4)) % 16] for i in range(16)]       # ShiftRows (column-major state)
        if r != 10:
            t = []
            for c in range(4):
                a = s[4 * c:4 * c + 4]
                x = a[0] ^ a[1] ^ a[2] ^ a[3]
                t += [a[i] ^ x ^ _xt(a[i] ^ a[(i + 1) % 4]) for i in range(4)]
            s = t
        s = [a ^ b for a, b in zip(s, rk[r])]
    return bytes(s)


# ---------------------------------------------------------------- helpers
def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(cmd, timeout=600, **kw):
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout, **kw)
    if r.returncode:
        raise RuntimeError(f"command failed ({r.returncode}): {' '.join(map(str, cmd))}\n{r.stdout}{r.stderr}")
    return r


def kv(text: str) -> dict:
    t = text.split()
    return {t[i]: int(t[i + 1]) for i in range(0, len(t) - 1, 2)}


def emit(result: dict, output: Path | None) -> None:
    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if output:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(encoded)
    print(encoded, end="")


def build(gcc: str, work: Path, name: str, nogray: bool = False) -> dict:
    """Compile one reference set; returns {'normal': exe, 'nogray': exe (optional)}."""
    src = work / name
    shutil.copytree(REF / name, src)
    (src / "driver.c").write_text(DRIVER)
    inc = [f"-I{src}/src", f"-I{src}/generator", f"-I{src}/lib/aes", f"-I{src}/lib/sha3",
           f"-I{src}/lib/sha3/opt64"]
    flags = ["-O2", "-std=gnu11", "-DNDEBUG", "-DONLY_REF_IMPLEMENTATION", *inc]
    jobs, objs = [], []
    for rel in LIB_SRC + ["driver.c", "src/vole_expansion.c"]:
        obj = src / (rel.replace("/", "_") + ".o")
        jobs.append([gcc, *flags, "-c", str(src / rel), "-o", str(obj)])
        objs.append(obj)
    if nogray:
        vole = src / "src/vole_expansion.c"
        text = vole.read_text()
        if text.count(NOGRAY_OLD) != 1:
            raise RuntimeError("Gray-map anchor not unique in vole_expansion.c")
        patched = src / "src/vole_expansion_nogray.c"
        patched.write_text(text.replace(NOGRAY_OLD, NOGRAY_NEW))
        ng_obj = src / "vole_expansion_nogray.o"
        jobs.append([gcc, *flags, "-c", str(patched), "-o", str(ng_obj)])
    with ThreadPoolExecutor(max_workers=3) as ex:
        list(ex.map(run, jobs))
    out = {"normal": work / f"drv_{name}"}
    run([gcc, *map(str, objs), "-o", str(out["normal"]), "-lm"])
    if nogray:
        out["nogray"] = work / f"drv_{name}_nogray"
        run([gcc, *map(str, objs[:-1]), str(ng_obj), "-o", str(out["nogray"]), "-lm"])
    return out


def spec_pages(pdftotext: str) -> dict:
    pages = {}
    for key, (disp, anchor) in SPEC_ANCHORS.items():
        pdf = disp + PAGE_OFFSET
        if pdf not in pages:
            txt = run([pdftotext, "-f", str(pdf), "-l", str(pdf), str(SPEC), "-"]).stdout
            pages[pdf] = " ".join(txt.split())
    return {key: anchor in pages[disp + PAGE_OFFSET] for key, (disp, anchor) in SPEC_ANCHORS.items()}


def bits_of(b: bytes, start: int, n: int) -> int:
    return (int.from_bytes(b, "little") >> start) & ((1 << n) - 1)


# ---------------------------------------------------------------- main
def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    t0 = time.time()
    result = {
        "candidate": "SDitH-5",
        "evidence": ("Interoperability only: five places where the submitted reference code (and so its "
                     "KATs) departs from the literal v3.0 specification text, each tested against the "
                     "cited specification page (H1 Algorithm 1 witness encoding, H2 psi without phi_Gray, "
                     "H3 ExpandH stream layout, H4 ExpandEvaluationChallenge layout, H5 Algorithm 3 field "
                     "order); no forgery, key recovery or other security impact is shown."),
    }
    gcc = shutil.which("gcc", path="/usr/bin") or shutil.which("gcc")
    pdftotext = shutil.which("pdftotext")
    if not gcc or not pdftotext:
        result.update(status="SKIP", reason="gcc or pdftotext not found", checks={}, measurements={},
                      source_sha256={}, not_covered=[])
        emit(result, args.output)
        return 77

    source_sha = dict(SOURCE_SHA256)
    source_sha["SDitH-spec.pdf"] = SPEC_SHA256
    pinned_ok = sha(SPEC) == SPEC_SHA256
    for pattern, digest in SOURCE_SHA256.items():
        files = sorted(PKG.glob(pattern))
        pinned_ok &= len(files) == 12 and all(sha(p) == digest for p in files)

    anchors = spec_pages(pdftotext)
    checks: dict = {"sources_and_spec_pinned": pinned_ok}
    meas: dict = {}

    sig_c = (REF / "cat1_fast/src/sdith_signature.c").read_text()
    prng_c = (REF / "cat1_fast/src/sdith_prng.c").read_text()
    piop_c = (REF / "cat1_fast/src/piop_circuit.c").read_text()
    rsd_c = (REF / "cat1_fast/src/rsd.c").read_text()

    with tempfile.TemporaryDirectory(prefix="sdith-se3-") as tmp:
        work = Path(tmp)
        exes = {}
        for name in H1_SETS:
            exes[name] = build(gcc, work, name, nogray=(name == "cat1_fast"))

        # ---------------- H1: Algorithm 1 witness encoding vs. mux convention
        h1 = {"control_accept": 0, "code_reencode_identical": 0, "alg1_differs": 0, "alg1_accept": 0}
        n1 = 0
        for name in H1_SETS:
            for seed in H1_SEEDS:
                r = kv(run([str(exes[name]["normal"]), "witenc", str(seed)], timeout=300).stdout)
                n1 += 1
                for k in h1:
                    h1[k] += r[k]
        # the code's one-hot convention (rsd.c) and the mux gate (piop_circuit.c) agree with Sec. 3.2.6
        code_conv = "const uint32_t one_hot = (UINT32_C(1) << sij) >> 1;" in rsd_c
        checks["H1_alg1_witness_encoding_rejected"] = (
            anchors["H1_alg1"] and anchors["H1_mux"] and code_conv
            and h1["control_accept"] == n1 and h1["code_reencode_identical"] == n1
            and h1["alg1_differs"] == n1 and h1["alg1_accept"] == 0)
        meas["H1"] = {"spec_displayed_pages": [11, 34], "spec_pdf_pages": [13, 36],
                      "spec_text": "Alg.1 l.10-11 keeps e_0..e_{mu-2}; Sec.3.2.6 Mux(b,u)=u0+sum b_{j-1}(u_j-u0)",
                      "keys": n1, "sets": list(H1_SETS),
                      "unmodified_accepted": h1["control_accept"],
                      "code_convention_reencode_identical": h1["code_reencode_identical"],
                      "alg1_encoding_differs_from_sk": h1["alg1_differs"],
                      "alg1_encoding_accepted": h1["alg1_accept"]}

        # ---------------- H2: psi(istar) without phi_Gray
        normal_ok = nogray_ok = 0
        for seed in H2_SEEDS:
            normal_ok += kv(run([str(exes["cat1_fast"]["normal"]), "selfverify", str(seed)], timeout=300).stdout)["accept"]
            nogray_ok += kv(run([str(exes["cat1_fast"]["nogray"]), "selfverify", str(seed)], timeout=300).stdout)["accept"]
        vole_c = (REF / "cat1_fast/src/vole_expansion.c").read_text()
        checks["H2_psi_without_gray_rejects_own_signatures"] = (
            anchors["H2_alg11"] and anchors["H2_alg11_l4"] and anchors["H2_alg12_l4"] and anchors["H2_alg12_l14"]
            and vole_c.count(NOGRAY_OLD) == 1
            and normal_ok == len(H2_SEEDS) and nogray_ok == 0)
        meas["H2"] = {"spec_displayed_pages": [29, 30], "spec_pdf_pages": [31, 32],
                      "spec_text": "Alg.11 l.4 and Alg.12 l.4: Delta_inv <- psi([i*[0..tau-1]]); Alg.12 l.14 uses phi_Gray(i*[e])",
                      "set": "cat1_fast", "signatures": len(H2_SEEDS),
                      "unmodified_build_accepts": normal_ok, "no_gray_build_accepts": nogray_ok}

        # ---------------- H3: ExpandH layout
        hd = work / "h3"
        hd.mkdir()
        info = kv(run([str(exes["cat1_fast"]["normal"]), "dumph", str(H3_SEED), str(hd)]).stdout)
        lam, codim, kcols = info["lambda"], info["codim"], info["k"]
        pk = (hd / "pk.bin").read_bytes()
        hb = (hd / "H.bin").read_bytes()
        cb = (codim + 7) // 8
        rk = aes128_expand(pk[:lam // 8])          # PRG.Init(seedpk), no salt, no tweak
        cache: dict = {}

        def block(i):
            if i not in cache:
                cache[i] = aes128_encrypt(rk, i.to_bytes(16, "little"))
            return cache[i]

        def stream_bits(start, n):                 # bits [start, start+n) of the PRG output, LSB-first per byte
            b0, b1 = start // 128, (start + n - 1) // 128
            s = b"".join(block(i) for i in range(b0, b1 + 1))
            return bits_of(s, start - 128 * b0, n)

        bpr = (codim + 127) // 128
        colset = list(range(10)) + [kcols // 2, kcols - 1]
        code_layout = seq_layout = 0
        seq_match_cols = []
        for i in colset:
            col = int.from_bytes(hb[i * cb:(i + 1) * cb], "little")
            if col == stream_bits(i * bpr * 128, codim):
                code_layout += 1
            if col == stream_bits(i * codim, codim):
                seq_layout += 1
                seq_match_cols.append(i)
        aes_kat = aes128_encrypt(aes128_expand(bytes.fromhex("000102030405060708090a0b0c0d0e0f")),
                                 bytes.fromhex("00112233445566778899aabbccddeeff")).hex() \
            == "69c4e0d86a7b0430d8cdb78070b4c55a"
        checks["H3_expandh_not_one_sequential_stream"] = (
            anchors["H3_prg_alg6"] and anchors["H3_expandh"] and anchors["H3_sample_f2"] and aes_kat
            and lam == 128 and code_layout == len(colset) and seq_match_cols == [0])
        meas["H3"] = {"spec_displayed_pages": [17, 18], "spec_pdf_pages": [19, 20],
                      "spec_text": "ExpandH: colsH'[i] <- SampleFieldElements(prg, Fq, n-k) from one PRG (Alg.6 CTR) stream",
                      "set": "cat1_fast", "codim_bits": codim, "columns_k": kcols,
                      "blocks_per_column_code": bpr, "columns_tested": colset,
                      "columns_matching_code_layout": code_layout,
                      "columns_matching_sequential_stream": seq_layout,
                      "sequential_matching_column_indices": seq_match_cols}

        # ---------------- H4/H5: challenge layout and signature field order
        sigf = work / "sigs.bin"
        s_info = kv(run([str(exes["cat1_fast"]["normal"]), "sigs", str(H4_SEED), str(H4_SIGS), str(sigf)],
                        timeout=600).stdout)
        raw = sigf.read_bytes()
        S, lam, kap, tau, wpow = (s_info["sigbytes"], s_info["lambda"], s_info["kappa"],
                                  s_info["tau"], s_info["w"])
        hb2 = 2 * lam // 8
        nsig = len(raw) // S
        code_ok = spec_ok = same_e0 = same_idx = tot_idx = h5_spec_pos = 0
        for i in range(nsig):
            s = raw[i * S:(i + 1) * S]
            hp, ctr = s[S - 4 - hb2:S - 4], s[S - 4:]
            out = hashlib.shake_128(hp + ctr).digest(4 * tau + 16)
            code_ok += bits_of(out, kap * tau, wpow) == 0                      # code: vpow = bits [k*tau, k*tau+w)
            spec_ok += bits_of(out[4 * tau:], 0, wpow) == 0                     # spec: after tau 4-byte draws
            spec_idx = [int.from_bytes(out[4 * e:4 * e + 4], "little") % (1 << kap) for e in range(tau)]
            code_idx = [bits_of(out, e * kap, kap) for e in range(tau)]
            same_e0 += spec_idx[0] == code_idx[0]          # identical by construction (bits 0..k-1)
            same_idx += sum(a == b for a, b in zip(spec_idx[1:], code_idx[1:]))
            tot_idx += tau - 1
            # H5: Alg.3 l.9 puts hpiop right after the salt
            hp_spec = s[lam // 8:lam // 8 + hb2]
            out2 = hashlib.shake_128(hp_spec + ctr).digest(4 * tau + 16)
            h5_spec_pos += bits_of(out2, kap * tau, wpow) == 0
        cipher_src = (
            "const uint8_t PROOFOW_H0_PREFIX = 5;" in sig_c and "const uint8_t PROOFOW_H1_PREFIX = 6;" in sig_c
            and prng_c.count("xof_init_and_seed_shake128(&xof, &PROOFOW_H0_PREFIX, 1);") == 1)
        init_fn = prng_c[prng_c.index("EXPORT void proofow_init_cipher_cat1_ref"):]
        init_fn = init_fn[:init_fn.index("\n}\n")]
        counter_free = "ctr" not in init_fn                                     # first XOF(5 || hpiop): no counter
        shake_src = ("xof_init_and_seed_shake128(&ctx, s->h_piop, H_PIOP_BYTES);\n    xof_seed_shake128(&ctx, &ctr, 4);"
                     in prng_c and "extract_kappabit_uint(s->w, s->kappa_tau, s->delta0_and_vgrind)" in prng_c)
        checks["H4_challenge_layout_differs_from_spec"] = (
            anchors["H4_xof"] and anchors["H4_istar"] and anchors["H4_vpow"] and anchors["H4_sampleintegers"]
            and s_info["variant"] == 0 and s_info["failed"] == 0 and nsig == H4_SIGS
            and code_ok == nsig and spec_ok < nsig // 4 and shake_src and cipher_src and counter_free)
        meas["H4"] = {"spec_displayed_pages": [18, 19], "spec_pdf_pages": [20, 21],
                      "spec_text": "ExpandEvaluationChallenge: i*[e] <- SampleIntegers(xof,{0..2^k-1},tau) (4 bytes each), then vpow <- SampleFieldElements(xof,F2,w)",
                      "set": "cat1_fast", "signatures": nsig, "kappa": kap, "tau": tau, "w": wpow,
                      "code_layout_vpow_zero": code_ok, "spec_layout_vpow_zero": spec_ok,
                      "spec_layout_expected": round(nsig / 2 ** wpow, 1),
                      "istar_agreement_e0": same_e0,
                      "istar_agreement_e_ge_1": same_idx, "istar_indices_e_ge_1": tot_idx,
                      "istar_agreement_e_ge_1_expected": round(tot_idx / 2 ** kap, 1),
                      "cipher_variant": "source only: prefixes 5/6 (sdith_signature.c:11-12), "
                                        "first XOF(5 || hpiop) without counter (sdith_prng.c:323-347)"}
        order = re.findall(r"MAP_FIELD\(sign_fields, res\.(\w+),", sig_c)
        code_order = ["global_salt", "ggm_sibling_path", "ggm_hidden_leaf_cmt", "corr_u", "cchk_u",
                      "circuit_in_pub", "circuit_cz_pub", "hash_piop", "proofow_ctr_reveal"]
        checks["H5_signature_field_order_differs_from_alg3"] = (
            anchors["H5_alg3_l9"] and order == code_order
            and code_ok == nsig and h5_spec_pos < nsig // 4)
        meas["H5"] = {"spec_displayed_pages": [13], "spec_pdf_pages": [15],
                      "spec_text": "Alg.3 l.9: Serialize(salt || hpiop || aux || pdecom || (alpha_2..alpha_d))",
                      "code_order": code_order, "signatures": nsig,
                      "grinding_holds_with_hpiop_at_code_tail": code_ok,
                      "grinding_holds_with_hpiop_at_alg3_position": h5_spec_pos}

    meas["spec_anchor_found"] = anchors
    meas["runtime_seconds"] = round(time.time() - t0, 1)
    checks["spec_text_found_on_cited_pages"] = all(anchors.values())
    result.update(checks=checks, measurements=meas, source_sha256=source_sha, not_covered=[],
                  status="PASS" if all(checks.values()) else "FAIL")
    emit(result, args.output)
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
