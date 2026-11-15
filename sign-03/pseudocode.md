# sign-03 CEDRUS+C — algorithm summary

Stateless hash-based signature in the SPHINCS+ family: a `d`-layer hypertree of XMSS
Merkle trees whose leaves are *inhomogeneous* WOTS+C one-time keys (per-chain Winternitz
parameters `w = (w_0,...,w_{len-1})`, plus a constant-sum "checksum-free" encoding found by
a counter search), signing a FORS+C few-time key at the bottom layer. Security rests only
on the (2nd-)preimage / collision resistance of SM3 and the XOF derived from it. The spec is
in English (53 pages) and is a near-verbatim re-write of the SLH-DSA/SPHINCS+ document;
§5.1 states the deltas explicitly (inhomogeneous chains, per-layer tree heights, WOTS+C and
FORS+C counters).

Specification: `sign-03-spec.pdf` (53 pages), Chapter 1 §1.1–§1.11 (Algorithms 1–18);
parameter sets Table 1.1 (p. 36), hash instantiations Tables 1.2/1.3 (p. 37).

## Parameters

Spec Table 1.1. `h_i` is not in Table 1.1; §2.3 (p. 43) fixes it as
`h_0..h_{κ-1} = ⌈h/d⌉`, `h_κ..h_{d-1} = ⌊h/d⌋`, `κ = h − d⌊h/d⌋`.
`n_ctr^OTS = n_ctr^FTS = 4` for all sets.

| parameter | 160s | 160f | 256s | 256f | 384s | 384f | 512s | 512f | meaning |
|---|---|---|---|---|---|---|---|---|---|
| n | 20 | 20 | 32 | 32 | 48 | 48 | 64 | 64 | hash/node size (bytes) |
| h | 67 | 66 | 65 | 66 | 67 | 64 | 65 | 67 | total hypertree height |
| d | 9 | 17 | 9 | 14 | 8 | 12 | 8 | 11 | XMSS layers |
| a | 12 | 7 | 13 | 8 | 13 | 10 | 14 | 11 | FORS subtree height |
| k | 13 | 30 | 22 | 44 | 33 | 55 | 44 | 59 | FORS subtrees |
| a' | 15 | 9 | 14 | 10 | 17 | 12 | 15 | 12 | zero bits forced in msg digest |
| w | [32]²×[64]²⁴ | [8]²×[16]³⁸ | [32]×[64]⁴¹ | [8]²×[16]⁶² | [16]⁹⁴ | [32]⁷⁶ | [32]¹⁰¹ | [16]×[32]¹⁰¹ | per-chain Winternitz |
| z_b | 6 | 2 | 5 | 2 | 8 | 4 | 7 | 3 | zero bits forced in OTS digest |
| len | 26 | 40 | 42 | 64 | 94 | 76 | 101 | 102 | WOTS+C chains |
| claimed security | 160 | 160 | 256 | 256 | 384 | 384 | 512 | 512 | classical bits (spec's claim) |

Sizes (bytes). Spec column = Table 1.1; impl column = OBSERVED. `pk = 2n`, `sk = 4n`,
`|σ| = n + n_ctr^FTS + k(a+1)n + d·n_ctr^OTS + (d·len + h)n` (Alg. 18 line 1).

| instance | pk spec | pk impl | sk spec | sk impl | sig spec | sig impl | match |
|---|---|---|---|---|---|---|---|
| CEDRUSC-160s | 40 | 40 | 80 | 80 | 9460 | 9460 | yes |
| CEDRUSC-160f | 40 | 40 | 80 | 80 | 19812 | 19812 | yes |
| CEDRUSC-256s | 64 | 64 | 128 | 128 | 24104 | 24104 | yes |
| CEDRUSC-256f | 64 | 64 | 128 | 128 | 43548 | 43548 | yes |
| CEDRUSC-384s | 96 | 96 | 192 | 192 | 61572 | 61572 | yes |
| CEDRUSC-384f | 96 | 96 | 192 | 192 | 75988 | 75988 | yes |
| CEDRUSC-512s | 128 | 128 | 256 | 256 | 98212 | 98212 | yes |
| CEDRUSC-512f | 128 | 128 | 256 | 256 | 121520 | 121520 | yes |

All 8 sets: spec sizes reproduce OBSERVED exactly. No size mismatch.

## Pseudocode

### KeyGen (Alg. 16)
```
ADRS ← toByte(0,32); ADRS.setLayerAddress(d-1)
PK.root ← xmssNode(SK.seed, 0, h_{d-1}, PK.seed, ADRS)     // Alg. 7, recursive Merkle
return SK = (SK.seed, SK.prf, PK.seed, PK.root), PK = (PK.seed, PK.root)
// leaf of an XMSS tree = pkGenOTS (Alg. 4): for i<len, sk←PRF(PK.seed,SK.seed,skADRS[i]),
//   tmp[i] ← chain(sk, 0, w_i-1, ...); pk ← T_len(PK.seed, otspkADRS, tmp)
```

### Sign (Alg. 17; sub-algorithms 14, 10, 8, 5, 3)
```
optRand ← addrnd (hedged) or PK.seed (deterministic)
R ← PRF_MSG(SK.prf, optRand, msg)
ctrFTS ← 0
repeat  ctrFTS++ ; digest ← H_MSG(R, PK.seed, PK.root, msg, ctrFTS)
until   trailingBits(digest[0 : ⌈a'/8⌉], a') = 0^{a'}          // Alg. 17 l.5-10
md      ← digest[⌈a'/8⌉ .. +⌈k·a/8⌉]                            // FORS message
idxTree ← toInt(next ⌈(h-h_0)/8⌉ bytes) mod 2^{h-h_0}
idxLeaf ← toInt(next ⌈h_0/8⌉  bytes) mod 2^{h_0}
ADRS ← (layer 0, tree idxTree, type FORS_TREE, keypair idxLeaf)
σ_FORS ← signFORS(ctrFTS, md, SK.seed, PK.seed, ADRS)          // Alg. 14
   indices ← base2b(md, a, k)
   for i<k: σ ‖= skGenFORS(.., i·2^a + indices[i])              // Alg. 12 = PRF
            for j<a: Auth[j] ← nodeFORS(.., i·2^{a-j} + (⌊indices[i]/2^j⌋⊕1), j, ..)
pkFORS ← pkFromSigFORS(σ_FORS, md, PK.seed, ADRS)              // Alg. 15
σ_HT   ← signHT(pkFORS, SK.seed, PK.seed, idxTree, idxLeaf)    // Alg. 10
   root ← pkFORS
   for j = 0..d-1:  σ_XMSS ← SignXMSS(root, SK.seed, idxLeaf, PK.seed, ADRS@layer j)
                        Auth[t] ← xmssNode(SK.seed, ⌊idxLeaf/2^t⌋⊕1, t, ..), t < h_j
                        σ_OTS ← SignOTS(root, ..)              // Alg. 5, see below
                    root ← pkFromSigXMSS(...); idxLeaf ← idxTree mod 2^{h_{j+1}};
                    idxTree ← idxTree ≫ h_{j+1}
σ ← R ‖ ctrFTS ‖ σ_FORS ‖ σ_HT                                 // Fig. 1.14
```
SignOTS (Alg. 5) — the WOTS+C counter search that replaces the classical checksum:
```
digest ← H_Root(PK.seed, rootHashAddr, n); s ← ⌊Σ_j (w_j-1)/2⌋
while leadingBits(digest, z_b) ≠ 0^{z_b}
      or Σ IntSeq_w(trailingBits(digest, 8n-z_b)) ≠ s:
    rootHashAddr.counterIncrement(); digest ← H_Root(PK.seed, rootHashAddr, n)
(s_0..s_{len-1}) ← IntSeq_w(trailingBits(digest, 8n-z_b))
for i<len: sig[i] ← chain(PRF(PK.seed,SK.seed,skADRS[i]), 0, s_i, PK.seed, ADRS)
return ctr ‖ sig                                               // Fig. 1.10
```

### Verify (Alg. 18)
```
if |SIG| ≠ n + 4 + k(a+1)n + d(4 + len·n) + h·n: return false
R, ctr, SIG_FORS, SIG_HT ← parse(SIG)
digest ← H_MSG(R, PK.seed, PK.root, msg, ctr)
if trailingBits(digest[0:⌈a'/8⌉], a') ≠ 0^{a'}: return false    // recheck the counter
md, idxTree, idxLeaf ← split(digest)   as in Sign
pkFORS ← pkFromSigFORS(SIG_FORS, md, PK.seed, ADRS)            // Alg. 15
return verifyHT(pkFORS, SIG_HT, PK.seed, idxTree, idxLeaf, PK.root)   // Alg. 11
   // d× pkFromSigXMSS (Alg. 9): pkFromSigOTS (Alg. 6) then hash up the auth path;
   // each layer first checks its 4-byte ctr_OTS is "valid" (spec does not define this)
```

### Hash / PRF instantiation (Tables 1.2, 1.3)
```
n ≤ 32 (160, 256):  F/H/T_l/H_Root/PRF(PK.seed,ADRS,M) = Trunc_n(SM3(PK.seed ‖ toByte(0,64-n)
                                                        ‖ ADRS_c ‖ M))   // ADRS_c = 22 bytes
                    PRF_MSG = Trunc_n(SM3(SK.prf ‖ optRand ‖ M))
n > 32  (384, 512): same functions = XOF_SM3(PK.seed ‖ ADRS ‖ M, 8n)   // full 32-byte ADRS
all sets:           H_MSG = XOF_SM3(R ‖ PK.seed ‖ PK.root ‖ M ‖ ctr, 8m)
                    m = ⌈a'/8⌉ + ⌈k·a/8⌉ + ⌈(h-h_0)/8⌉ + ⌈h_0/8⌉
```
8 address types (§1.5): OTS_HASH 0, OTS_PK 1, XMSS_TREE 2, FORS_TREE 3, FORS_ROOT 4,
OTS_PRF 5, FTS_PRF 6, ROOT_HASH 7 (the last carries the WOTS+C counter).

## Implementation vs specification

Checked `src/CEDRUSC-160s` line by line against Chapter 1, and diffed the parameter headers
and `hash_sm3.c` of all 8 instances. `sign.c` = Alg. 16/17/18, `merkle.c`+`utilsx1.c` =
Alg. 7/8/9, `wots.c`+`wotsx1.c` = Alg. 3/4/5/6, `fors.c` = Alg. 12–15, `hash_sm3.c` =
H_MSG/PRF_MSG, `thash_sm3_simple.c` = F/H/T_l/H_Root, `address.c`/`address.h` = §1.5.

Agreements. All 8 `params/params-cedrusc-sm3-*.h` reproduce Table 1.1; spot-checked
`SPX_N`/`SPX_FULL_HEIGHT`/`SPX_D`/`SPX_FORS_HEIGHT`/`SPX_FORS_TREES`/`SPX_WOTS_LEN`/
`WOTS_ZERO_BITS`/`SPX_FORS_ZERO_LAST_BITS`/`SPX_WOTS_W_ARRAY` for 160s, 160f, 256s, 384s,
512f — all match. `WANTED_CHECKSUM` equals `⌊Σ(w_j-1)/2⌋` in every case I recomputed
(787, 292, 1307, 705, 1573), and `csum = Σ(w_i-1-x_i) = WANTED_CHECKSUM` is equivalent to the
spec's `x ∈ C_s(Π[w_j])`. The per-layer heights in `sign.c:146-152` implement §2.3's
`⌈h/d⌉`/`⌊h/d⌋` split exactly. Key/signature formats, the 8 address types
(`address.h:7-14`), and `SPX_DGST_BYTES` = spec `m` all agree.

- **(a) Real deviation, serious — the bottom-layer tree index is hard-wired to zero.**
  `hash_sm3.c:172` is `*tree = 0;` (identical in all 8 instances, all at line 172). The
  `⌈(h-h_0)/8⌉`-byte `idxTree` field of the digest is skipped, not decoded (Alg. 17 l.11/13,
  Alg. 18 l.13/15 require `idxTree ← toInt(...) mod 2^{h-h_0}`). Signer and verifier share the
  routine, so KATs pass, but only the single bottom-layer XMSS tree #0 is ever reachable.
  In addition `SPX_BOTTOM_TREE_HEIGHT` is defined without parentheses as
  `SPX_TREE_HEIGHT + 1` (`params/params-cedrusc-sm3-*.h:37`), so the leaf mask shift
  `32 - SPX_LEAF_BITS` (`hash_sm3.c:78,176`) expands to `32 - SPX_TREE_HEIGHT + 1` and keeps
  `h_0 - 2` bits. The number of distinct FORS+C key pairs therefore drops from `2^h`
  (2^67…2^64) to `2^{h_0-2}`: 4/64/8/64/16/128/32/128 for
  160f/160s/256f/256s/384f/384s/512f/512s. FORS few-time security
  collapses; it also makes layers 1..d-1 of the hypertree dead weight. Recorded as
  sign-03-1 in `report.md`.
- **(a) Real deviation — the Table 1.2 instantiation for n ≤ 32 is not implemented.**
  `thash_sm3_simple.c:16-31` hashes `PK.seed(n) ‖ ADRS(32) ‖ M` for every n, i.e. the
  `toByte(0, 64-n)` one-block padding and the 22-byte compressed `ADRS_c` of Table 1.2 are
  absent (no `ADRS_c` construction exists anywhere in the tree; `sm3_offsets.h` only names
  field offsets inside the full 32-byte address). 160/256 therefore use the Table 1.3 input
  format with SM3-256 truncated to n. A spec-faithful implementation would not interoperate.
  §1.11's stated motivation for the padding (Merkle–Damgård state reuse) is likewise unrealised.
- **(a) Deviation, encoding — the 4-byte `ctr_OTS` sits at the end of each layer block.**
  `counters.c:14-28` writes/reads it at offset `SPX_WOTS_BYTES + h_i·n`, i.e. after the WOTS
  chains *and* after the authentication path; Fig. 1.10/1.11 put `ctr_OTS` first, before the
  chains. Total length is unchanged, so the size table still matches. The FORS counter, by
  contrast, is at offset `n` (`counters.c:11,37`), matching Fig. 1.14.
- **(a) Deviation — the WOTS+C zero-bit window is in the wrong place.** Alg. 5 l.6 requires
  `leadingBits(digest, z_b) = 0` with the base-w digits taken from the *trailing* `8n-z_b`
  bits. `wots.c:116,138` test `digest[n-1] & (0xFF << (8-z_b))` while `base_w` (`wots.c:49-64`)
  consumes the *leading* `Σ log2 w_j = 8n-z_b` bits. For 160s that constrains bits 152..157,
  two of which are already inside the last base-w digit, and leaves bits 158..159 both unused
  and unchecked. Self-consistent between signer and verifier (KATs pass) but the spec text
  does not describe the deployed encoding. The reference and optimized trees use the same
  mask in all sets; the low bits of the last byte that are neither encoded nor tested number
  2/2/2/3/4/0/3/1 for 160f/160s/256f/256s/384f/384s/512f/512s (`min(z_b, 8-z_b)`), so only
  384s is exact (sign-03-3).
- **(b) Spec inconsistency — `ctrFTS` counted twice.** Alg. 14 l.1 sets `σ_FORS ← ctrFTS`,
  yet Alg. 17 l.22 emits `R ‖ ctrFTS ‖ σ_FORS ‖ σ_HT`. Fig. 1.14 and the length test in
  Alg. 18 l.1 count it once. The implementation counts it once (`sign.c:134-135`,
  `fors.c` writes no counter), which is the self-consistent reading.
- **(b) Spec typos.** Alg. 6 l.10 writes `chain(sig[i], s_i, w-1-s_i, ..)` with a scalar `w`
  in an inhomogeneous scheme (`wots.c:146` correctly uses `w_i`); Alg. 9 l.16 has a misplaced
  parenthesis; Alg. 15 l.19 says `FORS_ROOTS` where §1.5 defines `FORS_ROOT`; Alg. 18 l.19
  says `PORS_TREE`/`pk_PORS` (leftover from a predecessor scheme). None affect the code.
- **(c) Equivalent.** Alg. 11 l.5/13 "if `ctr_OTS` is valid" is undefined in the spec; the code
  rejects `ctr == 0` (`sign.c:249-251`) and, inside `wots_pk_from_sig` (`wots.c:138-141`),
  zeroes the recomputed WOTS public key when the checksum or zero-bit test fails, which makes
  the root comparison fail — equivalent to returning false. `merkle_gen_root` passes
  `SPX_TREE_HEIGHT` for the top layer, which is correct because the `⌈h/d⌉` layers are the
  bottom `κ ≤ d-1` ones, so the `auth_path` buffer at `merkle.c:98` is correctly sized.

Not verified: the SM3/XOF primitives in `auxfunc.c` were not checked against the NICCS
API reference; `treehashx1`/`wotsx1` were read for structure but their index arithmetic was
not exhaustively re-derived; I did not execute anything, so the `*tree = 0` consequence is a
source-level reading, not a demonstrated forgery.
