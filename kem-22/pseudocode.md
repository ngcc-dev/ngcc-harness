# kem-22 Mithril (ARCANE suite) — algorithm summary

Lattice KEM from *radical-ring* Learning-With-Rounding (RR-LWR, a transported-basis
Ring-LWR instance over `S_{q,n,k} = (Z_q[y]/(y^n+1))[x]/(x^k - y - 2)`). An IND-CPA
Saber-style rounding PKE is turned into an IND-CCA KEM by the Hofheinz–Hövelmanns–Kiltz
modular FO transform with **implicit rejection**, variant `U^{≠⊥}_m` (the ciphertext is
*not* an input to the encapsulation-time key derivation).

Specification: `kem-22-spec.pdf` (49 pages), §4 "Algorithm Description", Algorithm 1;
parameters in Table 2 (p. 17); security in §5.1.

## Parameters

| parameter | Mithril-128 | Mithril-256 | Mithril-512 | meaning |
|---|---|---|---|---|
| n | 128 | 128 | 128 | base-ring degree (`y^n+1`) |
| k | 5 | 9 | 17 | radical extension degree (module rank analogue) |
| ℓ | 1 | 2 | 4 | number of top x-coefficients carrying the message (ℓn = λ) |
| ε_q (log q) | 13 | 13 | 13 | ring modulus q = 2^13 |
| ε_p (log p) | 11 | 11 | 11 | rounding modulus p = 2^11 |
| ε_t (log t) | 3 | 3 | 8 | ciphertext compression modulus t |
| D_A | U_{ε_q} | U_{ε_q} | U_{ε_q} | distribution of A (uniform 13-bit) |
| D_s | U_2 | U_2 | U_2 | secret uniform on [−η, η−1], η = q/2p = 2 (2 bits/coeff) |
| failure prob. (log2) | −603 | −280 | −230 | spec Table 2 |
| Core-SVP classical / quantum | 129 / 117 | 257 / 233 | 526 / 478 | bits |
| claimed security | 128 | 256 | 512 | bits (spec's own claim) |

Sizes (bytes), specification (Table 2) vs the built reference library (OBSERVED):

| instance | pk spec | pk impl | sk spec | sk impl | ct spec | ct impl | ss spec | ss impl | match |
|---|---|---|---|---|---|---|---|---|---|
| Mithril-128 | 944 | 944 | 1136 | 1136 | 928 | 928 | 16 | 16 | yes |
| Mithril-256 | 1648 | 1648 | 2000 | 2000 | 1680 | 1680 | 32 | 32 | yes |
| Mithril-512 | 3056 | 3056 | 3728 | 3728 | 3504 | 3504 | 64 | 64 | yes |

The submitted `Test_Vectors/KAT_KEM_Mithril-*.txt` carry the same lengths
(`PK_Len = 944`, `SK_Len = 1136`, …) and the build reproduces them (PASS ×3).

## Pseudocode

Spec Algorithm 1 (p. 18). `[k−ℓ:k−1]` selects the ℓ most significant x-coefficients of
a ring element, read in descending x-degree order.

### PKE.KeyGen / KEM.KeyGen
```
PKE.KeyGen():
 1  seedA, seed_s  <- {0,1}^512                  # 64 bytes each
 2  A <- D_A(S_{q,n,k}; seedA)                   # uniform 13-bit
 3  s <- D_s(S_{q,n,k}; seed_s)                  # uniform in [-2,1]
 4  b = round(p/q * A*s) in S_{p,n,k}
 5  return (pk = (seedA, b), sk = s)

KEM.KeyGen():
 1  (pk_pke, sk_pke) <- PKE.KeyGen()
 2  z   <- {0,1}^{ell*n}                         # implicit-rejection fallback
 3  hpk <- F_{H_F}(pk_pke, lambda/8)
 4  return (pk = pk_pke, sk = (sk_pke, pk_pke, hpk, z))
```

### PKE.Enc / KEM.Encaps
```
PKE.Enc(pk=(seedA,b), m ; seed_s'):
 1  s' <- D_s(S_{q,n,k}; seed_s')
 2  A  <- D_A(S_{q,n,k}; seedA)
 3  b' = round(p/q * A*s')  in S_{p,n,k}
 4  v' = (b*s')[k-ell:k-1] + q/(2p)              in R_{p,n}^ell
 5  cm = floor( t/p * ((v' + (p/2)*m) mod p) )   in R_{t,n}^ell
 6  return c = (cm, b')

KEM.Encaps(pk):
 1  m   <- {0,1}^{ell*n}
 2  hpk <- F_{H_F}(pk, lambda/8)
 3  (ss, seed_s') <- G_{H_G}(hpk, m, lambda/8 + 64)
 4  c   <- PKE.Enc(pk, m; seed_s')               # deterministic
 5  return (c, ss)                               # note: c not hashed into ss
```

### PKE.Dec / KEM.Decaps
```
PKE.Dec(sk=s, c=(cm,b')):
 1  v  = (b'*s)[k-ell:k-1] in R_{p,n}^ell
 2  m' = round( 2/p * ((v - (p/t)*cm + (p/(2t) - q/(2p))) mod p) ) in R_{2,n}^ell
 3  return m'

KEM.Decaps(sk, c):
 1  m' <- PKE.Dec(sk_pke, c)
 2  (ss', seed'_s') <- G_{H_G}(hpk, m', lambda/8 + 64)
 3  c' <- PKE.Enc(pk, m'; seed'_s')
 4  ss_bar <- H_{H_H}(c, z, lambda/8)
 5  ss = (c == c') ? ss' : ss_bar                # constant time
```

### Hashing / domain separation (spec §4, "Hashing", p. 19-20)
```
encode(label, outlen, X_1..X_nf) =
      "ARCANE-Mithril-v1"          (17 bytes)
   || len(label) (1 byte) || label
   || lambda  (u16 LE) || k (1 byte) || outlen (u32 LE) || nf (1 byte)
   || len(X_1) (u64 LE) || X_1 || ... || len(X_nf) (u64 LE) || X_nf
F = XOF(encode("H_F", ...)),  G = XOF(encode("H_G", ...)),  H = XOF(encode("H_H", ...))
XOF = pseudoXOF (the official ICCS SM3-based extendable output function)
```

## Implementation vs specification

Built tree: `Implementations/Reference_Implementation/Mithril-{128,256,512}`,
sources `kem.c pke.c arith/{poly,ring,packing,uniform}.c utils/{auxfunc,drng}.c`
(api header is `kem.h`; there is no `KEM_AlgorithmInstance.h`). The three instance
directories are byte-identical except for `parameters.h`.

- **Parameter constants agree.** `parameters.h` defines, for 128/256/512:
  `RRLWR_N=128`, `RRLWR_K=5/9/17`, `RRLWR_PKE_ELL=1/2/4`, `LOGQ=13`, `LOGP=11`,
  `LOGT=3/3/8`, `LOG_ETA=1` — all six sampled values match spec Table 2, including
  the ε_t = 8 jump at level 512.
- **Sizes agree** and are derived, not hard-wired:
  `PK = 64 + k·(11n/8)`, `CT = ℓ·(ε_t·n/8) + k·(11n/8)`,
  `SK = k·(2n/8) + PK + λ/8 + λ/8`. Evaluating gives exactly the Table 2 column.
- **FO transform present and correct** (`kem.c:73-159`). `kem_enc` derives
  `(ss ‖ seed_s') = G(hpk, m)` with `outlen = λ/8 + 64` and never hashes `ct` into
  `ss` — this is the `U^{≠⊥}_m` variant the spec claims. `kem_dec` re-encrypts and
  selects with a constant-time mask: `ct_cmp` (`pke.c:24-31`) ORs the byte
  differences and returns `0xFF`/`0x00`; `ss[i] = K[i] ^ ((K[i]^Kb[i]) & compare)`
  (`kem.c:150-153`) — not an early return. No inverted condition.
- **Rounding offsets match Algorithm 1.** `poly_add_msg`
  (`pke.c:11-22`) adds `q/(2p)` then `p/2·m` and reduces mod p (Enc line 4-5);
  `poly_subp` (`pke.c:3-9`) adds exactly `h2 = p/(2t) − q/(2p)` (Dec line 2). Both
  reductions are `& (p−1)`, valid since p = 2^11. These two functions are not
  called in the reference tree (the Optimized and Additional trees call them at
  `pke.c:106,147`). The reference path is in `arith/packing.c`:
  `poly_pack_ciphertext_t_from_acc_msg` (`:146`) adds `q/(2p) = 2` to the
  accumulator before truncating, and `poly_pack_message_from_acc_cm`
  (`:200-255`) adds the same `h2`, `+126` for ε_t = 3 and `+2` for ε_t = 8.
- **Randomness** comes only from the official DRNG: `seedA`, `seed_s`, `z` in
  `kem_keygen` and `m` in `kem_enc` are all `get_random_number(&drng_algorithm, …)`
  (`kem.c:53,54,64,91`). No `/dev/urandom`, no `rand()`.
- **Sampling.** `A` is 13-bit uniform with no rejection (`arith/uniform.c:9-42`);
  legitimate because q = 2^13 exactly, matching `D_A = U_{ε_q}`. `s`/`s'` are 2-bit
  uniform, matching `D_s = U_2`, η = q/2p = 2.
- **Observation (c) — second domain encoding.** The implementation uses a
  *different*, shorter encoding than F/G/H for the expansion of `A` and `s`
  (`hash_domain.h:rrlwr_domain_encode_xof`): prefix `"AMX1"` (4 bytes), a numeric
  label id (1 = public/A, 2 = secret/s) instead of the string labels `XOF_A`/`XOF_S`
  that the header defines but never uses, `λ` (u16 LE), `k`, `outlen` (u16 LE),
  1-byte seed length, seed, then a coefficient index and a lane byte. The spec
  gives this compact `AMX1` encoding on p. 20 with the same fields in the same
  order (id 1 for XOF_A, id 2 for XOF_S). It is domain-separated from F/G/H by the
  different prefix, and it is the encoding the shipped KATs were generated with.
- **Observation (c) — instance naming.** `kem.h:27` defaults
  `ALGORITHM_INSTANCE "KEM_RRLWR"` and the candidate Makefile passes
  `KEM_RRLWR<level>`, while the submitted KAT files are named
  `KAT_KEM_Mithril-<level>.txt`. The candidate's own build therefore writes files
  under a name that does not match its own submitted vectors. Cosmetic.
- **Not verified:** the claimed failure probabilities and Core-SVP estimates (spec
  §5.1.4 ships the lattice-estimator script; not rerun), and the correctness of the
  `ring_mul_Awin_*` lazy-accumulator arithmetic beyond the fact that it reproduces
  the KATs.

No deviation found between spec Algorithm 1 and the reference code.
