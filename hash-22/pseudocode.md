# hash-22 Pavelor — algorithm summary

Pavelor is a classical sponge hash family built on **Paff**, a 24-round,
2560-bit permutation whose round function is twenty parallel *AES round
transformations without round-key XOR* wired together by two fixed index
permutations `p1`, `p2` (an AESENC-based "wide-trail over subblocks" design in
the Haraka / AESQ / Simpira family). Security rests on the hermetic-sponge
argument: generic capacity bound plus absence of structural attacks on the
full-round Paff.

Specification: `hash-22-spec.pdf` (49 pages), §1.2 (mode), §1.3 (permutation),
§2.5.2 (final parameters), §3.1 (claims).

## Parameters

| parameter | Pavelor-512 | Pavelor-768 | Pavelor-1024 | meaning |
|---|---|---|---|---|
| state width b | 2560 | 2560 | 2560 | bits = 20 subblocks × 128 bits |
| rate r | 1536 | 1024 | 512 | bits XORed/read per permutation call |
| capacity c | 1024 | 1536 | 2048 | b − r, always c = 2h |
| digest h | 512 | 768 | 1024 | bits |
| rounds | 24 | 24 | 24 | `Paff = ρ23 ∘ … ∘ ρ0`, §1.3 |
| AES rounds / permutation | 24 × 19 = 456 | idem | idem | one `A()` per non-constant subblock |
| absorb calls | ⌈(|M|+2)/r⌉ | idem | idem | one Paff per r-bit block |
| squeeze calls | 1 | 1 | 2 | ⌈h/r⌉ |
| IV | ⟨r⟩₁₂₈‖⟨c⟩₁₂₈‖⟨b⟩₁₂₈‖⟨h⟩₁₂₈‖0…0 | idem | idem | §1.2.2 |
| collision (classical) | 2²⁵⁶ | 2³⁸⁴ | 2⁵¹² | spec Table 5 |
| preimage (classical) | 2⁵¹² | 2⁷⁶⁸ | 2¹⁰²⁴ | spec Table 5 |
| 2nd-preimage (classical) | 2⁵¹² | 2⁷⁶⁸ | 2¹⁰²⁴ | spec Table 5 |
| collision (quantum) | 2⁵¹²ᐟ³ | 2²⁵⁶ | 2¹⁰²⁴ᐟ³ | spec Table 6 |
| preimage / 2nd-preimage (quantum) | 2²⁵⁶ | 2³⁸⁴ | 2⁵¹² | spec Table 6 |

Sizes (bytes), specification vs the built reference library:

| instance | digest spec | digest impl (OBSERVED) | match |
|---|---|---|---|
| Pavelor-512 | 64 | digest_bits=512, digest_bytes=64 | yes |
| Pavelor-768 | 96 | digest_bits=768, digest_bytes=96 | yes |
| Pavelor-1024 | 128 | digest_bits=1024, digest_bytes=128 | yes |

Round-count / structure detail (relevant to rebound and differential
cryptanalysis): every round applies **19** independent AES `A` transformations
(one per subblock except the constant-injection slot), so the full permutation
costs 456 AES rounds. The spec's MILP bounds (Table 7) give ≥125 differentially
and >86 linearly active S-boxes at **7** rounds, already past 2⁻⁵¹² for a single
characteristic; the design target derived from diffusion analysis was
`Dsub = 6`, `Dbyte = 7`, and the §2.5 cost model uses `Rsec = 2·Dsub = 12`.
Shipping 24 rounds is therefore **2× the spec's own security-round target**, i.e.
a large declared margin; the MITM (§3.3.2) and rebound (§3.3.3) analyses reach
at most 7–11 rounds.

## Pseudocode

### Pavelor-h(M) (Algorithm 1)
```
M* <- Pad(M, r)
S  <- InitState(h)
S  <- Absorb(S, M*, r)
H  <- Squeeze(S, h, r)
return H
```

### Pad — Algorithm 2 (§1.2.1)
```
Pad(M, r):
    M1 = M || 1
    k  = (r - 1 - (|M1| mod r)) mod r
    return M1 || 0^k || 1          # i.e.  M || 1 || 0^k || 1   ("10*1")
```
Both the leading and the trailing 1 are mandatory, so `|M*|` is always a
positive multiple of r and the rule is injective (this is the length-extension
argument of §3.3.1 combined with c = 2h).

### InitState — Algorithm 3 (§1.2.2)
```
S_j <- IV_{h,j},  0 <= j < 20
IV_h = <r>_128 || <c>_128 || <b>_128 || <h>_128 || 0^128 || ... || 0^128
```
This is the **domain separation between digest lengths**: r, c and h all enter
the initial state, so Pavelor-512/768/1024 are provably distinct functions even
though they share Paff.

### Absorb — Algorithm 4 (§1.2.3)
```
split M* into l blocks M^(0..l-1) of r bits
for i = 0 .. l-1:
    S_rate <- S_rate XOR M^(i)        # first r bits of S
    S      <- Paff(S)
```

### Squeeze — Algorithm 5 (§1.2.4), variable digest length
```
H = eps
while |H| < h:
    H <- H || floor(S_rate)_{min(r, h-|H|)}
    if |H| >= h: break
    S <- Paff(S)
return floor(H)_h
```
Only h ∈ {512, 768, 1024} is defined; there is no XOF/arbitrary-length mode.
For h ≤ r (512 and 768) one squeeze block suffices; Pavelor-1024 (r = 512)
needs two, with one extra Paff between them.

### Paff — Algorithm 6 (§1.3), 24 rounds
```
A(X)            = MixColumns(ShiftRows(SubBytes(X)))      # AES round, no key XOR
AESENC(X, Y)    = A(X) XOR Y

p1 = (19,4,6,17,9,0,13,11,8,5,15,10,16,3,12,14,18,7,2,1)
p2 = ( 1,0,3, 4,5,2, 7, 8,9,10, 6,12,13,14,15,16,17,18,19,11)
GammaR = (1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1)   # zero at INPUT index 11

for i = 0 .. 23:
    # intermediate: A() on every input subblock except the masked one
    T_j = A(S_{i,j})   for j != 11 ;   T_11 = RC_i
    # rewire: local-update path p1, original-state coupling path p2
    S_{i+1,j} = T_{p1(j)} XOR S_{i,p2(j)}
    # equivalently, since p1(7) = 11 and p2(7) = 8:
    #   S_{i+1,j} = AESENC(S_{i,p1(j)}, S_{i,p2(j)})  for j != 7
    #   S_{i+1,7} = RC_i XOR S_{i,8}
return S_24
```
`RC_0..RC_23` are 128-bit words from the fractional part of π (spec Table 3,
starting 0x243f6a8885a308d313198a2e03707344; RC7 and RC22 deviate from π, see below).

## Implementation vs specification

Checked: `src/Pavelor-{512,768,1024}/CryptHash_AlgorithmInstance.c` — the three
files are **byte-identical** (md5 `7326775e…`); only `DIGEST_BIT_LENGTH` in the
`.h` differs. `drng.c` is the unmodified official DRNG, unused by the hash. All
3 instances PASS KAT (RESULTS.md).

Verified item by item:

- **Round count is 24**, `for (int round = 0; round < 24; round++)` in `paff()`
  (line 110) — matches §1.3 exactly. No reduced-round shortcut.
- **All 20 round-function lines** of `paff()` (lines 112–132) were checked
  against Algorithm 6 one at a time, and against the `p1`/`p2` tables of §2.5.2:
  every (source, XOR-source) pair agrees, including the constant slot
  `next[7] = RC[round] XOR s[8]`.
- **Round constants**: `RC[0]` and `RC[23]` (and spot checks at 6, 10, 15)
  reproduce spec Table 3 byte for byte. 22 of the 24 equal consecutive 128-bit
  pieces of frac(π); RC7 and RC22 are the π pieces `0x0801f2e2…4e69` and
  `0x0f6d6ff3…2004` shifted by one hex digit (b). The code follows Table 3.
- **AES round**: `aes_round_no_key()` is SubBytes (standard AES S-box table,
  verified against the FIPS-197 values), ShiftRows in the column-major byte
  layout, then MixColumns via the standard `a ^ x ^ xtime(a_i ^ a_{i+1})`
  identity. `aesenc()` XORs the second operand afterwards = `AESENC(X,Y)` ✓.
- **Rate/capacity**: `init_state()` sets r = 1536/1024/512 for h = 512/768/1024
  and c = 2560 − r, matching Table 1; c = 2h holds for all three.
- **IV**: `put_u128_low16()` writes ⟨x⟩₁₂₈ big-endian into subblocks 0..3 as
  r, c, b=2560, h, rest zero — reproduces spec Table 2 (e.g. Pavelor-512 gives
  0x600, 0x400, 0xA00, 0x200 ✓).
- **Padding**: `absorb_padded()` implements `M‖1‖0^k‖1` incrementally. I checked
  all three boundary cases against Algorithm 2: `rem = 0` (incl. the empty
  message) → final block `1‖0^{r-2}‖1`; `rem = r-2` → `M‖1‖1`; `rem = r-1` →
  the block is closed with the first 1 and an **extra** all-but-last-bit-zero
  block `0^{r-1}‖1` is absorbed. All agree with the spec. The file also ships a
  straight-line `absorb_padded_reference()` and a `PavelorBackToBackSelfTest()`
  that cross-checks the two over 27 (length, h) cases — dead code in the built
  library, but useful evidence the authors checked this themselves.
- **Squeeze**: `squeeze_digest()` permutes *between* output blocks and not
  before the first or after the last, matching Algorithm 5 (the `if (written <
  out_bytes) paff(state);` guard). Only Pavelor-1024 takes the second iteration.

Discrepancies and notes:

- **(b) Apparent spec inconsistency, resolved — worth stating because it reads
  like a defect.** §1.3 / Algorithm 6 put the round-constant injection at
  *output* subblock `j = 7`, while §2.5.2 gives `ΓR` with its single zero at
  *index 11*. These are consistent only once one notices that `ΓR` is indexed by
  the **input** subblock and `p1(7) = 11`; i.e. the masked intermediate word
  `T_11 = RC_i` is routed to output 7 by `p1`. The spec never says which
  indexing `ΓR` uses. The implementation follows §1.3 (`next[7] = RC ^ s[8]`),
  which is the correct reading. No deviation, but the spec should state it.
- **(a) Minor API deviation: the instance's declared digest length is not
  enforced.** `CryptHash()` (line 290) checks only `digest_len_bits % 8 == 0`
  and that `init_state()` recognises the length; it never compares against
  `DIGEST_BIT_LENGTH`. Because the three `.c` files are identical, calling the
  *Pavelor-512* shared library with `digest_len_bits = 1024` silently returns a
  full Pavelor-1024 digest. This does not affect KAT conformance (the driver
  passes `DIGEST_BIT_LENGTH`) and is not a cryptographic weakness — the IV
  domain-separates the three — but it is a deviation from the one-instance-
  per-library convention that the other candidates in this batch enforce.
- **No round-count, padding, capacity/rate or domain-separation deviation was
  found.** The 24 rounds, the `10*1` padding, the (r, c) pairs of Table 1 and the
  ⟨r,c,b,h⟩ IV are all implemented as specified.
- **Coverage:** no independent differential/rebound analysis of Paff was
  performed here; the spec's own Table 7 bounds (≤7 rounds) and the §3.3.3
  rebound discussion were read but not re-derived. Because the round function is
  literally 19 AES rounds per round, rebound/super-S-box techniques apply
  directly and the 24-round claim depends on the (unverified here) claim that
  cross-subblock diffusion needs `Dsub = 6` rounds.
