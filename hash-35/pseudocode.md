# hash-35 Wish — algorithm summary

Wish is a **wide AES-based hash family** built from a tweakable permutation
`Wish-P_τ` in the **Sponge-with-feed-forward (Sponge-F)** mode of Guo et al.
(spec ref [GHJ+25], the same CRYPTO-2026 mode family as hash-33 Thunder).
`Wish-P_τ` follows the "parallel-AES-rounds-then-mix" paradigm: the `n`-bit state
is `ℓ = n/128` AES lanes; each *step* applies AddTweak, two key-less AES rounds
to every lane in parallel (with a 128-bit round constant into lane 0 after each),
then a state-wide byte permutation `Mix`. The tweak is the absorption **block
counter**, the capacity is fed forward across each permutation call, and the
digest is the final **capacity** (`r = c = h = n/2`).

Specification: `hash-35-spec.pdf` (24 pages), §3 "Algorithm Description"
(Algorithms 1–2, Tables 2–4, Eqs. 1–5). English.

## Parameters

| parameter | Wish-512 | Wish-1024 | meaning |
|---|---|---|---|
| n | 1024 | 2048 | state width (bits) |
| ℓ = n/128 | 8 | 16 | AES lanes |
| t | 9 | 12 | steps of Wish-P |
| AES rounds per step (ρ) | 2 | 2 | key-less `MixColumns∘ShiftRows∘SubBytes` |
| total AES rounds | 18 | 24 | = 2t |
| r (rate) | 512 | 1024 | = n/2, the first ℓ/2 lanes |
| c (capacity) | 512 | 1024 | = n/2, the last ℓ/2 lanes |
| h (digest) | 512 | 1024 | = c |
| Mix | `Mix_1024` (Eq. 2) | `Mix_2048` (Eq. 3) | byte permutation, 128 / 256 entries |
| Rcon used | Rcon[0..17] | Rcon[0..23] | first 2t of Table 3 |
| tweak τ_i | `enc64(i) ‖ 0^64` | same | XORed into lane `L_{ℓ/2}` each step |
| θ (final-block const) | `0^{c-1}1` | `0^{c-1}1` | domain separator |
| IV | all-zero | all-zero | `R_0 = 0^r, C_0 = 0^c` |
| max message | 2^64 − 1 | 2^64 − 1 | bits |
| claimed collision | 256 | 512 | bits, Table 4 (= h/2) |
| claimed preimage | 512 | 1024 | bits, Table 4 (= h) |
| claimed 2nd preimage | 512 | 1024 | bits, Table 4 (= h) |
| claimed quantum col/pre/spre | 128† / 256 / 256 | 256† / 512 / 512 | † conservative lifting bound |

Sizes (bytes), specification vs the built reference library (OBSERVED):

| instance | digest spec | digest impl | match |
|---|---|---|---|
| Wish512 (spec name "Wish-512") | 64 | 64 | yes |
| Wish1024 (spec name "Wish-1024") | 128 | 128 | yes |

## Pseudocode

### Wish-P_τ (spec Algorithm 1, §3.3)
```
State S = L_0 || L_1 || ... || L_{l-1},  each L_k a 128-bit AES lane
         (byte i+4j+16k of S is row i, column j of lane k — AES column order)
AESR(L) = MixColumns o ShiftRows o SubBytes(L)      # key-less AES round

Wish-P_tau(S):
  for j = 0 .. t-1:                                  # Step_j
      L_{l/2} <- L_{l/2} XOR tau                     # AddTweak, every step
      for k = 0 .. l-1:  L_k <- AESR(L_k)            # AESR_{2j}
      L_0 <- L_0 XOR Rcon[2j]                        # AddConsts_{2j}
      for k = 0 .. l-1:  L_k <- AESR(L_k)            # AESR_{2j+1}
      L_0 <- L_0 XOR Rcon[2j+1]                      # AddConsts_{2j+1}
      S   <- Mix_n(S)         # byte perm: out[i] = in[Mix[i]]
  return S

Rcon[0] = 243f6a8885a308d313198a2e03707344            # frac(pi), 32 hex digits
Rcon[j] = LFSR(Rcon[j-1]),  LFSR(x) = (x6,x5,x4,x3,x2,x1,x0, x7 XOR x5) per byte
                                                      # Skinny/Deoxys LFSR
```

### Hash(M, h) — Wish-n (spec Algorithm 2, §3.4–§3.5)
```
Wish(M):
  r <- n/2 ; c <- n/2 ; h <- c
  # Padding pd10*: append 1 then zeros to the smallest multiple of r > |M|
  b  <- floor(|M|/r) + 1
  M_0 || ... || M_{b-1} <- pd10*(M),  |M_i| = r
  R <- 0^r ;  C <- 0^c ;  theta <- 0^{c-1} || 1
  for i = 0 .. b-1:
      tau_i <- enc64(i) || 0^64                    # block counter as 128-bit tweak
      R <- R XOR M_i                               # absorb into the rate
      if i = b-1:  C <- C XOR theta                # mark the final block
      F <- C                                       # feed-forward value
      R || C <- Wish-P_{tau_i}(R || C)
      C <- C XOR F                                 # feed-forward into capacity
  return left_h(C)                                 # digest IS the capacity
```
Since `h ≤ c`, squeezing needs **no** extra permutation call (the "FOL" property
of Sponge-F).

## Implementation vs specification

Checked: `src/Wish512/wish.c` (257 lines, **byte-identical** to
`src/Wish1024/wish.c`), `CryptHash_AlgorithmInstance.c` (27 lines, also
identical) and both headers. Note the instance *directories* are named
`Wish512_reference` / `Wish1024_reference` while `ALGORITHM_INSTANCE` is
`"Wish512"` / `"Wish1024"` — the labels in OBSERVED and RESULTS.md follow the
instance strings. Both instances KAT-PASS (`results/summary.tsv`).

Agreements:

- `S_Box[]` is the standard AES S-box (`63 7c 77 7b f2 6b …`); `MC_Matrix` is the
  standard AES MixColumns matrix; `ShiftRows()` implements row `r` ← shift left
  by `r` in AES column-major order; `AESR()` = `MixColumns∘ShiftRows∘SubBytes`
  with no AddRoundKey, as §3.3 specifies.
- `mix_1024[128]` and `mix_2048[256]` reproduce Eqs. 2 and 3 entry for entry in
  row-major order, and are applied as `temp[i] = state[mix[i]]` — exactly the
  spec's `σ(S)[i] = S[σ(i)]` convention (§2).
- `rcon[24]` reproduces Table 3 exactly (checked rows 0, 1, 12 and 23 byte for
  byte); `rcon[0] = 24 3f 6a 88 …` is frac(π). I also verified the LFSR rule:
  `0x24 = 0b00100100 → 0b01001001 = 0x49 = rcon[1][0]`, matching Eq. 4.
- Step structure `AddTweak → AESR → Rcon[2j] → AESR → Rcon[2j+1] → Mix`, with
  the tweak re-added at **every** step and the constants going to **lane 0**
  only — Algorithm 1 lines 3–10.
- `num_steps = (n == 1024) ? 9 : 12` = Table 2's `t`.
- Padding: `num_blocks = msg_len_bits / block_len_bits + 1` = `b = ⌊|M|/r⌋ + 1`
  (Eq. in §3.4); the `1` bit is placed MSB-first (`0x80 >> msg_tail_bits`, or
  `0x80` on a byte boundary) and the rest is zero — `pd10*`. A message that is a
  multiple of `r` correctly gets a whole extra block.
- `theta[15] = 0x01` XORed into the **last** lane = `θ = 0^{c-1}1` (§3.4), and
  only when `blk == num_blocks - 1` — Algorithm 2 lines 7–8.
- Feed-forward: `feedfwd` is snapshotted from the capacity **after** the message
  and `θ` have been XORed in (`F_i = right_c(X_i)`), and XORed back into the
  capacity after the permutation — Algorithm 2 lines 9–11.
- Digest = the capacity half of the final state (`memcpy(digest,
  &state[num_lanes/2], …)`) = `left_h(C_b)` with `h = c` — Eq. 5.

Discrepancies:

- **(a) Real deviation — the tweak is encoded little-endian (recorded as hash-35-5).**
  Spec §3.3.2: "`τ = enc64(i) ‖ 0^64`, where enc64 denotes the **big-endian**
  64-bit encoding of the block index", reinforced by §2 ("all multi-byte strings
  are processed in big-endian order"). The implementation does
  `for (b = 0; b < 8; ++b) tweak[b] = (byte)(blk >> (b * 8));`
  which places the **least** significant byte of the counter at `tweak[0]`, i.e.
  **little-endian**. The two agree only for block index 0, so every message
  longer than one rate block (64 bytes for Wish-512, 128 for Wish-1024) is
  affected. The shipped KATs — which include 8 388 608-bit (1 MiB, ≈16 384-block)
  messages — were generated with the little-endian code and pass, so an
  independent implementation written from the spec would **fail** the submitted
  test vectors. Security impact is nil (the counter is still an injective,
  distinct-per-position tweak, so the §5.2 second-preimage argument survives),
  but the specification does not define the implemented function.
- **(b) Robustness — no digest-length validation, out-of-bounds read possible.**
  `CryptHash()` forwards its `digest_len_bits` argument unchecked as
  `Wish_hash(digest_len_bits, 2*digest_len_bits, …)`; it never compares it with
  the instance's `DIGEST_BIT_LENGTH`. Inside `Wish_hash`, `num_steps` and `mix`
  are selected by `state_len_bits == 1024 ? … : …`, so **any** state width other
  than 1024 falls through to `mix_2048` and 12 steps. Calling the library with,
  say, `digest_len_bits = 256` gives a 512-bit (64-byte) state indexed by
  `mix_2048[]` entries up to 255 — a heap over-read of up to 192 bytes, and a
  meaningless digest. The NGCC harness only ever passes `DIGEST_BIT_LENGTH`, so
  the KATs are unaffected, but the exported `CryptHash` ABI is not safe against
  its own documented argument. Compare hash-34, which rejects any other length.
- (b, consequence) Because `CryptHash` ignores `DIGEST_BIT_LENGTH` and `wish.c`
  is identical in both directories, `libWish512.so` and `libWish1024.so` are the
  **same function**; the instance is chosen entirely by the caller's argument.
- (cosmetic) Algorithm 2 line 3 writes the separation constant as "`θ = 1c`",
  while §3.4 defines `θ = 0^{c-1}1`. The implementation follows §3.4. Also, the
  `wish.h` comment documents a `num_steps` parameter that the prototype does not
  have.
- (cosmetic) `malloc()` return values are never checked, and a fresh `temp`
  buffer is `malloc`/`free`d inside the innermost step loop. Correct but
  wasteful; no security impact.

### Domain separation between digest lengths

Wish-512 and Wish-1024 do **not** share a permutation: `n` (1024 vs 2048), `t`
(9 vs 12) and `Mix` (Eq. 2 vs Eq. 3) all differ, as do the rate, the capacity
and the digest length. Both start from an all-zero state and share the same
`Rcon` array (the 512 instance uses its first 18 entries, the 1024 instance all
24), but since the state widths differ there is no truncation relation between
the two outputs and no instance-specific IV is needed. Within one instance the
digest length is fixed at `h = c`, so there is no variable-output-length issue
of the kind that would require a suffix. The per-block tweak `τ_i` additionally
separates absorption positions, and `θ` separates the final block — the latter
is what the spec relies on for length-extension resistance (§5.2), correctly:
the digest is the capacity of a state whose rate half (`r = h` bits) is never
revealed, and the same block would be processed with `0^c` instead of `θ` if
more blocks followed.

Not verified: the active-S-box / differential-trail counts of §5.3, and whether
`Mix_1024`/`Mix_2048` have the branch-number properties claimed in §4.2.
