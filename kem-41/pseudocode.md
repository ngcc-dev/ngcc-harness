# kem-41 ZEN — algorithm summary

NTRU KEM in the power-of-two cyclotomic ring R_{n,q} = Z_q[x]/(x^n+1) with q = 769,
public-key relation h ≡ g·f^{-1}, made IND-CCA by an ML-KEM-style FO transform with
implicit rejection. ZEN is DAWN-derived; its distinguishing feature is **zero-divisor
(double) encoding** of the message (see below) instead of the usual integer encoding,
which lets one ciphertext ring element of degree n carry only n/4 message bits but
tolerate one "q-wrap" error per group of four coefficients.

Specification: `kem-41-spec.pdf` (38 pages), §2 (encodings/packing), §3 (Algorithms 1–8),
§3.3 (correctness / SimpleDecoding analysis), §6.1 Table 3 (parameters). English.

## The zero-divisor encoding (the distinguishing feature)

Message space M = {0,1}^{n/4}. The encoder does **not** map a bit to one coefficient; it
lifts m by the *canonical lift*

```
Lift(m) = (q+1)/2 · (1 + x^{n/4} + x^{n/2} + x^{3n/4}) · m        (in R_{n,q})
```

i.e. each message bit is replicated at the four positions r, r+n/4, r+n/2, r+3n/4 with
amplitude (q+1)/2 ≈ q/2. The multiplier (1 + x^{n/4} + x^{n/2} + x^{3n/4}) is
(x^{n/2}+1)(x^{n/4}+1) — a product of two **zero divisors** of F_2[x]/(x^n+1), and the
second divides the first. Decryption peels them off in two binary layers:

1. **First zero divisor x^{n/2}+1** — decryption multiplies by (x^{n/2}+1) before
   reducing mod 2, which makes the noise term ε_wrap = (x^{n/2}+1)(gs + fe + f·e_compress)
   carry the factor x^{n/2}+1 and therefore *vanish* modulo ⟨2, x^{n/2}+1⟩. What survives
   is m′ ≡ (x^{n/4}+1)m + e_wrap·f_inv (mod ⟨2, x^{n/2}+1⟩), where e_wrap = ⌊z_wrap/q⌋ is
   the *integer wrap* count, not a small additive error. So the "error" left to correct is
   only the per-coefficient q-wrap indicator, a binary vector.
2. **Second zero divisor x^{n/4}+1** — modulo ⟨2, x^{n/4}+1⟩ the four positions of a group
   G_r = {r, r+ν, r+2ν, r+3ν} (ν = n/4) are identified, so the *message* term
   (x^{n/4}+1)m vanishes there and the reduction yields the pure wrap syndrome
   W_r = B_{r,0} ⊕ B_{r,1} ⊕ B_{r,2} ⊕ B_{r,3}, B_{r,j} = (e_wrap)[r+jν] mod 2.
3. **SimpleDecoding** turns the syndrome into a correction. Recovery happens modulo
   ⟨2, x^{n/2}+1⟩, where the group splits into two half-cycles with target bits
   T_{r,0} = B_{r,0} ⊕ B_{r,2} and T_{r,1} = B_{r,1} ⊕ B_{r,3}, and W_r = T_{r,0} ⊕ T_{r,1}.
   If W_r = 0 both are set to 0. If W_r = 1 exactly one half-cycle wrapped, and the decoder
   picks it by a *soft* comparison of how close the centred q-level residues of a are to
   the wrap boundary q/2:
   S_{r,0} = min(||a[r]| − q/2|, ||a[r+2ν]| − q/2|), S_{r,1} = min(||a[r+ν]| − q/2|,
   ||a[r+3ν]| − q/2|); S_{r,0} < S_{r,1} ⟹ (T̂_{r,0}, T̂_{r,1}) = (1,0), else (0,1).
   Hence **each group of four coefficients tolerates exactly one wrap**; two or more
   wraps in one group (or a tie) is the decoding failure counted in the DFR.
4. Subtracting η·f_inv from m′ and truncating to degree < n/4 (i.e. "mod ⟨2, x^{n/4}⟩",
   keeping the first of the four copies) returns m.

Design cost/benefit stated by the spec (§4): the first zero divisor kills noise
amplification at the price of a half-dimensional message space, the second kills the
message term so the wrap can be located, costing another half — hence M = {0,1}^{n/4}.

## Parameters

| parameter | ZEN_128 | ZEN_256 | ZEN_512 | meaning |
|---|---|---|---|---|
| n | 512 | 1024 | 2048 | ring degree (power of 2) |
| q | 769 | 769 | 769 | prime modulus (all rows) |
| M | {0,1}^128 | {0,1}^256 | {0,1}^512 | message space {0,1}^{n/4} |
| D_g | B1 + S_{1/8} | S_{3/16} | S_{3/32} | distribution of g |
| D_f | B1 | S_{1/8} | S_{3/32} | distribution of f |
| D_s | B1 + S_{3/32} | B1 | S_{1/8} | distribution of s |
| D_e | B2 | B1 | S_{1/8} | distribution of e |
| compression M_c | 769 → 256 | 769 → 256 | 769 → 256 | ciphertext coefficient range |
| CoreSVP | 128 | 257 | 519 | bits (spec Table 3) |
| log2 DFR_est | −128 | −175 | −179 | spec Table 3 |

(B_η = centred binomial of width η; S_d = ternary with Pr[+1] = Pr[−1] = d. Table 3 also
lists three rows that are **not** shipped: ZEN-light (n = 512, CT 448) and the two
uncompressed "-backup" rows at M_c = q = 769.)

Sizes (bytes), specification vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | ct spec | ct impl | ss spec | ss impl | match |
|---|---|---|---|---|---|---|---|---|---|
| ZEN_128 | 615 | 615 | (not given) | 1303 | 512 | 512 | 16 | 16 | yes |
| ZEN_256 | 1229 | 1229 | (not given) | 2605 | 1024 | 1024 | 32 | 32 | yes |
| ZEN_512 | 2458 | 2458 | (not given) | 5210 | 2048 | 2048 | 64 | 64 | yes |

The spec gives no sk size. Implementation layout (params.h):
sk = f̂ (10 bits/coeff = 10n/8 bytes) ‖ f_inv (n/4 bits = n/32 bytes) ‖ pk ‖ h_pk
(ZEN_SYM_LEN_BYTES) ‖ κ′ (ZEN_SHAREDKEY_LEN_BYTES) = 640+16+615+16+16 = 1303 etc.
ss = n/4 bits, the same length as the PKE message; it is the leading bytes of
H_m(m ‖ h_pk) (`KEM_AlgorithmInstance.c:128`), not m itself.

## Pseudocode

### FastInversion(f, d) — inverse in F_2[x]/(x^d+1) (Algorithm 1)
```
if f ≡ 0 (mod ⟨x+1, 2⟩): return ⊥        # x^n+1 ≡ (x+1)^n mod 2, so this is the only obstruction
l ← log2(d);  k ← (f+1)/(x+1);  f_inv ← 1
for i = 0 .. l-1:
    b ← k·f_inv        (mod ⟨x^{2^i}+1, 2⟩)
    k ← k + f·b        (mod ⟨x^n+1, 2⟩)
    k ← k / (x^{2^i}+1)
    f_inv ← f_inv + (x^{2^i}+1)·b        # Newton-style doubling of the modulus
return f_inv
```

### SimpleDecoding(a, m′, f_inv) (Algorithm 2)
```
δ ← a (mod ⟨2, x^{n/4}+1⟩)               # the wrap syndrome W
η ← 0
for i = 0 .. n/4-1:
    v0 ← min(| |a[i]|      − q/2 |, | |a[i+n/2]|   − q/2 |)
    v1 ← min(| |a[i+n/4]|  − q/2 |, | |a[i+3n/4]| − q/2 |)
    η ← η + (v0 < v1 ? x^i : x^{i+n/4})·δ[i]
m ← ( m′ + η·f_inv (mod ⟨2, x^{n/2}+1⟩) ) (mod ⟨2, x^{n/4}⟩)
return m
```

### ZEN.PKE.KeyGen(ρ_g, ρ_f) (Algorithm 3)
```
N ← 0
repeat  g ← Sample(D_g^n, PRF(ρ_g, N));  ĝ ← NTT(g);  N++   until  ∏ ĝ[i] ≠ 0
repeat  f ← Sample(D_f^n, PRF(ρ_g, N));  f_inv ← FastInversion(f, n/2);
        f̂ ← NTT(f);  N++                                     until  ∏ f̂[i] ≠ 0 and f_inv ≠ ⊥
ĥ ← ĝ ⊙ f̂^{-1} (mod ⟨q, x^n+1⟩)
pk ← Pack_q(ĥ);   sk_PKE ← (f̂, f_inv)
```

### ZEN.PKE.Encrypt(pk, m, ρ) (Algorithm 4)
```
ĥ ← Unpack_q(pk)
s ← Sample(D_s^n, PRF(ρ, 0));   e ← Sample(D_e^n, PRF(ρ, 1))
u ← INTT(ĥ ⊙ NTT(s)) + e                       (mod ⟨q, x^n+1⟩)
c ← u + (q+1)/2 · (1 + x^{n/4} + x^{n/2} + x^{3n/4})·m        # zero-divisor lift
ct ← Pack_{M_c}(Compress_{q,M_c}(c))
```

### ZEN.PKE.Decrypt(sk_PKE, ct) (Algorithm 5)
```
(f̂, f_inv) ← sk_PKE
c† ← Decompress_{q,M_c}(Unpack_{M_c}(ct))
a  ← (x^{n/2}+1)·INTT(NTT(c†) ⊙ f̂)             (mod ⟨q, x^n+1⟩)
m′ ← a·f_inv                                    (mod ⟨2, x^{n/2}+1⟩)
m  ← SimpleDecoding(a, m′, f_inv)
```

### ZEN.KEM (Algorithms 6, 7, 8)
```
KeyGen:  (pk, sk_PKE) ← PKE.KeyGen;  κ′ ←$ {0,1}^{n/4};  h_pk ← H_pk(pk)
         sk_KEM ← (sk_PKE, pk, h_pk, κ′)
Encaps:  m ←$ {0,1}^{n/4};  (K, ρ) ← H_m(m, H_pk(pk));  ct ← PKE.Encrypt(pk, m, ρ)
Decaps:  m ← PKE.Decrypt(sk_PKE, ct);  (K, ρ) ← H_m(m, h_pk);  K′ ← H_K(κ′, ct)
         ct′ ← PKE.Encrypt(pk, m, ρ);  return ct′ = ct ? K : K′     # implicit rejection
```
Hash instantiation (spec §3.2): "domain-separated SM3-based hash functions" for
H_m, H_pk, H_K. Packing (§2): coefficients mod 769 in blocks of five → 48 bits → 6 bytes
(NEV/DAWN packing); power-of-two ranges by plain bit concatenation.

## Implementation vs specification

Checked (`Implementations/Reference_Implementation/ZEN_{128,256,512}`, the files the
Makefile builds): `params.h`, `pke.c` (Alg. 3/4/5), `poly.c` (FastInversion, pack/unpack,
compress, NTT glue), `sample.c` (the four distributions), `symmetric.c`,
`KEM_AlgorithmInstance.c` (Alg. 6/7/8), `../api/auxfunc.c` (SM3, pseudohash, pseudoXOF —
byte-identical to the shipped copy).

Agreements:
- All three instances' (n, q, M) match Table 3 exactly (`params.h`: ZEN_N 512/1024/2048,
  ZEN_Q 769, msg = n/4 bits).
- **The four distributions match Table 3 for all three rows**, which is the fiddliest part
  to get right: ZEN_128 g = `cbd1 + tenary1_8` (B1+S_{1/8}), f = `cbd1` (B1),
  s = `cbd1 + tenary3_32` (B1+S_{3/32}), e = `cbd2` (B2); ZEN_256 g = `tenary3_16`,
  f = `tenary1_8`, s = e = `cbd1`; ZEN_512 g = f = `tenary3_32`, s = e = `tenary1_8`.
  `tenary1_8` computes (a−b)·c over uniform bits, giving Pr[±1] = 1/8 each; `tenary3_32`
  computes ((a∧b) − (c∧d))·e, giving Pr[±1] = 3/32 each — as the S_d notation demands.
- The zero-divisor lift is exactly Algorithm 4 line 6 (`pke.c:96-99`, the four copies at
  i, i+n/4, i+n/2, i+3n/4 scaled by (q+1)/2), and `pke_dec` is exactly Algorithm 5 +
  Algorithm 2 including the branch-free min/compare form of the S_{r,0} vs S_{r,1} test
  (`pke.c:160-186`), with two differences: the comparison result selects the address of
  the store `t2[idx]` (`pke.c:185-186`), and the code tests `c0 <= c1` where Algorithm 2
  line 6 tests `v0 < v1`, so on a tie the code takes x^i and the spec x^{i+n/4}.
- The FO layer is Algorithms 6–8 with a constant-time ciphertext compare and a
  constant-time select between K and K′ (`KEM_AlgorithmInstance.c:165-172`); no early
  return on rejection.
- All randomness comes from the seeded DRNG (`get_random_number(&drng_algorithm, …)` in
  `pke_keygen` and `kem_enc`/`kem_keygen`); there is no `/dev/urandom` path in the tree.
- pk and ct byte sizes reproduce Table 3 exactly for all three rows.

Discrepancies and observations:
1. **(c) deliberate equivalent optimisation — f_inv is computed in the wrong-looking ring,
   but correctly.** Algorithm 3 line 9 says `f_inv ← FastInversion(f, n/2)`, i.e. an
   inverse in F_2[x]/(x^{n/2}+1) (n/2 bits). The implementation reduces f to
   F_2[x]/(x^{n/4}+1) first (`pke.c:32-35`) and inverts there; `ZEN_F2_PACK = n/32` bytes
   = n/4 bits, so **half** the stated key material is stored. This is sound: if
   f·f_inv = 1 + (x^{n/4}+1)w then in F_2[x]/(x^{n/2}+1) the residual term is multiplied
   by (x^{n/4}+1)² = x^{n/2}+1 = 0, so the decoding identity still holds. The spec never
   states this, and a spec-faithful implementation would produce a larger sk.
2. **(a) deviation from the spec's own claim — H_pk and H_K are not domain-separated.**
   §3.2 says the FO functions are "domain-separated SM3-based hash functions". In
   ZEN_128 and ZEN_256 both H_pk and H_K are the *same* call, `sm3hash(256, ·)`, with no
   domain tag: H_pk(pk) = SM3(pk) (`KEM_AlgorithmInstance.c:74`) and
   H_K(κ′, ct) = SM3(κ′ ‖ ct) (`:161`). Only the fixed, differing input lengths separate
   them. (ZEN_512 happens to use `pseudohash` for H_pk and `pseudoXOF` for H_K, so it is
   separated by accident of construction rather than by design.)
3. **(a) deviation — H_pk is truncated to 128 bits in ZEN_128.** `ZEN_SYM_LEN_BYTES` is 16
   there, so the stored h_pk and the h_pk absorbed by H_m are only the first 16 bytes of
   the 256-bit SM3 output (`KEM_AlgorithmInstance.c:74` writes 32 bytes into a 16-byte
   field and the next line overwrites the tail with κ′; `:126` absorbs only
   MSG+SYM bytes). The multi-target/pk-binding hash therefore offers 64-bit collision
   resistance in a row claiming 128-bit security. Still in-bounds memory-wise, but the
   "write 32, use 16" idiom is fragile and the truncation is not in the spec.
4. **(b) spec bug — Algorithm 3 never uses ρ_f.** KeyGen is declared as KeyGen(ρ_g, ρ_f)
   but lines 3 and 8 both read `PRF(ρ_g, N)`; ρ_f is dead. The implementation follows the
   body, not the signature: one 64-byte seed with a single running nonce feeds g, f, and
   the rejection retries (`pke.c:28-65`). Worth fixing in the spec, since as written the
   two-seed API is meaningless.
5. **(b) spec omission — rejection-loop nonce discipline.** Algorithm 3 increments N once
   per retry but does not say the g-loop and the f-loop share N. The implementation shares
   one counter across both loops and both `poly_generate_*` calls, so g and f are never
   derived from the same substream; that is the safe reading, but the spec does not pin it
   and any re-implementation that restarts N at the f loop would produce different keys.
   The implementation also samples f first (`pke.c:28`, nonce 0) and g afterwards
   (`pke.c:55`), the reverse of Algorithm 3's order.
6. **(a) API-contract deviation (minor).** `KEM_AlgorithmInstance.h` documents
   `kem_dec` as returning −1 on unsuccessful decapsulation; the implementation always
   returns 0 and signals failure only through the pseudorandom shared secret. That is the
   right thing for implicit rejection, but it contradicts the shipped header's own
   contract.
7. **(informational)** Three of the six Table 3 rows are not implemented: ZEN-light
   (the 448-byte-ciphertext row motivated at length in §6.2) and the two uncompressed
   `-backup` rows. Only the compressed 128/256/512 rows are built.

Not verified: the NTT/`basemul`/`montgomery_reduce` layer and the 5-coefficient→6-byte
Z_769 packer were read but not tested for edge cases; the CoreSVP and DFR numbers of
Tables 1–3 were not recomputed; `FastInversion` in `poly.c` is a hand-unrolled
bit-sliced rewrite of Algorithm 1 (with instance-specific `mul_in_R2_256/512/1024`
helpers) and was checked only for the ring dimension it works in, not step by step.
