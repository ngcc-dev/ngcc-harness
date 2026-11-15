# kem-15 FLIT — algorithm summary

NTRU-shaped KEM over R_q = Z_q[X]/(X^N+1) with a prime, NTT-friendly q: the
public key is h = g·f^-1 and the ciphertext is c = Compress_d(h·r + e +
((q+1)/2)·Encode(m)), so key recovery is NTRU and message recovery is RLWE.
Secrets are ternary. The message is expanded by a **repetition code**
(D2 / D4 lattice code, N/n copies of each bit) that is *homomorphic* over
Z_2[X]/(X^n+1): decryption recovers f·m mod (X^n+1, 2) and multiplies by the
precomputed inverse f' of f in that ring. IND-CCA comes from a Kyber-style
FO transform with implicit rejection.

Specification: `kem-15-spec.pdf` (24 pages), §1.2 (Algorithms 6–7,
Encode/Decode), §1.3.1 (Algorithms 8–10, CPAPKE), §1.3.2 (Algorithms 11–13,
CCAKEM), parameters §1.5 Table 2, sizes §4.2.4 Table 11.

## Parameters

Spec Table 2 (p. 12). T_sigma = ternary with Pr[±1] = sigma.

| parameter | Flit128 | Flit256 | Flit512 | meaning |
|---|---|---|---|---|
| N | 512 | 1024 | 2048 | ring degree |
| n | 256 | 256 | 512 | message length in bits (N/n copies per bit) |
| q | 769 | 769 | 3329 | modulus |
| Tf | 1/8 | 5/32 | 7/16 | ternary parameter for f |
| Tg | 5/16 | 1/4 | 7/16 | ternary parameter for g |
| Tr | 1/8 | 5/32 | 7/16 | ternary parameter for r |
| Te | 5/16 | 1/4 | 7/16 | ternary parameter for e |
| d | 8 | 8 | 9 | ciphertext compression bits |
| delta (DFR) | 2^-187.5 | 2^-176.1 | 2^-195.5 | decryption failure probability |
| claimed level | 128 | 256 | 512 | bits |

Sizes (bytes), spec Table 11 (p. 21) vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | ct spec | ct impl | ss impl | match |
|---|---|---|---|---|---|---|---|---|
| FLIT128 | 615 | 615 | **839** | **1351** | 512 | 512 | 32 | **no (sk)** |
| FLIT256 | 1229 | 1229 | **1581** | **2605** | 1024 | 1024 | 32 | **no (sk)** |
| FLIT512 | 3072 | 3072 | **3776** | **6336** | 2304 | 2304 | 64 | **no (sk)** |

## Pseudocode

### Encode / Decode (spec Algorithms 6–7) — repetition code
```
Encode(m in {0,1}^n) -> M in R_q:          // M = m(X) * (1 + X^n + ... + X^{N-n})
  for i in 0..n-1:  M[i+jn] := m[i]  for j = 0 .. N/n - 1

Decode(c = ((q+1)/2)Encode(m) + e) -> m:
  for i in 0..n-1:
     if sum_{j=0}^{N/n-1} | c[i+jn] - (q+1)/2 |  <  (N/n)*(q-1)/4   : m[i] = 1
     else                                                            : m[i] = 0
  // spec writes the bound as (q-1)/2 for N/n = 2 and (q-1) for N/n = 4
```
Lemma 1: f·Encode(m) = Encode(f·m mod (X^n+1,2)) mod (X^N+1,2) for *every*
f — this is why decryption can strip f afterwards. Lemma 2 gives the decoding
condition sum_j |e_{i+jn} mod±q| < (N/n)·(q-1)/4.

### Flit.CPAPKE.KeyGen (spec Algorithm 8)
```
1. z <- {0,1}^lambda ;  kappa := 0
3. repeat: f := Ternary_{Tf}(PRF(z, kappa));  kappa++
6. until f invertible in R_2 and in R_q
7. g  := Ternary_{Tg}(PRF(z, kappa))
8. f' := Inversion(f)                  // inverse of f in Z_2[X]/(X^n+1)
9-11. fhat := NTT(f); ghat := NTT(g); hhat := ghat o fhat^-1
12-13. sk := (fhat, f');  pk := hhat   // public key kept in NTT domain
```
### Flit.CPAPKE.Enc(pk = hhat, m, rho) (spec Algorithm 9)
```
1. r := Ternary_{Tr}(PRF(rho, 0));   2. e := Ternary_{Te}(PRF(rho, 1))
3. rhat := NTT(r)
4. c1 := NTT^-1(hhat o rhat) + e + ((q+1)/2) * Encode(m)
5. c  := Compress_q(c1, d)
```
### Flit.CPAPKE.Dec(sk = (fhat, f'), c) (spec Algorithm 10)
```
1-2. c1 := Decompress_q(c, d);  c1hat := NTT(c1)
3.   c2 := NTT^-1(fhat o c1hat)         // = gr + fe + ((q+1)/2) f*Encode(m)
4.   m' := Decode(c2)                   // = f*m mod (X^n+1, 2)
5.   m  := m' * f'  mod (X^n+1, 2)
```
There is no failure symbol at the PKE layer: Decode always returns n bits.
Correctness (Theorem 1) holds when, for every i,
sum_j |u_{i+jn} mod± q| < (N/n)(q-1)/4 with u = gr + f(e+e') + floor(fM/2) and
e' the compression error; the residual probability is delta in the table.

### Flit.CCAKEM (spec Algorithms 11–13)
```
KeyGen: rho <- {0,1}^lambda; (sk', pk) <- CPAPKE.KeyGen()
        sk := sk' || pk || H(pk) || rho

Enc(pk): m <- {0,1}^lambda;  m := H(m)        // do not send raw RNG output
         (Kbar, r) := G(m || H(pk))
         c := CPAPKE.Enc(pk, m, r)
         K := KDF(Kbar || H(c))

Dec(sk, c): m' := CPAPKE.Dec(sk, c)
            (Kbar', r') := G(m' || H(pk))
            c' := CPAPKE.Enc(pk, m', r')
            if c = c': K := KDF(Kbar' || H(c))  else  K := KDF(rho || H(c))
```
Hashes (implementation, `symmetric.h`): for FLIT128/256 H = `sm3hash(256)` and
G = `pseudohash(512)`; for FLIT512 H = `pseudohash(512)` and G =
`pseudohash(1024)`; PRF and KDF are `pseudoXOF`.

## Implementation vs specification

Checked: `src/FLIT{128,256,512}_REF/params.h` for constants and
`kem.c`, `indcpa.c`, `poly.c`, `symmetric.h` for the flow.

Agreements:
- N, q, d and all four ternary parameters match Table 2 for all three
  instances. The P* macros are the decimal digits of sigma:
  FLIT128 `PF 125, PG 3125, PR 125, PE 3125` = (1/8, 5/16, 1/8, 5/16);
  FLIT256 `15625, 25, 15625, 25` = (5/32, 1/4, 5/32, 1/4);
  FLIT512 all `4375` = 7/16. The corresponding samplers
  (`ternary_15625`, `ternary_3125`, `ternary_4375`, ...) use exactly the
  bits-per-coefficient and thresholds of §1.1 Algorithm 2 and Table 1
  (5 bits with cut-offs 5/10 for 5/32, 4 bits with 5/10 for 5/16, etc.) and
  are branchless, as the spec's "no rejection sampling" rationale requires.
- n is `KEM_CPAPKE_MSGBYTES = SEEDBYTES` = 32/32/64 bytes = 256/256/512 bits,
  matching Table 2's n column.
- The FO transform is Algorithms 11–13 line for line (`kem.c`), including
  `m := H(m)` on the raw DRNG output (`hash_h(buf, buf, SEEDBYTES)`),
  the `H(pk)` multi-target binding, `K = KDF(pre-k || H(c))` in both branches,
  and a constant-time `verify` + `cmov` that swaps in rho on failure.
- Key generation implements both invertibility tests of Algorithm 8 line 6
  (`poly_inv_in_F2` for R_2^x, `poly_baseinv` for R_q^x) with the same
  resample-and-advance-nonce structure, and g is sampled with the nonce
  following the accepted f.
- All randomness (`indcpa_keypair` seed, the z value, the encapsulation seed)
  comes from `get_random_number(&drng_algorithm, ...)`.
- pk and ct sizes match Table 11 exactly: `KEM_POLYBYTES` is the hard-coded
  compact NTT-domain encoding (615 / 1229, exploiting the gap between q = 769
  and 1024) or the full-width 3072 for q = 3329, and
  `KEM_POLYCOMPRESSEDBYTES = N*d/8` gives 512 / 1024 / 2304.

Discrepancies:
- **(a) every secret-key size in spec Table 11 is wrong, and impossibly so.**
  The implementation's
  `KEM_SECRETKEYBYTES = (N*Q_BITS/8 + SEEDBYTES) + KEM_POLYBYTES + 2*SEEDBYTES`
  (`params.h`) = 1351 / 2605 / 6336, which is what the library reports, and
  which matches the spec's own description of sk (fhat uncompressed in the NTT
  domain, f' as n bits, a copy of pk, H(pk), and the FO seed — Algorithm 11
  line 3). Table 11's 839 / 1581 / 3776 cannot be right under that
  description: at FLIT128 the public-key copy alone is 615 bytes and fhat in
  uncompressed NTT form needs at least N*ceil(log2 q)/8 = 640, already 1255
  bytes before f', H(pk) and rho. The same holds at the other two levels
  (1229 + 1280 = 2509 > 1581; 3072 + 3072 = 6144 > 3776). The spec's sk column
  appears to have been computed from an earlier, seed-only key format. This is
  the highest-value finding for this candidate; pk and ct are fine.
- **(b) the shared-secret length is not specified.** Algorithm 12 line 5 just
  says K := KDF(...). The implementation's `kdf` emits `SEEDBYTES` bytes,
  i.e. 32 / 32 / 64, matching the observed ss. Reasonable but undocumented.
- **(c) equivalent.** `hash_h`/`hash_g` are instantiated differently per level
  (SM3-256 + pseudohash-512 at 128/256; pseudohash-512 + pseudohash-1024 at
  512) so that H and G outputs scale with lambda; §1.1 fixes no
  instantiation, and §4.2.3 offers a separate FIPS-202 variant
  (SHA-256 / SHA-512 / SHAKE-256) that is not the tree built here; that
  variant's code defines H and G as `sha3_256` and `sha3_512`
  (`FLIT_FIPS202_REF128/symmetric.h:21-22`).
- **(c)** The public key is stored and transported in the NTT domain
  (Algorithm 8 line 13 says so explicitly), and `indcpa_enc` therefore skips a
  forward transform; the ciphertext is serialized in the standard domain
  because compression truncates low-order bits — both as §4.2.4 explains.

Not verified: the `Inversion` / `NewtonInv` / `Translate` chain (Algorithms
3–5) against §1.1, the `KaratsubaMul`/NTT layer (Algorithm 1), and the
Table 2 failure probabilities (the spec's own `FLIT.py` script was not run).
