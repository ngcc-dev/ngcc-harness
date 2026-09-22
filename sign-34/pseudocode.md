# sign-34 YuanYang.DSA — algorithm summary

YuanYang.DSA is a GPV hash-and-sign lattice signature over NTRU lattices in
R = Z[X]/(X^d+1) — the Falcon/Antrag/Solmae family. KeyGen produces a short
NTRU basis B = [(f,g) | (F,G)] with f·G − g·F = q, public key h = g/f mod q;
signing samples a lattice Gaussian near the target c = (0, H(ω‖M‖pk)) and
outputs the short syndrome s. Two design points distinguish it: the *annular*
trapdoor generation of Antrag (sampling f, g directly through their FFT
magnitudes in an annulus), and a **floating-point-free online sampler** — the
Gram root is computed once in KeyGen, rounded to fixed precision (Â), and the
quantization error corrected by a second rejection step. Security reduces to a
t-R-ISIS assumption relative to the NTRU trapdoor generator (spec Def. 5.1,
Thm. 5.2).

Specification: `sign-34-spec.pdf` (40 pages), §4 (Algorithms 1–24; KeyGen
Alg. 1, PairGen Alg. 2, UnifCrown Alg. 3, SecurityLoss Alg. 4, NTRUSolver
Alg. 5, SamplerPrecomp Alg. 7, Sign Alg. 8, Sample Alg. 10, Verif Alg. 14,
SamplerZ Alg. 17, BaseSampler Alg. 18, rANS Alg. 21–24), Table 3 (§4.5)
parameters, Table 6 (§6.2) sizes.

## Parameters

| parameter | YuanYang.DSA-512 | -1024 | -2048 | meaning |
|---|---|---|---|---|
| d | 512 | 1024 | 2048 | ring degree; lattice dim 2d |
| q | 2689 | 4481 | 7681 | NTRU modulus |
| λ | 128 | 256 | 512 | security parameter |
| salt (λ+80 bits) | 208 b = 26 B | 336 b = 42 B | 592 b = **74 B** (impl 72 B) | ω |
| η (smoothing) | 1.026 | 0.974 | 0.928 | base Gaussian width |
| ϵ | 2⁻²⁹ | 2⁻²⁶ | 2⁻²³·⁵ | smoothness |
| (p1, p2) | (2¹¹, 2²²) | (2¹¹, 2²²) | (2¹¹, 2²²) | precision of û / Â |
| Norm_fg | 2840 | 5076 | 9000 | minimum ‖(f,g)‖² accepted |
| LossBound | 3.8 | 22 | 128 | SecurityLoss filter |
| α (quality) | 1.29 | 1.4 | 1.43 | annulus target quality |
| R⁻ / R⁺ | 49.1 / 58.0 | 63.12 / 78.41 | 82.63 / 103.98 | annulus radii |
| σ_sig | 69.768 | 92.40825 | **128.262** (see below) | signature width |
| τ (slack) | 1.047 | 1.033 | 1.010 | γ = τ·σ_sig·√(2d) |
| γ | 2337 | 4317 | 7641 | norm bound |

Sizes (bytes), specification Table 6 vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | sig spec | sig impl | match |
|---|---|---|---|---|---|---|---|
| yuanyang-512 | 738 | 738 | 15 584 | 15 584 | 561 | 561 | yes |
| yuanyang-1024 | 1570 | 1570 | 31 264 | 31 264 | 1150 | 1150 | yes |
| yuanyang-2048 | 3330 | 3330 | 62 720 | 62 720 | 2364 | 2364 | yes |

## Pseudocode

### KeyGen (spec Algorithm 1; subroutines Alg. 2–7)
```
 1  repeat
 2      repeat
 3          (f,g) <- PairGen(q, α, R⁻, R⁺)            # Alg. 2
 4          loss  <- SecurityLoss(f,g)                # Alg. 4
 5      until ‖(f,g)‖² ≥ Norm_fg ∧ loss ≤ LossBound ∧ f ∈ R₂ˣ ∧ f ∈ R_qˣ
 6      (F,G) <- NTRUSolver(q, f, g)                  # Alg. 5, f·G − g·F = q
 7  until (F,G) != ⊥
 8  B <- [(f,g) | (F,G)]
 9  h <- g/f mod q
10  (û, B̂⁻¹, Â, Σ_δ, r, C) <- SamplerPrecomp(B, σ, η, p1, p2)   # Alg. 7
11  sk <- (B, û, B̂⁻¹, Â, Σ_δ, r, C) ;  pk <- (q, h, σ_sig, η, γ)

PairGen (Alg. 2):
  for i = 1..d/2:  (x_i, y_i) <- UnifCrown(R⁻, R⁺); θx, θy <-$ U(0,1)
                   φ_{f,i} <- |x_i|·e^{2iπθx} ;  φ_{g,i} <- |y_i|·e^{2iπθy}
  (f_R, g_R) <- FFT⁻¹(φ_f), FFT⁻¹(φ_g);  (f,g) <- (round f_R, round g_R)
  restart unless  q/α² ≤ |φ_i(f)|² + |φ_i(g)|² ≤ α²q  for all i ≤ d/2
  # R⁻ = ((1/3)α + (2/3)/α)√q ,  R⁺ = ((2/3)α + (1/3)/α)√q  (narrowed for rounding)
```

### Sign (spec Algorithm 8)
```
 1  repeat
 2      repeat
 3          ω <-$ {0,1}^{λ+80}
 4          m <- H(ω ‖ M ‖ pk)
 5          c <- (0, m)
 6          v <- Sample(c, sk)              # Alg. 10, v ~ D_{L_NTRU, c, σ_sig}
 7          (s1, s2) <- c − v
 8      until ‖(s1,s2)‖² ≤ γ²
 9      str <- Compress(s1)                 # Alg. 23, rANS, fixed length s_len
10  until str != ⊥
11  return (ω, str)

Sample (Alg. 10): p <- Presampler(Â)           # perturbation, covariance ÂÂᵗ+I
                  ĉ <- B̂⁻¹(c − p)
                  z2 <- RingSamp(ĉ2); c1' <- ĉ1 − z2·û; z1 <- RingSamp(c1')
                  two rejection steps with
                     Δ1 = ρ_{η,c'}(R²)/ρ_η(R²)
                     Δ2 = (1/C)·exp( (z−c)ᵗ T (z−c) / (2σ_sig²) ),  T = Σ_{i≤k}(Σ_δ/σ²)^i
```

### Verify (spec Algorithm 14)
```
1  s1 <- Decompress(s)                 # Alg. 24; ⊥ -> Reject
2  c  <- H(ω ‖ M ‖ pk)
3  s2 <- c + h·s1 mod q                # from the relation s2 − h·s1 ≡ c
4  if ‖(s1, s2)‖² > γ² : Reject
5  Accept
```

### Integer Gaussian sampling / coding
```
SamplerZ(µ, η̄) (Alg. 17):  a=⌊µ⌋; c=µ−a
   loop: z0 <- BaseSampler(η̄); b <-$ {0,1}; z <- b + (2b−1)z0
         x <- ((z−c)² − z0²)/(2η̄²)
         if BerExp(x,1)=1: return z+a
BaseSampler (Alg. 18): u <-$ {0..2⁹⁶−1}; z0 = Σ_i [u < RCDT_{η̄}[i]]
   (96-bit thresholds as three 32-bit limbs; separate tables for η and 4η)

Compress (Alg. 23): per coefficient — sign bit σ_i, ℓ low bits l_i, high part
   h_i = |s_i| >> ℓ; h's rANS-coded, σ‖l stored raw; str = rANS ‖ aux ‖ 1 ‖ 0*,
   padded to s_len; ⊥ if h_i ∉ H or |str| > s_len.
Decompress (Alg. 24) mirrors it and rejects on: wrong length, no terminator,
   short body, rANS ⊥, or a_i = 0 with σ_i = 1 (negative zero).
rANSDecode (Alg. 22) additionally rejects unless the final state equals x_init
   and the input is fully consumed.
```

Hash: the spec writes `H : {0,1}* -> R_q` abstractly. The submitted code
implements it as `seed = SM3(M ‖ h)` followed by
`c = KDF-SM3(ω ‖ seed)` parsed by rejection mod q (all from the ICCS auxfunc).

## Implementation vs specification

Checked, in `Implementations/Reference_Implementation/yuanyang-{512,1024,2048}`:
`yuanyang_params.h` and `keygen/constants.h` (parameters, sizes),
`keygen/{keygen,pairgen,security_loss}.c` and `keygen/ntrugen/*` (Alg. 1–7),
`sign.c`/`sign_ntt.c`/`sampler.c` (Alg. 8, 10, 17–20), `vrfy.c` (Alg. 14),
`codec.c` (Alg. 21–24 and the key encodings), `prng.c`,
`SIG_AlgorithmInstance.c`. Not executed.

Agreements:
- All three parameter sets match Table 3 on d, q, α, R⁻, R⁺, Norm_fg
  (`YYKG_NORM_FG_MIN` = 2840 / 5076 / 9000), LossBound
  (`YUANYANG_SL_LOSS_BOUND` = 3.8 / 22 / 128), and γ
  (`YUANYANG_REJECTION_BOUND` = 2337 / 4317 / 7641). Spot-checked every one of
  these for all three instances.
- Sizes: `YUANYANG_PK_BYTES = 2 + (d/4)·{46,49,52}/8` = 738/1570/3330;
  `YUANYANG_SK_BYTES` (packed h + 4·d signed-8-bit basis coefficients + û at
  2 B/coef + three Â polys at 4 B/coef + a 2×2 Σ_δ at 22 bits/coef) =
  15584/31264/62720; `salt + COMP_SIG_BYTES` = 561/1150/2364. All match
  Table 6 and the built library exactly.
- Verification (vrfy.c) is Algorithm 14 and nothing is skipped: exact
  `sn_len_bytes == YUANYANG_SIGNATURE_BYTES` check, Decompress with all of
  Alg. 22/24's rejections present (`codec.c:1006`
  `if (state != YUANYANG_RANS_BYTE_L || ptr != end) reject`, plus the
  negative-zero and length checks), public-key canonicality enforced
  (`yuanyang_decode_public_key` rejects `h[i] >= q` and a wrong q prefix),
  then `s2 = c + h·s1 mod q`, centre-lift, and `‖(s1,s2)‖² ≤ γ²`. The result is
  propagated by `sig_verify` (SIG_AlgorithmInstance.c:64).
- Randomness comes from the shared seeded DRNG in both KeyGen and Sign:
  `prng_init(&rng, drng_randombytes, &drng_algorithm)`
  (`keygen/keygen.c:79`, `sign.c:590`). The optional `getrandom()` backend is
  compiled out because it requires `YUANYANG_FORCE_SYSTEM_RANDOMNESS` *and*
  the absence of `NGCC_KATS`, and the build defines `-DNGCC_KATS` (prng.c:12).

Discrepancies (all on the specification side except the last two):
- **(a) Salt length at d = 2048: spec Table 3 says λ+80 = 592 bits (74 bytes),
  the code uses 72 bytes.** `yuanyang-2048/yuanyang_params.h:13`
  `#define YUANYANG_SALT_BYTES 72u` — i.e. 576 bits = λ+64. The 512 and 1024
  sets do match (26 B = 208 b, 42 B = 336 b). Note that Table 6's signature
  size, 2364 B = 72 + 2292, is consistent with the *code*, so Table 3's salt
  row is the outlier; but as written, the implementation gives 16 fewer salt
  bits than the specification requires at the highest level. (The relevant
  multi-target/collision margin is unaffected in practice at 576 bits.)
- **(b) Spec Table 3 gives σ_sig = 128.262 for d = 2048, which is inconsistent
  with its own γ and with the code.** γ = τ·σ_sig·√(2d) with τ = 1.010 and
  √4096 = 64 gives 8290, not the tabulated 7641. The implementation's constant
  `fpr_yuanyang_sigma_sig` decodes to σ_sig ≈ 118.22 (and
  `fpr_yuanyang_inv_2sqrsigma_sig` = 1/(2σ_sig²) agrees), which reproduces
  γ = 1.010·118.22·64 = 7641 exactly and equals η·σ = 0.928·127.5 to 0.1 %.
  So Table 3's σ_sig entry for the 2048 set is a typo; the code is the
  self-consistent side. The 512 and 1024 rows are consistent (1.026·68 = 69.768,
  0.974·(1518/16) = 92.408).
- **(b) Spec Tables 1 and 2 label the d = 2048 column "η = 0.899" / "4η = 3.596",
  contradicting Table 3's η = 0.928.** The thresholds printed in those tables
  are byte-for-byte the ones shipped in `yuanyang-2048/sampler.c`
  (`yuanyang_small_rcdt[0..2]` reassembles to
  31588619392639700572488302970, exactly Table 1's first d=2048 entry), and
  that value corresponds to Pr[z₀ ≥ 1] = 0.39871, i.e. η = 0.928, not 0.899
  (η = 0.899 would give 0.3928). The code's `fpr_yuanyang_eta` likewise decodes
  to 0.928 and `fpr_yuanyang_inv_2sqr_large_sampler_sigma` to 1/(2·3.712²).
  The column headers of Tables 1–2 are therefore wrong; the tables' contents
  and the code are correct and mutually consistent.
- **(a, minor, dead constant) `yuanyang-512/yuanyang_params.h:86`
  `#define NORM_FG_MIN 2964u`** contradicts the spec's Norm_fg = 2840. It is
  unreferenced — `pairgen.c:185` uses `YYKG_NORM_FG_MIN` from
  `keygen/constants.h`, which is 2840 — so behaviour is correct, but a stale
  duplicate with a different value is a trap for anyone re-tuning the
  parameter set. The 1024/2048 params.h files do not carry the duplicate.
- **(c, equivalent) the hash is two-stage, not the spec's single H(ω‖M‖pk).**
  `vrfy.c` / `sign.c` compute `seed = SM3(M ‖ h)` then
  `c = KDF-SM3(ω ‖ seed)` (mirroring the submission's own Python reference).
  All three inputs are bound, the salt is absorbed after the message, and the
  public key enters only through `h` (not through q, σ_sig, η, γ — those are
  fixed per instance). Functionally equivalent for the security argument, but
  it is not literally Algorithm 8 line 4 / Algorithm 14 line 3.
- **(a, minor) `vrfy.c:31` `signature_and_key_values_fit_ntt_input` tests
  `s1[i] > q || h[i] > q`** — an off-by-one (`>` instead of `>=`) and, for the
  signed `s1`, no lower bound. It is a redundant guard: `h` has already been
  validated `< q` by `yuanyang_decode_public_key`, `s1` is bounded by the
  Decompress symbol set, and the norm test bounds it again. No exploitable
  path was found, but the guard does not do what its comment says.

Subsequent review verified that `SamplerPrecomp` (Alg. 7) is implemented with
the wrong basis orientation: `sigma_p_set_slot()` subtracts the row Gram
matrix, while the specification's column-basis convention requires
`B_hat B_hat^*`. Public signatures reproduce the resulting key-dependent
anisotropy; see `sign-34-1` in [report.md](report.md). The same review confirmed
the non-injective radix-`q` public-key packing recorded as `sign-34-2`.

The two rejection probabilities Δ1 and Δ2, the remaining fixed-point Q-format
arithmetic, and the `ntrugen` basis completion (Alg. 5) were not checked
numerically. The rANS frequency tables were not re-derived. The remainder of
§5 was not audited.
