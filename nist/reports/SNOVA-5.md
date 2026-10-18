# SNOVA-5 — Low: secret-dependent tables and elimination in the reference signer

Severity: Low
Status: Confirmed
Layer: Side-channel
Affected: All 18 submitted On-Ramp Round 3 q=16 reference API variants (nine sets times SHAKE/AES); the harness builds these reference implementations
Discovery: Moderate
Exploitation: Unproven; source-level address/control-flow leakage with no timing measurement, secret recovery, or forgery
Remediation: Local
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-10-18

The submitted reference signer performs finite-field arithmetic through tables
indexed by secret-derived operands and performs Gaussian elimination with
secret-dependent branches. This makes its address and branch trace dependent on
the signing secret and per-signature state.

In the representative `SNOVA_I_K` source, `gf_mult`, `gf_inv`, `gf_add`, and
`gf_set_add` index `gf_multtab`, `gf_invtab`, and `gf_addtab` with their operands
at
`snova_ref.c:26` (`SNOVA/submission/SNOVA_Round3/Reference_Implementation/SNOVA_I_K/snova_ref.c#L26`).
The signer expands the secret `T12` at
`snova_ref.c:784` (`SNOVA/submission/SNOVA_Round3/Reference_Implementation/SNOVA_I_K/snova_ref.c#L784`),
uses it in field operations beginning at
`snova_ref.c:799` (`SNOVA/submission/SNOVA_Round3/Reference_Implementation/SNOVA_I_K/snova_ref.c#L799`),
and constructs the signing system from secret-derived vinegar and central-map
values at
`snova_ref.c:973` (`SNOVA/submission/SNOVA_Round3/Reference_Implementation/SNOVA_I_K/snova_ref.c#L973`).
Its elimination then branches on zero pivots and row entries, stops its row
search at the first usable pivot, and retries singular systems at
`snova_ref.c:1078` (`SNOVA/submission/SNOVA_Round3/Reference_Implementation/SNOVA_I_K/snova_ref.c#L1078`).
The resulting trace exposes table indices, pivot positions, conditional row
work, and the retry count.

The 18 reference copies are byte-identical (SHA-256
`ef5dc33896f799c19631f888dd8c9192565e219df54e7932849188ae2e7e117c`),
apart from their separate parameter headers, so the same source finding applies
to every reference library catalogued in `libraries.json`. All are recommended
q=16 sets. The odd-q builds use the same source; they were not separately
assessed here.

The [SNOVA specification](https://csrc.nist.gov/csrc/media/Projects/pqc-dig-sig/documents/SNOVA_specifications.pdf), §5.4 (physical PDF page 53), claims
that the **optimized implementation** is constant time and identifies rejection
sampling in its Valgrind discussion. It does not make that claim for the
reference implementation. The submitted optimized signer uses masked pivot
selection and fixed row traversal at
`snova_opt_16.c:1185` (`SNOVA/submission/SNOVA_Round3/Optimized_Implementation/SNOVA_I_K/snova_opt_16.c#L1185`).
The separate AVX2 path likewise declassifies only the aggregate retry flag at
`rct_sign_gfni.h:103` (`SNOVA/submission/SNOVA_Round3/Additional_Implementations/AVX2/SNOVA_I_K/src/platforms/x86_avx2/rct_sign_gfni.h#L103`).
These submitted alternatives justify the Local rating, but this review does not
certify either complete optimized implementation as constant time.

Run the deterministic source certificate with:

```sh
python3 SNOVA/reproduce_snova_5.py --output SNOVA/results/SNOVA-5.json
```

It hashes and checks every reference copy, verifies the secret-indexed table and
elimination constructs, demonstrates two different abstract branch traces and
table offsets, and checks for the masked optimized-path controls. It does not
perform timing or cache measurements. A practical attack would still need to
show that observations through a concrete local or remote interface recover
useful information across signatures.

Use the submitted constant-time optimized path where its platform and build
requirements are met, or replace reference table operations and pivoting with
fixed-address arithmetic and masked row selection/elimination. Keep only the
overall rejection result public and validate the repaired complete signing path
with secret-taint tooling and controlled timing tests.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py SNOVA-5
```
