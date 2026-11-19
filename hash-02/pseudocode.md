# hash-02 AXIS — algorithm summary

A stream-cipher-style ("expandable iterative stream") hash, not a sponge and
not Merkle-Damgard. The 1536-bit state is eight 192-bit shift registers in a
ring. One *beat* updates all eight registers: each register absorbs one
message (or constant) bit, receives a 6-to-1 nonlinear contribution from
registers `j+1, j+3, j+5`, one linear bit from register `j-1` and one nonlinear
bit from itself, XORs the result into six fixed tap positions, then shifts by
one. Hashing is three phases of beats: message processing, 1024 blank
finalisation beats driven by a constant stream, and a digest-generation phase
whose beats emit 1 or 2 output bits each. There is no permutation, no round
function, no rate/capacity split in the sponge sense.

Specification: `hash-02-spec.pdf` (57 pages), §1.2 (notation, `ExL`, `UpL`,
beat counts), §2.1 (state), §2.2 (padding, SBox, `UpRes`/`UpFull`/`ReUpFull`,
`GenDigest`), §2.3 + Alg. 1 (the hash), Table 1 (initial constants), Table 2
and Table 4 (security goals). English-language spec.

## Parameters

| parameter | AXIS-512 | AXIS-768 | AXIS-1024 | meaning |
|---|---|---|---|---|
| state width | 1536 | 1536 | 1536 | 8 registers x 192 bits |
| register width | 192 | 192 | 192 | one shift register |
| "rate" (message bits/beat) | 2 | 1.5 (avg) | 1 | §2.3 message processing |
| "capacity" | n/a | n/a | n/a | not a sponge; the whole state chains |
| "rounds" | n/a | n/a | n/a | beats, not rounds |
| `NMeBeat` | `len(Mbar)/2` | see below | `len(Mbar)` | message beats |
| `NFinBeat` | 1024 | 1024 | 1024 | blank beats, all instances |
| `NDigGen` | 256 | 512 | 1024 | digest-generation beats |
| digest bits/beat | 2 | 1.5 (avg) | 1 | §2.3 digest generation |
| digest `d` | 512 | 768 | 1024 | `NDigGen x bits/beat` |
| `ExL` (indices 6..0) | `(11,125,107,96,32,12,0)` | same | same | extraction taps |
| `UpL` (indices 5..0) | `(188,178,162,90,75,66)` | same | same | injection taps |
| nonlinear function | `x5x4 ^ x3x2 ^ x1x0` | same | same | 6-to-1, §2.2 Eq. (1) |
| initial state | Table 1 constants | same | same | identical for all three |
| classical collision | 256 | 384 | 512 | Table 2 |
| classical preimage | 512 | 768 | 1024 | Table 2 |
| classical 2nd-preimage | 512 | 768 | 1024 | Table 2 |
| quantum collision | 128 (`d/4`) | 192 | 256 | Table 4 (deliberately conservative) |
| quantum pre / 2nd-pre | 256 | 384 | 512 | Table 4 |

`NMeBeat` for AXIS-768 (`L = len(Mbar)`): `2L/3` if `L mod 192 = 0`;
`2L/3 + 64/3` if `L mod 192 = 64`; `2L/3 + 32/3` if `L mod 192 = 128`.

The three instances differ **only** in the message/digest bit rates and
`NDigGen`. The state, taps, nonlinear function and initial constants are
identical, and so is `NFinBeat`. There is no domain separation between the
three instances beyond the differing beat schedule.

Digest sizes, specification vs the built reference library
(`OBSERVED/hash-02.txt`):

| instance | digest spec (bits) | digest impl (bits) | bytes | match |
|---|---|---|---|---|
| AXIS-512 | 512 | 512 | 64 | yes |
| AXIS-768 | 768 | 768 | 96 | yes |
| AXIS-1024 | 1024 | 1024 | 128 | yes |

## Pseudocode

Indices follow the spec's convention: `X[0]` is the **rightmost** bit, and
`S^j[i+1] = S^j[0]` when `i = 191` (bit indices mod 192, register indices
mod 8).

### Padding — spec §2.2 `Pad10^{s+1}1`

```
s <- ( -len(M) - 3 ) mod 64
Mbar <- M || 1 || 0^(s+1) || 1 || Binary64( len(M) )     # |Mbar| = 0 mod 64
```

### Register update — spec §2.2 `UpRes(S, j, m, ExL, UpL)`

```
 1  tau <- SBox( S^{j+5}[ExL[3]], S^{j+5}[ExL[2]],
                 S^{j+3}[ExL[5]], S^{j+3}[ExL[1]],
                 S^{j+1}[ExL[4]], S^{j+1}[ExL[0]] )
 2  b   <- m XOR SBox( S^j[ExL[5]], S^j[ExL[2]], S^j[ExL[4]],
                       S^j[ExL[1]], S^j[ExL[3]], S^j[ExL[0]] )
             XOR tau XOR S^{j-1}[ExL[6]]
 3  t0  <- S^j[ExL[2]] XOR S^j[ExL[1]] XOR S^j[ExL[0]] XOR b
 4  t1  <- S^j[ExL[5]] XOR S^j[ExL[4]] XOR S^j[ExL[3]] XOR b
 5  for i in UpL[2,1,0] (=90,75,66):    S^j[i] <- S^j[i] XOR t1
 6  for i in UpL[5,4,3] (=188,178,162): S^j[i] <- S^j[i] XOR t0
 7  S^j[191..0] <- S^j[0,191,...,1]              # cyclic shift by one position
```

`SBox(x5,x4,x3,x2,x1,x0) = x5*x4 XOR x3*x2 XOR x1*x0` (three AND terms only).
`UpRes` reads the *current* state, so within a beat register `j` already sees
the updated registers `0..j-1`.

```
UpFull (S, mu||mv, ExL, UpL): for j = 0..7: UpRes(S, j, (j even ? mu : mv), ...)
ReUpFull(S, c7||..||c0, ExL, UpL): for j = 0..7: UpRes(S, j, c_j, ...)
```

### Digest extraction — spec §2.2 `GenDigest(S, ExL[0])`, `ExL[0] = 0`

```
h_mu <- S^6[0] XOR S^4[0] XOR ( S^2[0] AND S^0[0] )
h_nu <- S^7[0] XOR S^5[0] XOR ( S^3[0] AND S^1[0] )
```

### The hash — spec Alg. 1

```
 1  S <- initS                                       # Table 1, 8 x 192 bits
 2  Mbar <- Pad10^{s+1}1(M)
 3  for t = 0 .. NMeBeat-1:                          # message processing
 4      S <- UpFull(S, mu||mv, ExL, UpL)             # bits taken from Mbar
 5  C <- ... || initS || initS                       # repeated constant stream
 6  for i = 0 .. NFinBeat-1 (= 1023):                # finalization, no output
 7      S <- ReUpFull(S, c7||..||c0, ExL, UpL)
 8  for i = 0 .. NDigGen-1:                          # digest generation
 9      h_i <- GenDigest(S, ExL[0])                  # 1 or 2 bits, see below
10      S   <- ReUpFull(S, c7||..||c0, ExL, UpL)
11  return HV = h_{NDigGen-1} || ... || h_0
```

Message-bit schedule per beat (§2.3): AXIS-512 takes `mu||mv = Mbar[i+1]||Mbar[i]`
every beat; AXIS-1024 takes `mu = mv = Mbar[i]` every beat; AXIS-768 alternates
(`mu = mv = Mbar[i]` on even beats, two fresh bits on odd beats), with the last
64 bits (if `|Mbar| mod 192 = 64`) or last 32 bits (if `= 128`) forced to
one-bit beats. Digest schedule is the mirror image: AXIS-512 emits
`h_mu||h_nu` every beat, AXIS-1024 emits `h_mu` only, AXIS-768 emits `h_mu` on
even beats and `h_mu||h_nu` on odd beats.

### Variable digest lengths

Not an XOF and not variable. `CryptHash()` rejects
`digest_len_bits != DIGEST_BIT_LENGTH` with `-2`. The digest length is
determined by `NDigGen` and the per-beat output rate, both fixed per instance.
Note that the digest-generation phase *is* structurally a squeeze and could
produce any length, but the spec defines only the three.

## Implementation vs specification

Checked: `hash-02/src/AXIS-512/axis_core.c` (1703 lines), `def.h`,
`CryptHash_AlgorithmInstance.{c,h}`. `axis_core.c` and `def.h` are
**byte-identical** across the three instance directories; only
`CryptHash_AlgorithmInstance.c` (the `AXIS_VARIANT_*` argument) and the header
differ.

Verified agreements:

- **`ExL` and `UpL`** (`def.h:22-34`) match §1.2 exactly:
  `AXIS_EXL_0..6 = 0,12,32,96,107,125,11` and
  `AXIS_UPL_0..5 = 66,75,90,162,178,188`.
- **Initial constants**: `AXIS_INIT_WORDS` stores each 192-bit constant as
  three little-endian 64-bit words; reassembled
  (`w2||w1||w0`) they are exactly Table 1's `Constant0..Constant7`
  (e.g. `Constant0 = 243F6A88 85A308D3 B7E15162 8AED2A6A 9E3779B9 7F4A7C15`).
  `axis_get_bit` indexes bit 0 as the LSB, i.e. the spec's right-to-left
  numbering.
- **`SBox`**: `axis_sbox6(x0..x5) = (x0&x1)^(x2&x3)^(x4&x5)`, invoked with the
  argument order that reproduces §2.2's
  `S^{j+5}[ExL[3]]*S^{j+5}[ExL[2]] ^ S^{j+3}[ExL[5]]*S^{j+3}[ExL[1]] ^
  S^{j+1}[ExL[4]]*S^{j+1}[ExL[0]]` for `tau`, and
  `S^j[ExL[5]]*S^j[ExL[2]] ^ S^j[ExL[4]]*S^j[ExL[1]] ^ S^j[ExL[3]]*S^j[ExL[0]]`
  for the self term (`axis_upres_b_default`, `axis_step_generic`,
  `axis_reupfull`). All three copies of the update agree with each other and
  with the spec.
- **`t0`/`t1` tap assignment**: `UpL[2,1,0]` (66/75/90) get `t1` and
  `UpL[5,4,3]` (162/178/188) get `t0` — the spec's *crossed* assignment
  (`t1` to the low taps, `t0` to the high taps) is reproduced correctly; this
  is easy to get backwards and the code has it right.
- **Shift**: `axis_shift_right_one` gives `new[i] = old[i+1]`, `new[191] =
  old[0]`, exactly §2.2 step 7 (a cyclic shift, not a logical shift).
- **Sequential in-register update**: registers are updated in place in the
  order `j = 0..7`, so `S^{j-1}` is the already-updated value — matching
  `UpFull`'s sequential definition.
- **`GenDigest`**: `axis_gen512_h0/h1` are exactly `S^6[0]^S^4[0]^(S^2[0]&S^0[0])`
  and `S^7[0]^S^5[0]^(S^3[0]&S^1[0])`, with `ExL[0] = 0`.
- **`NFinBeat = 1024`** for all three (`AXIS512/768/1024_BLANK_ROUNDS`), and
  `NDigGen` = 256 / 512 / 1024 beats producing 512 / 768 / 1024 bits, matching
  §1.2 and §2.3. Output ordering is `HV = h_{N-1}||...||h_0` (first generated
  bit lands at the rightmost position).
- **AXIS-768 tail rule**: `axis_768_one_bit_tail()` returns 64 when
  `(|Mbar|/64) mod 3 = 1` and 32 when `= 2`, which is exactly
  `|Mbar| mod 192 = 64` and `= 128`; I checked algebraically that the
  resulting beat count equals the spec's three-case `NMeBeat` formula in all
  three cases.
- **Constant stream**: `C` is the infinite repetition of `initS`, read from
  offset `|Mbar|` (`axis_constant_stream_bit`), with `c_j` taken from stream
  position `8*beat + j` — consistent with "determined by the length of the
  padded message" and "the lowest eight consecutive bits".
- **Bit order**: the padded message is consumed from `|Mbar|-1` downwards in
  the implementation's left-to-right indexing, which is index `0` upwards in
  the spec's right-to-left indexing — the two conventions match.

Discrepancies:

- **(a) REAL DEVIATION (recorded as hash-02-8) — the padding zero count is off by 64 bits when
  `|M| = 62 (mod 64)`.** The spec's `Pad10^{s+1}1` (§2.2) uses
  `s = (-len - 3) mod 64` and appends `s+1` zeros, i.e. **between 1 and 64**
  zero bits (total padding 67..130 bits). The implementation computes
  `zero_count = (64 - ((message_bits + 2) & 63)) & 63`
  (`axis_core.c:1050`, and identically at `:1096`, `:1306`, `:1401`, `:1465`), i.e.
  **0 to 63** zeros (total padding 66..129 bits). The two agree for every
  residue of `|M| mod 64` except `62`, where the spec appends
  `s+1 = 64` zeros and the implementation appends **none** — producing
  `M || 1 || 1 || Binary64(|M|)`, which is not even of the form `10*1` that the
  padding function's own name asserts. The padded message is 64 bits (one
  192-bit-aligned group of beats) shorter than the spec requires, so
  `NMeBeat` and the digest both differ. Affected lengths are exactly
  `|M| in {62, 126, 190, 254, ...}`, all of which have 6 bits in the final
  partial byte. The shipped KAT generator sweeps every bit length from 0 to
  4096, so the shipped KAT files **do** contain these lengths and encode the
  implementation's behaviour; an independent implementation written from the
  specification would fail the KATs at `Msg_Len = 62`. This is a concrete,
  testable spec-vs-implementation conformance defect. Both rules are
  individually injective (the `Binary64(|M|)` field disambiguates), so I see
  no security consequence — it is an interoperability defect, and either the
  specification or the code has to change.
- **(b) spec text errors.** §1.2 defines `NDigGen` as "the number of beats in
  the finalization phase" — a copy-paste of the `NFinBeat` entry; it is the
  digest-generation beat count. §2.2 writes `Pad10^{s+1}1(M)` in the heading
  but Alg. 1 line 2 calls it `Pad10^{s+1}1(64, M)` with an undocumented first
  argument.
- **(b) dead code / unverified equivalence.** `axis_core.c` carries two
  parallel implementations of every phase: a bit-at-a-time reference path and
  a 32-beat "SoA" block path selected by `axis_hash_byte_aligned_fast()` for
  byte-aligned messages, plus a generated header
  `axis_zero32_expr_word_generated.h`. The byte-aligned fast path is what the
  ICCS API actually uses for byte-aligned inputs, and it is *not* the path I
  traced against the specification. I verified the bitwise path
  (`axis_step_default` / `axis_step_generic` / `axis_reupfull`) line by line
  against §2.2; **I did not verify that the 32-beat block path computes the
  same function.** Any divergence between the two would be invisible to the
  shipped KATs only if the KATs were generated with the same path, which they
  were. This is an audit gap, stated explicitly.
- **(b) `axis_binary64_double_bits()` is a no-op** (`axis_core.c:825`) whose
  name and comment claim it "encodes the 64-bit length field with each source
  bit duplicated as required by the AXIS padding rule" — it just returns its
  argument. The spec has no bit-duplication in the length field, so the code
  is right and the comment is a leftover from an earlier design; flagged
  because the name actively misleads a reviewer.
- **(b) robustness.** `axis_core_update_bits()` silently sets
  `allocation_failed` on OOM and `axis_core_final()` then returns an
  **all-zero digest** with return code 0 from `CryptHash()`. A caller cannot
  distinguish a genuine digest from an allocation failure. Also, the context
  buffers the entire message in heap memory even though the construction is
  a pure stream and needs no buffering.
- **(b) no domain separation between instances.** The three variants share the
  identical initial state, taps and `NFinBeat`; they differ only in the beat
  schedule. The spec offers no argument that e.g. a prefix relationship
  between AXIS-512 and AXIS-1024 digests is impossible. Not a deviation, but
  the spec's §3 makes no cross-instance claim.
- **Not verified:** the security claims of §3, the algebraic-degree estimates
  in Table 3, and the equivalence of the two implementation paths noted above.
  All three instances pass the candidate's KATs in the build
  (`hash-02/security_findings.md`).
