# kem-21 MAMBA-Viper — algorithm summary

Module-lattice KEM over `R_q = Z_q[X]/(X^256+1)` with power-of-two `q`, whose
hardness assumption is **Module Learning With Quantization (MLWQ)**: like its
sibling kem-20 Frost, Viper samples **no explicit error** — every error term is
the residual of a **publicly dithered quantizer** that simultaneously
compresses. `Viper.PKE` is IND-CPA under Split-MLWQ (which the spec proves
decision-equivalent to normal-form MLWE); `Viper.KEM` is the FO transform with
implicit rejection, proved in ROM and QROM. Messages are embedded through a
**scaled E8 lattice code** and recovered by nearest-neighbour decoding.

Specification: `kem-21-spec.pdf` (38 pages), §2 (Algorithms 1–8), §3 (Tables 3–5),
§8.5–§8.8 (bit-exact dithers, quantizer, E8 maps), Appendix A (MLWQ definitions,
Theorems A.4–A.7).

## The quantization function (the novel component)

For `x ∈ {pk, u, v}`: `p_x = 2^{t_x}`, `Δ_x = q/p_x`, `g_x = log2 Δ_x`,
`Λ_x = Δ_x Z^n`, and the dither `d_x` has coefficients uniform in `{0,…,Δ_x−1}`.
Spec §8.7 fixes the bit-exact rule (coefficientwise, `q = 2^{QLOG}`):

```
label        b  = Q_{Δ_x Z, d}(a)/Δ_x = ((a + d + 2^{g_x-1}) >> g_x) & (2^{t_x} - 1)
compensate   â  = Δ_x·b - d           = ((b << g_x) - d) & (q - 1)
```

**Theorem A.4**: for any fixed `y ∈ Z_q`, if `d ← U({0,…,Δ−1})` then
`e = Q_{ΔZ,d}(y) − d − y` is uniform over `{−Δ/2+1,…,Δ/2}`, *independent of `y`*,
with **mean 1/2** and variance `(Δ²−1)/12`. Corollary A.5 lifts this
coefficientwise to `R_q^m`. **Theorem A.6**: normal-form MLWQ *is* MLWE with
error distribution `χ_{q,p} = U({−Δ/2+1,…,Δ/2})` — "the reduction is the
identity map". **Theorem A.7**: Split-MLWQ (which is what Viper transmits: the
label `c` plus the public dither `d`) and normal-form MLWQ are related by the
coefficientwise bijection `(c,d) ↦ Δc − d mod q`, with equal advantage in both
directions. So the published "quantization labels" are a *lossless
re-encoding* of an MLWE transcript, and the whole novelty sits in Theorem A.4.

What this replaces and what it costs:
- **There is no error sampler.** `viper_pke_keypair` computes `b ← A s` and
  immediately quantizes (`viper.c:502-504`); no `e` is added anywhere. The CBD
  sampler `viper_sample_secret` is used only for `s` and `r`.
- The error is **uniform and biased** (mean 1/2, not 0) and its width is *not a
  free parameter*: `Δ_x = q/2^{t_x}` welds the noise magnitude to the
  compression rate. At Viper-128 `Δ_pk = Δ_u = 8` while `Δ_v = 256`.
- The difference from MLWR is exactly the dither: without it the rounding error
  is a deterministic function of `As`, so Theorem A.4 fails. **The uniformity of
  the dither over the full residue system mod Δ is therefore load-bearing** —
  see Discrepancy 1, where the implementation gets this wrong at Viper-128.
- **Assumption boundary.** `A` and `d_pk` are both expanded from the same seed
  `ρ`, and `(d_u, d_v)` from `µ`; Theorem A.4 assumes a truly uniform,
  independent dither. Theorem 6.1 covers this only for "the seeded
  implementation" by idealising the expansion, and the document does not bound
  the expansion advantage.

## Parameters

| parameter | Viper-128 | Viper-192 | Viper-256 | Viper-384 | Viper-512 | meaning |
|---|---|---|---|---|---|---|
| target λ | 128 | 192 | 256 | 384 | 512 | also `ℓ_K = \|ss\| bits` |
| n | 256 | 256 | 256 | 256 | 256 | ring degree, `X^n+1` |
| q | 2^12 | 2^12 | 2^12 | 2^13 | 2^13 | power-of-two modulus |
| k | 2 | 3 | 4 | 7 | 9 | module rank |
| (η_s, η_r) | (2,2) | (3,3) | (3,3) | (1,1) | (1,1) | CBD parameters, *secrets only* |
| (t_pk, t_u, t_v) | (9,9,4) | (10,9,6) | (10,10,5) | (11,11,5) | (11,11,8) | quantization widths |
| ⇒ (g_pk, g_u, g_v) | (3,3,8) | (2,3,6) | (2,2,7) | (2,2,8) | (2,2,5) | dither bits/coeff = log2 Δ |
| b_msg | 1 | 1 | 1 | 2 | 2 | message bits per E8 coordinate (impl) |
| active E8 blocks | 16 | 24 | 32 | 24 | 32 | of 32 total; rest zeroed |
| log2 δ_dec | −177.91 | −172.22 | −272.99 | −351.83 | −421.06 | spec Table 5 |
| claimed security | 141.9 | 199.4 | 266.0 | 410.2 | 564.6 | MATZOV; CoreSVP C/Q 118.6/108.7, 177.5/162.1, 246.2/225.3, 393.7/361.1, 549.5/521.0 |

Sizes (bytes), specification (Table 4) vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | ct spec | ct impl | ss | match |
|---|---|---|---|---|---|---|---|---|
| MAMBA-Viper-128 | 608 | 608 | 1424 | 1424 | 736 | 736 | 16 | yes |
| MAMBA-Viper-192 | 992 | 992 | 2200 | 2200 | 1088 | 1088 | 24 | yes |
| MAMBA-Viper-256 | 1312 | 1312 | 2912 | 2912 | 1472 | 1472 | 32 | yes |
| MAMBA-Viper-384 | 2496 | 2496 | 5488 | 5488 | 2656 | 2656 | 48 | yes |
| MAMBA-Viper-512 | 3200 | 3200 | 7040 | 7040 | 3456 | 3456 | 64 | yes |

All five agree with the spec formulas `|pk| = 32 + k·n·t_pk/8`,
`|ct| = 32 + (k·n·t_u + n·t_v)/8`, `|sk| = k·n·log2 q/8 + |pk| + 32 + |ss|`
(note the secret is stored *uncompressed*, `log2 q` bits per coefficient).
All 5 instances PASS KAT.

## Pseudocode

### Viper.PKE.KeyGen — spec Algorithm 3
```
ρ          <- U({0,1}^256)
(A, d_pk)  <- GenPublic(ρ)          # A in R_q^{k x k}, d_pk in D_pk^k
s          <- B_{η_s}^k             # the only sampled randomness
b          <- Q*_{Λ_pk, d_pk}(A s) / Δ_pk      # label vector in R_{p_pk}^k
pk <- (ρ, b) ;  sk_PKE <- s
```

### Viper.PKE.Enc(pk, m, ω) — spec Algorithm 4 (+ 1 for Encode)
```
ω -> (σ, µ) in {0,1}^256 x {0,1}^256
(A, d_pk) <- GenPublic(ρ)
b̂ <- Δ_pk·b - d_pk            (mod q)      # normal form: b̂ = A s + e_pk
r <- GenSecret_{η_r}(σ)
(d_u, d_v) <- GenDither(µ)
u <- Q*_{Λ_u, d_u}(A^T r) / Δ_u
v <- Q*_{Λ_v, d_v}(b̂^T r + Encode(m)) / Δ_v
ct <- (u, v, µ)
```

### Viper.PKE.Dec(sk, ct) — spec Algorithm 5 (+ 2 for Decode)
```
(d_u, d_v) <- GenDither(µ)
û <- Δ_u·u - d_u ;  v̂ <- Δ_v·v - d_v       (mod q)
w  <- v̂ - s^T û
m' <- Decode(w)
# w - Encode(m) = e_pk^T r - s^T e_u + e_v  (the DFR object, §2.3)
```

### E8 codec — spec Algorithms 1, 2, 9, 10 (§8.8)
```
M = 2^{b_msg} ;  α = q / M
β = (b_msg-1, b_msg, ..., b_msg, b_msg+1)      # 8 entries, sum 8·b_msg
  equivalently (p_0..p_7) = (M/2, M, M, M, M, M, M, 2M),  M/2 = 1 when b_msg = 1
B_E8 = rectangular-form basis = U·diag(2,1,1,1,1,1,1,1/2), U in GL_8(Z),
       so M·Z^8 = B_E8 · diag(p_0..p_7) · Z^8 and prod_i {0..p_i-1} labels E8/(M Z^8)

Label(a_0..a_7):  λ <- B_E8 · a  mod M Z^8      # stored in R_q as α·λ mod q
Delabel(λ):       a* <- B_E8^{-1} λ ;  a_i <- a*_i mod p_i

Encode(m): for each of B_msg active blocks, parse the next 8·b_msg bits into
           (a_0..a_7) little-endian with widths β, Label, write into
           coefficients 8j..8j+7 of M. Remaining coefficients are set to 0.
Decode(w): per block y = (τ(w)_{8j..8j+7}); λ <- Q_{α E8}(y) mod q Z^8 on
           CENTERED representatives; Delabel; re-serialize the bits.
Correct decoding iff the error block lies in the Voronoi region of α·E8.
```

### Viper.KEM.KeyGen / Encaps / Decaps — spec Algorithms 6, 7, 8
```
KeyGen:  z <- U({0,1}^ℓK) ; (pk, sk_PKE) <- PKE.KeyGen()
         h_pk <- H(pk) ;  sk <- (sk_PKE, pk, h_pk, z)

Encaps:  h_pk <- H(pk) ;  m <- U({0,1}^ℓK)
         (K̄, ω) <- G(h_pk, m)
         ct <- PKE.Enc(pk, m, ω)
         K  <- H_{ℓK}( K̄ , H(ct) )

Decaps:  m'       <- PKE.Dec(sk_PKE, ct)
         (K̄', ω') <- G(h_pk, m')
         ct'      <- PKE.Enc(pk, m', ω')
         K <- H_{ℓK}(K̄', H(ct))  if ct' = ct,  else  H_{ℓK}(z, H(ct))
         (§2.4 and §8.10: both candidates must be computed, the comparison must
          scan the full ciphertext, and the selection must be a mask/cmov)
```

## Implementation vs specification

Checked: `viper_params.h` (constants for all five levels), `kem.c` (FO layer),
`viper.c` (GenPublic, GenDither, quantize/reconstruct, CBD, KeyGen/Enc/Dec,
re-encryption check), `viper_e8.c` + `viper_message_codec.c` (codec),
`verify.c` (`verify`/`cmov`), `viper_api_hash.c` (symmetric backend).
Constants sampled: `VIPER_N`, `VIPER_Q/QLOG`, `VIPER_K`, `VIPER_ETA_S/R`,
`VIPER_T_PK/T_U/T_V` and `VIPER_MSG_BITS_PER_COEFF` — for **all five** levels
(they live in one `#if`-chained header).

- **Agreement.** Every parameter matches Table 3 exactly: (128) `256, 2^12, k=2,
  η=2, (9,9,4)`; (192) `k=3, η=3, (10,9,6)`; (256) `k=4, η=3, (10,10,5)`;
  (384) `2^13, k=7, η=1, (11,11,5)`; (512) `2^13, k=9, η=1, (11,11,8)`.
  `viper_quantize`/`viper_reconstruct` (`viper.c:51-70`) are literally the §8.7
  formulas, with per-width unrolled variants that use the same
  `quantize_shift`. The CBD nibble LUT for η=2 (`viper.c:423-430`) and the generic
  bit-slice path both compute `Σb_j − Σb'_j` per §8.6. `verify()` and `cmov()`
  (`verify.c`) are correct constant-time primitives, and
  `viper_reencrypt_check` (`viper.c:554-585`) accumulates differences over the
  recomputed `u` and `v` before returning, then `kem.c:62` selects with `cmov`.
  Its `µ` term (`viper.c:583`) compares the ciphertext's `µ` with itself (`mu`
  points into `ct`, `viper.c:562`), and decapsulation does not recompute
  `µ = H_32(K̄')`, so `µ` is not checked. All five size formulas
  reproduce the OBSERVED lengths exactly.

- **Discrepancy 1 — the public-key dither is one bit too narrow at Viper-128
  (real deviation, invalidates the MLWQ→MLWE step for that profile).**
  `VIPER_PUBLIC_DPK_POLYBYTES` is hardcoded as `VIPER_N * 2u / 8u`
  (`viper.c:319`) and `viper_gen_public_parse_dpk` (`viper.c:368-379`, duplicated
  at `viper.c:410-416`) unpacks **exactly 2 bits per coefficient**:
  ```c
  dpk[i][l+0] = b & 3u;  dpk[i][l+1] = (b >> 2) & 3u;  ...
  ```
  regardless of the profile. But the public-key quantizer is invoked with
  `VIPER_T_PK` (`viper.c:504`, `:519`, `:571`), i.e. shift
  `g_pk = QLOG − T_PK`. That is 2 for Viper-192/256/384/512 — correct — but
  **3 for Viper-128** (`QLOG = 12`, `T_PK = 9`, `Δ_pk = 8`). So at Viper-128 the
  public-key dither ranges only over `{0,1,2,3}` instead of `{0,…,7}`.
  Theorem A.4's hypothesis ("`d` uniform over `{0,…,Δ−1}`") then fails: writing
  `c = (a+4) mod 8`, the error is `e = 4 − ((c+d) mod 8)` with `d ∈ {0..3}`, so
  `e` takes only 4 of its 8 values and *which four depends on `a mod 8`* — for
  `a ≡ 4 (mod 8)` the error is always positive (`e ∈ {1,2,3,4}`), for
  `a ≡ 0 (mod 8)` always non-positive. The public key is therefore **not** a
  normal-form MLWE sample; it is a partially-derandomised, MLWR-like sample with
  one bit less error entropy per coefficient than the parameter table assumes.
  Theorems A.4/A.6 — the entire justification for the security estimate in
  Table 5 — do not apply to Viper-128's public key. Note `viper_gen_dither`
  (`viper.c:303-316`) *does* compute the width correctly from
  `QLOG − T_U` / `QLOG − T_V`, so the ciphertext-side dithers are right; only
  `d_pk` is hardcoded. I did not attempt to turn this into a concrete attack;
  MLWR is itself believed hard, so this is a broken proof step and a reduced
  noise budget rather than a demonstrated break.

- **Discrepancy 2 — `µ` is derived from the FO pre-key, not from `G` (real
  deviation).** Spec Algorithm 4 parses the *output* of `G` as `ω = (σ, µ)`.
  The implementation computes `kr = G(m ‖ h_pk)` as `K̄ ‖ σ` and then sets
  `µ = H_32(K̄)` (`kem.c:40`), transmitting it in the ciphertext. The ciphertext
  therefore carries a 32-byte hash commitment to the FO pre-key `K̄`. Since
  `ss = H(K̄ ‖ H(ct))` and `|K̄| = |ss|`, brute-forcing `K̄` through `µ` costs the
  same as brute-forcing `ss` directly, so no loss is demonstrated — but the
  construction is undocumented and makes the pre-key an offline-verifiable
  target, which the FO proof of §6.3 does not model.

- **Discrepancy 3 — coin lengths (real deviation).** Algorithm 4 fixes
  `ω ∈ {0,1}^512` with `σ, µ` each 256 bits. The implementation uses
  `VIPER_FALLBACK_KEY_BYTES = VIPER_SSBYTES` for `σ` (`viper_params.h:98`), i.e.
  **128 bits at Viper-128** and 192 at Viper-192. `µ` is 32 bytes as specified.

- **Discrepancy 4 — the message `m` is hashed before use (minor).** Algorithm 7
  says `m ← U(M_FO)`; `kem.c:33-35` draws `m` then replaces it with `H(m)`.
  Distributionally harmless.

- **Symmetric backend (spec-permitted, but the names mislead).** `shake128()`
  and `shake256()` in this build are **not** Keccak: `viper_api_hash.c`
  implements them as the ICCS `pseudoXOF` over a prefixed ASCII domain label
  (`"Viper-shake128"` / `"Viper-shake256"`), and `sha3_256()` is `sm3hash`.
  §8.5 states the API_PKC path routes through `sm3hash`/`pseudohash`/`pseudoXOF`,
  so this conforms — but the header `fips202.h` and the file-level comment in
  `kem.c` ("FO KEM using SHAKE128 hashes/KDF") describe something else. One
  robustness nit: on `malloc` failure `ds_xof` (`viper_api_hash.c:12-19`) falls
  back to `pseudoXOF` **without** the label, silently dropping domain
  separation.

- **Not verified.** The DFR computation of Appendix B, the Core-SVP/MATZOV
  figures of Table 5, the ROM/QROM FO proofs (Theorems 6.2/6.3) beyond reading
  them, and the correctness of `viper_e8.c`'s nearest-neighbour decoder against
  Algorithm 10 coefficient by coefficient. The E8 codec is not constant-time
  (see `constant_time.md`).
