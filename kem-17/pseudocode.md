# kem-17 HEP-QC — algorithm summary

HEP-QC ("Hybrid Equivalent Punctured and Quasi-Cyclic") is an HQC-style
code-based KEM over the quasi-cyclic ring R = F2[x]/(x^n − 1), hardened by
*hiding the decodable code*: the public generator matrix is
G′ = T·ExpandMat(G)·P, where G generates a concatenated
Reed–Solomon ∘ duplicated Reed–Muller code, T ∈ GL(k, F2) is a secret
scrambler and P a secret permutation of the n′ = 64⌈n/64⌉ coordinates. An
adversary therefore faces a random-looking [n′, k] code (decoding hard), while
the holder of (T, P) reduces to decoding C. On top of that sits the usual
2-QCSD syndrome-decoding assumption for u = r1 + h·r2 and the
Fujisaki–Okamoto transform (HHK, implicit rejection, salted) to reach
IND-CCA2.

Specification: `kem-17-spec.pdf` (34 pages), §3 (Algorithms 1–6:
PKE.KeyGen/Encrypt/Decrypt and KEM.KeyGen/Encaps/Decaps), §3.2–3.5 (sampling,
matrix/permutation generation, Expand/Truncate, the concatenated code),
§4 Tables 4.1–4.2 (parameters and sizes).

## Parameters

| parameter | HEP-QC-1 | HEP-QC-3 | HEP-QC-5 | HEP-QC-7 | meaning |
|---|---|---|---|---|---|
| n | 17669 | 35851 | 57637 | 197123 | smallest primitive prime > n1·n2 |
| n1 | 46 | 56 | 90 | 220 | Reed–Solomon length |
| n2 | 384 | 640 | 640 | 896 | duplicated Reed–Muller length |
| k | 128 | 192 | 256 | 512 | message bits (RS dim = k/8 symbols) |
| ω | 66 | 100 | 131 | 261 | weight of x, y |
| ω_r = ω_e | 75 | 114 | 149 | 297 | weight of r1, r2, e |
| δ (RS) | 15 | 16 | 29 | 78 | RS error-correcting capacity |
| \|seed\| / \|salt\| / \|K\| | 32 / 16 / 32 | same | same | same | bytes (§4.2) |
| security | 128 | 192 | 256 | 512 | bits |
| DFR | < 2⁻¹²⁸ | < 2⁻¹⁹² | < 2⁻²⁵⁶ | < 2⁻⁵¹² | spec's own bound |

Sizes (bytes), specification Table 4.2 vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | ct spec | ct impl | ss spec | ss impl | match |
|---|---|---|---|---|---|---|---|---|---|
| hep-qc-1 | 285 889 | 285 889 | 285 969 | 285 969 | 4433 | 4433 | 32 | 32 | yes |
| hep-qc-3 | 866 210 | 866 210 | 866 298 | 866 298 | 8978 | 8978 | 32 | 32 | yes |
| hep-qc-5 | 1 852 485 | 1 852 485 | 1 852 581 | 1 852 581 | 14421 | 14421 | 32 | 32 | yes |
| hep-qc-7 | 12 644 449 | 12 644 449 | 12 644 577 | 12 644 577 | 49297 | 49297 | 32 | 32 | yes |

§4.2 formulas, reproduced exactly by the code:
pk = |seed| + ⌈n/8⌉ + 8k·⌈n/64⌉ (the dense k × n′ matrix G′ dominates),
sk = pk + |seed| + ⌈k/8⌉ + |seed|, ct = ⌈n/8⌉ + ⌈n1n2/8⌉ + |salt|.

## Pseudocode

### PKE.KeyGen (spec Algorithm 1)
```
 1  (seed_sk, seed_pk) <- H(seed_PKE)
 2  ctx_sk <- XOF.Init(seed_sk)
 3  y <- SampleFixedWeightVect$(ctx_sk, R_ω)        # rejection-sampled, unbiased
 4  x <- SampleFixedWeightVect$(ctx_sk, R_ω)
 5  P <- GeneratePerm$(ctx_sk, R)                   # permutation of n′ = 64⌈n/64⌉ coords
 6  T <- GenerateMat$(ctx_sk, R2)                   # non-singular k×k over F2
 7  sk_PKE <- seed_sk                               # 32 bytes only
 8  ctx_pk <- XOF.Init(seed_pk)
 9  h <- SampleVect(ctx_pk, R)
10  s <- x + h·y
11  G″ <- ExpandMat$(G, n′ − n)                     # G: concatenated RS ∘ dup-RM
12  G′ <- T·G″·P                                    # the "hidden equivalent" code
13  pk_PKE <- (seed_pk, s, G′)
```

### PKE.Encrypt (spec Algorithm 2)
```
 1  parse pk = (seed_pk, s, G′) ;  ctx_pk <- XOF.Init(seed_pk) ;  h <- SampleVect(ctx_pk, R)
 2  ctx_θ <- XOF.Init(θ)
 3  r2 <- SampleFixedWeightVect(ctx_θ, R_{ω_r})     # biased (non-rejection) sampler
 4  e  <- SampleFixedWeightVect(ctx_θ, R_{ω_e})
 5  r1 <- SampleFixedWeightVect(ctx_θ, R_{ω_r})
 6  u <- r1 + h·r2
 7  v <- m·G′ + ExpandVect(s·r2 + e, n′ − n)
 8  c <- (u, v)
```

### PKE.Decrypt (spec Algorithm 3)
```
1  ctx_sk <- XOF.Init(sk) ; regenerate y, P, T
2  parse c = (u, v)
3  c′ <- v − ExpandVect(u·y, n′ − n)
4  m′ <- C.Decode( TruncateVect(c′·P⁻¹, n′ − n1n2) )
5  m  <- m′·T⁻¹
```
Correctness holds when
ω( TruncateVect( ExpandVect(x·r2 − r1·y + e, n′−n)·P, n′−n ) ) ≤ Δ.

### KEM (spec Algorithms 4–6; FO/HHK with implicit rejection)
```
KeyGen:                                    Encaps(ek):
 1 seed_KEM <-$ B^{|seed|}                  1 m    <-$ B^{|k|}
 2 ctx <- XOF.Init(seed_KEM)                2 salt <-$ B^{|salt|}
 3 seed_PKE <- XOF.GetBytes(ctx, |seed|)    3 (K, θ) <- G( H(ek) ‖ m ‖ salt )
 4 σ        <- XOF.GetBytes(ctx, |k|)       4 c_PKE <- PKE.Encrypt(ek, m, θ)
 5 (pk, sk_PKE) <- PKE.KeyGen(seed_PKE)     5 c_KEM <- (c_PKE, salt)
 6 ek <- pk ; dk <- (ek, sk_PKE, σ, seed_KEM)

Decaps(dk, c):
 1 parse dk = (ek, sk_PKE, σ, …) ; parse c = (c_PKE, salt)
 2 m′ <- PKE.Decrypt(sk_PKE, c_PKE)
 3 (K′, θ′) <- G( H(ek) ‖ m′ ‖ salt )
 4 c′_KEM <- (PKE.Encrypt(ek, m′, θ′), salt)
 5 K̄ <- J( H(ek) ‖ σ ‖ c_KEM )
 6 if m′ = ⊥ or c′_KEM ≠ c_KEM :  K′ <- K̄
 7 return K′
```

### Symmetric primitives (spec §3.1)
The spec states plainly that the API_PKC functions are used "solely for
correctness verification and preliminary performance testing" and "will be
replaced with cryptographically secure hash functions and XOFs" in later
rounds. Concretely: XOF = KDF-SM3 (`pseudoXOF`), H = SM3-256, G and J = the
SM3/HMAC-SM3 construction with 512/768/1024-bit output.

## Implementation vs specification

Checked, in `Implementations/Implementations/`:
`common/hep-qc-<n>/api.h` and `ref/hep-qc-<n>/parameters.h` (all four
instances), `ref/KEM_HEP_QC.{c,h}` (Alg. 4–6), `ref/PKE_HEP_QC.c` (Alg. 1–3),
`common/symmetric.c` (XOF/PRNG/hashes), `common/parsing.c`, `common/vector.c`,
`common/{code,reed_muller,reed_solomon,gf,gf2x,fft}.c`. Not executed.

Agreements:
- Every `parameters.h` matches spec Table 4.1 exactly for its row
  (n, n1, n2, ω, ω_r = ω_e, security level) and `MSG_BIT` = k with
  `PARAM_K` = k/8 RS symbols, `PARAM_DELTA` = ⌊(n1 − k/8)/2⌋; `SEED_BYTES 32`,
  `SALT_BYTES 16`, `CRYPTO_BYTES 32` as §4.2. `api.h`'s four byte counts match
  Table 4.2 and the library for all four instances.
- The PKE follows Algorithms 1–3 step for step, including the sampling order
  r2, e, r1 from ctx_θ (`PKE_HEP_QC.c:129-131`, matching Alg. 2 lines 7–9),
  the `SampleFixedWeightVect$`/`SampleFixedWeightVect` split (rejection-based
  for the secret x, y; biased for the encryption randomness, as §3.2.1 says),
  the secret permutation applied in decryption
  (`vect_permute_columns_inplace`) and the final m = m′T⁻¹.
- The FO transform is present in `kem_dec`: recompute (K′, θ′), re-encrypt and
  compare u, v and salt, then select between K′ and K̄ = J(H(ek)‖σ‖c) with a
  branch-free mask. The ciphertext parse consumes exactly ⌈n/8⌉ + ⌈n1n2/8⌉ +
  |salt| = CIPHERTEXT_BYTES bytes, with no unused/unmasked padding inside the
  compared regions, so there is no obvious ciphertext-malleability gap.
- `sk_PKE` really is just the 32-byte seed (x, y, P, T are regenerated on every
  decryption), as Algorithm 1 line 7 specifies.

Discrepancies:
- **(a) CRITICAL — the scheme's randomness source is never seeded; every fresh
  process produces the identical key pair and the identical encapsulation.**
  `ref/KEM_HEP_QC.c:31` declares `extern DRNG_ctx drng_algorithm` and
  `:34` carries a comment showing how it should be used
  (`get_random_number(&drng_algorithm, …)`) — but the call is commented out and
  `get_random_number` is never invoked anywhere in the tree. `seed_KEM`
  (KeyGen), `m` and `salt` (Encaps) all come from `prng_get_bytes`
  (`common/symmetric.c:32`), which squeezes the file-scope global
  `shake256_xof_ctx shake256_prng_ctx` (`symmetric.c:17`). The only function
  that ever writes that global is `prng_init` (`symmetric.c:23`), and
  `prng_init` has no caller in the KEM/PKE path — a repository-wide grep finds
  only its declaration, its definition, and the intermediate-values test. The
  context therefore starts as zero-filled BSS and is advanced only by its own
  ratchet, so the whole output stream is a fixed, publicly computable sequence.
  The spec is explicit that this must not be so: Algorithm 4 line 1
  (`seed_KEM ←$ B^{|seed|}`) and Algorithm 5 lines 1–2 (`m ←$ B^{|k|}`,
  `salt ←$ B^{|salt|}`) require fresh randomness, and the submission
  guidelines route it through the seeded DRNG. KATs still reproduce (the
  deterministic stream is the same on every run), which is exactly why the
  defect is invisible to the KAT harness. Compare kem-16 HARE, which routes
  `prng_get_bytes` to `get_random_number(&drng_algorithm, …)`.
- **(a) The XOF ratchet derives the output and the next state from the same
  input with the same XOF, so the output is a prefix of the next state.**
  `xof_proc` (`common/symmetric.c:111-116`) does
  `pseudoXOF(outlen·8, aux, …, h)` and then
  `pseudoXOF(CTX_LEN·8, aux, …, ctx)` from the identical `aux`. `pseudoXOF` is
  SM3 in KDF counter mode (`api/auxfunc.c:482`), which has the prefix
  property, so `output = XOF(aux)[0 : outlen]` and
  `new_ctx = XOF(aux)[0 : 26 bytes]` (`CTX_LEN·8` = 208 is a length in bits; the
  other 182 bytes of the 208-byte context are not rewritten). Consequences: (i) any
  squeeze of ≥ 26 bytes hands an observer the context's entire next state, hence
  its future output — the code does squeeze
  ⌈n/8⌉ = 2 209…24 641 bytes for `h`; (ii) every shorter output, e.g. the
  16-byte public `salt`, is literally the first 16 bytes of the generator's
  next internal state, leaving 10 state bytes not shown by a ciphertext; the
  consequence for the encapsulation generator is filed under kem-17-3. The
  construction has no forward secrecy and is not what a XOF-based PRNG should
  look like. The spec does not define the ratchet at all.
- **(a) API contract: the four `[out]` length parameters are never written.**
  `kem_keygen` does `(void)pk_len_bytes; (void)sk_len_bytes;`,
  `kem_enc` does `(void)ss_len_bytes; (void)ct_len_bytes;`, and `kem_dec` does
  `(void)ss_len_bytes;` (`ref/KEM_HEP_QC.c`), although `KEM_HEP_QC.h:44-66`
  declares all of them `@param[out]`. A caller that does not pre-fill them from
  `kem_get_*_len_bytes()` reads uninitialised memory. The same functions also
  ignore the `[in]` lengths (`pk_len_bytes`, `sk_len_bytes`, `ct_len_bytes`),
  so a short buffer is over-read rather than rejected.
- **(a) `kem_dec` always returns 0**, even when re-encryption fails, although
  `KEM_HEP_QC.h:74` states "if decapsulation unsuccessfully, return −1".
  Implicit rejection does still fire (the returned shared secret is K̄), so this
  is a contract deviation rather than a break; kem-16 HARE returns −1 in the
  same situation.
- **(a) `ALGORITHM_INSTANCE` is the untouched template placeholder
  `"AlgorithmInstance"`** (`ref/KEM_HEP_QC.h:21`) for all four instances, so
  the four parameter sets are indistinguishable through the metadata block and
  their KAT files collide in name.
- **(c, equivalent) Expand/Truncate are folded into the word arithmetic.**
  `hep_qc_pke_encrypt` adds `s·r2 + e` over `VEC_N1N2_SIZE_64` words instead of
  materialising `ExpandVect(·, n′ − n)`, and `hep_qc_pke_decrypt` likewise.
  Equivalent because n1n2 ≤ n ≤ n′ and the surplus words are zero, but it is
  not literally Algorithm 2 line 11 / Algorithm 3 lines 8–9.
- **(minor, performance/side channel) T⁻¹ is recomputed on every decryption.**
  `hep_qc_pke_decrypt` calls `invertible_matrix(t_inverse, t, MSG_BIT)` —
  a k×k F2 inversion (k up to 512) per decapsulation, on a secret matrix, with
  data-dependent pivoting.
- **(minor, UB) `uint8_t m[…]` is cast to `uint64_t *`** when passed to
  `hep_qc_pke_encrypt`/`hep_qc_pke_decrypt` (`ref/KEM_HEP_QC.c`, `m`,
  `m_prime`), an alignment violation that happens to work on x86-64.
- **(b, spec) §3.1 concedes that the symmetric primitives are placeholders**
  ("used solely for correctness verification … will be replaced with
  cryptographically secure hash functions and XOFs that meet the required
  security levels"). The security claims of §6 are therefore not made about the
  submitted instantiation.
- Packaging note (already in RESULTS.md): KAT files reach 507 MB (12.6 MB
  public keys at level 7); `Lib/drng` and `Lib/auxfunc` are byte-identical to
  the official copies modulo CRLF and the official `api/` ones are linked.

Not verified: the concatenated RS ∘ duplicated-RM decoder
(`common/{reed_solomon,reed_muller,code,fft}.c`) was read for structure only
and not checked against §3.4's decoding conditions; `GenerateMat$`,
`GeneratePerm$` and `ExpandMat$` were not checked for sampling bias. §6 DFR
and security analysis was not audited.
