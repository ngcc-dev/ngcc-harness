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

Since verified, see "Findings" below: `SamplerPrecomp` (Alg. 7) builds the
wrong Gram matrix, and the public-key packing is not injective.

Still not verified: the fixed-point Q-format
arithmetic in `fpr.c`/`sampler.c` and the `ntrugen` basis completion (Alg. 5)
were not checked against the spec numerically. The rANS frequency tables were
not re-derived. §5 security analysis was not audited.


## Findings

Both are reproduced by `reproduce_transcript_leak.c` (`make -C sign-34 exploit`,
then `tools/reproduce.sh sign-34`), which uses only the uniform ABI and the
published encodings; the secret key is never inspected. All three parameter
sets build and reproduce the submitted KATs (`make -C sign-34 test`: 3 pass).

### sign-34-1 — signatures leak the secret basis (the sampler is not GPV)

`keygen/perturbation.c:sigma_p_set_slot()` builds the per-FFT-slot perturbation
covariance as

```
Sigma_p = sigma^2 I - B B*        with rows b1 = (f,g), b2 = (F^,G^)
    Sigma_p[00] = sigma^2 - (|f|^2 + |g|^2)        <- ||b1||^2
    Sigma_p[11] = sigma^2 - (|F^|^2 + |G^|^2)      <- ||b2||^2
    Sigma_p[01] = -(conj(f) F^ + conj(g) G^)       <- <b1,b2>
```

but the sampler emits `v = z1*b1 + z2*b2`, whose ambient coordinates are
`(z1 f + z2 F^, z1 g + z2 G^)`. Cancelling that requires the Gram of the basis
*columns*, `sigma^2 I - B^T conj(B)`, whose diagonal is
`(|f|^2 + |F^|^2, |g|^2 + |G^|^2)`. `B` is not symmetric, so the two differ and
the error survives into every signature:

```
Var(s1 at slot j) = sigma_eff^2 - |phi_j(g)|^2 + |phi_j(F^)|^2
Var(s2 at slot j) = sigma_eff^2 + |phi_j(g)|^2 - |phi_j(F^)|^2
```

Measured over 20 000 signatures from one yuanyang-512 key, with the slope
**fixed at 1** (nothing fitted but the constant): R² = 0.975 for both halves,
residual rmse 97 against a per-slot signal of sd 558. Free four-term regression
on (|f|², |g|², |F̂|², |Ĝ|²) gives R² = 0.989 with coefficients
(−0.05, −1.07, +1.10, +0.07) — the predicted (0, −1, +1, 0). `Var0 + Var1` is
constant per slot (mean 9997, sd 116) while `Var0 − Var1` has sd 1224.

Consequences: the per-slot variance of `s1` spans a factor ≈ 4 (yuanyang-512:
2748…11061 against σ_sig² = 4867), the transcript is not simulatable from the
public key, and the EUF-CMA reduction of spec §5 does not apply. An attacker
who sees signatures learns an affine image of the secret basis' Gram —
`|phi_j(g)|² − |phi_j(F^)|²` for every slot. A cross-key control confirms it is
key-dependent, not a fixed artefact: one key's transcript profile against
another key's basis gives corr = +0.011, against its own, −0.78.

Signatures needed (yuanyang-512, fixed-slope R²): 50 → 0.41, 100 → 0.61,
300 → 0.83, 1000 → 0.93, 3000 → 0.96. Present in all three sets at N = 1000:
yuanyang-512 R² = 0.926, -1024 R² = 0.963, -2048 R² = 0.932.

This report demonstrates the leak and identifies its source; it does not carry
it through to a full `(f,g)` recovery, which would need the recovered Gram to
be fed to a lattice step.

### sign-34-2 — the public-key encoding is not canonical

`yuanyang_encode_uniform(h, d, q, k=4, ...)` packs four coefficients into a
46-bit word, but `q^4 = 52 283 326 179 841 < 2^46 = 70 368 744 177 664`.
`yuanyang_decode_uniform()` recovers the coefficients with repeated
`word % q`, so any block whose word is below `2^46 − q^4` (25.7 % of blocks)
has a second encoding, `word + q^4`, that decodes to exactly the same four
coefficients. `yuanyang_decode_public_key()` only checks `h[i] < q`, which both
encodings satisfy, so roughly 2^33 distinct 738-byte strings per key decode to
the same `h` and accept the same signatures. The same packing is used for the
`h` prefix of the secret key. The earlier note in this file that public-key
canonicality is enforced is therefore wrong: coefficient range is checked, the
encoding is not. Affects anything that fingerprints, pins or compares
serialised public keys. The 1024 and 2048 sets use 49- and 52-bit blocks with
the same slack.

### Lower-severity observations

- `keygen/pairgen.c:fill_rand_doubles()` derives every uniform in PairGen from
  a **single byte** (`u = byte/256`), so `u_rho`, `u_theta`, `theta_x`,
  `theta_y` each take only 256 values instead of the specification's continuous
  `U(0,1)`. The comment defends this as "~2 bits of entropy" per coordinate.
  Total keygen entropy (8192 bits at d = 512) does not collapse, but the FFT
  magnitudes and angles of `(f,g)` live on a coarse 8-bit grid, which is not
  the sampled distribution the annular trapdoor analysis assumes.
- `keygen/perturbation.c` bounds the Σ_δ coefficients by `INT32_MIN/MAX`, but
  `encode_scaled_sigma_delta_mat2()` writes them as **22-bit** signed fields
  (`write_bits_le(..., 22, z & MASK)`). Any coefficient outside
  `[-2^21, 2^21)` is silently truncated instead of rejected.
- `sign.c` never range-checks `s2 = c − v1` against `(q−1)/2`, although
  `yuanyang_sign_stats` still carries the unused `s2_range_reject` counter.
  Verification recovers `s2` by centre-lifting mod q, so a coefficient outside
  that range would make a signature the signer accepted fail verification.
  At σ_sig ≈ 70 the event is ~19σ and does not occur in practice.
