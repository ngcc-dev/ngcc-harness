# sign-07 CS ("Compact-Simple") — algorithm summary

**Bimodal** Fiat–Shamir-with-aborts lattice signature over R_q = Z_q[x]/(x^n+1),
working modulo 2q with a sign flip `qj` (as in BLISS/HAETAE), whose security rests
on decisional MLWE (key recovery) and MSIS / BimodalSelfTargetMSIS (forgery). Two
compactness tricks distinguish it: the public key carries only `b1 = HighBits(b, β)`,
and the response polynomials `z0, z1` plus the hint `h` are **rANS entropy-coded**.
Masks come from a centered *multinomial* distribution D_{a,b}, and the rejection
probability is approximated with integer arithmetic only (no floating point).

Specification: `sign-07-spec.pdf` (40 physical pages; doc page = physical − 1).
Full scheme §3.3 Algorithms 10/11/12 (doc p.17–20); simplified scheme §3.2
Algorithms 7–9; hint high/low bits Def. 2.1/2.2 §2.5; rANS Def. 2.3 and
Algorithms 2/3 §2.6; rejection sampling §2.7 + §3.1 Algorithms 4–6; parameters
§3.5 Table 3 (doc p.23); sizes §5.1 (doc p.33).

## Parameters

| parameter | CS-128 | CS-256 | CS-512 | meaning |
|---|---|---|---|---|
| λ | 128 | 256 | 512 | security parameter |
| n | 256 | 512 | 512 | ring degree |
| q | 32257 | 64513 | 64513 | prime modulus, 2n \| (q−1); arithmetic mod 2q |
| (k, ℓ) | (3, 3) | (3, 3) | (6, 5) | rows/cols of A′ ∈ R_q^{k×ℓ} |
| η | 1 | 1 | 1 | secret-key coefficient bound, s1∈S^ℓ, s2∈S^k |
| (a0, b0, B0) | (5, 5, 116) | (6, 6, 278) | (6, 6, 261) | mask y0 ∈ R, D_{a0,b0}; norm bound |
| (a1, b1, B1) | (10, 9, 2779) | (9, 10, 5385) | (12, 11, 12793) | mask y1 ∈ R^ℓ |
| (a2, b2, B2) | (6, 9, 1962) | (9, 9, 2569) | (8, 11, 9771) | mask y2 ∈ R^k |
| (τ, τ′) | (23, 4) | (44, 3) | (118, 6) | challenge weight; iterative-bimodal step |
| N = ⌈τ/τ′⌉ | 6 | 15 | 20 | number of RejectSample iterations |
| β | 32 = 2^5 | 128 = 2^7 | 32 = 2^5 | public-key compression base |
| α | 2016 | 8064 | 4032 | hint / HighBits step, α \| 2(q−1) |
| (d0, d1) | (0, 6) | (2, 6) | (2, 7) | low bits of z0 / z1 kept uncompressed |
| entropy of c | 131.08 | 256.4 | 512.18 | bits, spec Table 3 |
| repetition count M | 1.75 | 1.96 | 2.00 | spec Table 3 |
| claimed security (C/Q) | 131.4/115.3 MLWE primal; 128.8/113.1 MSIS ∞ | 285.6/250.7; 267.5/234.8 | 518.9/455.5; 548.7/481.6 | spec Table 4, bits |

Sizes (bytes), specification (§5.1, doc p.33) vs the built reference library
(OBSERVED `sign-07.txt`):

| instance | pk spec | pk impl | sk spec | sk impl | sig spec | sig impl | match |
|---|---|---|---|---|---|---|---|
| CS-128 | 976 | 976 | 1888 | 1888 | 1548 | 1548 | yes |
| CS-256 | 1760 | 1760 | 3968 | 3968 | 3164 | 3164 | yes |
| CS-512 | 4288 | 4288 | 7808 | 7808 | 5975 | 5975 | yes |

The signature is a **fixed-size** buffer even though the rANS payloads are
variable-length; the slack is zero padding (see below).

## Pseudocode

### KeyGen  (spec Algorithm 10, §3.3)
```
ρ  <-$ {0,1}^{2λ}
(ρ0, ρ1, K) := H(ρ, k, ℓ)  in {0,1}^λ × {0,1}^{2λ} × {0,1}^λ
A' := ExpandA(ρ0)   in R_q^{k×ℓ}
(s1, s2) := ExpandS(ρ1)   in S^ℓ × S^k
b  := A' s1 + s2    in R_q^k
b1 := HighBits(b, β),  b0 := LowBits(b, β)      # so A s = q j + 2 b0  mod 2q
A  := ( q·j − 2β·b1 | 2A' | 2 I_k )  in R_{2q}^{k×(1+ℓ+k)}
s  := (1, s1^T, s2^T)^T
pk := (ρ0, b1) ;  tr := H(pk) in {0,1}^{2λ}
sk := (s1, s2, K, tr, ρ0, b0)
```

### Sign  (spec Algorithm 11, §3.3)
```
A' := ExpandA(ρ0) ;  A := (q j − 2β b1 | 2A' | 2 I_k)
µ  := H(tr, M)  in {0,1}^{2λ} ;   κ := 0
ρ' := H(K, µ)                     # randomized variant: ρ' <-$ {0,1}^{2λ}
repeat
    y0 := ExpandMask(ρ', κ, a0, b0)   ~ D_{a0,b0}      in R
    y1 := ExpandMask(ρ', κ, a1, b1)   ~ D_{a1,b1}      in R^ℓ
    y2 := ExpandMask(ρ', κ, a2, b2)   ~ D_{a2,b2}      in R^k
    y  := (y0, y1^T, y2^T)^T
    w  := A y   mod 2q
    c~ := H( µ, HighBits_h(w, α), LSB(y0) )   in {0,1}^{2λ}
    c  := SampleInBall(c~, τ)                 # Alg. 1, coefficients in {0,±1}, weight τ
    (z, c_r) := RejectSample(s, y, c, τ')     # Alg. 6: N = ceil(τ/τ') bimodal steps,
                                              # integer-only acceptance test (Alg. 4/5)
    if z = ⊥ : κ++                            # expected M ≈ 1.75 / 1.96 / 2.00 iterations
until z ≠ ⊥
h  := HighBits_h(w, α) − HighBits_h(w − 2 z2 + 2 c_r b0, α)   mod± 2(q−1)/α
σ0 := rANS.Encode(z0) ;  σ1 := rANS.Encode(z1)
return σ = (σ0, σ1, h, c~)
```

### Verify  (spec Algorithm 12, §3.3;  B2' = B2 − τ + α/4 + 1 + τβ/2)
```
A' := ExpandA(ρ0) ;  A0 := q j − 2β b1 ;  A1 := 2A'
µ' := H( H(pk), M )
z0' := rANS.Decode(σ0) ;  z1' := rANS.Decode(σ1)
c'  := SampleInBall(c~, τ)
v   := A0 z0' + A1 z1' − q c' j              mod 2q
w'  := HighBits_h(v, α) + h                  mod 2(q−1)/α
w'' := LSB(z0' − c')
z2' := ( α·w' + w''·j − v  mod 2q ) / 2      mod± q
accept iff  ‖z0'‖∞ ≤ B0 − ⌈τ/τ'⌉  and  ‖z1'‖∞ ≤ B1 − τ
        and ‖z2'‖∞ ≤ B2'  and  c~ = H(µ', w', w'')
```

### Signature encoding (implementation `encodings.c`; the spec fixes no byte layout)
Fixed length `SIGNATUREBYTES = HBYTES + POLYZ0L + POLYZ0H + ℓ·POLYZ1L + POLYZ1H + POLYZ2`:
```
[0                     : HBYTES)          c~                       (λ/4 bytes)
[HBYTES                : +POLYZ0L)        LowBits_{d0}(z0), BitPack (absent when d0 = 0)
[..                    : +POLYZ0H)        cnt0 (2-byte LE) || rANS(z0 or HighBits z0) || 0-pad
[..                    : +ℓ·POLYZ1L)      LowBits_{d1}(z1[i]) for i < ℓ, BitPack
[..                    : +POLYZ1H)        cnt1 (2-byte LE) || rANS(HighBits_{d1} z1) || 0-pad
[..                    : +POLYZ2)         cnt2 (2-byte LE) || rANS(h) || 0-pad
```
`POLYZ0H/POLYZ1H/POLYZ2` are hard-coded worst-case budgets (240/545/155,
405/1250/165, 404/2135/940). The encoder zero-fills each field first and aborts
the signing attempt if `cnt > FIELD − 2`; the **decoder reads `cnt` and validates
nothing about it or about the padding**. That is where the recorded defects live.

### Hash
`H(OUT, OUTLEN, IN, INLEN) = pseudoXOF(IN, OUTLEN)` (`symmetric.h:10`) — the ICCS
SM3/HMAC-SM3-based competition XOF in `auxfunc.c`. `ExpandMask`, `tr`, `µ`, `ρ'`,
the key-generation seed expansion and the challenge hash call it. `ExpandA`,
`ExpandS`, `SampleInBall` and the `SampleSign` coins instead use the ICCS DRBG
(`H_Init`/`H_Squeeze`/`G_Init`/`G_Squeeze` = `init_random_number`/`get_random_number`,
`symmetric.h:13-16`).

## Implementation vs specification

Checked against the built sources (Makefile: every `.c` in
`Implementations/Reference_Implementation/CS-{128,256,512}` except `KAT_SIG.c`,
with `-fcommon` and a `patches/common/NTT.h` stand-in — see `RESULTS.md`):
`cs.c` (KeyGen/Sign/Verify = Alg. 10–12), `encodings.c` (pk/sk/sig packing, rANS),
`sampling.c` (ExpandA/ExpandS/ExpandMask/SampleInBall/RejectSample),
`bits-hints.c` (Def. 2.1/2.2 HighBits/LowBits/hint), `conversion.c`, `ntt.c`,
`params.h`.

Agreements: **every** constant of spec Table 3 matches `params.h` for all three
instances — `q` 32257/64513/64513, `n` 256/512/512, `(k,l)` (3,3)/(3,3)/(6,5),
`eta` 1, `(a0,b0,B0)`, `(a1,b1,B1)`, `(a2,b2,B2)`, `(tau,taup)` (23,4)/(44,3)/(118,6),
`alpha` 2016/8064/4032, and `d0/d1`. The spec's β = 32/128/32 is stored as its
**log**: `#define beta 5/7/5`, used as a shift (`ShiftLeftVector(t1, beta)`,
`cs.c:83`) and as `1 << (beta-1)` in the verifier bound — consistent, not a
mismatch. `N = ((tau + taup - 1)/taup)` (`conversion.h:6`) is exactly the spec's
⌈τ/τ'⌉. All three `PUBLICKEYBYTES`/`SECRETKEYBYTES`/`SIGNATUREBYTES` expressions
evaluate to the OBSERVED and §5.1 values. The transcript
(`tr = H(pk)`, `µ = H(tr‖M)`, `ρ' = H(K‖rnd‖µ)`, `c~ = H(µ ‖ w1Encode(HighBits_h w) ‖ LSB(z0))`,
`cs.c:79-95,113-118`) matches Alg. 11 lines 3–11, except that `ρ'` also absorbs the
random `rnd` (Alg. 11 line 5 gives `H(K, µ)` or a uniform `ρ'`) and `ExpandMask`
copies only the first `SEEDBYTES` = λ/8 bytes of the `HBYTES` = λ/4-byte `ρ'`
(`sampling.c:673`; sign-07-6); and the verifier reconstructs the
same absorption (`cs.c:171-199`). The verifier's three norm checks
(`cs.c:165`, `cs.c:206`) are exactly Alg. 12 line 12 with
`B2' = B2 − tau + alpha/4 + 1 + tau·2^{beta−1}`.

Discrepancies:

1. **(a) Non-canonical signature encoding — the recorded SUF-CMA violation.**
   `sigEncode` (`encodings.c:1451-1496`) zero-fills each of the three fixed-size
   rANS fields and writes a 2-byte length; `sigDecode` (`encodings.c:1509-1547`)
   reads that length and decodes exactly that many bytes but **never requires the
   remaining bytes of the field to be zero**. Every byte from `cnt+2` to the end
   of each field is free, so a single flipped padding byte — e.g. the last byte of
   the signature, inside the `POLYZ2` (hint) field — yields a distinct, accepted
   encoding of the same (M, σ). This is `security_findings.md`
   LH-SIGN-07-001/002/003 (`sig-signature-flip` accepted on all three instances)
   and the root cause of the `sig-append` result. The specification is silent on
   the byte layout, so this is an implementation defect that the spec's §4.2
   computational-unique-response / sUF-CMA claim does not survive.

2. **(a) Unbounded attacker-controlled rANS byte count — the recorded
   verifier-memory-safety defect.** In `sigDecode`, `cnt` is declared
   `int32_t cnt = 0` and filled by `memcpy((int32_t*)&cnt, sig, 2)`, i.e. any
   value in [0, 65535]; it is then passed straight to
   `decode_rans(..., len = cnt, ...)` (`encodings.c:1521/1527, 1538, 1545`), which sets
   `buf_end = buf + len` and walks the stream. The *encoder* checks
   `cnt > POLYZ0H_PACKEDBYTES - 2` etc. (`encodings.c:1472, 1484, 1491`); the
   decoder has no matching check against its enclosing fixed field, so decoding
   runs off the end of the signature buffer into adjacent stack storage. The
   `RansDecVerify` / `size_used != len` checks (`encodings.c:1431-1438`) happen
   only *after* the walk and therefore do not prevent the over-read. Reproduced as
   `stack smashing detected` (SIGABRT) in the exhaustive bit sweep.

3. **(a) The ABI's signature length is ignored.** `CS_Verify` takes
   `const uint8_t sig[SIGNATUREBYTES]` (`cs.c:154`) with no length parameter, so
   the shim's `sn_len_bytes` is never enforced — matching the recorded
   `api-length-validation` finding.

4. **(b) The specification does not fix the hash, the XOF, or any serialization.**
   §2.3 only says "a hash function"; Algorithms 10–12 write `H(·)` with varying
   arities (`H(ρ, k, l)`, `H(tr, M)`, `H(µ, w', w'')`) and never define the
   concatenation or domain separation. The implementation resolves all of them to
   the ICCS `pseudoXOF` over a fixed-layout buffer (`symmetric.h:10`,
   `cs.c:88-95`). The scheme as specified is therefore under-determined; the
   Fiat–Shamir transcript could not be checked against the document, only for
   signer/verifier self-consistency (which holds).

5. **(non-security) `malloc`'d `buf` leaks** on every early `return false` in
   `CS_Verify` (`cs.c:163, 166, 200, 207`) and on the `return 1` in `CS_Sign`
   (`cs.c:122`); only the success paths reach `free(buf)`.

6. **(a) `ExpandS` nonce — recorded as sign-07-4.** `RejBoundedPoly` writes the
   2-byte nonce at `rho[HBYTES]` but seeds the DRBG with only `SEEDBYTES + 2`
   bytes (`sampling.c:248-252`), and `HBYTES = 2·SEEDBYTES`, so the nonce is not
   absorbed and all `l + k` calls from `ExpandS` (`sampling.c:300-309`) use the
   same seed.

7. **(a) Message length — recorded as sign-07-5.** `CS_Sign` and `CS_Verify` take
   `int len` (`cs.c:65-68,151-154`) and the `H` macro computes `(INLEN) * 8` in
   `int` (`symmetric.h:10`).

8. **Build-level, already recorded in `RESULTS.md`:** `ntt.c` includes a
   `NTT.h` that is absent from the submission (only `ntt.h` ships, and it declares
   `MultiplyNTT` non-static while `ntt.c` defines it static), and `symmetric.h`
   relies on a tentative definition of `DRNG_ctx drng_algorithm`, so the build
   needs `-fcommon`. The reference implementation as submitted does not compile
   unmodified on any current toolchain.

Not verified: the `RejectSample` integer approximation (Alg. 4–6) against the
spec's `M0[]` acceptance tables and Theorem 3.2, the rANS frequency tables
(`esyms_*`, `symbol_*`) against Def. 2.3, the NTT constants, and the concrete
security estimates of Table 4. KAT PASS for all three instances is the only
end-to-end evidence for those.
