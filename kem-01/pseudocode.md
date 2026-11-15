# kem-01 Aigis-Enc+ — algorithm summary

Structured-lattice KEM over power-of-two cyclotomic rings, hard problem: *asymmetric*
RLWE (ARLWE) with semi-uniform (rounded) seeds — secret and error are drawn from
*different* centred binomial distributions B_{η_s}, B_{η_e}. An IND-CPA PKE
(Aigis-PKE+) with a 4-dimensional lattice plaintext encoding and modulus-switching
compression is lifted to an IND-CCA KEM by the Fujisaki–Okamoto transform with
implicit rejection.

Specification: `kem-01-spec.pdf` (61 physical pages, English), §3.1–3.3 (mathematical
description) and §5.3–5.6 (Algorithms 3–20, the implementation-level pseudocode).
The three built instances correspond to the spec's Aigis-Enc+-512/-1024/-2048.

## Parameters

| parameter | Aigis-enc1 | Aigis-enc2 | Aigis-enc3 | meaning |
|---|---|---|---|---|
| n | 512 | 1024 | 2048 | degree of R_q = Z_q[x]/(x^n+1) |
| q | 3329 | 3329 | 3329 | modulus (shared, dimension-128 NTT) |
| χ_α (secret) | B_5 | B_2 | B_2 | CBD for s, r |
| χ_β (error) | B_6 | B_6 | B_3 | CBD for e, e1, e2 |
| d_t | 10 | 10 | 10 | public-key compression bits |
| d_u | 10 | 10 | 10 | ciphertext u compression bits |
| d_v | 4 | 3 | 4 | ciphertext v compression bits |
| κ' = κ/8 | 16 | 32 | 64 | message / shared-secret bytes |
| k = n/ℓ | 4 | 4 | 4 | coefficients per plaintext bit (Pt2poly) |
| dec. failure | 2^-145 | 2^-145 | 2^-157 | spec Table 1 |
| claimed security | 130 C / 114 Q | 273 C / 240 Q | 572 C / 503 Q | bits, primal, Core-SVP (spec Table 2) |

Sizes (bytes), specification vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | ct spec | ct impl | ss spec | ss impl | match |
|---|---|---|---|---|---|---|---|---|---|
| Aigis-enc1 | 656 | 656 | **16** (Table 1) / 1456 (Alg. 18) | 1456 | 896 | 896 | 16 | 16 | pk/ct/ss yes; sk see below |
| Aigis-enc2 | 1312 | 1312 | **32** (Table 1) / 2912 (Alg. 18) | 2912 | 1664 | 1664 | 32 | 32 | " |
| Aigis-enc3 | 2624 | 2624 | **64** (Table 1) / 5824 (Alg. 18) | 5824 | 3584 | 3584 | 64 | 64 | " |

The spec's own Table 1 `|sk|` column (16/32/64) is internally inconsistent with the
spec's Algorithm 18, whose secret key is `sk ∈ B^{3n/2 + 3κ' + n·d_t/8}` = 1456/2912/5824.
The implementation follows Algorithm 18. See "Implementation vs specification" below.

## Pseudocode

### Plaintext encoding (spec §3.1)

```
Pt2poly(M):                  # M in {0,1}^ℓ, ℓ = κ
  return m = M_0 + M_1 x + ... + M_{ℓ-1} x^{ℓ-1}   in R_q
# k = largest integer with k | n and n/k >= ℓ;  f^{-1} = (q+1)/2 · (1 + x^{n/k} + ... + x^{(k-1)n/k})
# so f^{-1}·m replicates each bit, scaled by (q+1)/2, into k coefficients.

Poly2Pt(w):                  # w in R_q
  for i in [n]:  w~_i := (w_i - (q+1)/2) mod± q
  for j in [ℓ]:  t_j := sum_{i = j mod n/k} |w~_i|
                 M_j := 1 if t_j < k(q-1)/4 else 0
  return M
# Lemma 1: correct whenever the accumulated error per residue class is < k(q-1)/4.
```

### KeyGen (spec Algorithm 18, over Algorithm 14)

```
Aigis-PKE+.KeyGen():                                     # Algorithm 14
  ξ  <-$ B^{κ'}
  (ρ, σ) := H2(ξ)                                        # κ' + κ' bytes
  â  := GenA(ρ)                             # Alg. 11, rejection sampling, already in NTT domain
  s  := SamplePolyCBD(χ_α, σ, 0)                         # Alg. 13
  e  := SamplePolyCBD(χ_β, σ, 1)
  ŝ  := NTT(s)
  t  := NTT^{-1}(â ∘ ŝ) + e
  t̄  := Compress_{d_t}(t)
  pk' := ρ || Encode_{d_t}(t̄);   sk' := Encode_12(ŝ)

Aigis-Enc+.KeyGen():                                     # Algorithm 18
  s <-$ B^{κ'}                                           # implicit-rejection seed z
  (pk', sk') := Aigis-PKE+.KeyGen(1^κ)
  pk := pk'
  sk := sk' || pk || H1(pk) || s
  return (pk, sk)
```

### Encaps (spec Algorithm 19, over Algorithm 15)

```
Aigis-Enc+.Encap(pk):                                    # Algorithm 19
  M <-$ B^{κ'}
  h := H1(pk)                                # multi-target / contributory countermeasure
  (K, ρ') := H2(M || h)                      # κ' bytes key, κ' bytes coins
  c := Aigis-PKE+.Enc_internal(pk, M, ρ')
  return (c, K)

Aigis-PKE+.Enc_internal(pk, M, ρ'):                      # Algorithm 15
  ρ := pk[0:κ'];  t̄ := Decode_{d_t}(pk[κ':]);  t' := Decompress_{d_t}(t̄)
  â := GenA(ρ)
  r  := SamplePolyCBD(χ_α, ρ', 0);  e1 := SamplePolyCBD(χ_β, ρ', 1);  e2 := SamplePolyCBD(χ_β, ρ', 2)
  u  := NTT^{-1}(â^T ∘ NTT(r)) + e1
  y  := NTT^{-1}(NTT(t') ∘ NTT(r)) + e2
  m  := Pt2poly(M)
  ū  := Compress_{d_u}(u);   v̄ := Compress_{d_v}(y + f^{-1} m)
  return C := Encode_{d_u}(ū) || Encode_{d_v}(v̄)
```

### Decaps (spec Algorithm 20, over Algorithm 17) — **the FO reject path**

```
Aigis-PKE+.Dec(sk', C):                                  # Algorithm 17
  ŝ := Decode_12(sk')
  ū := Decode_{d_u}(C[0 : n·d_u/8]);  v̄ := Decode_{d_v}(C[n·d_u/8 : ])
  z := Decompress_{d_v}(v̄) - NTT^{-1}(ŝ ∘ NTT(Decompress_{d_u}(ū)))
  return M' := Poly2Pt(z)

Aigis-Enc+.Decap(sk, c):                                 # Algorithm 20
  parse sk = sk' || pk || h || s                         # h = H1(pk), s = rejection seed
  M'       := Aigis-PKE+.Dec(sk', c)                     # line 5
  (K', ρ'') := H2(M' || h)                               # line 6
  c'       := Aigis-PKE+.Enc_internal(pk, M', ρ'')       # line 7  (deterministic re-encryption)
  K        := H3(s, c)                                   # line 8  <-- implicit-rejection value
  if c' = c then K := K'                                 # lines 9-11 (constant-time select)
  return K                                               # line 12
```

§3.3 states the same rule in prose: *"If c′ = c, return K = K′, otherwise, return
K = H3(s, c)."* Both K′ and H3(s,c) are κ′ bytes; H3 is XOF-128/256/512 per Table 3.

### Hashes (spec §5.2, Table 3)

```
Hash-d(M)   = Keccak[2d](M || 01,   d)      # d = 128, 256, 512
Hash-1024(M) = Keccak[1024](M || 01, 1024)
XOF-d(M, r) = Keccak[2d](M || 1111, r)      # d = 128, 256, 512

              XOF1          XOF2          H1         H2          H3
  -512     XOF-128       XOF-128      Hash-128   Hash-256    XOF-128(·,128)
  -1024    XOF-128       XOF-256      Hash-256   Hash-512    XOF-256(·,256)
  -2048    XOF-128       XOF-512      Hash-512   Hash-1024   XOF-512(·,512)
```

## Implementation vs specification

Checked: `Implementations/Reference_Implementation/Aigis-Enc+-{I,II,III}` as wired by
`kem-01/Makefile` (`-DPARAMS={1,2,3} -DUSE_NICCS_API`). `kem.c` = Algorithms 18–20;
`owcpa.c` = Algorithms 14/15/17; `pack.c` = Encode/Decode/Compress/Decompress;
`cbd.c` + `poly.c` = Algorithms 12–13; `gen_a.c` = Algorithm 11; `poly_frommsg` /
`poly_tomsg` in `poly.c` = Pt2poly / Poly2Pt; `verify.c` = constant-time compare + cmov;
`hashkdf.c` + `api.h` = the Table 3 instantiation. `kat_test/KEM_AlgorithmInstance.c`
is a pass-through wrapper (lines 48/58/67).

Agreements:
- All three `params.h` files are identical and hold all three parameter sets behind
  `PARAMS`. Sampled constants agree with the table above: `PARAM_Q 3329`,
  `PARAM_N 512/1024/2048`, `(ETA_S, ETA_E) = (5,6)/(2,6)/(2,3)`, `BITS_PK 10`,
  `BITS_C1 10`, `BITS_C2 4/3/4`, `SEED_BYTES = MSG_BYTES = 16/32/64`. (6 constants
  spot-checked per instance; NTT tables in `precomp.c` not verified.)
- `SK_BYTES = POLYVEC_BYTES + PK_BYTES + SEED_BYTES + SEED_BYTES` (`params.h:70`)
  is exactly Algorithm 18's `3n/2 + 3κ' + n·d_t/8`, and all pk/sk/ct/ss byte counts
  match OBSERVED exactly.
- `mkem_keygen` (`kem.c:11-21`) reproduces Algorithm 18 line by line, including the
  fresh `randombytes` draw of the implicit-rejection seed z into
  `sk[SK_BYTES-SEED_BYTES ..]` (`kem.c:18`).
- `mkem_enc` (`kem.c:23-37`) reproduces Algorithm 19: `buf = M || H1(pk)`,
  `Hash2(kr, buf, 2·SEED_BYTES)` = H2, `kr[0:κ']` = K, `kr[κ':2κ']` = coins.
- `owcpa_keypair` derives `(noiseseed, publicseed) = H2(ξ)` from a single κ'-byte
  draw, matching Algorithm 14 lines 1–2 (the two halves are used in the opposite
  order to the spec's `(ρ, σ)` naming — cosmetic, type (c)).
- Two byte-layout differences, type (c): `pack_pk`/`unpack_pk` (`pack.c:8-10,15-17`)
  store the public key as `Encode_{d_t}(t̄) || ρ`, not `ρ || Encode_{d_t}(t̄)` as in
  Algorithm 14 line 9; and for η = 3 `cbd3` (`cbd.c:49-57`) takes CBD_1 from the
  first n/4 bytes of z and CBD_2 from the next n/2, whereas Algorithm 12 lines 11–12
  take CBD_2 first.

### Discrepancy 1 — **the implicit-rejection branch of Decaps is dead code** (real deviation, critical)

`Implementations/Reference_Implementation/Aigis-Enc+-I/kem.c:55-83` (identical in
`-II` and `-III`):

```c
70   Hash2(kr, buf, 2*SEED_BYTES);                  /* (K',rho'') = H2(M' || h)   -- Alg.20 line 6 */
71   memcpy(ss, kr, SEED_BYTES);                    /* ss := K'  ** BEFORE the check **            */
72   owcpa_enc(cmp, buf, pk, kr+SEED_BYTES);        /* c' = Enc(pk, M'; rho'')    -- Alg.20 line 7 */
74   fail = verify(ct, cmp, CT_BYTES);              /* 0 if c' == c, else 1                        */
76   memcpy(buf2, sk-SEED_BYTES, SEED_BYTES);       /* z  ** out-of-bounds, see Discrepancy 2 **   */
77   memcpy(buf2 + SEED_BYTES, ct, CT_BYTES);
78   KDF(buf2, SEED_BYTES, buf2, SEED_BYTES + CT_BYTES);   /* buf2 := H3(z, c)   -- Alg.20 line 8  */
80   cmov(buf, buf2, SEED_BYTES, fail);             /* writes into **buf**, not ss  -- Alg.20 l.9-11*/
81   fail = 0;
82   return 0;
```

Mapping this against Algorithm 20:

| Alg. 20 step | implementation | status |
|---|---|---|
| 5 `M' := PKE.Dec(sk',c)` | `owcpa_dec(buf, ct, sk)` (`kem.c:65`) | present |
| 6 `(K',ρ'') := H2(M'‖h)` | `kem.c:67-70` | present |
| 7 `c' := Enc(pk,M';ρ'')` | `kem.c:72` | present |
| 8 `K := H3(s, c)` | `kem.c:76-78`, into `buf2` | present but never reaches the output |
| 9-11 `if c'=c then K := K'` | `kem.c:74,80` — `cmov` target is `buf`, not `ss` | **omitted / mis-targeted** |
| 12 `return K` | `ss` was already fixed to `K'` at `kem.c:71` | **wrong on failure** |

The re-encryption comparison at line 74 is computed correctly and in constant time,
but its result is never applied to the output. `ss` is committed to `K'` at line 71,
*before* the check; the conditional move at line 80 overwrites `buf` (the recovered
message, dead after line 72) instead of `ss`; `fail` is then zeroed at line 81 and the
function unconditionally returns 0. Decapsulation therefore always returns
`K' = H2(M'‖H1(pk))[0:κ']` and never `H3(s,c)`. The FO transform degenerates to a
plain "decrypt-then-hash", i.e. the scheme is reduced to IND-CPA and admits the
standard chosen-ciphertext key-recovery attacks the FO transform exists to prevent.
This is the confirmed finding `KEM-FO-REENC-FAILPATH` / LH-KEM-01-001..003 in
`security_findings.md`: an exhaustive sweep of Aigis-enc1 found 5632 of 7168
single-bit ciphertext flips returning the *identical* shared secret, and it is
reproduced at levels 2 and 3. All three submitted parameter sets are affected.

The minimal spec-conforming fix is to delete line 71 and make line 80 read
`cmov(ss, buf2, SEED_BYTES, fail)` after `memcpy(ss, kr, SEED_BYTES)` — i.e. select
into the output, not into a scratch buffer.

### Discrepancy 2 — out-of-bounds read of the rejection seed (real deviation, independent of 1)

`kem.c:76` reads the implicit-rejection seed z from `sk - SEED_BYTES`, i.e. SEED_BYTES
*before* the start of the caller's secret-key buffer. `mkem_keygen` stores z at
`sk + SK_BYTES - SEED_BYTES` (`kem.c:18`), so the correct source is
`sk + SK_BYTES - SEED_BYTES`. As written the "fallback" key is derived from
uninitialised/foreign heap memory. Because of Discrepancy 1 this value is discarded,
so the defect is currently unobservable — but any fix to Discrepancy 1 alone would
leave a key derived from out-of-bounds memory, and the read itself is undefined
behaviour today. Filed as kem-01-2 in `report.md`.

### Discrepancy 3 — spec-internal size inconsistency (spec ambiguity / typo)

Spec Table 1 (§4) lists `|sk|` = 16/32/64 bytes, which is a duplicate of the `|ss|`
column and cannot hold `Encode_12(ŝ)` (3n/2 bytes) let alone `pk‖H1(pk)‖s`.
Algorithm 18 gives `sk ∈ B^{3n/2+3κ'+n·d_t/8}` = 1456/2912/5824, which is what
`params.h:70` implements and what OBSERVED reports. Table 1 is a documentation error,
not an implementation deviation. (Two further cosmetic spec slips: Algorithms 19/20
declare `K ∈ B^32` in the header while line 8 and Table 3 give `K ∈ B^{κ'}`; and
Algorithm 19 line 4 / Algorithm 20 line 7 write `Aigis-Enc+.Enc_internal` where
`Aigis-PKE+.Enc_internal` is meant.)

### Discrepancy 4 — symmetric primitives are not the Table 3 Keccak ones (deliberate substitution)

`kem-01/Makefile` builds with `-DUSE_NICCS_API`, which routes `hashkdf.c` away from
SHA-3/SHAKE onto the competition-harness primitives: `hash128 -> pseudoXOF`, `hash256 -> sm3hash`,
`hash512/1024 -> pseudohash`, `kdf128/256/512 -> pseudoXOF`
(`hashkdf.c:7-88`). This is a build-harness substitution of the whole symmetric layer
rather than a scheme bug, but it means the built library does not realise the H1/H2/H3
instantiation of spec Table 3, and the shipped `fips202.c` is unused on this path.
Type (c), flagged for completeness.

### Not verified

- The `poly_frommsg`/`poly_tomsg` sign convention: `owcpa_enc` computes
  `v := t̂r + e2 - k` and `owcpa_dec` computes `mp := ŝ·u - v` (i.e. `-z` relative to
  Algorithm 17). The two are internally consistent and KATs pass (3/3 per
  `security_findings.md`), but the equivalence to Lemma 1's decoding rule was not
  re-derived.
- `precomp.c` NTT/zeta tables and the partial (dimension-128) NTT of Algorithms 1–2.
- The decryption-failure bound of §3.2 and the Table 2 Core-SVP/MATZOV estimates were
  not independently recomputed.
