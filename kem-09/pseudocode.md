# kem-09 CheetahKEM — algorithm summary

CheetahKEM is a lattice KEM resting on RLWE (Cheetah128, k=1) and MLWE
(Cheetah256/384/512, k=2/3/4) over `R_q = Z_q[X]/(X^N+1)`. A CPA-secure
"Cheetah PKE" (Kyber-like, but with an NTRU-style `mod 2` decode and a
`Truncate` step that ships only λ of the N coefficients of v) is lifted to an
IND-CCA2 KEM by the Fujisaki–Okamoto transform with implicit rejection.

Specification: `kem-09-spec.pdf` (28 pages), §2.2 (PKE, Alg. 15–17), §2.3 (KEM,
Alg. 18–20), §2.4 (Table 2). The canonical spec is **English**; a Chinese
translation ships separately as `中文资料/cheetah_KEM_cn.pdf` and was not needed.

## Parameters

Table 2, §2.4. `d_q = ceil(log2 q) = 13`; `Δ = (q+1)/2 = 3841`.

| parameter | Cheetah128 | Cheetah256 | Cheetah384 | Cheetah512 | meaning |
|---|---|---|---|---|---|
| N | 640 | 640 | 640 | 640 | ring degree, `X^N+1` |
| k | 1 | 2 | 3 | 4 | module rank (k=1 ⇒ RLWE) |
| q | 7681 | 7681 | 7681 | 7681 | NTT-friendly prime |
| λ | 128 | 256 | 384 | 512 | message / shared-secret bits |
| η | 5 | 4 | 3 | 2 | centred binomial ψ_η |
| d_b | 10 | 10 | 11 | 11 | pk compression |
| d_u | 10 | 10 | 11 | 11 | ct (u) compression |
| d_v | 4 | 4 | 4 | 8 | ct (v) compression |
| δ (DFR) | 2^-129 | 2^-176 | 2^-189 | 2^-243 | decapsulation failure |
| claimed security | 128 | 256 | 384 | 512 | bits, classical (spec's claim) |

Sizes (bytes). Spec column = §2.3 formulae `eksize = λ/8 + kN·d_b/8 + 16`,
`dksize = kN·d_q/8 + eksize + |H(ek)| + λ/8 + 16` (with |H| = 32, SM3),
`|c| = (kN·d_u + λ·d_v)/8`, `|K| = λ/8`; impl column = OBSERVED library values.

| instance | pk spec | pk impl | sk spec | sk impl | ct spec | ct impl | ss | match |
|---|---|---|---|---|---|---|---|---|
| Cheetah128 | 832 | 832 | 1936 | 1936 | 864 | 864 | 16 | yes |
| Cheetah256 | 1648 | 1648 | 3808 | 3808 | 1728 | 1728 | 32 | yes |
| Cheetah384 | 2704 | 2704 | 5920 | 5920 | 2832 | 2832 | 48 | yes |
| Cheetah512 | 3600 | 3600 | 7872 | 7872 | 4032 | 4032 | 64 | yes |

## Pseudocode

### KEM.KeyGen — Algorithm 18 (calls PKE.KeyGen, Algorithm 15)
```
PKE.KeyGen():                                        # Alg 15
  seed <- RNG();  (rho, sigma, gamma) <- XOF(seed)    # each in B^{lambda/8+16}
  Ahat <- SamplePolyMat(XOF(rho), k, k)               # already in NTT domain
  s    <- SampleNoisePolyVec(XOF(sigma), k)           # psi_eta
  e    <- SampleNoisePolyVec(XOF(gamma), k)           # psi_eta
  shat <- NTT_640(s, FORWARD)
  b    <- Compress(Ahat o shat + e, 2^{d_q - d_b})
  ek_PKE <- rho || EncodePolyVec(b, k, d_b)
  dk_PKE <- EncodePolyVec(shat, k, d_q)               # secret kept in NTT domain

KEM.KeyGen():                                        # Alg 18
  salt <- B^{lambda/8+16};  (ek_PKE, dk_PKE) <- PKE.KeyGen()
  return ek = ek_PKE,  dk = dk_PKE || ek || H(ek) || salt
```

### KEM.Encaps — Algorithm 19 (calls PKE.Encrypt, Algorithm 16)
```
KEM.Encaps(ek):                                      # Alg 19
  m       <- B^{lambda/8}
  (K, gamma) <- G(m || H(ek))                        # gamma = (g1,g2,g3)
  c       <- PKE.Encrypt(ek, m, gamma)
  return (K, c)

PKE.Encrypt(ek, m, gamma):                           # Alg 16
  rho <- ek[0 : lambda/8+16-1];  Ahat <- SamplePolyMat(XOF(rho), k, k)
  b   <- Decompress(DecodePolyVec(ek[lambda/8+16 : ...], k, d_b), 2^{d_q-d_b})
  r   <- SampleNoisePolyVec(XOF(g1), k)
  e1  <- SampleNoisePolyVec(XOF(g2), k)
  e2  <- SampleNoisePolyVec(XOF(g3), 1)
  rhat <- NTT_640(r, FORWARD)
  u   <- Compress(rhat o Ahat + e1, 2^{d_q - d_u})
  vt  <- Truncate(rhat o NTT(b) + e2, lambda) + Delta * m    # keep lambda coeffs
  v   <- Compress(vt, 2^{d_q - d_v})
  return c = ( EncodePolyVec(u,k,d_u) , IntegersToBytes(v,lambda,d_v) )
```

### KEM.Decaps — Algorithm 20 (calls PKE.Decrypt, Algorithm 17)
```
PKE.Decrypt(dk_PKE, c1, c2):                         # Alg 17
  u' <- Decompress(DecodePolyVec(c1,k,d_u), 2^{d_q-d_u})
  v' <- Decompress(BytesToIntegers(c2,lambda,d_v), 2^{d_q-d_v})
  shat <- DecodePolyVec(dk_PKE, k, d_q)
  m' <- ( 2 * ( v' - Truncate(NTT(u') o shat, lambda) )  mod^{+-} q )  mod 2
  return IntegersToBytes(m', lambda, 1)

KEM.Decaps(dk, c):                                   # Alg 20
  (dk_PKE, ek_PKE, h, salt) <- dk
  m'          <- PKE.Decrypt(dk_PKE, c)
  (K', gamma')<- G(m' || h)
  Kbar        <- H(salt || c)                        # implicit-rejection key
  c'          <- PKE.Encrypt(ek_PKE, m', gamma')     # re-encryption
  if c != c' then K' <- Kbar
  return K'
```

Symmetric primitives (§2.1, "Auxiliary Cryptographic Functions"): the spec only
names PRF, H, G and an XOF without fixing them. The implementation instantiates
H = SM3-256 (`sm3hash`) and G = XOF = `pseudoXOF`, a KDF-SM3 counter-mode
expansion (`auxfunc.c:482`), for **every** parameter set.

## Implementation vs specification

Checked: `src/Cheetah128..512/{params.h,KEM_Cheetah.c,auxfunc.c}` (the files the
Makefile builds). Line numbers are Cheetah128; the other three directories are
structurally identical (Cheetah256 at the same offsets, Cheetah384/512 shifted
by up to three lines).

Agreements:
- `kem_keygen`/`kem_enc`/`kem_dec` (`KEM_Cheetah.c:208`, `:227`, `:252`) follow
  the structure of Alg. 18/19/20 (deviations below), including `dk = shat || ek || SM3(ek) || salt`
  and `(K,gamma) <- G(m || H(ek))` at `:239`.
- Parameter spot-check (sampled N, k, q, η, d_b/d_u/d_v and the λ-derived
  lengths, in all four `params.h`): `N 640`, `Q 7681`, `DELTA 3841`, `QBITS 13`
  everywhere; `CHEETAH_K` 1/2/3/4 and `ETA` 5/4/3/2 match Table 2 row-for-row;
  `DB/DU/DV` = 10/10/4, 10/10/4, 11/11/4, 11/11/8 match Table 2; `SEED_BYTES`
  = λ/8+16 (32/48/64/80) matches the spec's `B^{λ/8+16}`.
- All four pk/sk/ct/ss sizes match the §2.3 formulae exactly — **no size
  mismatches found**.

Discrepancies:
- **(a) real deviation — broken implicit-rejection mask.** `KEM_Cheetah.c:285-293`
  ORs all ciphertext byte differences into `unsigned char fail`, then uses
  `fail = (unsigned char)(-fail)` (`:290`) directly as the select mask in
  `ss[i] ^= fail & (kbar[i]^ss[i])` (`:292`). Two's-complement negation maps
  only `fail==1` to `0xff`; e.g. `fail==0x80` gives mask `0x80`, so seven bits
  of the *candidate* key K' survive into the returned secret. This is the
  already-recorded finding LH-KEM-09-001..004 in `security_findings.md`
  (confirmed IND-CCA distinguisher, all four instances).
- **FO ordering (the kem-01 bug) is NOT present here.** The candidate key is
  written to the output buffer at `:279-281`, but the selection happens *after*
  the re-encryption compare loop (`:285-288`), via the branchless cmov at
  `:291-293` writing into the output `ss`. So the rejection branch is reached
  and is branchless; only the mask value is wrong, not the control flow.
- **(a) real deviation — fallback not bound to the received ciphertext.**
  Alg. 20 line 4 specifies `Kbar <- H(salt || c)`. The implementation builds
  `salt_ct = salt || c'` from the *re-encrypted* `c'` (`:277`, `:283`) — the
  received `ct` is never hashed. Harmless when `c == c'`, but on rejection the
  fallback is derived from `c'` rather than `c`, which is not the proved
  transform. Filed as kem-09-4 in `report.md`.
- **(a/c) `Kbar` uses G, not H.** `:283` calls `pseudoXOF` (the XOF/G) rather
  than `sm3hash` (H), with no domain separator distinguishing it from the
  `(K,gamma)` call at `:271`. Both are SM3-based, but the spec's H/G
  separation in Alg. 20 is not honoured.
- **(a) real deviation — KeyGen seed expansion.** Alg. 15 lines 1–2 draw one
  `seed` and set `(rho, sigma, gamma) <- XOF(seed)`; `indcpa_keypair` draws all
  `3*SEED_BYTES` directly from the RNG (`:65`).
- **(b) symmetric-strength gap.** `HASH_BYTES` is 32 in all four `params.h`, so
  `h = H(ek)` is 256-bit even for Cheetah384/512, whose claimed classical
  levels are 384/512 bits. The spec writes `|H(ek)|` abstractly, so this is an
  ambiguity the implementation resolved downward.
- **(b) spec inconsistency.** Alg. 19 takes `m` as an *input* yet re-samples
  `m <- B^{λ/8}` in its line 1. The implementation samples `m` internally
  (`:237`), matching line 1.

Not verified here: the NTT/`Compress`/`Truncate`/`SamplePolyMat` internals in
`poly.c`/`ntt.c`/`mod.c` were not traced coefficient-by-coefficient against
Alg. 1–14, and no DFR measurement or independent lattice estimate was made.
