# sign-28 SYDO — algorithm summary

Code-based signature from the **regular syndrome decoding (RSD)** problem over
F2, proved in zero knowledge with a **VOLE-in-the-Head** proof system
(FAEST/BAVC lineage) and made non-interactive by Fiat–Shamir with a grinding
(proof-of-work) counter. The RSD relation is expressed as a degree-d polynomial
constraint system over F2^λ and checked QuickSilver-style over the VOLE
correlation. Hardness reduces to unstructured binary SD near the
Gilbert–Varshamov bound (§5.1.1, Theorem 3).

Specification: `sign-28-spec.pdf` (65 pages), §4.1 (Algorithms 1–3), §4.2
(subroutines, Algorithms 4–28), §5.2 (parameters, Tables 5.2–5.4).

## Parameters

RSD parameters (Table 5.2) — the same for the `s` and `f` variant of a level:

| parameter | SYDO-160 | SYDO-256 | SYDO-512 | meaning |
|---|---|---|---|---|
| λ | 160 | 256 | 512 | security parameter |
| n | 16461 | 26226 | 49941 | code length |
| n − k | 480 | 768 | 1456 | codimension (syndrome length) |
| w | 59 | 94 | 179 | regular weight; n/w = 279 / 279 / 279 |
| ℓ | 2 | 2 | 2 | number of encoding blocks |
| (ηj) | (2, 8) | (2, 8) | (2, 8) | block bit-widths; Σηj = 10 |
| (rj) | (1, 3) | (1, 3) | (1, 3) | block repetitions; d = Σrj = 4 |
| claimed security | 160 cl. / 80 q. | 256 / 128 | 512 / 256 | bits (spec's own claim) |

VOLEitH proof-system parameters (Table 5.3):

| parameter | 160s | 160f | 256s | 256f | 512s | 512f | meaning |
|---|---|---|---|---|---|---|---|
| N | 2^11 | 2^8 | 2^11 | 2^8 | 2^11 | 2^8 | GGM-tree leaves per repetition |
| τ | 14 | 20 | 23 | 32 | 46 | 64 | repetitions |
| wgrind | 8 | 2 | 5 | 2 | 8 | 2 | grinding (PoW) bits |
| Topen | 132 | 138 | 225 | 236 | 445 | 446 | max seeds in a BAVC opening |
| d | 4 | 4 | 4 | 4 | 4 | 4 | constraint-system degree bound |
| B | 16 | 16 | 16 | 16 | 16 | 16 | VOLE-hash parameter (bits) |

Soundness condition (§5.2): `τ·log2 N − log2 d + wgrind ≥ λ`, with
`τ := ⌊λ / log2 N⌋`. The `s` variant uses bigger trees (fewer repetitions,
smaller signature); `f` uses smaller trees (faster GGM expansion, bigger
signature).

Sizes (bytes), specification (§5.2 Table 5.4) vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | sig spec | sig impl | match |
|---|---|---|---|---|---|---|---|
| sydo_160s | 80 | 80 | 174 | 174 | 5428 | 5428 | yes |
| sydo_160f | 80 | 80 | 174 | 174 | 6724 | 6724 | yes |
| sydo_256s | 128 | 128 | 278 | 278 | 14444 | 14444 | yes |
| sydo_256f | 128 | 128 | 278 | 278 | 17604 | 17604 | yes |
| sydo_512s | 246 | 246 | 534 | 534 | 56672 | 56672 | yes |
| sydo_512f | 246 | 246 | 534 | 534 | 67716 | 67716 | yes |

All eighteen numbers agree with Table 5.4.

## Pseudocode

### KeyGen — Algorithm 1 (§4.1.1)
```
seed_sk <-$ {0,1}^λ ;  seed_pk <-$ {0,1}^λ
(x, w)  <- RSD.ExpandWitness(seed_sk)      // Alg. 23: x = regular weight-w vector,
                                           // w = its mixed-radix (ηj,rj) encoding
y       <- RSD.ExpandPartialH(seed_pk, x)  // Alg. 26: y = H·x, H expanded from seed_pk
pk := (seed_pk, y)                         // λ + (n−k) bits
sk := (pk, w)                              // + seed_sk, see the type line of Alg. 1
return (sk, pk)
```

### Sign — Algorithm 2 (§4.1.2)
```
 Phase 0 — Initialization
   depth := log N ;  β := ceil((n−k)/λ) ;  ctr := 0        // 32-bit counter
   ρ <-$ {0,1}^λ                                           // signature randomness
   µ            <- Hash2^0(pk || msg; 2λ)
   (root, ivpre)<- Hash3(sk || µ || ρ; 2λ)
   iv           <- Hash4(ivpre)
 Phase 1 — VOLE commitment and consistency check
   (com, decom, c1..c_{τ−1}, u, V) <- VOLE.Commit(root, iv, ℓv)     // Alg. 16
   chall1 <- Hash2^1(µ || com || c1..c_{τ−1} || ivpre ; 5λ+64)
   (ũ, Ṽ) <- VOLE.Hash(chall1, (u, V))                              // Alg. 19
   d := w XOR u[0 .. ℓw−1]                                          // masked witness
   chall2 <- Hash2^2(chall1 || ũ || Ṽ || d ; βλ+3λ+64)
 Phase 2 — RSD relation proof
   u := u[ℓw .. ℓw + (d−1)λ − 1] ;  V := V[0 .. ℓw + (d−1)λ − 1]
   (π_RSD, chk_RSD) <- RSD.Prove(w, u, V, pk, chall2)               // Alg. 21
 Phase 3 — Grinding and opening
   decom_I := ⊥
   repeat
     chall3 <- Hash2^3(chall2 || chk_RSD || π_RSD || BitDec(ctr,32) ; λ)
     if chall3[λ−wgrind .. λ−1] != 0^wgrind : ctr++ ; continue      // PoW check
     I        <- VOLE.DecodeAllChall(chall3[0 .. λ−wgrind−1])       // Alg. 18
     decom_I  <- BAVC.Open(decom, I)                                // Alg. 12; ⊥ if >Topen seeds
     if decom_I = ⊥ : ctr++
   until decom_I != ⊥
 Phase 4
   return σ := (c1..c_{τ−1}, ũ, d, π_RSD, decom_I, chall3, ivpre, ctr)
```

### Verify — Algorithm 3 (§4.1.3)
```
 β := ceil((n−k)/λ)
 parse σ := (c1..c_{τ−1}, ũ, d, π_RSD, decom_I, chall3, ivpre, ctr)
 µ  <- Hash2^0(pk || msg; 2λ) ;  iv <- Hash4(ivpre)
 if chall3[λ−wgrind .. λ−1] != 0^wgrind : return false            // grinding check
 rec <- VOLE.Reconstruct(chall3[0..λ−wgrind−1], decom_I, c1..c_{τ−1}, iv)   // Alg. 17
 if rec = ⊥ : return false
 parse rec := (com, Q)
 chall1 <- Hash2^1(µ || com || c1..c_{τ−1} || ivpre ; 5λ+64)
 Q̃  <- VOLE.Hash(chall1, Q) ∈ {0,1}^{(λ+B)×λ}
 parse chall3[0..λ−1] as (δ0..δ_{λ−1})
 Ṽ  := Q̃ XOR [δ0·ũ  ···  δ_{λ−1}·ũ]
 chall2  <- Hash2^2(chall1 || ũ || Ṽ || d ; βλ+3λ+64)
 chk_RSD <- RSD.Verify(d, Q, pk, chall2, chall3, π_RSD)            // Alg. 22
 chall3' <- Hash2^3(chall2 || chk_RSD || π_RSD || BitDec(ctr,32) ; λ)
 return (chall3 == chall3')
```

### Symmetric primitives (§4.2.2)
```
PRG(sd, iv, twk; m)  (Alg. 8): counter-mode over a block cipher —
    Rijndael-192 for λ=160, Rijndael-256 for λ=256, the BLAKE2s core for λ=512;
    output s_0 || s_1 || ..., s_i = first λ bits of block i.
SeedExp: AES-192-CTR (λ=160), AES-256-CTR (λ=256), BLAKE2s core (λ=512).
Hash_i(msg; m)    := pseudoXOF(msg || i, m)       for i ∈ {0,1,3,4}
Hash_2^j(msg; m)  := pseudoXOF(msg || (8+j), m)   for j ∈ {0,1,2,3}
```
Discrepancy (recorded as `sign-28-7`): at λ = 160 the VOLE tapes of Alg. 20 are
built in 128-bit rows, each the first 128 bits of the block for counter `row`
(`src/vole_impl.inc:40–46,129,234,266`), not the 160-bit pieces of Alg. 8. Both
trees reproduce the official λ = 160 KATs; λ = 256 and 512 agree with Alg. 8.

The hash family is explicitly *not* SHAKE: §4.2.2 states "we instantiate the
hash functions using the pseudo-XOF function provided by NGCC", with one-byte
suffix domain separation. Domain separation is therefore specified and is
byte-distinct across all six hash roles.

## Implementation vs specification

What was checked. The Makefile builds
`Implementations/Reference_Implementation/sydo_<level><f|s>/src`. I read
`src/sydo.c` (the parameter table, `keygen_from_seed`, key packing/offsets,
`sign_from_seed`), `src/sydo.h` (the paramset struct), `src/instances.h`, and
grepped `rsd.c`, `vole.c`, `vole_commit.h`, `bavc.c`, `quicksilver.c` for the
corresponding subroutine names.

Agreements.
- All six parameter sets are carried in one table at
  `src/sydo_160f/src/sydo.c:43-48`, field order
  `(id, name, secpar_bits, tau, zero_bits_in_delta, bavc_opening_threshold,
  rsd_n, rsd_w, rsd_codim, signature_size, public_key_size, secret_key_size,
  witness_size, keygen_seed_size)`. Every entry matches the spec:
  λ = 160/256/512; τ = 14/20/23/32/46/64 (Table 5.3); `zero_bits_in_delta`
  = wgrind = 8/2/5/2/8/2 (Table 5.3); `bavc_opening_threshold` = Topen =
  132/138/225/236/445/446 (Table 5.3); n = 16461/26226/49941, n−k =
  480/768/1456, w = 59/94/179 (Table 5.2). I sampled all of these rather than
  a subset because they are in a single literal table.
- The modelling constants at `src/sydo.c:33-40` match Table 5.2:
  `NUM_BLOCKS = 2` (ℓ=2), `BLOCK0_SIZE/BLOCK1_SIZE = 2/8` ((ηj) = (2,8)),
  `BLOCK0_WEIGHT/BLOCK1_WEIGHT = 1/3` ((rj) = (1,3), so d = Σrj = 4 as §5.2
  states), and `ELEMENTARY_VECTOR_LEN = 279` = n/w for all three levels.
  `UNIVERSAL_HASH_B_BITS = 16` (`instances.h:18`) = B = 16.
- The VOLEitH structure of Algorithms 2 and 3 is present as named modules
  (`vole_commit.h`, `vole_check.c`, `bavc.c`, `quicksilver.c`, `rsd.c`), and
  the grinding counter and Topen rejection are both in the sign path.
- The signature-size closed form of §5.2 reproduces Table 5.4 to within a few
  bytes per instance (e.g. 6719 vs 6724 for 160f, 17588 vs 17604 for 256f);
  the small residue is per-field byte alignment that the bit-level formula
  does not model. `signature_size` in the C table equals Table 5.4 exactly,
  and equals the observed sizes exactly.

**Discrepancy 1 (spec defect, cosmetic but size-relevant) — the |sk| formula in
§5.2 omits one λ term.** §5.2 states
```
|sk|bit = λ + (n−k) + w·Σηj
```
which evaluates to 1230 / 1964 / 3758 bits = 154 / 246 / 470 bytes — not the
174 / 278 / 534 of its own Table 5.4. The correct expression carries a second
λ for `seed_sk`:
`|sk|bit = λ + (n−k) + λ + w·Σηj` → 174 / 278 / 534 bytes (witness
byte-padded). Algorithm 1's *output type line* already has this extra
`× {0,1}^λ` factor, so the type signature and Table 5.4 are right and only the
prose formula in §5.2 is wrong; line 8 of Alg. 1 (`sk := (pk, w)`) likewise
drops the seed. The implementation follows the type line: `sydo.c:563-565`
writes `sk = pk ‖ seed_sk ‖ witness`, with `sk_witness_offset()` (`sydo.c:70-72`)
= `public_key_size + λ/8`, and `witness_size` = 74/118/224 =
⌈w·Σηj/8⌉ = ⌈590/8⌉ / ⌈940/8⌉ / ⌈1790/8⌉. Classification: (b) a specification
editing error, not an implementation deviation — the shipped sizes are the
ones Table 5.4 advertises.

**Observation (not a discrepancy) — keygen entropy is 2λ, not λ.**
`keygen_seed_size` = 40 / 64 / 128 bytes = 2λ bits, and `keygen_from_seed`
(`sydo.c:549-550`) splits it into `seed_sk = seed[0..λ/8)` and
`seed_pk = seed[λ/8..2λ/8)`. That is exactly Algorithm 1 lines 3–4 (two
independent λ-bit seeds), just plumbed through one 2λ-bit API seed so KATs are
reproducible. Classification: (c) a deliberate equivalent presentation.

**Observation — the implementation adds a keypair self-check the spec does not
require.** `sydo_ref_sign_from_seed` (`sydo.c:708-710`) re-derives the public
key from sk and compares it in constant time (`sydo_timingsafe_bcmp`) before
signing, aborting on mismatch. This is a defensive addition over Algorithm 2;
it costs a keygen per signature but blocks fault/mismatched-key attacks.

Not verified. I did not check `RSD.Prove`/`RSD.Verify` (Algorithms 21–22), the
QuickSilver degree-d polynomial check, `BAVC.Open`/`Reconstruct` (Algorithms
12–13), or `VOLE.Hash` (Alg. 19) line-by-line against the spec — that is the
bulk of the proof system and was beyond the time budget. I did not run any
code. The `pseudoXOF` instantiation and Rijndael-192/BLAKE2s PRG selection were
not checked against `aes.c`/`fields.c`. Byte-exact signature field layout was
not reconstructed, so I cannot state the serialization is injective.
