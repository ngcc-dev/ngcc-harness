# kem-40 YuanYang.KEM — algorithm summary

NTRU-paradigm lattice KEM over R = Z[X]/(X^d+1): an IND-CPA PKE whose public key
is h = g/f mod q (decisional NTRU) and whose ciphertext masks the message with a
Ring-LWE term h·s + e, made IND-CCA by an FO-style transform with **implicit
rejection**. Two distinctive features: a *small masking modulus* p = 1 + X^{d/2}
in place of the usual p = 3, and a constant-time single-overflow **error
correction** step that drives the decryption-failure rate below 2^-130. It is the
KEM companion of YuanYang.DSA and can reuse a DSA key pair unchanged.

Specification: `kem-40-spec.pdf` (23 pages, English), §4.1–§4.4
(Algorithms 1–9), Table 1 (parameters), Table 2 (CDT tables), Table 4 (sizes).

## Parameters

| parameter | yuanyang-512 | yuanyang-1024 | yuanyang-2048 | meaning |
|---|---|---|---|---|
| d | 512 | 1024 | 2048 | ring degree, R = Z[X]/(X^d+1) |
| q | 2689 | 4481 | 7681 | modulus |
| p | 1 + X^256 | 1 + X^512 | 1 + X^1024 | masking modulus |
| k | 3 | 4 | 4 | rounding / compression factor |
| λ = d/4 | 128 | 256 | 512 | message and shared-secret bits |
| σ_f | 1.69 | 1.60 | 1.49 | key-coefficient std dev |
| σ_s | √3 | √2 | √2 | encryption randomness s |
| σ_e | √2.5 | √1.6 | 1 | encryption randomness e |
| encode batch B (ct) | 1 | 4 | 1 | integers packed per word |
| DFR | 2^-130.5 | 2^-270.2 | 2^-531.5 | spec Table 1 |
| claimed security (key rec. C/Q) | 129/117 | 267/243 | 546/496 | bits, Table 3 |
| claimed security (msg rec. C/Q) | 128/116 | 256/232 | 520/472 | bits, Table 3 |

Sizes (bytes), specification (Table 4) vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | ct spec | ct impl | ss impl | match |
|---|---|---|---|---|---|---|---|---|
| yuanyang-512 | 736 | 736 | — | 1536 | 656 | 656 | 16 | yes |
| yuanyang-1024 | 1568 | 1568 | — | 3168 | 1344 | 1344 | 32 | yes |
| yuanyang-2048 | 3328 | 3328 | — | 6528 | 2880 | 2880 | 64 | yes |

pk and ct reproduce §6's formulas B⌈log2(q)·d/8⌉/B and B⌈log2(q/k+2)·d/(8k)⌉/B + λ/8
exactly. The spec gives **no secret-key size**; the implementation's
sk = f ‖ f2^-1 ‖ H_pk ‖ K' ‖ pk = 3d/2 + 2·(λ/8) + |pk| matches all three
observed values.

## Pseudocode

### Z-Sampler (Algorithm 1)
```
v = 0 ; r <- Uniform[0..255] ; s = 1 - 2*(r mod 2) ; r >>= 1
for 0 <= i < len(T):  v += (T[i] <= r)
return v * s
```
T is the per-σ CDT of Table 2; the sign uses one bit, the magnitude seven.

### KeyGen (Algorithms 2 + 7)
```
repeat:
    f, g <- D_{R, sigma_f}                       # Z-Sampler, coefficient-wise
    if f not invertible in R_2: continue
    if f not invertible in R_q: continue
h      = f^{-1} g mod q
f2^-1  = inverse of f in R_2                     # Newton iteration mod 2
K'     <- U({0,1}^lambda)                        # implicit-rejection fallback key
H_pk   = Hash_pk(pk)
pk = h ;  sk = (f, f2^-1, H_pk, K', pk)
```

### Encrypt (Algorithm 3) and Encapsulate (Algorithm 8)
```
Encapsulate(pk):
  msg   <- U({0,1}^lambda)
  (K, rho) = Hash_msg(msg, Hash_pk(pk))
  c      = Encrypt(pk, rho, msg)
  K      = Hash_K(K, c)
  return (c, K)

Encrypt(h, rho, msg):
  msg' <-$ (U({0,1}^lambda); rho)                # ACWC: encrypt a fresh string
  m     = (msg', 0, ..., 0)  in Z_2[X]/(X^{d/2}+1)
  s <-$ (D_{R,sigma_s}; rho) ,  e <-$ (D_{R,sigma_e}; rho)
  cbar  = (h*s + e + m*p^{-1}) mod q             # p^{-1} = (q+1)(1 - X^{d/2})/2
  c     = Compress(cbar),  Compress(x) = floor((x - 1 + k mod 2)/k)
  u     = F(msg') XOR msg
  return (c, u)
```

### Decrypt (Algorithm 4), ErrCorrection (Algorithm 6), Decapsulate (Algorithm 9)
```
Decrypt((c,u), (f, f2^-1)):
  cbar  = k*c mod q                              # = h*s + e + e_cmp + p^{-1} m
  cbar' = cbar * f * p mod q
  cbar' = cbar' - q*round( (cbar' - (1/2) f ((1-k%2) sum_{d/2..d-1} 2X^i + sum_{0..d/4-1} X^i)) / q )
  a     = ErrCorrection(cbar', sk)               # a in {0} u {+-X^i}
  C     = cbar' - q*a
  m     = C * f2^-1 mod p
  msg'  = Low(m)
  return F(msg') XOR u

ErrCorrection(t, sk):                            # Algorithm 6, constant time
  tbar  = t mod (p, 2)                           # = f*m + a  mod (p,2)
  tbar' = tbar * f2^-1 mod (p, 2)                # = m + a*f2^-1 mod (p,2)
  u     = tbar' - Low(tbar')
  v     = u * (f2^-1 mod (p,2))  mod (X^{d/2} - 1)
  a' = sum_{0<=i<d/2} X^i * [ v[i] == Hamming(u) ]
  return a' * [ Hamming(u) > 0 ]

Decapsulate(c, sk = (K', sk', pk, H_pk)):
  msg       = Decrypt(c, sk')
  (K, rho)  = Hash_msg(msg, H_pk)
  ok        = (c == Encrypt(pk, rho, msg))       # full-length, constant-time compare
  return Hash_K( ok ? K : K' , c)
```

Hashes are the official ICCS SM3-based `pseudohash`. Domain separation is by a
trailing byte, not a prefix: 42 for Hash_msg, 43 for Hash_K, 44 for F, 45 for the
encryption-seed expansion (spec Remark 4.1 says only that "different prefixes"
separate the oracles).

## Implementation vs specification

Checked: `Implementations/Reference_Implementation/yuanyang-*/` — `kem.c`
(`ring_samp`, `yy_encrypt`, `yy_encapsulate_API`, `yy_decrypt_ciphertext`,
`yy_decapsulate_API`), `kem-keygen.c` (`ring_sampler`, `inverse_mod_2`,
`yy_kem_keygen`), `codec.c` (`yuanyang_{en,de}code_uniform`), `prng.c`/`prng.h`,
`params.h`. `KEM_AlgorithmInstance.c` is a thin forwarder to `yy_*`.

**Agreements.** `params.h` d and q match Table 1 for all three sets (512/2689,
1024/4481, 2048/7681); `SCALETAB = {3,4,4}` matches Table 1's k; `ENCODETAB =
{1,4,1}` matches §6's "batch size B of 4 … of the 256 bit version". **All nine
CDT tables** of Table 2 are reproduced exactly (σ_f in `kem-keygen.c:75-81`, σ_s
and σ_e in `kem.c:71-78`), each padded with a trailing 128 that can never fire
against a 7-bit sample — a harmless length-normalisation. The KEM randomness
comes from `drng_algorithm` via the `drng_randombytes` callback in both keygen
(`kem-keygen.c:145`) and encaps (`kem.c:158`); `prng.c`'s system-randomness path
is behind an opt-in `YUANYANG_FORCE_SYSTEM_RANDOMNESS` that `NGCC_KATS`
overrides and that the NGCC build never defines. Implicit rejection is
implemented exactly as Algorithm 9: the re-encryption compare is over the full
ciphertext length and accumulated with `ok &= …` rather than `memcmp`, and the
key select is the branch-free `ok*Kbar + (1-ok)*Kp` (`kem.c:261-269`). The
`f` invertibility test in R_2 is replaced by `eval_at_one_is_even_mod2`
(`kem-keygen.c:56,157`) — since d is a power of two, R_2 = F_2[X]/((X+1)^d) and
f is a unit iff f(1) ≠ 0, so this is an exact and deliberate equivalent
optimisation, not a weakened check.

**(a) Deviation — the encryption seed ρ is silently truncated for yuanyang-2048.**
`kem.c:111`: `init_random_number(&drng, seed, 1+YY_SEC/8 < SEEDLEN ? 1+YY_SEC/8 : SEEDLEN)`
with `SEEDLEN = 55` (`drng.h:15`) and `YY_SEC = d/4`. For d = 512 and d = 1024 the
seed is 17 and 33 bytes, under the cap. For d = **2048** it is 1 + 64 = 65 bytes,
so the DRNG is seeded with only the first **55 bytes = 440 bits** of ρ, and the
domain-separation byte 45 — which lives at seed offset 64 — is **dropped
entirely**. Because msg′, s and e are all expanded from that DRNG and
u = F(msg′) ⊕ msg, an exhaustive search over the 440-bit truncated seed,
verifiable against c, recovers msg in ≈2^440 classical / 2^220 Grover steps,
against Table 3's claimed 520-bit classical / 472-bit quantum message recovery
for yuanyang-2048. Encaps and decaps truncate identically, so the KATs still
reproduce. This is a hard-wired constant (`SEEDLEN`) interacting with a parameter
the spec intends to scale with λ.

**(a) Deviation — the sign bit of s is inverted relative to Algorithm 1.**
`kem.c:82-85` samples s with `int sign = random & 1; … s[u] = sign ? value : -value;`
whereas Algorithm 1 line 3 specifies `s ← 1 − 2(r mod 2)`, i.e. `+value` when the
bit is 0. The e sampler (`kem.c:90-93`) and the key sampler (`kem-keygen.c:83`)
both use the spec's `1-2*(random&1)` form. The discrete Gaussian is symmetric and
the bit is uniform, so the *distribution* is unaffected; but a spec-faithful
reimplementation will not reproduce these KATs, and the inconsistency between the
two adjacent loops in one function looks unintended.

**(a) Deviation — DRNG failures are swallowed in encapsulation.**
`prng_get_u8` (`prng.h`) returns a literal `0` when `prng_refill` fails, and
`prng_get_bytes` leaves the destination untouched. `yy_encrypt` ignores the
return of `ring_samp` (`kem.c:120`), `yy_encode_message` ignores `yy_encrypt`,
`yy_encapsulate_API` ignores both `yy_encode_message` and the
`drng_randombytes(&drng_algorithm, msg, YY_SEC/8)` at `kem.c:158`, and
`yy_decapsulate_API` ignores `yy_decrypt_ciphertext`. Under a failing DRNG,
`kem_enc` therefore returns success with `msg` left as uninitialised stack and
s = e = 0 — an all-zero-randomness ciphertext. `yy_kem_keygen` does check
`ring_sampler`'s status, but then fills the fallback key K' with bare
`prng_get_u8(&rng)` (`kem-keygen.c:178`), so a failed DRNG yields K' = 0 with no
error. The spec has nothing to say about RNG failure, but the inconsistency
within one submission (keygen checks, encaps does not) marks this as an omission
rather than a design choice.

**(b) Spec ambiguity — the coefficient packing is non-injective.**
`yuanyang_decode_uniform` (`codec.c:89`) recovers coefficients with `word % b`,
so each block of ⌈log2(b^k)⌉ bits has 2^⌈log2(b^k)⌉ − b^k surplus values that
alias onto valid coefficient tuples. For the *ciphertext* this is neutralised by
the FO byte-for-byte re-encryption compare. For the **public key** there is no
such check, so a pk admits several byte encodings that decode to the same h while
producing different `Hash_pk(pk)` — contributory-behaviour / key-confirmation
edge cases only, no effect on the KEM's stated security. The spec's §6 size
formula defines the packing but never states a canonicity requirement.

**Cosmetic but worth recording.** `kem.c`'s file header describes "Falcon3-512
signing" and `codec.c`'s header describes a signature key layout with salt and
compressed s1 — both are unedited copy-paste from the companion signature
codebase and describe code that is not in these files. `kem-keygen.c:31` defines
`SIGMAKEYTAB = {1.7, 1.6, 1.5}` against Table 1's σ_f = 1.69, 1.60, 1.49; the
array is dead (the sampler uses the exact CDT), so the rounding has no effect.
The fixed stack buffers `uint8_t tmphash[3000]` (`kem.c:159`) and `[4096]`
(`kem.c:250`) are sized by hand-computed comments with no static assertion; they
hold for all three shipped sets (worst case 2945 bytes) but would silently
overflow for a larger d.

**Not verified.** Nothing was built or executed. The NTT/inversion layer
(`ntt.c`, `poly_inv_ntt.c`, `ntttable.h`) and the bit-exact correctness of
`yy_decrypt_ciphertext`'s bias/centred-lift arithmetic against Algorithm 4 line 3
were read but not proved equivalent; the KAT pass recorded in `RESULTS.md` is the
only evidence for them.
