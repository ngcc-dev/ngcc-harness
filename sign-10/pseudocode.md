# sign-10 Facto-DSA — algorithm summary

Facto-DSA is a multivariate (MQ-family) hash-and-sign signature in the
`T ∘ F ∘ S` paradigm whose central map is *polynomial multiplication* in
F_q[t]: `F_{Q,R}(X,Y) = M_n((Q(X)+R(Y)) ⊗ Y)` with `Q` a triangular quadratic
map and `R` a random quadratic map (spec §1.1.2, §1.3.3). The public key is a
dense system of `m` homogeneous **cubic** forms in `r = 2n` variables; signing
inverts it by factoring a degree-`D` polynomial into two degree-`<n` factors and
solving the triangular system by square roots (`q ≡ 3 mod 4`). Security is
claimed EUF-CMA in the ROM (spec §1.1.1, §3.1), resting on a new
"Facto-DSA public inversion" assumption plus MinRank/Gröbner resistance (§2.1,
§3.2.4). Output-minus (`s = D − m` hidden rows) supplies the signing freedom.

Specification: `sign-10-spec.pdf` (58 pages), §1.4 (Algorithms 1–11), §2.2
(parameter table).

## Parameters

| parameter | Facto-DSA-128 | Facto-DSA-256 | Facto-DSA-512 | meaning |
|---|---|---|---|---|
| q | 65519 | 65519 | 65519 | prime field, q ≡ 3 (mod 4) |
| n | 10 | 17 | 32 | factor vector length |
| r = 2n | 20 | 34 | 64 | public variables |
| D = 2n−1 | 19 | 33 | 63 | product length |
| m | 13 | 32 | 62 | public cubic equations |
| s = D−m | 6 | 1 | 1 | output-minus / completion dimension |
| R_max | 512 | 1024 | 2048 | signing trials |
| claimed security | 128 | 256 | 512 | bits classical (quantum: 80/128/256) |

Sizes (bytes), specification §2.2 Table 2 vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | sig spec | sig impl | match |
|---|---|---|---|---|---|---|---|
| Facto-DSA-128 | 40,040 | 40040 | 3,094 | 3094 | 40 | 40 | yes |
| Facto-DSA-256 | 456,960 | 456960 | 11,662 | 11662 | 68 | 68 | yes |
| Facto-DSA-512 | 5,674,240 | 5674240 | 61,922 | 61922 | 128 | 128 | yes |

All three follow the spec's own formulae: pk = `2m·C(r+2,3)`, sig = `2r`,
sk = `2(r² + D² + N_Q(n) + N_R(n)) + 32` (§2.2). No size discrepancy.

## Pseudocode

### KeyGen (spec Algorithm 9, §1.4.3)
```
S  ← SampleInvertibleMatrix(q, r);  S^-1 ← MatrixInverse(S)
(L1, L2) ← SplitRows(S, n, n)                  # X = L1 z, Y = L2 z
U  ← SampleInvertibleMatrix(q, D);  U^-1 ← MatrixInverse(U)
T  ← FirstRows(U, m)                           # m × D, full row rank
Q  ← SampleTriangularQuadraticMap(q, n)        # Alg. 4: Qi = ai Xi² + bi(X<i)Xi + ci(X<i), ai ≠ 0
R  ← SampleHomogeneousQuadraticMap(q, n, n)    # Alg. 2
C  ← ExpandCubicPublicKey(...)                 # Alg. 8: coeffs of T·Mn(Q(L1z)+R(L2z), L2z)
                                               #   in basis za zb zc, 0 ≤ a ≤ b ≤ c < r
pk ← EncodePublicKey(param, C);  pkh ← Hpk(Encode(pk))
sk ← EncodeSecretKey(param, S^-1, U^-1, Q, R, pkh)
```

### Sign (spec Algorithm 10, §1.4.4)
```
(param, S^-1, U^-1, Q, R, pkh) ← DecodeSecretKey(sk)
h ← HashToField_q^m(FDSA-H, pkh ‖ M)           # deterministic target, Alg. 3
for j = 0 .. Rmax-1:
    c ← SampleVector(q, s)                     # random completion (empty if s = 0)
    y ← U^-1 · Concat(h, c) ∈ F_q^D            # so that T y = h
    if y = 0: continue
    f ← VectorToPolynomial(y, D)
    for (A0, Y0) ∈ BoundedFactorSplits(f, n):  # Alg. 5: f = A0·Y0, deg A0 < n, deg Y0 < n
        for λ ∈ F*_q:                          # spec text §1.4.4(d): "rescaling schedule Λ ⊆ F*_q"
            A ← λ A0;  Y ← λ^-1 Y0;  W ← A − R(Y)
            X ← TriangularInvert(Q, W)         # Alg. 7, below; ⊥ on failure
            if X ≠ ⊥:
                z ← S^-1 · Concat(X, Y)
                if T·Mn(Q(L1 z) + R(L2 z), L2 z) = h:   # internal re-check
                    return σ = Encode(z)       # 2r bytes
return ⊥
```

### TriangularInvert(Q, W) (spec Algorithm 7)
```
B ← {()}                                       # branch list
for i = 0 .. n-1:
    for each partial x ∈ B:
        β ← bi(x0..x_{i-1});  γ ← ci(x0..x_{i-1})
        Δ ← β² − 4 ai (γ − Wi)
        for each v ∈ SquareRoots(Δ):           # Alg. 6: v = Δ^((q+1)/4), uses q ≡ 3 mod 4
            extend x with u = (−β + v)(2ai)^-1
    if no extension survives: return ⊥
    B ← BranchSchedule(B')                     # fixed order + implementation branch cap
return SelectBranch(B)
```

### Verify (spec Algorithm 11, §1.4.5)
```
(param, C) ← DecodePublicKey(pk)
if ByteLength(σ) ≠ 2r: reject
z ← Decode_q^r(σ);  reject if any limb ≥ q
pkh ← Hpk(Encode(pk))
h   ← HashToField_q^m(FDSA-H, pkh ‖ M)
µ   ← CubicMonomialVector(z, CanonicalCubicOrder(r))    # all za zb zc, a ≤ b ≤ c
accept iff C·µ = h
```

### Hash / XOF layer (spec §1.4.2, Algorithm 3)
```
Hpk(b)                   = fixed-length digest of the canonical pk encoding (32 bytes)
HashToField_q^m(ℓ, d):   B ← XOF(ℓ ‖ d);  repeat { w ← ReadUInt16(B); accept if w < q }
                         until m elements collected      # rejection sampling, 2-byte limbs
Domain label: "FDSA-H" (the spec lists exactly one label)
```

## Implementation vs specification

Checked: `src/<inst>/SIG_AlgorithmInstance.{h,c}` (one file per instance, plus
verbatim ICCS `drng.c` / `auxfunc.c`); `Makefile` builds all three instances
from `Reference_Implementation/Facto-DSA-{128,256,512}`.

Parameter sampling (6 constants × 3 instances, all in
`src/<inst>/SIG_AlgorithmInstance.{h,c}`): `FACTO_N` = 10/17/32, `FACTO_M` =
13/32/62, `FACTO_Q` = 65519, and derived `FACTO_D = 2N−1`, `FACTO_S = D−M`
(= 6/1/1), `FACTO_RMAX` = 512/1024/2048 — **all agree with spec Table 2**.
`PK_BYTES`, `SK_BYTES`, `SN_BYTES` (lines 82–84) reproduce the spec's size
formulae and the observed library sizes exactly.

Agreements: cubic monomial order `0 ≤ a ≤ b ≤ c < r` and `FACTO_CUBIC_TERMS =
C(r+2,3)`; discriminant `Δ = β² + 4a(W_i − γ)` in `invert_Q_rec` matches Alg. 7
line 7; `f_sqrt` uses the `q ≡ 3 mod 4` exponentiation; verify enforces
`sn_len == 2r`, rejects limbs ≥ q in both σ and pk, recomputes `pkh` from the
whole pk, and accepts iff `P(z) = h` (Alg. 11 followed step for step).

Discrepancies / deviations:

- **(a) deviation, benign-to-positive** — domain label. Spec §1.4.2 defines the
  single label `FDSA-H`; the code uses per-instance labels `"FDSA-H-128"`,
  `"FDSA-H-256"`, `"FDSA-H-512"` (`SIG_AlgorithmInstance.c:1487` in each
  instance dir). Stronger separation than specified, but the spec text is what
  a third-party re-implementation would follow — KATs would not interoperate.
- **(a) deviation** — the hash input is `label ‖ pkh ‖ M ‖ ctr32` with a 4-byte
  big-endian counter appended per XOF block (`…:1495–1515`); Alg. 3 specifies a
  single `XOF(label ‖ data)` stream with no counter. `M` carries no length
  prefix, but the trailing fixed-width counter keeps the encoding unambiguous.
- **(c) equivalent representation** — the secret key stores `T`'s right inverse
  `U` (D×m) plus a kernel basis `K` (s×D) obtained by Gaussian elimination from
  a uniformly random full-rank `T` (`SK_U_ELEMS`/`SK_K_ELEMS`, lines 70–71;
  `random_matrix(T, M, D)` at :1342), instead of the spec's `U ∈ GL_D(F_q)`
  with `T = FirstRows(U, m)` and stored `U^-1`. Completion is
  `y = U·h + Σ_s β_s K_s` with random `β_s` (`sample_completion`, :1797). Same
  affine solution set `{y : T y = h}` and the same element count
  (`D·m + s·D = D²`), hence the identical sk size.
- **(c) restricted search** — Alg. 10 iterates over *all* bounded factor splits
  (Alg. 5) and all `λ ∈ F*_q`. The code takes **one** split per trial
  (`split_factorisation`, subset-sum DP choosing the admissible A-degree
  closest to D/2) and at most `FACTO_MAX_SCALES = 64` scalings (λ = 1 then 63
  random nonzero values) before drawing a fresh completion. Covered by the
  spec's own wording "rescaling schedule Λ ⊆ F*_q" (§1.4.4 d) but not by
  Algorithm 10 as written; it lowers per-trial success probability, which the
  §2.3 failure analysis does not model.
- **(a) omission** — the internal re-check `P(z) = h` (Alg. 10 lines 24–25,
  §1.4.4 step 3f) is **not performed**: `try_split_solution` returns `z` as
  soon as `invert_Q` succeeds, and `sig_sign` emits it directly (:1818–1854,
  :2010–2030). Correctness makes it redundant in the fault-free case, but the
  spec mandates it and it is the natural fault-attack countermeasure.
- **(a) minor** — `EncodePublicKey`/`EncodeSecretKey` are specified to carry a
  "parameter identifier" (§1.4.2 routine table, Alg. 9 line 10). The
  implementation encodes only the raw field-element tables (`PK_BYTES =
  m·C(r+2,3)·2`), the instance being fixed at compile time; consequently
  `DecodePublicKey` cannot "reject" on a parameter mismatch as the spec allows.
- **(c)** `invert_Q_rec` implements Alg. 7 as depth-first backtracking over both
  square roots rather than a breadth-first branch list with a `BranchSchedule`
  cap — it explores at least as many branches, so it is equivalent or stronger.

Not verified (time-boxed): byte-level agreement of `pseudoXOF`/`sm3` with the
submission API spec, the §2.3 failure-probability bound under the restricted
split/scale search, and any estimator claim in §3.2.

Known from `security_findings.md` (not re-derived): Facto-DSA-512 ships **no
KAT file** (NOKAT); -128 and -256 pass KAT. Parameter/estimator validation and
hash-domain auditing are open there as well.

## Subsequent algebraic-recovery review

`sign-10-1` observes that the hidden `n`-dimensional linear subspace
`K2 = ker(L2)` lies in the public zero locus. A random `(n+1)`-dimensional
subspace of the `2n`-dimensional ambient space must intersect it, converting
the specification's `q^n` point search into a restricted polynomial-system
solve. The implementation in `cryptanalysis/` recovers the exact `K2` from
public reduced instances through `n=7` and recovers an equivalent triangular
map at `n=6`. Full-size solving degree and the final container-identification
step are not demonstrated; [report.md](report.md) therefore records a Lead,
not a confirmed forgery.
