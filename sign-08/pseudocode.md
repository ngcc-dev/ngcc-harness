# sign-08 DARTS — algorithm summary

Module-lattice Fiat–Shamir **with aborts** (not hash-and-sign / no GPV trapdoor),
using **bimodal** rejection sampling over Euclidean **hyperballs** (ℓ2-norm,
"double sphere distribution") instead of discrete Gaussians. Security rests on
MLWE (key recovery) and MSIS (forgery) over `R_q = Z_q[x]/(x^n+1)`. Lineage:
HAETAE/Cheon et al. hyperball sampling + BLISS-style bimodal trick; keys are
built so that `A·s ≡ ((q+1)/2)·α (mod q)`, which is what makes the bimodal
`(-1)^b` branch verify. Signatures are entropy-coded with rANS.

Specification: `sign-08-spec.pdf` (28 pages), §1.1–§1.5 (Algorithms 1–8,
Tables 1–2). Hash/XOF instantiated as **SM3-XOF** in the built reference code
(spec Tables 4/5 give both SM3 and FIPS-202 benchmarks).

## Parameters

Spec Table 2 (p. 13). Code symbol names in parentheses where they differ.

| parameter | DARTS128 | DARTS256 | DARTS512 | meaning |
|---|---|---|---|---|
| n | 512 | 512 | 1024 | degree of R, R_q |
| q | 130817 | 130817 | 260609 | modulus (prime, n/2 \| q-1 → incomplete NTT) |
| (k, ℓ) | (1, 2) | (2, 3) | (2, 3) | dimensions of z2, z1 |
| ρ(λ) | 32 | 32 | 64 | seed bytes (`SEEDBYTES`) |
| τ | 30 | 59 | 115 | weight of challenge c (all +1 coeffs) |
| p | 0.15 | 0.3125 | 0.20 | ternary-secret parameter, Pr[s_i=±1]=p |
| γ | 34.93 | 62.02 | 72.73 | sk-rejection parameter (`GAMMA`) |
| ⌊log2\|C\|⌋ | 161 | 257 | 513 | challenge-space entropy |
| B_sc | 191.30 | 476.40 | 780.01 | bound on ‖sc‖ |
| d | 10 | 9 | 10 | compression parameter (`D`) |
| B0 | 5858.47 | 18429.83 | 73426.13 | inner rejection radius |
| B′ | 6046.76 | 18900.22 | 74202.04 | outer rejection radius (`B1`) |
| B | 6049.79 | 18906.23 | 74206.14 | y-sampling radius (`B`) |
| B″ | 7500.00 | 23000.00 | 80000.00 | verify radius (`B11`) |
| B_SIS | 10435.93 | 31240.06 | 91607.91 | SIS radius used in security analysis |
| M | 4.83 | 5.03 | 3.24 | expected signing repetitions |
| claimed security | 128 | 256 | 512 | bits (spec Table 3: classical core-SVP 137/261/535 SIS, 153/256/524 with dim-for-free) |

Sizes (bytes), specification (Table 2, p. 13) vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | sig spec | sig impl | match |
|---|---|---|---|---|---|---|---|
| DARTS128 | 1120 | 1120 | 1536 | 1536 | 1449 | 1449 | yes |
| DARTS256 | 2208 | 2208 | 2880 | 2880 | 2489 | 2489 | yes |
| DARTS512 | 4672 | 4672 | 6016 | 6016 | 5851 | 5851 | yes |

## Pseudocode

### KeyGen (spec Algorithm 6, p. 8)
```
 1  seed        <- {0,1}^rho(lambda)
 2  (seedA, seeds, K) <- Hgen(seed)
 3  A1          <- expandA(seedA)              in R_q^{k x (l-1)}      [Alg. 1]
 5  (s0,s1,e1)  <- expandS(seeds, counter_s)   ternary T_p             [Alg. 2,3]
 6  if s0 not invertible in R_q or N(s) > gamma^2 * n: goto 5
      // N(s) = tau * sum_{i=1..m} i-th max_j ||s(w_j)||_2^2
      //        + r * (m+1)-th max_j ||s(w_j)||_2^2 ,  m=floor(n/tau), r=n mod tau
      // guarantees ||c*s||_2 <= gamma*sqrt(tau) for every H2 output c
 7  A0 <- -s0^{-1} * ( A1*s1 + e1 - ((q+1)/2)*alpha )^T   in R_q^k
 8  A  <- (A0, A1, I_k)        // so that  A*s == ((q+1)/2)*alpha (mod q)   (Eq. 1)
 9  s  <- (s0, s1, e1)
10  return pk = (A0, seedA),  sk = (pk, s, K)
```

### Sign (spec Algorithm 7, p. 9)
```
 1  reconstruct A = (A0, A1, I_k) from pk
 2  mu      <- H1(pk, M)
 3  seedbuf <- H1(K, mu)                    // deterministic-ish per (key,msg)
 4  counter_y <- 0
 5  (y, b, b') <- expandY(seedbuf, counter_y)   // y=(y1,y2) uniform on hyperball B
 6  counter_y++
 7  w  <- A * round(y)   (mod q)
 8  c  <- H2( Comp_d(w), mu )               // exactly tau coefficients = 1
 9  if Comp_d(w) != Comp_d( w + (1-b)*c*alpha ):  goto 5     // hint-safety abort
11  z = (z1, z2) <- y + (-1)^b * s*c        // bimodal branch
12  if ||z||_2 > B' or ( ||2z - y||_2 < B and ||z||_2 > B0 and b' = 0 ): goto 5
14  h  <- Comp_d(w) - Comp_d( w - round(z2) + (1-b)*c*alpha )  (mod 2^d)
15  x  <- rANS.Encode( Comp_d(round(z1)) )                      [Alg. 4]
16  ez <- round(z1) - Decomp_d( Comp_d(round(z1)) )             // "low bits"
17  return sigma = ( c, ez, x, rANS.Encode(h) )
```

### Verify (spec Algorithm 8, p. 9)
```
 1  z1~ <- Decomp_d( rANS.Decode(x) ) + ez
 2  h~  <- rANS.Decode( Encode(h) )
 3  A1  <- expandA(seedA);   Abar <- (A0, A1) mod q
 5  w1~ <- h~ + Comp_d( Abar*z1~ + ((q+1)/2)*c*alpha )  (mod q)
 6  mu~ <- H1(pk, M)
 7  if c != H2(w1~, mu~): return 0
10  z2~ <- Decomp_d(w1~) - ( Abar*z1~ + ((q+1)/2)*c*alpha )  (mod q)
12  return [ ||(z1~, z2~)||_2 < B'' ]        // B'' = B' + sqrt(n(k+l))/2 + sqrt(nk)*ceil(q/2^{d+1})  (Thm. 1)
```

### Hashes / compression
```
Hgen, H1, H2 = SM3-XOF (absorb-once / absorb-twice, squeeze)   [symmetric.c]
  absorb  : derived_seed = pseudohash(512, input)              [symmetric.c:10,28,83]
  squeeze : pseudoXOF(derived_seed || 4-byte block counter)    [symmetric.c:47,66]
  mu      = XOF(pk || M)              , CRHBYTES = 64
  seedbuf = XOF(K || mu)
  c       = poly_challenge( pack_compressed(w1) , mu )   -> weight-tau {0,1} poly
Comp_d(r)  = round( (2^d/q) * r ) mod 2^d ;  Decomp_d(r') = round( (q/2^d) * r' ) mod q
NTT: (log2 n - 2)-level incomplete NTT, x^n+1 = prod (x^4 - zeta^{2 br(i)+1}),
     base case multiplied with Karatsuba (16 -> 9 mults).
```

### Signature byte layout (implementation, `packing.c:145` / `packing.c:217`)
```
sig = c[N/8 bits, 1 bit per coeff]           (POLY_C_PACKEDBYTES = 64/64/128)
   || LowBits(z1)  = ez, L polys, "BAT" packing (POLY_LOWBITS_PACKEDBYTES * L)
   || len_hb_z1 - BASE_ENC_HB_Z1   (1 byte offset)
   || len_h     - BASE_ENC_H       (1 byte offset)
   || rANS Encode(HighBits(z1))    (len_hb_z1 bytes)
   || rANS Encode(h)               (len_h bytes)
   || zero padding to CRYPTO_SIGNATUREBYTES
```

## Implementation vs specification

**What was checked.** Built sources per `sign-08/Makefile` (`sign.c packing.c
polyvec.c poly.c ntt.c reduce.c sampler.c encoding.c polyfix.c polymat.c fft.c
fixpoint.c symmetric.c drng.c auxfunc.c`) in
`Implementations/Reference_Implementation/DARTS{128,256,512}` (three copies of
the same tree, parameter set fixed by `DARTS_MODE` in `config.h`). Mapped
Alg. 6 → `sign.c:crypto_sign_keypair`, Alg. 7 → `sign.c:crypto_sign_signature`,
Alg. 8 → `sign.c:crypto_sign_verify` (line ~285+), Alg. 1–3 → `polymat.c` /
`poly.c` / `sampler.c`, Alg. 4–5 → `encoding.c`, packing → `packing.c` +
`poly.c:626/746`. Parameters spot-checked in `params.h` for all three modes:
`N`, `Q`, `K`, `L`, `TAU`, `D`, `GAMMA`, `P`, `B/B0/B1/B11`.

**Agreements.**
- All nine spec sizes (Table 2) reproduce exactly from `params.h` formulas and
  match OBSERVED byte-for-byte: `pk = SEEDBYTES + K*POLY_Q_PACKEDBYTES`,
  `sk = pk + SEEDBYTES + (K+L)*POLY_S_PACKEDBYTES`,
  `sig = POLY_C_PACKEDBYTES + L*POLY_LOWBITS_PACKEDBYTES + 2 + ENCODE_MAXSIZE`.
- `n, q, (k,ℓ), τ, d, B″` agree with Table 2 for all three sets.
- Bimodal structure is present and correct: keygen actually inverts `s0` and
  forms `A0 = -s0^{-1}(A1 s1 + e1 - ((q+1)/2)α)`; the secret-rejection test
  `N(s) > γ²n` is implemented (`sign.c`, `squared_singular_value >= GAMMA*GAMMA*N`
  via the FFT in `fft.c`).
- Sign's three rejection predicates map 1:1 onto Alg. 7 step 12, with `b` bit 0
  carrying the bimodal sign and bit 1 carrying `b'`, matching `expandY`'s
  `(y, b, b')` output. The byte is derived from `seed ‖ ni` with the nonce `ni`
  that the next call uses for its first mask polynomial (`polyfix.c:256,288-299`,
  retry at `sign.c:169`), so after a rejection its bits are the sign bits of
  coefficients 0–7 of the next `y1[0]` (sign-08-5). The hint-safety abort (step 9)
  is present.
- Transcript binding matches between signer and verifier: both absorb
  `pk ‖ M` for `mu` (the signer passes `sk`, whose first `CRYPTO_PUBLICKEYBYTES`
  bytes are `pk`), then `c = poly_challenge(pack(w1), mu)`. The verifier
  compares the recomputed `c1` against the transmitted `c` coefficient-wise,
  so the transmitted weight-τ structure needs no separate check.
- `unpack_sig` **does** reject non-zero trailing padding (`packing.c:252-256`)
  and re-derives the two length fields with an explicit total-length bound —
  i.e. the *outer* signature framing is canonical, unlike the sibling findings
  recorded for sign-01 / sign-07. `crypto_sign_verify` also rejects
  `siglen != CRYPTO_SIGNATUREBYTES`. The rANS decoders validate initial state,
  symbol range, final state and consumed length (`encoding.c:159-190`,
  `encoding.c:260-288`).

**Discrepancy (a) — real deviation: non-canonical `ez` / LowBits encoding
(DARTS128 and DARTS256; SUF/malleability).**
`poly_pack_lowbits` (`poly.c:626`) packs each group of 8 coefficients as
24 (or 32) bits of `x2` plus a **base-17 digit string** `base17_val =
Σ x1[j]·17^(7-j)` written into a **33-bit** field. Valid values are
`< 17^8 = 6 975 757 441`, but the field holds `< 2^33 = 8 589 934 592`, leaving
`1 614 177 151` unused codepoints per block. `poly_unpack_lowbits`
(`poly.c:768` reads the 33 bits, `poly.c:784` peels digits with
`% 17` / `/= 17`) performs **no range check**, and the discarded final quotient
makes `v` and `v + 17^8` decode to *identical* `x1[0..7]`, hence identical
coefficients. Consequently, for any valid signature, any 57-bit (resp. 65-bit)
block whose `base17_val < 2^33 - 17^8` can be replaced by
`base17_val + 17^8`, producing a **different byte string that decodes to the
same `z1`**, reproduces the same `w1~` and `c`, and is accepted by
`crypto_sign_verify`. A signature contains `L·n/8` = 128 (128-bit set), 192
(256-bit set) such blocks and the condition holds for roughly a quarter of blocks,
so essentially every signature is malleable. This breaks the SUF-CMA property
that spec §3.3.3 explicitly adds. DARTS512 is **not** affected: its LowBits
packing is a plain `r[i] = coeff + 128` byte map (`poly.c:726-731`), a
bijection. Recorded as sign-08-6; `sign-08/security_findings.md` lists
`signature-encoding-malleability` as `not_found`, but that harness only tried
random bit mutations, truncation and extension, which cannot hit this
structured transformation. There is no `sign-08/review-o48.md`.

**Discrepancy (b) — numeric mismatches between Table 2 and `params.h`
(cosmetic-to-minor).** DARTS256: `GAMMA 62.08` vs spec `62.02`;
`B0 18429.40` vs spec `18429.83`; `B 18906.24` vs spec `18906.23`.
DARTS512: `B0 73426.15` vs spec `73426.13`. The `B0` gap at the 256-bit set
(0.43) is larger than rounding and slightly changes the rejection region and
therefore the output distribution relative to the analysed `M_reject`; the
others are last-digit rounding. Classified as a spec/implementation
transcription inconsistency, not a security break by itself.

**Not verified (time-boxed).** `expandY`'s hyperball sampler
(`sampler.c`/`polyfix.c`/`fixpoint.c`) was not checked against the
uniformity/precision requirements of refs [21],[8]; the rANS frequency tables
in `precomputations_rANS` were not checked against the claimed symbol
distributions; the incomplete-NTT constants (`ROOT_OF_UNITY 667 / 1092`) and
the `N(s)` FFT evaluation were not independently recomputed. No estimator
re-run for Table 3.
