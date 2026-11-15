# sign-02 BiT — algorithm summary

BiT ("Bimodal Triangular") is a Fiat–Shamir-with-aborts lattice signature over
`R_q = Z_q[x]/(x^d+1)`, with security reduced to MLWE (key indistinguishability) and
BimodalSelfTargetMSIS / MSIS (unforgeability). Its distinguishing feature is a *bimodal*
response `z = y + (-1)^b s c` in which the masking vector `y` is drawn from a **triangular**
distribution (difference of two uniform integers — no Gaussian/floating point), with rejection
sampling from a triangular source onto a trapezoidal target. The bimodal trick requires the key
relation `A s = q̂ j_n + b_0` with `q̂ = 2^-1 mod q`.

Specification: `sign-02-spec.pdf`, 39 PDF pages. Framework §1.2 Fig. 2 (printed p.8);
compressed scheme §3.2 Fig. 4 (printed p.15 / PDF p.16); concrete pseudocode §3.5 Fig. 5
(printed p.18 / PDF p.19). Parameters: Table 3 (printed p.21) and Table 9 (printed p.33).

## Parameters

Spec Table 9 (printed p.33) — the three submitted instances:

| parameter | BiT-128 | BiT-256 | BiT-512 | meaning |
|---|---|---|---|---|
| d | 256 | 512 | 1024 | polynomial degree (`BIT_N`) |
| q | 26881 | 119297 | 520193 | NTT-friendly prime modulus |
| (n, m) | (3,3) | (3,3) | (3,3) | module dimensions (`BIT_K`, `BIT_L`) |
| η | 1 | 1 | 1 | secret coefficient bound, `s0,e ∈ S_η` |
| τ | 30 | 58 | 115 | Hamming weight of challenge `c ∈ B_τ` |
| β = ητ | 30 | 58 | 115 | `‖sc‖_∞` bound |
| γ_{1,0} | 2^3 | 2^4 | 2^4 | mask width for `y_0` (masks the constant 1) |
| γ_{1,1} | 2^10 | 2^12 | 2^13 | mask width for `y_1` (masks `s_0 c`) |
| γ_{1,2} | 2^11 | 2^13 | 2^15 | mask width for `y_2` (masks `e c`) |
| γ_2 | 682 | 5517 | 69481 | HighBits granularity |
| γ_b | 2^4 | 2^6 | 2^6 | public-key compression granularity |
| B_∞ | 2^11 | 2^13 | 2^17 | signature norm bound |
| ‖h‖_∞ (B_h) | 3 | 3 | 1 | hint bound |
| acceptance rate | 18% | 53% | 33% | expected 1/M |
| claimed security | 128 (MLWE 129 / MSIS 155, SUF 128) | 256 (256 / 301, SUF 256) | 512 (512 / 597, SUF 512) | classical core-SVP bits |

Sizes (bytes), specification (Table 1 printed p.9 = Table 9) vs the built reference library
(OBSERVED). The spec gives no secret-key size.

| instance | pk spec | pk impl | sk spec | sk impl | sig spec | sig impl | match |
|---|---|---|---|---|---|---|---|
| BiT-128 | 1048 | 1048 | — | 1864 | 1504 | 1504 | yes |
| BiT-256 | 2144 | 2144 | — | 4160 | 3456 | 3456 | yes |
| BiT-512 | 5056 | 5056 | — | 9024 | 6695 | 6695 | yes |

All six published sizes agree exactly. (Table 9's `pk+sig` cell for level 128 reads `332552`;
Table 1 gives `2552 = 1048+1504`, so Table 9 has a typographic defect, not a size disagreement.)

## Pseudocode

Transcribed from §3.5 Figure 5 (printed p.18). `H` = SM3 (Table 2); `XOF` = SM3-based stream;
`q̂ = 2^-1 mod q`; `j_n = (1,0,…,0)^T`; `HighBits(w,γ) = (w − (w mod γ))/γ`, `LowBits(w,γ) = w mod γ`
(§2.6). `SampleInBall` (§2.5) expands `c̃` into `c ∈ B_τ` by the Fisher–Yates-style loop
`for i = d−τ … d−1: j ← {0..i}; c_i := c_j; c_j := 1`
(the §1.2 overview describes the nonzero coefficients as {±1}; §2.5 and `poly.c:200` use 1).

### KeyGen  (Fig. 5, KGen)
```
1:  ρ, ρ' ← {0,1}^κ
2:  A_0 ∈ R_q^{n×m} := ExpandA(ρ)
3:  (s_0, e) ∈ S_η^m × S_η^n := ExpandS(ρ')
4:  b := A_0 s_0 + e
5:  (b_1, b_0) := (HighBits(b, γ_b), LowBits(b, γ_b))
6:  tr := H(pk)
7:  return pk := (ρ, b_1),  sk := (ρ, b_1, ρ', tr, s_0, e, b_0)
    # implied verification matrix A = (q̂ j_n − γ_b b_1, A_0, I_n) = (A_1, I_n),
    # signing key s = (1, s_0, e), so that A s = q̂ j_n + b_0.
```

### Sign  (Fig. 5, Sign)
```
 1: A_0 := ExpandA(ρ);  A := (q̂ j_n − γ_b b_1, A_0, I_n);  s := (1, s_0, e)
 4: µ := H(tr ‖ M)
 6: rnd ← {0,1}^κ ;  seed_y := XOF(ρ' ‖ rnd ‖ µ) ;  counter := 0
 8: y := (y_0, y_1, y_2) ∈ T_{γ1,0} × T_{γ1,1}^m × T_{γ1,2}^n := ExpandMask(seed_y, counter)
 9: w := A y ;  counter := counter + m
10: w_1 := HighBits(w, γ_2)
11: c̃ := H(µ ‖ w_1)
12: c := SampleInBall(c̃) ∈ B_τ
13: b ← {0,1}
14: z := y + (−1)^b s c ;  (z_{1,0}, z_{1,1}, z_2) := Split(z)
16: z_{1,0} := RejectSample(z_{1,0}, γ_{1,0}, c,      1)     # only the τ nonzero coeffs
17: z_{1,1} := RejectSample(z_{1,1}, γ_{1,1}, s_0 c,  β)
18: z_2     := RejectSample(z_2,     γ_{1,2}, e c,    β)     # any ⊥ ⇒ restart at 8
19: if HighBits(w, γ_2) ≠ HighBits(w + (−1)^b j_n c, γ_2): restart at 8   # hides b
20: ŵ := w − z_2 + (1−b) j_n c + (−1)^b b_0 c
21: h := HighBits(w, γ_2) − HighBits(ŵ, γ_2)
22: if ‖h‖_∞ ≥ B_h: restart at 8
23: if ‖(z_1, γ_2 h)‖_∞ > B_∞: restart at 8
24: return σ := (z_1, h, c̃)            where z_1 = (z_{1,0}, z_{1,1})
```
`RejectSample` (Fig. 3, printed p.12): for each coefficient with offset `v_i`, `β_1,i := |v_i|`;
accept `z_i` with probability `G(z_i, γ, β_2)/F(z_i, γ, β_1,i)`, where `F` is the bimodal
triangular source and `G` the trapezoidal target (§2.4); coefficients in the overlap region
`[−γ+β_1, −β_2] ∪ [β_2, γ−β_1]` are accepted outright, only the rest need a coin flip.

### Verify  (Fig. 5, Verify)
```
1: A_0 := ExpandA(ρ) ;  A_1 := (q̂ j_n − γ_b b_1, A_0)
3: tr := H(pk) ;  µ := H(tr ‖ M)
5: c := SampleInBall(c̃)
6: ŵ' := A_1 z_1 + q̂ j_n c
7: w'  := HighBits(ŵ', γ_2) + h
8: accept iff  ‖(z_1, γ_2 h)‖_∞ ≤ B_∞   and   c̃ = H(µ ‖ w')
```

Hashing: `H` is "instantiated as SM3" (Table 2, printed p.17) and Fig. 5 types `µ` and `c̃` as
κ-bit strings; the implementation's `H` output is 256 bits for BiT-128 and 512 bits for
BiT-256/BiT-512 (see below). The
transcript is `µ = H(H(pk) ‖ M)` then `c̃ = H(µ ‖ w_1)`, i.e. the public key is bound only
indirectly through `tr`. Challenge compression is two-stage: `c̃` (short, in the signature) →
`SampleInBall`/XOF → `c ∈ B_τ` (§2.5).

## Implementation vs specification

Checked against `src/BiT-128` (= `Implementations/Reference_Implementation/BiT-128`), which the
NGCC `Makefile` builds with no `DEFS_`, plus a parameter diff against `BiT-256`/`BiT-512`.
Step→file map: `sign.c:24` `bit_sig_keygen` (Fig. 5 KGen), `sign.c:62` `bit_sig_sign`,
`sign.c:217` `bit_sig_verify`; rejection tests in `sample.c:235,263,278,300`; encodings in
`packing.c`; `HighBits`/hint in `polyvec.c`; `symmetric.c` for `H`/XOF.

Agreements:
- Parameter spot-check (13 constants, BiT-128 `params.h:11–61`, cross-checked for 256/512):
  `BIT_N 256`, `BIT_Q 26881`, `BIT_K 3`, `BIT_L 3`, `BIT_TAU 30`, `BIT_BETA 30`,
  `BIT_GAMMA1_0 8`, `BIT_GAMMA1 1024`, `BIT_GAMMA1_2 2048`, `BIT_GAMMA2 682`,
  `BIT_GAMMA_B 16`, `BIT_B_INF 2048`, `BIT_H_INF 3` — all match Table 9 exactly.
  `BIT_Q_HAT 13441 = 2^-1 mod 26881` ✓.
- Transcript order matches Fig. 5: `µ = H(tr‖M)` (`sign.c:93`, and `sign.c:249–250` in verify
  where `tr` is recomputed from `pk`), `c̃ = H(µ‖w1)` (`sign.c:94–95,130–131`; verify
  `sign.c:264–268`), `seed_y = XOF(ρ'‖rnd‖µ)` (`sign.c:100–103`). Randomised signing, as claimed.
- `H` defaults to SM3 (`symmetric.h:14` `BIT_USE_SHAKE 0`, `symmetric.c:31`), matching Table 2;
  BiT-256 and BiT-512 instead call `bit_h512` = `pseudohash(512, …)` (`BiT-256/symmetric.c:65–70`,
  `BiT-256/sign.c:53,94,129`; `BIT_CHALLENGEBYTES`/`BIT_MESSAGEBYTES`/`BIT_TRBYTES` = 64);
  the FIPS-202 variant of Table 8 is a compile-time alternative and is not what NGCC builds.
- Signature layout (`packing.c:266` `pack_sig`) is `c̃ ‖ z_{1,0} ‖ z_{1,1} ‖ h`, sizes
  32 + 128 + 3·352 + 3·96 = 1504, matching `BIT_SIGNBYTES` and the spec's 1504.
- Hint decoding is canonical-checked: `unpack_polyveck_h_bits` (`packing.c:28–43`) rejects the
  unused 3-bit code 7 (`packing.c:38`), and `unpack_sig` propagates the failure
  (`packing.c:343`). `bit_sig_verify` rejects `siglen != BIT_SIGNBYTES` (`sign.c:238`), so there
  are no trailing/ambiguous bytes. This is consistent with the "no trivial malleability found"
  entry already recorded in `security_findings.md`; there is no `review-o48.md` for sign-02.
- Sizes: all pk/sig values in OBSERVED are exactly the `params.h` expressions and exactly the
  spec's Table 9 figures. No size mismatch.

Discrepancies / notes:
- **(a) partial re-randomisation of the mask.** Fig. 5 steps 16–18 say every rejection returns to
  step 8, i.e. a *fresh* `y = (y_0, y_1, y_2)`. The implementation, on the first rejection branch
  (`z_{1,0}/z_{1,1}`), resamples only `y_0` and `y_1` (`sign.c:154–157`,
  `polyvecy_sample_y0`/`_y1`) and **reuses `y_2` from the rejected iteration**. Because `c`
  depends on `w`, which depends on `y_2`, conditioning on the rejection event can bias the reused
  `y_2`. The later branches (`sign.c:167–196`) do resample all of `y`. §3.1.2 motivates
  partitioned rejection sampling but does not authorise reusing a mask component across
  iterations, and no analysis of this optimisation appears in the spec. Not exploited here —
  flagged as an unanalysed deviation.
- **(b) spec-internal inconsistency in the norm bound.** §3.2.1(3) defines
  `B_∞ = max{γ_{1,1} − 1, γ_2‖h‖_∞}`, but Table 9's `B_∞` equals `γ_{1,1}` alone (e.g. 2^13 for
  BiT-256, whereas `γ_2‖h‖_∞ = 5517·3 = 16551`). The implementation resolves this by checking the
  two bounds *separately* — `‖z_1‖_∞ ≤ B_∞` and `‖h‖_∞ ≤ B_h` in one pass
  (`sample.c:278–298`, `check_reject_norm`) — rather than the single `‖(z_1, γ_2 h)‖_∞ ≤ B_∞` of
  Fig. 5 step 23. Verify applies the same combined test (`sign.c:246`), so signer and verifier
  agree and correctness is unaffected; it is the spec that is ambiguous.
- **(b) stale parameter table.** Table 3 (printed p.21) and Table 9 (printed p.33) disagree for
  the 256 set: `γ_{1,0} = 2^3` vs `2^4` and `‖h‖_∞ = 2` vs `3`. The implementation
  (`BiT-256/params.h`: `BIT_GAMMA1_0 16`, `BIT_H_INF 3`) follows Table 9; Table 3 appears stale.
- **(c) harmless redundancy.** `check_reject_hint_range` (`sample.c:300`) re-tests exactly the
  hint half of `check_reject_norm`; step 22 of Fig. 5 is therefore applied twice
  (`sign.c:188`, `sign.c:193`). No effect on output.
- Not verified in this pass: the coefficient-level probability arithmetic inside
  `reject_sample_coeffs` (`sample.c`) against the exact `G/F` ratio of §2.4/Fig. 3; the base-1681
  packing of `b_1` in the public key (`packing.c:45–…`) for canonical decoding; the NTT
  correctness and the Barrett constants; and Table 10's 80/192/384-bit sets, which are not built.
