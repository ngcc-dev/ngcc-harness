# UOV-6 — Low: clang compiles the reference field multiplication into branches on secret multiplier bits

Severity: Low
Status: Confirmed
Layer: Side-channel
Affected: Reference and amd64 trees, all parameter sets, when built with clang at -O2 or -O3 (clang 21 tested; Ip and Is run); gcc builds, clang -O1 and the AVX2 tree are not affected
Discovery: Moderate
Exploitation: Compiled-code and taint witness of branches on secret vinegar and elimination multipliers; no timing measurement or key recovery
Remediation: Local
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-11-15

## Defect

`gf256v_mul_u32` and `gf16v_mul_u32` are branch-free C of the form
`r32 ^= a32 * ((b32 >> k) & 1)`
(`gf16.h:255-285` (`UOV/submission/UOV-R3/Reference_Implementation/Ip/gf16.h#L255`),
`gf16.h:187-197` (`UOV/submission/UOV-R3/Reference_Implementation/Ip/gf16.h#L187`); the amd64
tree's `gf16_u64.h` forms are the same). clang 21 at `-O2` and `-O3` compiles
them into conditional jumps on single bits of the multiplier `b`: bits 0 to 6
of a GF(256) multiplier and 0 to 2 of a GF(16) one, the top bit going through
a vector multiply. During signing `b` is a vinegar element or an elimination
multiplier, both secret.

## Evidence

- Object code of `blas_matrix_ref.c` for Ip: clang `-O2`/`-O3` has 28
  single-bit test-and-branch pairs in the multiplication; gcc `-O2`/`-O3` and
  clang `-O1` have none.
- Taint run of one Ip signature, clang `-O3`: 84 contexts in the
  multiplication, 63 of them with a secret multiplier (`ov.c:138,141,146`).
  They remain when only the 32-byte secret seed is tainted. Is and the amd64
  tree behave the same way.
- Controls: gcc `-O2`/`-O3`, clang `-O1` and the AVX2 tree under clang report
  only the known retry branch ([UOV-1](/nist/reports/UOV-1.html)), with the same number of
  attempts.

The submitted Makefiles write `CC ?= clang`, which does not override GNU
make's built-in `CC = cc`, so whether clang is used depends on the system's
`cc`. The NEON Makefile sets `CC = clang` unconditionally.

## Classification and limits

Low / Confirmed / Side-channel: a compiler-introduced secret-dependent branch
with no measured channel and a local repair (a mask instead of the 0/1
multiplication, checked per compiler). Key generation (where entries of O are
multipliers), III, V, the compressed key formats, GFNI and NEON were not run,
and only the host's clang 21 was tested.

## Reproduction

```sh
python3 UOV/reproduce_uov_6.py
```

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py UOV-6
```
