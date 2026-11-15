# kem-18 LoongKEM — algorithm summary

Lattice KEM over the **semi-structured LWE (SLWE)** problem: the public matrix
`A = [[A1,A2],[A3,A4]]` is block **anti-circulant** in `A1,A2,A3` (i.e. elements of
`Rq = Zq[X]/(X^N+1)`) but `A4` is a fully generic, unstructured `Zq` matrix — a
deliberate half-way point between Kyber (fully module-structured) and FrodoKEM
(unstructured). An IND-CPA PKE (Loong.PKE) is lifted to an IND-CCA2 KEM by a
Fujisaki–Okamoto transform with implicit rejection. Messages are `N×N` bit
*matrices*, and decryption recovers them NTRU-style by parity
(`(2·(V−W) mod± q) mod 2`) instead of the usual `q/4` rounding.

Specification: `kem-18-spec.pdf` (33 pages), §2.2–§2.4 (Algorithms 19–24, Table 2).

## Parameters

| parameter | Loong128 | Loong256 | Loong384 | Loong512 | meaning |
|---|---|---|---|---|---|
| λ | 128 | 256 | 384 | 512 | claimed classical bit security; also \|ss\| = λ/8 |
| q | 8191 | 8191 | 8191 | 8191 | Mersenne prime 2^13−1, `dq = 13` |
| N | 12 | 16 | 20 | 24 | degree of `X^N+1`; plaintext is an `N×N` bit matrix |
| (k1, k2) | (48, 4) | (64, 6) | (72, 8) | (80, 10) | structured / unstructured block counts |
| η | 5 | 4 | 3 | 2 | centred binomial parameter ψ_η |
| (db, du, dv) | (10,10,4) | (10,10,10) | (11,11,6) | (11,11,4) | pk / ct compression widths |
| dη | 4 | 4 | 4 | 4 | bits per secret coefficient (Alg. 17/18) |
| δ | 2^−141 | 2^−161 | 2^−190 | 2^−260 | claimed decapsulation-failure probability |
| claimed security | 128 | 256 | 384 | 512 | bits (spec Table 2, §2.4) |

Sizes (bytes), specification vs the built reference library. The spec gives
formulas, not a size table (§2.3): `pksize = (λ + k1·N·db + k2·N²·db)/8 + 16`,
`ctxsize = (k1·N·du + k2·N²·du + N²·dv)/8`,
`sksize = (k1+k2)N/2 + pksize + |H(pk)| + λ/8 + 16` (with `|H(pk)| = 32`).

| instance | pk spec | pk impl | sk spec | sk impl | ct spec | ct impl | ss | match |
|---|---|---|---|---|---|---|---|---|
| Loong128 | 1472 | 1472 | 1848 | 1848 | 1512 | 1512 | 16 | yes |
| Loong256 | 3248 | 3248 | 3888 | 3888 | 3520 | 3520 | 32 | yes |
| Loong384 | 6444 | 6444 | 7340 | 7340 | 6680 | 6680 | 48 | yes |
| Loong512 | 10640 | 10640 | 11832 | 11832 | 10848 | 10848 | 64 | yes |

All four instances PASS KAT (RESULTS.md).

## Pseudocode

### PKE.KeyGen — spec Algorithm 19
```
seed          <- RNG()                              # λ/8+16 bytes
(ρ, σ, γ)     <- XOF(seed)
A1<-SamplePolyMat(XOF(ρ), k1,k1);  A2<-SamplePolyMat(.., k1,k2)
A3<-SamplePolyMat(.., k2,k1);      A4<-SampleMat(.., k2N, k2N)   # unstructured
s1 <- SampleNoisePolyVec(XOF(σ), k1);  s2 <- SampleNoisePolyVec(.., k2)
S2 <- PolysToBlockMat(s2, k2, 1)
e1 <- SampleNoisePolyVec(XOF(γ), k1);  E2 <- SampleNoiseMat(.., k2N, N)
b1 <- A1 s1 + A2 s2 + e1                                     # in Rq^k1
B2 <- PolysToBlockMat(A3 s1, k2,1) + A4 S2 + E2              # in Zq^{k2N x N}
b  <- Compress(b1, 2^(dq-db));   B <- Compress(B2, 2^(dq-db))
pk <- ρ || EncodePolyVec(b,k1,db) || EncodeMat(B,k2N,N,db)
sk <- EncodeNoisePolyVec(s1,k1,dη) || EncodeNoisePolyVec(s2,k2,dη)
```

### PKE.Encrypt(pk, M, γ=(γ1,γ2,γ3)) — spec Algorithm 20
```
regenerate A1..A4 from ρ = pk[0:λ/8+16];  b1,B2 <- Decompress(decode(pk))
(r1,r2) <- SampleNoisePolyVec(XOF(γ1));  R2 <- PolysToBlockMat(r2^T, 1, k2)
(e3,E4) <- SampleNoisePolyVec/Mat(XOF(γ2));   E5 <- SampleNoiseMat(XOF(γ3), N,N)
u' <- r1^T A1 + r2^T A3 + e3
U' <- PolysToBlockMat(r1^T A2, 1,k2) + R2 A4 + E4
V' <- PolyToMat(r1^T b1) + R2 B2 + E5 + ((q+1)/2)·M           # Δ = 2^-1 mod q
c  <- EncodePolyVec(Compress(u',2^(dq-du)),k1,du)
   || EncodeMat(Compress(U',2^(dq-du)),N,k2N,du)
   || EncodeMat(Compress(V',2^(dq-dv)),N,N,dv)
```

### PKE.Decrypt(sk, c) — spec Algorithm 21
```
u',U',V' <- Decompress(decode(c));   (s1,s2) <- DecodeNoisePolyVec(sk)
W <- PolyToMat(u'^T s1) + U' S2
M <- (2·(V' - W) mod± q) mod 2            # NTRU-style parity recovery
```

### KEM.KeyGen / Encaps / Decaps — spec Algorithms 22, 23, 24
```
KeyGen:  salt <- RNG();  (pk,sk') <- PKE.KeyGen();  sk <- sk' || pk || H(pk) || salt

Encaps:  M <- RNG()                       # N*N random bits
         (K, γ) <- G(M || H(pk))
         c <- PKE.Encrypt(pk, M, γ);  return (K, c)

Decaps:  (sk', pk, h, salt) <- sk
         M'      <- PKE.Decrypt(sk', c)
         (K',γ') <- G(M' || h)
         Kbar    <- H(salt || c)
         c'      <- PKE.Encrypt(pk, M', γ')
         if c != c':  K' <- Kbar            # implicit rejection
         return K'
```

Hash/XOF structure: the spec only declares that LoongKEM "requires a
pseudorandom function RNG, hash functions H and G, and an extendable-output
function XOF" (§2.1, p. 14) — **it never names a concrete primitive**. The
implementation instantiates `H = sm3hash` (32-byte output), and both `G` and
`XOF` as the ICCS `pseudoXOF()` from `auxfunc.c`.

## Implementation vs specification

Checked: `src/Loong128..512/params.h` (constants), `KEM_Loong.c` (FO wrapper +
`indcpa_keypair/encrypt/decrypt`), `poly.c` (sampling, compression, encoding).
Constants sampled for all four levels: `N, K1, K2, Q, ETA, DB, DU, DV, DELTA`
and the derived length macros — every one matches Table 2
(`src/Loong128/params.h:5-35` etc.; `DELTA 4096 = (Q+1)/2 = 2^-1 mod 8191`).
All spec size formulas reproduce the OBSERVED byte lengths exactly (table above).
Compress/Decompress (`poly.c:474-485`) are the spec's shift form including the
`min(·, q−1)` clamp; `Encode/DecodeNoisePolyVec` use `dη = 4` as specified.

**Discrepancy 1 (real deviation, security-relevant).** The implicit-rejection
selector in `src/Loong*/KEM_Loong.c:297-305` is not a boolean mask:

```c
unsigned char fail = 0;
for (i = 0; i < CIPHERTEXT_BYTES; i++) fail |= ct[i] ^ salt_ct[i+SEED_BYTES];
fail = (unsigned char)(-fail);                       /* line 302 */
for (i = 0; i < SHARED_KEY_BYTES; i++) ss[i] ^= fail & (kbar[i]^ss[i]);
```

`fail` is the OR of ciphertext-byte differences, so on mismatch it is an
arbitrary non-zero byte and `-fail` is `256-fail`, which equals `0xFF` **only
when `fail == 1`**. For every other value the mask has zero bits, and in those
bit positions the returned shared secret keeps `K'` — the candidate key derived
from the attacker-influenced decrypted `M'` — instead of `Kbar`. Spec Algorithm
24 line 7 requires the *whole* of `K'` to be replaced by `K̄`. This matches the
IND-CCA distinguisher already recorded in `kem-18/security_findings.md`
("non-boolean rejection mask ... preserving known candidate-key bit positions").
The idiomatic fix is `fail = -(unsigned char)(fail != 0)` or the usual
`verify`/`cmov` pair. Present identically in all four instances.

**Discrepancy 2 (real deviation, cryptographically neutral).** `sample_vector`
(`poly.c:114-126`) assembles the rejection-sampling word big-endian,
`x = buf[i+1] | (buf[i] << 8)`, whereas spec Algorithm 7 line 3 specifies
`x = bytes[i] + 2^8·bytes[i+1]` (little-endian). Since the input is XOF output
the distribution is unchanged, but the KATs are not those of the written spec.

**Discrepancy 3 (real deviation, latent memory bug).** The same loop bounds `i`
by `4*len` while every caller passes a buffer of only `3*len` bytes
(`KEM_Loong.c:70,121`), so a sufficiently rejection-heavy XOF block would read
out of bounds. Spec Algorithm 7 bounds the loop by `3t`. Unreachable in
practice (per-draw rejection probability 2^−13), but the guard is wrong.

**Discrepancy 4 (deliberate equivalent optimisation).** Spec Algorithm 19
draws one `seed` from `RNG()` and expands it to `(ρ,σ,γ)` with `XOF`; the
implementation draws all `3·SEED_BYTES` directly from `drng_algorithm`
(`KEM_Loong.c:64`) and skips the expansion. Distributionally equivalent.

**Discrepancy 5 (real deviation).** Spec Algorithm 24 line 4 computes
`K̄ = H(salt‖c)` over the received ciphertext. `kem_dec` writes the
re-encryption `c′` into `salt_ct+SEED_BYTES` (`KEM_Loong.c:289`) and derives
`kbar` from that buffer (`:295`), so the rejection key is computed over
`salt‖c′`; the received `ct` enters only the comparison at `:298-300`. Filed as
`kem-18-4`.

**Spec ambiguity.** `H`, `G` and `XOF` are never instantiated in the document
(§2.1 p. 14), so the choice of SM3 for `H(pk)` and `pseudoXOF` for `G` and for
`K̄ = H(salt‖c)` cannot be checked against the spec. Note the implementation
uses *different* primitives for the two roles both written `H` in Algorithm 24.

## Reducible-ring security check

The §5.3 estimate says it “temporarily ignore[s]” LoongKEM's algebraic
structure. That is not conservative for three sets. Over the integers,
`X^12+1`, `X^20+1`, and `X^24+1` split into factors of degrees `(4,8)`,
`(4,16)`, and `(8,16)`. Their factor resultants are respectively 81, 625, and
6561, hence units modulo 8191. Reducing the pure-ring public equation
`b1=A1*s1+A2*s2+e1` modulo either factor gives a smaller public SLWE instance;
the two quotient secrets uniquely determine the original secret by CRT.

The weak quotient folds 3, 5, and 3 independent CBD coefficients. Current
MATZOV BDD estimates are 73.06, 103.34, and 203.74 bits. In the complementary
quotient every marginal folds two coefficients, but paired coordinates share
one source coefficient and have correlation magnitude `1/2`. Ignoring those
correlations gives 128.59, 365.51, and 399.31 bits. Thus the algebra and the
large projected cost reduction are confirmed, while full recovery remains a
Lead pending a covariance-aware attack check. See
`reproduce_reducible_ring.py` and finding `kem-18-2`.

**Not verified.** The decryption-failure-rate derivation (§2.2.3) was not
re-computed.
