# sign-13 GreatWall — algorithm summary

VOLE-in-the-Head (VOLEitH) signature: a Fiat–Shamir-compiled NIZKPoK of a preimage
of a public value under **Pylon**, a dedicated one-way function over F_{2^n}
(inverse-Mersenne S-boxes + dense linearized-polynomial linear layers, nested
feed-forward). Security rests only on preimage resistance of Pylon plus soundness
of the VOLEitH/QuickSilver proof system (no structured algebraic assumption);
EUF-CMA in the ROM/QROM. Construction and code are FAEST-derived
(faest-one-tree, single interleaved GGM tree + grinding).

Specification: `sign-13-spec.pdf` (63 pages, English), §2.2 (Pylon), §2.3
(scheme), Algorithms 1–19. Extra: `Algorithm specifications/Clarification4GreatWall.pdf`.
The `中文资料/` directory contains only signed admin/IP forms, not the spec.

## Parameters

Pylon (spec Table 2) — one parameter set per variant, shared by the S and F profiles:

| parameter | GW-128 | GW-192 | GW-256 | GW-512 | meaning |
|---|---|---|---|---|---|
| λ | 128 | 192 | 256 | 512 | security parameter |
| n | 137 | 197 | 263 | 521 | field degree, F_{2^n} |
| (e1,e2) | (70,75) | (94,105) | (129,136) | (248,273) | S-box exponents, S(x)=x^{(2^e−1)^{−1}} |
| f(X) | X^137+X^21+1 | X^197+X^21+X^2+X+1 | X^263+X^93+1 | X^521+X^32+1 | irreducible poly |
| r | 2 | 2 | 2 | 2 | Pylon rounds (2 S-boxes) |
| ℓ = witness bits | 288 | 400 | 528 | 1056 | w=(pt,x1), each ⌈n/8⌉ bytes |
| NGCC level (spec Table 3) | 1 | 1 | 3 | 5 | superseded by the clarification: — / 1 / 2 / 3 |

VOLEitH profiles (spec Table 10; k0,k1,τ0 derived from Alg. 18 with n_ch = λ − w_grind):

| instance | τ | w_grind | T_open | k0 | k1 | τ0 | ℓ̂ = ℓ+2λ+B bits |
|---|---|---|---|---|---|---|---|
| 128s | 11 | 7 | 100 | 11 | 11 | 0 | 560 |
| 128f | 16 | 8 | 108 | 8 | 7 | 8 | 560 |
| 192s | 16 | 8 | 183 | 12 | 11 | 8 | 800 |
| 192f | 24 | 8 | 184 | 8 | 7 | 16 | 800 |
| 256s | 22 | 6 | 246 | 12 | 11 | 8 | 1056 |
| 256f | 32 | 7 | 248 | 8 | 7 | 25 | 1056 |
| 512s | 44 | 0 | 512 | 12 | 11 | 28 | 2096 |
| 512f | 64 | 0 | 512 | 8 | 8 | 0 | 2096 |

B = 16 (VOLE-consistency hash is λ+B bits). PRG = AES-λ-CTR (§2.3.4); for λ = 512
the code has no AES and instead computes SHAKE256(K ‖ (iv ⊕ ctr) ‖ 0x00)
(`GreatWall512s/prgs.h:6-54`, `hash.h:95-130`), which §2.3.4 does not mention.
H0,H1,H2^j,H3 = SHAKE128 (λ=128) / SHAKE256 (otherwise) with a 1-byte domain
separator (§2.3.4).

Sizes (bytes). The spec gives no closed-form size formula, only Table 10; the
"spec" columns are Table 10 (pk, sig) and Alg. 1 (`sk := pt`, i.e. ⌈n/8⌉ bytes).

| instance | pk spec | pk impl | sk spec | sk impl | sig spec | sig impl | match |
|---|---|---|---|---|---|---|---|
| 128s | 36 | 36 | 18 | 36 | 2758 | 2758 | pk/sig yes, **sk no** |
| 128f | 36 | 36 | 18 | 36 | 3396 | 3396 | pk/sig yes, **sk no** |
| 192s | 50 | 50 | 25 | 50 | 6804 | 6804 | pk/sig yes, **sk no** |
| 192f | 50 | 50 | 25 | 50 | 8012 | 8012 | pk/sig yes, **sk no** |
| 256s | 66 | 66 | 33 | 66 | 12236 | 12236 | pk/sig yes, **sk no** |
| 256f | 66 | 66 | 33 | 66 | 14260 | 14260 | pk/sig yes, **sk no** |
| 512s | 132 | 132 | 66 | 132 | 50012 | 50012 | pk/sig yes, **sk no** |
| 512f | 132 | 132 | 66 | 132 | 57812 | 57812 | pk/sig yes, **sk no** |

pk = ct‖iv_owf = 2·⌈n/8⌉ (18,25,33,66 → 36,50,66,132), matching Table 10 exactly.
The implementation's sk = pt‖iv_owf is twice the spec's `sk := pt`; see below.

## Pseudocode

### KeyGen (spec Alg. 1, Pylon in Eq. (2) / §2.2)

```
repeat
    pt  <-$ GF(2^n);  iv_owf <-$ {0,1}^n
    a1 := SHAKE(iv_owf||0);  a2 := SHAKE(iv_owf||1)
    (ct,x1,x2) := Pylon(iv_owf, pt), where           # Eq. (2)
        x1 = S1(M1(pt) + g1)                         # S_i(x) = x^{(2^{e_i}-1)^{-1} mod 2^n-1}
        x2 = S2(M2(x1+pt) + iv_owf)
        ct = M3(x2) + M2(x1+pt) + iv_owf + pt + g2
until <a1,x1> != 0 and <a2,x2> != 0                  # non-degeneracy for the dot constraints
return pk := (iv_owf, ct), sk := pt
```

### Sign (spec Alg. 2, with Alg. 3–11)

```
 1 ctr := 0; rho <-$ {0,1}^lambda
 2 mu   := H1(ct || m ; 2*lambda)
 3 (r, iv_vole) := H3(pt || mu || rho)                       # r: lambda bits, iv_vole: 128 bits
 4 (com,decom,c_1..c_{tau-1},u,V) := VOLECommit(r, iv_vole, l+2*lambda+B)   # Alg.3: BAVC.Commit
 5                                                            #  (Alg.4, one interleaved GGM tree)
 6                                                            #  + ConvertToVOLE (Alg.5), c_i = u xor u_i
 7 chall1 := H2^1(mu||com||c_1..c_{tau-1}||iv_vole ; 5*lambda+64)
 8 u~ := VOLEHash(chall1,u); V~ := VOLEHash(chall1,V)         # Alg.6, universal hash to lambda+B bits
 9 hV := H1(V~)
10 (ct,x1,x2) := Pylon(iv_owf,pt);  w := (pt, x1)             # extended witness, l bits
11 d  := w xor u[0..l)
12 chall2 := H2^2(chall1 || u~ || hV || d ; 3*lambda+64)
13 u := u[0..l+lambda); V := V[0..l+lambda)
14 (a~, b~) := ZK.OWFProve(w,u,V,ct,iv_owf,chall2)            # Alg.7
15 ctr := -1
16 repeat                                                     # grinding, Alg.2 l.22-30
17     ctr := ctr+1
18     chall3 := H2^3(chall2 || a~ || b~ || ctr ; lambda)
19     if chall3[lambda-w_grind : lambda] != 0^{w_grind}: continue
20     I := DecodeAllChall3(chall3[0 : lambda-w_grind])        # Alg.19 / ChalDec Alg.18
21     decom_I := BAVC.Open(decom, I)                          # Alg.12, aborts if |S| > T_open
22 until decom_I != bot
23 return sigma := (c_1..c_{tau-1}, u~, d, a~, decom_I, chall3, iv_vole, ctr)
```

`ZK.OWFProve` (Alg. 7–11): LinearTransform (Alg. 8) recomputes the wire values
s1 = M1·pt⊕g1, y1 = x1, z1 = y1^{2^{e1}}, s2 = M2·(y1⊕pt)⊕iv_owf,
y2 = M3^{-1}(ct⊕s2⊕g2⊕pt), z2 = y2^{2^{e2}}; two MulCstrnts (Alg. 9, s_j·y_j = z_j
reduced mod f(X) — this is the inverse-Mersenne relation) and two DotCstrnts
(Alg. 10, <a_i^dot, x_i> ≠ 0) give (a0,a1), compressed by ZKHash (Alg. 11).

### Verify (spec Alg. 13, with Alg. 14–15)

```
 1 parse sigma; (iv_owf,ct) <- pk; mu := H1(ct||m)
 2 if chall3[lambda-w_grind:lambda] != 0^{w_grind}: reject
 3 (com,Q') := VOLEReconstruct(decom_I, chall3[0:lambda-w_grind], c_1..c_{tau-1}, iv_vole)  # Alg.14
 4 if reconstruction failed: reject
 5 chall1 := H2^1(mu||com||c_1..c_{tau-1}||iv_vole ; 5*lambda+64)
 6 for i in [0..tau): (d_0..d_{k_b-1}) := ChalDec(chall3,i); D~_i := [d_j * u~]
 7                    Q_i := Q'_i xor [d_j * c_i]   (i>0)                 # undo the corrections
 8 Q~ := VOLEHash(chall1, Q);  hV := H1(Q~ xor [D~_0 .. D~_{tau-1}])
 9 chall2 := H2^2(chall1 || u~ || hV || d ; 3*lambda+64)
10 b~ := ZK.OWFVerify(d, Q[0..l+lambda), chall2, chall3, a~, ct, iv_owf)   # Alg.15
11 accept iff H2^3(chall2 || a~ || b~ || ctr ; lambda) == chall3
```

Signature layout (derived from Alg. 2 l.32; the spec states no formula):
`(τ−1)·ℓ̂/8 + (λ+B)/8 + ℓ/8 + λ/8 + (2τ+T_open)·λ/8 + λ/8 + 16 + 4` bytes.
This reproduces all eight OBSERVED sizes exactly (checked numerically).

## Implementation vs specification

Checked (reference implementation, `src/GreatWall*` → `Implementations/Reference_Implementation`):
`api.c`/`faest.c` (KeyGen/Sign/Verify skeleton, key packing), `vole_commit.c`,
`vector_com.c` (BAVC/GGM + grinding), `vole_check.c`, `quicksilver.c`,
`owf_proof.c` (Pylon relation), `greatwall-<lvl>-matrix.c` (constants),
`faest.h`/`config.h`/`owf_proof.h`/`vole_params.h` (sizes).

Agreements (sampling: 6 constants per instance × all 8 instances):
- `config.h` `BITS_PER_WITNESS` / `ZERO_BITS_IN_CHALLENGE_3` /
  `BATCH_VECTOR_OPENING_SEEDS_TRESHOLD` equal spec Table 10's (τ, w_grind, T_open)
  for **all eight** instances; also encoded in `CRYPTO_ALGNAME`
  (e.g. `sec128_gwccs_11_7_100`).
- `faest.h:13-21` `GREATWALL_SECRET_KEY_BITS` = 137/197/263/521 = spec Table 2's n.
- `greatwall.h` matrix/constant symbols carry exactly Table 2's exponents:
  `pow_mat_137_70/75`, `197_94/105`, `263_129/136`, `521_248/273`.
- `owf_proof.c:222-227` reduces with X^137 = X^21 + 1 — spec Table 2's f(X) for n=137.
- `owf_proof.c:531-662` implements Alg. 8 (M1, M2, M3^{-1}, Frobenius powers) plus
  2 MulCstrnts + 2 DotCstrnts with α_i = SHAKE(iv_owf‖i) — matches Alg. 7 l.7-12.
- `FAEST_SIGNATURE_BYTES` (`faest.h:35`) expands to the layout above and to the
  observed sizes for all eight instances; no signature-size discrepancy.

Discrepancies:
1. **(a) Secret-key size.** Spec Alg. 1 sets `sk := pt` (⌈n/8⌉ = 18/25/33/66 bytes);
   the implementation stores `pt‖iv_owf` (`faest.c:57-79`, `GREATWALL_SECRET_KEY_BYTES`
   in `faest.h:26`) = 36/50/66/132 bytes, i.e. exactly 2× the spec value.
   Functionally necessary for the NIST `crypto_sign(…, sk)` API (iv_owf is needed to
   sign and is not otherwise recoverable), but the spec never says so and gives no
   sk size in Table 10. Spec text should be corrected.
2. **(b) μ binds the whole public key.** Spec Alg. 2 l.5 / Alg. 13 l.3 write
   μ = H1(ct‖m); `faest.c:352` hashes the full packed pk = ct‖iv_owf. Harmless
   (iv_owf is bound anyway through the OWF constraints) but a notation/spec gap.
3. **(b) Undocumented outer restart counter.** `faest.c:369-379` feeds an 8-byte
   `attempt_num` into H3 alongside sk‖μ‖ρ; spec Alg. 2 l.6 has only H3(pt‖μ‖ρ).
   This is the outer re-randomisation loop (`faest.c:525-531`) when the inner
   grinding loop exhausts its 32-bit counter; the spec does not describe it.
4. **(b) k0/k1 definition inconsistent inside the spec.** §2.3 ("Small-field
   instantiations") defines k0 = ⌈λ/τ⌉, k1 = ⌊λ/τ⌋, τ0 = λ mod τ, but Alg. 18
   (ChalDec) uses n_ch = λ − w_grind. Alg. 14 l.9 (`pos != λ − w_grind`) and the
   implementation follow Alg. 18; the §2.3 text is wrong whenever w_grind > 0.
5. **(b) Security-level table.** Spec Table 3 maps 128→L1, 192→L1, 256→L3, 512→L5;
   `Clarification4GreatWall.pdf` remaps to 192→L1, 256→L2, 512→L3 with 128 an
   "additional variant". The two documents disagree; the clarification is newer.

Not verified (time-boxed): the round constants / matrices in
`greatwall-<lvl>-matrix.c` were not regenerated from SHAKE256("GreatWall-N") as
§2.2 prescribes, and the density/permutation rejection conditions on L and L^{-1}
were not re-checked. Constant-time behaviour and the KAT vectors are outside this
file's scope (KATs: 8/8 PASS per `security_findings.md`).
