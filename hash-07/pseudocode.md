# hash-07 Dragon — algorithm summary

Permutation-based hash/XOF family using the authors' **sponge-forward** mode
`Sponge-F^{P,pd10*}` (Guo et al., CRYPTO 2026): an ordinary sponge in which the
*inner* (capacity) part of the pre-permutation state is fed forward and XORed
back into the inner part after every permutation call. The feed-forward raises
(second-)preimage security to ≈ min{c, h} instead of the classical c/2, so a
1600-bit state suffices for a 1024-bit preimage target. The primitive is
**Dragon-p**, a 16-round ARX permutation on a 5×5 array of 64-bit lanes.

Specification: `hash-07-spec.pdf` (42 pages), §2 (Fig. 1, §2.4 permutation,
§2.5 hash instantiation, §2.6 XOF, §2.7 claims). English-language spec.
Implementation tree used for the comparison below:
`Dragon/Implementations and Test_vector/Dragon-x86/`. The `Dragon-ARM/` tree and
the optimized and additional implementations were reviewed on 2026-11-17: the
ARM reference sources equal the x86 ones up to whitespace, the optimized and
additional sources are plain C in both trees, and every one of them agrees with
the x86 reference for all message lengths from 0 to 4,200 bits.

## Parameters

All six instances share b = 1600 bits and the same 16-round Dragon-p; they
differ only in c, r, the IV and the output rule.

| parameter | Dragon-512 | Dragon-768 | Dragon-1024 | XOF-256 | XOF-384 | XOF-512 |
|---|---|---|---|---|---|---|
| state width b | 1600 | 1600 | 1600 | 1600 | 1600 | 1600 |
| capacity c | 576 | 832 | 1088 | 576 | 832 | 1088 |
| absorb rate r | 1024 | 768 | 512 | 1024 | 768 | 512 |
| n_c = c/64 | 9 | 13 | 17 | 9 | 13 | 17 |
| n_r = r/64 | 16 | 12 | 8 | 16 | 12 | 8 |
| rounds | 16 | 16 | 16 | 16 | 16 | 16 |
| squeeze rate r' | — (single read) | — | — | 1280 | 1152 | 1024 |
| digest length, spec | 512 | 768 | 1024 | ν (any) | ν | ν |
| digest_bits in OBSERVED | 512 | 768 | 1024 | **1280** | **1152** | **1024** |
| digest_bytes | 64 | 96 | 128 | 160 | 144 | 128 |
| claimed collision | 256 | 384 | 512 | — | — | — |
| claimed preimage | 512 | 768 | 1024 | — | — | — |
| claimed generic level log2 Q | — | — | — | 256 | 384 | 512 |

**Note on the XOF names and the OBSERVED digest lengths.** `Dragon-XOF-256/
384/512` name a *security level* (log2 Q, spec Table 6), not an output length.
The number the ICCS one-shot API reports is the squeezing rate r' — one
squeeze block — which is 1280 / 1152 / 1024 bits respectively (spec Table 6).
So `digest_bits` for XOF-256 exceeds that of XOF-512. This is spec-conformant
but is a naming trap for anyone reading the metadata; the implementation does
support arbitrary ν (see below).

## Pseudocode

### Mode: sponge-forward (spec Fig. 1)

```
Sponge-F^{P,pd10*}(M, nu):
    (M[1..L]) <- split_r( pd10*(M) )            # pd10*(M) = M || 1 || 0^(r-((|M|+1) mod r))
    X <- 0^r || IV                              # rate zero, capacity = public IV
    # --- absorb ---
    for i = 1..L:
        if i != L:  X <- X XOR (M[i] || 0^c)
        else:       X <- X XOR (M[i] || C)      # C = 0^(c-1)||1, final-block domain constant
        X <- P(X) XOR (0^r || right_c(X))       # <-- the feed-forward; right_c(X) is the
                                                #     inner part BEFORE the permutation
    # --- squeeze ---
    lambda <- ceil(nu / r')
    for i = 1..lambda:
        Z[i] <- right_{r'}(X)
        if i < lambda:  X <- P(X) XOR (0^r || right_c(X))
    return left_nu( Z[1] || ... || Z[lambda] )
```

For a fixed-output instance with h < c the whole digest is one `right_h(X)`
read, so squeezing costs no extra permutation call.

### Padding and final-block rule (spec §2.5.3)

```
t = |M| mod r
  0 < t < r-1 : final block = M_rem || 1 || 0^(r-t-1)      ; domain constant C applied
  t = r-1     : block  = M_rem || 1                        ; C NOT applied here,
                then one extra block 0^r                   ; C applied to that block
  t = 0       : one extra block 1 || 0^(r-1)               ; C applied
```

`C` is XORed into the least significant bit of lane `S[0,0]`.

### Lane layout (spec §2.5.2, §2.5.4)

```
linear lane index l(x,y) = x + 5y           # 0..24, S[0,0] .. S[4,4]
capacity = lanes 0 .. n_c-1                 # lane j holds IV_j at initialization
rate     = lanes 25-n_r .. 24
message block M' = M_{n_r-1} || ... || M_0  # M_{n_r-1} = most significant, first on the wire
    M_i  ->  lane  k_i = 25 - n_r + i
digest (fixed output, n_d = h/64 lanes):
    D = D_{n_d-1} || ... || D_0,  D_j = lane j,  each lane serialized MSB-first
```

### Permutation Dragon-p (spec §2.4)

```
P = R_15 o ... o R_0 ,   R_i = phi o iota_i          # 16 rounds
iota_i:  S[0,0] ^= RC_i                              # RC_i = 0x0F,0x1E,0x2D,...,0xF0 (i*15+15)
phi   =  Col o Diag
    Diag_j(T) = Update( S[0,j], S[1,j+1], S[2,j+2], S[3,j+3], S[4,j+4] ), j=0..4
    Col_x(U)  = Update( T[x,0], T[x,1], T[x,2], T[x,3], T[x,4] ),         x=0..4

Update(a,b,c,d,e):                                   # + is mod 2^64, <<< is rotl
    b += e ;  d = (d <<< 33) ^ b
    a += d ;  c = (c <<< 24) ^ a
    e += c ;  b = (b <<< 10) ^ e
    d += b ;  a = (a <<< 1)  ^ d
    c += a ;  e = (e <<< 49) ^ c
    # (r_a, r_b, r_c, r_d, r_e) = (1, 10, 24, 33, 49)
```

## Implementation vs specification

Source reviewed: `hash-07/src/Dragon-*/CryptHash_AlgorithmInstance.{c,h}`.
The `.c` file is identical for Dragon-512 / -768 / -1024 (191 lines); the
three XOF instances share a second, 245-line variant that adds a real
multi-block squeeze loop. All instance parameters live in the header.

Verified agreements:

- `TOTAL_ROUNDS 16`, `RC[16]` (c ll. 18–25) equal spec Table 3 exactly, and
  `ra,rb,rc,rd,re = 1,10,24,33,49` (l. 28) equal the spec's offsets.
- `Update()` (ll. 36–52) is a statement-by-statement transcription of the
  spec's Update; `phi()` (ll. 55–67) applies the five diagonals
  `(S[0][j],S[1][j+1],...)` then the five columns, i.e. `phi = Col o Diag`;
  `permutation()` does `iota` then `phi` per round — matches `R_i = phi o iota_i`.
- Per-instance `CAPACITY_BITS` = 576 / 832 / 1088 (and the same for the three
  XOFs), with `RATE_BITS = 1600 - CAPACITY_BITS` — matches Tables 4 and 6 for
  all six instances. `SQUEEZE_BIT_LENGTH` = 1280 / 1152 / 1024 matches r' in
  Table 6.
- IV loading: the header stores the IV array in **reverse** spec order
  (`IV[0] = IV_{n_c-1}`, documented in the header comment) and the code reads
  `IV[IV_LANE_NUM-1-i]` into lane `i`, so lane j receives spec `IV_j`. The
  9 lanes of Dragon-512 and of Dragon-XOF-256 were checked against Table 5;
  they match. The rate lanes start at zero (`u64 X[5][5] = {0}`).
- Message-to-lane mapping (ll. 111–114): `msg` bytes `8i..8i+7` are loaded
  big-endian into lane `24-i`, i.e. the first wire word goes to lane 24 and
  the last to lane `25-n_r` — exactly the spec's `M_i -> lane 25-n_r+i` with
  `M' = M_{n_r-1}||...||M_0`.
- Feed-forward: the capacity lanes are copied to `S[]` before the permutation
  and XORed back afterwards (ll. 108–120, 161–169) — matches
  `X <- P(X) XOR (0^r || right_c(X))`. In the final block the save happens
  *after* `X[0][0] ^= 1`, which is correct: the spec's line 9 operates on the
  state that already contains `C`.
- Final-block rule (ll. 126–184): `X[0][0] ^= (!need_extra_zero_block)`
  implements the spec's exception that `M_rem || 1` filling a whole block is
  *not* domain separated and the following `0^r` block carries `C` instead.
  The `t = 0` case yields `pad_block[0] = 0x80`, i.e. `1 || 0^(r-1)`.
- Digest extraction (ll. 186–189): lane `n_d-1` first, lane 0 last, each
  big-endian — matches `D = D_{n_d-1}||...||D_0` of §2.5.4. For every fixed
  instance `n_d < n_c`, so the digest comes entirely from the capacity, as
  §2.7 assumes (`c > h`).
- **Partial-byte input is handled correctly** (ll. 134–139):
  `pad_block[k] = msg[k] & (0xFF << (8-partial_bits)); pad_block[k] |= 0x80 >> partial_bits;`
  masks the bits beyond `msg_len_bits` before inserting the `1` bit, so the
  digest depends only on the declared bits. This is the correct behaviour that
  hash-05 and hash-10 lack.
- XOF squeeze loop (XOF `.c` ll. 211–241): honours the caller's
  `digest_len_bits` for any ν, emitting `SQUEEZE_BIT_LENGTH` bits per
  permutation call with the feed-forward applied between squeezes, and
  bit-accurate MSB-first assembly via `copy_bits_msb()`. This matches Fig. 1
  lines 11–16. The fixed `DIGEST_BIT_LENGTH` in the header is only the value
  advertised to the KAT harness.

**No deviation from the specification was found for hash-07.** Parameter
constants, round constants, rotation offsets, lane mapping, padding,
domain-separation rule, feed-forward and digest order all agree.

### Minor observations (not deviations)

- `CryptHash()` in the fixed-output instances ignores `digest_len_bits`
  entirely: it always writes `DIGEST_LANE_NUM` lanes and returns 0. A caller
  passing a smaller value gets a buffer overflow; a caller passing a larger
  one gets a short, silently-truncated answer. The XOF variant only rejects
  `digest_len_bits < 0`. Filed as `hash-07-2`.
- `ROTL64(x, shift)` is undefined behaviour for `shift == 0`, but no rotation
  constant used is 0, so it is unreachable.
- The security claims hold "for messages of length up to 2^64 bits" (§2.7);
  the implementation's absorbing loop uses `unsigned long long` throughout and
  has no lower internal limit.

### Verified on 2026-11-17

- All six IV tables were recomputed from the stated rule and match the headers
  and Table 5: the stream `SM3(string ‖ ctr)` for a 32-bit big-endian counter
  from 0 fills `IV_{n_c-1}` first and `IV_0` last, with the strings
  `Dragon-512|IV|r=1024|c=576|` and `Dragon-XOF-256|IV|r=1024|c=576|r'=1280|`
  and their analogues.
- Table 7 (bit-level diffusion) reproduces exactly: 568 and 1600 fully diffused
  bits after one and two forward rounds, 0, 879 and 1600 after one, two and
  three inverse rounds.
- Table 9 reproduces (correlation 1 after one round, 2^-0.42 after two for the
  listed pair). The listed two-round pair is not the strongest single-bit pair:
  the same input difference `S[2][0]` bit 39 gives 2^-0.22 on `S[3][2]` bit 0.
  After three rounds no single-bit pair exceeded the noise floor of a
  2^22-sample scan over all 1600 input bits (about 2^-8.5).

### Not verified

- The mode's security proof (§4.1), the MILP trail searches and UNSAT bounds
  (§4.2.3), and the integral search (§4.2.5) were not re-run; the numbers in
  the parameter table are the spec's claims.
