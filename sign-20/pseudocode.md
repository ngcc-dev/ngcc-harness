# sign-20 Qingluan (青鸾 / Qing Luan) — algorithm summary

Code-based signature from the Restricted Syndrome Decoding Problem (R-SDP) over the
order-7 multiplicative subgroup E = ⟨2⟩ ⊂ F*_127: given H ∈ F_p^{(n−k)×n} and s, find
e ∈ E^n with eH^T = s. The scheme is an explicit re-instantiation of **CROSS-RSDP**'s
5-pass MPC-in-the-Head Σ-protocol made non-interactive by Fiat–Shamir over t fixed-weight
parallel rounds, with every hash/XOF/DRBG built from SM3 (GB/T 32905-2016) instead of
SHAKE. There is **no party count N and no GGM seed tree**: per-round soundness is ≈ 1/2
and the t revealed seeds / commitments are sent directly ("fast-style"), which is why
signatures are large. All commitments and the message digest are salted (eTCR).

Specification: `sign-20-spec.pdf` (25 pp., English; a Chinese `中文资料/Basic information.pdf`
also ships). Algorithm text is §1.7 (KeyGen), §1.8 (Sign), §1.9 (Verify), §1.10 (seeds),
§1.11 (parameters), §1.12 (sizes); §2.2 is the parameter rationale.

## Parameters

Spec §1.11 table (p, z, g are fixed at all levels, as in CROSS-RSDP).

| parameter | QingLuan-128 | -256 | -384 | -512 | meaning |
|---|---|---|---|---|---|
| p | 127 | 127 | 127 | 127 | base field F_p (7-bit) |
| z = \|E\| | 7 | 7 | 7 | 7 | order of restricted subgroup |
| g | 2 | 2 | 2 | 2 | generator, E = {1,2,4,8,16,32,64} |
| n | 127 | 251 | 370 | 491 | code length |
| k | 76 | 150 | 221 | 293 | code dimension (rate ≈ 0.596) |
| r = n−k | 51 | 101 | 149 | 198 | rows of H = [V \| I_r] |
| t | 256 | 512 | 763 | 1018 | parallel FS rounds (tau) |
| w | 212 | 424 | 631 | 842 | weight of chall2 ∈ B(t,w) |
| λ | 128 | 256 | 384 | 512 | security parameter |
| SEED_BYTES | 16 | 32 | 48 | 64 | λ/8 |
| SALT/HASH_BYTES | 32 | 64 | 96 | 128 | 2λ/8 |
| HASH_PIPES | 1 | 2 | 3 | 4 | ⌈2λ/256⌉ SM3 pipes |
| claimed security | 128 | 256 | 384 | 512 | classical bits (§2.2.6: raw forgery 128.8 / 257.2 / 384.3 / 512.1; quantum claimed = classical/2) |

Sizes (bytes), spec §1.12 vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | sig spec | sig impl | match |
|---|---|---|---|---|---|---|---|
| QingLuan-128 | 77 | 77 | 32 | 32 | 18,720 | 18,720 | yes |
| QingLuan-256 | 153 | 153 | 64 | 64 | 74,248 | 74,248 | yes |
| QingLuan-384 | 227 | 227 | 96 | 96 | 164,940 | 164,940 | yes |
| QingLuan-512 | 302 | 302 | 128 | 128 | 292,816 | 292,816 | yes |

Spec size formulas (§1.12), which I evaluated independently for all four levels and which
reproduce the observed values exactly:

```
|pk|  = 2λ/8 + ceil(r*7/8)
|sk|  = 2λ/8                                   (a single Seed_sk)
|sig| = Salt + 2*Hash + w*Seed + w*Hash + (t-w)*( ceil(n*7/8) + ceil(n*3/8) + Hash )
```

## Pseudocode

### KeyGen (spec §1.7)
```
 1  Seed_sk  <- DRBG(2λ/8)                                   # sk = Seed_sk, nothing else
 2  (Seed_e, Seed_pk) <- XOF(DOMAIN_EXPAND_SK=0x00 || Seed_sk)   # 2λ/8 bytes each
 3  V  <- XOF-F_p^{r×k}(DOMAIN_MATRIX=0x01 || Seed_pk);  H = [ V | I_r ]
 4  eta <- XOF-F_z^n(DOMAIN_ERROR=0x02 || Seed_e)            # 3-bit rejection, accept < 7
 5  e_j = g^{eta_j} in E                                     # constant-time table lookup
 6  s = e H^T = V*e_A^T + e_B  in F_p^r
 7  pk = Seed_pk || pack_7(s);   sk = Seed_sk
 8  erase eta, e, V, s
```

### Sign (spec §1.8, phases 1–5)
```
 1  (eta,H) <- ExpandSK(Seed_sk); e = g^eta; s = eH^T; pk = Seed_pk||pack(s)
 2  pk_hash = Hash(pk)                                      # BUFF-style key binding
 3  Salt <- {0,1}^{2λ};  Seed <- {0,1}^{λ}
 4  digest_Msg = Hash(DOMAIN_MSG=0x0A || Salt || pk_hash || m)     # eTCR
 5  Seed[1..t] <- XOF(DOMAIN_SEEDLEAVES=0x03 || Seed || Salt)      # §1.10: no GGM tree
 6  for i = 1..t:                                                  # dom = i + c, c = 2t-1
 7      (eta'[i], u'[i]) <- XOF(DOMAIN_ROUND=0x04 || Seed[i] || Salt || dom)  # F_z^n, F_p^n
 8      v[i]  = eta - eta'[i]  (mod z);  vE[i] = g^{v[i]};  e'E[i] = g^{eta'[i]}
 9      u[i]  = vE[i] * u'[i]  (componentwise);  s'[i] = u[i] H^T
10      cmt0[i] = Hash(DOMAIN_CMT0=0x05 || s'[i] || v[i] || Salt || dom)
11      cmt1[i] = Hash(DOMAIN_CMT1=0x06 || Seed[i] || Salt || dom)
12  digest_cmt0 = Hash(0x07||cmt0[1..t]); digest_cmt1 = Hash(0x08||cmt1[1..t])
13  digest_cmt  = Hash(0x09 || digest_cmt0 || digest_cmt1)
14  digest_chall1 = Hash(DOMAIN_CHALL1=0x0B || digest_Msg || digest_cmt || Salt)
15  chall1 <- XOF-(F_p^*)^t(0x0C || digest_chall1 || t+c)         # one nonzero scalar / round
16  for i = 1..t:  y[i] = u'[i] + chall1[i] * e'E[i]  in F_p^n
17  digest_chall2 = Hash(DOMAIN_CHALL2=0x0D || y[1..t] || digest_chall1)
18  chall2 <- XOF-B(t,w)(0x0E || digest_chall2 || t+c+1)          # fixed weight w
19  sigma = Salt || digest_cmt || digest_chall2
20          || { Seed[i] : chall2[i]=1 }          # Path,  w * SEED_BYTES
21          || { cmt0[i] : chall2[i]=1 }          # Proof, w * HASH_BYTES
22          || { pack_7(y[i]) || pack_3(v[i]) || cmt1[i] : chall2[i]=0 }   # t-w responses
```

### Verify (spec §1.9)
```
 1  parse pk -> Seed_pk, s = unpack(.); expand V, H = [V|I_r]
 2  reject unless |sigma| is exactly the fixed length; parse the five fields
 3  pk_hash, digest_Msg, digest_chall1, chall1 recomputed as in Sign; chall2 from digest_chall2
 4  for i = 1..t:
 5      if chall2[i]=1:  cmt1[i] = Hash(0x06||Seed[i]||Salt||dom);
 6                       (eta'[i],u'[i]) from Seed[i]; y[i] = u'[i] + chall1[i]*g^{eta'[i]};
 7                       cmt0[i] taken from Proof
 8      else:            read (y[i], v[i], cmt1[i]); CHECK v[i] in F_z^n else reject;
 9                       y'[i] = g^{v[i]} * y[i];  s'[i] = y'[i]H^T - chall1[i]*s;
10                       cmt0[i] = Hash(0x05||s'[i]||v[i]||Salt||dom)
11  digest_cmt' = Hash(0x09||Hash(0x07||cmt0[1..t])||Hash(0x08||cmt1[1..t]))
12  digest_chall2' = Hash(0x0D || y[1..t] || digest_chall1)
13  accept iff digest_cmt == digest_cmt' AND digest_chall2 == digest_chall2'  (constant time)
```

### Hashing (spec §1.5)
```
Hash(msg) = SM3(0x00||msg) || SM3(0x01||msg) || ... || SM3(byte(P-1)||msg), P = ceil(2λ/256)
            truncated to 2λ/8 bytes;  P = 1 (λ=128) uses plain SM3 with NO prefix byte.
XOF: absorb with the multi-pipe hash -> key; squeeze O_i = SM3(key || BE32(i)).
DRBG: SM3 Hash-DRBG (GM/T 0005), 32-byte seed, auto-reseed every 1024 outputs.
```
Honest caveat stated by the spec itself: multi-pipe widens preimage/2nd-preimage/TCR to
2λ bits but **not** collision resistance beyond ≈ 2^128 (Joux multicollision), hence the
universal salting.

## Implementation vs specification

Checked against the reference sources in the original submission archive under
`Implementations and Test_Vectors/Implementations/Reference_Implementation/QingLuan-<lvl>/`
(`include/params.h`, `include/api.h`, `src/keygen.c`, `src/sign.c`, `src/verify.c`,
`src/mpc.c`, `src/rsdp.c`, `src/restr.c`, `api_pkc/SIG_AlgorithmInstance.c`).

- **Parameter sampling.** I checked (p, z, g), (n, k, r), (t, w) and the byte lengths for
  all four levels in `include/params.h` lines 64–151: every value matches the §1.11 table
  (e.g. -512: `PARAM_N 491`, `PARAM_K 293`, `PARAM_R 198`, `PARAM_TAU 1018`, `PARAM_W 842`).
  `PARAM_C = 2*PARAM_TAU - 1` matches c = 2t−1.
- **Sizes.** `QINGLUAN_SIG_BYTES` (params.h:177) is literally the §1.12 formula; the
  `sig_get_{pk,sk,sn}_len_bytes()` exports return these macros, and all 12 spec/impl size
  cells above agree. No size mismatch at any level.
- **Domain separation.** All 16 tags `DOMAIN_EXPAND_SK`..`DOMAIN_COMMIT` (params.h:204–219)
  match the §1.6 table byte-for-byte (0x00..0x0F), and `src/mpc.c` absorbs the tag plus a
  16-bit little-endian instance constant (i+c, t+c, t+c+1) exactly where §1.8 says.
- **Verifier range checks.** `rsdp_unpack_fz` (src/rsdp.c:~227) enforces v[i] < z, the §1.9
  restricted-ness check; `rsdp_unpack_fp` enforces y[i] < p. Both additionally reject
  non-zero padding bits (`pad_bits_zero`), i.e. the implementation enforces canonical
  encoding more strictly than the spec requires — consistent with the "no trivial
  malleability found" result in `security_findings.md`. Fixed signature length is checked
  first (`verify.c:47`).
- **Discrepancy (c), cosmetic/equivalent — seed-leaf derivation.** §1.10 writes
  `Seed[i] = CSPRNG(DOMAIN_SEEDLEAVES || Seed || Salt || i)`, one expansion per index;
  `src/sign.c:74–81` instead absorbs `DOMAIN_SEEDLEAVES || Seed || Salt` once and squeezes
  one contiguous `t * SEED_BYTES` stream, slicing it into the t leaves. Same independence
  property, but the literal formula in §1.10 does not include the index the way the code
  derives leaves. §1.8 Phase 1 describes it the looser (matching) way, so this is an
  internal spec inconsistency rather than a code bug.
- **Observation (not a discrepancy).** `include/params.h:29–47` carries a `QL_TOY`
  parameter set (`QingLuan-TOY`, n=12, k=6, t=8, w=5, `QINGLUAN_SECURITY 8`) for TDD. It is
  guarded by `#ifdef QL_TOY`, which the NGCC Makefile never defines, so it cannot be
  selected in our build; each instance folder also self-defines its own level at
  params.h:4–6, making the Makefile's `-DQINGLUAN_<lvl>` redundant but consistent.
- **Observation.** The spec's own §2.2.2 (and the header comment at params.h:53–55) admit
  that (n,k) for 384/512 are *linear extrapolations* of the CROSS R-SDP cat-1/cat-5 codes,
  not independently analysed parameters; §2.2.3 declines the CROSS "+5" credit, leaving
  ≈5 bits of forgery margin. Not verified here — estimator work is listed as `not_tested`
  in `security_findings.md`.
- **Not verified (time-box).** SM3 multi-pipe/XOF/DRBG byte-level behaviour in
  `api_pkc/hash_adapter.c` and `api_pkc/drng.c`, the constant-time claims of §2.4.2, and a
  full signer/verifier transcript-absorption diff. KAT conformance is PASS for all four
  instances per `RESULTS.md` / `security_findings.md`.
