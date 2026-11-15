# kem-03 BAG-Loong — algorithm summary

Rank-metric code-based KEM ("Loong with Blended-block errors and Augmented Gabidulin
codes"). Hardness: the 2-BRSL / ℓ-BRSD blended-block rank (syndrome) decoding problems
over F_{q^m}, q = 2 — an *unstructured* (no ideal/quasi-cyclic) rank instance. An
IND-CPA PKE whose auxiliary code is an **augmented Gabidulin** code is lifted to an
IND-CCA2 KEM by the **salted FO (SFO)** transform with implicit rejection.

Specification: `kem-03-spec.pdf` (26 pages, English), §1.3 Algorithm 1 (PKE),
§1.4 Algorithm 2 (KEM), §1.5 Tables 1–3 (parameters/sizes), §2.3 (AG decoding).

## Parameters

| parameter | 128 | 256 | 384 | 512 | meaning |
|---|---|---|---|---|---|
| n | 42 | 65 | 83 | 104 | H is n×n over F_{q^m} |
| m | 47 | 67 | 83 | 97 | extension degree (prime, anti-algebraic) |
| n′ | 46 | 67 | 83 | 97 | Gabidulin length inside the AG code (n′ ≤ m) |
| n₁ | 10 | 12 | 14 | 15 | X,Y are n×n₁; AG length is n₁n₂ |
| n₂ | 10 | 13 | 15 | 15 | ciphertext block rows |
| ε | 33 | 51 | 66 | 73 | augmentation/tail-support dimension |
| k | 3 | 4 | 5 | 6 | AG code dimension = message length in F_{q^m} |
| A_{X,Y} | [[5,4],[4,5]] | [[5,3],[3,6]] | [[6,3],[3,6]] | [[7,4],[4,7]] | keygen blended-rank matrix |
| A_{R1,E|R2} | [[5,3],[3,5]] | [[6,3],[3,6]] | [[7,4],[4,7]] | [[7,4],[4,7]] | encryption blended-rank matrix |
| f(X) | X⁴⁷+X⁵+1 | X⁶⁷+X⁵+X²+X+1 | X⁸³+X⁷+X⁴+X²+1 | (see note) | F_{q^m} = F_q[X]/⟨f⟩ (Table 2) |
| claimed security | 128 | 256 | 384 | 512 | bits classical (80/128/192/256 quantum) |

q = 2 throughout. Decoding radius δ = ⌊(n′ − k + ε)/2⌋ (§1.2, §2.2).

Sizes (bytes), specification (Table 3) vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | ct spec | ct impl | ss | match |
|---|---|---|---|---|---|---|---|---|
| BAG-Loong-128 | 2500 | 2500 | — | 5064 | 3055 | 3071 | 16 | pk yes, **ct +16** |
| BAG-Loong-256 | 6597 | 6597 | — | 13258 | 8384 | 8400 | 32 | pk yes, **ct +16** |
| BAG-Loong-384 | 12152 | 12152 | — | 24368 | 15096 | 15112 | 48 | pk yes, **ct +16** |
| BAG-Loong-512 | 19043 | 19043 | — | 38150 | 21644 | 21660 | 64 | pk yes, **ct +16** |

The spec's ct column is exactly ⌈(n·n₂ + n₁·n₂)·m/8⌉, i.e. (C₁,C₂) only; Algorithm 2
also outputs the 128-bit `salt`, so the shipped ciphertext is 16 bytes larger at every
level. The spec gives no secret-key size column.

## Pseudocode

### KeyGen — spec Algorithm 1 (PKE) + Algorithm 2 (KEM)
```
PKE.KeyGen(1^λ):
  ρ_H  <-$ {0,1}^{2λ}                       # spec text says {0,1}^512; size formula uses 2λ/8
  H    := shake256(ρ_H) in F_{q^m}^{n×n}
  (X,Y) <-$ S_A^{n×n1}(F_{q^m}) with A_(X,Y) = A and 1 in Supp(Y)   # blended-block rank pair
  S    := H·X + Y                           # (§2.2 eq.(1) writes S = X·H + Y — spec inconsistency)
  return pk' := (ρ_H, S), sk' := X          # code stores sk' = (ρ_H, X); ρ_H regenerates g and H; the keygen seed is not stored
KEM.KeyGen:
  (pk',sk') <- PKE.KeyGen(1^λ);  ξ <-$ {0,1}^512
  return pk := pk',  sk := (sk', pk', ξ)
```

### Encaps — spec Algorithm 2
```
Encap(pk):
  m    <-$ F_{q^m}^k ;  salt <-$ {0,1}^128
  (k,θ) <- G(ID(pk), m, salt)               # G: {0,1}* -> {0,1}^512, spec says SHA3-512
  ct   <- PKE.Enc(pk, m; θ)
  K    <- K(k, ct)                          # K: SHAKE256, truncated to λ/8 bytes
  return (ct, salt, K)

PKE.Enc(pk=(ρ_H,S), m; θ):
  H := shake256(ρ_H); G_AG := generator of the augmented Gabidulin code G_g^+(n1 n2, n', k, m)
  (R1, E, R2) <-$ S_{A'}^{n2×n'}(F_{q^m}) from seed θ, with A_(R1,E|R2) = A'
  C1 := R2·H + R1
  C2 := R2·S + E + Fold(m·G_AG)
  return C = (C1, C2)
```

### Decaps — spec Algorithm 2, with the AG decoder of §2.3
```
Decap(sk=(sk',pk',ξ), ct, salt):
  m' <- PKE.Dec(sk', ct)
  (k',θ') <- G(ID(pk'), m', salt)
  ct'     <- PKE.Enc(pk', m'; θ')
  if ct' = ct: return K(k', ct')  else: return K(ξ, ct')      # implicit rejection

PKE.Dec(sk'=X, C=(C1,C2)):
  Ybar := Unfold(C2 - C1·X)  in F_{q^m}^{n1 n2}
       =  m·G_AG + (R2·Y + E - R1·X)      # correctness needs w_R(R2Y - R1X) <= ⌊(n'-k+ε)/2⌋ = δ
  return m := D_{G_g^+}(Ybar)

# D_{G_g^+}: augmented-Gabidulin decoding, spec §2.3 ("new fast decoding algorithm", O(n^2) ops)
# The "augmented" part: gbar = (g | 0^{n1n2-n'}) — the Gabidulin code of length n' is padded
# with n1n2-n' zero coordinates, so those tail positions of the received word are PURE ERROR.
# That free error window is what lets the radius grow from ⌊(n'-k)/2⌋ (plain Gabidulin) to
# ⌊(n'-k+ε)/2⌋, at the cost of a nonzero DFR (§1.2 formula).
D_{G_g^+}(ybar):
  1. split ybar = (y | tail), |y| = n', |tail| = n1 n2 - n'
  2. E2 := <tail>_{F_q};  ε' := dim E2   (assumed <= ε)
  3. V2 := unique monic q-polynomial annihilating E2      # linearized annihilator
  4. z_i := V2(y_i), i = 1..n'                            # eq. (*) : V2(y_i)=V2∘f(g_i)+V2(e_i)
  5. decode z in the Gabidulin code G(g, n', ε'+k)        # d' = n'-ε'+k+1, corrects δ-ε' errors
     -> recovers the q-polynomial (V2 ∘ f)
  6. f := left-Euclidean-divide (V2 ∘ f) by V2 ; reject if remainder != 0
  7. return m = coefficients of f (k elements of F_{q^m})
```
The spec does **not** fix the inner Gabidulin decoder of step 5 (it only says "apply the
decoding algorithm for G(g, n′, ε+k)"); this is a genuine specification gap.

Hashes (spec §1.2): `G = SHA3-512`, `K = SHAKE256`, expansion of ρ_H and of the error
samplers by `shake256(·)`, `ID(·)` a deterministic entropy extraction from pk.

## Implementation vs specification

Checked `src/BAG-Loong-128/` (full read) plus a parameter diff over all four instances
(`src/<label>/src/loong_parameters.h`). Mapping: `loong_kem.c` = Algorithm 2,
`loong_pke.c` = Algorithm 1 (`loong_pke.c:1017` calls the decoder), `augabidulin.c`
= §2.3 steps 1–5, `gabidulin.c`/`qpoly.c` = the inner q-polynomial decoder and step 6,
`parsing.c` = packing, `lib/rbc-<m>/` = F_{q^m} arithmetic.

- **Parameters agree.** Sampled n, m, n′, n₁, n₂, ε, k, A-matrices and security bits for
  all four instances against Table 1: exact match (e.g. 128: n=42, m=47, n′=46, ε=33,
  k=3, A_{X,Y}=[[5,4],[4,5]]; 512: n=104, m=97, n′=97, ε=73, k=6). The file also carries
  `LOONG_PARAM_STATIC_ASSERT` lines re-asserting each value.
- **Decoder matches §2.3.** `augabidulin_code_decode()` (`augabidulin.c:107-179`):
  `recover_support_basis` on the last n₁n₂−n′ coordinates (step 2),
  `qpoly_set_interpolate_zero` for V2 (step 3), `qpoly_evaluate` loop (step 4),
  `gabidulin_code_init(..., k + tail_rank, n_prime)` then
  `gabidulin_code_decode_with_annihilator` (step 5), and `qpoly_left_div` against the
  annihilator with a zero-remainder check (step 6, `gabidulin.c:415-422`). Guard
  `k + ε ≤ n′` is enforced at init.
- **(a) Deviation — tail rank:** the code uses the *measured* tail rank rather than the
  fixed ε of the spec, giving the inner code dimension k+tail_rank, and rejects when
  `tail_rank > ε` (`augabidulin.c:30,145`). The specification uses ε as a minimum in the
  failure analysis, not as a cap on the decoder input; see kem-03-5.
- **(a) Deviation — symmetric primitives.** The spec names SHA3-512 / SHAKE256. The
  implementation uses the NGCC `lib/api_pkc` primitives instead: `loong_hash.c:187`
  `ID = sm3hash(256, ...)`, `loong_hash.c:196` `G = pseudohash(512, ...)`,
  `loong_hash.c:162/211` `K` and every XOF expansion = `pseudoXOF(...)`. Inputs are
  length-framed with a domain tag (`build_tagged_message`), which the spec does not
  describe. Plausibly mandated by the competition API, but the spec was not updated.
- **(a) Deviation — (k,θ) is not split.** Spec: `(k,θ) ← G(...)`, a 512-bit value split
  into a pre-key and encryption coins. `loong_kem.h:6-7` sets
  `LOONG_KEM_G_SECRET_BYTES = LOONG_KEM_THETA_BYTES = LOONG_G_BYTES = 64`, and
  `loong_kem.c:148/159` passes the *same* 64 bytes as both θ (encryption seed) and k
  (KDF secret). The two uses are separated only by the tags `BAG-Loong/PKE-enc/v1` and
  `BAG-Loong/K/v1` (`loong_hash.c:12,15,225,258`).
- **(b) Spec ambiguity — KDF binds ct′, not ct.** Algorithm 2 literally writes
  `K(k',ct')` and `K(ξ,ct')`; `loong_kem.c:241` follows it (`ct_prime` in both branches).
  Consequence: the rejection key is bound to the re-encryption, not to the received
  ciphertext, and `salt` never enters K. This is the spec's own wording, but it is not
  the standard SFO binding.
- **Size mismatch:** every instance's ciphertext is spec+16 (the salt); see table above.
  pk matches exactly (2λ/8 seed + ⌈n·n₁·m/8⌉); sk = 2λ/8 + |X| + |pk| + 64 (ξ), which is
  self-consistent but never stated in the spec.
- **Spec-internal inconsistency:** Table 2 gives f(X) = X¹⁰⁴+X⁴+X³+X+1 for BAG-Loong-512,
  but that instance has m = 97; the implementation uses degree 97
  (`LOONG_FIELD_POLY_DEGREE 97`, with an explicit `LOONG_FIELD_POLY_NEEDS_ERRATUM 0`
  assert). Table 2's 512 row appears to be an erratum (n = 104 substituted for m = 97).
  Also §1.3 writes S := H·X + Y while §2.2 eq.(1) writes (I H)(Y;X)ᵀ = S.
- **Not verified / caveats:** the support sampler `sample_support_pair_logical`
  (`loong_pke.c:350-418`) discards its XOF reader and fills the supports from the fixed
  pool 1, β, β², … (kem-03-3); only the matrix coefficients come from the seed. The
  position-rejection logic was not traced line by line, and the DFR formula of §1.2
  was not recomputed. The decode path is **not constant time**: `recover_support_basis`
  branches per coordinate, `augabidulin.c` and `gabidulin.c` return
  `LOONG_ERR_CRYPTO_REJECT` early on secret-dependent conditions, and
  `loong_kem.c:219-221` branches on `dec_status`. Only the final compare/select
  (`loong_ct_equal`/`loong_ct_select`) is constant time. This matches the
  `KEM-CONSTANT-TIME-REJECT-SELECT` item left `not_tested` in `security_findings.md`.
