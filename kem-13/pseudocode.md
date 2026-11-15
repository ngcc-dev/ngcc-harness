# kem-13 DKEM (Ding Key Encapsulation) — algorithm summary

Module-lattice KEM over **MLWE**. Instead of the LPR encrypt-a-message approach
of ML-KEM, the CPA core (DKE) is a *reconciliation* key exchange in the Ding
family: both parties compute an approximately equal ring element, the responder
publishes an l-bit-per-coefficient **signal** w = Sig(k_B, b), and both extract
one bit per coefficient with Rec(k, w) = (k - wL mod q) mod 2. The CCA layer is
a **tag-based FO transform**: the encapsulator masks its own coins with the DKE
shared secret and ships the mask as a ciphertext tag; decapsulation unmasks,
re-encapsulates deterministically and compares, with implicit rejection.
Arithmetic (NTT, CBD sampling, q = 3329, n = 256) is inherited from ML-KEM for
DKEM-128/256; DKEM-512 uses q = 7681, n = 512.

Specification: `kem-13-spec.pdf` (64 pages), §2.3–2.4 (Algorithms 10–16),
signal/reconciliation §2.2.1 (Algorithms 2–3), parameters §2.5 Table 1, sizes
§4 Table 2.

## Parameters

Spec Table 1 (p. 21).

| parameter | DKEM-128 | DKEM-256 | DKEM-512 | meaning |
|---|---|---|---|---|
| n | 256 | 256 | 512 | ring degree |
| q | 3329 | 3329 | 7681 | modulus |
| k | 2 | 4 | 4 | module rank |
| eta | 3 | 2 | 3 | CBD parameter for secrets and errors |
| l | 4 | 5 | 4 | signal bits per coefficient (2^l intervals) |
| d | 10 | 11 | 11 | compression bits d_B for u_B |
| failure prob. | 2^-132.7 | 2^-181.2 | 2^-167.0 | reconciliation failure rate |
| NGCC level | 1 | 2 | 3 | |
| NIST equivalent | ML-KEM-512 | ML-KEM-1024 | — | |

Sizes (bytes), spec Table 2 (p. 30) vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | ct spec | ct impl | ss spec | ss impl | match |
|---|---|---|---|---|---|---|---|---|---|
| DKEM-128 | 800 | 800 | 1600 | 1600 | 800 | 800 | 32 | 32 | yes |
| DKEM-256 | 1568 | 1568 | 3136 | 3136 | 1600 | 1600 | 32 | 32 | yes |
| DKEM-512 | 3392 | 3392 | 6784 | 6784 | 3136 | 3136 | 64 | 64 | yes |

Every size matches, and each is reproduced by the spec's own formulas:
pk = k·n·⌈log2 q⌉/8 + n/8, sk = k·n·⌈log2 q⌉/8 + pk + n/8,
ct = k·n·d/8 + n·l/8 + n/8.

## Pseudocode

### Sig(f, b) (spec Algorithm 2) and Rec(f, w) (Algorithm 3)
```
Sig:  for i in 0..n-1:  w[i] = the unique w in {0..2^l-1} with f[i]-b[i] mod q in I_w
Rec:  for i in 0..n-1: ss[i] = (f[i] - w[i]*L mod q) mod 2
      I_0 = (-L/2 .. L/2],  I_w = I_0 + wL,  L = (q-1)/2^l
```
b is a fresh uniform bit per coefficient; Lemma 2.1 shows the extracted bit is
unbiased even given w.

### DKE.Initiate(coins) (spec Algorithm 10)
```
1. (r, rho) <- XOF(coins; 2n)
2. Abar <- GenMatrix(rho)                      // k x k in NTT domain
3. (sA, nonce) <- SampleSecret_eta(r, 0)
4. eA          <- SampleSecret_eta(r, nonce)
5-6. sAbar = NTT(sA);  eAbar = NTT(eA)
7. uAbar = Abar o sAbar + eAbar
8. pk = ByteEncode(uAbar) || rho
9. sk = ByteEncode(sAbar)
```
### DKE.Response(pk, coins) (spec Algorithm 11)
```
 1. (uAbar, rho) <- pk;   2. (r, b) <- coins
 3. AbarT <- GenMatrix(rho, 1)
 4-5. sB, nonce <- SampleSecret_eta(r,0);  eB <- SampleSecret_eta(r,nonce)
 6-9. sBbar = NTT(sB); uB = NTT^-1(AbarT o sBbar) + eB
10. uB' = Compress(uB, dB)
11-12. kB = NTT^-1(uAbar o sBbar)
13-15. r <- XOF(r||nonce; 2*eta*n); eB <- SampleBinomialPoly(r); kB = kB + eB
16. kB = 2 * kB                               // randomized doubling
17. w  = Sig(kB, b)
18. c  = (uB' || w)
19. ss = Rec(kB, w)
```
### DKE.DeriveSecret(c, sk) (spec Algorithm 12)
```
1-2. (uB', w) <- c;  uB'' = Decompress(uB', dB)
3-5. kA = 2 * NTT^-1(NTT(uB'') o sAbar)
6.   ss = Rec(kA, w)
```
There is no decoder and no failure symbol at the DKE layer: Rec always returns
n bits. Correctness holds iff every coefficient of kA - kB is even and within
the tolerance of Lemma 3.1; otherwise the derived ss differs and the CCA layer's
re-encapsulation check fires (failure probabilities in Table 1 above).

### DKEM.KeyGen / Internal / Encaps / Decaps (spec Algorithms 13–16)
```
KeyGen:   coins <-$ {0,1}^n;  (pk, sk) <- DKE.Initiate(coins)
          rej <-$ {0,1}^n;  dk = (sk || pk || rej);  ek = pk

Internal(ek, coins):
  1. rho <- last n bits of ek
  2. r   <- XOF(coins || rho; 2n)
  3. (c1, ss) <- DKE.Response(ek, r)
  4. c2 = coins XOR ss                         // tag: OTP-mask the coins
  5. ct = (c1 || c2)
  6. K  = KDF(ss; n)                           // KDF of the RAW ss, before the OTP
Encaps(pk): coins <-$ {0,1}^n; return DKEM.Internal(ek, coins)

Decaps(dk, ct):
  1-2. (sk, ek, rej) <- dk;  (c1, c2) <- ct
  3.   ss    <- DKE.DeriveSecret(c1, sk)
  4.   coins  = c2 XOR ss
  5.   (ct', K) <- DKEM.Internal(ek, coins)
  6.   Kbar   = KDF(rej || c2; n)
  7-9. if ct != ct': return Kbar   else: return K      // implicit rejection
```
Hashes (implementation, `dke_hash.c` with `DKE_HASH=0`): XOF = `pseudoXOF`,
KDF = `sm3hash(256)` when |ss| = 32 and `pseudohash(512)` when |ss| = 64;
GenMatrix uses SHAKE128-style rejection sampling through the same abstraction.

## Implementation vs specification

Checked: `src/DKEM-{128,256,512}/parameters.h` for constants and
`dkecca.c`, `dkecpa.c`, `dke_utils.c`, `packing.c`, `random_sampling.c`,
`KEM_AlgorithmInstance.c` for the flow (the three instance directories ship
identical sources selected by `-DDKE_MODE=<n>`).

Agreements:
- n, q, k, l (`DKE_L`), d (`DKE_DB`) match Table 1 for all three modes
  (`parameters.h:75-128`), and the effective CBD parameter `DKE_CBD_ETA`
  (`parameters.h:165-171`) is 2 for DKEM-256 and 3 otherwise, exactly the
  spec's eta column.
- The size macros reproduce Table 2 exactly:
  `DKE_PKBYTES = DKE_PACOMPRESSEDBYTES + DKE_SEEDBYTES`,
  `DKE_CTBYTES = DKE_K*DKE_N*DKE_DB/8 + DKE_L*DKE_N/8 + DKE_SSBYTES`,
  `DKE_SKBYTES = DKE_CPA_SKABYTES + DKE_PKBYTES + DKE_SSBYTES`.
- The tag-based FO is exactly Algorithms 14/16, including the "KDF before the
  one-time pad" ordering (`dkecca.c`, `DKE_CCA_enc_derand`: K = KDF(cpa_ss)
  is computed *before* `cpa_ss ^= coins`), the rejection key
  `Kbar = KDF(z || tag)`, a full-ciphertext `DKE_verify`, and a branchless
  `DKE_cmov` select.
- Randomness: `KEM_AlgorithmInstance.c` draws keygen coins
  (SEEDBYTES + SSBYTES) and encapsulation coins (SEEDBYTES) from
  `get_random_number(&drng_algorithm, ...)`, i.e. the seeded NGCC DRNG.
  `randombytes.c` / `randombytes_sys.c` (getrandom(2)) are compiled out by
  `-DDKE_RANDOM=0` and not linked.
- `DKE_poly_scale2` implements the randomized doubling of Algorithm 11 line 16,
  and `DKE_derive_ss` = apply_signal (subtract wL, centre) then mod 2, i.e.
  Algorithm 3.
- Sig follows Algorithm 2 line 3: `DKE_rand_to_poly` (`dke_utils.c:72-81`)
  encodes each bit of b as 0 or -1, so `DKE_signal` (`dke_utils.c:97-105`)
  takes the signal of `f[i] - b[i]`.

Discrepancies:
- **(b) misleading dead constants.** `DKE_NOISE_A` and `DKE_NOISE_B`
  (`parameters.h:81-82, 100-101, 119-120`) are **3 in all three modes**, which
  contradicts Table 1's eta = 2 for DKEM-256. They are referenced nowhere in
  any `.c` file — the sampler uses `DKE_CBD_ETA`, which is correct — so this
  is vestigial text, not a behavioural deviation. It is worth removing, since
  a reader auditing eta against the spec will land on the wrong macro.
- **(c) seed order in Initiate.** Algorithm 10 line 1 writes
  `(r, rho) <- XOF(coins; 2n)`. `DKE_CPA_keygen_derand` (`dkecpa.c:26-28, 36`)
  takes rho (the matrix seed) from the first half of the XOF output and r from
  the second, so an implementation written literally from Algorithm 10 would
  not reproduce the KATs.
- **(c)** Algorithm 11 lines 13–14 derive the extra error `eB` for `kB` from
  `XOF(r || nonce)`; the implementation calls `DKE_geterrorA(&e, coins,
  nonce++)` (`dkecpa.c`), which is the same `XOF(seed || nonce)` construction
  reached through the common nonce counter. Equivalent.
- **Build note (already in RESULTS.md):** `KEM_AlgorithmInstance.c:19` defines
  `DRNG_ctx drng_algorithm;` itself; the official KAT driver relies on
  `-fcommon` to merge the two tentative definitions, hence the harness's
  `-DNGCC_NO_DRNG`.

Not verified: the NTT constants (`DKE_NTT_ZETAS_LEN`, `DKE_MONT`, `DKE_QINV`,
`DKE_INVNTT_F`) against §2.2.2 Algorithms 4–7, the interval boundaries in
`DKE_getsignal`/`DKE_poly_fromsignal` against the I_w definition, and the
Table 1 failure probabilities were not recomputed. The AVX2 / AArch64 /
Cortex-M4 paths in these files are compiled out by `-DDKE_FORCE_SCALAR` and
were not reviewed.
