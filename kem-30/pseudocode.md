# kem-30 PolarLAC — algorithm summary

LAC-style module-LWE KEM over R_q = Z_q[x]/(x^n + 1) with a very small modulus
(q = 257, or 769 for the 512* set), rank k = 2, ternary secrets/noise, and a
rate-1/2 error-correcting code. Its distinguishing feature is that LAC's
BCH + D2 error correction is replaced by a **polar code with soft-decision
successive-cancellation decoding**, backed by two further anti-DFR-attack
techniques: *spectral (frequency-domain) rejection sampling* of secrets and
noise, and a *degraded virtual channel* used to compute the decoder's
likelihood ratios. IND-CCA comes from an FO transform with implicit rejection
applied to PolarLAC.PKE.CPA.

Specification: `kem-30-spec.pdf` (44 pages, English), §4.1 (Algorithms 8–10,
PKE), §4.2 (Algorithms 11–13 KEM; 14–15 salted variant), the polar subroutines
in Algorithms 1–2, compression in 3–4, sampling in 5–7; parameters §5.5 Table 3
and §5.4 Table 2.

## Parameters

| parameter | Light | 128 | 256 | 512 | 512* | meaning |
|---|---|---|---|---|---|---|
| k | 2 | 2 | 2 | 2 | 2 | module rank |
| n | 256 | 256 | 512 | 1024 | 1024 | ring degree, x^n + 1 |
| q | 257 | 257 | 257 | 257 | 769 | modulus |
| dis (Ψ_σ) | Ψ_{1/4} | Ψ_{3/8} | Ψ_{1/4} | Ψ_{11/64} | Ψ_{3/8} | ternary secret/noise distribution |
| R | 1/2 | 1/2 | 1/2 | 1/2 | 1/2 | message length / code length |
| l_v | 256 | 256 | 512 | 1024 | 1024 | polar code length |
| l_m = l_k | 128 | 128 | 256 | 512 | 512 | message / session-key bits |
| d1 | 8 | 8 | 8 | 8 | 9.60 | bits per c1 coefficient |
| d2 | 3 | 4 | 4 | 4 | 4 | bits per c2 coefficient |
| T0 (spectral threshold) | 24 | 30 | 36 | 43 | 64 | ‖FFT(x)‖_∞ bound actually used |
| T (analysis threshold) | 36 | 42 | 58 | 80 | 116 | conservative bound in the DFR proof |
| DFR | 2^−159 | 2^−146 | 2^−265 | 2^−324 | 2^−545 | spec's claim |
| claimed security (core-SVP C/Q) | 121.6/111.7 | 132.5/122.6 | 261.6/240.8 | 512.1/482.5 | 527.1/484.7 | bits |
| claimed security (refined BKZ) | 143.8/138.8 | 154.5/148.1 | 280.8/263.4 | 527.3/499.3 | 540.1/500.3 | bits |

Sizes (bytes), specification (Table 3) vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | ct spec | ct impl | ss impl | match |
|---|---|---|---|---|---|---|---|---|
| POLARLAC-Light | 530 | 530 | 1570 | 1570 | 608 | 608 | 16 | yes |
| POLARLAC-128 | 530 | 530 | 1570 | 1570 | 640 | 640 | 16 | yes |
| POLARLAC-256 | 1060 | 1060 | 3140 | 3140 | 1280 | 1280 | 32 | yes |
| POLARLAC-512 | 2116 | 2116 | 6276 | 6276 | 2560 | 2560 | 64 | yes |
| POLARLAC-512-Star | 2522 | 2522 | 6682 | 6682 | 2970 | 2970 | 64 | yes |

All fifteen values agree. Table 3 gives no separate ss column; the spec sets
l_k = l_m = n/2 bits, i.e. 16/16/32/64/64 bytes, which is what the library
reports. Structure: pk = seed_a(16) ‖ zero-selector-packed b (k·(n+1) at q=257);
sk = s_ntt(2kn bytes) ‖ pk ‖ z(l_m/8); ct = c1(k·n·d1/8) ‖ c2(l_v·d2/8).

## Pseudocode

### PolarEnc / PolarDec — Algorithms 1–2 (§4.1)
```
PolarEnc(m):  u <- 0;  u_I <- m  (I = information set of Polar[l_v, I])
              m̂ <- u^t · G ;  return m̂ ∈ {0,1}^{l_v}
PolarDec(y):  LR_i <- P(y_i | m̂_i = 0) / P(y_i | m̂_i = 1),  i = 0..l_v-1
              u <- SCDecoder(LR);  return m <- u_I
```

### PolarLAC.PKE.CPA — Algorithms 8–10 (§4.1)
```
KG():
  repeat
      seed_a, seed <-$ S
      A    <- RejSampU(R_q; seed_a) ∈ R_q^{k×k}                 // Alg. 6, rejection-free variant
      s, e <- RejSamp(T, Ψ_σ; seed) ∈ R_q^{k}                   // Alg. 7, spectral rejection: ‖FFT(x)‖∞ <= T
      b    <- A·s + e
      cnt  <- (q = 257) ? #{coefficients of b equal to 0 or 256} : 0
  until cnt <= γ                                                // zero-selector budget, §5.3
  return pk = (seed_a, b), sk = s

Enc(pk, m; seed):
  A      <- RejSampU(R_q; seed_a)
  m̂      <- PolarEnc(m) ∈ {0,1}^n
  r, e1  <- RejSamp(T, Ψ_σ; seed) ;  e2 <- Samp(Ψ_σ; seed)
  c1     <- A^t·r + e1 ∈ R_q^k
  c2'    <- b^t·r + e2 + q̄·m̂ ∈ R_q                              // q̄ = round(q/2) = 129
  c2     <- Compr_d(c2') ∈ Z_{2^d}^n                            // Alg. 3: round(x·2^d/q) mod 2^d
  return c = (c1, c2)

Dec(sk = s, c):
  u    <- s^t·c1 ;  c2'' <- DeCompr_d(c2) ;  c_m <- c2'' - u ∈ R_q
  return PolarDec(c_m)
```

### PolarLAC.KEM.CCA — Algorithms 11–13 (§4.2)
```
KG():   (pk, sk') <- PKE.KG();  z <-$ S;  sk := (sk', pk, z)

Enc(pk; seed_m):
  m <- Samp(U(M); seed_m)
  (seed, K) <- G(m ‖ pk)
  c <- PKE.Enc(pk, m; seed)
  return (c, K)

Dec(sk, c):                                       // FO with implicit rejection
  m       <- PKE.Dec(sk', c)
  (K,seed)<- G(m ‖ pk) ;  K' <- H(z ‖ c)
  c'      <- PKE.Enc(pk, m; seed)
  if c' != c then K <- K'
  return K
```

The spec also gives a **salted** variant (Algorithms 14–15) with
(seed, K) <- G(m ‖ pk ‖ salt) and c = (salt, ĉ), as a multi-target defence; it is
optional and, per Table 3's ciphertext sizes, not the one costed.

## The polar-code decoding step (the distinguishing feature)

Three pieces, per §4.1 and §6:

1. **Frozen/information set.** Polar[l_v, I] is fixed per parameter set; I is
   chosen by reliability of the synthesised channels W_n (§2.4, Fig. 1), with
   |I| = l_m = l_v/2 (rate exactly 1/2). The spec discusses polarization of both
   capacity and *reliability* and states that for KEMs reliability is the design
   target; it does not print the concrete I, nor state whether Bhattacharyya
   parameters, density evolution or Gaussian approximation was used to rank the
   channels. That is a specification gap — the information set is only
   recoverable from the implementation's tables.
2. **LLRs from a degraded virtual channel (§6).** Rather than assume the
   decryption noise h = e^t·r − s^t·e1 + e2 + n_{c1} + n_{c2} is i.i.d. — which
   a CCA adversary can violate by choosing spectrally correlated r, e — the spec
   constructs a *virtual* noise e_vir that is memoryless, symmetric and
   provably of larger magnitude than the real noise e_real (Fig. 5), by a
   two-step degradation pipeline argued in the frequency domain. The likelihood
   ratios LR_i of Algorithm 2 are computed against e_vir, so the decoder is
   deliberately mis-matched in the safe direction.
3. **SC decoding.** The likelihood ratios are handed to a successive-cancellation
   decoder (§4.1, citing Arıkan). The spec specifies SC only — no list decoding,
   no BP — and claims the polarization phenomenon plus soft-decision decoding
   gives the DFR bounds of Table 3.

## Implementation vs specification

Checked (built sources per `kem-30/Makefile`:
`Implementations/Reference_Implementation/x86/<inst>/` — `polar.c`, `poly.c`,
`pke.c`, `sample.c`, `ntt.c`, `fft.c`, `symmetric.c`, `fips202.c`,
`KEM_AlgorithmInstance.c`, `auxfunc.c`, `drng.c`, built with
`-DBIT_USE_SHAKE=0 -DRL_KEM_USE_CONJ_NTT_REJECTION=0`).

The decoder, concretely:
- `polar.c:395 decode_polar` is a genuine **Fast-SSC (simplified successive
  cancellation)** decoder, not a slicer: the min-sum `f`/`g` recursions are the
  `f_macro`/`g_macro` at `polar.c:20-21`, the LLR/bit recursion depths come from
  the `llr_layer_vec` / `bit_layer_vec` / `lambda_offset` tables
  (`polar.c:35-47`), and the code is pre-decomposed into 47 constituent nodes in
  `node_type_matrix` (`polar.c:52`) typed R0 (−1), R1 (1), Rep (2) and Type-I (3),
  each decoded by a specialised routine instead of bit-by-bit. This is
  functionally SC (identical output to plain SC for these node types), just
  faster — a class (c) equivalent optimisation. **List size is 1**; no SCL, as
  the spec specifies.
- The information set is the hard-coded `info_nodes[]` bitmap and
  `data_pos_sorted[128]` (`polar.c:30`, `polar.c:59`), |I| = 128 = l_v/2 for
  POLARLAC-128, matching R = 1/2.
- The "virtual worse channel" is realised as a **precomputed fixed-point LLR
  table**: `poly.c:26 llr_table[]` holds 256 int64 entries and `poly.c:332` does
  `llr[i] = llr_table[centered + RATIO - 1]`, i.e. the LLR is a pure lookup on
  the centred coefficient value of c_m, with "0 modulated to −q/4, 1 to +q/4".
  The table contents are fixed, but the address is indexed by the decrypted
  coefficient. The lookup is therefore a secret-dependent cache access, not a
  constant-time LLR computation; the table's derivation from e_vir is also not
  reproducible from the spec (see below).

Agreements:
- Every Table 3 parameter I sampled matches `params.h` across all five
  instances: k = 2 (`RL_KEM_K`), n = 256/256/512/1024/1024 (`RL_KEM_N`),
  q = 257 ×4 and 769 for 512* (`RL_KEM_Q`), l_v = n (`RL_KEM_Lv`),
  d2 = 3 for Light and 4 elsewhere (`D_C2_BITS`), l_m/8 = 16/16/32/64/64
  (`MESSAGE_LEN_BYTES`), q̄ = 129 (`RATIO`).
- The Table 2 spectral thresholds match, stored squared:
  `RL_KEM_T` = 576, 900 (written `30*30`), 1296, 1849, 4096 = T0² for
  T0 = 24, 30, 36, 43, 64. The implementation uses **T0**, the measured
  ~1–1.7 %-rejection threshold, not the conservative T of the DFR proof — which
  is exactly what the spec says it does (§5.4: "T0 is the threshold we used in
  implementation, while T is the one [used in the analysis]").
- The FO transform of Algorithms 11–13 is implemented faithfully and in constant
  time: `derive_g` computes (ss, seed) = XOF(m ‖ pk) (`KEM_AlgorithmInstance.c:59-75`),
  `derive_reject_key` computes H(z ‖ ct) (`:77-91`), and `kem_dec` re-encrypts,
  compares with `ct_verify` and selects with `ct_cmov` (`:225-232`) — no branch
  on the decryption result.
- The zero-selector public-key compression of §5.3 is present
  (`PK_ZERO_SELECTOR_BYTES`, `PK_POLY_BYTES = RL_KEM_N + PK_ZERO_SELECTOR_BYTES`),
  giving pk = 16 + k(n+1) = 530 at n = 256, exactly Table 3.

Discrepancies / notes:
- **(b) the hash backend is a build-time switch, and the spec's §5 choice is not
  what "BIT_USE_SHAKE" defaults to in all trees.** `symmetric.c` and
  `random_fips.h` select between SHAKE (via `fips202.c`) and SM3 (via
  `auxfunc.c`) on `BIT_USE_SHAKE`. The candidate's own Makefile defaults to
  `BIT_USE_SHAKE=0` (SM3 + the NGCC `drng_algorithm`), which is what
  `kem-30/Makefile` pins and what the submitted KATs were produced with; with
  `BIT_USE_SHAKE=1` the tree would instead draw entropy from the OS
  (`getrandom` / `/dev/urandom`) rather than from the NGCC DRNG, which would be
  a conformance problem for KAT reproducibility. This matches the note already
  recorded in `RESULTS.md` (kem-30 "`BIT_USE_SHAKE=1` path"); flagged here only
  for cross-reference, not rediscovered.
- **(b) specification gap — the information set and the LLR table are not
  derivable from the spec.** §2.4/§4.1 say the polar code is "uniquely
  determined by l_v and I" but never print I, and §6 describes e_vir
  qualitatively (spherical, memoryless, larger magnitude) without giving the
  standard deviation actually chosen or the quantisation used for the LLRs. The
  implementation's `data_pos_sorted[]` and `llr_table[]` are therefore
  unverifiable against the document: an independent implementer could not
  reproduce this scheme's ciphertexts. I could not verify these two tables.
- **(b) the salted variant is specified but not implemented.** Algorithms 14–15
  add a public salt to defend against the multi-target attack the spec itself
  cites for FrodoKEM-640 and HQC-128 (§4.2). The reference implements the
  unsalted Algorithms 12–13 — ct = c1 ‖ c2 with no salt field, consistent with
  Table 3's ciphertext sizes. So the spec describes a defence that the submitted
  parameter sets and code do not carry.
- **(c) SSC node decomposition.** As above, `node_type_matrix` replaces bit-wise
  SC with constituent-code decoding. Equivalent output, undocumented in the spec.

Not verified: the DFR analysis of §6 and the novel-attack analysis of §6.4; the
`fft.c` spectral rejection sampler against Algorithm 7 beyond confirming the T0²
constants; the ARM/SVE and AVX2 trees (not built); the 512-Star 9.6-bit c1
packing in detail (its pk/ct sizes match Table 3, which I took as sufficient).
The library was not executed; KAT conformance is reported in
`kem-30/security_findings.md` and `RESULTS.md`.
