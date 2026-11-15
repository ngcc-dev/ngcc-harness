# kem-27 NTRE — algorithm summary

NTRU-style lattice KEM: an IND-CPA PKE (h = g·f⁻¹ with f ≡ 1 mod 2, ciphertext
c = h·r + e, decryption by c·f mod ±q mod 2) over the non-power-of-two cyclotomic
ring R_q = Z_q[X]/(X^n − X^{n/2} + 1), whose security the spec reduces to the
R-NTRU_η and R-LWE_η problems. IND-CCA2 is obtained by a variant of the FO_AC
transform the submission calls **FOAC′** — an explicit-rejection FO in which the
message is split into m1‖m2, m2 carries r masked by F(m1), and re-encryption plus
an m1 equality check gate the shared secret. Symmetric primitives are the ICCS
NGCC API: SM3 for G, KDF-SM3 `pseudoXOF` with one-byte domain prefixes for
XOF/F/H, and the NGCC DRNG for randomness (§1.4).

Specification: `kem-27-spec.pdf` (43 pages, English), §1.3 (Algorithms 8–13),
parameters in §1.4 Table 1.

## Parameters

| parameter | NTRE-128 | NTRE-256 | NTRE-512 | meaning |
|---|---|---|---|---|
| spec's own name | NTRE-2593-648 | NTRE-2917-1296 | NTRE-3457-2304 | §1.4 naming |
| n | 648 | 1296 | 2304 | ring degree, R_q = Z_q[X]/(X^n − X^{n/2} + 1) |
| q | 2593 | 2917 | 3457 | modulus (⌈log2 q⌉ = 12 for all three) |
| η | ψ_1 | ψ_1 | ψ_1 | centered binomial, Pr[±1]=0.25, Pr[0]=0.5 |
| d (NTT base degree) | 3 | 4 | 4 | impl-only; base ring degree for `basemul` |
| δ (DFR) | 2^−549 | 2^−370 | 2^−298 | spec's decapsulation failure bound |
| \|m1\| = ⌊n/16⌋ | 40 | 81 | 144 | bytes |
| \|m2\| = \|r\| = ⌈n/16⌉ | 41 | 81 | 144 | bytes |
| \|ρ\| = 3n/8 | 243 | 486 | 864 | encryption coin bytes |
| claimed security | 132 cl / 118 qu | 290 / 259 | 584 / 513 | bits (spec's own claim) |

Sizes (bytes), specification (§1.3 formulas, Table 1) vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | ct spec | ct impl | ss spec | ss impl | match |
|---|---|---|---|---|---|---|---|---|---|
| NTRE-128 | 972 | 972 | 1976 | 1976 | 972 | 972 | 64 | 64 | yes |
| NTRE-256 | 1944 | 1944 | 3920 | 3920 | 1944 | 1944 | 64 | 64 | yes |
| NTRE-512 | 3456 | 3456 | 6944 | 6944 | 3456 | 3456 | 64 | 64 | yes |

The encoding explains the observed shape. Every transmitted object is one ring
element serialised at exactly ⌈log2 q⌉ = 12 bits per coefficient, so
**pk = ct = 12n/8 bytes** (pk is h, ct is c = h·r + e — the same object type;
there is no second ciphertext component and no compression). The CCA secret key
is sk = sk′ ‖ pk ‖ G(pk) (Algorithm 11 line 2) = 12n/8 + 12n/8 + 32, i.e.
**sk = 2·pk + 32**, which is where the ~2× comes from. The shared secret is a
fixed 64 bytes (B^64) at every level, independent of the security level.

## Pseudocode

### CPAPKE.KGen — Algorithm 8 (§1.3.1)
```
repeat
    f' <- Sample(n)                     // f' <-$ ψ_1^n  (Algorithm 5, CBD1)
    f_hat := NTT(2·f' + 1)              // f := 2f' + 1, so f ≡ 1 (mod 2)
until f invertible in R_q
g' <- Sample(n);  g_hat := NTT(2·g')    // g := 2g'
h_hat := g_hat · f_hat^{-1}
pk := Encode_q(h_hat)                   // h = g·f^{-1}, 12 bits/coeff
sk' := Encode_q(f_hat)
return (pk, sk')
```

### CPAPKE.Enc(pk, m; r) — Algorithm 9
```
r      := CBD1(r_[1..n/4-1])                       // r <-$ ψ_1^n      (Algorithm 3)
r_hat  := NTT(r)
e      := CBD1'( m ‖ r_[n/4 .. 3n/8-1] )           // e := CBD1'(m, r'), Algorithm 4:
                                                   //   message bits bias the error poly
e_hat  := NTT(e)
h_hat  := Decode_q(pk)
c_hat  := h_hat·r_hat + e_hat                      // c := h·r + e
return c := Encode_q(c_hat)
```

### CPAPKE.Dec(sk, c) — Algorithm 10
```
f_hat := Decode_q(sk);  c_hat := Decode_q(c)
m := NTT^{-1}(c_hat · f_hat) mod 2                 // (c·f mod ±q) mod 2
return Bits2Bytes(m)
```

### CCAKEM.KGen / Encaps / Decaps — Algorithms 11, 12, 13 (§1.3.2)
```
KGen:  (pk, sk') <- CPAPKE.KGen();  sk := sk' ‖ pk ‖ G(pk);  return (pk, sk)

Encaps(pk):                                        // FOAC'
  r          <-$ B^{⌈n/16⌉}
  (m1,ρ,K)   := H(r ‖ G(pk))                       // B^{⌊n/16⌋} × B^{3n/8} × B^{64}
  m2         := r ⊕ F(m1)                          // F : B^{⌊n/16⌋} -> B^{⌈n/16⌉}
  c          := CPAPKE.Enc(pk, m1‖m2; ρ)
  return (K, c)

Decaps(sk, c):
  parse sk = (sk', pk, h)                          // h = G(pk), cached
  m1‖m2      := CPAPKE.Dec(sk', c)
  r          := m2 ⊕ F(m1)
  (m1',ρ,K)  := H(r ‖ h)
  c'         := CPAPKE.Enc(pk, m1‖m2; ρ)
  if m1 = m1' and c = c' then return K else return ⊥
```

Hash instantiation (§1.4): `XOF = pseudoXOF(0x00 ‖ ·)`, `F = pseudoXOF(0x01 ‖ ·)`,
`H = pseudoXOF(0x02 ‖ ·)` (all KDF-SM3), `G = sm3hash(·)` → 32 bytes.

## Implementation vs specification

Checked (built sources per `kem-27/Makefile`:
`Implementations/Reference_Implementation/<inst>/` — `src/poly.c`, `src/ntt.c`,
`src/symmetric.c`, `KEM_AlgorithmInstance.c`, `auxfunc.c`, `drng.c`).
`KEM_AlgorithmInstance.c` implements Algorithms 8–13 directly (`kem_keygen`,
`kem_enc`, `kem_dec`, static `cpapke_enc` / `cpapke_dec`); `src/poly.c` holds
CBD1/CBD1′ and the Encode_q/Decode_q 12-bit packing; `src/symmetric.c` the four
hash wrappers.

Agreements:
- Table 1 is reproduced exactly: `src/params.h` has `NTRE_N 648 / NTRE_Q 2593`,
  `1296 / 2917`, `2304 / 3457` for the three instances, and `poly.h:8` documents
  the ring as `R_q = Z_q[X]/(X^n - X^(n/2) + 1)` as in §1.4.
- The derived byte lengths follow the spec's formulas symbolically rather than
  being hand-entered: `NTRE_M1BYTES (N/16)`, `NTRE_M2BYTES ((N+15)/16)`,
  `NTRE_RHOBYTES (3*N/8)`, `NTRE_SSBYTES 64`, and
  `NTRE_SECRETKEYBYTES ((NTRE_POLYBYTES << 1) + NTRE_GBYTES)` = 2·pk + 32
  (`src/params.h:14-25`). All six size values match OBSERVED.
- The hash instantiation of §1.4 is implemented literally, including the
  one-byte domain prefixes: `data[0] = 0x00` for the sampling XOF, `0x01` for F,
  `0x02` for H, and `sm3hash(256, pk, ...)` for G (`src/symmetric.c:10,19,30,40`).
  `HASH_H_OUTBYTES = M1BYTES + RHOBYTES + SSBYTES` (`src/symmetric.h:7`) matches
  the output space of H in Algorithm 12 line 2.
- The FOAC′ structure is present and correct in `kem_dec`
  (`KEM_AlgorithmInstance.c:191-209`): recompute r = m2 ⊕ F(m1), re-derive
  (m1′, ρ, K) = H(r‖h), check **both** m1 = m1′ and c = c′ after re-encryption —
  the two checks Algorithm 13 line 6 requires — using a constant-time `ct_diff`.
- `f := 2f′ + 1` and the invertibility retry loop of Algorithm 8 lines 1–4 are in
  `sample_invertible_f` / the `do { ... } while (sample_invertible_f(...))` loop
  at `KEM_AlgorithmInstance.c:98-101`.

Discrepancies / notes:
- **(b) naming, not an algorithmic deviation.** The submission's build labels are
  `NTRE-128 / NTRE-256 / NTRE-512`, but §1.4 names the same three parameter sets
  `NTRE-2593-648 / NTRE-2917-1296 / NTRE-3457-2304` and claims 132 / 290 / 584
  classical bits. The labels are therefore NGCC security-category tags, not the
  spec's own names, and the level-512 label corresponds to a 584-bit claim.
- **(b/c) rejection behaviour.** Algorithm 13 returns ⊥. The implementation
  instead masks the shared secret to **all zeros** on failure and returns −1
  (`KEM_AlgorithmInstance.c:206-211`: `mask_byte = fail - 1; ss[i] = key[i] &
  mask_byte`). That is a faithful reading of "return ⊥" for an API that must fill
  a buffer, but the rejection output is a *fixed constant* rather than a
  pseudorandom value, so every invalid ciphertext yields the same shared secret
  to a caller that ignores the return code. The spec does not specify what to
  write into the output buffer on rejection, so this is a spec gap the
  implementation resolves in the weakest safe way.
- No parameter, size or hash-input-order mismatch was found.

Not verified: the NTT constants in `src/ntt.c` (zetas table, Montgomery constants
for the X^n − X^{n/2} + 1 ring with base-degree d = 3 for NTRE-128 and d = 4 for NTRE-256/512) were not checked against
any spec statement — the spec does not specify an NTT, only the ring. The DFR
analysis of §2.3 and the security estimates of Table 2 were not re-derived. The
library was not executed; KAT conformance is reported in
`kem-27/security_findings.md` and `RESULTS.md`.
