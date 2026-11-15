# kem-16 HARE — algorithm summary

HARE is an HQC-style code-based KEM: an IND-CPA PKE over the quasi-cyclic ring
R = F2[x]/(x^n − 1) whose hardness is quasi-cyclic syndrome decoding (2-QCSD),
lifted to IND-CCA2 by the Fujisaki–Okamoto transform in the HHK form with
implicit rejection and a salt. Its three deltas from HQC are (i) *asymmetric*
error weights (w, w_r for the key and r-vectors, a larger w_e for e),
(ii) *distance-informed erasure decoding* of the duplicated Reed–Muller inner
code (output ⊥ instead of the nearest codeword when the top two distances are
within a threshold α, so the outer RS code can use erasure decoding), and
(iii) ciphertext compression with a KR covering code [51,41] of radius 2
applied to the first l2 bits of v.

Specification: `kem-16-spec.pdf` (22 pages, v1.5), §3 (Algorithms 1–8: syndrome
table decoding, RM Decode′, PKE.Keygen/Encrypt/Decrypt = Alg. 3–5,
KEM.Keygen/Encaps/Decaps = Alg. 6–8), §4 Tables 2–3 (parameters and sizes).

## Parameters

Built instances map onto the spec's six sets by classical security level:
HARE-128 = **HARE-2**, HARE-256 = **HARE-5**, HARE-384 = **HARE-7**,
HARE-512 = **HARE-9**.

| parameter | HARE-128 (=HARE-2) | HARE-256 (=HARE-5) | HARE-384 (=HARE-7) | HARE-512 (=HARE-9) | meaning |
|---|---|---|---|---|---|
| n | 20899 | 52379 | 104869 | 173981 | QC ring length |
| k | 128 b (16 B) | 256 b (32 B) | 384 b (48 B) | 512 b (64 B) | message dim. of C1 |
| w = w_r | 79 | 131 | 193 | 259 | weight of (x,y) and (r1,r2) |
| w_e | 145 | 224 | 361 | 449 | weight of e |
| n1 (RS length) | 32 | 81 | 91 | 151 | outer Reed–Solomon |
| n2 (RM length) | 640 | 640 | 1152 | 1152 | inner duplicated Reed–Muller |
| α | 11 | 9 | 15 | 13 | erasure threshold in Decode′ |
| covering code | [51,41] R=2 | [51,41] R=2 | [51,41] R=2 | [51,41] R=2 | KR code for C2 |
| \|seed\|=\|salt\|=\|K\| | 16 | 32 | 48 | 64 | aligned to classical level |
| classical / quantum sec. | 128 / 80 | 256 / 128 | 384 / 192 | 512 / 256 | bits (spec Table 2) |
| DFR | < 2⁻¹³⁰ | < 2⁻²⁵⁸ | < 2⁻³⁸⁴ | < 2⁻⁵¹⁹ | spec's own bound |

Sizes (bytes), specification Table 3 vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | ct spec | ct impl | ss spec | ss impl | match |
|---|---|---|---|---|---|---|---|---|---|
| HARE-128-kr | 2629 | 2629 | 2677 | 2677 | 4688 | 4688 | 16 | 16 | yes |
| HARE-256-kr | 6580 | 6580 | 6676 | 6676 | 11790 | 11790 | 32 | 32 | yes |
| HARE-384-kr | 13157 | 13157 | 13301 | 13301 | 23693 | 23693 | 48 | 48 | yes |
| HARE-512-kr | 21812 | 21812 | 22004 | 22004 | 39294 | 39294 | 64 | 64 | yes |

## Pseudocode

### PKE.Keygen (spec Algorithm 3)
```
1  seed_dk, seed_ek <- I(seed_PKE)
2  ctx_dk <- XOF.Init(seed_dk) ; ctx_ek <- XOF.Init(seed_ek)
3  y <- SampleFixedWeightVec(ctx_dk, R_w)
4  x <- SampleFixedWeightVec(ctx_dk, R_w)
5  dk_PKE <- seed_dk
6  h <- SampleVec(ctx_ek, R)
7  s <- x + h·y
8  ek_PKE <- (seed_ek, s)
```

### PKE.Encrypt (spec Algorithm 4)
```
 1  seed_ek <- ek_PKE[0 : |seed|-1] ; ctx_ek <- XOF.Init(seed_ek)
 2  h <- SampleVec(ctx_ek, R) ;  s <- ek_PKE[|seed| : …]
 3  ctx_θ <- XOF.Init(θ)
 4  e  <- SampleFixedWeightVec(ctx_θ, R_{w_e})      # spec order: e, r1, r2
 5  r1 <- SampleFixedWeightVec(ctx_θ, R_{w_r})
 6  r2 <- SampleFixedWeightVec(ctx_θ, R_{w_r})
 7  u <- r1 + h·r2
 8  v <- m·G1 + Truncate(s·r2 + e, l1)              # l1 = n1·n2
 9  v1 <- v[0, l2-1] ; v2 <- v[l2, l1-1]            # l2 = n_cov·⌊l1/n_cov⌋
10  m̃ <- C2.Decode(v1)                              # KR covering code, compression
11  c <- (u, m̃, v2)
```

### PKE.Decrypt (spec Algorithm 5)
```
1  ctx_dk <- XOF.Init(dk_PKE) ;  y <- SampleFixedWeightVec(ctx_dk, R_w)
2  parse c = (u, m̃, v2)
3  ṽ <- (m̃·G2 ‖ v2)
4  m <- C1.Decode(ṽ − Truncate(u·y, l1))
```
`C1` = RS(n1,k) ∘ duplicated RM(n2); `C1.Decode` runs RM `Decode′` (Alg. 2)
which returns ⊥ (an erasure) when the two nearest codewords differ in distance
by less than α, and then an RS decoder that corrects g errors and h erasures
whenever 2g + h ≤ n1 − k.

### KEM.Keygen / Encaps / Decaps (spec Algorithms 6–8, FO with implicit rejection)
```
Keygen:                                  Encaps(ek):
 1 seed_KEM <-$ B^{|seed|}                1 m    <-$ B^{|K|}
 2 ctx <- XOF.Init(seed_KEM)              2 salt <-$ B^{|salt|}
 3 seed_PKE <- XOF.GetBytes(ctx,|seed|)   3 (K,θ) <- G( H(ek) ‖ m ‖ salt )
 4 σ        <- XOF.GetBytes(ctx,|K|)      4 c_PKE <- PKE.Encrypt(ek, m, θ)
 5 (ek,dk_PKE) <- PKE.Keygen(seed_PKE)    5 c_KEM <- (c_PKE, salt)
 6 dk_KEM <- (ek, dk_PKE, σ, seed_KEM)    6 return (K, c_KEM)

Decaps(dk, c):
 1 parse dk = (ek, dk_PKE, σ, …) ;  parse c = (c_PKE, salt)
 2 m' <- PKE.Decrypt(dk_PKE, c_PKE)
 3 (K',θ') <- G( H(ek) ‖ m' ‖ salt )
 4 c'_KEM <- (PKE.Encrypt(ek, m', θ'), salt)
 5 K̄ <- J( H(ek) ‖ σ ‖ c_KEM )
 6 if m' = ⊥ or c'_KEM != c_KEM :  K' <- K̄
 7 return K'
```

Hashes: the spec leaves XOF, G, H, I, J abstract and defers to the ICCS API.
The submitted code (`common/symmetric_api_pkc.c`, `-DHARE_SYMMETRIC_MODE_B`)
instantiates all five with the ICCS `pseudoXOF` under explicit one-byte domain
separators (PRNG 0xB0, XOF 0xB1, H 0xB2, I 0xB3, G 0xB4, J 0xB5) and draws
system randomness from `drng_algorithm`.

## Implementation vs specification

Checked: `HARE-<n>/kr/{parameters.h,api.h,KEM_AlgorithmInstance.c}` for all
four instances, plus the shared core
`_shared/hare_core/common/{kem.c,symmetric_api_pkc.c,code.c,parameter_contract.c}`
and `_shared/hare_core/ref/{hqc.c,vector.c,parsing.c,reed_muller.c,
reed_solomon.c,compression_kr.c,gf.c,gf2x.c}`. Not executed.

Agreements:
- Every parameter in `parameters.h` matches spec Table 2 for the corresponding
  row (n, w = ω, w_r, w_e, n1, n2, α; `PARAM_K` is k in *bytes*, i.e. k/8).
  The covering code is `PARAM_COMP_N 51 / PARAM_COMP_K 41 / PARAM_COMP_R 2`,
  the KR [51,41] code of §3.1. The RS field is `PARAM_GF_POLY 0x11D`, which is
  1 + α² + α³ + α⁴ + α⁸ as §3.2.2 specifies. Checked all four instances.
- The size formulas of §4.2 reproduce Table 3 and the library exactly:
  pk = |seed| + ⌈n/8⌉, sk = pk + 3·|seed|, ct = ⌈n/8⌉ + ⌈(nm + nv2)/8⌉ + |salt|
  with nm = 41·⌊l1/51⌋ and nv2 = l1 − 51·⌊l1/51⌋.
- The FO transform is present and correct: `crypto_kem_dec_status`
  (common/kem.c) recomputes (K',θ'), re-encrypts, compares the whole ciphertext
  with the constant-time `vect_compare` (returns 0 iff equal), and selects
  between K' and K̄ = J(H(ek)‖σ‖c) with a branch-free mask
  (`result |= compare_fail; result -= 1;` yields 0xFF or 0x00, and both inputs
  are in {0,1}). The salt is copied from the received ciphertext before
  re-encryption, so a salt substitution is caught by the comparison.
- `parameter_contract.c` turns the size and shape relations into
  compile-time assertions (n odd, n1 ≤ 255, n2 ≡ 0 mod 128, l1 ≤ n,
  l2 = 51·⌊l1/51⌋, the API byte counts), so a mis-set parameter fails to build.
- Randomness: `prng_get_bytes` is `get_random_number(&drng_algorithm, …)`
  (symmetric_api_pkc.c:90) — the seeded ICCS DRNG, used for `seed_KEM` in
  Keygen and for `m`/`salt` in Encaps. No OS-entropy path is compiled in.
- `kem_enc`/`kem_dec` validate the supplied key/ciphertext lengths against
  `kem_get_*_len_bytes()` and return −2 on mismatch; `kem_dec` returns −1 on
  re-encryption failure (while still writing the implicit-rejection key).

Discrepancies:
- **(a) The order in which e, r1, r2 are drawn from ctx_θ differs from
  Algorithm 4.** The spec draws e (line 6), then r1 (7), then r2 (8);
  `hqc_pke_encrypt` (ref/hqc.c:115-117) draws **r2, e, r1**. Since encryption
  is called identically in Encaps and in the Decaps re-encryption, the
  implementation is internally consistent and its KATs reproduce — but an
  independent implementation written from Algorithm 4 would produce different
  ciphertexts and shared secrets for the same (m, θ). This is an
  interoperability-breaking deviation, not a security one.
- **(b, spec) §4.2's ciphertext-size formula is wrong.**
  `|c_KEM| = ⌈n/8⌉ + ⌈(l1 − l2 + k2·⌊l1/n2⌋)/8⌉ + |salt|` uses n2 (the RM
  length) where it must be the covering-code length n_cov = 51: at HARE-2 it
  gives 41·⌊20480/640⌋ = 1312 bits instead of 41·⌊20480/51⌋ = 16441. The number
  printed in Table 3 (4688) matches the correct formula and the code; only the
  displayed formula is wrong.
- **(b, spec) Table 1 and Table 2 disagree on the outer RS codes.** Table 1
  lists RS-S1 (n=40, k=16), RS-S3 (n=50, k=24), RS-S5 (n=70, k=32), while
  Table 2's n1 column gives 41, 32, 51, 81, 91, 151 for HARE-1/2/3/5/7/9.
  The implementation follows Table 2 (`PARAM_N1` = 32 / 81 / 91 / 151), with
  `PARAM_DELTA = (n1 − k)/2` the corresponding error-correction capability.
  Table 1 describes codes that no shipped parameter set uses.
- **(a, minor) `hqc_pke_decrypt` always returns 0** (ref/hqc.c:219) — the
  `m' = ⊥` branch of Algorithm 8 line 11 is never signalled from the PKE. In
  practice a decode failure is still caught by the re-encryption comparison, so
  the implicit rejection still fires; the code's `result` variable is
  nonetheless dead at that point.
- **(a, minor, side channel) key generation uses the variable-time sampler.**
  `hqc_pke_keygen` calls `vect_sample_fixed_weight1` for the secret x and y,
  which is rejection sampling plus an O(w²) duplicate scan with a data-dependent
  loop count (ref/vector.c:62-93); `hqc_pke_decrypt` calls it again for y in
  every decapsulation (ref/hqc.c:185, ref/parsing.c:53-56); encryption uses
  the constant-time `vect_sample_fixed_weight2` (HQC's ePrint 2021/1631 Algorithm 5). The
  specification says nothing about constant-time requirements, but the
  asymmetry means the long-term secret support is the one sampled in variable
  time.
- **(minor, portability) `vect_generate_random_support2` casts a `uint32_t[]`
  to `uint8_t*` for the XOF output** (ref/vector.c:107), so the sampled
  supports are little-endian-dependent. Inherited from the HQC reference; the
  build targets are little-endian.
- Known packaging note (already in RESULTS.md): the candidate's own
  `_shared/api_pkc/drng.c` and `auxfunc.c` are lightly modified copies of the
  official files (UB-free `rotl32`); the harness links the official `api/`
  copies, which are output-equivalent.

Not verified: the erasure-decoding Decode′ threshold logic in
`ref/reed_muller.c` and the erasure-tolerant RS decoder in
`ref/reed_solomon.c` were read for structure but not checked against §3.2's
decoding conditions; the KR syndrome table (`kr_syndrome_table_generated.h`)
was not re-derived. §5/§6 DFR analysis was not audited.
