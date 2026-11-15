# sign-01 Aigis-Sig+ — algorithm summary

Module-lattice Fiat–Shamir-with-aborts signature (a Dilithium variant) whose
security reduces to the *asymmetric* MLWE / MSIS / SelfTargetAMSIS problems
(different noise bounds η1 for s1 and η2 for s2, hence "asymmetric"). Signing uses
rejection sampling plus two extra "enhanced" rejection conditions (on `v = r0+ct0`
and on the hint weight); verification uses Dilithium-style hints.

Specification: `sign-01-spec.pdf` (91 physical pages; doc page = physical − 3).
Scheme description §3.2 (doc p.12–13); parameters §4 Table 1/2 (doc p.14–15);
encoding §5.3 Algorithms 8–21 (doc p.19–25); full pseudocode §5.5 Algorithms
29–31 (doc p.29–31). Hash instantiation Table 3 (doc p.19).

## Parameters

| parameter | PARAMS I (Aigis-sig1) | PARAMS II (Aigis-sig2) | PARAMS III (Aigis-sig3) | meaning |
|---|---|---|---|---|
| n | 512 | 512 | 512 | ring degree, R = Z[X]/(X^n+1) |
| q | 4171777 | 4171777 | 4171777 | modulus = 2^22 − 11·2^11 + 1 |
| (k, ℓ) | (2, 2) | (4, 4) | (8, 7) | module dimensions of A ∈ R_q^{k×ℓ} |
| d | 15 | 15 | 13 | Power2Round drop bits |
| (η1, η2) | (1, 5) | (1, 1) | (1, 1) | secret bounds for s1, s2 |
| τ (PARAM_C) | 24 | 44 | 118 | non-zero ±1 coefficients of c |
| (β1, β2) | (24, 120) | (44, 44) | (118, 118) | bounds on ‖cs1‖∞, ‖cs2‖∞ |
| γ1 | 2^14 = 16384 | 2^16 = 65536 | 2^19 = 524288 | mask range, y ∈ S̄_{γ1}^ℓ |
| γ2 | (q−1)/12 = 347648 | (q−1)/12 = 347648 | (q−1)/8 = 521472 | α = 2γ2 decompose step |
| γ3 | 504000 | 575000 | 612000 | bound on v = r0 + c·t0 |
| ω | 72 | 176 | 102 | max total hint weight |
| A_ω | B_{64,15,ω} | B_{64,15,ω} | B_{64,15,ω} | ≤15 ones per 64 coeffs **and** ≤ω total |
| (κ1, κ2) | (32, 48) | (32, 48) | (64, 96) | seed / CRH byte lengths |
| H, CRH, XOF2 | SHA3-256, XOF-256(·,48) | SHA3-256, XOF-256(·,48) | SHA3-512, XOF-512(·,96) | Table 3 (see discrepancy 1) |
| expected repetitions | 6.41 | 5.16 | 5.71 | spec Table 2 |
| claimed security (C/Q) | 134/117 AMLWE primal, 129/113 AMSIS-SUF | 287/252, 274/241 | 561/494, 551/484 | spec Table 2, bits |

Sizes (bytes), specification vs the built reference library
(OBSERVED `sign-01.txt`; spec Table 2 gives |pk| and |σ| only, |sk| recomputed
from Algorithm 18):

| instance | pk spec | pk impl | sk spec | sk impl | sig spec | sig impl | match |
|---|---|---|---|---|---|---|---|
| Aigis-sig1 | 928 | 928 | (2800) | 2800 | 2015 | 2015 | yes |
| Aigis-sig2 | 1824 | 1824 | (4976) | 4976 | 4533 | 4533 | yes |
| Aigis-sig3 | 4672 | 4672 | (8800) | 8800 | 9134 | 9134 | yes |

The signature figure is the **maximum**: `SIG_MAX_SIZE_PACKED`. Signatures are
variable-length (spec §5.3: "The variable-length output of HintPack leads to
non-fixed signature lengths"); actual signatures are shorter (2004 / 4512 / 9106
observed in `security_findings.md`). `SIG_MIN_SIZE_PACKED = ℓ·⌈n·bitlen(2γ1)/8⌉ + κ1 + 1`.

## Pseudocode

### KeyGen  (spec Algorithm 29, §5.5)
```
ξ  <-$ B^κ1
(rnd, ρ, K) := XOF2(ξ, 3κ1)                     # three κ1-byte seeds
(s1, s2) := ExpandS(rnd)   in S_{η1}^ℓ × S_{η2}^k   # Alg. 28 / RejEta1, RejEta5
Â := ExpandA(ρ)  in R_q^{k×ℓ}                  # Alg. 23, Parse = rejection on 22-bit words
t  := NTT^{-1}(Â o NTT(s1)) + s2                # t = A s1 + s2
(t1, t0) := Power2Round_q(t, d)                 # Alg. 1, t = t1·2^d + t0
pk := pkPack(ρ, t1)                             # Alg. 16
tr := CRH(pk)  in B^κ2
sk := skPack(ρ, K, tr, s1, s2, t0)              # Alg. 18
return (pk, sk)
```

### Sign  (spec Algorithm 30, §5.5; scheme description §3.2)
```
(ρ, K, tr, s1, s2, t0) := skUnpack(sk);  Â := ExpandA(ρ)
µ := CRH(tr || M)   in B^κ2 ;   N := 0 ;   (z,h) := ⊥
while (z,h) = ⊥:
    y := ExpandMask(K, µ, N)  in S̄_{γ1}^ℓ ;  N := N + ℓ
    w := NTT^{-1}(Â o NTT(y))                       # w = A y
    (w1, w0) := Decompose_q(w, 2γ2)                 # Alg. 2
    c~ := H(µ, w1) ;  c := SampleInBall(c~) in B_τ  # Alg. 24, Fisher-Yates
    z  := y + c·s1 ;  r0 := w0 − c·s2
    if z ∉ S̄_{γ1−β1}^ℓ  or  r0 ∉ S̄_{γ2−β2−η1}^k : (z,h) := ⊥ ; continue
    v := r0 + c·t0
    h := MakeHint_q(v, w1, 2γ2)                     # Alg. 3
    if v ∉ S̄_{γ3}^k  or  h ∉ B_{64,15,ω} : (z,h) := ⊥
(σ, siglen) := sigPack(z, c~, h)                    # Alg. 20
return (σ, siglen)
```

### Verify  (spec Algorithm 31, §5.5)
```
(ρ, t1) := pkUnpack(pk) ;  (z, c~, h) := sigUnpack(σ)
c := SampleInBall(c~) ;  Â := ExpandA(ρ)
µ  := CRH(CRH(pk) || M)
u  := NTT^{-1}(Â o NTT(z)) − c·t1·2^d               # u = A z − c t1 2^d
w1' := UseHint_q(h, u, 2γ2)                          # Alg. 4
c~' := H(µ, w1')
return 1 iff  z ∈ S̄_{γ1−β1}^ℓ  and  c~' = c~
          and u − w1'·2γ2 ∈ S̄_{γ3}^k  and  h ∈ B_{64,15,ω}
```

### Signature encoding  (spec Algorithms 20/21 + 14/15, §5.3) — where the defect lives
```
sigPack(z, c~, h):
  σ := || _{i<ℓ} Encode_{bitlen(2γ1)}(z[i], γ1, n)      # ℓ · n·SZBITS/8 bytes, fixed
  σ := σ || c~                                          # κ1 bytes, fixed
  (b, hlen) := HintPack(h) ; σ := σ || b                 # VARIABLE length
  siglen := n·ℓ·bitlen(2γ1)/8 + κ1 + hlen

HintPack(h):                       # u := n/64 = 8 blocks per polynomial
  y[]  := positions (0..63) of the set hint bits, in order         (index of them)
  t[iu+j] := number of set bits in block j of h[i]                 (4 bits each)
  max  := 1 + index of the LAST non-zero block   (0 if h = 0)
  b := IntegerToBytes(max,1) || Encode_4(t, 0, max) || Encode_6(y, 0, index)
  hlen := 1 + ⌈max/2⌉ + ⌈6·index/8⌉

HintUnpack(b):
  max := b[0]                                # 1 byte, ATTACKER-CONTROLLED, UNBOUNDED
  t   := Decode_4(b[1 : max+1], 0, max)      # max nibbles
  index := Σ t[i]
  y   := Decode_6(b[max+2 : ...], 0, index)  # NOTE: spec offset max+2, code uses 1+⌈max/2⌉
  set h[z/u][64·(z mod u) + y[r]] := 1 for each block z < max, t[z] entries
```
The legal range is `max ≤ k·n/64` (16, 32, 64) and `index ≤ ω`; **neither the spec's
HintUnpack nor the implementation enforces either bound**, and nothing re-derives
`hlen` from the decoded hint to compare with the supplied signature length.

### Hashes
`H(M, d)`, `CRH(M)`, `XOF1/XOF2(M, d)` per Table 3: XOF-128/256/512 and
SHA3-256/512 at the nominal level; `Parse` rejection-samples 22-bit words < q,
`RejEta1` maps base-4 digits {0,1,2} to {1,0,−1}, `SampleInBall` is Fisher–Yates
over n = 512 positions driven by the squeezed stream.

## Implementation vs specification

Checked against the built sources (Makefile: `Implementations/Implementations/Reference_Implementation/Aigis-Sig+-{I,II,III}`,
`-DPARAMS={1,2,3} -DUSE_ICCS`): `sign.c` (KeyGen/Sign/Verify = Alg. 29–31),
`packing.c` (Alg. 14–21), `poly.c`/`polyvec.c` (Alg. 1–4, 22–28, `challenge`),
`ntt.c`, `pspm.c` (§5.1 PSPM), `rounding.c`, `hashkdf.c`/`auxfunc.c` (Table 3).

Agreements: all spec Table 1 constants sampled against `src/Aigis-sig1/params.h`
match — `PARAM_N 512`, `PARAM_Q 4171777`, `(PARAM_K,PARAM_L)` = (2,2)/(4,4)/(8,7),
`PARAM_D` 15/15/13, `PARAM_C` 24/44/118, `GAMMA1` 16384/65536/524288,
`GAMMA2` 347648/347648/521472 (= (q−1)/12, (q−1)/8), `GAMMA3` 504000/575000/612000,
`OMEGA` 72/176/102, `(ETA1,ETA2)` (1,5)/(1,1)/(1,1), `(SEEDBYTES,CRHBYTES)` =
(32,48)/(32,48)/(64,96). All six derived sizes (`PK_/SK_/SIG_MAX_SIZE_PACKED`)
evaluate to exactly the OBSERVED values. The seed chain
(`msig_keygen` `sign.c:37-40` = XOF2(ξ,3κ1)), `tr = CRH(pk)` (`sign.c:61`),
`µ = CRH(tr‖M)` (`sign.c:98`), `ExpandMask(K,µ,N)` (`polyvec.c:199-213` absorbs
the full `K‖µ` = SEEDBYTES+CRHBYTES plus a 2-byte nonce) and
`c~ = H(µ‖w1)` (`polyvec.c:35-45`, `polyvec.c:104-114`) all match Alg. 29/30.
All three rejection conditions of Alg. 30 are present (`sign.c:122,127,132`) and
the hint-weight abort `n > OMEGA` is at `sign.c:138`.

Discrepancies:

1. **(b/c) Hash instantiation is not the one in Table 3.** The build defines
   `-DUSE_ICCS`, so `hashkdf.c:8-45` routes every one-shot XOF/CRH call to `pseudoXOF()`
   and every `hash256/512/1024` to `sm3hash()`/`pseudohash()` in `auxfunc.c`
   (the ICCS SM3/HMAC-SM3 competition auxiliary functions), and the
   absorb/squeeze streams of `Parse`, `RejEta` and `SampleInBall` go through
   `kdf_init`/`kdf_squeezeblocks` (`hashkdf.c:82-89`) to the ICCS DRBG
   (`init_random_number`/`get_random_number`, sign-01-7), not to the
   SHA3-256/512 and XOF-128/256/512 that spec Table 3 specifies. This is the
   competition-mandated common-primitive substitution rather than a coding
   error, but the specification does not document it; the shipped KATs are
   consistent with the ICCS build (see `RESULTS.md` on the second, older-format
   `*_iccs.txt` KAT set).

2. **(a) Verify omits the `h ∈ B_{64,15,ω}` check of Algorithm 31 line 9.**
   `msig_verf` (`sign.c:147-205`) checks `‖z‖∞`, `c~' = c~` and the `γ3` norm, but
   never checks the hint's per-64-block weight (≤15) or its total weight (≤ω);
   `unpack_h` (`packing.c:303-336`) simply sets whatever bits the encoding names.
   Combined with the spec's own variable-length `HintPack`, the encoding is
   non-canonical: `max` may over-declare trailing all-zero blocks, `t`/`y` may
   name duplicate positions, and the unused high nibble of the last `Encode_4`
   byte and any trailing bytes up to `SIG_MAX_SIZE_PACKED` are unconstrained.
   This is the already-recorded SUF-CMA violation
   (`security_findings.md` LH-SIGN-01-001/002/003, `sig-signature-flip` accepted
   at the last signature byte of all three instances) and the `sig-append`
   observation; `sign.c:159` only range-checks `smlen` between
   `SIG_MIN_SIZE_PACKED` and `SIG_MAX_SIZE_PACKED` and never re-derives `hlen`
   from the decoded hint.

3. **(a) `unpack_h` stack overflow — already recorded.** `packing.c:307`
   (`int t[PARAM_K * PARAM_N / SEC]`, i.e. 16/32/64 entries) and
   `packing.c:311` (`int max = sm[0]`) let an attacker-controlled byte up to 255
   drive `unpack4bits(t, sm, max)` (`packing.c:318-319`) past the end of `t`,
   then `k = Σ t[i]` (up to 255·15) past the end of `pos[OMEGA]`
   (`packing.c:325`), and finally `i = k/(PARAM_N/SEC)` past `h->vec[PARAM_K]`
   (`packing.c:330-334`). Reproduced as a SIGABRT at Aigis-sig1 signature bit
   15617. The spec's Algorithm 15 is equally unbounded, so the root cause is a
   specification gap, not only a coding slip.

4. **(b) Minor: `HintUnpack` offset.** Spec Alg. 15 line 13 reads `y` from
   `b[max+2 : ...]`, which is only correct when `max` is even; the implementation
   uses the consistent `1 + ⌈max/2⌉` advance returned by `unpack4bits`
   (`packing.c:319`). The implementation is right and the spec text is off by
   `⌈max/2⌉ − max` — a spec typo, not a deviation.

5. **(non-security) `msig_verf` leaks `buf`** on the two failure returns after
   `malloc` (`sign.c:194`, `sign.c:201`); only the success path reaches
   `free(buf)` at `sign.c:203`.

6. **(a) `SampleInBall` signs — recorded as sign-01-5.** In the `PARAM_C <= 64`
   branch of `unpack_c` (sets I and II) the loop sets `signs = 0` after the
   first coefficient instead of shifting (`polyvec.c:98-99`), so every later
   non-zero coefficient is set to +1; Alg. 24 gives each its own sign bit.

7. **(a) `ExpandMask` extra unpack — recorded as sign-01-4.**
   `polyvecl_uniform_gamma1` calls `polyz_unpack(v->vec + i, outbuf)` once more
   after its loop, with `i == PARAM_L` (`polyvec.c:214`).

8. **(a) Hash input length — recorded as sign-01-9.** `kdf128/256/512` take
   `int inlen` and pass `inlen * 8` to `pseudoXOF` (`hashkdf.c:8-45`); signing
   and verification pass `CRHBYTES + mlen` (`sign.c:98,174`).

Not verified: the NTT/PSPM constants and the `Parse` byte-for-byte
output (KAT PASS for all three instances is the only evidence here); the spec's
concrete AMLWE/AMSIS security estimates in Table 2 were not recomputed; the
AVX2/NEON implementations were not examined (not built).
