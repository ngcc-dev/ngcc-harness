# Algebraic RSD literature reproducer

`rsd_literature.py` evaluates three published algebraic results on the exact
binary Regular Syndrome Decoding instances used by SDitH v3.  It is a
deterministic, standard-library-only calculation; it does not build or execute
submission code.

Run it from the repository root:

```sh
python3 SDitH/analysis/rsd_literature.py
```

The script first reproduces one published row from each attack estimator:

* Briaud and Øygarden, *A New Algebraic Approach to the Regular Syndrome
  Decoding Problem and Implications for PCG Constructions*, IACR ePrint
  [2023/176](https://eprint.iacr.org/2023/176), Table 2, Modeling 2 over
  `F_2`: `(n,k,w)=(2^14,3482,198)`, `(f,u,d)=(197,11,2)`, 106 bits after
  rounding.  The implementation follows Equation (24) and Proposition 9.
* Wang et al., *A Hybrid Algorithm for the Regular Syndrome Decoding Problem*,
  IACR ePrint [2025/1284](https://eprint.iacr.org/2025/1284), Table 7:
  `(n,k,w)=(2^14,3482,338)`, `(f_bar,u_bar,g)=(100,6,3)`, 118.53 bits.  The
  implementation follows the optimized binary formula in Theorem 4.

It then applies both estimators to the parameters in SDitH v3 specification
Table 3 (displayed p.40 / physical PDF p.42):

| Category | q | n | k | w | block size b | claimed gate target |
|---|---:|---:|---:|---:|---:|---:|
| I | 2 | 10,360 | 9,928 | 56 | 185 | 143 |
| III | 2 | 18,396 | 17,804 | 73 | 252 | 207 |
| V | 2 | 19,864 | 19,064 | 104 | 191 | 272 |

The third calculation evaluates the degree-2 equation-counting ratio

```text
w * binom(b, 2) / binom(k + 1, 2)
```

from Cueto Noval et al., *On the Soundness of Algebraic Attacks Against
Code-based Assumptions*, IACR ePrint
[2025/415](https://eprint.iacr.org/2025/415), and checks the two field-size
hypotheses in Theorem 2.  The conjectural degree-2 boundary has ratio one.  The
proved polynomial-time results require `q >= b-1` with a `3/2` equation-count
factor, or `q >= sqrt(b)` with a `9/4` factor.  SDitH has `q=2`, so neither
large-field theorem applies.

The Briaud–Øygarden search uses their Section 7 restriction
`u < k/w - 1`, which excludes the region already reduced to regular Prange.
The Wang search requires `v >= 2`: its displayed binary cost contains
`log2(v^2/2)` and `log2(5v/2)`, so treating the degenerate `v=0` boundary as a
zero-cost solve would be an invalid extrapolation.

These values are arithmetic-operation estimates under the papers' modeling
assumptions.  They are not security lower bounds, and they are not directly
equivalent to the NIST gate-count targets.  Their purpose is to test whether
the newer algebraic methods supply a plausible attack lead.  The computed
costs remain above all three claimed targets; direct syndrome-decoding costs
recorded in the design survey are also lower than either algebraic estimate.
