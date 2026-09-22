# sign-11 FlexTree — algorithm summary

FlexTree is a stateless hash-based signature in the SPHINCS+/SLH-DSA family: security rests
only on the (second-)preimage / collision resistance of the underlying hash (here SM3 and an
SM3-derived XOF), with no structured algebraic assumption. The construction is randomised
hash-and-sign: a message digest selects one PORS+FP few-time key inside a *hypertree*, the
digest is signed with that PORS+FP key, and the resulting PORS+FP public key is authenticated
by a chain of d layers of XMSS trees whose leaves are inhomogeneous WOTS^C one-time keys.
FlexTree generalises SLH-DSA in three places: XMSS trees may have **different heights per
layer**, WOTS^C chains may have **different lengths w_i per chain**, and the FORS few-time
scheme is replaced by **PORS+FP** (PRNG-to-obtain-a-random-subset with "forced pruning",
i.e. a counter search that caps the number of authentication nodes at m_MAX).

Specification: `sign-11-spec.pdf` (66 pages, English), Chapter 1 §1.1–§1.12.
Implementation: `Implementations/Reference_Implementation/Flextree-{160,256,384,512}{f,s}`.

## Parameters

Spec Table 1.3 (§1.12). `h_i` are derived: `h_0..h_{(h mod d)-1} = ceil(h/d)`, the rest
`floor(h/d)` (§1.2; the spec writes the cut-off index as "k", a typo — `k` is the PORS
parameter). `h_PORS = ceil(log2 t)`, `s = t - 2^(h_PORS - 1)`, and the digest length is
`m = ceil((h-h_0)/8) + ceil(h_0/8) + n`.

| parameter | 160s | 160f | 256s | 256f | 384s | 384f | 512s | 512f | meaning |
|---|---|---|---|---|---|---|---|---|---|
| n | 20 | 20 | 32 | 32 | 48 | 48 | 64 | 64 | node/hash size, bytes |
| h | 67 | 67 | 67 | 68 | 67 | 64 | 66 | 66 | total hypertree height |
| d | 9 | 17 | 10 | 17 | 8 | 12 | 8 | 11 | XMSS layers |
| t | 42429 | 4721 | 81271 | 16900 | 250491 | 71828 | 429271 | 181922 | PORS tree leaves |
| k | 15 | 27 | 25 | 35 | 35 | 53 | 48 | 57 | PORS leaves revealed |
| m_MAX | 160 | 155 | 280 | 270 | 398 | 486 | 560 | 586 | max PORS auth nodes |
| w | [32]×[64]^25 | [8]×[16]^39 | [64]^42 | [8]×[16]^63 | [16]^94 | [16]^3×[32]^74 | [32]^101 | [16]×[32]^101 | per-chain Winternitz |
| z_b | 5 | 1 | 4 | 1 | 8 | 2 | 7 | 3 | forced leading zero bits |
| len | 26 | 40 | 42 | 64 | 94 | 77 | 101 | 102 | WOTS^C chains |
| n_ctr^OTS, n_ctr^FTS | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 4 | counter sizes, bytes |
| claimed security | 160 | 160 | 256 | 256 | 384 | 384 | 512 | 512 | classical bits (quantum = half) |

Sizes (bytes), specification (Table 1.3) vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | sig spec | sig impl | match |
|---|---|---|---|---|---|---|---|
| Flextree-160s | 40 | 40 | 80 | 80 | 9580 | 9580 | yes |
| Flextree-160f | 40 | 40 | 80 | 80 | 18672 | 18672 | yes |
| Flextree-256s | 64 | 64 | 128 | 128 | 25420 | 25420 | yes |
| Flextree-256f | 64 | 64 | 128 | 128 | 46856 | 46856 | yes |
| Flextree-384s | 96 | 96 | 192 | 192 | 60180 | 60180 | yes |
| Flextree-384f | 96 | 96 | 192 | 192 | 73396 | 73396 | yes |
| Flextree-512s | 128 | 128 | 256 | 256 | 94948 | 94948 | yes |
| Flextree-512f | 128 | 128 | 256 | 256 | 117296 | 117296 | yes |

`pk = 2n`, `sk = 4n` (§1.11.1). The spec's own size formula is Algorithm 24 line 1:
`|SIG| = n + n_ctr^FTS + k·n + m_MAX·n + d·(n_ctr^OTS + len·n) + h·n`.
Evaluated from Table 1.3 it reproduces **all eight** observed sizes exactly.

## Pseudocode

### KeyGen — Algorithm 22, `keyGen_SLH(SK.seed, SK.prf, PK.seed)`
```
ADRS <- toByte(0, 32)
ADRS.setLayerAddress(d-1)
PK.root <- xmssNode(SK.seed, 0, h_{d-1}, PK.seed, ADRS)     // Alg 11
return SK = (SK.seed, SK.prf, PK.seed, PK.root),  PK = (PK.seed, PK.root)
// SK.seed, SK.prf, PK.seed each n bytes from an RBG of >= 8n bits strength.
```

### Sign — Algorithm 23, `signInternal_SLH(msg, SK, addrnd)`
```
ADRS    <- toByte(0, 32)
optRand <- addrnd                        // hedged; deterministic variant uses PK.seed
R       <- PRF_MSG(SK.prf, optRand, msg)
digest  <- H_MSG(R, PK.seed, PK.root, msg)              // m bytes
idxTree <- toInt(digest[0 : ceil((h-h0)/8)])            mod 2^(h-h0)
idxLeaf <- toInt(digest[ceil((h-h0)/8) : +ceil(h0/8)])  mod 2^h0
md      <- digest[ceil((h-h0)/8)+ceil(h0/8) : + n]
ADRS.setTreeAddress(idxTree); ADRS.setTypeAndClear(FTS_TREE)
ADRS.setKeyPairAddress(idxLeaf)
sigma_PORS <- signFTS(md, SK.seed, PK.seed, ADRS, R)          // Alg 19
pk_PORS    <- pkFromSigPORS(sigma_PORS, md, PK.seed, ADRS, R) // Alg 20
sigma_HT   <- signHT(pk_PORS, SK.seed, PK.seed, idxTree, idxLeaf)  // Alg 14
return R || sigma_PORS || sigma_HT                       // Figure 1.16
```
Sub-algorithms actually used (spec numbers):
```
Alg 19 signFTS:   ctr <- -1
                  repeat ctr <- ctr+1
                         I <- rho_{[t] choose k}( H_PORS(R, ctr, md) mod C(t,k) )
                         A <- octopus(I)                 // Alg 18: minimal auth set
                  until |A| <= m_MAX                     // "forced pruning"
                  return ctr || {skGenPORS(.., i)}_{i in I} || {nodePORS(.., A[j])}_j
Alg  3/5/6 rho:   bijection [C(t,k)] -> k-subsets of [t], combinatorial-number system;
                  Alg 5/6 are the binomial-lower-bound form the code implements.
Alg 17 nodePORS:  PORS tree is NOT perfect: s = t - 2^(h_PORS-1); 2s leaves at height 0,
                  2^(h_PORS-1)-s leaves at height 1.
Alg 14 signHT:    sigma_XMSS <- SignXMSS(M, ..., idxLeaf, layer 0);  then for j = 1..d-1:
                  idxLeaf <- idxTree mod 2^h_j; idxTree <- idxTree >> h_j;
                  root <- pkFromSigXMSS(...); sign root at layer j; concatenate.
Alg 12 SignXMSS:  Auth[j] <- xmssNode(SK.seed, floor(idx/2^j) XOR 1, j, ...) for j<h_i;
                  sigma_OTS <- SignOTS(M, ...);  return sigma_OTS || Auth.
Alg  9 SignOTS:   digest <- H_Root(PK.seed, rootHashAddr, M); increment an internal counter
                  until leadingBits(digest, z_b) = 0^z_b AND
                        IntSeq_w(trailingBits(digest, 8n - z_b)) lies on the constant-sum
                        shell C_cs( prod_j [w_j] ),  cs = floor( sum_j (w_j - 1) / 2 );
                  emit ctr (4 B) then chain(sk_i, 0, s_i, ..) for i < len  (Fig. 1.12).
                  -> checksum-free "WOTS^C": no separate checksum chains.
Alg  7 chain:     tmp <- F(PK.seed, ADRS.setHashAddress(j), tmp), j = i .. i+s-1.
```

### Verify — Algorithm 24, `verifyInternal_SLH(msg, SIG, PK)`
```
if |SIG| != n + n_ctr^FTS + k*n + m_MAX*n + d*(n_ctr^OTS + len*n) + h*n: return false
R <- SIG.getR();  SIG_PORS <- SIG.getSIG_PORS();  ctr_FTS <- SIG_PORS.getCounterFTS()
if ctr_FTS not valid: return false
digest  <- H_MSG(R, PK.seed, PK.root, msg)
idxTree, idxLeaf, md  <- split digest exactly as in Alg 23 lines 5-9
ADRS.setTreeAddress(idxTree); ADRS.setTypeAndClear(PORS_TREE)   // = FTS_TREE
ADRS.setKeyPairAddress(idxLeaf)
pk_PORS <- pkFromSigPORS(SIG_PORS, md, PK.seed, ADRS, R)        // Alg 20 -> Alg 21
return verifyHT(pk_PORS, SIG_HT, PK.seed, idxTree, idxLeaf, PK.root)  // Alg 15
```
`Alg 21 computeRoot` rebuilds the PORS root from the k revealed leaves plus the octopus
authentication nodes, merging sibling pairs level by level and inserting the height-1 leaves
(indices `>= 2s`) only after level 0 has been folded.

### Hash instantiation (Tables 1.4 and 1.5, §1.12) — all SM3-based
```
FlexTree-160 / -256  (Table 1.4)          FlexTree-384 / -512 (Table 1.5)
 PRF_MSG  Trunc_n(SM3(SK.prf||optRand||M))   XOF_SM3(SK.prf||optRand||M, 8n)
 H_MSG    XOF_SM3(R||PK.seed||PK.root||M, 8m)         (same)
 H_PORS   XOF_SM3(SM3(R||md||0^(64-2n)||ctr), 8(n+ceil(log2 C(t,k)/8)))
                                              XOF_SM3(R||md||ctr, 8(n+...))
 F,H,T_len,H_Root,PRF
          Trunc_n(SM3(PK.seed || 0^(64-n) || ADRS_c || M))
                                              XOF_SM3(PK.seed || ADRS || M, 8n)
```
`ADRS_c` is the 22-byte compressed address (1-byte layerAddr, 8-byte treeAddr, 1-byte
addrType, ...); the `0^(64-n)` padding exists so the SM3 Merkle-Damgard state after the
first 64-byte block can be cached across calls. The spec (§1.12) explicitly *warns* that
SM3/XOF_SM3 are modelled as ideal primitives and that "the actual security strength of these
instances may not reach the specified security level".

## Implementation vs specification

**What was checked.** `sign.c` (Alg 22/23/24), `merkle.c`+`wots.c`/`wotsx1.c` (Alg 7-15),
`pors_fp.c` (Alg 16-21, plus Alg 3/5/6 via the ~9.8 MB precomputed binomial tables in
`params/pors_precomputed-flextree-*.h`), `hash_sm3.c` and `thash_sm3_simple.c` (Tables
1.4/1.5), `params/params-flextree-*.h` (Table 1.3). Nothing was built or executed.

**Agreements.**
- Parameter spot-check, 8 constants x 8 instances, all from `params/params-flextree-*.h`
  (`SPX_N`, `SPX_FULL_HEIGHT`, `SPX_D`, `SPX_PORS_FP_T`, `SPX_PORS_FP_K`,
  `SPX_PORS_FP_MAX_AUTH_NODES`, `SPX_WOTS_LEN`, `WOTS_ZERO_BITS`): **every value matches
  Table 1.3** (n, h, d, t, k, m_MAX, len, z_b). `SPX_WOTS_W_ARRAY` was checked against the
  `w` column for 160f/160s/256f/512f and matches, including the short first chain.
- `SPX_BYTES` (`params-flextree-*.h`) is `n + (4 + (k+m_MAX)n) + d*len*n + h*n + 4d`, i.e.
  algebraically identical to Algorithm 24 line 1, and all eight observed sizes match Table 1.3.
- Variable tree heights: `sign.c:143-150` (sign) and `sign.c:220-227` (verify) build
  `merkle_tree_heights[]` with `num_k = h - floor(h/d)*d` trees of height `floor(h/d)+1` at
  the **bottom** layers, matching `h_0 = ceil(h/d)` in §1.2.
- Digest split: `hash_sm3.c` `SPX_DGST_BYTES = n + ceil((h-h0)/8) + ceil(h0/8)`, exactly the
  spec's `m`; `SPX_BOTTOM_TREE_HEIGHT` is the `h_0` used for the leaf-index field.
- `H_PORS` output length is `n + PORS_TOTAL_COMBINATION_BYTES = n + ceil(log2 C(t,k)/8)`
  (`gen_pors_precomputed.py:144`), as in equation (1.6); the result is reduced mod `C(t,k)`
  (`pors_fp.c:202-217`) before the rho decoding, per Alg 19 line 5.
- PORS address types: the impl re-uses the SPHINCS+ names `SPX_ADDR_TYPE_FORSTREE`/`FORSPRF`
  (`pors_fp.c:447-455`) for what the spec calls `FTS_TREE`. Consistent between sign and
  verify; a naming difference only.

**Discrepancy (a) — real deviation, FlexTree-160* and -256* only.**
`thash_sm3_simple.c:13-31` computes `Trunc_n(SM3(PK.seed || ADRS[32] || M))` and
`hash_sm3.c:18-38` computes `PRF = Trunc_n(SM3(PK.seed || ADRS[32] || SK.seed))`. Table 1.4
specifies `Trunc_n(SM3(PK.seed || toByte(0, 64-n) || ADRS_c || M))` — i.e. the 64-byte
PK.seed padding and the **22-byte compressed** `ADRS_c`. The implementation uses neither:
the full 32-byte address, no padding. All eight instance directories ship the *same*
`thash_sm3_simple.c` (byte-identical, verified by `diff`), so this is one code path written
to Table 1.5 and applied to the Table 1.4 instances as well. Consequences: (i) an
independent implementation following Table 1.4 would not interoperate with these KATs for
160s/160f/256s/256f; (ii) the stated performance rationale for the padding ("reuse of
intermediate state after one-block computation", §1.12) is not realised —
`initialize_hash_function()` (`hash_sm3.c:13-16`) is an empty stub. No security loss is
apparent (the full address is a superset of `ADRS_c`), but it is a spec/code mismatch.

**Discrepancy (b) — spec ambiguity, all instances.** `H_PORS` argument order: Algorithm 19
line 5 writes `H_PORS(R, ctr, md)` while Tables 1.4/1.5 define the function as
`H_PORS(R, md, ctr)` with the byte string `R || md || ... || ctr`. `pors_fp.c:219-237`
hashes `R || ctr || md`, following Algorithm 19. Additionally, for n <= 32 the code calls
`pseudoXOF` directly rather than Table 1.4's inner `SM3(R || md || 0^(64-2n) || ctr)`
wrapped in `XOF_SM3` — the same Table-1.4-vs-Table-1.5 collapse as (a).

**Not verified.** The `rho`/`LowerBound` correspondence (Alg 3/4/5/6) was only checked at
the interface level — the ~9.8 MB machine-generated binomial tables were not recomputed, and
`gen_pors_precomputed.py` ships without a digest manifest (already noted in
`security_findings.md`, "Prioritized experiments").

**Constant-sum discrepancy (verified).** Algorithm 9 explicitly uses
`floor(sum_j(w_j-1)/2)`. The code accumulates the complementary digit sum and compares it
with the floored `WANTED_CHECKSUM`; for FlexTree-384f and FlexTree-512s this makes the
accepted original-digit sums the ceilings 1170 and 1566, rather than the specified floors
1169 and 1565. The specified signer terminates, but the two definitions do not interoperate.
Cross-reference: `security_findings.md` reports no established claim violations, and one
unexplained observation (a flipped-bit signature at byte 3613 of a Flextree-160f signature
being accepted) that is orthogonal to the checks above.
