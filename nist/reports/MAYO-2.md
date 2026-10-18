# MAYO-2 — Low: Theorem 1 covers only a small signing-query range

**Severity:** Low
**Status:** Proof gap
**Layer:** Design
**Affected:** On-Ramp Round 3 MAYO1, MAYO2, MAYO3 and MAYO5 security reduction
**Discovery:** Moderate
**Exploitation:** proof/model limitation; no forgery or key-recovery attack
**Remediation:** Moderate
**Credit:** Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
**Date:** 2026-10-18

## Bound and scope

Theorem 1 of the On-Ramp Round 3 specification (§5.2, physical p.28) proves a ROM
EUF-CMA bound only under `Qs*B < 1`, where `Qs` is the number of signing
queries and

```text
B = (q^(k-(n-o)) + q^(m-k*o)) / (q-1).
```

The reduction contains the multiplicative loss `(1-Qs*B)^-1`. Applying the
submitted parameters gives the following limits:

| Set | `log2(B)` | `log2(1/(2B))` | `log2(1/B)` |
|---|---:|---:|---:|
| MAYO1 | -3.9069 | 2.9069 | 3.9069 |
| MAYO2 | -7.9069 | 6.9069 | 7.9069 |
| MAYO3 | -11.9069 | 10.9069 | 11.9069 |
| MAYO5 | -11.9069 | 10.9069 | 11.9069 |

The middle column is the signing-query exponent below which this factor stays
under two. The last is where the theorem's premise fails altogether. All are
far below NIST's evaluation allowance of up to `2^64` chosen-message signing
queries.

## Classification and limits

The specification explicitly discusses this limitation in §5.3, physical
pp.31–32. It says the proof gives essentially no guarantee at realistic query
counts for MAYO1 and MAYO2, describes the loss as information-theoretic leakage
from rejection sampling, and reports no attack that exploits it. It expects
MAYO to remain secure even with unboundedly many signatures.

This report therefore records a Low proof gap under the project's uniform
proof-gap rule: a candidate-specific reproduced calculation makes the
submitted reduction ineffective within NIST's stated evaluation budget. The
calculation does not establish a distinguisher,
key recovery, or forgery, and it does not show that the claimed parameter
strengths are below target. The gap applies to the submitted theorem for all
four sets; its practical consequence remains unknown. A proof that controls
the restart leakage for realistic query counts, or a signing construction that
avoids the loss, would close it.

## Reproduction

```sh
python3 MAYO/reproduce_mayo_2.py
```

The deterministic standard-library script pins the specification, evaluates
`B` with exact rational arithmetic from every submitted parameter tuple, and
checks both the constant-factor and theorem-validity boundaries. It is a
formula/model check, not an attack.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py MAYO-2
```
