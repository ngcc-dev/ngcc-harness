# sign-29 Tins — algorithm summary

MPC-in-the-Head signature: a TCitH (threshold-computation-in-the-head) zero-
knowledge proof for the **Normalized Subfield Bilinear Collision (NSBC)**
problem of Huth–Joux (2024), made non-interactive by Fiat–Shamir with a
proof-of-work (grinding) term on the last challenge. Commitments use a batched
all-but-one vector commitment (BAVC) over GGM seed trees; the relation is
checked by opening one random evaluation point of a degree-1 polynomial per
repetition.

Specification: `sign-29-spec.pdf` (23 pages), §2.2 (NSBC), §4.1 (subroutines,
Algorithms 1–14), §4.2–4.4 (Algorithms 15–17), §5 (Table 2), §7.2 (Table 5).

Hardness assumption (§2.2, Definition 1 / §6.1, Assumption 1). Given
û = (u, u_{n−2}, u_{n−1}) ∈ F_{q^k}^n and v̂ = (v, v_{n−2}, v_{n−1}) ∈ F_{q^k}^n,
linearly independent over F_q, find α, β ∈ F_q^{n−2} with
```
(⟨u,α⟩ + u_{n−2})·(⟨v,β⟩ + v_{n−1}) = (⟨u,β⟩ + u_{n−1})·(⟨v,α⟩ + v_{n−2}).
```
Note the *subfield* structure: û, v̂ live in the big field F_{q^k} but the
witness α, β is over the base field F_q = F_2.

## Parameters (§5, Table 2)

| parameter | Tins-128 | Tins-256 | Tins-512 | meaning |
|---|---|---|---|---|
| λ | 128 | 256 | 512 | security parameter |
| q | 2 | 2 | 2 | base field |
| k | 276 | 528 | 1044 | extension degree; |F_{q^k}| element = k bits ≈ 2λ |
| n | 140 | 268 | 524 | vector length; spec requires n ≈ k/2 |
| µ | 12 | 12 | 12 | tree depth, N = 2^µ |
| N | 2^12 | 2^12 | 2^12 | leaves per GGM tree (parties) |
| τ | 12 | 24 | 47 | repetitions |
| Topen | 125 | 255 | 510 | max revealed nodes in a BAVC opening |
| ω | 6 | 4 | 6 | grinding (proof-of-work) bits |
| claimed security | 137 | 261 | 521 | classical bits (spec's own claim) |

Soundness target (§5): choose N, τ, ω with `(2/N)^τ ≤ 1/2^{λ−ω}`; Topen is set
so that ≈20% of openings fit.

Sizes (bytes), specification (§7.2 Table 5) vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | sig spec | sig impl | match |
|---|---|---|---|---|---|---|---|
| Tins128 | 51 | 51 | 32 | 32 | 3284 | 3284 | yes |
| Tins256 | 98 | 98 | 64 | 64 | 13012 | 13012 | yes |
| Tins512 | 195 | 195 | 128 | 128 | 51187 | 51187 | yes |

The secret key is exactly two λ-bit seeds (2λ/8 = 32/64/128 bytes), so
sk = seed only, as §4.2 states. pk = λ + k bits = ⌈(λ+k)/8⌉ = 51/98/195.
The §4.3 signature formula
`|σ| = 2λ + 64 + 2λ + (λ·Topen + τ·2λ) + τ(2n − 4 + k)` bits reproduces
3284 / 13012 / 51187 bytes exactly for all three instances.

## Pseudocode

### KeyGen — Algorithm 15 (§4.2)
```
seed_sk <- TRG.Seed(λ)
seed_pk <- TRG.Seed(λ)
(û, v̂, α, β) <- ExpandSecretVector(seed_sk, seed_pk)        // Alg. 3
pk <- (seed_pk, v_{n−1})
sk <- (seed_sk, seed_pk)
```
`ExpandSecretVector` (Alg. 3) is where the NSBC instance is *planted*:
```
(û, v, v_{n−2}) <- ExpandPublicVector(seed_pk)               // Alg. 2: (2n−1)·k F_2-bits
(α, β)          <- SampleFieldElement(seed_sk, 2n−4, offset) // the witness, over F_q
retry (offset += 2n−4) while ⟨u,α⟩ + u_{n−2} = 0
v_{n−1} := [ (⟨u,β⟩+u_{n−1})(⟨v,α⟩+v_{n−2}) − (⟨u,α⟩+u_{n−2})⟨v,β⟩ ] / (⟨u,α⟩+u_{n−2})
retry if û = γ·v̂ for some γ ∈ F_q                            // independence over F_q
```
so v_{n−1} — the only non-seed part of pk — is the value that makes (α, β) a
collision by construction.

### Sign — Algorithm 16 (§4.3)
```
salt  <- TRG.Seed(2λ)
rseed <- TRG.Seed(λ)
(û, v̂, α, β) <- ExpandSecretVector(seed_sk, seed_pk)
(base, δ, h_sh, key, aux) <- CommitPoly(salt, rseed, α, β)     // Alg. 9: BAVC.Commit
                                                               // + per-rep sharing of (α,β)
for e = 0 .. τ−1:
    (p_mid[e], p_base[e]) <- ComputePoly(base[e], δ[e], α, β, û, v̂)   // Alg. 10
h_piop <- Hash2( seed_pk, salt, msg, h_sh,
                 p_mid[0..τ−1], p_base[0..τ−1], v_{n−1} )
(ctr, π_BAVC) <- OpenRandomEva(key, h_piop)                    // Alg. 12: grind on ctr
                    // derive the challenge points and v_grinding from
                    // PRG(h_piop || bin64(ctr), τ·µ + ω)  (Alg. 11),
                    // require ω zero grinding bits, open all-but-one per tree,
                    // restart if the opening needs more than Topen nodes
σ := (salt, ctr, h_piop, π_BAVC, p_mid, aux)
```

### Verify — Algorithm 17 (§4.4)
```
parse pk = (seed_pk, v_{n−1});  σ = (salt, ctr, h_piop, π_BAVC, p_mid, aux)
(û, v, v_{n−2}) <- ExpandPublicVector(seed_pk);   v̂ := (v, v_{n−2}, v_{n−1})
(eval, point, h_sh, v_grinding) <- ComputeEva(salt, ctr, h_piop, π_BAVC, aux)  // Alg. 13
                    // re-derives the challenge points from PRG(h_piop || bin64(ctr)),
                    // rebuilds the N−1 opened leaves, checks the ω grinding bits
for e = 0 .. τ−1:
    p_base[e] <- RecomputePolyProof(point[e], eval[e], û, v̂, p_mid[e])  // Alg. 14
h'_piop <- Hash2( seed_pk, salt, msg, h_sh,
                  p_mid[0..τ−1], p_base[0..τ−1], v_{n−1} )
return (h'_piop == h_piop)
```
(The ordering above is the *signer's*; see Discrepancy 2 — Alg. 17 line 6 in the
spec prints a different, incompatible ordering.)

### Hash / domain separation (§4.1.1)
```
Hash_i : {0,1}* -> {0,1}^{2λ},  Hash_i(data) := Hash( i , data ),  i ∈ {1,2,3}
   with the prefix i encoded in ONE byte.
PRG(seed, klen), TRG.Seed(λ): left abstract by the spec ("black box").
Encrypt-λ(key, msg): a λ-bit block cipher, "selection depends on the security level".
```

## Implementation vs specification

What was checked. The Makefile builds
`Implementations/Reference_Implementation/Tins{128,256,512}` from
`ff_arith.c SIG_TINS<level>.c drng.c bavc_commit.c auxfunc.c`. I read
`src/Tins128/params.h` (all three levels), and the whole of `sig_sign` and
`sig_verify` in `src/Tins128/SIG_TINS128.c`, plus the corresponding regions of
`SIG_TINS256.c` and `SIG_TINS512.c`.

Agreements.
- Parameter sampling — all eight scheme constants, all three levels, from
  `src/Tins<L>/params.h`: `LAMBDA` 128/256/512, `N 4096` (= 2^µ with `MU 12`),
  `K` 276/528/1044, `N_TUPLE` 140/268/524 (= n), `TAU` 12/24/47,
  `T_OPEN` 125/255/510, `Omega` 6/4/6. Every one matches Table 2.
- `PK_SIZE = PK_SEEDLEN + FE_BYTES` = λ/8 + ⌈k/8⌉ and
  `SK_SIZE = SK_SEEDLEN + PK_SEEDLEN` = 2λ/8 reproduce Table 5 exactly, and
  match §4.2's statement that pk = (seed_pk, v_{n−1}) and sk = two seeds.
- `SIG_SIZE` (`params.h:34`) = `SALT_SIZE + sizeof(long long) + COMMIT_SIZE +
  NODE_SIZE·T_OPEN + COMMIT_SIZE·TAU + (K·TAU + (N_TUPLE−2)·TAU·2 + 7)/8`
  is term-by-term the §4.3 formula (2λ salt, 64-bit ctr, 2λ h_piop,
  λ·Topen + τ·2λ for π_BAVC, τ(k + 2(n−2)) for p_mid ‖ aux) and evaluates to
  3284/13012/51187.
- Domain separation is implemented as specified: `nonce[0] = 2` before the
  Hash2 absorption in both signer (`SIG_TINS128.c:279`) and verifier (`:419`).
- Hash instantiation differs by level: Tins128 uses `sm3hash(256, ...)`
  (SM3, 2λ = 256-bit output); Tins256/Tins512 use `pseudohash(2λ, ...)`. Both
  match the §4.1.1 interface `{0,1}* -> {0,1}^{2λ}`; the spec deliberately
  leaves the concrete primitive open, so this is (c), not a deviation.

**Discrepancy 1 (real deviation, Tins128 only, security-relevant) — the
verifier compares against uninitialized memory and destroys the value it was
supposed to compare.** In `src/Tins128/SIG_TINS128.c`:
```
363:  memcpy(h_piop, sn + pos, sizeof(hash_t));   // parse h_piop OUT OF THE SIGNATURE
414:  hash_t test_h_piop;                          // declared, never written
430:  sm3hash(256, nonce, ..., h_piop);            // RECOMPUTED hash overwrites the parsed one
436:  ch = ch | (h_piop[i] ^ test_h_piop[i]);      // compares recomputed vs UNINITIALIZED
439:  return !(ch == 0);
```
The signature-supplied `h_piop` is clobbered at line 430, and the comparison at
line 436 reads 32 bytes of uninitialized stack. This is undefined behaviour and
means the final Fiat–Shamir equality check of Algorithm 17 line 7 is **not
performed at all** on Tins128. The same code in `Tins256`
(`SIG_TINS256.c:430`) and `Tins512` (`SIG_TINS512.c:439`) writes the recomputed
digest into `test_h_piop` and is correct, which confirms line 430 of the 128-bit
file is a transcription slip rather than an intended design. Classification:
(a) a real deviation. It is a plausible root cause for the `sig-signature-flip`
FINDING recorded for Tins128 in `security_findings.md` ("modified signature
accepted at sampled bit 843"); the corresponding findings for Tins256/Tins512
must have a different cause and are not explained by this.

**Discrepancy 2 (spec defect) — Algorithms 16 and 17 hash different
transcripts.** Alg. 16 line 8 absorbs
`(seed_pk, salt, msg, h_sh, p_mid[0..τ−1], p_base[0..τ−1], v_{n−1})` while
Alg. 17 line 6 absorbs
`(seed_pk, v_{n−1}, salt, msg, h_sh, p_mid[0], p_base[0], …, p_mid[τ−1],
p_base[τ−1])` — v_{n−1} moved to the front and p_mid/p_base interleaved instead
of concatenated. As printed, no honest signature would ever verify. The
implementation uses the signer's ordering in both directions
(`SIG_TINS128.c:279-288` vs `:419-430`: tag ‖ seed_pk ‖ salt ‖ msg ‖ h_sh ‖
all p_mid ‖ all p_base ‖ v_{n−1}), so Alg. 16 is the correct reading.
Classification: (b) spec error, no implementation impact.

**Discrepancy 3 (real deviation, all three levels) — the signature length is
not bound.** `sig_verify` derives the BAVC path length arithmetically from the
caller-declared length (`SIG_TINS128.c:354-356`):
`path_size = (sn_len_bytes − NTemp) / NODE_SIZE`, with
`NODE_SIZE = λ/8` = 16/32/64. Integer division means any appended tail shorter
than NODE_SIZE bytes is silently ignored and never enters the hash, so an
extended signature verifies unchanged. This is the root cause of the
`sig-append` FINDING ("signature with appended byte accepted") recorded for all
three instances. The spec's §4.3 formula is introduced as the *maximum* size
("The maximum bit size of a Tins signature σ is …"), i.e. variable-length
signatures are intended, but neither §4.3 nor §4.4 specifies how the verifier
must recover Topen/path_size or that it must reject a non-canonical length.
Classification: (a) implementation defect on top of (b) a spec gap.

**Discrepancy 4 (spec gap).** §4.1.2 explicitly leaves `TRG.Seed` and `PRG` as
black boxes ("in practical implementation, it can be concretely realized based
on specific circumstances"), and §4.1.1 leaves `Encrypt-λ` unspecified. The
random-oracle/PRG assumptions of Theorem 1 therefore cannot be checked against
any named primitive. The build wires randomness to the shared NGCC
`drng_algorithm` (per the Makefile), and `security_findings.md` confirms
keygen consumes it.

**Discrepancy 5 (real deviation, Tins128 and Tins256) — grinding predicate.**
`ExpandChallengePoint` (`Tins128/bavc_commit.c:481-508`) seeds the DRNG with
`h_piop ‖ ctr`, draws `TAU*MU + Omega` bits and sets `v_grinding` to
`(buffer[last] >> 4) | (buffer[(TAU-1)/2*3+1] << 4)` (`:507`; `Tins256` `:501`,
`Tins512` `:523`). For even τ the second byte's low nibble is the top four bits
of the last challenge index, so `v_grinding == 0` tests four fresh bits and
requires `i*[τ−1] < 256`, instead of ω fresh bits as in Alg. 11. A driver
calling `ExpandChallengePoint` for 400000 counters (gcc 15.2.0, `-O2 -std=c99`)
passed 1 in 263 (Tins128, ω = 6) and 1 in 270 (Tins256, ω = 4), with the last
index never above 255; Tins512 (odd τ) passed 1 in 65, consistent with ω = 6.
Classification: (a) a real deviation.

Not verified. I did not check `CommitPoly` (Alg. 9), `ComputePoly` (Alg. 10),
`OpenRandomEva` (Alg. 12), `ComputeEva` (Alg. 13) or `RecomputePolyProof`
(Alg. 14) line-by-line against the spec, nor the BAVC tree construction in
`bavc_commit.c`, nor the F_{2^k} arithmetic in `ff_arith.c`. Apart from the
Discrepancy 5 driver I did not run any code; the test outcomes cited above are
quoted from `sign-29/security_findings.md`. Discrepancy 1's *runtime* effect depends on
stack contents and compiler choices, which I did not measure — the source-level
defect is certain, the observable behaviour is not fully characterised.
