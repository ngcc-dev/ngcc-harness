# kem-33 QUBE (Quasi-Cyclic with Unbalanced & multi-Block Errors) — algorithm summary

Code-based KEM in the HQC family. The PKE works over `R = F_2[X]/(X^r − 1)`
with a **(3,1)-QCSD** public key and a **(4,2)-QCSD** ciphertext, both with
*unbalanced* weights (`w_x ≠ w_y`, `w_f ≠ w_g ≠ w_e`) and multiple error blocks —
that is QUBE's contribution over HQC. The message is protected by a concatenated
**duplicated Reed–Muller ∘ shortened Reed–Solomon** code. The KEM is the
`FO^{≠⊥}_m` transform with a salt, i.e. implicit rejection.

Specification: `kem-33-spec.pdf` (27 pages), §1.3 (Figs 1–3, PKE), §1.4
(Figs 4–6, KEM), §1.5 (Tables 1–4, parameters), §1.6 + Table 5 (sizes),
§2.2/Table 6 (security), §2.3/Table 7 (DFR).

## Parameters

Spec Table 1 / Table 2. `*QUBE-192` is marked in the spec as "included for
reference and better comparison to HQC", not as a submitted level.

| parameter | QUBE-128 | *QUBE-192 | QUBE-256 | QUBE-384 | QUBE-512 | meaning |
|---|---|---|---|---|---|---|
| λ | 128 | 192 | 256 | 384 | 512 | security parameter (bits) |
| r | 13109 | 25357 | 41651 | 82757 | 135851 | ring degree `X^r − 1` |
| w_x | 74 | 110 | 146 | 219 | 291 | weight of x1, x2 |
| w_y | 76 | 113 | 151 | 226 | 301 | weight of y |
| w_f | 29 | 43 | 57 | 85 | 113 | weight of f1, f2 |
| w_g | 27 | 39 | 52 | 78 | 103 | weight of g |
| w_e | 83 | 121 | 162 | 244 | 324 | weight of e |
| RS `[n1,k1,d1]` | [34,16,19] | [66,24,43] | [108,32,77] | [128,48,81] | [150,64,87] | shortened Reed–Solomon |
| RM mult. `[n2,k2,d2]` | 3, [384,8,192] | 3, [384,8,192] | 3, [384,8,192] | 5, [640,8,320] | 7, [896,8,448] | duplicated Reed–Muller |
| L = n1·n2 | 13056 | 25344 | 41472 | 81920 | 134400 | concatenated code length |
| PK problem (3,1)-QCSD, class./quant. | 144.77 / 102.44 | 208.68 / 136.73 | 273.19 / 170.71 | 402.58 / 237.87 | 530.55 / 303.73 | Table 6 |
| CT problem (4,2)-QCSD, class./quant. | 180.78 / 120.88 | 259.00 / 162.29 | 341.17 / 205.09 | 505.38 / 289.60 | 666.27 / 371.91 | Table 6 |
| overall class./quant. | 136.5 / 98.3 | 199.2 / 132.3 | 263.7 / 166.1 | 392.8 / 232.9 | 520.3 / 298.6 | Table 6 |
| **claimed DFR** (log2 1/DFR) | **137.5** | **200.1** | **264.3** | **392.4** | **520.6** | Table 7 |
| claimed security | 128 | 192 | 256 | 384 | 512 | bits |

The DFR requirement the spec states for itself (§2.3): the FO transform needs
`DFR < 2^{−λ}`; every column of Table 7 is above λ, which is the whole
correctness argument.

### Sizes (bytes) — the three-way comparison

The spec's size formulas (§1.6) are
`|pk| = λ/8 + 2⌈r/8⌉`, `|sk| = 3λ/8 + 2⌈r/8⌉`, `|c| = ⌈r/8⌉ + (L+λ)/8`,
`|k| = λ/8`.

| instance | | spec (Table 5) | reference impl (OBSERVED = in-tree KAT) | submitted `Test_Vectors/` |
|---|---|---|---|---|
| qube-128 | pk | 3,294 | 3,294 | **3,310** |
| | sk | 3,326 | 3,326 | **3,390** |
| | ct | 3,287 | 3,287 | 3,287 |
| | ss | 16 | 16 | **32** |
| qube-192 | pk | 6,364 | 6,364 | **— (no KAT file)** |
| | sk | 6,412 | 6,412 | — |
| | ct | 6,362 | 6,362 | — |
| | ss | 24 | 24 | — |
| qube-256 | pk | 10,446 | 10,446 | 10,446 |
| | sk | 10,510 | 10,510 | **10,542** |
| | ct | 10,423 | 10,423 | **10,503** |
| | ss | 32 | 32 | 32 |
| qube-384 | pk | 20,738 | 20,738 | **20,722** |
| | sk | 20,834 | 20,834 | 20,834 |
| | ct | 20,633 | 20,633 | **21,113** |
| | ss | 48 | 48 | **64** |
| qube-512 | pk | 34,028 | 34,028 | **33,996** |
| | sk | 34,156 | 34,156 | **34,124** |
| | ct | 33,846 | 33,846 | **35,174** |
| | ss | 64 | 64 | 64 |

**The specification and the reference implementation agree exactly, in all
twenty numbers.** The submitted `Test_Vectors/` do not, and the absence of
`KAT_KEM_qube_192.txt` means one of the five specified parameter sets ships with
no vectors at all.

Where the submitted numbers come from — they are exactly
`Implementations/Optimized_Implementation/src/x86/qube-{1,3,4,5}`:

```
Optimized api.h:   CRYPTO_PUBLICKEYBYTES  = VEC_N_SIZE_BYTES + SEED_BYTES + VEC_N_SIZE_BYTES
                   CRYPTO_SECRETKEYBYTES  = pk + SEED_BYTES + PARAM_SECURITY_BYTES + SEED_BYTES
                   CRYPTO_CIPHERTEXTBYTES = VEC_N_SIZE_BYTES + VEC_N1N2_SIZE_BYTES + SALT_BYTES
                   CRYPTO_BYTES           = 32  (qube-1, qube-3)  /  64  (qube-4, qube-5)
Optimized parameters.h:  SEED_BYTES = 32, SALT_BYTES = 16   (ALL levels)
Reference parameters.h:  SEED_BYTES = SALT_BYTES = SHARED_SECRET_BYTES = PARAM_SECURITY_BYTES = lambda/8
```
Evaluating them reproduces the submitted vectors bit for bit:

| | VEC_N | VEC_N1N2 | pk | sk | ct | ss |
|---|---|---|---|---|---|---|
| qube-1 (128) | 1639 | 1632 (L=13056) | 1639+32+1639 = **3310** | +32+16+32 = **3390** | 1639+1632+16 = **3287** | **32** |
| qube-3 (256) | 5207 | 5280 (L=**42240**) | 5207+32+5207 = **10446** | +32+32+32 = **10542** | 5207+5280+16 = **10503** | **32** |
| qube-4 (384) | 10345 | 10752 (L=**86016**) | 10345+32+10345 = **20722** | +32+48+32 = **20834** | 10345+10752+16 = **21113** | **64** |
| qube-5 (512) | 16982 | 18176 (L=**145408**) | 16982+32+16982 = **33996** | +32+64+32 = **34124** | 16982+18176+16 = **35174** | **64** |

Three distinct divergences are visible, and only one of them is cosmetic:

1. **`SEED_BYTES` is fixed at 32** in the optimized tree instead of λ/8. At
   QUBE-128 that inflates pk/sk; at QUBE-384/512 it shrinks them.
2. **The shared secret length is wrong at two levels.** The optimized
   `api.h` hard-codes `CRYPTO_BYTES 32` for qube-1 and `64` for qube-4, so the
   submitted vectors establish a **32-byte secret at QUBE-128** (spec: 16) and a
   **64-byte secret at QUBE-384** (spec: 48).
3. **The optimized tree uses different codes at 256/384/512.** Its
   `PARAM_N1N2` is 42240 / 86016 / 145408 where spec Table 2 and the reference
   give `L = n1·n2 =` 41472 / 81920 / 134400. That is a different concatenated
   code, not a different serialization: the DFR figures of Table 7 and the
   ciphertext lengths both depend on `L`. This is what makes the submitted
   ciphertexts 80 / 480 / 1328 bytes longer than specified.

## Pseudocode

Spec Figures 1–6. `Pack`/`Unpack` are bit↔byte packing, `trunc(·, L)` keeps the
first L bits, `C.Encode`/`C.Decode` are the RM∘RS concatenated code.

### PKE.KeyGen (Fig. 1)
```
 1  (rho_0, rho_1) = PRG(rho)                                  # rho in B^{lambda/8}
 2  H = (h1, h2) = GenUniformVector(rho_0, (r, r))  in R^2
 3  (y, x1, x2) = GenShortVector(rho_1, r, (w_y, w_x, w_x))    # fixed weights
 4  S = (s1, s2) = (x1 + y*h1,  x2 + y*h2)
 5  pk_PKE = rho_0 || Pack(S)                                  # lambda/8 + 2*ceil(r/8) bytes
 6  sk_PKE = rho_1                                             # lambda/8 bytes
```

### PKE.Enc (Fig. 2)
```
 1-3 parse rho_0 || s = pk;  H = GenUniformVector(rho_0,(r,r));  S = Unpack(s,(r,r))
 4   (f1, f2, e, g) = GenShortVector(rho_c, r, (w_f, w_f, w_e, w_g))
 5   u = h1*f1 + h2*f2 + g
 7   v = trunc(s1*f1 + s2*f2 + e, L)  +  C.Encode(Unpack(m, lambda))
 8   c_PKE = Pack(u) || Pack(v)                                # ceil(r/8) + L/8 bytes
```

### PKE.Dec (Fig. 3) — the decoding step
```
 2  y = GenShortVector(rho_1, r, w_y)                          # regenerate y from sk
 3-4 parse u || v = c_PKE ; unpack
 5  m = C.Decode( v - trunc(y*u, L) )
```
`C.Decode` = duplicated-RM decode then shortened-RS decode:
```
reed_muller_decode: for each of the n1 blocks, expand-and-sum the `mult` copies,
    Hadamard transform, take the coefficient of LARGEST absolute value
    (find_peaks: peak_abs_value = max), sign bit -> the 8th message bit
reed_solomon_decode: syndromes -> Berlekamp-Massey error locator -> additive-FFT
    root search -> Forney error values -> correct; corrects up to
    delta = (n1 - k1)/2 symbol errors
```
The DFR bound of §2.3 composes the inner RM block-error probability `p_in` with
Proposition 5's tail `sum_{j>delta} C(n1,j) p_in^j (1-p_in)^{n1-j}`.

### KEM (Figs 4–6)
```
KeyGen((rho, rho_k)):
    (pk_PKE, sk_PKE) = PKE.KeyGen(rho)
    pk = pk_PKE ;  sk = sk_PKE || pk || rho_k

Encaps(pk, m, sigma):                 # m in B^{lambda/8}, salt sigma in B^{lambda/8}
    (k, rho_c) = G( m || sigma || H(pk) )
    c_PKE      = PKE.Enc(pk, m, rho_c)
    c          = c_PKE || sigma
    return (k, c)

Decaps(sk, c):
    parse sk_PKE || pk || rho_k = sk ;   parse c_PKE || sigma = c
    m'      = PKE.Dec(sk_PKE, c_PKE)
    (k',c') = Encaps(pk, m', sigma)                 # deterministic re-encapsulation
    k*      = J( rho_k || c || H(pk) )
    return  c' == c  ?  k'  :  k*                   # implicit rejection
```
All of `PRG, H, G, J, XOF` are domain-separated by **appending a distinct tag
byte** to the input (§1.5). Spec Table 3 gives two instantiations per level;
the built reference is the **SM3** one, and the spec marks with `[⋆]` the cells
where the chosen primitive admittedly does not reach the stated security level
(SM3-512/768/1024 and SM3-XOF at QUBE-384/512, SM3-512 at QUBE-192/256).

## Implementation vs specification

Built tree: `Implementations/Reference_Implementation/qube-{128,192,256,384,512}`,
sources `src/common/{code,crypto_memset,fft,kem,kem_qube,symmetric}.c`,
`src/ref/{gf,gf2x,parsing,qube,reed_muller,reed_solomon,vector}.c`,
`lib/api_pkc/{auxfunc,drng}.c`. API header `src/common/kem_qube.h`
(`ALGORITHM_INSTANCE "qube"`, to which `KAT_KEM.c` appends `_<PARAM_SECURITY>`).

Agreements (reference tree only):

- `src/ref/parameters.h` reproduces spec Table 1 and Table 2 **exactly** for all
  five sets: `PARAM_N` = r, `PARAM_N1`/`PARAM_N2`/`PARAM_N1N2` = the RS length /
  RM length / L, `PARAM_OMEGA_X1/X2/Y1/R11/R21/R22/E` =
  `w_x, w_x, w_y, w_g, w_f, w_f, w_e`. All five checked.
- The size macros are the spec's formulas verbatim:
  `PUBLIC_KEY_BYTES = SEED_BYTES + 2*VEC_N_SIZE_BYTES`,
  `SECRET_KEY_BYTES = SEED_BYTES + PUBLIC_KEY_BYTES + SEED_BYTES`,
  `CIPHERTEXT_BYTES = VEC_N_SIZE_BYTES + VEC_N1N2_SIZE_BYTES + SALT_BYTES`,
  with `SEED_BYTES = SALT_BYTES = SHARED_SECRET_BYTES = PARAM_SECURITY_BYTES = λ/8`.
- `src/common/kem.c` implements Figures 4–6 line for line, including
  re-encapsulating with the *received* salt (`c_kem.salt`, `kem.c:160`) and a
  **constant-time** select `vect_select(ss, ss_prime, ss_fallback, …, mismatch)`
  (`kem.c:174-175`) — the condition is not inverted and there is no early return
  on mismatch. `qube_pke_decrypt` never reports a decoding failure, so the
  implicit-rejection path is always reached through the ciphertext comparison.
- Domain separation is implemented as the spec describes: `symmetric.c:15-19`
  defines `QUBE_DOMAIN_{PRG,H,G,J,XOF} = 0x01..0x05` and `append_domain` puts the
  tag byte **after** the message (`buf[msg_len] = domain`).
- All randomness comes from the official DRNG: `kem.c:25-27`
  `get_random_number(&drng_algorithm, out, 8*out_len)`; `rho`, `rho_k`, `m` and
  the salt are the only draws.
- The RM decoder takes the **maximum** |Hadamard coefficient| (`find_peaks`,
  `reed_muller.c:147-162`), which is the correct sense; the RS decoder is a
  standard BM + additive-FFT root search + Forney correction, capacity
  `δ = (n1 − k1)/2`.
- Secrets are zeroised (`memset_zero`) on every exit path in `kem.c` and
  `qube.c`.

Discrepancies:

- **(a) The submitted `Test_Vectors/` were not generated by the designated
  reference implementation.** The reference tree reproduces its own in-tree
  `qube-*/KAT/KAT_KEM_qube_*.txt` byte for byte (verified), and those agree with
  spec Table 5 and with the built library. The package-level `Test_Vectors/`
  differ at four of five levels (SHA-256 mismatch, recorded by `make -C kem-33 test`) and
  carry the sizes of `Optimized_Implementation/src/x86/qube-{1,3,4,5}`, as
  derived arithmetically above. A verifier who checks the submission's own test
  vectors against the submission's own reference code gets four MISMATCHes.
- **(a) No test vectors exist for QUBE-192.** The spec specifies the set
  (Tables 1, 2, 5, 6, 7 all have a `*QUBE-192` column) and the reference tree
  builds and self-tests it, but `Test_Vectors/KAT_KEM_qube_192.txt` is absent.
  The optimized tree has no 192 directory at all (`qube-1,3,4,5,6,7,8`, no
  `qube-2`), which is why.
- **(a) Wrong shared-secret length in the submitted vectors.** `|k| = λ/8` per
  §1.6, i.e. 16 bytes at QUBE-128 and 48 at QUBE-384. The submitted vectors say
  `SS_Len = 32` and `SS_Len = 64`, because the optimized `api.h` hard-codes
  `CRYPTO_BYTES 32`/`64` rather than deriving it from `PARAM_SECURITY_BYTES`.
  The reference is correct (16/24/32/48/64).
- **(a) The optimized tree uses a different concatenated code at 256/384/512.**
  `PARAM_N1N2` = 42240 / 86016 / 145408 against spec Table 2's
  `L = 41472 / 81920 / 134400`. `L` is an input to the DFR computation of §2.3
  and to the CT-problem dimension, so the optimized build is a different
  parameter set, and the Table 7 DFR figures do not describe it. It also has
  three further parameter sets (`qube-6/7/8`, r = 42283 / 86027 / 145451, with
  different weights, e.g. `w_x1 = w_x2 = 161, w_y = 128` at level 256) that
  appear nowhere in the specification.
- **(c) Self-declared placeholder primitives.** Spec Table 3 marks with `[⋆]`
  that `SM3-512`, `SM3-768`, `SM3-1024` and `SM3-XOF` "technically do not meet
  the corresponding requirements about security levels" at QUBE-192/256/384/512.
  The reference `symmetric.c:44-56` implements exactly those (`sm3hash(256,…)`
  for 32-byte outputs, `pseudohash(512|768|1024, …)` otherwise, `pseudoXOF` for
  the XOF). This is an honest disclosure in the spec, not a hidden defect, but it
  means QUBE-384 and QUBE-512 cannot reach their nominal levels with the
  symmetric primitives as instantiated.
- **(c)** The five reference instance directories are separate copies of the
  same sources differing only in `src/ref/parameters.h`; `qube-128`'s
  `parameters.h` additionally carries
  `typedef char qube_ss_len_check[(SHARED_SECRET_BYTES == 16) ? 1 : -1];`, a
  per-level compile-time assertion that would have caught the optimized tree's
  `CRYPTO_BYTES 32` had the same guard been present there.

Not verified: the Table 6 ISD/combinatorial-gap estimates, the Table 7 DFR
computation, the GF(2^8) / additive-FFT arithmetic beyond KAT reproduction, and
the optimized implementation's own correctness (it was not built).
