# kex-08 NIIKE — algorithm summary

A genuinely **non-interactive** key exchange (0 passes) from a free and
transitive ideal-class-group action on **oriented** supersingular elliptic
curves — a CSIDH-style scheme, but with the orientation given by an imaginary
quadratic order `O = Z[sigma]` of large discriminant (`Nrd(sigma) = M^r`) on
curves over `F_{p^2}`, rather than by Frobenius over `F_p`. The orientation is
carried explicitly as a *full kernel representation*: a cycle of `r` curves
joined by `M`-isogenies, `E = {(E_j, P_j, Q_j) | 1 <= j <= r}`. Both public key
and secret key are static; the shared secret is `j([a] * E_other)`, the
j-invariant of the acted-on curve. Security rests on the vectorisation /
parallelisation problem for this class-group action.

Specification: `kex-08-spec.pdf` (63 pages), §3.3-3.6 (class-group action,
Alg. 2-3, tail pruning), §4.1-4.4 (setup, Alg. 4 KeyGen, Alg. 5 KeyAgr, key
sizes), §5.1-5.5 (parameters), §8 (implementation), §9.3 (side channels),
§10 (failure analysis). English-language spec.

**I did not run this candidate**: the reference implementation is ~15-18
gigacycles per operation at NGCC-I and ~68000 gigacycles (about a day per
exchange) at NGCC-III, per spec Table 7.2 and the build's own notes. Everything
below is from reading the specification and the source.

## Parameters

| parameter | NIIKE-lv128 | NIIKE-lv256 | NIIKE-lv512 | meaning |
|---|---|---|---|---|
| `lambda` | 128 | 256 | 512 | target classical security |
| `log2 p` (spec says) | 248 | 511 | 16392 | §5.5 |
| `log2 p` (actual) | **255** | 511 | ~16393+ | measured from the printed `p` |
| `p mod 4` | 3 | 3 | 3 | so `E0: y^2 = x^3 + x` is supersingular |
| `r` = cycle length | 13 | 14 | 2 | number of curves in the kernel representation |
| `B` = iterations | 35 | 70 | 1 | class-group action repetitions |
| `n` = #primes (spec) | 49 (31 + 18) | **83** (53 + 30) | 758 (implied) | `M = M+ * M-` |
| `n` = #primes (impl) | 49 (31 + 18) | **84** (54 + 30) | 759 (759 + 0) | `P_LEN + M_LEN` |
| `M+` | divides `p+1` | divides `p+1` | `p+1` | `3^4*5*7*11^2*...*269` at lv128 |
| `M-` | divides `p-1` | divides `p-1` | `1` | `13*17*...*241` at lv128 |
| `e_i` | mostly 1 (`3^4`, `11^2`) | mostly 1 (`2^6,7^3,11^2`) | 1 or 2 | exponents |
| secret key space | `prod (1 + B*e_i)` | same | same | Eq. (4.1), must be `>= 2^{2 lambda}` |
| `log2` of that (impl) | 256.28 | 521.71 | 1202.4 | I recomputed these |
| shared secret | `j(E) in F_{p^2}` | same | same | **no KDF, raw j-invariant** |
| passes | 0 | 0 | 0 | true NIKE |

Sizes (bytes), specification Table 7.1 ("Current Impl." column) vs the built
reference library (`OBSERVED/kex-08.txt`):

| instance | sk spec | sk impl | pk spec | pk impl | ss spec | ss impl | sta/stb/msgs | match |
|---|---|---|---|---|---|---|---|---|
| NIIKE-lv128 | 196 | 196 | 4030 | **4160** | 62 | **64** | 0 / 0 / 0 | sk yes, pk/ss no |
| NIIKE-lv256 | 332 | **336** | 8943 | **8960** | 128 | 128 | 0 / 0 / 0 | ss yes, sk/pk no |
| NIIKE-lv512 | 3032 | **3036** | 40980 | **41000** | 2049 | **4100** | 0 / 0 / 0 | none |

Every impl figure is consistent with
`pk = r * 5 * FP2_ENCODED_BYTES`, `sk = 4*n`, `ss = FP2_ENCODED_BYTES`, with
`FP_ENCODED_BYTES` = 32 / 64 / 2050. The mismatches are analysed below.

## Pseudocode (pass structure)

`kex_get_passes_num() = 0` — matching OBSERVED `passes=0`, `msgs=0`,
`sta=0`, `stb=0`. There are **no protocol messages at all**: each party
publishes a static public key and both derive the same secret from their own
secret key and the peer's public key.

### Init_A / Init_B — spec Alg. 4 `NIIKE.KeyGen()`

Identical for both parties.

```
Init_X():
 1  sk = (sk_i)_{i=1..n}  <- uniform in [0, B*e_i]        # rejection-sampled
 2  E <- E0                                               # distinguished origin
 3  (s_i) <- sk
 4  for j = 1 .. B:
 5      for k = 1 .. n:  s_k <- s_k - e_k ;  s'_k <- max(min(s_k, e_k), 0)
 6      E <- GroupActionofNIIKE(E, (s'_i))
 7  pk <- E = {(E_j, P_j, Q_j) | 1 <= j <= r}
 8  st <- (empty, 0 bytes)
```

Line 1 is what NIIKE-lv128 does. NIIKE-lv256 samples the same way and then
sets `sk[0] = 0` (`NIIKE-lv256/protocols/bundleprotocols_internal.c:63`).
NIIKE-lv512 does not sample: `make_SecretKey` alternates between two
hard-coded vectors (`NIIKE-lv512/protocols/bundleprotocols_internal.c:53-79`,
kex-08-2).

The clamp `s'_k = max(min(s_k, e_k), 0)` turns the secret `sk_k in [0, B e_k]`
into a radix-`e_k` "unary" schedule: at most `e_k` `l_k`-isogeny steps per
iteration, greedily front-loaded, so over `B` iterations exactly `sk_k` steps
are taken in the `+` direction and the remaining `B e_k - sk_k` in the `-`
direction. That is what makes the walk deterministic, dummy-free and
constant-time: the *same number* of isogenies is always computed; only the
direction depends on the secret.

### passes 1, 2, 3

None. `kex_generate_pass1_msg_a`, `kex_generate_pass2_msg_b` and
`kex_generate_pass3_msg_a` are stubs that return 0 and emit nothing.

### DeriveSS_A / DeriveSS_B — spec Alg. 5 `NIIKE.KeyAgr(E_peer, sk)`

Identical for both parties (this is the non-interactive part).

```
DeriveSS(pk_peer, sk):
 1  E <- decode(pk_peer) ;  (s_i) <- sk
 2  for j = 1 .. B:
 3      for k = 1 .. n:  s_k <- s_k - e_k ;  s'_k <- max(min(s_k, e_k), 0)
 4      if j <= B - r + 1:
 5          E <- GroupActionofNIIKE(E, (s'_i))                       # full cycle
 6      else:
 7          E <- GroupActionofNIIKETailPruning(E, (s'_i), B - j + 1) # short cycle
 8  (E, _, _) <- E[1]
 9  ss <- j(E)                                            # returned raw, unhashed
```

**Tail pruning**: `{(E'_j, P'_j, Q'_j)}` depends only on tuples `j` and `j+1`
of the input, so the last `r-1` iterations need not maintain the whole cycle;
iteration `j > B-r+1` updates only the first `B-j+1` tuples, saving
`r(r-1)/2` tuple updates. The pruning schedule depends only on public indices.

### The class-group action — spec §3.5, Alg. 2

For a full kernel representation `E` and a vector `(s_i) in [0, e_i]^n`,
`[prod l_i^{s_i}] * E` is computed tuple by tuple:

```
for each cycle position j = 1..r:
    # forward direction: r isogenies from P_j
    for i = 1..n:
        phi+_{j,i} has kernel < [M / l_i^{s_i}] phi+_{j,i-1} o ... o phi+_{j,0}(P_j) >
    # backward direction: from Q_{j+1}
    for i = 1..n:
        phi-_{j,i} has kernel < [M / l_i^{e_i - s_i}] phi-_{j,i-1} o ... o phi-_{j,0}(Q_{j+1}) >
    phi+_j = phi+_{j,n} o ... o phi+_{j,1} ;  phi-_j = phi-_{j,n} o ... o phi-_{j,1}
    E'_j = codomain(phi+_j) = codomain(phi-_j)
    P'_j = phi-_j (P_{j+1}) ;  Q'_j = phi+_j (Q_j)
```

Each `l_i`-step goes in the `+` or the `-` direction according to
`t > s_i` (`t = 1..e_i`); the implementation realises this with a
constant-time masked swap of the two working vertices rather than a branch, so
all `sum_i e_i` isogenies are always computed. There is no dummy isogeny.

### Constant-time concerns (what the design does, and where it stops)

- **Deterministic and dummy-free by construction** (§3.5, §9.3). Every
  `l_i`-isogeny of the schedule is always evaluated; only the *direction*
  (which of the two vertices `R[0]`, `R[1]` is the "active" one) is secret, and
  that is selected by `bundleswap_vertices(R[0], R[1], b, len)` with
  `b = compare_gt(t, s[i])` — a full-word 0/-1 mask, not a branch.
- **The isogeny-degree schedule is public**: loop bounds are `e_i` and `n`,
  both public; the "strategy" arrays `strategy_s/strategy_t` are public
  precomputed data; the one `if (stack_scalar[top] < degree_s_num - jj)` in the
  strategy walk is on public counters and is annotated as such in the source.
- **Tail pruning uses only public indices** (`tasklength = min(B-i, r)`), so it
  leaks nothing.
- **Where constant time is *not* achieved**: secret-key sampling.
  `make_SecretKey()` is a rejection loop
  (`do { draw; mask; } while (sk[i] > B*e_i);`) whose iteration count depends
  on the drawn values. This is the standard, benign form of rejection sampling
  (the rejected draws are independent of the accepted one), but it does
  contradict §9.3's blanket claim that "the execution flow and instruction
  sequence are independent of the secret input" and that the implementation
  avoids "secret-dependent loop bounds".
- **`fp2_inv` / `fp2_sqrt` / `ec_isomorphism`** are used in encoding and in the
  cycle-closing correction; these are exponentiation-based (Fermat) in the
  SQIsign-derived code, but I did not audit them for constant time.
- **No public-key validation**: `decode_bundlevertex()` reads `5*r` field
  elements, sets all `Z` coordinates to 1, and returns. There is **no check**
  that the curves are supersingular, that the points have order `M`, that
  `<P_j> cap <Q_j> = {0}`, or that the tuple is a genuine full kernel
  representation of the orientation. For a scheme with *static* keys on both
  sides this is the classic adaptive-attack surface (GPST-style); see below.

## Implementation vs specification

Checked: `kex-08/src/NIIKE-lv128/{ngccapi/KEX_AlgorithmInstance.c,
protocols/bundleprotocols.c, protocols/include/protocols_helper.h}`, plus the
per-level `precomp/{torsion_constants.c, protocol_constants.c,
include/{encode_sizes.h, ec_params.h, protocol_setup.h}}` and
`NIIKE-lvN/protocols/bundleprotocols_internal.c` for all three levels.
`ngccapi/KEX_AlgorithmInstance.c` and `protocols/bundleprotocols.c` are shared
by all three instances; only the `precomp/` data and `CYCLE_LENGTH` /
`ITERATIONS` / `P_LEN` / `M_LEN` differ. The code descends from SQIsign 1.0 and
"Project OSIDH-LD" (SPDX headers), consistent with the spec's citation of
[AAA+24] and [Hou25b].

Agreements (recomputed, not just read):

- **`CYCLE_LENGTH` = `r`** = 13 / 14 / 2 and **`ITERATIONS` = `B`** = 35 / 70 / 1,
  matching §5.5 exactly for all three levels.
- **NGCC-I parameters are fully consistent.** I parsed the printed `p`, `M+`
  and `M-` and verified: `p` is a 255-bit **prime**, `p = 3 mod 4`, the printed
  factorisation `3^4 * 5 * 7 * 11^2 * ... * 269` equals the printed `M+` hex
  exactly, the `M-` factorisation equals its hex exactly, `M+ | p+1`,
  `M- | p-1`, and hence `M+ * M- | p^2 - 1` as §4.1 requires. The
  implementation's `TORSION_ODD_PRIMES[49]` and `TORSION_ODD_POWERS[49]` are
  precisely the spec's `M+` list followed by the `M-` list, and
  `p_plus_minus_bitlength[49]` matches those primes' bit lengths entry by entry.
- **NGCC-II `p` is a 511-bit prime `= 3 mod 4`** with `M+ | p+1`, `M- | p-1`.
- **`B` minimality (Eq. 4.1).** For NGCC-I, `sum_i log2(1 + 35 e_i) = 256.28 >=
  2*128` while `B = 34` gives `254.28 < 256` — so `B = 35` is exactly the
  smallest integer satisfying (4.1), as §4.1 demands. For NGCC-III, `B = 1`
  gives 1202.4 >= 1024 and is trivially minimal.
- **Tail pruning matches Alg. 5.** `niike_SecretAgreement()` uses
  `tasklength = (ITERATIONS - i >= CYCLE_LENGTH) ? CYCLE_LENGTH : ITERATIONS - i`,
  which for 0-based `i` is exactly "full cycle while `j <= B-r+1`, otherwise
  parameter `B-j+1`".
- **The clamp `max(min(s,e),0)` is implemented correctly** and in constant
  time, via `digit_t b = compare_gt(t, s[i])` driving masked vertex swaps for
  `t = 1..e_i`: `s_i >= e_i` gives all `+` steps, `s_i <= 0` gives all `-`
  steps, and intermediate values split correctly.
- **Public key contents**: `encode_bundlevertex()` writes, per cycle position,
  the five `F_{p^2}` values `A24, Ps, Pt, Qs, Qt` (affine, after a batched
  inversion), i.e. the Montgomery coefficient plus the `M+`- and `M-`-parts of
  each of `P_j` and `Q_j` — the decomposition described in §4.4.
- **Shared secret** is `ec_j_inv` of the first curve of the resulting cycle,
  matching Alg. 5 lines 13-14.

Discrepancies:

- **(a) REAL DEFECT — the ICCS API's output length parameters are never
  written.** `kex_init_a` and `kex_init_b` (`ngccapi/KEX_AlgorithmInstance.c`)
  never assign `*pka_len_bytes`, `*ska_len_bytes` or `*sta_len_bytes`; likewise
  `kex_derive_ss_a` / `kex_derive_ss_b` never assign `*ssa_len_bytes` /
  `*ssb_len_bytes`. The header documents all of these as `[out]`. A caller that
  reads them gets whatever was in the caller's buffer (uninitialised in the
  general case). The NGCC harness happens to take the sizes from the
  `kex_get_*_len_bytes()` queries, so the KATs pass, but kex-06 and kex-07 both
  set these correctly and this instance does not. **Concrete API-conformance
  defect.**
- **(a) spec/impl parameter mismatch — the NGCC-II factorisation omits `2^6`.**
  §5.5 lists `M+ = 3 * 7^3 * 11^2 * 13 * ... * 937` (53 primes), and the
  printed `M+` hex equals exactly that product. The implementation's
  `TORSION_ODD_PRIMES[84]` for `NIIKE-lv256` begins `{2, 3, 7, 11, 13, ...}`
  with `TORSION_ODD_POWERS = {6, 1, 3, 2, 1, ...}`, i.e. it uses
  `M+_impl = 2^6 * M+_spec` (`P_LEN = 54`, not 53). I verified `64 * M+ | p+1`,
  so the implementation's choice is mathematically sound — the *specification's
  listing is incomplete*. The consequences propagate: `n` is 84 not 83, the
  secret key is 336 bytes not the 332 of Table 7.1. The extra coordinate does
  not enlarge the sampled key space: `make_SecretKey` sets `sk[0] = 0` after
  sampling (`NIIKE-lv256/protocols/bundleprotocols_internal.c:63`, both trees;
  lv128 has no such line), so the `l = 2` exponent is always zero. Over the
  remaining 83 coordinates `B = 70` gives `log2 |SK| = 512.99 >= 512` and *is*
  exactly minimal (`B = 69` gives 511.29), which confirms the spec's tables
  were computed without the `2^6`. The nominal 84-coordinate space would be
  521.71 bits, and `B = 65` would already give 512.86 there. **Either the
  spec's §5.5 listing or the implementation's prime table needs correcting.**
- **(a) spec/impl parameter mismatch — NGCC-III private key.** Table 7.1 gives
  3032 B (`n = 758`); the implementation has `P_LEN = 759`, `M_LEN = 0`, hence
  3036 B. Same off-by-one pattern; §5.5 gives no explicit prime list for
  NGCC-III (`M+ = p+1`), so I could not determine which side is right.
- **(a) spec error — `log2 p` for NGCC-I.** §5.5 states `log2 p = 248`, but the
  printed `p` is a **255-bit** number (64 hex digits, leading nibble 5). All of
  Table 7.1's NGCC-I figures were computed from 248 (`ss = 62 = 2*248/8`,
  `pk = 4030 = 13*5*62`), which is why they disagree with the implementation's
  64 B and 4160 B. Also note §5.1 requires `log2 p >~ 2*lambda = 256`; the real
  `p` is 255 bits, i.e. marginally *below* the stated requirement, and the
  stated 248 is well below it. **This is the finding I would raise first for
  this candidate's parameter section.**
- **(a) spec internal inconsistency — the public-key size formula is wrong.**
  §4.4 Eq. (4.3) computes `r * (2 log p + 4 log p) = 6 r log p` bits, i.e.
  three `F_{p^2}` elements per cycle position. The implementation (and
  Table 7.1's own "Current Impl." column) uses **five**: `A24, Ps, Pt, Qs, Qt`.
  The actual public key is therefore `10 r log p` bits, 5/3 times Eq. (4.3)
  (2418 B vs 4030 B at NGCC-I). Eq. (4.3)'s prose ("storing the information of
  `P_i` and `Q_i` requires `2 * log(p^2) = 4 log p` bits") accounts for one
  x-coordinate per point, but each point is split into its `M+` and `M-` parts
  and both x-coordinates are stored.
- **(a) spec internal inconsistency — NGCC-III shared secret size.** Table 7.1
  says 2049 B, which is `log2(p)/8`, i.e. one `F_p` element. Eq. (4.6) says the
  shared secret is `log(p^2) = 2 log p` bits, and the implementation emits
  `FP2_ENCODED_BYTES = 4100`. The 2049 entry is off by a factor of two.
- **(a) spec prose error — `min` where it must be `max`.** §4.2 says "set
  `s'_k = e_k` if `s_k = sk_k - j*e_k >= e_k`, and `s'_k = min(s_k, 0)`
  otherwise". `min(s_k, 0)` is `<= 0` and would drive every step backwards;
  the correct clamp, given by Algorithm 4 line 6 and implemented in the code,
  is `max(min(s_k, e_k), 0)`. The prose contradicts the algorithm box.
- **(b) spec pseudocode off-by-one.** Algorithms 4 and 5 decrement
  (`s_k <- s_k - e_k`) at the *top* of the loop body, so the first group action
  uses `j = 1`. Then `sum_{j=1}^{B} clamp(sk_k - j e_k, 0, e_k) = sk_k - e_k`
  when `sk_k = B e_k` — the schedule applies one step too few per prime. The
  implementation applies the action with the **undecremented** key on the first
  iteration and decrements afterwards (`niike_PublicKey` / `niike_SecretAgreement`
  call `bundleeval_action_stra(R, copysk, ...)` *before* the
  `copysk[j] -= TORSION_ODD_POWERS[j]` loop), i.e. `j = 0 .. B-1`, which is the
  correct greedy decomposition. The pseudocode should start `j` at 0 or move
  the decrement to the end of the body.
- **(b) the shared secret is the raw `j`-invariant, not a hashed key.**
  `kex_derive_ss_*` returns `fp2_encode(j(E))` verbatim (64 / 128 / 4100
  bytes). The spec (Alg. 5 line 14, §4.4) specifies exactly this. The output is
  therefore a non-uniform element of `F_{p^2}` — at NGCC-I roughly 255 of the
  512 emitted bits per `F_p` half are structurally zero-padded (`p` is 255 bits
  in a 256-bit encoding), and the j-invariant ranges over supersingular
  j-invariants, not all of `F_{p^2}`. Any deployment must hash it; the
  specification never says so.
- **(b) no public-key validation for a static-key NIKE.**
  `decode_bundlevertex()` performs no supersingularity, order or
  linear-independence check on the received `5r` field elements, and the
  spec's §9 contains no validation requirement. Because both keys are static
  and the same secret is reused for every peer, this is precisely the setting
  in which adaptive (GPST-style) attacks against oriented/class-group schemes
  are launched. The spec's §9.3 side-channel discussion and §10 ("NIIKE
  guarantees flawless execution... zero failure rate") do not address
  malformed peer public keys at all. I did not construct an attack — the
  orientation's full-kernel structure may make malformed inputs detectable —
  but the absence of any validation and of any discussion of it is a gap worth
  raising with the submitters.
- **(b) `assert()` in the protocol hot path.** `bundleeval_action_stra()` uses
  `assert(top <= stack_volume)` and `assert(top == 0)` to guard a stack-indexed
  kernel buffer; compiled with `-DNDEBUG` these vanish and an out-of-range
  `top` would silently overflow `stack_kernel[2][stack_volume]`. The buffer is
  also a VLA sized by the extern `stack_volume`.
- **(b) leftover debug scaffolding.** `bundleeval_action_stra()` contains a
  `bool log = false;` plus a dozen `if (log) { time(...); printf(...); }`
  blocks and commented-out `bundlexMULv2` calls. Dead, but it clutters the one
  function that carries the whole secret-dependent computation and makes the
  constant-time argument harder to audit.
- **Not verified:** I did **not run** any instance (see above), did not verify
  the NGCC-III prime or its factorisation, did not verify the precomputed
  `bundle_base_cycle` is a valid full kernel representation of the stated
  orientation, did not audit the SQIsign-derived field/curve arithmetic for
  constant time, and did not assess the concrete security estimates of §9.
  The build reports KAT PASS for lv128 and for 3 of 10 lv256 records;
  lv512 is not built by default.
