# sign-25 SQIsign2D2 — algorithm summary

Isogeny-based signature: a 3-round Σ-protocol between supersingular elliptic curves over
F_p2 (hardness: supersingular endomorphism-ring computation / isogeny finding), made
non-interactive by the Fiat–Shamir transform (spec §3.1.2). The response isogeny is verified
by reconstructing a single (2^a, 2^a)-isogeny of principally polarised abelian surfaces
(dimension-2 / theta coordinates). Two signature encodings ship: *uncompressed* (§3.4) and
*compact/compressed* (§3.5), selected by `COMPRESSED` in `config.h`.

Specification: `sign-25-spec.pdf` (117 pages), Chapter 3 (Algorithms 3.1–3.6) and Chapter 4
(parameters, Algorithm 4.1). Reference code needs GMP (`-lgmp`).

## Parameters

Prime is always `p = f · 2^a · 3^b − 1` with `2^a ≈ 3^b ≈ √p`, `p ≡ 3 (mod 4)` (spec §3.2.1,
§4.1). Challenge space is `[3^b]`; `HASH` is built from SM3 by Algorithm 4.1 (spec §4.2).

Sizes are in bytes, written `uncompressed / compressed` (the two builds per directory).
**Spec Table 6.1 and the built libraries agree on all 48 size values** — `match` = all six
numbers of that row agree with `OBSERVED/sign-25.txt`. Claimed strengths (§4.2) are
128/160/256/512 classical and 64/80/128/256 quantum for Level1/2/3/5.

| set (§4.2) | λ | f | a | b | HASH instantiation | pk | sk | sig | match |
|---|---|---|---|---|---|---|---|---|---|
| Level1-eff | 128 | 1 | 131 | 78 | SM3-512_124 ∘ SM3-512^∘31 | 64/65 | 488/240 | 200/168 | yes |
| Level1-sec | 128 | 1 | 137 | 84 | SM3-512_134 | 68/69 | 521/257 | 212/178 | yes |
| Level2-eff | 160 | 61 | 161 | 96 | SM3-512_153 ∘ SM3-512^∘255 | 80/81 | 610/298 | 248/208 | yes |
| Level2-sec | 160 | 11 | 168 | 101 | SM3-512_161 | 84/85 | 643/315 | 260/218 | yes |
| Level3-eff | 256 | 1 | 263 | 156 | SM3-512_248 ∘ SM3-512^∘511 | 128/129 | 976/468 | 392/326 | yes |
| Level3-sec | 256 | 1 | 264 | 163 | SM3-512_259 | 132/133 | 1009/489 | 404/338 | yes |
| Level5-eff | 512 | 11 | 514 | 319 | SM3-1024_506 ∘ SM3-1024^∘127 | 256/257 | 1952/936 | 776/648 | yes |
| Level5-sec | 512 | 17 | 536 | 334 | SM3-1024_530 | 268/269 | 2046/982 | 812/678 | yes |

## Pseudocode

Lines marked `(C)` are the compact-variant differences (Algorithms 3.4/3.5/3.6).

### KeyGen — spec Algorithm 3.1 (uncompressed) / 3.4 (compact)
```
repeat sample prime N_tau < p^(1/4) until legendre(3,N_tau) = -1
(E_A, tau(P0),tau(Q0),tau(R0),tau(S0), I_tau)
     <- ImRanIso_{O_0}(N_tau, 3^b, 2^a, 2^floor(a/2), {P0,Q0,R0,S0})
pk <- (E_A);   sk <- (E_A, I_tau, N_tau, tau(P0),tau(Q0),tau(R0),tau(S0))
(C) P_A,Q_A,hint2 <- TorsionBasisToHint2(E_A,a); R_A,S_A,hint3 <- TorsionBasisToHint3(E_A,b)
    M_sk,2 <- ChangeOfBasis2^a(E_A,(tau(P0),tau(Q0)),(P_A,Q_A)); M_sk,3 likewise on 3^b
    pk <- (E_A,hint2,hint3);  sk <- (E_A,hint2,hint3,I_tau,N_tau,M_sk,2,M_sk,3)
```

### Sign — spec Algorithm 3.2 (uncompressed) / 3.5 (compact)
```
// commitment (§3.4.1)
repeat sample prime N_psi < sqrt(p) until gcd(N_psi,6)=1
(E_com, psi(P0..S0), I_psi) <- ImRanIso_{O_0}(N_psi, 3^b, 2^a, 1, {P0,Q0,R0,S0})
R_com,S_com,hint3,com <- TorsionBasisToHint3(E_com, b)
// challenge (§3.4.2)
chl <- HASH( pk || j(E_com) || msg )                                   // Alg. 3.2 line 12
// response (§3.4.3)
E_chl,{phi(psi(P0)),phi(psi(Q0))} <- ThreeIsogenyChain(R_com+[chl]S_com, E_com, b, ...)
P_chl,Q_chl,hint2,chl <- TorsionBasisToHint2(E_chl, a)
M_com,3 <- ChangeOfBasis3^b(E_com,(psi(R0),psi(S0)),(R_com,S_com)); [c0,c1]^T <- M_com,3[1,chl]^T
I'_phi <- KernelDecomposedToIdeal(c0,c1);   J <- conj(I_tau)(I_psi ∩ I'_phi)
repeat alpha <- RandomEquivalentQuaternion(J); q <- Nrd(alpha)/Nrd(J)
       until (q odd and q <= 2^a)                                      // Alg. 3.2 lines 19-23
I_sigma <- J·conj(alpha)/Nrd(J);  d <- 2^a - q
(sigma(tau(P0)),sigma(tau(Q0))) <- [(3^b N_psi)^-1](phi(psi(P0)),phi(psi(Q0)))·M_conj(alpha)
(E_aux, phi_aux(tau(P0)),phi_aux(tau(Q0))) <- GenImRanIso_{tau,I_tau}(d, 2^a, 3^b, ...)
P_aux,Q_aux,hint2,aux <- TorsionBasisToHint2(E_aux, a)
M_aux,2 <- ChangeOfBasis2(E_aux,(phi_aux(tau(P0)),phi_aux(tau(Q0))),(P_aux,Q_aux))
(U2,V2) <- -(sigma(tau(P0)),sigma(tau(Q0)))·M_aux,2
M_ver,2 <- ChangeOfBasis2(E_chl,(P_chl,Q_chl),(U2,V2))
S <- (E_aux, M_ver,2, hint2,aux, hint2,chl)                            // Alg. 3.2 line 32
(C) push the response through Isogeny22Chain onto E_A x E_3; t,g from the Tate pairings
    t_{3^b}(T,R_chl), t_{3^b}(T,S_chl); bin <- [Montg.coeff(E_chl) > Montg.coeff(E_aux)];
    S <- (E_3, M_ver,2, t, g, bin, chl)                                // Alg. 3.5 line 47
```

### Verify (uncompressed) — spec §3.4.4 text + Algorithm 3.3
The spec's §3.4.4 prose mandates **five** checks; Algorithm 3.3 only writes down the last two.
```
(V1) E_A and E_aux are supersingular — checked by verifying that the hint-derived bases of
     E_A[2^a], E_aux[2^a], E_chl[2^a] really span the full 2^a-torsion (§3.4.4 ¶1-2)
(V2) each hint-derived basis is a *correct* basis before it is fed to the isogeny routines
(V3) the dim-2 kernel K is a maximal isotropic subgroup (§3.4.4 ¶3, ref. to §7.5)
(V4) codomain check:  A ≅ F1 x F2 and (F1 ≅ E_A or F2 ≅ E_A)
(V5) torsion/kernel check, Alg. 3.3 lines 7-8:  Phi(0_{E_aux}, T) must be
     (0_{E_A}, *)   resp.   (*, 0_{E_A})  —  i.e. the pushed dual-challenge point of order
     3^b VANISHES in the factor isomorphic to E_A
1: R_com,S_com,hint3,com <- TorsionBasisToHint3(E_com, b)
2: E_chl, T <- ThreeIsogenyChain(R_com+[chl]S_com, E_com, b, 3)        // T = dual of phi
3: P_chl,Q_chl,hint2,chl <- TorsionBasisToHint2(E_chl, a)
4: P_aux,Q_aux,hint2,aux <- TorsionBasisToHint2(E_aux, a)
5: (U2,V2) <- (P_chl,Q_chl)·M_ver,2
6: (F1 x F2, T1) <- Isogeny22Chain((P_aux,U2),(Q_aux,V2),(0_{E_aux},T))
7: accept iff (V4) and (V5) hold, else reject
```
Algorithm 3.3 as printed is itself defective/incomplete — see discrepancy (b). Note that
this Verify has **no** explicit hash-equality step: message binding runs only through
chl -> challenge kernel -> E_chl and is therefore carried entirely by (V4)+(V5).

### Verify (compact) — spec Algorithm 3.6
```
1: P3,Q3,hint2,E3 <- TorsionBasisToHint2(E_3, a);  (U3,V3) <- (P3,Q3)·M_ver,2
3: P_A,Q_A,hint2 <- TorsionBasisToHint2(E_A, a)
4: F1 x F2 <- Isogeny22Chain((P_A,U3),(Q_A,V3))
5-8: order F1,F2 by Montgomery coefficient;  E_chl <- F1 if bin=0 else F2
14: R_chl,S_chl,hint3,chl <- TorsionBasisToHint3(E_chl, b)
15-19: E_com <- ThreeIsogenyChain(R_chl+[t]S_chl or [t]R_chl+S_chl, E_chl, b)   // g selects
20: return  chl == HASH(pk || j(E_com) || msg)                        // explicit hash compare
```

### HASH — spec Algorithm 4.1 / §4.2
```
H := SM3-{512|1024}, iterated n times (table above), truncated to ceil(log2 3^b) bits
cnt <- 0 (one byte); repeat chl <- H(msg||cnt), cnt++ until chl < 3^b; return chl
```

## Implementation vs specification

Checked: `sqisign/sign.c` (`protocols_sign`, `protocols_verif_internal`, `hash_to_challenge`),
`sqisign/keygen.c`, `pack.c` (`signature_from_bytes`), `sqisign/SIG_AlgorithmInstance.c`,
`ec_arithmetic/include/{ec_params,fp_constants,encoded_sizes,klpt_constants}.h`.
`sign.c` is byte-identical across all instances except Level1-eff, so
the verifier discussed below is shared by 7 of the 8 parameter sets. Apart from line endings
and three `(void)found;` statements, Level1-eff differs in the response-search loop bounded
by `FINDq_num_trial`: it increments `count` only when a short candidate is found
(`sign.c:267`), while the other seven increment it on every trial (`sign.c:290`).

Spot-check sampling (6 constants × all 8 sets): `POWER_OF_2` = a, `POWER_OF_3` = b,
`SECURITY_BITS` = λ, `FP2_ENCODED_BYTES`, `NWORDS_ORDER_{2,3}_BYTES`, `HASH_num_iter`.
**All agree with spec §4.2.** E.g. Level2-eff `POWER_OF_2 161`, `POWER_OF_3 96`,
`SECURITY_BITS 160`, `HASH_num_iter 256` (= 255+1 SM3-512 iterations) with `LOCATION 25`
giving 128+25 = 153-bit truncation — exactly `SM3-512_153 ∘ SM3-512^∘255`; Level5-eff
`514/319/512`, `HASH_num_iter 128`; Level2-sec `HASH_num_iter 1`. `encoded_sizes.h`
reproduces Table 6.1 exactly (Level2-eff uncompressed sig = 2·80+4·21+4 = 248).
**No size mismatch anywhere.** Also present and correct: the Fiat–Shamir structure, the
abort loop on `q` (odd and ≤ 2^a), Algorithm 4.1's rejection loop with one-byte counter, and
the compact verifier's hash comparison (`sign.c:1208` `memcmp(out, sig->chall, …)` = Alg. 3.6
line 20).

Discrepancies:

- **(a) Real deviation — the missing verification check.** In the uncompressed verifier
  `protocols_verif_internal`, `sign.c:1104-1119` implements spec check **(V5)** (Alg. 3.3
  lines 7-8, `T1 = (0_{E_A}, *)`) **inverted**: it accepts when the pushed dual-challenge
  point is *non-zero* (`!ec_is_zero(&T1.P1)` / `!ec_is_zero(&T1.P2)`), where the spec
  requires it to be the identity. The companion conditions are commented out at
  `sign.c:1105-1106`, `1112-1113` and the second/third torsion points are never pushed
  (`sign.c:1082-1097`). What remains is only (V4): `j(E_A) == j(codomain.E1 or .E2)`.
  This is the check whose absence makes `SQISign2Dsquare-Level2-eff_uncompressed` a
  CRYPTOFAIL (known finding LH-SIGN-25-001/002 in `security_findings.md` / `RESULTS.md`:
  modified message accepted, and for some keypairs an all-zero signature accepted). The
  same defective code is compiled into the other 6 uncompressed instances sharing `sign.c`;
  they happened to reject the sampled forgeries, so the defect is latent there, not absent.
- **(a) Real deviation.** Spec checks **(V1)-(V3)** (supersingularity of E_A/E_aux via full
  2^a-torsion, basis correctness, maximal-isotropy of the dim-2 kernel) have no counterpart
  in the uncompressed verifier: `ec_curve_to_basis_2f_from_hint` results go straight into
  `theta_chain_comput_strategy` (`sign.c:1057-1080`) with no order or isotropy test. The
  only order assertions present are `#if _DEBUG` (`sign.c:1053-1055`) and the structural
  `assert`s at `sign.c:1033-1035` are compiled out by the submission's own `-DNDEBUG`.
- **(a) Real deviation (input validation).** `pack.c:295+` `signature_from_bytes` decodes
  fixed-size fields with no range or validity check, so an all-zero buffer yields a
  syntactically valid signature; `SIG_AlgorithmInstance.c:63-64` also discards
  `pk_len_bytes` / `sn_len_bytes`.
- **(b) Spec incompleteness.** The uncompressed signature's content is stated three ways
  (§3.4.4, Alg. 3.2 line 32, §3.5). The code ships `(E_aux, E_com, M_ver,2, hint_aux,
  hint_com[2], hint_chall)` = 2·FP2 + 4·word + 4 bytes, i.e. it transmits `E_com` and *not*
  `chl` — matching §3.5 and Table 6.1, not §3.4.4. Algorithm 3.3 is further defective as
  printed: empty `if` body at line 7 (no `return`), `E1/E2` vs `F1/F2`, and an input list
  `(E_com, K_cha, E_aux, M_ver,2)` that matches neither §3.4.4 nor §3.5.
- **(c) Cosmetic.** `hash_to_challenge` (`sign.c:312-353`) absorbs
  `j(E_com) || j(E_A) || msg || cnt`, i.e. commitment before public key, whereas Alg. 3.2
  line 12 writes `HASH(pk || j(E_com) || msg)`. Same fields, different order.

Coverage limits (time-boxed, nothing built or executed): the dimension-2 kernels
(`theta_isogenies.c`, `hd.c`), `ImRanIso`/`GenImRanIso`, the quaternion layer (`klptx.c`,
`lattice.c`) and the compact signer were not line-checked against Chapters 2 and 7, and the
precomputed tables (`quaternion_data.c`, `torsion_constants.c`, strategies) were not checked.
