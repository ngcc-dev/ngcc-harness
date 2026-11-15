# kem-20 MAMBA-Frost — algorithm summary

Unstructured (FrodoKEM-shaped) lattice KEM whose hardness assumption is
**Learning With Quantization (LWQ)**: the scheme adds *no explicit error at
all*. Every LWE error term is the residual of a **publicly dithered
quantizer** that simultaneously performs ciphertext/public-key compression.
`Frost.PKE` is IND-CPA under Split-LWQ; `Frost.KEM` is the FO transform with
implicit rejection. Messages are embedded with an **E8 lattice code** (8
coordinates per block) to widen the correctness margin.

Specification: `kem-20-spec.pdf` (33 pages), §2 (Algorithms 1–8), §3 (Tables 3–4),
§9.7–§9.8 (bit-exact quantizer and codec), Appendix A/B (LWQ definitions, proofs).

## The quantization function (the novel component)

For each component `x ∈ {pk, u, v}` the profile fixes a width `t_x`; set
`p_x = 2^{t_x}`, `Δ_x = q/p_x`, `g_x = log2 Δ_x`, and the product lattice
`Λ_x = Δ_x Z^d`. A **public dither** `D_x` is expanded from a public seed to be
uniform over `{0,…,Δ_x−1}`. Spec §9.7 gives the exact rule:

```
label       b  = Q_{Λ_x, d}(a) / Δ_x = ((a + d + 2^{g_x-1}) >> g_x) & (2^{t_x} - 1)
compensate  â  = Δ_x·b - d           = ((b << g_x) - d) & (q - 1)
```

so `â = a + e_Q` with `e_Q = Q_Λ(a+d) − (a+d)`. **Theorem 5** (spec p. 26): for
any fixed `a`, if `d` is uniform over a complete residue system mod `Δ`, then
`e_Q` is *independent of `a`* and uniform over `{−Δ/2+1,…,Δ/2}`. **Theorem 6**
then states that the LWQ normal form `Q_Λ(As+d) − d = As + e_Q` is *identically
distributed* to LWE with error distribution `χ_{q,p} = U({−Δ/2+1,…,Δ/2})`, and
that Split-LWQ (which publishes `(A, Q_Λ(As+d), d)`, as the scheme does) is
related to it by the public coordinatewise bijection `(a,d) ↦ Δa − d mod q`, so
the two decision problems have *identical* advantage.

How this replaces the usual LWE noise, and what to watch:
- **No noise sampler exists.** `frost_mul_add_as_plus_e(B_raw, S, E_zero, ρ)`
  (`Frost/src/kem.c:262`, and the two encryption calls at `kem.c:372,395`) is
  always passed an all-zero error matrix `E_zero`. The CBD sampler
  (`noise.c:frost_sample_n`) is used **only for the secrets `S` and `R`**.
- The error is therefore **uniform, not Gaussian/binomial**, and its width is
  *not a free parameter*: `Δ_x = q/2^{t_x}` ties the noise magnitude to the
  compression rate, so noise and bandwidth are a single knob. For Frost-128
  `Δ_pk = Δ_u = 2^5 = 32` but `Δ_v = 2^10 = 1024`.
- This is exactly what LWR's deterministic rounding would give **except** that
  in LWR the rounding error is a deterministic function of `As` (hence not
  independent, which is why LWR needs its own reduction). The dither is the
  whole contribution: it restores the additive-noise structure.
- **Assumption boundary.** Theorems 5/6 assume `d ← U(S)` truly uniform and
  independent of `A`. In the scheme `D_pk` and `A` are both expanded from the
  *same* seed `ρ`, and `(D_u, D_v)` from the ciphertext salt. The IND-CPA proof
  (Appendix B.3) handles this only by "replacing the concrete seed-expansion
  procedures by ideal expansion procedures", charging an unquantified
  `Adv_Exp(C_Exp)` term. So LWQ ⇒ LWE is proved cleanly, but the *instantiated*
  scheme's reduction runs through an idealised XOF, and the document does not
  bound `Adv_Exp`. The proofs of Theorems 5/6 themselves are short and correct.

## Parameters

| parameter | Frost-128 | Frost-192 | Frost-256 | Frost-384 | Frost-512 | meaning |
|---|---|---|---|---|---|---|
| λ | 128 | 192 | 256 | 384 | 512 | security level; also `κ_m = κ_ss = λ` |
| n = m | 512 | 880 | 1288 | 1928 | 2600 | unstructured matrix dimension |
| (ℓr, ℓs) | (8,8) | (8,8) | (8,8) | (8,12) | (8,16) | message-matrix shape (CC variants swap them) |
| q | 2^15 | 2^16 | 2^16 | 2^16 | 2^16 | power-of-two modulus |
| (η_s, η_r) | (2,2) | (1,1) | (1,1) | (1,1) | (1,1) | CBD parameters for the *secrets only* |
| b_msg | 2 | 3 | 4 | 4 | 4 | message bits per E8 coordinate; `b_msg·ℓr·ℓs = λ` |
| (t_pk, t_u, t_v) | (10,10,5) | (11,11,6) | (13,12,8) | (13,13,9) | (14,14,7) | quantization widths → `Δ = q/2^t` |
| log2 δ_dec | −169.44 | −199.84 | −266.61 | −440.52 | −581.12 | DFR upper bound (Prop. 1) |
| claimed security | 131.85 | 197.65 | 257.08 | 393.79 | 512.97 | bits, MATZOV (CoreSVP-C 107.8 / 175.3 / 236.4 / 376.8 / 498.7) |

The `Frost-CC-*` variants keep everything and only exchange `ℓr ↔ ℓs`
(§3.2), which preserves `|pk|+|ct|` and moves bytes from ct to pk. At levels
128/192/256 the swap is the identity, so those three CC instances are
duplicates (see below).

Sizes (bytes), specification (Table 3/4/5) vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | ct spec | ct impl | ss | match |
|---|---|---|---|---|---|---|---|---|
| MAMBA-Frost-128 | 5152 | 5152 | 6736 | 6736 | 5192 | 5192 | 16 | yes |
| MAMBA-Frost-192 | 9712 | 9712 | 11528 | 11528 | 9760 | 9760 | 24 | yes |
| MAMBA-Frost-256 | 16776 | 16776 | 19416 | 19416 | 15552 | 15552 | 32 | yes |
| MAMBA-Frost-384 | 25096 | 25096 | 29032 | 29032 | 37736 | 37736 | 48 | yes |
| MAMBA-Frost-512 | 36432 | 36432 | 41728 | 41728 | 72944 | 72944 | 64 | yes |
| MAMBA-Frost-CC-128 | 5152 | 5152 | 6736 | 6736 | 5192 | 5192 | 16 | yes |
| MAMBA-Frost-CC-192 | 9712 | 9712 | 11528 | 11528 | 9760 | 9760 | 24 | yes |
| MAMBA-Frost-CC-256 | 16776 | 16776 | 19416 | 19416 | 15552 | 15552 | 32 | yes |
| MAMBA-Frost-CC-384 | 37628 | 37628 | 43492 | 43492 | 25204 | 25204 | 48 | yes |
| MAMBA-Frost-CC-512 | 72832 | 72832 | 83328 | 83328 | 36544 | 36544 | 64 | yes |

All ten agree with the spec's formulas `|pk| = 32 + mℓ_r t_pk/8`,
`|ct| = 32 + (nℓ_s t_u + ℓ_r ℓ_s t_v)/8`,
`|sk| = |pk| + nℓ_r t_s/8 + 32 + κ_ss/8` with `t_s = ⌈log2(2η_s+1)⌉`, and with
the spec's own KAT table (Table 5). All 10 instances PASS KAT.

## Pseudocode

### Frost.PKE.KeyGen — spec Algorithm 1
```
ρ            <- U({0,1}^256)
(A, D_pk)    <- GenPublic(ρ)                # A in Z_q^{m x n}, D_pk in {0..Δ_pk-1}^{m x ℓr}
S            <- SampleCBD_{η_s}(n, ℓr)      # the ONLY sampled randomness besides seeds
B            <- Q_{Λ_pk, D_pk}(A S) / Δ_pk  # label matrix in Z_{p_pk}^{m x ℓr}
pk <- (ρ, B) ;  sk_PKE <- S
```

### Frost.PKE.Enc(pk, M, r_Enc) — spec Algorithm 2 (+ 4 for Encode)
```
parse r_Enc as (σ, µ)
(A, D_pk) <- GenPublic(ρ) ;  R <- SampleCBD_{η_r}(σ, m, ℓs)
(D_u, D_v) <- GenDither(µ)
B̂ <- Δ_pk·B - D_pk                (mod q)   # compensated normal form = A S + E_pk
U <- Q_{Λ_u, D_u}(A^T R) / Δ_u
V <- Q_{Λ_v, D_v}(B̂^T R + Encode(M)) / Δ_v
ct <- (µ, U, V)
```

### Frost.PKE.Dec(sk, ct) — spec Algorithm 3 (+ 5 for Decode)
```
(D_u, D_v) <- GenDither(µ)
Û <- Δ_u·U - D_u ;  V̂ <- Δ_v·V - D_v        (mod q)
W  <- V̂ - S^T Û                             (mod q)
M' <- Decode(W)
```

### E8 message codec — spec Algorithms 4, 5, 9, 10 (§9.8)
```
α = q / 2^{b_msg} ;  C_E8 = α·E8 ∩ [0,q)^8
b = (b_msg-1, b_msg, b_msg, b_msg, b_msg, b_msg, b_msg, b_msg+1)   # sums to 8·b_msg
p_E8 = (2^{b_0}, ..., 2^{b_7});  rows of diag(p_E8)·G_E8 generate 2^{b_msg} Z^8,
  so prod_h {0..p_h-1} is a complete set of representatives for E8 / P Z^8.

Label(a_0..a_7):   y <- a·G_E8 ;  c <- α·y mod q            # c in C_E8
Delabel(c):        y <- c/α ;  ã <- y·G_E8^{-1} ;  a_h <- ã_h mod p_h
Encode(M):  split M into N_blk = ℓr·ℓs/8 blocks of 8·b_msg bits, Label each,
            place as a column of Y if ℓr = 8, else as a row.
Decode(W):  per column (resp. row) take the E8 nearest-neighbour point
            Q_{α E8}(w) then Delabel. Decoding succeeds iff the error e obeys
            |<e,r>| < α for every root r of R(E8).
```

### Frost.KEM.KeyGen / Encaps / Decaps — spec Algorithms 6, 7, 8
```
KeyGen:  z <- U({0,1}^κss) ; (pk, sk_PKE) <- PKE.KeyGen()
         h_pk <- H_pk(pk) ;  sk_KEM <- (sk_PKE, pk, h_pk, z)

Encaps:  h_pk <- H_pk(pk) ;  M <- U({0,1}^κm)
         (r_Enc, K') <- G_FO(h_pk || M)
         ct <- PKE.Enc(pk, M, r_Enc)
         K  <- H_K( H_ct(ct) || K' )

Decaps:  M'            <- PKE.Dec(sk_PKE, ct)
         (r'_Enc, K')  <- G_FO(h_pk || M')
         ct'           <- PKE.Enc(pk, M', r'_Enc)
         K <- H_K(H_ct(ct) || K')  if ct' = ct,  else  H_K(H_ct(ct) || z)
         (constant-time comparison and selection are mandated by §2.3 and §9.9)
```

## Implementation vs specification

Checked: `Frost{,-CC}/src/frost*.c` (per-profile constants),
`Frost/src/kem.c` (KeyGen/Encaps/Decaps, quantizer, dither expansion, packing),
`noise.c` (CBD), `frost_e8.h` (Label/Delabel/nearest-neighbour decode),
`util.c` (`ct_verify`/`ct_select`), `frost_macrify_reference.c` (matrix
arithmetic), `config.h` (backend selection). Constants sampled: `PARAMS_N`,
`PARAMS_NBAR_R/S`, `PARAMS_LOGQ`, `PARAMS_ETA_S/R`, `PARAMS_EXTRACTED_BITS`,
`PARAMS_PK_LOGP/U_LOGP/V_LOGP` for Frost-128 and Frost-CC-384 in full, plus the
derived static assertions, which the compiler checks for every profile.

- **Agreement.** Frost-128 `(512, 8, 8, 2^15, η=2, b_msg=2, t=(10,10,5))` and
  Frost-CC-384 `(1928, ℓr=12, ℓs=8, 2^16, η=1, b_msg=4, t=(13,13,9))` match
  Table 3/4 exactly. The quantizer `frost_quantize_local` (`kem.c:131-137`) and
  the compensation in `frost_reconstruct_dithered_profile` (`kem.c:203`) are
  literally the §9.7 formulas. Dither expansion (`kem.c:140-152`) masks 16-bit
  little-endian XOF words to `Δ−1`, which is *exactly* uniform because `Δ` is a
  power of two — no rejection-sampling bias. Dither seeds follow Table 13: `ρ`
  for `D_pk`, the ciphertext salt for `D_u`/`D_v`, with one-byte domain tags.
  CBD (`noise.c:12-28`) is the §9.6 bit-slicing map. The XOF is SHAKE128 for
  Frost-128 and SHAKE256 elsewhere, per §9.5. The E8 decoder
  (`frost_e8.h:127-187`) is a constant-time `D8` nearest-neighbour search over
  two cosets with masked selects, and `ct_verify`/`ct_select`
  (`util.c:112-133`) are the correct 0/−1 FrodoKEM idioms — the implicit
  rejection here is done properly. All ten size formulas reproduce the OBSERVED
  lengths exactly.

- **Discrepancy 1 (real deviation, FO input plumbing).** Spec Algorithm 7
  derives the dither seed from the FO hash: `r_Enc = (σ, µ) ← G_FO(h_pk‖M)`, and
  §2.3 repeats "The value `r_Enc` output by `G_FO` is parsed as `(σ, µ)`". The
  implementation instead draws the salt from the RNG alongside the message,
  `randombytes(mu, BYTES_MU + BYTES_SALT)` (`kem.c:346`), and feeds it *into*
  the hash as `G2in = h_pk ‖ M ‖ salt` (`kem.c:354`), whose output is only
  `(seedSE, K')`. The salt is then appended to the ciphertext. This is
  FrodoKEM's salted-FO construction rather than the written algorithm; the
  re-encryption check still binds because decapsulation re-reads the salt from
  the ciphertext, so it is not an attack, but the specification does not
  describe the deployed transform.

- **Discrepancy 2 (deliberate equivalent optimisation).** Algorithms 7/8 define
  `K = H_K(H_ct(ct) ‖ K')`; the implementation hashes the *ciphertext itself*,
  `shake(ss, ‖ss‖, ct ‖ K')` (`kem.c:413` and `kem.c:573`). Equivalent or
  stronger, but it makes the final hash absorb up to 73 KB at Frost-512.

- **Discrepancy 3 (spec-equivalent, worth noting).** §2.3 requires the equality
  test to compare "the full ciphertext, including … the public seed µ". The
  implementation compares only the `U` and `V` label arrays
  (`kem.c:569`); the salt is copied verbatim from the input ciphertext into the
  re-encryption, so comparing it would be vacuous. Equivalent in effect.

- **Discrepancy 4 (packaging).** `MAMBA-Frost-CC-128` is byte-identical to
  `MAMBA-Frost-128` apart from symbol names and a comment banner, and their
  submitted KAT files are identical (`md5 05853e50…` for both). The same holds
  at 192 and 256. The spec states this openly (§4, last paragraph: the
  directories exist "to keep a uniform API and KAT structure"), so three of the
  ten "instances" are not distinct parameter sets.

- **Build note (not a spec issue).** The sources select the portable matrix
  backend on `USE_REFERENCE`, which `config.h:82-83` derives from the
  `-D_REFERENCE_` the harness passes; `-D_AES128_FOR_A_` selects the AES128
  expansion of `A` (`config.h:90-93`). §9.5 explicitly permits either the AES128
  or the SHAKE matrix backend, provided one is used consistently — so the choice
  is inside the spec's latitude, but it means the KATs are backend-specific.

- **Not verified.** The Chernoff/CGF DFR bound of Proposition 1, the Lattice
  Estimator figures in Table 3, the IND-CPA game hops of Appendix B.3 beyond
  reading them, and the constant-time behaviour of the E8 decoder under an
  optimising compiler. `frost_quantize_dithered_profile` `malloc`s the dither
  buffer on every call and returns 1 on failure; the failure path was not
  exercised.
