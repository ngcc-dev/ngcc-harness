# MQOM-3 — Low: correlated-tree standard-MQ bound falls 19–49 bits short

**Severity:** Low
**Status:** Proof gap
**Layer:** Design
**Affected:** Nine On-Ramp Round 3 correlated-tree (`-ct`) parameter sets; one-tree (`-ot`) sets are excluded
**Discovery:** Moderate
**Exploitation:** proof/model gap in the PGOW-to-standard-MQ justification; no forgery or key recovery
**Remediation:** Moderate
**Credit:** Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
**Date:** 2026-10-18

## Proof scope

MQOM's security discussion (§5.1, physical pp.73–78) first gives a formal
EUF-KO theorem common to both tree variants. It then states that EUF-CMA follows
by simulating the signing oracle. For the correlated-tree variant, that step is
given by the external FR26 result and assumes partial-guessing one-wayness of
MQ (PGOW-MQ). For the one-tree variant, the stated reduction instead relies
directly on standard MQ. Thus the specification does discuss EUF-CMA; the gap
reported here is confined to the correlated-tree assumption chain.

The specification calls PGOW-MQ weaker than standard MQ because the adversary
has a prefix-validation oracle. Section 5.3.2, physical pp.90–91, gives a
non-tight reduction from PGOW-MQ to standard MQ. Table 26 (physical p.91)
reports these lower bounds:

| Field | Category | Table 26 lower bound | Submission target | Gap |
|---|---:|---:|---:|---:|
| GF(2) | 1 | 122 | 143 | 21 |
| GF(2) | 3 | 188 | 207 | 19 |
| GF(2) | 5 | 245 | 272 | 27 |
| GF(16) | 1 | 121 | 143 | 22 |
| GF(16) | 3 | 171 | 207 | 36 |
| GF(16) | 5 | 223 | 272 | 49 |

Every one of the nine correlated-tree sets maps to one of these field/category
rows, so the standard-MQ reduction alone certifies a lower bound 19–49 bits
below the submission's adopted NIST gate-count target. The specification
describes the reduction's loss relative to its full-MQ estimates as 20–65 bits;
that is a different comparison from the target gaps tabulated above.

## Classification and limits

This is a Low proof gap, not an attack. The calculation does not give an
algorithm meeting a Table 26 lower bound. Section 5.3.3 separately analyzes
PGOW-MQ directly and argues that its validation oracle is unlikely to provide
a significant practical advantage. Those estimates support the parameter
rationale but do not turn the non-tight standard-MQ reduction into a proof at
the claimed targets.

The nine one-tree sets are outside this report because their stated EUF-CMA
reduction relies directly on standard MQ without the PGOW-MQ step. Closing the
finding requires a tighter reduction, a sufficiently supported direct PGOW-MQ
hardness result at each target, or use of the one-tree construction.

## Reproduction

```sh
python3 MQOM/reproduce_mqom_3.py
```

The deterministic standard-library script pins the specification, recomputes
the six target gaps from Table 26, and checks `parameters.csv` to confirm that
all nine `-ct` sets and no `-ot` sets are in scope. It is a table/model check,
not a cryptanalytic attack.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py MQOM-3
```
