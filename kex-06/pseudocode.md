# kex-06 MAMBA-NIKE — algorithm summary

A NewHope-style ring key exchange over `Rq = Zq[X]/(X^n+1)` with `q = 2^13`,
in which the RLWE error is not sampled but *produced by public dithered
quantization*: every transmitted ring element is a quantization label, and its
"normal form" reconstruction carries a bounded, input-independent error. The
hardness assumption is Ring Learning With Quantization over Z (RLWQ-Z).
Agreement between the two close ring products is turned into a raw key by
NewHope's D4 reconciliation, and the raw string is bound to the transcript by
SHAKE256. Despite the name, after public-key distribution the protocol needs
**one online message** (`kex_get_passes_num()` returns 1), and the initiator's
own long-term key is not used at all (see the findings below).

Specification: `kex-06-spec.pdf` (34 pages), §2.2 (Algorithms 1-3), §3.1-3.2
(Tables 3-4), §8.2-8.9 (implementation rules), App. A.3 (quantizer and D4),
App. C (correctness). English-language spec.

## Parameters

| parameter | -128 | -192 | -256 | -384 | -512 | meaning |
|---|---|---|---|---|---|---|
| `n` | 1024 | 1024 | 1024 | 2048 | 2048 | ring degree |
| `q` | 2^13 | 2^13 | 2^13 | 2^13 | 2^13 | power-of-two modulus (§1.1.2) |
| `t_pk` | 9 | 10 | 10 | 11 | 11 | public-key compression width |
| `t_u` | 10 | 10 | 10 | 11 | 11 | response compression width |
| `t_v` | 6 | 6 | 6 | 6 | 6 | local reconciliation width (not sent) |
| `Delta_pk = q/2^t_pk` | 16 | 8 | 8 | 4 | 4 | quantization step |
| `Delta_u` | 8 | 8 | 8 | 4 | 4 | |
| `Delta_v` | 128 | 128 | 128 | 128 | 128 | |
| `eta_s, eta_r` | 2,2 | 3,3 | 7,7 | 2,2 | 5,5 | centered binomial |
| `kappa = n/4` | 256 | 256 | 256 | 512 | 512 | D4 blocks = raw key bits |
| helper bytes `n/4` | 256 | 256 | 256 | 512 | 512 | 4 two-bit values per byte |
| `r_D` | 2 | 2 | 2 | 2 | 2 | D4 radius; `T_D4 = floor(3q/4)-2 = 6142` |
| claimed target | 128 | 192 | 256 | 384 | 512 | bits, classical |
| MATZOV classical | 272.1 | 279.3 | 294.3 | 538.6 | 590.2 | Table 3 |
| MATZOV quantum | 251.1 | 258.4 | 273.9 | 512.5 | 512.5 | Table 3 |
| key-rejection prob. | 2^-42.4 | 2^-40.7 | 2^-40.0 | 2^-39.7 | 2^-37.8 | Table 3 |

Sizes (bytes), specification Table 4 vs the built reference library
(`OBSERVED/kex-06.txt`):

| instance | pk spec | pk impl | sk spec | sk impl | M1 spec | msgs impl | ss spec | ss impl | sta/stb impl | match |
|---|---|---|---|---|---|---|---|---|---|---|
| MAMBA-NIKE-128 | 1184 | 1184 | 3232 | 3232 | 1568 | 1568 | 32 | 32 | 32 / 0 | yes |
| MAMBA-NIKE-192 | 1312 | 1312 | 3360 | 3360 | 1568 | 1568 | 32 | 32 | 32 / 0 | yes |
| MAMBA-NIKE-256 | 1312 | 1312 | 3360 | 3360 | 1568 | 1568 | 32 | 32 | 32 / 0 | yes |
| MAMBA-NIKE-384 | 2848 | 2848 | 6944 | 6944 | 3360 | 3360 | 48 | 48 | 48 / 0 | yes |
| MAMBA-NIKE-512 | 2848 | 2848 | 6944 | 6944 | 3360 | 3360 | 64 | 64 | 64 / 0 | yes |

All ten size formulas reproduce exactly: `|pk| = 32 + n*t_pk/8`,
`|sk| = 2n + |pk|`, `|M1| = 32 + n*t_u/8 + n/4`.

## The quantization function (the novel component)

For each quantized component `x in {pk, u, v}`: `p_x = 2^{t_x}`,
`Delta_x = q/p_x`, `g_x = log2(q) - t_x`. A public dither `d` with coefficients
uniform in `{0, ..., Delta_x - 1}` is derived from the relevant seed.

```
# split label (what is transmitted), spec §8.7 / §2.1
Q_{Lambda_x, d}(a)[i] = ( ( a[i] + d[i] + 2^{g_x - 1} ) >> g_x ) & (2^{t_x} - 1)

# compensated normal form (what all arithmetic uses), spec §8.7
ahat[i] = ( ( b[i] << g_x ) - d[i] ) & (q - 1)
```

The point of the dither is that `e = ahat - a` is then a bounded error whose
distribution `chi_{q,p}` is **independent of the hidden value `a`** (App. A.3.1,
Thm. A.1), so `(a, bhat)` is an ordinary RLWE-type normal-form sample
`bhat = a*s + e` and the whole security argument reduces to a standard lattice
problem. This is what makes compression a "first-class primitive" rather than a
post-processing step: the transmitted object is the `t_x`-bit label, never the
13-bit coefficient, yet the algebra behaves like RLWE. Because `q` and `p_x`
are both powers of two, both directions are a shift, an add and a mask — no
modular reduction and no data-dependent branch.

Dither derivation (spec §8.5): SHAKE-128 over `domain || seed`, coefficients
read as 16-bit little-endian words masked to `Delta_x - 1`. Domain bytes:
`0xA0` for `d_pk` (from `rho`), `0xB0` for `d_u` and `0xB1` for `d_v` (both
from `mu`).

## Pseudocode (pass structure)

`kex_get_passes_num() = 1`. In NGCC terms A is the initiator (sends the one
message) and B the responder; in the specification's own wording the *responder*
owns the long-term key and the *initiator* sends `M1`.

### Init_A / Init_B — spec Alg. 1 `KeyGen()`

Both sides run the identical key generation; only B's key is actually used.

```
Init_X():
 1  rho <- {0,1}^256                                 # public seed
 2  (a, d_pk) <- GenPublic(rho)                      # a via SHAKE128(rho), d_pk via 0xA0||rho
 3  s <- B_eta_s^n                                   # ChaCha20-expanded CBD
 4  b <- Q_{Lambda_pk, d_pk}( a*s )                  # split label in R_{p_pk}
 5  pk <- Pack_{t_pk}(b) || rho                      # 32 + n*t_pk/8 bytes
 6  sk <- serialize(s) || pk                         # 2n + |pk| bytes
 7  st <- empty                                      # sta/stb both length 0 here
```

### pass1 (A -> B) — spec Alg. 2 `Pass1(pk_B)`

```
Pass1(sk_A, pk_B):                                   # NOTE: sk_A is NOT used
 1  (b, rho) <- parse(pk_B)
 2  (a, d_pk) <- GenPublic(rho)
 3  bhat <- (b << g_pk) - d_pk    (mod q)            # normal form of B's pk
 4  mu <- {0,1}^256
 5  r <- B_eta_r^n                                   # ephemeral secret
 6  (d_u, d_v) <- GenDither(mu)
 7  u <- Q_{Lambda_u, d_u}( a*r )                    # transmitted label
 8  v <- Q_{Lambda_v, d_v}( bhat*r ) ;  vhat <- (v << g_v) - d_v  (mod q)
 9  h <- HelpRec(vhat, rand)                         # kappa blocks, 2 bits x 4 per block
10  nu_A <- Rec(vhat, h)                             # kappa = n/4 raw bits
11  M1 <- mu || Pack_{t_u}(u) || Pack2(h)            # 32 + n*t_u/8 + n/4 bytes
12  K_A <- KDF(nu_A, pk_B, M1)
13  state_A <- K_A                                   # cached; sta = SS_BYTES
```

Message contents of pass1: `M1 = (mu, u, h)` — the 32-byte online dither seed,
the `t_u`-bit-packed response label, and the packed D4 helper. State after
pass1: A holds the finished shared secret (`sta = |ss|` bytes); B holds nothing
(`stb = 0`).

### passes 2 and 3

Not used. `kex_generate_pass2_msg_b` and `kex_generate_pass3_msg_a` are
compatibility stubs that zero their outputs and return 0 without emitting a
message (spec §8.1).

### DeriveSS_A

```
DeriveSS_A(state_A): return state_A          # the cached K_A, no recomputation
```

### DeriveSS_B — spec Alg. 3 `Derive(sk_B, pk_B, M1)`

```
 1  (mu, u, h) <- parse(M1)
 2  (d_u, d_v) <- GenDither(mu)               # d_v is regenerated but unused
 3  uhat <- (u << g_u) - d_u   (mod q)
 4  w <- uhat * s_B
 5  nu_B <- Rec(w, h)
 6  K_B <- KDF(nu_B, pk_B, M1)
```

Correctness: `vhat - w = e_pk*r - e_u*s + e_v` must satisfy
`|| blk_j(vhat - w) ||_1 < T_D4 = floor(3q/4) - 2 = 6142` for all `kappa`
blocks (spec §8.9, App. C).

### KDF

`K = SHAKE256( "MAMBA-NIKE-KDF20" || nu || pk_B || M1 , |ss| )`, with
`|nu| = n/32` bytes and `|ss|` per Table 4.

### D4 reconciliation (spec §8.9, App. A.3.2)

Block `j` is `(x_j, x_{j+kappa}, x_{j+2kappa}, x_{j+3kappa})` with
`kappa = n/4`; `HelpRec` emits four 2-bit values per block, randomized by one
ChaCha20-derived bit per block; `Rec` recovers one raw bit per block.

## Implementation vs specification

Checked: `kex-06/src/MAMBA-NIKE-128/{params.h, nike.c, poly.c,
error_correction.c, toom.c, KEX_AlgorithmInstance.c}` and the `params.h` of all
five instances. The five instance directories differ only in `params.h`
(verified by inspection of the parameter block).

Agreements:

- **All five parameter sets match Table 3 exactly**: `PARAM_N`, `PARAM_Q`,
  `PARAM_T_PK`, `PARAM_T_U`, `PARAM_T_V`, `PARAM_K` (= eta) are
  `1024/8192/9/10/6/2`, `1024/../10/10/6/3`, `1024/../10/10/6/7`,
  `2048/../11/11/6/2`, `2048/../11/11/6/5`. `params.h` additionally contains
  `#error` guards asserting `q = 2^LOG2Q` and `t_x = LOG2Q - h_x`.
- **All byte lengths match Table 4 and the OBSERVED values** (table above), and
  `params.h` hard-asserts them with `#if NIKE_SENDABYTES != 1184` style checks.
- **Quantization/compensation** (`nike.c:146-163`) is character-for-character
  the §8.7 rule, including the `2^{g-1}` rounding term and the final mask.
- **Dither generation** (`nike.c:108-144`) uses SHAKE-128 with distinct domain
  bytes `0xA0 / 0xB0 / 0xB1` and masks each coefficient to `Delta_x - 1`,
  matching §8.5.
- **Protocol flow**: `nike_keygen` = Alg. 1, `nike_sharedb` = Alg. 2,
  `nike_shareda` = Alg. 3, step for step, including the order
  `quantize v -> dequantize v -> HelpRec -> Rec` in Alg. 2 lines 7-10.
- **KDF** (`nike.c:165-186`) is `SHAKE256` over the 16-byte literal
  `"MAMBA-NIKE-KDF20"`, `nu`, the received public key and the full `M1`,
  matching §8.5 and Alg. 2/3.
- **Arithmetic**: `poly_convolution()` is Toom-Cook-4 over `int64_t` followed by
  the negacyclic fold `r[i] = prod[i] - prod[i+n]` and a mask by `q-1`, exactly
  §8.8. `poly_ntt()` / `poly_invntt()` are no-ops and `poly_pointwise()` is
  aliased to the full convolution, exactly as §8.8 states.
- **D4**: `helprec`/`rec` in `error_correction.c` are the NewHope routines with
  the power-of-two shortcuts `t = x >> LOG2Q` and `t = x >> (LOG2Q+2)`; the
  block indexing `(i, i+n/4, i+2n/4, i+3n/4)` and `kappa = n/4` match §8.9, and
  one ChaCha20 bit per block is consumed. I checked `f`, `g` and `LDDecode`
  against the published NewHope reference: the comparison directions
  (`k = (2q-1-k) >> 31` in `helprec`, `t -= 8q; t >>= 31` in `LDDecode`) are
  unchanged, so there is no inverted-comparison / min-vs-max defect here. All
  shift inputs are provably non-negative (`x in [0, 8q)` in `helprec`,
  `tmp in [7q, 24q]` in `rec`), so the arithmetic-shift shortcut is sound.
- **Constant time**: no secret-dependent branch or index was visible in
  `poly_getnoise`, `poly_quantize`, `poly_dequantize`, `poly_pack_bits`,
  `poly_convolution`, `helprec` or `rec`; `helprec` uses the NewHope masked
  select rather than a branch, as §8.10 requires.

Discrepancies:

- **(a) REAL DEVIATION — §8.5's description of the expansion of `a` does not
  match the code.** §8.5 says "coefficients of `a` are read as 16-bit
  little-endian words and masked modulo q". `poly_uniform()`
  (`poly.c:42-100`) instead, for `q == 8192`, unpacks the SHAKE-128 stream as a
  **contiguous 13-bit bit-stream** through an accumulator
  (`acc |= buf[pos] << bits; while (bits >= 13) coeff = acc & 0x1FFF`). These
  produce completely different polynomials `a` from the same seed. All five
  submitted profiles use `q = 8192`, so the 13-bit path is always taken. An
  implementer following §8.5 literally would produce different public keys and
  fail every KAT. (The 16-bit-word-and-mask description *is* accurate for the
  dither, which is the second half of the same sentence — so this reads as the
  spec text not having been updated when the 13-bit packer was introduced.)
  **Highest-value finding for this candidate.**
- **(a) claim/name mismatch — the initiator's long-term key is never used, so
  this is not a NIKE.** `kex_generate_pass1_msg_a()`
  (`KEX_AlgorithmInstance.c:233-262`) receives `ska` but passes only `pkb` to
  `nike_sharedb(ss, m1, pkb)`; `ska` is used solely for a length and
  canonicality check. `nike_sharedb` samples a *fresh ephemeral* `r` and the
  shared secret is `KDF(nu, pk_B, M1)` — a function of B's static key and A's
  ephemeral only. Consequences: (i) the scheme is an unauthenticated one-pass
  ephemeral-static exchange (essentially an IND-CPA KEM encapsulation to B),
  not a non-interactive key exchange; (ii) anybody can produce a valid `M1` for
  B, so there is no implicit authentication of A whatsoever; (iii)
  `kex_init_a`'s keypair is dead weight (1184-3232 bytes per instance). The
  specification is consistent with the code (§2.2 "one online pass is
  sufficient", §8.1, App. B.4 excludes active/CCA claims), so this is a
  **naming and product-positioning defect rather than a code bug** — but the
  title "Non-Interactive Key Exchange" is not what the submitted primitive
  does, and a reviewer reading only the title would draw the wrong conclusion.
- **(b) spec gap — the source of the `HelpRec` randomness is unspecified.**
  §8.9 says only "one ChaCha20-derived random bit per block". The code
  (`nike.c:261`) calls `helprec(&c, &v, noiseseed, 3)` with **the same 32-byte
  seed used to sample the ephemeral secret `r`**, separated only by the
  ChaCha20 nonce (`n[0]=nonce` in `poly_getnoise` versus `n[7]=nonce` in
  `helprec`, so seeds 0 and 3 do not collide). This is inherited from NewHope
  and is not itself a break, but reusing the ephemeral secret seed for public
  helper randomness is not stated anywhere in the specification and deserves an
  explicit sentence; an implementer picking a fresh seed would fail the KATs.
- **(b) spec table looks wrong — Table 3 quantum column.** MAMBA-NIKE-384 and
  MAMBA-NIKE-512 both report `MATZOV-Q = 512.5` while their classical columns
  differ (538.6 vs 590.2). Two different parameter sets (eta = 2 vs 5) giving
  an *identical* quantum estimate to one decimal place looks like a copy-paste
  in the table rather than a genuine estimator output. Also, both n=2048
  profiles claim more quantum security (512.5) than the n=1024 profiles' 251-274
  despite `MAMBA-NIKE-384`'s target being 384 — the row is at best confusing.
- **(b) dead comment.** `error_correction.c` says "only the first
  NIKE_RECGROUPS groups are reconciled", but `NIKE_RECGROUPS == PARAM_N/4 ==
  group`, so *all* blocks are consumed (as §8.9 requires). Harmless, but the
  comment implies a truncation that does not exist and would mislead an auditor
  into thinking part of the state is unused.
- **(b) `poly_dither` buffer arithmetic** (`nike.c:124`) refills when
  `pos > SHAKE128_RATE - 2`; with `SHAKE128_RATE = 168` this is exact (the last
  read is at `pos = 166,167`) and does not overrun, but it is an off-by-one
  away from a read past the buffer. Noted, not a defect.
- **Not verified:** I did not re-run the estimator, did not verify the
  key-rejection probabilities of Table 3 (the spec itself notes
  `compute_rejection.py` truncates at 1e-13), did not check the Toom-Cook-4
  implementation for correctness beyond reading it, and did not verify the
  Optimized/AVX2 track beyond a file comparison (only the Reference tree is
  built): at every level it differs from the Reference tree only in `Makefile`,
  `README.txt`, the ChaCha20 backend (`crypto_stream_chacha20.c`, `chacha.S`)
  and `poly.c`/`poly.h`/`nike.c`, where the four protocol multiplications call
  an AVX2 schoolbook `poly_mul_small` instead of `poly_pointwise`. All five
  instances pass the candidate KATs in the build
  (`kex-06/security_findings.md`).
