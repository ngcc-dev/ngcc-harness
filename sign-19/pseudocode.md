# sign-19 Phoenix — algorithm summary

Stateless hash-based signature in the SPHINCS+/SLH-DSA framework (security rests
only on the PRF/ITSR/SM-TCR/SM-DSPR properties of the underlying hash). Phoenix
keeps the standard XMSS + hypertree authentication layers but replaces WOTS+ by
**GWOTS+C** (Winternitz with a mixed-width digit profile and a *checksum-offset
window* `[L,U]` found by counter search) and FORS by **TFORS** (transposed FORS
whose Merkle overlap is pruned by an Octopus algorithm, giving a *variable-length*
few-time signature).

Specification: `sign-19-spec.pdf` (51 pages) — §2.3 (hash/PRF interface), §3
(GWOTS+C, Alg. 1–3), §4 (TFORS/Octopus), §5 (XMSS/hypertree, Alg. 14–16),
§6 (Phoenix, Alg. 17–19), §8 (parameters, Tables 3–4).

## Parameters

Spec §8.1 notation; values from spec Table 4 (SM3 sets; Table 3 states the
GWOTS+C profiles are shared by `SHAKE/SM3`). Both hash backends use the same
parameter numbers, so one table covers all 20 instances.

| parameter | 128s | 128f | 192s | 192f | 256s | 256f | 384s | 384f | 512s | 512f | meaning |
|---|---|---|---|---|---|---|---|---|---|---|---|
| n | 16 | 16 | 24 | 24 | 32 | 32 | 48 | 48 | 64 | 64 | hash/seed length (bytes) |
| h | 70 | 68 | 63 | 68 | 66 | 64 | 66 | 68 | 64 | 68 | total hypertree height |
| d | 10 | 17 | 9 | 17 | 11 | 16 | 11 | 17 | 8 | 17 | hypertree layers (h' = h/d) |
| a (t = 2^a) | 9 | 7 | 12 | 8 | 12 | 8 | 12 | 11 | 12 | 11 | TFORS set size exponent |
| k | 14 | 19 | 19 | 29 | 24 | 47 | 37 | 39 | 57 | 58 | TFORS sets selected |
| k' | 16 | 32 | 32 | 32 | 32 | 64 | 64 | 64 | 64 | 64 | TFORS sets available |
| m (msg chains) | 20 | 37 | 29 | 57 | 38 | 59 | 56 | 75 | 101 | 86 | GWOTS+C, Table 3 |
| m + d_off | 21 | 38 | 30 | 58 | 39 | 60 | 57 | 76 | 102 | 87 | incl. 1 offset chain |
| [L,U] | [870,901] | [189,204] | [1473,1504] | [275,290] | [2077,2108] | [594,609] | [3284,3315] | [1298,1313] | [1661,1692] | [2637,2652] | checksum window |
| claimed security | 128 | 128 | 192 | 192 | 256 | 256 | 384 | 384 | 512 | 512 | bits, classical (Table 9: quantum = half) |

Sizes (bytes). Spec: `pk = 2n`, `sk = 4n` (§6.1); signature size from Table 4
(same column for SHAKE and SM3). Impl: `OBSERVED/sign-19.txt`. Identical for the
`-SHAKE-` and `-SM3-` instance pairs at every level, so one row per level.

| level | pk spec | pk impl | sk spec | sk impl | sig spec | sig impl | match |
|---|---|---|---|---|---|---|---|
| 128s | 32 | 32 | 64 | 64 | 6258 | 6258 | yes |
| 128f | 32 | 32 | 64 | 64 | 13670 | 13670 | yes |
| 192s | 48 | 48 | 96 | 96 | 13356 | 13332 | **NO (-24)** |
| 192f | 48 | 48 | 96 | 96 | 30766 | 30766 | yes |
| 256s | 64 | 64 | 128 | 128 | 24618 | 24618 | yes |
| 256f | 64 | 64 | 128 | 128 | 44906 | 44906 | yes |
| 384s | 96 | 96 | 192 | 192 | 54726 | 54726 | yes |
| 384f | 96 | 96 | 192 | 192 | 88442 | 88442 | yes |
| 512s | 128 | 128 | 256 | 256 | 98476 | 98476 | yes |
| 512f | 128 | 128 | 256 | 256 | 138454 | 138454 | yes |

## Pseudocode

### KeyGen (spec Alg. 17, §6.1)
```
SK.seed, SK.prf, PK.seed  <-R  B^n each
addr <- toByte(0,32); addr.setLayerAddress(d-1)
PK.root <- XMSS_node(SK.seed, 0, h', PK.seed, addr)      # root of top XMSS tree
SK <- (SK.seed, SK.prf, PK.seed, PK.root);  PK <- (PK.seed, PK.root)
```

### Sign (spec Alg. 18, §6.2)
```
opt_rand <- addrnd            # hedged: n random bytes; deterministic: PK.seed
R <- PRF_msg(SK.prf, opt_rand, M)
ctr <- 0
loop:                                          # TFORS length grinding
  (md, idx_tree, idx_leaf) <- H_msg(R, PK.seed, PK.root, M || ctr)
  indices    <- message_to_indices(md, PK.seed)            # via H2 -> I_{k,k'}
  authCount  <- |Octopus_Auth_Indices(indices, h_TFORS)|
  siglen_TF  <- (k + authCount) * n
  if siglen_TF <= TFORS_SIG_MAX: break else ctr <- ctr+1
SIG <- R || toByte(ctr,4) || toByte(siglen_TF,2)
addr.setTreeAddress(idx_tree); addr.setKeyPairAddress(idx_leaf)
(SIG_TFORS, PK_TFORS) <- TFORS_sign(indices, SK.seed, PK.seed, addr)
SIG <- SIG || SIG_TFORS || HT_sign(PK_TFORS, SK.seed, PK.seed, idx_tree, idx_leaf)
```
Inner GWOTS+C sign (Alg. 2, §3.3), once per hypertree layer: search a 4-byte
counter until the digest's mixed-width digits `a_i` give a checksum
`C(a) = sum_i (2^{s_i}-1-a_i)` with `L <= C(a) <= U`; transmit the `m` message
chains plus a mixed-radix encoding of the offset `C(a)-L` in `d_off` chains.

### Verify (spec Alg. 19, §6.3)
```
parse SIG as (R, ctr, siglen_TF, SIG_TFORS, SIG_HT)
(md, idx_tree, idx_leaf) <- H_msg(R, PK.seed, PK.root, M || ctr)
indices   <- message_to_indices(md, PK.seed)
authCount <- |Octopus_Auth_Indices(indices, h_TFORS)|
if siglen_TF != (k + authCount) * n: return REJECT          # length check
PK_TFORS  <- TFORS_pkFromSig(PK.seed, addr, md, SIG_TFORS)
return HT_verify(PK_TFORS, SIG_HT, PK.seed, idx_tree, idx_leaf, PK.root)
```
`HT_verify` (Alg. 16) walks j = 0..d-1, recomputing `node <-
XMSS_pkFromSig(idx_leaf, SIG_XMSS[j], node, PK.seed, otsAddr)` and shifting
`idx_leaf <- idx_tree mod 2^{h'}`, `idx_tree <- floor(idx_tree / 2^{h'})`;
ACCEPT iff `node = PK.root`.

### Hash / PRF interface (spec §2.3)
```
PRF_msg(SK.prf, opt_rand, M) : B^n x B^n x B* -> B^n
H_msg(R, PK.seed, PK.root, M) : ... -> B^{l_msg},
      l_msg = L_TFORS + ceil((h-h')/8) + ceil(h'/8),  L_TFORS = max(n, ceil(k*a/8))
PRF(K1, K2, addr) : B^n x B^n x B^32 -> B^n
T_l(PK.seed, addr, M_l) : B^n x B^32 x B^{l n} -> B^n      # H = T_2, F = T_1
H2(PK.seed, M) -> I_{k,k'} = { distinct k-tuples over {0..k'-1} }
addr: 32 bytes, eight big-endian 32-bit words, SPHINCS+ layout extended with
      GWOTS_CHAIN / GWOTSPK / HASHTREE / TFORSTREE / GWOTSPRF / TFORSPRF /
      COMPRESS_GWOTS type constants (Appendix A); setTypeAndClear zeroes the
      last 12 bytes.
```
**The specification never gives the concrete instantiation of these seven
functions.** §2.3 ends with "The concrete instantiations of these functions
depend on the parameter set and are specified together with the full Phoenix
scheme in Section 6" — but §6 (Alg. 17–19) contains no instantiation, and §8
gives none either. The only mentions of a primitive are §1.3 ("can be
instantiated using the same symmetric primitives (SHA-256, SHAKE256, etc.)"),
the instance names, and the build commands in §9.1. So there is **no spec text
that pins SHAKE256 padding/domain separation or the SM3 mode of operation**.

## Implementation vs specification

What was checked. Build recipe `sign-19/Makefile` (20 instances, one directory
each, `-DPARAMS=phoenix-<hash>-<level>`). Mapped: `sign.c` → Alg. 17/18/19,
`tfors.c` + `octopus.c` → §4, `gwots.c`/`gwotsx1.c` + `counter.c` → Alg. 1–3,
`merkle.c` + `utils.c` → Alg. 14–16, `address.c` → Appendix A,
`hash_shake.c` / `thash_shake_simple.c` and `hash_sm3.c` / `thash_sm3_simple.c`
→ §2.3, `randombytes.c` → the `addrnd` source.

Agreements. Spot-checked `params/params-phoenix-{sm3,shake}-{128s,192s}.h`
against spec Tables 3–4: `SPX_N`, `SPX_FULL_HEIGHT`, `SPX_D`, `SPX_TFORS_A`,
`SPX_TFORS_K`, `SPX_TFORS_K_PRIME` all agree (128s: 16/70/10/9/14/16; 192s:
24/63/9/12/19/32). The Table 3 profiles agree too: 128s `S = 12x6 + 8x7` matches
`SPX_GWOTS_W1_LEN 12` with `LOGW1 6` and `W2_LEN 8` with `LOGW2 7`, `T = 1x5`
matches `SPX_GWOTS_LEN2 1` / `CHECKSUM_LOGW 5`, and the window `[870,901]` is
reproduced exactly by `GWOTS_SUM_BASE = (12*63+8*127)/2 - 16 = 870` with
`GWOTS_SUM_RANGE = 31` (192s likewise gives `[1473,1504]`). `SPX_PK_BYTES =
2*SPX_N`, `SPX_SK_BYTES = 4*SPX_N` match §6.1. Verify's length check
(Alg. 19 lines 10–12) and the signature layout `R || ctr(4) || len(2) ||
SIG_TFORS || d*(GWOTS || ctr(4)) || h*n` are both present.

**Discrepancy (a), 192s signature size.** Spec Table 4 gives 13,356 bytes for
Phoenix-192s; both `Phoenix-SHAKE-192s` and `Phoenix-SM3-192s` report 13,332.
`SPX_BYTES = n + 4 + 2 + TFORS_SIG_MAX + d*(GWOTS_BYTES+4) + h*n` with
`SPX_TFORS_SIG_MAX 5274` (params-phoenix-sm3-192s.h:54) evaluates to exactly
13,332; the spec's number is one `n`-byte element larger. Every other level
matches Table 4 to the byte, so this is an isolated stale value in the spec (or
a `TFORS_SIG_MAX` changed after the table was written). The spec's Table 5
"Sign size" column disagrees with Table 4 for most SHAKE rows (e.g. 6,252 /
13,668 / 26,624 / 47,464 / 121,961) — those are *measured* lengths of
variable-length signatures, not `CRYPTO_BYTES`, and should not be read as size
claims.

**Discrepancy (b), hash/PRF instantiation is unspecified — context for the known
KAT findings.** Cross-referencing RESULTS.md (not re-investigated here): the ten
`Phoenix-SM3-*` instances MISMATCH their submitted KATs, and the ten
`Phoenix-SHAKE-*` instances only reproduce when the official ICCS `drng.c` is
linked instead of the `drng.c` those directories ship. The spec gives no text to
adjudicate either, because §2.3's promised instantiation section does not exist.
What the code actually does:
- SHAKE backend: `hash_shake.c:38-43` `PRF_msg = SHAKE256(SK.prf || opt_rand ||
  M)`, `:63-68` `H_msg = SHAKE256(R || PK || M)` squeezed to `SPX_DGST_BYTES`,
  with a cached pre-absorbed `PK.seed` state (`:13-14`, `:34-35`, `:51-52`) —
  an optimisation the shipped `MODIFICATION_NOTES.md` §3 documents as a local
  change to the upstream `Love-MaShiro/phoenix` repo, along with "+2 bytes to
  `SPX_BYTES` in all SHAKE parameter headers" for the TFORS length field.
- SM3 backend: `hash_sm3.c:59` `PRF_msg = HMAC-SM3`, `:97` `H_msg` built from
  SM3 + `mgf1_sm3` (`thash_sm3_simple.c:41`). This is a different mode of
  operation from the SHAKE branch, not a drop-in hash swap — nothing in the
  spec sanctions either choice.
- H2 key-set selection: `h2_generate_indices` (`hash_shake.c:89`,
  `hash_sm3.c:184`) expands `md[0..n-1] || PK.seed` to a fixed 512-byte pool,
  reads it as 64 eight-byte samples, and marks `k' − k` distinct rejected
  positions; the `k` unmarked positions are the output. It returns −1 when the
  pool is used up first (`hash_shake.c:130`, `hash_sm3.c:208`), before writing
  any index. `message_to_indices` (`tfors.c:61`) ignores the return value and
  then reads its local `indices1[]`. The exact per-digest probability of that
  return is 2^-27.6 for 384s, 2^-35.3 for 384f, 2^-62.9 for 128f and 192s,
  2^-79.6 for 256f, and below 2^-118 for the other sets (sign-19-7).
- Randomness: Alg. 18's `addrnd` comes from `randombytes()`
  (`sign.c:127,175`), which under `-DSUBMISSION_DRNG` calls
  `get_random_number(&drng_algorithm, ...)` (`randombytes.c:22`) and otherwise
  reads `/dev/urandom` (`randombytes.c:46`). The spec says only "hedged" vs
  "deterministic (opt_rand = PK.seed)" and names no generator.
- The `Phoenix-SHAKE-*/drng.c` files are **not** the official ICCS DRNG: they
  define `SHAKE256_df` / `SHAKE256_DRNG_Instantiate` / `SHAKE256_DRNG_Generate`
  (`src/Phoenix-SHAKE-128s/drng.c:65,102,130`) over `shake256()`, whereas
  `api/drng.c` is SM3-based (`api/drng.c:44,128`). Since the spec mandates no
  DRNG, this is a deviation from the *ICCS API contract*, not from the spec.

Not verified. The TFORS/Octopus pruning internals (§4.2–4.3) and the Appendix A
address byte layout were read only at the level of function names, not
line-by-line against `octopus.c` / `address.c`. No instance was rebuilt or run;
the `-192s` size discrepancy above was derived from the parameter headers
arithmetically, consistent with OBSERVED.
