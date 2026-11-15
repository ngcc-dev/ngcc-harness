# kem-08 BW-KEM — algorithm summary

Module-lattice KEM over `R_q = Z_q[x]/(x^N+1)`, hardness = decisional MLWE.
An ML-KEM-shaped IND-CPA PKE (BW-PKE) is made IND-CCA by the
`FO^{≢⊥}_{ID(pk),m}` transform (implicit rejection, multi-user hardened). The
one structural change versus ML-KEM is the message layer: a *scalable nested
Barnes-Wall lattice code* `C_{n,τ}(λ)` replaces 1-bit-per-coefficient
modulation, giving genuine error correction.

Specification: `kem-08-spec.pdf` (35 pages); §3 lattice code (Alg. 1-3),
§4.1 BW-PKE (Fig. 2), §4.2 BW-KEM (Fig. 4), §6.2 parameters (Tables 1-2),
§7 sizes (Table 4).

## Parameters

| parameter | c128 | c256 | c512 | meaning |
|---|---|---|---|---|
| N | 256 | 256 | 512 | ring degree |
| q | 3329 | 3329 | 3329 | modulus (same as ML-KEM) |
| q̂ | 4096 | 4096 | 4096 | `2^⌈log2 q⌉`, decode-side scaling |
| l | 2 | 4 | 4 | module rank |
| n | 8 | 32 | 32 | BW lattice dimension (Table 1) |
| τ | 2 | 2 | 2 | shaping exponent (Table 1) |
| μ0 | 4 | 32 | 32 | message bits per BW block (Table 1) |
| (η_s, η_e, η_ct) | (6,6,5) | (3,3,3) | (2,2,2) | CBD parameters |
| (du, dv) | (10,4) | (10,5) | (10,6) | compression bits |
| κ ("λ" col., Table 2) | 128 | 256 | 512 | seed / shared-key bits |
| DFR δ | 2^-138.1 | 2^-209.9 | 2^-232.7 | spec's claim |
| claimed security (sec.c, sec.q) | (131,119) | (267,242) | (556,504) | core-SVP bits |

Sizes (bytes), specification (Table 4) vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | ct spec | ct impl | ss spec | ss impl | match |
|---|---|---|---|---|---|---|---|---|---|
| BW_KEM_C128 | 784 | 784 | 1585 | 1585 | 768 | 768 | 16 | 16 | yes |
| BW_KEM_C256 | 1568 | 1568 | 3169 | 3169 | 1440 | 1440 | 32 | 32 | yes |
| BW_KEM_C512 | 3136 | 3136 | 6337 | 6337 | 2944 | 2944 | 64 | 64 | yes |

The spec also defines c384 and eight `-s` (low-DFR) sets; only c128/c256/c512
are submitted as code (§7.1).

## Pseudocode

### Barnes-Wall message layer (§3.2, Alg. 1-3)

```
Encode_{C_{n,τ}(λ)}(m ∈ {0,1}^μ):            # Alg. 1;  n = 2^k, φ = 1+i
  pad m with 0s to length τn − (n/4)(k−1)
  split into n/2 blocks m_j of 2τ − wH(j) bits      # wH = Hamming weight of j
  v_j := ξ_{2τ−wH(j)}(m_j) ∈ Z[i]           # low ⌈l/2⌉ bits → Re, rest → Im
  return w := λ · (v·W_n mod 2^τ)           # W_n = (k−1)-fold Kronecker of [[1,1],[0,φ]]

BDD_{C_{n,τ}(λ)}(x ∈ C^{n/2}):               # Alg. 2, recursive, integer-only
  if n = 2: return λ·(⌊x/λ⌉ mod 2^τ)
  (x1,x2) := halves of x;  y1 := BDD_{n/2}(x1);  y2 := BDD_{n/2}(x2)
  z1 := ½·BDD_{C_{n/2,τ}(2λ)}(2φ^{-1}(x2−y1))    # the 2λ scaling keeps arithmetic integral
  z2 := ½·BDD_{C_{n/2,τ}(2λ)}(2φ^{-1}(x1−y2))
  dis1 := ‖y1−x1‖²_{2^τλ,2} + ‖y1+φz1−x2‖²_{2^τλ,2};  dis2 := symmetric
  return (y1, y1+φz1) if dis1 < dis2 else (y2+φz2, y2)

Decode_{C_{n,τ}(λ)}(x):                      # Alg. 3
  w := BDD_{C_{n,τ}(λ)}(x);  v := (1/λ)·w·W_n^{-1};  v_j = a_j + b_j i
  for j: b'_j := b_j mod 2^{τ−⌈wH(j)/2⌉};  a'_j := a_j − (b_j−b'_j) mod 2^{τ−⌊wH(j)/2⌋}
         m_j := ξ^{-1}(a'_j + b'_j i)
  return first μ bits of (m_0‖…‖m_{n/2−1})
```
Correction radius (Lemma 2 / Thm. 2): decoding succeeds whenever
`‖x − Encode(m)‖_{2^τλ,2} < r = sqrt(n/8)·λ`, the packing radius of `BW_n(λ)`
(`d²_min = (n/2)λ²`). Thm. 3 turns this into the per-block noise budget
`r0 = sqrt(2n)·q/2^{τ+2} − (sqrt(n)/2)(q/q̂ + 1)`. `PolyEncode` maps each of
the `d = N/n` message blocks through `Encode_{C_{n,τ}(q/2^τ)}`; `PolyDecode`
uses `Decode_{C_{n,τ}(q̂/2^τ)}` after the modulus switch. Spec places the n
coefficients of a block *d positions apart*.

### KeyGen (BW-PKE.KeyGen + BW-KEM.KeyGen, Figs. 2 and 4)

```
ρ, σ ←$ {0,1}^κ ;  z ←$ {0,1}^κ
A ∼ R_q^{l×l} := SampleA(ρ)
(s, e) ∼ β_{η1}^l × β_{η1}^l := SampleSE_{η1}(σ)
t := A·s + e
return pk := (t, ρ),  sk := (s, pk, ID(pk), z)
```

### Encaps (Fig. 4 + BW-PKE.Enc)

```
m ←$ M = {0,1}^{(N/n)·μ0}
(K, r) := H(ID(pk), m)
  A := SampleA(ρ); r ∼ β_{η1}^l := SampleR_{η1}(r); (e1,e2) := SampleE_{η2}(r)
  u0 := A^T r + e1 ;  v0 := t^T r + e2
  u := Compress_{q,2^du}(u0)
  v := Compress_{q,2^dv}(v0 + ⌊PolyEncode(m)⌉)
return c := (u,v), K
```

### Decaps (Fig. 4 + BW-PKE.Dec)

```
u' := Decompress_{2^du,q}(u);  v' := Decompress_{2^dv,q}(v)
w  := ⌊(q̂/q)(v' − s^T u')⌉                    # modulus switch q → q̂
m' := PolyDecode(w mod± q̂)
(K', r') := H(ID(pk), m')
K̃  := H1(ID(pk), z, c)
if BW-PKE.Enc(pk, m'; r') ≠ c: return K̃  else return K'   # implicit rejection
```
Hashes: all of `H`, `H1`, `SampleA` (XOF), `SampleSE/R/E` (PRF) are
instantiated from the ICCS **SM3**-based auxiliary functions, not SHAKE (§7.1).

## Implementation vs specification

Checked `src/BW_KEM_C{128,256,512}/` (symlinks into
`Implementations/Reference_Implementation/`): `kem.c` (FO layer), `indcpa.c` +
`poly.c` (BW-PKE, pack/compress, modulus switch), `BWcoding.c` (codec),
`params.h`, `symmetric-iccs.c` (SM3 hash/XOF/PRF/rkprf).

**Agreements.** All 12 size figures in Table 4 match the built library exactly
(no size mismatch). Parameter spot-check, 8 constants per instance from
`params.h` (C128 uses named `BWKEM128_*` macros, C256/C512 plain `KYBER_*`):
`Q 3329`; `N 256/256/512`; `K 2/4/4`; `ETA1/ETA2` `(6,5)/(3,3)/(2,2)`; `du`
via `POLYVECCOMPRESSEDBYTES = l·N·10/8` → 10 everywhere; `dv` via
`POLYCOMPRESSEDBYTES` 128/160/384 → 4/5/6; `INDCPA_MSGBYTES` 16/32/64 =
`(N/n)·μ0` bits; `SECRETKEYBYTES = sk_cpa + pk + PREFIXHASHBYTES + κ/8`
= 1585/3169/6337. All agree with Tables 1-2 and 4.
FO structure in `kem.c:crypto_kem_dec` is correct: `verify()` compares the
re-encryption, then `rkprf()` writes K̃ and `cmov(ss, kr, …, !fail)` overwrites
it only on success — the shared secret is selected **after** the comparison,
branch-free, so implicit rejection is genuinely reached there. The uniform-API
entry point `KEM_AlgorithmInstance.c:kem_dec` does not call `crypto_kem_dec`;
it repeats the same steps and then returns `KEM_API_DECAP_FAILURE` when
`fail != 0` (reference C128 `:116-123`), which Fig. 4 does not specify
(`kem-08-3`).

**Discrepancies.**
- (a) **Block interleaving.** Fig. 2 `PolyEncode`/`PolyDecode` place the n
  coefficients of a BW block `d = N/n` positions apart (lines 6 / 2-3); the
  code uses *contiguous* n-coefficient blocks — `BWcoding.c:codec_encode`
  (`out + block_index*8`) for C128, and `poly.c:poly_tomsg`/`poly_frommsg`
  (`a->coeffs[32*i+j]`, C256 `poly.c:199-225`, C512 same) for C256/C512.
  §5.1 (Lemma 4) explicitly derives the DFR from the *strided* block
  distribution, so the shipped layout is not the one the DFR claim analyses.
- (c) **C128 codec is a specialised subcode.** For n=8, τ=2 Alg. 1 allows
  μ0 ≤ 12, but Table 1 fixes μ0 = 4 and `BWcoding.c` (C128) realises it as a
  16-codeword binary first-order Reed-Muller code
  (`generator_masks {0x55,0x0f,0x3c,0xf0}`) at level `q/2`, decoded by
  minimum-cost search over both `D8` cosets rather than by the recursive
  Alg. 2. Legitimate and stronger (`d²_min = q²` vs `q²/4` for `BW_8(q/4)`),
  but textually not Alg. 1/2, and the spec never names the subcode. C256/C512
  do implement Alg. 2 recursively (`BDD_2/4/8/16/32`, pre-scaled by `2^τ`:
  `lambda = 1<<KYBER_EQ`).
- (c) **ID(pk) is a prefix, not a hash.** The code sets
  `ID(pk) := pk[0 .. PREFIXHASHBYTES-1]` (first κ/8+1 bytes of packed `t`,
  17/33/65), with `hash_h(...)` commented out in
  `kem.c:crypto_kem_keypair_derand` / `crypto_kem_enc_derand`. Spec leaves
  `ID : PK → {0,1}^γ` abstract, so a prefix is admissible, but Thm. 5's bound
  carries a `2^{-ℓ}` term for `ℓ = H∞(ID(pk))` that is then unjustified.
- (b) **K̃ omits ID(pk).** Spec Fig. 4 line 3 is `K̃ := H1(ID(pk), z, c)`;
  `symmetric-iccs.c:kyber_iccs_rkprf` hashes only `z ‖ ct`. Affects the
  multi-user domain separation the `FO^{≢⊥}_{ID(pk),m}` variant is meant to
  provide, not single-user CCA.
- (b) **Spec typos/collisions.** `λ` is the §3 lattice scaling factor, but the
  Table 2 column headed `λ` holds 128/256/512 = the seed/key bit-length κ
  (matches `SYMBYTES` 16/32/64); Table 2 gives no column for the real scaling
  factor. Fig. 2 `PolyEncode` line 1 ("for j from 0 to N−1") should read
  `N/n − 1`.

**Not verified** (time-boxed): the C256/C512 `BDD_*` scaling algebra
line-by-line against Alg. 2, and the §5 DFR derivations. No build or shipped
binary was run; sizes come from the pre-recorded library metadata.
