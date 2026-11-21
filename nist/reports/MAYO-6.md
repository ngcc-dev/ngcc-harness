# MAYO-6 — Low: extended Schur--Macaulay model puts five Table 2.2 additional MAYO sets below target

Severity: Low
Status: Lead
Layer: Design
Affected: Five On-Ramp Round 3 Table 2.2 additional parameter sets at r=1 (two Category 3, three Category 5), plus one Category 1 Table 2.2 set by 0.6 bit at r=0; for the four main implemented sets (MAYO1, MAYO2, MAYO3, MAYO5 in Table 2.1), the estimator finds no admissible tuple or stays above target
Discovery: Hard
Exploitation: Conditional public-key key-recovery cost/model estimate in a free-memory RAM model; no full-scale attack or memory-aware category comparison
Remediation: Moderate
Credit: Extended Schur--Macaulay framework and Magma estimator by Peigen Li and Siyong Tao; estimator recomputation, Table 2.2 sweep, and On-Ramp Round 3 evaluation by Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-11-21

## Result

Peigen Li and Siyong Tao's [Extending the Schur--Macaulay Framework for UOV
Attacks via Truncated Polynomial Algebras](https://eprint.iacr.org/2026/2444)
(posted 2026-10-11, revised 2026-11-17) reformulates Merz and Ran's
[Schur--Macaulay divided-power attack](https://eprint.iacr.org/2026/2181) in
multigraded `p`-truncated polynomial algebras and applies the characteristic-two
model (Table 4, p. 25) to seven additional parameter sets from
[MAYO-spec.pdf](https://csrc.nist.gov/csrc/media/Projects/pqc-dig-sig/documents/mayo_specification.pdf) Table 2.2 (physical p. 20).

Running a Python port of the authors' published Magma estimator
([`Complexity.magma`](https://raw.githubusercontent.com/Yulan24/Extended-Schur-Macaulay/a292e4c74371fd0d6bae3159ac8c257a441fea27/Complexity.magma)
and
[`Math.magma`](https://raw.githubusercontent.com/Yulan24/Extended-Schur-Macaulay/a292e4c74371fd0d6bae3159ac8c257a441fea27/Math.magma),
commit `a292e4c7`) across all four Table 2.1 main sets and all 28 Table 2.2
additional sets with `o <= 33` (in the paper's free-memory RAM-gate model,
`3 * rho * H^2 * 36` bit operations matching MAYO's 36-bit-operation `GF(16)`
convention) shows that **for the four main implemented parameter sets in
Table 2.1, the estimator finds no admissible tuple (`MAYO1`, `MAYO3`, `MAYO5`)
or stays above target (`MAYO2`)**, while **five Table 2.2 sets fall 2.8 to
41.7 bits below their claimed Category 3 (`207`) and Category 5 (`272`) targets
at `r = 1`** (and one additional Category 1 Table 2.2 set, `(77, 66, 11, 6)`,
reaches `142.41` at `r = 0`, 0.59 bit below `143`, versus `145.19` at `r = 1`):

| Spec table / level | Parameter set `(n, m, o, k, q)` | Target | Estimator `r=1` `(k1, k2, d1, d2)` | Paper Table 4 `(k1, k2, d1, d2, bit)` | Estimator `r=1` RAM bits (`r=0`) | Margin (`r=1`) | Balanced `k1=k2` (`r=1`) | Vector `log2(H)` |
|---|---|---:|---|---|---:|---:|---:|---:|
| Table 2.1 (MAYO1, L1) | `(88, 80, 8, 10, 16)` | 143 | no tuple (`Sto > 0`) | — | — | — | — | — |
| Table 2.1 (MAYO2, L1) | `(86, 64, 13, 5, 16)` | 143 | `(0, 9, 13, 4)` | — | 161.26 (165.80) | +18.26 | 196.07 | 70.00 |
| Table 2.1 (MAYO3, L3) | `(118, 108, 10, 11, 16)` | 207 | no tuple (`Sto > 0`) | — | — | — | — | — |
| Table 2.1 (MAYO5, L5) | `(154, 142, 12, 12, 16)` | 272 | no tuple (`Sto > 0`) | — | — | — | — | — |
| Table 2.2 (L1, sweep) | `(77, 66, 11, 6, 16)` | 143 | `(0, 5, 11, 6)` | — | 145.19 (**142.41**) | +2.19 (-0.59 at `r=0`) | 166.21 | 62.55 |
| Table 2.2 (L3) | `(113, 100, 13, 8, 16)` | 207 | `(0, 4, 13, 9)` | `(0, 3, 13, 10, 215)` | 215.78 (219.31) | +8.78 | 228.79 | 97.05 |
| Table 2.2 (L3) | `(112, 98, 14, 7, 16)` | 207 | `(0, 8, 14, 6)` | `(0, 7, 17, 10, 198)` | **198.71** (**202.89**) | **-8.29** | 227.43 | 88.24 |
| Table 2.2 (L3) | `(118, 96, 16, 6, 16)` | 207 | `(0, 12, 16, 4)` | `(0, 11, 16, 5, 196)` | **196.13** (209.21) | **-10.87** | 243.03 | 86.58 |
| Table 2.2 (L5) | `(152, 136, 16, 9, 16)` | 272 | `(1, 2, 15, 14)` (Magma tie: `(0, 3, 16, 13)`) | `(0, 3, 16, 13, 281)` | 285.17 (281.39) | +13.17 | 291.74 | 131.25 |
| Table 2.2 (L5) | `(151, 134, 17, 8, 16)` | 272 | `(0, 7, 17, 10)` | `(0, 6, 17, 11, 269)` | **269.24** (272.45) | **-2.76** | 290.50 | 122.81 |
| Table 2.2 (L5) | `(152, 132, 19, 7, 16)` | 272 | `(0, 15, 19, 4)` | `(0, 14, 19, 5, 230)` | **230.33** (**244.35**) | **-41.67** | 289.87 | 103.01 |
| Table 2.2 (L5) | `(168, 132, 22, 6, 16)` | 272 | `(0, 18, 22, 4)` | `(0, 17, 22, 5, 254)` | **254.87** (277.84) | **-17.13** | 324.64 | 114.90 |

In the companion paper [Attacking UOV-based Signatures over divided power
algebras](https://eprint.iacr.org/2026/2272) (Li and Tao, revised 2026-11-14,
Table 4), the single-variable divided-power wedge attack applies only to
Table 2.2 sets with `o >= 30` and leaves all evaluated MAYO sets above target
(214–215 bits at Level III and 278 bits at Level V).

## Estimator discrepancies, unbalanced regime, and modeling limits

1. **Unbalanced projection caveat.** Remark 6.4 (p. 24) states that the authors
   restrict their analysis to balanced projection down because they have not
   obtained an effective Hilbert-series estimate for unbalanced projection.
   Every below-target estimate in Table 4 and in `Complexity.magma`
   (`schtwocplx`) nevertheless uses an unbalanced tuple (`k1 = 0 < k2`).
   Restricting `schtwocplx` to balanced projection `k1 = k2` (with fixed
   degrees `d1 = d2 = o - k`, which is not a search over arbitrary balanced
   degrees) gives `227.43`–`243.03` bits at Level III and `289.87`–`324.64`
   bits at Level V (and `166.21` for `(77, 66, 11, 6)`), all above target.
2. **Discrepancies between Table 4 and the authors' Magma estimator.** Six of
   the seven Table 4 rows reproduce their printed floored bit costs in
   `Complexity.magma` at `k2 + 1` (and `d2 - 1`) rather than the printed
   `(k2, d2)` (and `(112, 98, 14, 7)` prints `d1 = 17 > o = 14`, whereas
   `schtwocplx` evaluates `(0, 8, 14, 6)` at `198.71`). The seventh row,
   `(152, 136, 16, 9)`, reproduces `281` at its printed tuple `(0, 3, 16, 13)`
   only with `r = 0` (`281.39`), not the paper's stated `r = 1` (`285.17` at
   `(1, 2, 15, 14)`, whereas `Complexity.magma` prints `(0, 3, 16, 13)` at
   `285` because it compares floored costs with `<=` and keeps the last tie).
3. **Conjectural rank premise and retained toy checks.** The solving-degree
   criterion relies on unproven Hilbert-series and structured-rank conjectures
   (Conjecture 3.11 and Eq. 5.2), with no full-scale experiments. The retained
   SageMath toy script [`analysis/unbalanced_schur_toy.sage`](https://github.com/ngcc-dev/ngcc-harness/blob/main/nist/MAYO/analysis/unbalanced_schur_toy.sage)
   checks small unbalanced `GF(16)` UOV instances (`n = 7..9`, `o = 4`,
   `k2 = 1..3`): all ten tested instances with `Sto <= 0` achieved exact
   corank one (`nullity = 1`), while two positive-`Sto` toys departed from
   `Sto` in opposite directions — `(n=8, o=4, m=2, k2=2)` gave `nullity = 72`
   versus `Sto = 59`, and `(n=7, o=4, m=4, k2=1)` gave `nullity = 1` despite
   `Sto = 221 > 0`. Thus `Sto` is not an exact dimension identity in the
   unbalanced regime, and `Sto > 0` alone does not prove that the attack fails
   (so for `MAYO1`, `MAYO3`, and `MAYO5`, the estimator finding no admissible
   tuple is a negative result of this model rather than a proof of immunity).
4. **Unaccounted memory cost.** Remark 6.3 explicitly omits memory. One
   block-Wiedemann vector holds `H` `GF(16)` elements (`2^88.6` to `2^124.8`
   bits for the five `r = 1` below-target sets; storing the sparse matrix
   requires roughly `2^104` to `2^141` bits). For `(152, 132, 19, 7)` at
   `230.33` RAM bits (`log2(H) = 103.01`), a logarithmic memory factor still
   leaves roughly a 35-bit shortfall, whereas a 2D-mesh `sqrt(memory)` penalty
   pushes the cost above 272 bits. Under [NIST's call §4.B.3](https://github.com/ngcc-dev/ngcc-harness/blob/main/nist/doc/NIST-rules.md),
   a RAM-only estimate without memory accounting is not a settled category
   break.
5. **Scope limited to Table 2.2 showcase sets.** None of the affected tuples is
   among the four submitted and implemented parameter sets in Table 2.1
   (`MAYO1`, `MAYO2`, `MAYO3`, `MAYO5`). Table 2.2 lists them as additional
   trade-off parameter sets in the pinned Round 3 specification.

## Classification

**Low / Lead / Design.** The calculation is candidate-specific, reproduces all
seven Table 4 floored bit costs from the authors' published Magma estimator
(plus the full Table 2.2 sweep for `o <= 33`), and puts five Table 2.2
trade-off sets 2.8 to 41.7 bits below their claimed RAM-gate targets at `r = 1`
(plus one Category 1 set by 0.6 bit at `r = 0`). It remains a **Lead** rather
than a confirmed category break because every below-target point sits in the
conjectural unbalanced regime (`k1 = 0 < k2`), omits `2^89`–`2^141`-bit memory
costs, executes no full-scale attack, and for all four main implemented sets
the estimator finds no admissible tuple or stays above target.

## Reproduction

```sh
python3 MAYO/reproduce_mayo_6.py
```

The standard-library script pins `MAYO/MAYO-spec.pdf` and ports `Complexity.magma`
and `Math.magma` (commit `a292e4c7`) to sweep all four Table 2.1 main sets and
all 28 Table 2.2 additional sets with `o <= 33` (both unbalanced and balanced
`k1 = k2` at `r = 0, 1`), verifying all seven Table 4 floored bit costs, all
six off-by-one `(k2, d2)` rows, and the `(152, 136, 16, 9)` `r = 0` row. It is
a deterministic cost/model check, not a key-recovery run. With SageMath, run
the small `GF(16)` unbalanced toy checks with:

```sh
sage MAYO/analysis/unbalanced_schur_toy.sage
```

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py MAYO-6
```
