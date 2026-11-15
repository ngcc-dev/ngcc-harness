# kem-26 NSS-HQC — algorithm summary

Code-based KEM in the HQC family: an IND-CPA PKE whose security rests on the
decisional quasi-cyclic syndrome decoding (DQCSD) problem over the cyclic ring
R = F2[X]/(X^n - 1), lifted to IND-CCA2 by a salted Fujisaki–Okamoto transform
with implicit rejection. NSS-HQC differs from standard HQC in three "core
modifications" it claims as novel: (M1) *asymmetric ephemeral weights*
w_r1 = w_r2 < w_r^std and w_e > w_e^std at constant total weight; (M2)
*blockwise quantisation* of the second ciphertext component v (Q = 4 levels,
b = 2 bits/symbol) with a public *dither*; and (M3) *reliability-based outer
decoding*, i.e. RS decoding with erasures driven by a per-block reliability
score from the Reed–Muller inner decoder.

Specification: `kem-26-spec.pdf` (53 pages, English), §2.8 (PKE) and §2.9 (KEM);
parameters in §2.11 Tables 1–4; sizes in Table 15.

## Parameters

| parameter | HQC-128 | HQC-256 | HQC-384 | HQC-512 | meaning |
|---|---|---|---|---|---|
| n | 29443 | 54493 | 122579 | 217901 | ring length (prime, ord_n(2)=n-1) |
| w_sk | 73 | 117 | 175 | 233 | weight of each secret vector x, y |
| w_r1 = w_r2 | 66 | 106 | 159 | 213 | ephemeral weights (M1, reduced) |
| w_e | 117 | 187 | 279 | 372 | error weight (M1, increased) |
| n1, k1 | 46, 16 | 70, 32 | 100, 48 | 130, 64 | shortened RS code over F256 |
| mult | 5 | 5 | 5 | 5 | RM duplication factor |
| n2 | 128 | 128 | 128 | 128 | RM(1,7) block length |
| n_C = n1·n2·mult | 29440 | 44800 | 64000 | 83200 | concatenated code length |
| Q, b | 4, 2 | 4, 2 | 4, 2 | 4, 2 | quantisation levels / bits (M2) |
| τ | 0.30 | 0.28 | 0.26 | 0.25 | erasure threshold (M3) |
| ε_max = ⌊(δ1−1)/2⌋ | 15 | 19 | 26 | 33 | erasure cap, δ1 = n1−k1+1 |
| κ (= \|ss\|) | 256 | 256 | 384 | 512 | shared-secret bits |
| λ_salt, λ_seed | 256 | 256 | 384 | 512 | salt / seed bits |
| claimed security | 128 cl / 80 qu | 256 / 128 | 384 / 192 | 512 / 256 | bits (spec's own claim) |

Hashes: G = I = SHA3-512, H_κ = SHAKE256, Expand = SHAKE256 XOF (§2.4).

Sizes (bytes), specification (Table 15) vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | ct spec | ct impl | ss spec | ss impl | match |
|---|---|---|---|---|---|---|---|---|---|
| HQC-128 | 3713 | 3713 | 3777 | 3777 | 5185 | 5185 | 32 | 32 | yes |
| HQC-256 | 6844 | 6844 | 6908 | 6908 | 9084 | 9084 | 32 | 32 | yes |
| HQC-384 | 15371 | 15371 | 15467 | 15467 | 18571 | 18571 | 48 | 48 | yes |
| HQC-512 | 27302 | 27302 | 27430 | 27430 | 31462 | 31462 | 64 | 64 | yes |

All four agree exactly. Structure: pk = seed_PKE ∥ s (λ_seed/8 + ⌈n/8⌉);
sk = seed_sk ∥ σ ∥ pk; ct_full = u ∥ v̂ ∥ salt, with |v̂| = n1·n2·b bits.

## Pseudocode

### PKE.KeyGen — Algorithm 1 (§2.8.1)
```
seed_PKE, seed_sk <-$ {0,1}^λseed ;  σ <-$ {0,1}^κ
h            <- Expand(seed_PKE ∥ "h", n)                  // h ∈ R
(seed_x, seed_y) <- I(seed_sk)                             // SHA3-512, two 256-bit halves
x <- R_{w_sk} via Expand(seed_x, ·)                        // constant-time rejection sampling
y <- R_{w_sk} via Expand(seed_y, ·)
s <- x + h·y                                               // XOR + cyclic convolution in R
return pk = (seed_PKE, s), sk = (seed_sk, σ, pk)
```

### PKE.Encrypt(pk, m, θ, salt) — Algorithm 2 (§2.8.2)
```
h  <- Expand(seed_PKE ∥ "h", n)
r1 <- R_{w_r1} via Expand(θ ∥ "r1", ·)                     // M1: asymmetric weights
r2 <- R_{w_r2} via Expand(θ ∥ "r2", ·)
e  <- R_{w_e}  via Expand(θ ∥ "e",  ·)
u      <- r1 + h·r2
v_raw  <- C.Encode(m) + π_{n_C}(s·r2 + e)                  // C = dup_mult ∘ RM(1,7) ∘ RS(n1,k1)
d_base <- Expand(salt ∥ pk ∥ "dither", n1·n2); d <- dup_mult(d_base)   // M2 dither
v̂      <- Q.CompressBlockwise(v_raw ⊕ d)                   // b = 2 bits per RM block
return ct = (u, v̂)
```

### PKE.Decrypt(sk, ct, salt) — Algorithm 3 (§2.8.3)
```
(seed_x, seed_y) <- I(seed_sk);  y <- R_{w_sk} via Expand(seed_y,·)   // only y is needed
d  <- dup_mult(Expand(salt ∥ pk ∥ "dither", n1·n2))
ṽ  <- Q.DecompressBlockwise(v̂) ⊕ d
w  <- ṽ - π_{n_C}(u·y)
for i = 1..n1:  (s_i, ρ_i) <- RM.Decode(w[block_i])        // symbol + reliability ρ_i
candidates   <- { i : ρ_i < τ }
erasure_mask <- the ε_max least reliable of candidates     // capped at ⌊(δ1-1)/2⌋
m' <- RS.DecodeErrorsAndErasures((s_1..s_n1), erasure_mask) // spec: Berlekamp-Massey + Forney
return m' or ⊥ on FAIL
```

### KEM.KeyGen / Encaps / Decaps — Algorithms 4, 5, 6 (§2.9)
```
KeyGen:  (ek, dk) <- PKE.KeyGen()                          // thin wrapper, ek = pk, dk = sk

Encaps(ek):
  m <-$ {0,1}^{8k1} ;  salt <-$ {0,1}^λsalt
  θ       <- G(m ∥ salt ∥ ek)                              // SHA3-512, derandomisation
  ct      <- PKE.Encrypt(ek, m, θ, salt)
  ct_full <- ct ∥ salt
  K       <- H_κ(m ∥ ct_full)                              // SHAKE256
  return (K, ct_full)

Decaps(dk, ct_full):                                       // FO with implicit rejection
  m'   <- PKE.Decrypt(dk, ct, salt)
  m̄    <- Select(m' = ⊥, 0^{8k1}, m')
  θ'   <- G(m̄ ∥ salt ∥ pk) ;  ct' <- PKE.Encrypt(pk, m̄, θ', salt)
  b_ok <- (m' ≠ ⊥) ∧ (ct' = ct)                            // byte-string compare
  K_acc <- H_κ(m̄ ∥ ct_full) ;  K_rej <- H_κ(σ ∥ ct_full)
  return CTSelect(b_ok, K_acc, K_rej)
```

## Implementation vs specification

Checked (built sources per `kem-26/Makefile`: `.../Reference_Implementation/<inst>/`
`nss_hqc_core.c`, `code_layer.c`, `KEM_AlgorithmInstance.c`, `auxfunc.c`, `drng.c`):
`nss_hqc_core.c` implements §2.8/§2.9 (`nss_hqc_keygen/enc/dec`, `hash_g`,
`hash_kappa`, `derive_h`, `derive_dither`), `code_layer.c` implements §2.6 (RS
encode/decode, RM(1,7) encode/decode with reliability, erasure selection).

Agreements:
- All of Table 1/3/4 is reproduced verbatim in `src/HQC-128/params.h` (n, n2,
  w_sk, w_r1, w_r2, w_e, n1, k1, mult, Q=4, b=2, τ=30/100, δ1=31, κ/λ_salt/λ_seed
  = 256) and spot-checked identically for HQC-384 and HQC-512 (n = 122579/217901,
  w_e = 279/372, τ = 26/100 and 25/100, κ = 384/512). No mismatch found.
- Sizes are hard-coded in `params.h` (`NSS_HQC_PK_BYTES` etc.) and equal both the
  spec's Table 15 and the OBSERVED library values for all four instances.
- FO transform present and correct: `nss_hqc_dec` (core `nss_hqc_core.c:995`)
  re-encrypts, compares with `ct_verify`, computes both K_acc and K_rej and
  selects with the constant-time `ct_select` — matching Algorithm 6 exactly.
- Hash inputs match the spec's concatenation order: `hash_g` builds m ∥ salt ∥ pk
  (`nss_hqc_core.c:398-411`), `hash_kappa` builds prefix ∥ ct_full (`:418-430`);
  θ = SHA3-512 truncated to κ/8 bytes, K = SHAKE256.
- M2 dither matches: `derive_dither_base` requests `NSS_HQC_NC / NSS_HQC_MULT`
  = n1·n2 bits then duplicates by `mult` (`nss_hqc_core.c:652-690`).
- M3 erasure cap matches: `nss_hqc_code_decode` un-marks the most reliable
  erasures until `erasures <= (n1-k1)/2` = ε_max (`code_layer.c:904-915`).

Discrepancies:
- **(a) deviation, low severity — RS decoder is not the specified one.** §2.6.3
  explicitly specifies "the Berlekamp–Massey algorithm … and Forney's formula
  (Eq. 22)". The implementation instead solves a Welch–Berlekamp key equation by
  Gaussian elimination over F256 (`code_layer.c:252 gf_solve`, `:355
  rs_try_decode`, `:490 rs_decode`). The decoding radius is the same, so this is
  closest to (c) a deliberate equivalent substitution, but it is undocumented and
  the spec's cited algorithm is not present anywhere in the tree.
- **(a) side channel in that decoder.** `rs_decode` loops `for errors = max_errors
  down to 0` and returns on the first success (`code_layer.c:515-529`), so the
  running time of decapsulation reveals the number of RS symbol errors. This is a
  secret-dependent branch inside the CCA decapsulation path, at odds with the
  spec's repeated "constant-time" claims for sampling; see also
  `kem-26/security_findings.md`.
- **(a) expansion tags, draw order and sampler differ from Algorithms 1–2.** The
  spec expands h, r1, r2 and e under separate tags ("h", "r1", "r2", "e"). The
  code appends the single byte 0x01 to every expansion input (`derive_h`,
  `nss_hqc_core.c:478`; `xof_stream_init`, `:500`) and draws r2, e, r1 in that
  order from one SHAKE256 stream on θ (`:719-723`). All fixed-weight vectors
  (x, y, r1, r2, e) come from `sample_fixed_weight1_stream` (`:532-577`), a
  partial Fisher–Yates shuffle with a rejection loop on the stream (`:553-563`)
  and swaps and bit writes at sampled indices (`:565-572`); it is not the
  "constant-time rejection sampling" the spec states.
- **(b) spec ambiguity, faithfully implemented.** The sub-seed width is fixed at
  256 bits: `#define NSS_HQC_I_SEED_BYTES 32u` in `src/HQC-512/nss_hqc_core.c:25`
  (identically in all four instances), so at HQC-384/HQC-512 the secret vectors
  x, y are derived from 32-byte sub-seeds even though Table 3 sets λ_seed = 384
  and 512 respectively. The spec's own Algorithm 1 comment ("SHA3-512 splits one
  256-bit seed into two 256-bit sub-seeds") says the same thing, so the spec is
  internally inconsistent with its own parameter table and the implementation
  follows the comment. seed_sk itself is still stored at full λ_seed/8 bytes
  (`params.h`), so the stored secret key is larger than the entropy it carries.
- **(cosmetic)** `params.h:4` documents its own provenance as "Provisional
  theory-aligned parameters extracted from ??????/??????/sections/06_params.tex"
  — a mojibake Chinese path, and "provisional" in a submitted reference
  implementation.

Not verified: the DFR analysis of §3, the concrete quantisation map Q (§2.7.1/2)
was read only at the level of Q = 4 / b = 2 block sizes, and RM(1,7) decoding
correctness was not checked against the spec's Eq. (13) reliability metric in
detail. The library was not executed; KAT conformance is reported separately in
`kem-26/security_findings.md` and `RESULTS.md`.
