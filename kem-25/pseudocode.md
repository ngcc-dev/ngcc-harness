# kem-25 NEV — algorithm summary

NTRU/RLWE hybrid KEM over `R_q = Z_q[x]/(x^n+1)`. The one-way PKE
(`NEV-PKE-OW`) is an NTRU-style scheme `h = g/f` with `f = f' + v^{-1}`, where
`v = 1 − x^{n/k}` and `v^{-1} = ((q+1)/2)(1 + x^{n/k} + … + x^{(k−1)n/k})`. The
message is embedded not as a coset but *inside the noise* by a randomized
encoding `Pt2noise` and recovered by a correlation/vector decoder `Noise2Pt`
over k = 4 coefficients per message bit — hence no extra ciphertext component.
The IND-CCA KEM is the FO transform with **explicit rejection**.

Specification: `kem-25-spec.pdf` (98 pages), §3.1 (Pt2noise/Noise2Pt, Lemma 3),
§3.2 (NEV-PKE-OW), §3.5 (NEV-KEM-CCA); implementation-level restatement
§5.7/§5.10 (Algorithms 15–18, 25–27); parameters Table 2 (p. 18).

## Parameters

k = 4 for every set (4 ring coefficients per message bit), `κ = SEED_BYTES` =
message = shared-secret length, and `n/k = 8κ` bits. `B_η` = centred binomial;
`T_{1/p}` = ternary with `Pr[±1] = 1/p` each; `T*_{1/3}` = its rounded variant.
The three "-C*" (compressed) sets replace the uncompressed ciphertext `c = hr+m`
by `c' = ⌊h^{-1}m · 256/769⌉` (Remark 5).

| set (dir) | PARAMS | n | q | κ | χ_f | χ_g | χ_r | χ_e | compress | DFR | NTRU primal C/Q | MATZOV† | claimed |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| NEV-C1* (NEV-C1-c) | 1 | 512 | 769 | 16 | B1 | B2 | T*_{1/3} | B2 | yes | 2^−160 | 132/116 | 129 | 128 |
| NEV-R1 (NEV-R1) | 4 | 512 | 1409 | 16 | B3 | B3 | B3 | B3 | no | 2^−133 | 135/119 | 134 | 128 |
| NEV-D1 (NEV-D1) | 7 | 512 | 3329 | 16 | B7 | B7 | B7 | B7 | no | 2^−136 | 135/118 | 134 | 128 |
| NEV-C1 (NEV-C1) | 10 | 512 | 769 | 16 | B1 | B2 | T_{1/3} | B2 | no | ≈2^−160 | ≈132/116 | ≈129 | 128 |
| NEV-C2* (NEV-C2-c) | 2 | 1024 | 769 | 32 | B1 | T_{1/8} | T*_{1/3} | B2 | yes | 2^−162 | 266/234 | 268 | 256 |
| NEV-R2 (NEV-R2) | 5 | 1024 | 1409 | 32 | B2 | B2 | B2 | B2 | no | 2^−158 | 284/250 | 261 | 256 |
| NEV-D2 (NEV-D2) | 8 | 1024 | 3329 | 32 | B4 | B4 | B4 | B4 | no | 2^−217 | 280/246 | 264 | 256 |
| NEV-C2 (NEV-C2) | 11 | 1024 | 769 | 32 | B1 | T_{1/8} | T_{1/3} | B2 | no | ≈2^−162 | ≈266/234 | ≈268 | 256 |
| NEV-C3* (NEV-C3-c) | 3 | 2048 | 769 | 64 | T_{1/8} | T_{1/8} | T*_{1/3} | B1 | yes | 2^−163 | 546/480 | 521 | 512 |
| NEV-R3 (NEV-R3) | 6 | 2048 | 1409 | 64 | T_{1/3} | T_{1/3} | T_{1/3} | B2 | no | 2^−147 | 583/513 | 526 | 512 |
| NEV-D3 (NEV-D3) | 9 | 2048 | 3329 | 64 | B2 | B3 | B2 | B3 | no | 2^−301 | 572/503 | 516 | 512 |
| NEV-C3 (NEV-C3) | 12 | 2048 | 769 | 64 | T_{1/8} | T_{1/8} | T_{1/3} | B1 | no | ≈2^−163 | ≈546/480 | ≈521 | 512 |

(Spec Table 2 lists the nine C*/R/D sets; the three uncompressed "NEV-C1/2/3"
sets are described in the §4 text as differing only in `χ_r = T_{1/3}` instead of
`T*_{1/3}`, "their estimated security and decryption failure rates remain
essentially the same" — hence the ≈ entries, which the spec does not tabulate.)

Sizes (bytes), specification Table 2 vs the built reference library (OBSERVED).
Table 2 gives only (PK, CT); the secret key is `(sk', pk, H1(pk))`, i.e.
`2·POLY_BYTES + κ`, which is what the library reports.

| instance | pk spec | pk impl | ct spec | ct impl | sk (derived) | sk impl | ss | match |
|---|---|---|---|---|---|---|---|---|
| NEV_512_769_C_ICCS | 615 | 615 | 512 | 512 | 1246 | 1246 | 16 | yes |
| NEV_512_1409_ICCS | 672 | 672 | 672 | 672 | 1360 | 1360 | 16 | yes |
| NEV_512_3329_ICCS | 768 | 768 | 768 | 768 | 1552 | 1552 | 16 | yes |
| NEV_512_769_ICCS | 615 | 615 | 615 | 615 | 1246 | 1246 | 16 | yes |
| NEV_1024_769_C_ICCS | 1229 | 1229 | 1024 | 1024 | 2490 | 2490 | 32 | yes |
| NEV_1024_1409_ICCS | 1344 | 1344 | 1344 | 1344 | 2720 | 2720 | 32 | yes |
| NEV_1024_3329_ICCS | 1536 | 1536 | 1536 | 1536 | 3104 | 3104 | 32 | yes |
| NEV_1024_769_ICCS | 1229 | 1229 | 1229 | 1229 | 2490 | 2490 | 32 | yes |
| NEV_2048_769_C_ICCS | 2458 | 2458 | 2048 | 2048 | 4980 | 4980 | 64 | yes |
| NEV_2048_1409_ICCS | 2688 | 2688 | 2688 | 2688 | 5440 | 5440 | 64 | yes |
| NEV_2048_3329_ICCS | 3072 | 3072 | 3072 | 3072 | 6208 | 6208 | 64 | yes |
| NEV_2048_769_ICCS | 2458 | 2458 | 2458 | 2458 | 4980 | 4980 | 64 | yes |

All 12 KATs reproduce (PASS ×12).

## Pseudocode

One block covers all 12 sets; the only per-set differences are `(n, q, κ)`, the
four distributions, and whether the ciphertext is compressed.

### Plaintext encoding / decoding (spec §3.1, Fig. 4)
```
v = 1 - x^{n/k},   v^{-1} = ((q+1)/2) * (1 + x^{n/k} + ... + x^{(k-1)n/k}),  k = 4

Pt2noise(M, eta):                                  # M in {0,1}^{n/k}
  s_0..s_{2k*eta-2} <- $ ({0,1}^{n/k})^{2k*eta-1}
  s_{2k*eta-1} = M XOR (s_0 XOR ... XOR s_{2k*eta-2})     # M hidden in the parity
  lay (s_0..s_{2k*eta-1}) out as 2*eta rows x n columns; column i gives the
  2*eta B_eta coins for coefficient i:
      m_{i*n/k + j} = sum_{t<eta} ( s_{2*i*eta+t, j} - s_{2*i*eta+eta+t, j} )
  return m in R_q                                  # marginally distributed as B_eta

Noise2Pt(w):
  w~_i = (w_i - (q+1)/2) mod± q                 for all i in [n]
  t_j  = sum_{i = j mod n/k} |w~_i|             for all j in [n/k]   (k terms)
  M_j  = 1 if t_j < k*(q-1)/4 else 0
```

### NEV-PKE-OW (spec §3.2 / Algorithms 15–18)
```
KeyGen():
  repeat  f' <- chi_f ;  f = f' + v^{-1}   until f invertible in R_q
  repeat  g  <- chi_g                       until g invertible in R_q
  return pk = h = g/f,  sk = f

Enc(pk = h, M ; rho):
  r <- chi_r  (coins from PRG(rho,0));  m <- Pt2noise(M, eta_e)  (coins PRG(rho,1))
  return c = h*r + m                       # compressed sets: c' = round( h^{-1}*m * 256/769 )

Dec(sk = f, c):
  w = f*c = g*r + f'*m + e' + v^{-1}*mbar
  return M' = Noise2Pt(w)
```

### NEV-KEM-CCA (spec §3.5 / Algorithms 25–27)
```
KeyGen():  (pk,sk') <- PKE-OW.KeyGen();  sk = (sk', pk, H1(pk))

Encap(pk): M <- $ {0,1}^{n/k}
           (K, rho) = H2( M || H1(pk) )
           c = PKE-OW.Enc_internal(pk, M ; rho)
           return (c, K)

Decap(sk, c):
           M'        = PKE-OW.Dec(sk', c)
           (K', rho')= H2( M' || H1(pk) )
           c'        = PKE-OW.Enc_internal(pk, M' ; rho')
           if c' != c: return _|_        # EXPLICIT rejection (spec Alg. 27 line 6)
           return K'
```

Symmetric roles (`api.h`): `H1 = Hash` (κ-byte output), `H2 = Hash2` (2κ-byte
output), the noise PRG is `KDF`. In the NGCC build (`-DUSE_ICCS`) all three are
the official ICCS `pseudoXOF`/`sm3hash`/`pseudohash`; the candidate's own build
(`-DUSE_SHA3`) uses SHAKE-128/256/512 instead.

## Implementation vs specification

Built tree: `Implementations/Reference_Implementation/NEV-*` with
`poly.c ntt.c owpke.c verify.c sample.c pack.c cca.c cpa.c symmetrics/*
kat_test/{rng,drng,KEM_AlgorithmInstance}.c`, `-DPARAMS=<1..12> -DUSE_ICCS`.
**All twelve directories are byte-identical** (verified by `diff` on
`cca.c owpke.c sample.c poly.c pack.c params.h` against NEV-R1) — the parameter
set is selected purely by `-DPARAMS=N`, so one review covers all twelve.

Agreements:

- Every distribution in spec Table 2 is reproduced by `params.h`'s
  `ETA_F/ETA_G/ETA_R/ETA_E`, where 1–7 select `poly_binomial_dist<η>` (B_η) and
  the two sentinel values 8 → `poly_bias8_ternary` (T_{1/8}) and 9 →
  `poly_bias3_ternary` (T_{1/3}) (`sample.c:843-912`). All 12 columns checked;
  e.g. PARAMS=2 → `(ETA_F,ETA_G)=(1,8)` = `(B1, T_{1/8})` = spec's NEV-C2*, and
  PARAMS=6 → `(9,9,9,2)` = `(T_{1/3},T_{1/3},T_{1/3},B2)` = NEV-R3.
  `poly_bias8_ternary` really does yield `Pr[±1] = 1/8` (a `cbd1` sample, which is
  nonzero with probability 1/2, ANDed with an independent 1/2 mask);
  `ternary3` (`sample.c:87-106`) is rejection-sampled base-3, i.e. uniform ternary
  = T_{1/3}.
- `POLY_BYTES` per set equals spec Table 2's PK column exactly, and the compressed
  sets use `PKE_OW_CT_BYTES = PARAM_N` = the Table's CT column. `KEM_CCA_SK_BYTES
  = PKE_OW_SK_BYTES + PK + κ` matches the spec's `sk = (sk', pk, H1(pk))`.
- `poly_add_vinv` (`poly.c`) adds `(q+1)/2` at the four positions
  `0, d/4, d/2, 3d/4`, i.e. `v^{-1} = ((q+1)/2)Σ x^{i n/k}` with k = 4, matching §3.1.
- `ow_pke_keypair` / `ow_pke_enc_interal` / `ow_pke_dec` in the uncompressed
  branch (`owpke.c:93-169`)
  implement `h = g/f`, `c = h r + Pt2noise(M)`, `w = f c`, `M' = Noise2Pt(w)`
  exactly as §3.2. The `COMPRESS == 1` branch (`owpke.c:14-89`), used by the
  three compressed sets, follows Remark 5 instead: the stored public key is
  `f/g` (the spec's `h^{-1}`), `sk = g`, no `r` is sampled,
  `c = poly_compress(pk · Pt2noise(M))`, and decryption computes
  `Noise2Pt(g · poly_decompress(c))`. The invertibility rejection loops increment the PRG nonce on
  each retry, so retries do not reuse coins.
- The FO wrapper (`cca.c:12-52`) matches Algorithms 25–27 line for line,
  including `H1(pk)` being stored in `sk` and reused at decapsulation rather than
  recomputed, and `Hash2(M ‖ H1(pk))` producing `(K ‖ ρ)`.
- Randomness in the NGCC build comes only from the official DRNG:
  `kat_test/rng.c` defines `randombytes()` as
  `get_random_number(&drng_algorithm, x, 8*xlen)`, replacing the shipped
  `randombytes.c`. `ow_pke_keypair` additionally hashes the seed before use.

Discrepancies:

- **(b) Spec-internal inconsistency the code inherits: the compressed sets use
  explicit rejection although §4 says they require implicit rejection.**
  Spec §4 (p. 18) states that the `c' = ⌊h^{-1}m·256/769⌉` compression comes "at
  the cost of introducing a rounding-based hardness assumption **and requiring an
  implicit rejection strategy for the security proof**". But the spec's own
  Algorithm 27 (§5.10) returns ⊥, and `cca.c:47-51` is the single shared
  implementation for all twelve sets:
  `fail = verify(...); if (!fail) { ss = kr; } return fail;`
  So NEV-C1*, NEV-C2*, NEV-C3* (built as `NEV_*_769_C_ICCS`) ship with explicit
  rejection, which is not the transform their claimed IND-CCA proof is stated for.
  There is no `COMPRESS`-dependent branch anywhere in `cca.c`.
- **(c) On rejection, `kem_cca_dec` leaves the shared-secret buffer untouched**
  (`cca.c:48-51`) and signals only through the return value. That is faithful to
  the spec's ⊥, but it means a caller that ignores the return code reads whatever
  was in `ss` beforehand rather than a pseudorandom value; there is no implicit
  fallback key in `sk` at all.
- **(c) Two compensating off-by-ones in `Noise2Pt`.** Spec §3.1 says
  `w̃_i = (w_i − (q+1)/2) mod± q` and `M_j = 1 iff t_j < k(q−1)/4` (= `q−1` for
  k = 4). The code (`pack.c`, `flipabs` and `poly_tomsg`) uses
  `|caddq(w_i) − ⌊q/2⌋|` — i.e. `(q−1)/2`, one less than `(q+1)/2` — and then
  decides `M_j = 1` iff `t_j − q` is negative, i.e. `t_j < q` rather than
  `t_j < q − 1`. The decision boundary therefore sits up to 5 units away from the
  spec's. Since the correct/incorrect gap is ≈ `k(q+1)/2 − k(q−1)/4 ≈ 1400`, this
  is far inside the noise margin and does not measurably change the tabulated DFRs,
  but it is not the stated rule, and the implementation is internally inconsistent
  (`poly_add_vinv` uses `(q+1)/2`, `flipabs` uses `⌊q/2⌋`).
- **(c) Domain separation in the ICCS build is incidental.** In the SHA3 build
  `H1`, `H2` and the noise `KDF` are three distinct primitives
  (SHAKE-128/256/512). Under `-DUSE_ICCS` (`symmetrics/hashkdf.c:58-119`) `hash128`,
  `kdf128`, `kdf256` and `kdf512` call `pseudoXOF`, `hash256` calls
  `sm3hash(256, …)` and `hash512`/`hash1024` call `pseudohash(512/1024, …)`,
  **none with a label or prefix**; the streaming `KDF_ABSORB`/`KDF_SQUEEZEBLOCK`
  used by `poly_bias3_ternary` is `sm3hash(256, input ‖ be32(counter))` per
  32-byte block (`hashkdf.c:17-54`). Roles that share a function (e.g. `H1` and
  `KDF` at κ = 16, both `pseudoXOF`)
  stay separated only because their input lengths happen to differ (`H1` absorbs
  `POLY_BYTES`, `H2` absorbs `2κ`, `KDF` absorbs `κ+1`, the keygen seed hash
  absorbs `κ`). Nothing in the code enforces that, and the spec's §5.3 "Instantiating
  of PRG" (Table 3) describes the SHAKE instantiation, not this one.
- **(c) Non-canonical decoding is not rejected.** `poly_frombytes` (`pack.c`)
  is base-q decoding (`encode16`/`decode6` pack two/six coefficients into a
  radix-q integer) and performs no range check; a ciphertext whose packed integer
  exceeds `q^2` (resp. `q^6`) decodes to out-of-range coefficients. The FO
  re-encryption comparison rejects such ciphertexts, so this is not exploitable
  here, but there is no explicit input validation.
- **(c) Serialization note.** The uncompressed ciphertext is written from the
  *partial-NTT domain* (`owpke.c:142-147`: no inverse NTT before `poly_tobytes`),
  and the stored `sk = f` is likewise in the NTT domain. This is a bijective
  re-encoding of the spec's ring elements, but the spec's §5.5 "Encoding of R_q
  Elements" describes the coefficient-domain base-q packing without saying which
  domain the packed element is in, so the wire format is not reconstructible from
  the specification alone.

Not verified: the DFR, Core-SVP and MATZOV† numbers of Table 2; the partial-NTT
identities; and the exact coefficient-index permutation used by `poly_tomsg`
(groups `{i, i+16, i+32, i+48}` within 64-coefficient blocks rather than the
spec's `i ≡ j mod n/k`) — the two agree up to the NTT ordering, which was not
re-derived here, only confirmed self-consistent through KAT reproduction.
