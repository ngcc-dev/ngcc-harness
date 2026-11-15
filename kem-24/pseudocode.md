# kem-24 MORNING-Scabbard — algorithm summary

Module-LWR (rounding, no explicit error) KEM in the Saber family: power-of-two
moduli `t < p < q`, an ℓ×ℓ uniform public matrix expanded from a seed, CBD(η=2)
secrets, and multi-bit message packing through an extended **2D Minal code**
("B2-2D-Minal", 2 payload bits per coefficient, B = 2). The IND-CPA PKE is turned
into an IND-CCA2 KEM by a Kyber-style FO transform with implicit rejection and a
`KDF(K ‖ H(c))` final step.

Specification: `kem-24-spec.pdf` (31 pages), §1 "Algorithm Specification and
Description" — Algorithms 1–8 (helpers), 9–11 (PKE), 12–14 (KEM); parameters
Table 1 (p. 12), sizes Table 2 (p. 19), security/δ Table 7 (p. 19).

## Parameters

| parameter | Scabbard-128 | Scabbard-256 | Scabbard-512 | meaning |
|---|---|---|---|---|
| ℓ | 9 | 9 | 8 | module rank |
| n | 64 | 128 | 256 | ring degree, `Z_m[x]/(x^n+1)` |
| q = 2^ε_q | 2^14 | 2^13 | 2^13 | main modulus |
| p = 2^ε_p | 2^10 | 2^11 | 2^11 | rounded modulus |
| t = 2^ε_t | 2^3 | 2^2 | 2^6 | ciphertext-compression modulus |
| κ | 16 | 32 | 64 | seed / message / shared-secret bytes |
| B | 2 | 2 | 2 | payload bits per coefficient |
| β (Minal) | 1030 | 0 | 550 | 2D-Minal off-diagonal constant |
| η | 2 | 2 | 2 | CBD parameter (4 bits/coefficient) |
| h | 2^(ε_q−ε_p−1) = 8 | 2 | 2 | rounding constant |
| CSS / QSS | 2^128 / 2^116 | 2^266 / 2^241 | 2^514 / 2^466 | Core-SVP (spec Table 7) |
| δ (DFP) | 2^−99 | 2^−125 | 2^−125 | spec Table 7 |
| claimed security | 128 | 256 | 512 | bits |

Sizes (bytes), specification Table 2 vs the built reference library (OBSERVED):

| instance | pk spec | pk impl | sk spec | sk impl | ct spec | ct impl | ss spec | ss impl | match |
|---|---|---|---|---|---|---|---|---|---|
| scabbard128 | 736 | 736 | 1056 | 1056 | 760 | 760 | 16 | 16 | yes |
| scabbard256 | 1616 | 1616 | 2256 | 2256 | 1648 | 1648 | 32 | 32 | yes |
| scabbard512 | 2880 | 2880 | 4032 | 4032 | 3072 | 3072 | 64 | 64 | yes |

All three KATs reproduce (PASS ×3).

## Pseudocode

### PKE.KeyGen (spec Algorithm 9)
```
 1  d      <- $ B^kappa
 2  seedA  <- XOF(d, kappa)                     # so the RNG state is not published
 3  seed_s <- $ B^kappa
 4  A <- SampleMatrixUD(seedA)                  # A in R_q^{l x l}, PRF2(rho||i||j)
 5  s <- SampleSecretCBD(seed_s)                # s in R_q^l, CBD_2, PRF1(sigma||i)
 6  b <- ((A^T s + h) mod q) >> (eq - ep)       # h = 2^(eq-ep-1)  <-- round-to-nearest
 7  pk <- PolyToBytes_{ep}(b) || seedA
 8  sk <- PolyToBytes_4(s)                      # 4-bit two's complement per coeff
```

### PKE.Encrypt (Algorithm 10)
```
 1-3 b <- BytesToPoly_{ep}(pk);  seedA <- pk[...];  A <- SampleMatrixUD(seedA)
 4   s' <- SampleSecretCBD(r)
 5   u  <- ((A s' + h) mod q) >> (eq - ep)      # <-- h again
 6   v' <- b^T s' mod p
 7   mu <- EncodeMessage(m)                     # Minal, mu in R_{2^2,n}
 8   v  <- (2^(eq-ep) v' - mu mod q) >> (eq - et - 2)
 9-11 c <- PolyToBytes_{ep}(u) || PolyToBytes_{et+2}(v)
```

### PKE.Decrypt (Algorithm 11)
```
 3-5 u' <- BytesToPoly_{ep}(c1);  v' <- BytesToPoly_{et+2}(c2);  s' <- BytesToPoly_4(sk)
 6   s  <- (s' XOR 8) - 8 mod p                 # sign-extend the 4-bit nibbles
 7   w  <- u'^T s mod p
 8   m' <- (2^(eq-ep) w - 2^(eq-et-2) v') mod q
 9   m  <- DecodeMessage(m')
```

### KEM (Algorithms 12–14), XOF = pseudoXOF (ICCS SM3-based)
```
KeyGen:   (pk,sk_pke) <- PKE.KeyGen();  z <- $ B^kappa
          sk <- sk_pke || pk || XOF(pk,kappa) || z

Encaps:   m <- $ B^kappa ;  m <- XOF(m, kappa)
          (K, r) <- XOF( m || XOF(pk,kappa), 2*kappa )
          c <- PKE.Encrypt(pk, m, r)
          K <- XOF( K || XOF(c,kappa), kappa )

Decaps:   m'      <- PKE.Decrypt(sk, c)
          (K',r') <- XOF( m' || pkh, 2*kappa )       # pkh read from sk
          c'      <- PKE.Encrypt(pk, m', r')
          K <- (c == c') ? XOF(K'||XOF(c,kappa), kappa)
                         : XOF(z ||XOF(c,kappa), kappa)
```

### Message coding (Algorithms 5–8) — the correctness boundary
```
G1 = [[q/4, beta],[beta, q/4]]        G2 = [[q/2, 0, q/4, beta],[0, q/2, beta, q/4]]

MinalB2.Encode(m0,m1,m2,m3):  c = G2 * (m0,m1,m2,m3)^T  in Z_q^2
MinalB2.Decode(c):
  Phase 1 (bits m2,m3):  c' = c mod floor(q/2)
                         pick v in {00,01,10,11} minimising ||c' - G1 v mod± q/2||^2
  Phase 2 (bits m0,m1):  cbest = G1 (m2,m3)^T ;  (dx,dy) = c - cbest mod± q
                         m0 = floor(|dx| / floor(q/4)) ;  m1 = floor(|dy| / floor(q/4))
EncodeMessage / DecodeMessage apply this to coefficient pairs (i, i + n/2),
i = 0 .. 2n/4, giving mu = 2n payload bits = 8*kappa bits.
```

## Implementation vs specification

Built tree: `Implementations/Reference_Implementation/scabbard{128,256,512}`,
sources `auxfunc.c cbd.c drng.c indcpa.c KEM_scabbard<n>.c packing.c poly.c
polyvec.c verify.c minal.c`. API header is `KEM_scabbard<n>.h`. The three
directories have separate, differing copies of `indcpa.c`, `poly.c`, `minal.c`
(they are *not* a single parameterised source), which is where the divergence
below comes from. `cbd.c`, `polyvec.c` and `verify.c` are identical across all three;
`packing.c` is identical in scabbard128 and scabbard512 and differs in scabbard256
by one blank line.

Agreements:

- `params.h` matches spec Table 1 for all three sets: `SCABBARD_L` 9/9/8,
  `SCABBARD_N` 64/128/256, `SCABBARD_EQ` 14/13/13, `SCABBARD_EP` 10/11/11,
  `SCABBARD_ET` 3/2/6, `SCABBARD_SYMBYTES` (= κ) 16/32/64, `SCABBARD_B` 2, and
  `MINAL_BETA` 1030/0/550 (spec's β column) in each `minal.c:13`.
  `h1 = 1 << (SCABBARD_EQ - SCABBARD_EP - 1)` (`indcpa.h:7`) is exactly the spec's
  constant vector h.
- Sizes are derived (`params.h:30-38`) and evaluate to Table 2 exactly; nothing
  is hard-wired.
- The FO transform is present and correctly oriented
  (`KEM_scabbard128.c:101-131`): decrypt → `pseudoXOF(m'‖pkh)` → re-encrypt →
  `verify(ct, cmp, len)` → `cmov(kr, z, κ, fail)` → final
  `pseudoXOF(K‖H(ct))`. The condition is not inverted and the select is a
  constant-time `cmov`, not a branch.
- CBD sampling (`cbd.c:19-38`) implements Algorithm 3 for η = 2 (4 bits per
  coefficient, `a - b` with `a,b ∈ {0,1,2}`); `gen_a`/`gen_s`
  (`polyvec.c`) use `pseudoXOF(seed ‖ i ‖ j)` / `pseudoXOF(seed ‖ i)`, matching
  PRF2/PRF1 of Eqs. (1.1)–(1.2) and their output lengths `nε_q/8` and `n/2`.
- Secret-key packing is the spec's `PolyToBytes_4` / `(s'⊕8)−8`
  (`poly.c:53-74`), i.e. 4-bit two's-complement nibbles, N/2 bytes per polynomial.
- Randomness comes only from the official DRNG
  (`get_random_number(&drng_algorithm, …)` in `indcpa.c:24,26` and
  `KEM_scabbard128.c:63,80`), and both public seeds are passed through
  `pseudoXOF` first, as spec Algorithm 9 line 2 / Algorithm 13 line 2 require.
- Phase 1 of the Minal decoder really is *minimum*-distance decoding
  (`minal.c:128-134` uses `secure_min64` over `(dist²<<8)|idx`), matching
  Algorithm 6 line 5 (`if d < min dist`). Phase 2's `|δ| > ⌊q/4⌉−1` is equivalent
  to the spec's `⌊|δ|/⌊q/4⌉⌋`.

Discrepancies:

- **(a) Deviation — the rounding constant `h` is missing from Encrypt in
  scabbard128 and scabbard256.** Spec Algorithm 10 line 5 is
  `u ← ((A s' + h) mod q) ≫ (ε_q − ε_p)`. The code is:
  - `scabbard128/indcpa.c:68` — `u.vec[i].coeffs[j] = (u.vec[i].coeffs[j]) & SCABBARD_Q;` (**no `+ h1`**)
  - `scabbard256/indcpa.c:66` — same, **no `+ h1`**
  - `scabbard512/indcpa.c:68` — `… = (u.vec[i].coeffs[j] + h1) & SCABBARD_Q;` (**correct**)

  All three *do* add `h1` in KeyGen (`indcpa.c:36`/`:34`/`:36`), so this is not a
  consistent reinterpretation — the same submission implements the same spec line
  two different ways at different security levels. Omitting `h` turns
  round-to-nearest into truncation for `u`, so the rounding error `e_u = A s' − 2^{ε_q−ε_p}u`
  is uniform on `[0, 2^{ε_q−ε_p})` instead of centred on
  `[−2^{ε_q−ε_p−1}, 2^{ε_q−ε_p−1})`. The decryption noise term `e_u^T s` then
  picks up an extra `2^{ε_q−ε_p−1}·Σ s_i` component; its standard deviation roughly
  doubles (for scabbard128: ℓn = 576 CBD(2) coefficients, adding a
  σ ≈ 8·√576 = 192 term on top of the σ ≈ 111 truncation noise). The claimed
  δ = 2^−99 (Scabbard-128) and 2^−125 (Scabbard-256) of Table 7 are computed
  for the spec's centred rounding and therefore do not describe the shipped code;
  the true DFP is substantially larger. This matters directly: the spec itself
  notes (p. 28) that at δ = 2^−99 a 2^64-ciphertext decryption-failure attack
  already reaches 2^−35, and §3 conditions the IND-CCA2 proof on δ being
  negligible. (The numeric DFR re-derivation above is an order-of-magnitude
  estimate, not a computed failure probability — but the code/spec mismatch itself
  is unambiguous.)
- **(a) Deviation — the Minal decoder masks with `q−2`, not `q−1`.**
  `minal.c:144-145` in all three instances:
  `target[0] = target[0] & (SCABBARD_Q - 1);`. `SCABBARD_Q` is already the mask
  `q−1` (see its use as `& SCABBARD_Q` in `indcpa.c:36,68,77,100,102`), so
  `SCABBARD_Q - 1` is `q−2`, which **clears bit 0** of each coordinate rather than
  reducing mod q. The inputs are already reduced mod q by `indcpa.c:102`, so the
  net effect is a deterministic −0/−1 perturbation of each decoder input. Harmless
  in magnitude (1 part in 2^13/2^14) but it is not the operation the spec states,
  and it is an off-by-one that a reader would not expect to be intentional.
- **(c) Off-by-one Minal constants.** `minal.c:9-12` derive
  `B2_MINAL_HALF_Q = (q−1)/2` and `MINAL_ALPHA = (q−1)/4`, i.e. 8191/4095 for
  q = 2^14 and 4095/2047 for q = 2^13, where the spec's G1/G2 use `⌊q/2⌉ = 8192`
  and `⌊q/4⌉ = 4096` (resp. 4096/2048). Encoder and decoder use the same constants,
  so the code is self-consistent and the KATs pass; the induced deviation is ≤ 1
  per coefficient. Classified as an equivalent-up-to-1 implementation choice, not
  a break, but the encoded lattice is not literally the one in Eq. (1.7).
- **(c) Cosmetic.** `params.h:14` comments `SCABBARD_ETA 4 /* CBD: sample from
  2 * \eta bits */`; the constant is 2η = 4 (bits per coefficient), the comment
  reads as if it were η.

Not verified: the Core-SVP / gate-count numbers of Table 7, the δ values
themselves, and the Toom-Cook/Karatsuba `matrix_vector_mul` identities beyond
KAT reproduction.
