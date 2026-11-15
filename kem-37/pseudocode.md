# kem-37 TriQ-KEM — algorithm summary

TriQ-KEM is an HQC-style code-based IND-CCA2 KEM over `R = F2[X]/(X^n - 1)` with
`n` a primitive prime. Messages are protected by a concatenated Reed–Solomon /
repeated Reed–Muller (RSRM) code; confidentiality comes from quasi-cyclic
syndrome decoding with *per-block* weight constraints (the spec's CW-QCSD
generalisation): 2-CW-QCSD for public-key recovery on `(x,y)`, and
bounded-density-conditioned 3-CW-QCSD for `(r1,r2,e)`. Its three departures from
HQC are unbalanced encryption weights (`wr < we`), a public bounded-density
rejection rule on `r1,r2`, and parameter selection against an attack-aware
*cumulative* DFR rather than the natural DFR. The FO transform with implicit
rejection and a per-ciphertext public salt gives the KEM.

Specification: `kem-37-spec.pdf` (42 pages), section III (Fig. 1, §III-A/B/C),
parameters in section IV (Tables 2–5).

## Parameters

| parameter | TriQ-KEM-128 | -256 | -384 | -512 | meaning |
|---|---|---|---|---|---|
| n | 16301 | 50363 | 97651 | 157627 | ring length (smallest primitive prime > n1n2) |
| n1 | 42 | 64 | 108 | 136 | outer RS length over F256 |
| n2 | 384 | 768 | 896 | 1152 | inner repeated RM length |
| n1·n2 | 16128 | 49152 | 96768 | 156672 | RSRM codeword length |
| k | 128 | 256 | 384 | 512 | RSRM dimension = plaintext bits |
| wx = wy = wr | 67 | 132 | 196 | 260 | secret and encryption-randomness weight |
| we | 106 | 234 | 350 | 475 | encryption error weight |
| δ (RS) | 13 | 16 | 30 | 36 | RS correction capability |
| L_bd | 384 | 768 | 896 | 1152 | bounded-density window length |
| γ | 7 | 9 | 9 | 9 | max support points per window |
| N_max | 256 | 256 | 256 | 256 | bounded-density resampling bound |
| \|salt\| | 32 | 32 | 32 | 32 | bytes |
| \|seed\| | 16 | 32 | 48 | 64 | bytes (= k/8) |
| claimed security | 128 | 256 | 384 | 512 | classical bits (classical ISD 146.7/277.0/404.1/532.1) |
| post-rejection CDFR | 2^-34.3 | 2^-85.6 | 2^-124.9 | 2^-146.6 | spec Table 6 |
| natural DFR | 2^-121.9 | 2^-173.4 | 2^-209.6 | 2^-230.0 | spec Table 6 |

Outer/inner codes (Table 3): `[42,16,27]_256`+`[384,8,192]_2`,
`[64,32,33]_256`+`[768,8,384]_2`, `[108,48,61]_256`+`[896,8,448]_2`,
`[136,64,73]_256`+`[1152,8,576]_2`. F256 uses `x^8+x^4+x^3+x^2+1`.

Sizes (bytes), spec Table 5 vs the built reference library:

| instance | ek spec | ek impl | dk spec | dk impl | ct spec | ct impl | ss | match |
|---|---|---|---|---|---|---|---|---|
| TriQ-KEM-128 | 2054 | 2054 | 2102 | 2102 | 4086 | 4086 | 16 | yes |
| TriQ-KEM-256 | 6328 | 6328 | 6424 | 6424 | 12472 | 12472 | 32 | yes |
| TriQ-KEM-384 | 12255 | 12255 | 12399 | 12399 | 24335 | 24335 | 48 | yes |
| TriQ-KEM-512 | 19768 | 19768 | 19960 | 19960 | 39320 | 39320 | 64 | yes |

All four match exactly (`|ek| = |seed| + ⌈n/8⌉`, `|dk| = |ek| + 3|seed|`,
`|ct| = ⌈n/8⌉ + ⌈n1n2/8⌉ + |salt|`).

## Pseudocode

From spec Fig. 1 and §III-A / §III-C. `Truncate(·, ℓ)` keeps the low `n1n2` bits
(`ℓ = n - n1n2`). `H, G, J` are domain-separated; `I` splits the PKE seed.

### PKE.KeyGen(seed_PKE) → (ek_PKE, dk_PKE)
```
1: (seed_dk, seed_ek) <- I(seed_PKE)
2: x <-$ R_{wx},  y <-$ R_{wy}          # from XOF(seed_dk)
3: h <-$ R                              # from XOF(seed_ek), uniform
4: s <- x + h*y
5: ek_PKE <- (seed_ek, s);   dk_PKE <- seed_dk
```

### PKE.Encrypt(ek_PKE, m, theta) → c_PKE
```
1: regenerate h from seed_ek; parse s
2: r2 <-$ A^{(wr)}_{Lbd,gamma},  e <-$ R_{we},  r1 <-$ A^{(wr)}_{Lbd,gamma}   # from XOF(theta)
3: u <- r1 + h*r2
4: v <- C.Encode(m) + Truncate(s*r2 + e, l)
5: c_PKE <- (u, v)
```

### PKE.Decrypt(dk_PKE, c_PKE) → m'
```
1: regenerate y from dk_PKE = seed_dk
2: m' <- C.Decode( v - Truncate(u*y, l) )
   # = C.Decode( C.Encode(m) + Truncate(x*r2 + r1*y + e, l) ), noise e' = x*r2 + r1*y + e
```

### Bounded-density sampler (§III-B)
```
A^{(w)}_{L,g} = { v in R_w : max_{a in Z_n} wt( v restricted to I_L(a) ) <= g }
repeat up to N_max:  v <-$ R_w ;  accept if v in A^{(w)}_{L,g}
# rejection probability bounded by U_clump(n,w,L,g) of Lemma III.1 (Table 4)
```

### KEM.KeyGen / Encaps / Decaps (§III-C)
```
KeyGen:  seed_KEM <-$ {0,1}^k ;  (seed_PKE, sigma) <- XOF(seed_KEM)
         (ek_PKE, dk_PKE) <- PKE.KeyGen(seed_PKE)
         ek_KEM <- ek_PKE ;  dk_KEM <- (ek_KEM, dk_PKE, sigma, seed_KEM)

Encaps:  m <-$ {0,1}^k ;  salt <-$ {0,1}^{8|salt|}
         (K, theta) <- G( H(ek_KEM) || m || salt )
         c_PKE <- PKE.Encrypt(ek_KEM, m, theta)
         return c_KEM = (c_PKE, salt),  K

Decaps:  m' <- PKE.Decrypt(dk_PKE, c_PKE)
         (K', theta') <- G( H(ek_KEM) || m' || salt )
         c_KEM' <- ( PKE.Encrypt(ek_KEM, m', theta'), salt )
         Kbar <- J( H(ek_KEM) || sigma || c_KEM )
         return K' if c_KEM' == c_KEM else Kbar       # implicit rejection
```

### Concatenated code
```
C.Encode(m) = RM_encode( RS_encode(m) )     # RS over F256, then repeated RM
C.Decode(y) = RS_decode( RM_decode(y) )     # RM by fast Hadamard transform,
                                            # RS by Berlekamp-Massey
```

## Implementation vs specification

Checked: `src/TriQ-KEM-*/src/common/{kem.c, code.c, symmetric.c}`,
`src/ref/{triq_pke.c, vector.c, parsing.c, reed_muller.c, reed_solomon.c}`,
`src/ref/triq-*/parameters.h` (all four instances), and
`KEM_AlgorithmInstance.c` for the RNG plumbing.

**Agreements.** Every parameter in the table above was read out of
`parameters.h` for all four instances and matches spec Tables 2–4 exactly,
including `PARAM_BD_L/GAMMA/N_MAX` and the RS generator coefficients. All
randomness originates from the official `drng_algorithm`:
`KEM_AlgorithmInstance.c:26-31` draws entropy and a personalisation string from
it and seeds the internal PRNG, so `prng_get_bytes` in `kem.c:52,117,118` is
DRNG-derived. The FO transform matches Fig. 1 term for term, including the salt
in `G`'s input and in the re-encryption comparison, and `J` binding
`H(ek_KEM) || sigma || c_KEM`. Domain separation is real: `symmetric.h:28-33`
gives `G/H/I/J` distinct domain bytes 0/1/2/3 and the PRNG/XOF domains 0/1.
`triq_pke.c:113-115` applies bounded-density sampling to `r2` and `r1` only and
plain fixed-weight sampling to `e`, as §III-A specifies, and in the sampling
order the spec's §III-A display gives (`r2, e, r1`). `UTILS_REJECTION_THRESHOLD
= 16773729 = ⌊2^24/16301⌋·16301` is the correct unbiased-rejection bound.
`code.c:27-53` is exactly RS-then-RM encode / RM-then-RS decode.

The decapsulation mask logic (`kem.c:205-211`) is correct: `vect_compare`
(`vector.c:294-302`) is branch-free and returns strictly 0 or 1, and
`triq_pke_decrypt` returns a constant 0, so `result` is always in {0,1} and
`result -= 1` yields exactly `0x00` or `0xFF`. (Had any of them been able to
return ≥ 2, the final loop would have blended `K'` and `K̄` byte-wise — worth
recording as a fragile idiom that happens to be safe here.)

Ciphertext parsing (`parsing.c:63-67`) does not mask the 3 unused high bits of
`u`'s last byte, but the FO check compares the full `⌈n/8⌉` bytes against a
recomputed `u'` whose padding is zero, so non-canonical ciphertexts are
*rejected* rather than accepted — no malleability.

**(a) Real deviation — the encryption vectors are not sampled uniformly, though
the spec says they are.** Two different fixed-weight samplers ship:
- `vect_generate_random_support1` (`vector.c:61-92`) uses 24-bit rejection
  sampling against `UTILS_REJECTION_THRESHOLD` followed by true rejection of
  duplicates. This is unbiased, and it is used for `x` and `y`
  (`triq_pke.c:47-48`) and to regenerate `y` at decryption (`parsing.c:22`).
- `vect_generate_random_support2` (`vector.c:103-123`) uses the HQC
  multiply-shift construction `support[i] = i + ((rand32 * (n-i)) >> 32)` with
  the constant-time "replace collision by `i`" fix-up. It is used for `r1`, `r2`
  and `e` (`triq_pke.c:113-115`). This is *not* uniform over `R_w`: it carries a
  multiply-shift modulo bias of order `n/2^32` per draw, and the
  collision-replacement step skews the support distribution. §III-B states
  "The sampler first draws a fixed-weight candidate **uniformly from R_w** …
  Conditioned on acceptance before the attempt limit, its output is **uniform
  over A^{(w)}_{Lbd,γ}**" — the accepted set is therefore not uniformly sampled
  as claimed. This is inherited from HQC's reference code and the bias is small,
  but it is a concrete mismatch with an explicit spec sentence, and it matters
  more here than in HQC because the bounded-density argument and the
  entropy-loss correction `λ_bd` in the §V-C ISD estimate are both stated for
  the uniform distribution. Using the unbiased sampler for the long-term secret
  and the biased one for the encryption randomness is a deliberate speed choice
  that the specification does not mention at all.

**(a) Minor deviation — the bounded-density check is not branch-free as claimed.**
§III-B asserts "The bounded-density check itself is constant-time: it computes
the maximum window count across all support positions using branch-free
arithmetic … This property is preserved in both the reference and the optimized
implementations." In `vector.c:216-218` the cyclic distance is computed with a
data-dependent ternary
```c
uint32_t dist = (support[j] >= start) ? (support[j] - start)
                                      : (PARAM_N - start + support[j]);
```
Only the max-tracking at lines 221-223 is written branch-free. The loop *bounds*
are fixed at `weight × weight`, so total cost does depend only on `w` as the spec
requires; whether the ternary becomes a `cmov` is left to the compiler. Low
severity, but the source does not match the stated property.

**(c) Deliberate equivalent — exhausting `N_max` keeps the last candidate.**
`vect_sample_fixed_weight_bd` (`vector.c:244-256`) breaks on acceptance and
otherwise falls through after `N_max` attempts, writing the final (rejected)
support. The spec only says the loop "repeats … until an upper bound on attempts
is reached" without prescribing the fallback. The event is unreachable in
practice: §IV-B bounds it by `U_clump^256 ≤ 0.0416^256 ≈ 2^-1175`. The code
comment states the intent (deterministic behaviour given the XOF context).

**(c) Note — `s` is not masked on public-key parse.** `parsing.c:41` copies
`⌈n/8⌉` bytes of `s` without clearing the unused high bits. Only a malformed
encapsulation key (the key owner's own) is affected; it is not reachable by a
CCA adversary.

**Not verified.** The CDFR / natural-DFR figures of Table 6 and the ISD and
quantum-Prange estimates of §V-C were not re-derived. `reed_muller.c` (FHT) and
`reed_solomon.c` (Berlekamp–Massey) were confirmed to be the standard HQC-derived
routines with the spec's `PARAM_DELTA` correction capability, but their decoding
radius was not independently checked. Only TriQ-KEM-128 was read line by line;
the other three were checked at the parameter-header level and share the same
source files.
