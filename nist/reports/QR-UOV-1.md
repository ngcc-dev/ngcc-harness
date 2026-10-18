# QR-UOV-1 — Medium: submitted solvers expose secret pivot structure

**Severity:** Medium
**Status:** Confirmed
**Layer:** Side-channel
**Affected:** On-Ramp Round 3 reference, optimized, and AVX2 signing implementations; all parameter sets
**Discovery:** Moderate
**Exploitation:** source-only; no measured timing/cache channel or key recovery
**Remediation:** Moderate
**Credit:** Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
**Date:** 2026-10-18

## Secret flow and affected operations

The vinegar vector `y` and secret transformation `sdt` build the signing
matrix at
`qruov_common.c:39-66` (`QR-UOV/submission/QR_UOV - Round3/Reference_Implementation/ref/qruov_common.c#L39`),
and `qruov_common_sign_solve_oil` passes that matrix to
`linsys_lu_decompose` at line 104.
The submitted solver then:

- searches until a nonzero secret-derived pivot is found, with early exits and
  secret loop counts at
  `linsys.c:29-45` (`QR-UOV/submission/QR_UOV - Round3/Reference_Implementation/ref/linsys.c#L29`);
- swaps row pointers according to that pivot at line 47;
- indexes `gf_inv_table` by the pivot at lines 50–54 and
  `gf.h:27-53` (`QR-UOV/submission/QR_UOV - Round3/Reference_Implementation/ref/gf.h#L27`);
- uses the derived rank and pivot structure to set later iteration bounds and
  sample a solution at `linsys.c:68-168`; and
- retries salt generation according to rank and consistency at
  `qruov_common.c:104-124` (`QR-UOV/submission/QR_UOV - Round3/Reference_Implementation/ref/qruov_common.c#L104`).

The `linsys.c` and `qruov_common.c` files are respectively byte-identical in
the reference, optimized, and AVX2 directories. No alternate constant-time
solver is present in the submitted archive.

## What the specification already discloses

Specification §4.10, physical p.29, states that the reference implementation
does not use a constant-time linear-algebra algorithm. Appendix A treats the
matrix rank as observable and analyzes that rank leak, while requiring the
remaining pivot positions, values, consistency decisions, and solution
assignments to use fixed ranges and mask-based updates (Appendix A.3, physical
pp.65–66). The archive's solver is the original data-dependent version, not
the countermeasure described there.

This report excludes the deliberately observable **rank alone** from its
claimed defect. It covers the finer pivot row/column sequence, table indices,
and data-dependent path that Appendix A itself says should be protected.

## Impact and limits

A local observer can in principle distinguish secret-derived control flow and
cache addresses during signing. Medium rests on the repeated fine-grained pivot
sequence and table-address trace that Appendix A says should be protected, not
on remediation cost alone. Replacing the solver with the specified fixed-range,
mask-based procedure is a material change and corroborates that scope. No timing
experiment, trace decoder, signing-key recovery, or forgery is demonstrated.
The report therefore does not assert a failure of QR-UOV's claimed NIST
categories or EUF-CMA security.

## Reproduction

```sh
python3 QR-UOV/reproduce_qruov_1.py
```

The offline witness checks the exact submitted hashes in all three backends,
the secret matrix flow, pivot loop, row swap, inverse table, rank retry, and
absence of a second constant-time solver. It does not measure a side channel.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py QR-UOV-1
```
