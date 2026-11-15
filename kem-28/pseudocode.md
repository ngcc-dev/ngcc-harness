# kem-28 OAEP-NTRU — algorithm summary

NTRU-lattice KEM whose distinguishing feature is that it is **not** FO-transformed.
It uses the submission's own **OAEP†** transform (§3.3.2): a one-round OAEP-like
padding in which the "trapdoor injective function" is the NTRU map
F(x1, x2) = h·x1 + x2, the randomness x2 = e is fed to F *directly* (no second
hash H′, hence no Feistel round), the message is padded by a **Semi-generalised
One-Time Pad (SOTP)** rather than by a group addition, and CCA validity is
enforced by an explicit authenticator σ = H_pk(x1, x2) carried in the ciphertext
instead of by re-encryption. Security rests on NTRU and Ring-LWE over
R = Z_q[X]/(X^n − X^{n/2} + 1).

Specification: `kem-28-spec.pdf` (48 pages, English), §2.2 (Algorithms 1–10),
§3.3 (the OAEP† transform, Figs. 2 and 4), parameters in §3.5 Table 1, sizes in
Table 4.

## Parameters

| parameter | OAEP-NTRU-648 | OAEP-NTRU-1296 | OAEP-NTRU-2592 | meaning |
|---|---|---|---|---|
| n | 648 | 1296 | 2592 | ring degree, R = Z_q[X]/(X^n − X^{n/2} + 1) |
| q | 7129 | 17497 | 28513 | modulus (⌈log2 q⌉ = 13, 15, 15) |
| \|k\| | 32 | 32 | 64 | encapsulated key bytes |
| λ | 128 | 256 | 512 | target classical security |
| λ/4 (= \|σ\| = \|pkDigest\|) | 32 | 64 | 128 | authenticator bytes |
| plen = ⌈log2 q⌉·n/8 | 1053 | 2430 | 4860 | serialised polynomial bytes |
| (d, L3, L2) | (3, 3, 2) | (2, 4, 2) | (3, 3, 4) | NTT layer configuration |
| correctness error | 2^−159 | 2^−978 | 2^−652 | spec's DFR |
| claimed security | ≥128 cl / ≥80 qu | ≥256 / ≥128 | ≥512 / ≥256 | bits (Table 1; Table 2 gives 144/138, 273/254, 512/468) |

Sizes (bytes), specification (Table 4) vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | ct spec | ct impl | ss spec | ss impl | match |
|---|---|---|---|---|---|---|---|---|---|
| OAEP-NTRU-648 | 1053 | 1053 | 2138 | 2138 | 1085 | 1085 | 32 | 32 | yes |
| OAEP-NTRU-1296 | 2430 | 2430 | 4924 | 4924 | 2494 | 2494 | 32 | 32 | yes |
| OAEP-NTRU-2592 | 4860 | 4860 | 9848 | 9848 | 4988 | 4988 | 64 | 64 | yes |

The **ct − pk overhead of 32 / 64 / 128 bytes is exactly the OAEP† authenticator
σ**, whose length is λ/4 bytes (= 2λ bits): ct = EncodePoly(c) ‖ σ, so
ct = plen + λ/4. Likewise sk = EncodePoly(f̂) ‖ EncodePoly(ĥ⁻¹) ‖ pkDigest(pk)
= 2·plen + λ/4, which is where sk ≈ 2·pk + λ/4 comes from.

## The OAEP† padding composition (the distinguishing feature)

This is **not** RSA-OAEP: there is no MGF1, no lHash/label, no PS‖0x01‖M byte
layout, and — the spec says so explicitly in §3.3.2 — "our OAEP† **does not have
padding in m**". Classical OAEP's two-round Feistel
`x1 := G(r) + m ; x2 := H′(x1) + r ; ct := F(x1,x2)` is replaced by:

```
x2  := e                       <-$ CBD1(n/4 random bytes)      // fed to F directly; H' removed
r   := G(x2)                   = XOF(0x01 ‖ EncodePolyShort(e), n/4)   // n/4 mask bytes
x1  := SOTPEncode(m, r)                                        // replaces "G(r) + m"
c   := F(x1, x2) = h·x1 + x2
σ   := H_pk(x1, x2) = XOF(0x05 ‖ pkDigest(pk) ‖ x1 ‖ x2, λ/4 + |k|)[0 : λ/4]
k   :=                                          same XOF output [λ/4 : λ/4+|k|]
ct  := (c, σ)
```

The padding proper is the SOTP (Alg. 4 / Alg. 5, Fig. 2), i.e. a mask-then-CBD
encoding, not a mask-then-concatenate one:

```
SOTPEncode(m ∈ B^{n/8}, r ∈ B^{n/4}) -> s ∈ {-1,0,1}^n :        // Algorithm 4
    tmp := ( r[0 : n/8-1] XOR m ) ‖ r[n/8 : n/4-1]              // first half masks m,
    s   := CBD1(tmp)                                            //   second half is the "-r2" part
    // equivalently (Fig. 2): s[i] = (m[i] XOR r1[i]) - r2[i] over Z, bitwise

SOTPDecodeCheck(y ∈ Z^n, r ∈ B^{n/4}) :                         // Algorithm 5
    t := BytesToBits(r)
    u[i] := y[i] + t[i + n]           for i = 0..n-1            // add back r2
    if u ∉ {0,1}^n : return ⊥                                   // the CCA validity check
    else m[i] = u[i] XOR t[i] ; return ⊤
```

So OAEP†'s "padding check" is *not* a redundancy field; it is the statement that
after removing the r2 half of the mask every coefficient lies in {0,1}. Together
with the σ equality check, that is the whole rejection mechanism — there is no
re-encryption anywhere in Decap.

**In the KEM (as opposed to the PKE-with-partial-message-recovery mode), the
padded message is the all-zero string**: Algorithm 2 line 1 sets m := 0^{n/8}.
All ciphertext entropy therefore comes from e; SOTPEncode degenerates to
s = CBD1(r) with r = G(e).

## Pseudocode

### KeyGen — Algorithm 1 (§2.2)
```
repeat  f' <-$ B^{n/4}; f' := CBD1(f'); f := 3·f' + 1; f_hat := NTT(f)  until f invertible in R
repeat  g' <-$ B^{n/4}; g' := CBD1(g'); g := 3·g';     g_hat := NTT(g)  until g invertible in R
h_hat := g_hat · f_hat^{-1}
pk := EncodePoly(h_hat)
sk := EncodePoly(f_hat) ‖ EncodePoly(h_hat^{-1}) ‖ pkDigest(pk)
```

### Encap — Algorithm 2
```
m  := 0^{n/8}                                  // KEM mode: no message
e' <-$ B^{n/4};  e := CBD1(e')                 // x2, the "randomness" fed straight to F
h_hat := DecodePoly(pk)
e  := EncodePolyShort(e)
r  := G(e)                                     // XOF(0x01 ‖ e, n/4)
s  := SOTPEncode(m, r);  s := EncodePolyShort(s)   // x1
s_hat := NTT(s); e_hat := NTT(e)
c  := h_hat ⊙ s_hat − e_hat                    // SEE DISCREPANCY BELOW: impl computes "+"
(σ, k) := H( pkDigest(pk) ‖ s ‖ e )            // XOF(0x05 ‖ ·, λ/4 + |k|)
ct := EncodePoly(c) ‖ σ
return (ct, k)
```

### Decap — Algorithm 3
```
c_hat    := DecodePoly(ct[0 : plen-1])
f_hat    := DecodePoly(sk[0 : plen-1]);  h_inv := DecodePoly(sk[plen : 2plen-1])
e        := InvNTT(c_hat ⊙ f_hat) mod 3        // g ≡ 0, f ≡ 1 (mod 3)  =>  recovers e
e_hat    := NTT(e)
s_hat    := h_inv ⊙ (c_hat − e_hat)
s        := InvNTT(s_hat)
if s ∉ {-1,0,1}^n : return ⊥                   // short-polynomial check
d        := sk[2plen : 2plen + λ/4 - 1]        // cached pkDigest(pk)
s := EncodePolyShort(s);  e := EncodePolyShort(e)
(σ', k') := H(d ‖ s ‖ e)
if σ' ≠ σ  or  SOTPDecodeCheck(s, G(e)) = ⊥ : return ⊥
else return k'
```

Hashes (Alg. 8–10): `pkDigest = HASH(0x00 ‖ pk)` → λ/4 bytes;
`G = XOF(0x01 ‖ ·, n/4)`; `H = XOF(0x05 ‖ ·, λ/4 + |k|)`. Instantiated with the
NGCC/ICCS library: SM3 / `pseudohash` and `pseudoXOF` (KDF-SM3).

## Implementation vs specification

Checked (built sources per `kem-28/Makefile`:
`Implementations/Reference_Implementation/<inst>/` — `KEM_AlgorithmInstance.c`,
`poly.c`, `ntt.c`, `symmetric.c`, `auxfunc.c`, `drng.c`). `kem_keygen/kem_enc/
kem_dec` implement Algorithms 1–3; `poly_sotp` / `poly_sotp_inv` (`poly.c:284`,
`poly.c:314`) implement Algorithms 4 and 5; `symmetric.c` the three hashes.

Agreements:
- Table 1 is reproduced exactly in `params.h` (n, q = 648/7129, 1296/17497,
  2592/28513) and the derived sizes follow the spec's formulas symbolically:
  `SECRETKEYBYTES = (POLYBYTES << 1) + SYMBYTES`, `CIPHERTEXTBYTES = POLYBYTES +
  SYMBYTES`, with `SYMBYTES` = 32/64/128 = λ/4 and `SSBYTES` = 32/32/64 = |k|.
  All twelve size values match OBSERVED and Table 4.
- The OAEP† padding is implemented exactly as specified. `poly_sotp` XORs the
  mask into the first n/8 bytes only and passes the concatenation through
  `poly_cbd1` (`poly.c:284-300`), matching Algorithm 4 line 1 verbatim.
- `poly_sotp_inv` (`poly.c:314-395`) implements the Algorithm 5 check and is
  **constant-time**: it accumulates `r |= coeff + maskbit` over all n
  coefficients and collapses it with `r = r >> 1; r = (-(uint64_t)r) >> 63`,
  with no data-dependent branch. Its three loop blocks cover coefficients
  0–511, 512–639 and 640–647 in exactly the interleaved order used by
  `poly_cbd1` (`poly.c:133-180`), so all 648/1296/2592 coefficients are checked
  — I verified the index arithmetic covers the full range with no gap.
- Domain-separation prefixes match Algorithms 8–10: the `data` initialiser is `{0x0}` for
  pkDigest, `{0x1}` for G, `{0x5}` for H (`symmetric.c:7, 19, 32` in OAEP-NTRU-648), and the H
  output is split as σ = first λ/4 bytes, k = last |k| bytes
  (`KEM_AlgorithmInstance.c:144-145`), matching `KeyConfirmation_BYTES =
  SSBYTES + SYMBYTES`.
- pkDigest output width is level-dependent as the spec requires: `sm3hash(256,…)`
  for 648, `pseudohash(512,…)` for 1296, `pseudohash(1024,…)` for 2592
  (`symmetric.c:14`, `:14`, `:16` in the three trees) = 32/64/128 bytes = λ/4.
- Decap has **no re-encryption**, as §3.3.2 claims — only the short-polynomial
  range check, `poly_sotp_inv`, and the σ comparison.

Discrepancies:
- **(a) sign error in the specification's Algorithm 2.** Alg. 2 line 11 writes
  `c := ĥ ⊙ ŝ − ê`. The implementation computes `c = h·s + e`:
  `poly_basemul_add(&p_c, &p_h, &p_s, &p_t)` (`KEM_AlgorithmInstance.c:137`)
  resolves to `r = a*b + c` (`ntt.c:424-432`, `poly.c:515-522`). The **plus** is
  the self-consistent one: Algorithm 3 line 4 recovers e as `InvNTT(ĉ⊙f̂) mod 3`,
  which equals +e only if c = h·s + e (since g ≡ 0 and f ≡ 1 mod 3), and
  Algorithm 3 line 6's `ĥinv ⊙ (ĉ − ê)` then yields s. With the spec's minus
  sign, step 4 would recover −e and the σ recomputed in step 14 could never
  match the one produced by Encap step 12. So Algorithm 2 line 11's minus is a
  spec typo and the implementation is right; but as written the spec does not
  describe a working scheme.
- **(a) the shared secret is written on rejection.** Algorithm 3 returns ⊥ on
  either check failing. `kem_dec` instead always executes
  `memcpy(ss, sigma + NTRUOAEP_SYMBYTES, NTRUOAEP_SSBYTES)` before returning the
  nonzero `fail` code (`KEM_AlgorithmInstance.c:205-209` in OAEP-NTRU-648,
  `:252-256` in -1296, `:231-236` in -2592). `fail` is an `int8_t` that
  accumulates `ct[i] ^ sigma[i]` over the tag bytes, so the returned code is the
  bytewise OR of the tag difference rather than a 0/1 flag (kem-28-2). The value written is
  k′ = H(d ‖ s ‖ e), where s and e are the results of decrypting the received
  body with the private key and d is read from the secret key
  (`KEM_AlgorithmInstance.c:195`). d is the public-key digest that key generation
  stores there (`hash_f(sk + 2 * NTRUOAEP_POLYBYTES, pk)`, `:90`; encapsulation
  recomputes it from pk at `:135`), so k′ is secret only through s and e. For an
  honest ciphertext body with a wrong tag, s and e are the sender's values and
  the buffer holds the honest shared secret although the call is rejected; a
  party who built the body itself already knows s and e and learns nothing new.
  A caller that reads the output buffer without checking the return code
  therefore uses a key that Algorithm 3 withholds. The spec gives no guidance on
  what to write into the output buffer on ⊥, so this is at least a spec gap; in
  an API where ⊥ must still fill a buffer, implicit rejection (a pseudorandom
  value from a secret) would be the safe reading.
- **(minor, non-constant-time)** the short-polynomial check uses short-circuit
  `||`: `fail |= (p_s.coeffs[i] > 1 || p_s.coeffs[i] < -1)`
  (`KEM_AlgorithmInstance.c:184`). `fail` is accumulated over the whole loop so
  no early exit occurs, but the per-coefficient comparison is compiler-dependent
  and may branch.
- **(cosmetic)** `poly.c:297-298` contains leftover Chinese developer comments
  questioning whether the code had been updated to "the new version with the
  subtraction" — directly adjacent to the sign issue above.

Not verified: the NTT/InvNTT layer configuration (d, L3, L2) of Table 1 against
`ntt.c`; the `EncodePoly13/15` bit-packing routines (Alg. 11–14) beyond
confirming `POLYBYTES` = ⌈log2 q⌉·n/8; the correctness-error and lattice-estimator
numbers of §3.4/§5.3. The library was not executed; KAT conformance is reported
in `kem-28/security_findings.md` and `RESULTS.md`.
