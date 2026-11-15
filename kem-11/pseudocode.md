# kem-11 COMPASS-KEM — algorithm summary

Module-lattice KEM based on **Module Learning With Rounding** (MLWR) plus a
non-standard "biased MLWR" variant the authors introduce to repair the SABER
proof. An IND-CPA PKE (COMPASS-PKE) with *no explicit error polynomials* —
all noise comes from the rounding/compression operators Comp_d — is turned into
an IND-CCA KEM by the FO transform with implicit rejection. The code is an
ML-KEM (Kyber) reference tree with the error additions commented out.

Specification: `kem-11-spec.pdf` (31 pages), §2.4 (PKE, Algorithms 1–3) and
§2.5 (KEM, Algorithms 4–6); parameters in §3.2 Table 1, sizes Table 2, DFR
Table 3.

## Parameters

Spec Table 1 (printed p. 15, PDF p. 18). p = (q-1)/2^d, eta_s = eta_2.

| parameter | Level 1 (128) | Level 2 (256) | Level 3 (384) | Level 4 (512) | meaning |
|---|---|---|---|---|---|
| n | 256 | 256 | 512 | 512 | ring degree, R_q = Z_q[X]/(X^n+1) |
| q | 3329 | 3329 | 7681 | 7681 | modulus |
| p | 832 | 832 | 1920 | 1920 | rounded modulus (q-1)/2^d |
| k x l | 2x2 | 4x4 | 3x3 | 4x4 | module rank |
| eta_s = eta_2 | 3 | 2 | 3 | 4 | CBD parameter for s and r |
| (d, d1, d2) | (2,2,8) | (2,2,6) | (2,2,8) | (2,2,6) | compression bits for t, c1, c2 |
| DFR (Table 3) | 2^-189.27 | 2^-181.13 | 2^-404.51 | 2^-250.31 | decryption failure rate |
| claimed security | 128 | 256 | 384 | 512 | bits (spec's "Required Security") |

Sizes (bytes), spec Table 2 vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | ct spec | ct impl | ss spec | ss impl | match |
|---|---|---|---|---|---|---|---|---|---|
| COMPASS-KEM-128 | 672 | 672 | 1504 | 1504 | 768 | 768 | 32 (n/8) | 32 | yes |
| COMPASS-KEM-256 | 1312 | 1312 | 2912 | 2912 | 1472 | 1472 | 32 (n/8) | 32 | yes |
| COMPASS-KEM-384 | 2144 | 2144 | 4704 | 4704 | 2432 | 2432 | **64 (n/8)** | **32** | **no** |
| COMPASS-KEM-512 | 2848 | 2848 | 6240 | 6240 | 3264 | 3264 | **64 (n/8)** | **32** | **no** |

pk/sk/ct match to the byte. The spec gives no explicit shared-secret column;
n/8 comes from §2.3.1 ("H and J ... produce one n-bit (n/8-byte) output") and
Algorithm 6's `Output: K in B^{n/8}`.

## Pseudocode

### COMPASS-PKE.KeyGen (spec Algorithm 1)
```
1. iota <-$ B^{n/8}
2. (rho, sigma) <- G(iota)
3. Ahat <- ExpandA(rho)  in R_q^{k x l}      // sampled directly in NTT domain
4. s    <- Sample(beta_{eta_s}, sigma) in R^l
5. shat <- NTT(s)
6. t'   <- INTT(Ahat (*) shat)               // no error term: LWR
7. t    <- Comp_d(t')
8. pk = (rho, t);   sk = (iota, shat)
```
### COMPASS-PKE.Encrypt(pk, m, r) (spec Algorithm 2)
```
1. Ahat <- ExpandA(rho)
2. t'   <- Decomp_d(t)
3. r    <- Sample(beta_{eta_2}, r)  in R^k
4. rhat <- NTT(r)
5. c1'  <- INTT(Ahat^T (*) rhat)
6. c2'  <- INTT(NTT(t')^T (*) rhat)
7. c2'  <- c2' + m * floor(q/2)   (mod q)    // one message bit per coefficient
8. c1 <- Comp_{d1}(c1');  c2 <- Comp_{d2}(c2');  ct = (c1, c2)
```
### COMPASS-PKE.Decrypt(sk, ct) (spec Algorithm 3)
```
1. c1' <- Decomp_{d1}(c1);  c2' <- Decomp_{d2}(c2)
2. u <- c2' - INTT(NTT(c1')^T (*) shat)  (mod q)
3. for i in 0..n-1: m[i] <- 1 if floor(q/4) <= u[i] < floor(3q/4) else 0
```
There is no decoder and no failure symbol: decryption always returns an n-bit
string; wrong plaintexts are caught only by the FO re-encryption check.

### COMPASS-KEM.KeyGen (spec Algorithm 4)
```
1. (ek_PKE, dk_PKE) <- PKE.KeyGen()
2. z <-$ B^{n/8}
3. ek = ek_PKE;  dk = dk_PKE || ek || H(ek) || z
```
### COMPASS-KEM.Encaps (spec Algorithm 5)
```
1. m <-$ B^{n/8}
2. (K, r) <- G(m || H(ek))
3. c <- PKE.Encrypt(ek, m, r)
4. return (K, c)
```
### COMPASS-KEM.Decaps (spec Algorithm 6)
```
1. parse dk = (dk_PKE, ek_PKE, h, z)
2. m'      <- PKE.Decrypt(dk_PKE, c)
3. (K',r') <- G(m' || h)
4. Kbar    <- J(z || c)
5. c'      <- PKE.Encrypt(ek_PKE, m', r')
6. if c != c': K' <- Kbar        // implicit rejection; flag must be destroyed
7. return K'
```
Hashes (spec §2.3.1): H, J : B* -> B^{n/8}; G : B* -> B^{n/8} x B^{n/8}.
Implementation: `H = sm3hash(256)`, `G = pseudohash(512)` (32+32 bytes),
`J = rkprf` (`symmetric.h:39-55`); ExpandA, Sample and `rkprf` all call the
ICCS `pseudoXOF` (`symmetric-shake.c:30,81,112`), not SHAKE.

## Implementation vs specification

Checked: `src/COMPASS-KEM-{128,256,384,512}/params.h` for constants, and
`kem.c`, `indcpa.c`, `poly.c`, `symmetric.h` of the 128 and 384 instances for
the algorithm flow.

Agreements:
- n, q, k, l, eta_s, eta_2, d, d1, d2 match Table 1 for all four instances
  (all nine constants checked per instance in `params.h`).
- p is implicit but consistent: `POLYCOMPRESSEDBYTES_D1` = 320 = 256*10/8 for
  p = 832 (10 bits) and 704 = 512*11/8 for p = 1920 (11 bits).
- Every pk/sk/ct size matches Table 2 exactly (formulas in `params.h`).
- The FO transform is complete and standard: `crypto_kem_dec` (`kem.c:142`)
  decrypts, re-derives `(K', r') = G(m' || h)` with the stored `h = H(ek)`,
  re-encrypts, compares with `verify()`, computes `rkprf(z, ct)` and selects
  with `cmov` — constant-time, no branch on the reject flag, as §2.5.3 demands.
- The message encoding uses `(q-1)/2`, which equals the spec's `floor(q/2)`
  for both 3329 and 7681; the decode window `[(q-1)/4, (q-1)/4 + (q-1)/2)` is
  the spec's `[floor(q/4), floor(3q/4))`.
- Error polynomials are genuinely absent (`indcpa.c:265-266, 338-340, 353-354`
  are commented out), i.e. the scheme really is LWR and not LWE, as Algorithms
  1–2 specify.

Discrepancies:
- **(a) real deviation — the 512-bit-degree instances encapsulate only 256
  bits.** `COMPASS_KEM_SYMBYTES`/`COMPASS_KEM_SSBYTES` are hard-coded to 32
  (`params.h`, near the end) for all four levels, and
  `COMPASS_KEM_INDCPA_MSGBYTES = SYMBYTES`. In `poly_frommsg`
  (`src/COMPASS-KEM-384/poly.c:332-352`) only the first 256 coefficients carry
  message bits and coefficients 256..511 are explicitly zeroed ("MLWR
  heterogeneous protection"); `poly_tomsg` (`poly.c:381`) likewise decodes only
  256 bits. The spec fixes m, K, H, J, and each G output at n/8 bytes, i.e. 64
  bytes when n = 512. For Levels 3 and 4 the implementation therefore uses a
  32-byte plaintext and a 32-byte shared secret against a claimed 384- and
  512-bit security target. The observed `ss=32` for all four instances
  confirms this at the ABI. Whether the spec's own 384/512-bit claim is
  meaningful with a 32-byte key is a design question the spec does not
  address, but the code does not implement what §2.3.1/Algorithms 5–6 say.
- **(b)/(a) sk does not contain iota.** Algorithm 1 step 9 sets
  `sk = (iota, shat)` and Algorithm 3 parses `sk = (z, shat)`, but
  `COMPASS_KEM_INDCPA_SECRETKEYBYTES = POLYVECBYTES` only
  (`params.h`), and `pack_sk` (`indcpa.c:285`) stores just `shat`. The seed is
  never kept. Harmless for decryption (shat alone suffices) and the sizes in
  Table 2 already assume this, so the spec text is the inconsistent side.
- **(c) equivalent/extra domain separation.** `indcpa_keypair_derand`
  (`indcpa.c:257-259`) computes `(rho, sigma) = G(coins || k)` — a
  module-rank byte is appended before hashing, as in ML-KEM. Algorithm 1 step
  2 says plain `G(iota)`. Output-neutral, but it means an independent
  implementation written from the spec would not reproduce the KATs.
- **(c)** `pack_pk`/`unpack_pk` apply Comp_d / Decomp_d inside the
  serialization routines rather than as separate steps, and the public key is
  kept in the NTT domain after `polyvec_ntt(&pkpv)` in `indcpa_enc`; both are
  the spec's own computation reordered.

Randomness: `crypto_kem_keypair` and `crypto_kem_enc` (`kem.c:55, 120`) draw
their coins from `get_random_number(&drng_algorithm, ...)`, i.e. the NGCC
seeded DRNG. The shipped `randombytes.c` (/dev/urandom) is not linked.

Not verified: the NTT constants and the `gen_a`/`gen_at` rejection sampler were
not checked against §2.3.2 (2.18)–(2.22), and the DFR values of Table 3 were
not recomputed.
