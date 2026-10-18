# MAYO-3 — Low: MAYO2 structural-forgery estimate depends on cost accounting

Severity: Low
Status: Lead
Layer: Design
Affected: Round-3 MAYO2 concrete Category-1 estimate
Discovery: Non-trivial
Exploitation: Published heuristic direct-forgery cost model; no attack execution or experiments
Remediation: Moderate
Credit: Original cryptanalysis by Massimo Ostuzzi; independent verification and cost-model reassessment by Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-10-18

## Result

Ostuzzi's September 2026 paper
[Geometric Forgeries: Structural Cryptanalysis of MAYO](https://eprint.iacr.org/2026/2247)
gives a new structural direct-forgery strategy for Round-3 MAYO. Its Table 6
reports 140 bits for MAYO2, against the submitters' Category-1 target of 143
bit operations. An independent reimplementation of the paper's Theorems 1 and
3 and Section 7.2 dimension model reproduced the estimate as `2^140.8` bit
operations. The same reimplementation reproduced all four printed Round-3
figures to within 0.8 bit.

The paper's result depends on how a field operation and per-guess work are
priced:

| Model | MAYO2 log2 bit operations |
|---|---:|
| Paper convention: 16 bit operations per GF(16) operation | 140.8 |
| Submission convention: 36 bit operations per GF(16) operation | 142.2 |
| Submission convention with cubic matrix arithmetic | 142.9 |
| Paper convention with symmetric specialization accounting | 145.1 |
| Submission convention with symmetric specialization accounting | 146.3 |

The submission uses 36 bit operations per GF(16) multiply-add and states that
known-attack estimates should exceed `2^143` for Category 1 (§5.4). On that
like-for-like convention, the estimate is about 0.8 bit below
the target, rather than the roughly 3-bit shortfall under the paper's
16-bit-operation convention. The result is insensitive to changing the matrix
exponent from 2.81 to 3: the corresponding value is 142.9.

## Specialization-cost sensitivity

The paper author's generic baseline charges a per-guess specialization cost,
but the structural Round-3 model omits that cost. Applying the same generic
specialization formula to the structural schedule raises the estimate to 145.1
under the paper convention and 146.3 under the submission convention. The
optimizing block schedule also changes from four blocks of size two to two
blocks of size four.

This sensitivity calculation is not a proof that the higher values are the
right attack cost. Gray-code enumeration may amortize some of the
specialization work. Conversely, the submission's own direct-attack estimate
also says that its methodology ignores the sometimes non-negligible cost of
variable substitutions and eliminations (§5.4). The defensible range therefore
crosses the 143-bit target, depending on a presently unsettled accounting
choice.

## Evidence, classification, and limits

For the paper convention, the selected MAYO2 schedule has `a=5`, `u=15`,
`r=15`, `w=40`, `h=46`, and four blocks of size two. Its two dominant terms
are approximately `2^140.1` for the endpoint iterations and `2^139.0` for the
small-system solves. The strategy is readily parallelized and does not depend
on the large-memory model that limits some other multivariate attacks.

The paper supplies no experiments. Its estimate assumes random-system
heuristics and treats dimension bounds as equalities. This study did not run a
forgery attack, and the symmetric-cost estimate moves the result above the
claimed target. The finding is therefore Low / Lead: it records a narrow,
model-dependent concrete-margin concern, not a demonstrated Category-1 break.
Experimental validation of the dimension assumptions and a consistent account
of per-guess specialization would determine whether the lead survives.

## Reproduction

```sh
python3 MAYO/reproduce_mayo_3.py
```

The deterministic standard-library script pins the Round-3 specification,
recomputes the selected dimensions, verifies the exact specialization
operation count, and checks all five estimates above from the independently
verified cost certificates. It records the verified full-paper SHA-256. It is
a selected-cost certificate, not a repeat of the external-estimator exhaustive
search, an experiment, or an attack execution.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py MAYO-3
```
