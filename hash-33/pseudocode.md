# hash-33 Thunder — algorithm summary

Thunder is a **sponge-with-feed-forward** family: the mode is
`Sponge-F^{P,pd10*}` of Guo et al. (spec ref [GHJ+25], "to appear at CRYPTO
2026"), in which the *inner* (capacity) part of the state is fed forward across
every permutation call and the digest is squeezed **from the inner part**. This
lifts (second-)preimage resistance from the usual `c/2` to ≈`min{c, h}` and so
allows `c = h + 64` instead of `c ≥ 2h`. The permutation **Thunder-p** is a
Keccak-p-shaped 1600-bit (5×5×64) permutation with 12 rounds and round
`R = ψ ∘ π ∘ θ ∘ ρ ∘ ι`, but with a *degree-3* nonlinear layer `ψ` and a
double-column-parity `θ` instead of Keccak's χ and θ.
A constant `C` injected into the inner part of the **final** block is what
prevents length extension (necessary, since the output *is* the inner part).

Specification: `hash-33-spec.pdf` (52 pages), §2 "Specification"
(§2.2 mode, §2.3 Thunder-p, §2.4 hash, §2.5 XOF, §2.6 claims; Tables 1–7,
Figs. 2 and 4). English. The build uses only
`Thunder/Implementations and Test_vector/Thunder-X86/.../Reference_Implementation`.
The ARM tree and the optimized and additional implementations were reviewed on
2026-11-17: all are plain C and agree with the reference for all message
lengths from 0 to 4,200 bits. One directory,
`Thunder-X86/.../Additional_Implementation/Thunder-1024`, does not compile as
shipped: it pairs the optimized source, which defines `IV`, with the reference
header, which also defines it.

## Parameters

| parameter | Thunder-512 | Thunder-768 | Thunder-1024 | XOF-256 | XOF-384 | XOF-512 | meaning |
|---|---|---|---|---|---|---|---|
| b | 1600 | 1600 | 1600 | 1600 | 1600 | 1600 | state width (5×5×64) |
| c | 576 | 832 | 1088 | 576 | 832 | 1088 | capacity (inner) |
| r = b−c | 1024 | 768 | 512 | 1024 | 768 | 512 | absorbing rate |
| n_c = c/64 | 9 | 13 | 17 | 9 | 13 | 17 | capacity lanes |
| n_r = r/64 | 16 | 12 | 8 | 16 | 12 | 8 | rate lanes |
| r′ (squeeze rate) | h = 512 | 768 | 1024 | **1280** | **1152** | **1024** | bits per squeeze |
| rounds | 12 | 12 | 12 | 12 | 12 | 12 | Thunder-p |
| h (digest) | 512 | 768 | 1024 | ν (arbitrary) | ν | ν | bits |
| IV | distinct 9 lanes | distinct 13 | distinct 17 | distinct 9 | distinct 13 | distinct 17 | Table 6 — domain separation |
| claimed collision | 256 | 384 | 512 | min{256, ν/2} | min{384, ν/2} | min{512, ν/2} | bits (Tables 1, 5, 7) |
| claimed preimage | 512 | 768 | 1024 | min{256, ν} | min{384, ν} | min{512, ν} | bits |
| data limit | 2^64 | 2^64 | 2^64 | 2^64 | 2^64 | 2^64 | message bits |

Sizes (bytes), specification vs the built reference library (OBSERVED):

| instance | digest spec | digest impl | match |
|---|---|---|---|
| Thunder-512 | 64 | 64 | yes |
| Thunder-768 | 96 | 96 | yes |
| Thunder-1024 | 128 | 128 | yes |
| Thunder-XOF-256 | ν (arbitrary) | 160 | n/a — harness fixes ν = r′ = 1280 |
| Thunder-XOF-384 | ν (arbitrary) | 144 | n/a — harness fixes ν = r′ = 1152 |
| Thunder-XOF-512 | ν (arbitrary) | 128 | n/a — harness fixes ν = r′ = 1024 |

The three XOFs are exposed through the fixed-length `CryptHash` API at exactly
one squeeze block (`DIGEST_BIT_LENGTH = SQUEEZE_BIT_LENGTH = r′`), so no
squeezing permutation call is made. Their `r′ > c` (e.g. 1280 > 576): a squeeze
block spans the whole inner part plus part of the outer part, which the mode
permits because of the feed-forward (spec §2.5).

## Pseudocode

### Thunder-p permutation (spec §2.3, Tables 2–4)
```
State D[x][y][z], x,y in Z_5, z in Z_64;  linear lane index idx(x,y) = 5y + x.

for i = 0 .. 11:
    iota : D[0][0] ^= RC_i                              # Table 2
    rho  : D[x][y][z] <- D[x][y][z + tau(x,y)]          # Table 3 (lane rotations)
    theta: P[x][z] = XOR_y D[x][y][z]
           Q[x][z] = XOR_y D[x][y][z + t_y]             # t = (41,50,6,10,27)
           E[x][z] = P[x][z]   XOR P[x-1][z + r_]       # r_ = 1
           F[x][z] = Q[x+1][z] XOR Q[x-2][z + s]        # s  = 36
           D[x][y][z] ^= E[x][z+u] XOR F[x][z+u+t_y]    # u  = 26
    pi   : D'[y][2x+3y+2][z] = D[x][y][z]               # lane permutation
    psi  : D'[x][y][z] = D[x][y+3][z]
                         XOR ( ~D[x][y+1][z] AND D[x][y+2][z] AND ~D[x][y+4][z] )
```
`ψ` has algebraic degree 3 (Keccak's χ has degree 2); `θ` mixes two column
parities `P` and `Q` rather than one.

### Hash(M, h) — Thunder-h (spec §2.2 Fig. 2, §2.4)
```
Thunder-h(M):                                   # h in {512,768,1024}, c = h+64
  # Initialization: IV lanes into the c lowest-indexed lanes, rate lanes zero
  X <- 0^r || IV                                        # IV_j -> lane j
  # Padding pd10*  (spec 2.4.3)
  Mpad <- M || 1 || 0^{r - ((|M|+1) mod r)}             # extra all-zero block
                                                        # if the 1 fills a block
  parse Mpad = M[1] .. M[l], |M[i]| = r
  # Absorption with inner feed-forward
  for i = 1 .. l:
      if i < l: X <- X XOR (M[i] || 0^c)
      else:     X <- X XOR (M[l] || C),  C = 0^{c-1}||1   # domain separation
      S <- right_c(X)                                     # inner part AFTER xor
      X <- P(X) XOR (0^r || S)                            # feed-forward
  # Squeezing: h < c, so ONE squeeze and no further permutation call
  return right_h(X)                                       # digest = inner lanes
```
Byte/lane conventions (§2.4.2, §2.4.4): the block `M' = M_{n_r-1}‖…‖M_0` is laid
into the `n_r` highest-indexed lanes, `M_{n_r-1}` into lane 24; the digest is
`D_{n_d-1}‖…‖D_0` with `n_d = h/64`, each lane serialized most-significant-byte
first.

### XOF — Thunder-XOF-s(M, ν) (spec §2.5)
Identical absorption; squeezing repeats
`Z[i] <- right_{r'}(X); S <- right_c(X); X <- P(X) XOR (0^r || S)`
until ν bits have been produced, then truncates to ν.

## Implementation vs specification

Checked: `src/Thunder-512/CryptHash_AlgorithmInstance.{c,h}` (310 + 65 lines)
and the five sibling instances. The three fixed-output instances share a
byte-identical `.c`; the three XOF instances share a second `.c` that differs
only by the multi-block squeeze loop (`copy_bits_msb`, `extract_squeeze_block`).
All parameters live in the per-instance header. All six instances KAT-PASS
(`results/summary.tsv`). Source comments are in GB-encoded Chinese.

Agreements (spot-checked constants — the whole permutation, not a sample):

- `RC[12]` = Table 2, exactly and in order (0x1, 0x3, 0xA, 0x88, 0x8081,
  0x8002, 0x9, 0x83, 0x800A, 0x89, 0x8083, 0x800B).
- `pho_rotc[25]` = Table 3 τ(x,y) row by row (y=0: 45,33,13,1,27 … y=4:
  6,18,49,17,43); implemented with `ROTR64`, consistent with the spec's
  "read index z + τ" convention.
- `theta_rotc[5] = {41,50,6,10,27}` = `t_y` of Table 4; `u = 26, r = 1, s = 36`
  = Table 4. `E[x] = ROTR(P[x],u) ^ ROTR(P[x-1],u+1)` and
  `F[x] = Q[x+1] ^ ROTR(Q[x+3],36)` are the spec's `E`/`F` pre-rotated by `u`
  (`x-2 ≡ x+3 mod 5`); the update
  `state[x+5y] ^= E[x] ^ ROTR(F[x], t_y+u)` then reproduces
  `D ⊕ E[x][z+u] ⊕ F[x][z+u+t_y]` exactly.
- `pi_piln[]` is the gather form of `D'[y][2x+3y+2] = D[x][y]`; I checked four
  entries by hand ((x,y) = (0,0)→10, (1,0)→20, (2,0)→5, (0,1)→1) and all agree.
- `chi_step()` = `ψ`: `state[j] = t3 ^ ((~t1) & t2 & (~t4))`, i.e.
  `D[x][y+3] ⊕ ~D[x][y+1]·D[x][y+2]·~D[x][y+4]` — the bar placement that the
  PDF's overlines imply (they are lost in text extraction, but the degree-3
  form with two complements is unambiguous from the code and matches §2.3).
- Round order `iota → pho → theta → pi → chi` = `R = ψ ∘ π ∘ θ ∘ ρ ∘ ι`; 12
  rounds.
- `CAPACITY_BITS` = 576/832/1088 and `RATE_BITS = 1600 - CAPACITY_BITS` =
  1024/768/512 reproduce Table 5 and Table 7 for all six instances.
- **IV values**: each header's `IV[]` is Table 6's list *written in reverse*,
  and the loader does `X[i] = IV[IV_LANE_NUM-1-i]`, so lane `j` receives spec
  `IV_j`. Verified for all six instances (e.g. Thunder-512 lane 0 =
  `BF6F0F35E3B4B690` = Table 6 `IV0`; XOF-512 lane 0 = `C2884D525C702B64` =
  Table 6 `IV0`).
- Feed-forward: `S[..] = X[i]` for the `n_c` capacity lanes, `permutation(X)`,
  then `X[i] ^= S[..]` — exactly `X ← P(X) ⊕ (0^r ‖ right_c(X))` (Fig. 2 line 9).
  In the final block the snapshot is taken **after** `X[0] ^= C`, matching
  Fig. 2 lines 8–9.
- Message layout: `X[24-i] ^= load64_be(msg + 8i)` puts the first eight message
  bytes in lane 24, i.e. `M_{n_r-1}` into the highest-indexed lane (§2.4.2).
  Digest: `store64_be(digest + 8i, X[n_d-1-i])` = `D_{n_d-1}‖…‖D_0` (§2.4.4).
- Padding: `pad_block[full] |= 0x80 >> partial_bits` (or `= 0x80`) is the pd10*
  `1` bit in MSB-first order; `need_extra_zero_block = (rem_bits == r-1)`
  appends the extra all-zero block, exactly the `|M_rem| = r−1` case of §2.4.3.
- Domain constant: `X[0] ^= !need_extra_zero_block` on the first padded block
  and `X[0] ^= 1` on the extra block — so `C = 0^{c-1}‖1` is XORed into the
  lowest bit of lane 0 (the last bit of the inner part) of the **last** block
  only, and of no other block. Matches §2.4.3.

Discrepancies / notes:

- No parameter or constant mismatch found between spec and implementation.
- (cosmetic) The header comment "`IV[0] is the least significant lane`" is wrong
  — the array is in descending spec-index order and `IV[n_c-1]` ends up in lane
  0. The net placement is correct.
- (b, harness) The XOF instances have no specified digest length; the NGCC
  wrapper pins `DIGEST_BIT_LENGTH` to one squeeze block (1280/1152/1024 bits),
  which is why OBSERVED reports 160/144/128 bytes. The spec's security targets
  for these instances are stated as `log2 q` = 256/384/512, so the *name* is the
  security level, not the output length — easy to misread in a size table.
- Fixed-output `CryptHash` ignores `digest_len_bits` and always writes the full
  digest. Filed as `hash-33-2`.
- Verified on 2026-11-17: `ψ` is a bijection with degree 3 in both directions,
  differential uniformity 14 and linearity 24, as §3.2 states; `θ` has rank
  1600; no kernel vector of `θ` has weight below 6, consistent with branch
  number 12; Table 8 (84, 1553 and 1600 bits of forward coverage) reproduces
  exactly; the round constants follow the affine LFSR `new = s1 ⊕ s4 ⊕ 1`
  clocked once per round; the two- and three-round entries of Table 13
  reproduce (1 and 0.913); and the trail of Table 14 is consistent. No
  weight-6 kernel state stays in the kernel for a second round under any
  S-box transition.
- Not verified: the trail weights beyond two rounds (Tables 9 and 10, which
  are best-found values, not bounds), the four- and five-round entries of
  Table 13, and the provenance of the IV constants. The spec gives no
  derivation rule for Table 6, and the values are not produced by the SM3
  counter rule that the sibling Dragon submission states. This is a
  transparency gap, not a break.

### Domain separation between digest lengths — the key check

**Thunder separates its output lengths properly.** Three independent mechanisms:

1. **Distinct IVs.** Spec Table 6: "Each instance uses a distinct c-bit IV for
   domain separation." Verified in every header: all six instances (including
   the two pairs that share a capacity — Thunder-512/XOF-256 with c = 576,
   Thunder-768/XOF-384 with c = 832, Thunder-1024/XOF-512 with c = 1088) load
   completely different IV lane sets. So sharing the permutation across output
   lengths is *not* undefended here.
2. **Distinct rate/capacity split** for the three security levels
   (576/832/1088), so messages are even blocked differently.
3. **Final-block constant** `C = 0^{c-1}‖1` separating the last block from all
   others; this is also the length-extension defence (§2.2, Table 1 footnote),
   which matters more than usual because the digest is read from the inner part.

The one caveat is inherent to any XOF: within a single XOF instance, outputs of
different lengths are prefixes of one another (`Thunder-XOF-256(M,256)` is a
prefix of `Thunder-XOF-256(M,512)`). The spec presents these as XOFs, so this is
expected behaviour, not a deviation — but the three harness-exposed fixed
lengths (1280/1152/1024) are *not* separate hash functions and must not be
treated as such.
