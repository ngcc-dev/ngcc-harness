# SNOVA-8 — Low: clang compiles the optimized signer's pivot mask into a branch on secret data

Severity: Low
Status: Confirmed
Layer: Side-channel
Affected: Optimized tree when built with clang: observed with clang 21 at -O3 and -Os on SNOVA_I_K and at -O3 on SNOVA_V_S, the two source files shared by all 18 optimized sets; gcc 15.2 -O3 and -Os builds showed no such branch, and the submitted Makefile uses gcc
Discovery: Moderate
Exploitation: Compiled-code taint witness of a branch on the secret Gaussian-elimination pivot; no timing measurement or key recovery
Remediation: Local
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-11-15

## Defect

The optimized signer builds its pivot mask from `ct_is_not_zero`, which is
`return val != 0;`
(`snova_opt_16.c:69-72` (`SNOVA/submission/SNOVA_Round3/Optimized_Implementation/SNOVA_I_K/snova_opt_16.c#L69`),
used at
`snova_opt_16.c:1184-1189` (`SNOVA/submission/SNOVA_Round3/Optimized_Implementation/SNOVA_I_K/snova_opt_16.c#L1184`);
`snova_opt_16_5.c` for l=5). clang 21 turns the mask into a conditional jump
that skips the row addition (`test %r15b,%r15b; jne`). The gcc 15.2 builds
tested emit a mask.
Specification §5.4 (p.53) says the optimized implementation is constant time
apart from rejection sampling. [SNOVA-5](/nist/reports/SNOVA-5.html) covers the reference
signer.

## Evidence

The reproducer builds the optimized signer for SNOVA_I_K and SNOVA_V_S and
runs signing under Valgrind memcheck with the secret key and randomness
marked undefined.

- clang 21 `-O3`, I_K: a conditional jump on undefined data at
  `snova_opt_16.c:1185`, exactly 4,560 times per signing attempt (one per row
  pair), unchanged when only the 32-byte private seed is tainted. clang `-Os`
  branches at line 1187, inside the row loop; V_S branches at
  `snova_opt_16_5.c:1207`.
- Control: gcc `-O3` builds of the same sources report nothing at the pivot,
  only the retry outcome, with the same number of attempts; a clang build with
  nothing tainted reports nothing.
- Recorded: the commented-out bitwise body of `ct_is_not_zero` gives the same
  branch under clang, so the cause is the compiler's treatment of the mask,
  not the `!=` comparison alone.

## Classification and limits

Low / Confirmed / Side-channel: a secret-dependent branch in compiled code with
no measured channel and a local repair. The repair (for example a value
barrier on the mask) has to be checked in the compiled code. Other clang
versions, architectures and the additional tree's `ARCH=ref` backend were not
tested.

## Reproduction

```sh
python3 SNOVA/reproduce_snova_8.py
```

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py SNOVA-8
```
