# kem-06 BRA — algorithm summary

BRA ("Blockwise RQC with Augmented Gabidulin codes") is a **rank-metric code-based**
KEM: an IND-CPA PKE in the RQC/ideal-code style over the Ideal Blockwise Rank Syndrome
Decoding (IBRSD) problem, lifted to IND-CCA2 by the **salted Fujisaki–Okamoto
transform** (implicit rejection). Encryption encodes the message with an **Augmented
Gabidulin (AG) `[n,k]_{q^m}` code**; decryption strips the ideal-code mask and runs a
**Welch–Berlekamp-like (WBL) q-polynomial reconstruction decoder**.

Specification: `kem-06-spec.pdf` (35 pages, **English**; `中文资料/` holds only the
Chinese basic-information and IP declaration, not the algorithm spec). §3.2 Alg. 1
(decoder), §3.3 Alg. 2–5 (PKE), §3.4 Alg. 6–9 (KEM); parameters §4, Tables 2–3.

## Parameters

| parameter | BRA-128 | BRA-256 | BRA-512 | meaning |
|---|---|---|---|---|
| q | 2 | 2 | 2 | base field |
| m | 67 | 83 | 127 | extension degree, `F_{q^m}` |
| n | 121 | 161 | 221 | code length / ring degree |
| k | 4 | 4 | 5 | AG code dimension |
| t | 67 | 83 | 127 | rank weight of generator `g` (= m) |
| w_x, w_y | 4, 5 | 5, 5 | 6, 6 | secret-key rank weights |
| w_r1, w_r2, w_e | 5, 5, 8 | 6, 6, 8 | 7, 8, 8 | encryption randomness weights |
| r | 53 | 68 | 98 | error rank to decode, `r = w_x·w_r2 + w_y·w_r1 + w_e` |
| decode bound | 58 | 78 | 108 | `min((n−k)/2, t−k)` ≥ r (Def. 3.4) |
| P(X) | X^121+X^18+1 | X^161+X^18+1 | X^221+X^8+X^6+X^2+1 | ring modulus |
| DFR | 2^−130 | 2^−262 | 2^−523 | Thm. 3.1 |
| claimed security | 128 (2^189 MM) | 256 (2^304) | 512 (2^556) | bits, spec Table 3 |

Sizes (bytes), specification Table 3 vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | ct spec | ct impl | ss | match |
|---|---|---|---|---|---|---|---|---|
| BRA-128 | 1078 | 1078 | 98 | **1176** | 2092 | 2092 | 64 | pk/ct/ss yes, **sk no** |
| BRA-256 | 1735 | 1735 | 106 | **1841** | 3406 | 3406 | 64 | pk/ct/ss yes, **sk no** |
| BRA-512 | 3573 | 3573 | 144 | **3717** | 7082 | 7082 | 64 | pk/ct/ss yes, **sk no** |

Spec formulae `pks = ceil(mn/8)+64`, `cts = 2·ceil(mn/8)+64`, `sks = ceil(mk/8)+64`
reproduce the impl pk/ct exactly. The sk gap is exactly `+pks`: the implementation
stores `sk = sk_seed(64) ‖ sigma(ceil(mk/8)) ‖ pk`, i.e. the spec's `sks` **plus a
cached copy of the public key** (`src/parsing.c:33-37`). Deliberate, but spec Table 3's
sk column does not describe the shipped format.

## Pseudocode

### KeyGen (spec Alg. 3 + Alg. 7)
```
g ←θ1 S_t^n ;  h ←θ1 F_{q^m}^n          # g = rank-t generator of the AG code
(x,y) ←θ2 S_{(w_x,w_y)}^{(n,n)}         # supports of x,y form a direct sum
s = x + h·y                             # in R = F_{q^m}[X]/<P(X)>
σ ←$ F_{q^m}^k
return pk = (g,h,s),  sk = ((x,y), σ)
```

### Encaps (spec Alg. 8, calling Alg. 4)
```
m ←$ F_{q^m}^k ;  salt ←$ {0,1}^512 ;  θ = G(pk, m, salt)
                                        # PKE.Enc(pk, m; θ):
  G_AG = k×n generator matrix of AG_{k,t}^g  (rows g, g^q, ..., g^{q^{k-1}})
  (r1, e, r2) ←θ S_{(w_r1,w_e,w_r2)}^{(n,n,n)}
  u = r1 + h·r2 ;  v = m·G_AG + s·r2 + e
ct = (u, v, salt) ;  K = H(pk, m, u, v, salt)      # 64 bytes
```

### Decaps (spec Alg. 9, calling Alg. 5 and the rank-metric decoder Alg. 1)
```
parse sk = ((x,y), σ, pk);  parse ct = (u, v, salt)
z = v − u·y = m·G_AG + (x·r2 + e − r1·y)    # Alg. 5 step 1; residual error rank
                                            #  ||x·r2+e−r1·y||_R ≤ r = w_x w_r2+w_y w_r1+w_e
--- RANK-METRIC DECODING: AG.Decode(g, z), spec §3.2 Alg. 1 (WBL) -----------
 # radius min((n−k)/2, t−k);  DFR ≤ γ_q·q^{a(t+r−a−n)}, a = t−k−r+1 (Thm. 3.1)
 1. Solve LR(g, z) (Def. 3.7) by the Welch–Berlekamp-like algorithm on q-polys:
      init:  A = annihilator q-poly of g_1..g_k ;  I = interpolator of z_1..z_k
             V0 = 0, V1 = X ;  discrepancies u0[i]=A(g_i)−V0(z_i), u1[i]=I(g_i)−V1(z_i)
      for i = k..n−1: pick next non-zero discrepancy; e1 = −u1[i]^q/u1[i],
             e2 = −u0[i]/u1[i];  (N0,V0) ← (N1^q−e1·N1, V1^q−e1·V1);
                                 (N1,V1) ← (N0−e2·N1,  V0−e2·V1)
      giving deg_q V ≤ (n−k)/2 and deg_q N ≤ k+(n−k)/2−1
 2. f(X) = V \ (N ◦ A) + I                 # symbolic left division (Loidreau's trick)
 3. Alg. 1 lines 5-8: if deg_q f ≤ k−1 and ||z − f(g)||_R ≤ r → (f, e := z−f(g))
                      else → ⊥
 4. m = first k coefficients of f
----------------------------------------------------------------------------
(u',v') = PKE.Enc(pk, m; G(pk, m, salt))          # re-encryption
if m = ⊥ or (u',v') ≠ (u,v) :  K = H(pk, σ, u, v, salt)   # implicit rejection
else                        :  K = H(pk, m, u, v, salt)
```
Hashes: `G` and `H` are the NGCC `pseudohash` (SM3 / HMAC-SM3), 512-bit output (§3.1);
`G` seeds the encryption sampler, `H` outputs the shared secret. Randomness is the NGCC
SM3-DRBG (55-byte state) with a 64-byte-seeded seedexpander.

## Implementation vs specification

Checked: `src/BRA-*/src/{kem.c,bra.c,augmented_gabidulin.c,qpoly.c,parsing.c,parameters.h}`
plus `src/rbc-{67,83,127}/`, as wired by `kem-06/Makefile`. The BRA-128 decoder
fault below is the runtime check `kem-06-1` (`tools/reproduce.sh`). BRA-256 and
BRA-512 are not built here.

**Parameter spot-check** (sampled: q,m,n,k,w_x,w_y,w_r1,w_r2,w_e plus the four size
macros, for all three instances): `parameters.h` matches spec Table 2 exactly, and
`r = w_x·w_r2 + w_y·w_r1 + w_e` reproduces Table 2's r (53/68/98) in all three cases,
each within `min((n−k)/2, t−k)`. `BRA_SALT_BYTES 64` matches §3.1's 512-bit salt;
`BRA_SHARED_SECRET_BYTES 64` matches `|K|`.

Agreements: the salted FO transform is complete — `kem.c:222` binds
`G = pseudohash(pk ‖ m ‖ salt)`, `kem.c:268` binds `H = pseudohash(pk ‖ m|σ ‖ u ‖ v ‖ salt)`
(pk- and ciphertext-binding as in Alg. 8/9); the re-encryption compare (`kem.c:249-252`)
is a non-short-circuiting OR-accumulator and the m/σ selection (`kem.c:257-259`) a
branchless XOR mask, matching the §5 "Constant time" claim.

Discrepancies:

- **(a) deviation — the decoder never signals failure.**
  `rbc_augmented_gabidulin_decode()` returns `void`
  (`src/BRA-128/src/augmented_gabidulin.h:29`, body `augmented_gabidulin.c:94-312`).
  Spec Alg. 1 lines 5–8 require checking `deg_q f ≤ k−1` **and** `‖y − f(g)‖_R ≤ r`
  and returning `⊥`; neither check exists in the code. `m` is taken unconditionally as
  `qpoly->values[0..k-1]` (`augmented_gabidulin.c:283`). Decaps therefore relies solely
  on the FO re-encryption comparison, which is sound for the KEM but is not Alg. 1.
- **(a/c) deviation — decoding radius.** The code uses `t = (n−k)/2`
  (`augmented_gabidulin.c:100`) as the reconstruction radius and never materialises the
  spec's `r` or the `t−k` branch of `min((n−k)/2, t−k)`. There is no `BRA_PARAM_R` or
  `BRA_PARAM_T` macro anywhere in `parameters.h`. For all three parameter sets
  `(n−k)/2 ≤ t−k`, so the radius is numerically correct, but the AG-specific bound is
  unverifiable from the constants alone.
- **(a) known crash — unchecked degree in the decoder's q-polynomial division.**
  `rbc_qpoly_left_div2()` (`src/BRA-128/src/qpoly.c:556-609`) sets `int i = k-1;` and
  does `i--` once per iteration of `while(rtmp->degree >= b->degree)` with **no floor**,
  passing `i` to `rbc_qpoly_mul2(t, b, s, capacity, i)` whose parameter is
  **`uint32_t p2_degree`** (`qpoly.c:449`). `mul2`'s `for(j = 0; j <= p2_degree; ++j)`
  writes `o->values[j]` (`qpoly.c:460`) with no comparison against `o->max_degree`.
  The only guard (`qpoly.c:450`) tests `o->max_degree < p1->degree + p2->degree` using
  the polynomials' *actual* degrees, while the loops are driven by the *passed* degree
  arguments. The inner composition also stores at `o->values[(i+j) % RBC_<m>_FIELD_M]`
  (`qpoly.c:464`), so a modest degree already reaches index `m−1` (66/82/126) in an
  array whose `max_degree+1` can be as small as `t+1`.
  Confirmed for BRA-128 at the harness default `-O2`: honest keygen, encapsulation
  and decapsulation succeed (the KAT passes), then `sk[0] ^= 0xff` and decapsulation
  of that ciphertext faults in `rbc_qpoly_mul2`'s store to `o->values[j]`, called from
  `rbc_qpoly_left_div2` (`kem-decoder-fault`). In a fresh process, an all-zero
  ciphertext and single flipped ciphertext bytes return a shared secret at `-O2`.
  At `-O0` the same all-zero ciphertext aborts after the honest decapsulation
  (signal 6, six of six runs). A one-bit change of `ct[0]` still returns.
- **(b) cosmetic.** `parameters.h` names the 64-byte hash output `SHA512_BYTES` with a
  "SHA2_512 and SHA3_512" comment and the build links XKCP Keccak, while §3.1 specifies
  SM3/HMAC-SM3 `pseudohash`. Call sites do use `pseudohash()` (`kem.c:126,141,222,268`),
  so this is leftover RQC naming, not a functional deviation.

Not verified: the rbc field/ideal-ring arithmetic was only skimmed; the DFR of Thm. 3.1,
the constant-time property of the decoder itself (the WBL loop is masked, but
`rbc_elt_inv`/`rbc_elt_nth_root` were not audited), and §7's security estimates were not
independently checked.
