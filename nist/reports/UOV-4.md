# UOV-4 — Low: selective divided-power rows conditionally put uov-V below target

Severity: Low
Status: Lead
Layer: Design
Affected: uov-V in classic, PKC and PKC+SKC formats; uov-Ip is not affected by the admissible construction
Discovery: Hard
Exploitation: Conditional public-key key-recovery cost/model path; the required partial-tower rank is not validated at scale
Remediation: Moderate
Credit: Original truncated-ring analysis by Hiroki Furue and Yasuhiko Ikematsu; divided-power attack by Simon-Philipp Merz and Lars Ran; sparse-row observation and On-Ramp evaluation by Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-10-18

## Result

The submission's §4.6 truncated-polynomial-ring calculation remains a useful
baseline. Recomputing it with exact integer power series gives:

| Parameter set | Specification-formula cost | Claimed reference | Margin |
|---|---:|---:|---:|
| uov-Ip | 144.75 | 143 | +1.75 |
| uov-Is | 158.55 | 143 | +15.55 |
| uov-III | 213.12 | 207 | +6.12 |
| uov-V | 272.05 | 272 | +0.05 |

Table 5 floors these logarithms to 144, 158, 213 and 272. The baseline puts
no set below its claim.

Merz and Ran's [divided-power Schur--Macaulay
attack](https://eprint.iacr.org/2026/2181) constructs several families of
linear equations in the oil-space Pluecker coordinates and uses block
Wiedemann to find the one-dimensional kernel. Their §6.1 cost estimate
randomly shortens the rectangular matrix to a square one and charges the
average row density. The full-shape estimates are about 143.9, 158.1, 216.6
and 280.7 bits for uov-Ip, uov-Is, uov-III and uov-V.

If the plain rows have their predicted rank, an independent subset can keep
all but its nullity from that cheap family and use denser divided-square rows
only to supply the missing rank. Under the partial-tower rank assumption
described below, the same RAM-gate convention gives:

| Parameter set | Selective-row cost | Random partial-family cost | Target | Margin |
|---|---:|---:|---:|---:|
| uov-Ip | no admissible instance | no admissible instance | 143 | -- |
| uov-Is | 153.43 | 155.68 | 143 | +10.43 |
| uov-III | 209.26 | 211.75 | 207 | +2.26 |
| uov-V | **267.58** | 271.32 | 272 | **-4.42** |

For uov-V the valid route first restricts the odd-dimensional public system
to a generic hyperplane and then applies the characteristic-two intersection:
`(v,o,m)=(163,96,96)` becomes `(163,95,96)` and then
`(v',o',m')=(231,27,286)`. At degree 27 the model has
`log2(N)=121.1327`, predicted plain-family nullity
`log2(h0)=110.4417`, plain row weight `C(233,2)=27028`, and divided-square
row weight `C(235,4)=123855810`. Selecting `N-h0+1` cheap rows and `h0-1`
dense rows gives `log2(3*nnz*N*136)=267.5751`.

The previously proposed 266.8-bit uov-V and 140.7-bit uov-Ip values are not
admissible. They apply the characteristic-two intersection directly at odd
ambient dimension, where the alternating matrices that must be inverted are
singular. Hyperplane restriction preserves the uov-V lead but eliminates the
uov-Ip instance. It also moves uov-III to 209.26, above its target.

## Unresolved rank premise

The calculation assumes the partial divided-power series

```text
(1+t)^n / ((1+t^2)^m (1+t^4)^m)
  = (1+t)^n (1-t^2)^m / (1-t^8)^m
```

predicts the ranks of the plain and divided-square row families at the
selected degree. Merz--Ran conjecture and test the full divided-power tower;
they do not establish this truncated-family series or sparse-first selection
at the uov-V dimensions. Their paper also warns that systems produced by the
intersection construction need not behave as generic systems.

The retained Sage witness finds a counterexample to the partial formula as a
general identity. Over GF(256), a planted UOV instance with
`(v,a,m)=(4,10,1)` has `N=1001`. The formula predicts ranks 911 and 1000 for
the plain and combined families, but three seeded trials give ranks 895 and
959. This instance has more oil than vinegar and only one form; even the plain
rank prediction fails. It does not model the uov-V ratio. A second planted toy
with `(v,a,m)=(9,8,3)` also misses the predicted plain rank (23680 versus
24193), although its plain-plus-divided-square family reaches corank one.

Independent finite-field toys strengthen the positive evidence. Planted toys
with `v/a` at least 1.43 and small intersection-derived UOV toys over GF(16)
and GF(256) matched the predicted plain and combined ranks. Selected square
submatrices reached corank one, with the oil Pluecker vector as their kernel.
The intersection-derived toys used random linear combinations of only part of
the `3m-2` transformed form family, however. They reached degree at most 8,
versus degree 27 and `v'/a=231/27` for uov-V. Neither the full transformed
family nor the submitted dimensions have been checked. Merz--Ran's published
conjecture covers the full divided-power tower for generic systems, not this
partial family after the intersection transform.

Conditional on the two required ranks, keeping the cheap rows is legitimate:
linear algebra guarantees a basis containing `N-h0` independent plain rows
and `h0-1` additional divided-square rows. Dropping rows can create extra
kernel vectors unless the selected matrix still has rank `N-1`. Small toys
confirm this rank for their instances; a practical full-scale selection
procedure and the rank premise itself remain unchecked. Oversampling the
divided-square rows by factors of 2 to 20 gives conditional uov-V estimates
of 268.4 to 271.5 bits. Random selection from only the plain and
divided-square families gives 271.32 bits under the same rank assumption, just
0.68 bit below target.

## Memory and interpretation

The block-Wiedemann vector alone contains `N` GF(256) elements, about
`2^124.13` bits for uov-V. Storing the selected sparse matrix needs at least
about `2^140.77` bits for its nonzero field elements, before indices and
overhead; regenerating rows trades storage for work. This report has not
recomputed the submission's MEM-2D model for the selective matrix, so the
`267.58` result is only a RAM-gate figure. A mesh-style sensitivity estimate
is above target but is not presented as an official NIST-category cost. NIST's
[call, §4.B.3](https://csrc.nist.gov/csrc/media/Projects/pqc-dig-sig/documents/call-for-proposals-dig-sig-sept-2022.pdf)
allows multiple resource metrics, including the cost of access to extremely
large memories; a RAM-only shortfall is therefore not a settled category
failure.

The `2N` or `1.1N` iteration variants discussed in [Chou and
Niederhagen](https://eprint.iacr.org/2026/1787) are not used in the stated
result. Their applicability and setup costs for this Pluecker-kernel problem
have not been established; even the optimistic iteration count does not put
the corrected uov-III construction clearly below target.

This is retained as **Low / Lead**, rather than Medium / Probable. The result
is candidate-specific and conditionally 4.42 bits below the primary RAM
target, but the new rank premise is not a general identity, the intersection
toys cover only subsets of forms at much smaller degrees, and no memory-aware
category comparison or full-scale row-selection procedure exists. The
random-partial estimate has less than one bit of margin. If rank, matrix
generation, and an attack-specific resource cost validate a below-target
path at full scale, its severity must be reassessed.

## Reproduction

```sh
python3 UOV/reproduce_uov_4.py
```

The Python reproducer checks the specification values, applies the required
odd-dimension reduction and prints the conditional selected-row arithmetic.
It is a deterministic cost/model check, not a key-recovery run. With SageMath:

```sh
sage UOV/analysis/row_selection_toy.sage
```

The second command constructs the seeded planted-UOV counterexample and
computes the exact finite-field ranks. The independent toy below constructs
planted or intersection-derived systems and tests selected-row ranks; the
intersection example uses twelve random combinations of the transformed
forms and may take several minutes in SageMath:

```sh
sage UOV/analysis/row_selection_verifier.sage inter 8 20261018 1 11 9 5 12
```

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py UOV-4
```
