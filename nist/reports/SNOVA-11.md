# SNOVA-11 — Low: multigraded divided-power wedge model puts two l=2 SNOVA alternatives below target

Severity: Low
Status: Lead
Layer: Design
Affected: Two On-Ramp Round 3 Table 6 l=2 alternative parameter sets (q16-III-l2 and q16-V-l2); q16-I-l2 stays above target and the nine recommended l=4, 5 sets are unaffected
Discovery: Hard
Exploitation: Conditional public-key key-recovery cost/model estimate in a free-memory RAM model; no SNOVA experiments or memory-aware category comparison
Remediation: Moderate
Credit: Multigraded divided-power wedge framework by Peigen Li and Siyong Tao; estimator recomputation, tuple search, and On-Ramp Round 3 evaluation by Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-11-21

## Result

Peigen Li and Siyong Tao's [Attacking UOV-based Signatures over divided power
algebras](https://eprint.iacr.org/2026/2272) (posted 2026-09-30, revised
2026-11-14) lifts SNOVA `l = 2` instances over `F_{16}` to multi-UOV systems
over `F_{256}` (`o` equations of multidegree `(2,0)`, `2o` of multidegree
`(1,1)`, and `o` of multidegree `(0,2)`) and evaluates a multigraded
divided-power wedge attack in Table 6 (p. 17) on the three `q = 16, l = 2`
alternative parameter sets from [SNOVA-spec.pdf](https://csrc.nist.gov/csrc/media/Projects/pqc-dig-sig/documents/SNOVA_specifications.pdf) Table 6
(physical p. 30; extracted in [alternative-parameters.csv](https://github.com/ngcc-dev/ngcc-harness/blob/main/nist/SNOVA/alternative-parameters.csv)).

Recomputing Li and Tao's multigraded Hilbert-series formula (`Vto <= 0`, §3.4;
matching `Vto` in the authors' published
[`Math.magma`](https://raw.githubusercontent.com/Yulan24/Extended-Schur-Macaulay/a292e4c74371fd0d6bae3159ac8c257a441fea27/Math.magma),
commit `a292e4c7`, which does not itself include a SNOVA cost function)
together with our own row-density implementation following §3.4 (`3 * rho *
H^2 * A(256)` with `A(256) = 2*8^2 + 8 = 136` bit operations, weighting
`(2,0)`, `(1,1)`, `(0,2)` rows by `o`, `2o`, `o` and ignoring memory access
cost), which reproduces the paper's three Table 6 values, and searching over
projection and level tuples `(k1, k2, r1, r2, r3)` with `r_i in {0, 1}`,
confirms that **two of the three `l = 2` alternatives fall below their claimed
Category 3 and Category 5 targets while `q16-I-l2` stays above Category 1**:

| Alternative set | `(v, o, q, l)` | Target | Spec Table 6 cost | Paper Table 6 `(k, r, D, bit)` | At printed tuple | Tuple-search minimum (`r_i <= 1`) | Margin | Classical `r=(0,0,0)` | Vector `log2(H)` |
|---|---|---:|---:|---|---:|---:|---:|---:|---:|
| `q16-I-l2` | `(50, 17, 16, 2)` | 143 | 158 | `((14,0),(0,1,1),(3,17),156)` | 155.33 | 154.95 (`r=(0,0,1)`) | +11.95 | 155.66 | 66.10 |
| `q16-III-l2` | `(76, 25, 16, 2)` | 207 | 223 | `((23,0),(0,1,1),(2,25),205)` | **204.70** | **204.63** (`r=(0,0,1)`) | **-2.37** | no admissible tuple | 89.64 |
| `q16-V-l2` | `(104, 33, 16, 2)` | 272 | 287 | `((31,0),(0,0,1),(2,33),263)` | **262.96** | **262.96** (`r=(0,0,1)`) | **-9.04** | no admissible tuple | 117.91 |

The nine recommended On-Ramp Round 3 sets (`l = 4` and `l = 5`) are not
covered by this `l = 2` construction.

## Modeling limits and classification

1. **Unproven multigraded Hilbert-series conjecture.** As Li and Tao state
   explicitly in §4 (p. 18), the estimates rest on the unproven multigraded
   extension of Conjecture 3.6 (§3.4, p. 12). Their small-instance experiments
   (Table 1, p. 12) cover single-graded UOV systems (`n <= 11`); no experiment
   was reported for lifted multigraded SNOVA systems.
2. **Unaccounted memory cost.** The cost formula charges only field operations
   in block Wiedemann (`3 * rho * H^2 * 136`) and ignores memory. Storing a
   single vector of `H` `GF(256)` elements requires `2^92.64` bits for
   `q16-III-l2` and `2^120.91` bits for `q16-V-l2`, and storing the sparse
   Macaulay matrix requires roughly `2^109` and `2^139` bits. Even a modest
   memory-access charge (`log2(H)` or mesh penalty) pushes `q16-III-l2` back
   above 207 and raises `q16-V-l2` near or above 272.
3. **Alternative-parameter scope.** Like [SNOVA-7](/nist/reports/SNOVA-7.html), this affects
   only the unimplemented Table 6 `l = 2` alternative parameter sets, not the
   nine recommended `l = 4` and `l = 5` parameter sets.

This is classified as **Low / Lead / Design**: a candidate-specific,
reproducible RAM-gate calculation putting two Table 6 alternatives 2.4 and 9.0
bits below their claimed categories, conditional on an unproven multigraded
Hilbert-series conjecture with no SNOVA experiments and omitted exponential
memory costs.

## Reproduction

```sh
python3 SNOVA/reproduce_snova_11.py
```

The standard-library script pins `SNOVA/SNOVA-spec.pdf` and
`SNOVA/alternative-parameters.csv`, evaluates the exact multigraded `Vto`
criterion and row-density cost both at the printed Table 6 tuples and across a
full projection/level search (`r_i in {0, 1}`) for all three `l = 2`
alternatives, and checks `r = (0, 0, 0)` classical-wedge controls. It is a
deterministic cost/model check, not an attack execution.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py SNOVA-11
```
