# kem-38 UVW-KEM — algorithm summary

Code-based (**not** multivariate): an IND-CPA PKE over a generalized
(U+V, U+W) code with a Reed–Solomon subcode, made IND-CCA by a Fujisaki–Okamoto
transform with **explicit rejection**. Hardness rests on General/Syndrome
Decoding over F_q plus indistinguishability of the (U+V, U+W) public matrix from
a random generator matrix. Decryption is a Guruswami–Sudan **list decoder** for
RS over F_q (q = 433/857/1709 — *not* F_3) followed by an information-set solve.

Specification: `kem-38-spec.pdf` (34 pages, English; the `中文资料/` directory
holds a Chinese companion). §1.2.1 Algorithms 2–5 (PKE), §1.2.2 Algorithms 6–9
(KEM), §1.3 Table 1 and §4.2 Table 2 (parameters and sizes).

## Parameters

| parameter | UVW128 | UVW256 | UVW512 | meaning |
|---|---|---|---|---|
| q | 433 | 857 | 1709 | field size (⌈log2 q⌉ = 9/10/11 bits) |
| n | 860 | 1708 | 3412 | code length |
| k = k1+k2 | 430 | 854 | 1706 | code dimension |
| k1 = k2 | 215 | 427 | 853 | half-dimensions (GU / GW rows) |
| w | 116 | 232 | 463 | error weight of e |
| m | 256 | 256 | 512 | message / seed bit-length |
| RS code | [430,215] | [854,427] | [1706,853] | α = (1,2,…,n/2) |
| GS multiplicity / list bound | 4 / 5 | 5 / 6 | 5 / 6 | `RS_M` / `RS_L` (impl only) |
| single-decap success prob. | 0.988123 | 0.987827 | 0.987803 | spec §2.2 Thm 2 |
| claimed security | 128 C / 80 Q | 256 C / 128 Q | 512 C / 256 Q | bits (spec §1.3) |

Sizes (bytes), specification (§4.2 Table 2) vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | ct spec | ct impl | ss spec | ss impl | match |
|---|---|---|---|---|---|---|---|---|---|
| UVW-KEM-128 | 208013 | 208013 | 35 | 35 | 1032 | 1032 | 64 | 64 | yes |
| UVW-KEM-256 | 911645 | 911645 | 35 | 35 | 2199 | 2199 | 64 | 64 | yes |
| UVW-KEM-512 | 4001850 | 4001850 | 67 | 67 | 4820 | 4820 | 64 | 64 | yes |

All twelve figures agree. sk = seed (m/8 bytes) ‖ i1 ‖ i2 ‖ i3; pk = ⌈k(n−k)·⌈log2 q⌉/8⌉;
ct = ⌈n·⌈log2 q⌉/8⌉ + 2·(m/8). The session key is 512 bits at *every* level.

## Pseudocode

### PKE.KeyGen (Algorithm 3)
```
seed <- {0,1}^m ; i1 = i2 = i3 = 0
repeat i1++ : GU <- PRNG(seed || i1 || 1) in Fq^{k1 x n/2}  until rank(GU) = k1
repeat i2++ : GW <- PRNG(seed || i2 || 2) in Fq^{k2 x n/2}  until rank(GW) = k2
GRS = generator of the [n/2, k2] RS code with alpha = (1,2,...,n/2)
GV  = GRS + GW
repeat i3++ : D <- PRNG(seed || i3 || 3), a monomial matrix in DP(n)
              Gsk = [[GU, GU], [GV, GW]] * D
              row-reduce Gsk -> (I_k || T)          # fails iff left k x k block singular
sk = (seed, i1, i2, i3)    pk = T
```
Rejection sampling for an Fq element: take 2 PRNG bytes, keep the low ⌈log2 q⌉
bits, accept if < q (spec §1.2.1 prose).

### KEM.Encaps (Algorithm 8)
```
m  <- {0,1}^m
(r, e) = H1(m)               # r in Fq^k, e in E^{q,n}_w  (weight-w)
c1 = r * (I_k || T) + e
c2 = m XOR H3(r, e)
d  = H2(m)
K  = H4(m, c1, c2)           # 512 bits
return ct = (c1, c2, d), K
```

### KEM.Decaps (Algorithm 9 + PKE.Dec Algorithm 5)
```
# --- PKE.Dec ---
rebuild GU, GW, D from (seed, i1, i2, i3); GV = GRS + GW; Gsk = [[GU,GU],[GV,GW]]D = (A || B)
(c11 || c12) = c1 * D^{-1}
(r2, ebar)   = ListDec.RS(GRS, c11 - c12, w)       # r2*GRS + ebar = c11-c12, |ebar| <= w
I = [n/2] \ supp(ebar)
loop:  pick I1 subset of I, |I1| = k1
       r1 = (c12 - r2*GW)_{I1} * (GU)_{I1}^{-1}
       r  = (r1 || r2) * A ;  e = c1 - r*(I_k||T)
       until wt(e) <= w
m' = c2 XOR H3(r, e)
# --- FO re-encryption (explicit rejection) ---
(r',e') = H1(m') ; d' = H2(m')
(c1',c2') = PKE.Enc(pk, m'; r', e')            # pk recomputed from the seed
if (c1,c2,d) != (c1',c2',d') : abort
return K = H4(m', c1, c2)
```

### List-decoding step (spec §1.1.3; impl `src/list_decoding.c`)
```
Guruswami-Sudan, interpolation multiplicity M, list bound L (per Table above):
1. Koetter interpolation over the n/2 points (alpha_i, y_i):
   maintain L+1 bivariate polys G_j initialised to y^j; for every point and every
   (a,b) with a+b < M, evaluate the (a,b) Hasse derivative of each G_j, pick the
   survivor of least (1, k2-1)-weighted degree and cross-cancel the others.
2. Take Q = the G_j of minimum weighted degree.
3. Roth-Ruckenstein factorisation (`rr_dfs`): depth-first over the k2 message
   coefficients; at each depth enumerate ALL y in Fq with Q(0,y) = 0, substitute
   y -> y + root, divide out x^h, recurse. Capped at MAX_LIST_SIZE = 10 candidates.
4. Filter: for every candidate f(x), count the Hamming distance between its
   evaluation vector and the received word; keep the minimiser; accept only if
   that distance <= w.
```
Decoding radius: the spec does not state an explicit GS radius; the code's
acceptance test is `min_err_dist <= UVW_W` on a length-n/2 word (116 of 430 for
UVW128, against unique-decoding radius 107 and a GS radius of ≈127).

## Implementation vs specification

Checked: `Implementations/Reference_Implementation/UVW-KEM-*/src/KEM_AlgorithmInstance.c`
(KeyGen/Encaps/Decaps, matrix expansion, packing), `src/list_decoding.c` (GS
decoder), `src/gf_math.c`, `include/params.h`, `src/uvw_constants.c`. Hashes are
the official ICCS `auxfunc.c` (`pseudoXOF`/`sm3hash`, SM3-based), matching §1.3's
"instantiated via SM3-based extensible output function algorithms".

**Agreements.** All seven parameters of all three sets in `include/params.h`
match Table 1 exactly (q, q_bits, n, k, k1, k2, w, m — full check, not a sample).
All twelve size figures match §4.2 Table 2. The four hash oracles are domain-
separated by a 1-byte prefix 0x01..0x04 (`KEM_AlgorithmInstance.c:42-45`), which
§1.2.2's "four secure hash functions" permits. The FO re-encryption compares
c1, c2 *and* d and aborts on any mismatch, i.e. explicit rejection exactly as
Algorithm 9 line 4 specifies. `FIXED_G_RS` in `uvw_constants.c` is a precomputed
GRS instead of Algorithm 3 step 4's on-the-fly construction — a legitimate
equivalent optimisation since α = (1,2,…,n/2) is fixed. Both keygen and encaps
draw their randomness from the official `drng_algorithm`
(UVW-KEM-128 `KEM_AlgorithmInstance.c:160`, `:314`).

**(a) Deviation — H1 is bottlenecked to 256 bits at every security level.**
UVW-KEM-512 `KEM_AlgorithmInstance.c:314-315` (encaps) and `:411` (decaps) declare
`unsigned char seed_h1[32]` and call `kem_hash(H1_PREFIX, m, UVW_M, …, 256, seed_h1)`;
(r, e) are then expanded from that 256-bit seed with a DRNG. The constant `256`
is hard-wired identically in all three parameter directories (verified in
UVW-KEM-512). For UVW512 the spec sets m = 512 and claims 512-bit classical /
256-bit quantum security, but the encryption randomness (r, e) — and hence c1 —
ranges over at most 2^256 values. Since c2 = m ⊕ H3(r, e), guessing the 256-bit
H1 seed, checking it against c1 and unmasking c2 recovers the plaintext in 2^256
classical / ≈2^128 Grover steps, against a 512/256-bit claim. This is exactly the
class of defect the brief calls out: a hard-wired constant where the spec's
parameter (m) varies per set.

**(a) Deviation — decapsulation is randomised and consumes the global DRNG.**
Spec Definition 7(3) requires Decaps to be *deterministic*. `uvw_pke_dec` picks
the information set I1 with
`get_rand_range_batched(limit, rand_pool, &pool_pos, &drng_algorithm)`
(UVW-KEM-128 `KEM_AlgorithmInstance.c:1274`), i.e. from the shared KAT DRNG. Consequences:
decapsulation mutates global RNG state, so a decap interleaved into a
keygen/encap sequence shifts every subsequent operation; and two decapsulations
of the same (sk, ct) can take different retry paths. Algorithm 5 step 10 does
license a *re-draw* of I1, so the retry loop itself is spec-conformant; drawing
it from `drng_algorithm` rather than a private stream derived from sk‖ct is the
deviation. (This is also the likely cause of the "ciphertext-integrity testing
was incomplete or abnormal" note in `kem-38/security_findings.md`.)

**(a) Deviation — the retry loop is bounded where the spec is unbounded.**
`max_attempts = 1000` (UVW-KEM-128 `KEM_AlgorithmInstance.c:1256`) and
`KEM_KEYGEN_MAX_ATTEMPTS 100` (`:35`) cap loops that Algorithm 3 steps 2/3/8 and
Algorithm 5 step 10 state as unconditional "repeat". Exhaustion returns an error
rather than retrying; the resulting failure probability is far below the DFR, so
this is a correctness-neutral engineering choice, but it is not what the spec says.

**(b) Spec ambiguity — decapsulation failure is signalled by distinct codes.**
`kem_dec` returns −2 for a PKE-decode failure, −1 for a `d` mismatch and −1 for a
ciphertext mismatch, at three different points, with `memcmp` (non-constant-time)
comparisons at UVW-KEM-128 `:436` and `:475-476`. The spec only says "abort". Explicit rejection
plus early exit is a textbook plaintext-checking / timing oracle surface; the spec
does not address it.

**(c) Equivalent — non-canonical ct encodings cannot be exploited.**
`compress_gf_array`/`decompress_gf_array` pack ⌈log2 q⌉ bits per coefficient, so
byte strings encoding values in [q, 2^⌈log2 q⌉) are parseable. They are caught,
because the FO check compares the *unpacked* re-encrypted `c1` (always < q)
against the unpacked received `c1`. No malleability results.

**Robustness note (not a spec deviation).** In UVW-KEM-256/512
`hasse_derivative_fast` accumulates a product of five sub-q factors in a
`uint64_t` without reduction (`list_decoding.c:148-165` in the 256 tree); at
q = 857 with ≈2·10^4 terms the accumulator peaks near 9.1·10^18, inside but
close to `UINT64_MAX`.

**Not verified.** The candidate was not built or run (per instruction — the
reference decapsulator costs ~10^8–2·10^10 cycles), so nothing here is backed by
execution; the list-decoder's numerical behaviour and the `uvw_constants.c`
GRS table (5816 lines) were read but not independently recomputed.
