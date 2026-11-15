# kem-02 Amoeba — algorithm summary

RLWE-based KEM over the general cyclotomic ring `R_q = Z_q[X]/(X^{kn} - X^{kn/2} + 1)`
(m = 2^s·3^t, n = φ(m) = m/3). Hardness: non-dual Ring-LWE (spec Def. 2.1). An
IND-CPA PKE (§1.3) is lifted to an IND-CCA2 KEM by the `FO^{⊥}_{ID(pk),m}` transform
of [10] (§1.4). Message/shared-secret length is fixed at 512 bits for all five sets;
a concatenated MSB + shortened-extended-Hamming code (§2.3.2) drives the DFR down.
Nussbaumer's trick maps `R_q ≅ A_q[X]/(X^k - Y)` with `A_q = Z_q[Y]/(Y^n - Y^{n/2}+1)`
shared by all parameter sets ("Amoeba"), so one NTT module serves every instance.

Specification: `kem-02-spec.pdf` (32 pages, English), §1.3–§1.5, Algorithms 1–7.

## Parameters

Spec Table 3 (§1.5). `m = 864 = 2^5·3^3`, `n = φ(m) = 288`, `q = 3457` for all sets.

| parameter | Amoeba-576 | -864 | -1152 | -1728 | -2304 | meaning |
|---|---|---|---|---|---|---|
| q | 3457 | 3457 | 3457 | 3457 | 3457 | prime modulus, m \| q-1 |
| n | 288 | 288 | 288 | 288 | 288 | degree of sub-ring A_q |
| k | 2 | 3 | 4 | 6 | 8 | X^k = Y; RLWE dim = kn |
| kn | 576 | 864 | 1152 | 1728 | 2304 | ring dimension |
| ℓ | 523 | 523 | 523 | 523 | 523 | codeword length (Trunc target) |
| η1 | 2 | 1 | 1 | 1 | 1 | CBD param for s, s' |
| η2 | 2 | 2 | 1 | 1 | 1 | CBD param for e, e1, e2 |
| (d0,d1,d2) | (10,10,5) | (10,11,6) | (11,10,6) | (11,11,6) | (12,12,7) | ModDown widths for b, c1, c2 |
| DFR | 2^-133.3 | 2^-208.6 | 2^-267.9 | 2^-229.3 | 2^-205.3 | spec Table 4 |
| claimed security | 128.4 | 198.3 | 266.5 | 423.8 | 585.3 | bits, classical Core-SVP (Table 5) |
| (quantum) | 112.5 | 173.8 | 233.5 | 371.4 | 512.9 | bits, Core-SVP 0.2563β |

Sizes (bytes), spec Table 4 / Eqs. (4)–(6) vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | ct spec | ct impl | ss | match |
|---|---|---|---|---|---|---|---|---|
| Amoeba128 / -576 | 784 | 784 | 1712 | 1712 | 1047 | 1047 | 64 | yes |
| Amoeba192 / -864 | 1144 | 1144 | 2504 | 2504 | 1581 | 1581 | 64 | yes |
| Amoeba256 / -1152 | 1648 | 1648 | 3440 | 3440 | 1833 | 1833 | 64 | yes |
| Amoeba384 / -1728 | 2440 | 2440 | 5096 | 5096 | 2769 | 2769 | 64 | yes |
| Amoeba512 / -2304 | 3520 | 3520 | 7040 | 7040 | 3914 | 3914 | 64 | yes |

No size mismatch. Impl columns are the OBSERVED `*_get_*_len_bytes()` values; they
reproduce Eqs. (4)–(6) exactly, e.g. -576 pk = 64 + ⌈10·2·288/8⌉ = 784, ct =
⌈10·2·288/8⌉ + ⌈5·523/8⌉ = 720+327 = 1047, sk = ⌈12·2·288/8⌉+784+64 = 1712.

## Pseudocode

### PKE.KeyGen (Algorithm 5)
```
 1: ρ, σ ← {0,1}^512
 2: â := Parse(XOF(ρ))
 3: s ← ψ_η1(PRF(σ, 0x00));  e ← ψ_η2(PRF(σ, 0x01))
 4: ŝ := NussNTT(s)
 5: b := InvNussNTT(ŝ ∘ â) + e
 6: t := Pack(ModDown(b, d0))
 7: return sk := ŝ,  pk := (ρ, t)
```
### PKE.Enc (Algorithm 6) — m ∈ {0,1}^512, coins r ∈ {0,1}^512
```
 1: â := Parse(XOF(ρ));   b' := ModUp(Unpack(t), d0)
 2: s' ← ψ_η1(PRF(r,0x02)); e1 ← ψ_η2(PRF(r,0x04)); e2 ← ψ_η2(PRF(r,0x08))
 3: ŝ' := NussNTT(s');  b̂' := NussNTT(b')
 4: u := InvNussNTT(â ∘ ŝ') + e1
 5: v := InvNussNTT(b̂' ∘ ŝ') + e2 + Encode(m)
 6: c1 := Pack(ModDown(u, d1));  c2 := Pack(ModDown(Trunc(v), d2))
 7: return ct := (c1, c2)
```
### PKE.Dec (Algorithm 7)
```
 1: u' := ModUp(Unpack(c1), d1);  v' := ModUp(Unpack(c2), d2)
 2: û' := NussNTT(u');  w := InvNussNTT(ŝ ∘ û')
 3: return m' := Decode(v' - Trunc(w))
```
### KEM.Encaps / KEM.Decaps (§1.4, prose only — the spec gives NO algorithm box)
```
Encaps(pk):  m ← {0,1}^512
             (K̄, r) := G(ID(pk), m)          # G: {0,1}* → {0,1}^1024
             ct := PKE.Enc(pk, m; r)
             K  := KDF(K̄, ct);  return (ct, K)
Decaps(sk=(ŝ, pk, z), ct):
             m' := PKE.Dec(ŝ, ct)
             (K̄', r') := G(ID(pk), m')
             if PKE.Enc(pk, m'; r') = ct then K := KDF(K̄', ct)
                                         else K := KDF(z, ct)   # implicit reject
```
Sub-blocks: `NussNTT` (Alg. 1) = Nuss then n-point NTT per coefficient; `InvNussNTT`
(Alg. 2); `NussMul` â∘b̂ (Alg. 3) = per-slot schoolbook mul mod (q, X^k−Y); `Sampler`
ψ_η (Alg. 4) samples CBD β_η over Z[X]/(X^{km/2}+1) then reduces mod Φ_km. Hash/XOF/
PRF/KDF are all instantiated with SM3 (§1.4); `ID: PK → {0,1}^512` is only required to
have "sufficient min-entropy" — the spec does not define it concretely.

## Implementation vs specification

Checked: `src/Amoeba128/backend/{params.h,cpapke.c,ccakem.c,hamming.c,poly.c}`
(the five instance trees are identical copies selected by `-DSECURITY_LEVEL`).
KeyGen/Enc/Dec = `cpapke.c`; FO wrapper = `ccakem.c`; Encode/Decode = `hamming.c`
+ `encode_MSB`/`decode_MSB` in `cpapke.c`; NTT/Nussbaumer = `ntt_2s3t.c`+`zetas.c`.

Spot-checked constants (6, in `backend/params.h`): `RLWE_Q 3457`, `RLWE_N 288`,
`RLWE_M 864`, `RLWE_ECC_N 523`, `RLWE_K/ETA1/ETA2/D0/D1/D2` for all five
`SECURITY_LEVEL` branches, and `RLWE_MSG_LEN = RLWE_SEED_LEN = RLWE_KEY_LEN = 64`.
All agree with spec Table 3 and with the 512-bit m/ρ/σ/r/K of §1.3–§1.4.

Agreements: the size formulas (4)–(6) are literally `RLWE_CPA_PK_LEN`/`CT_LEN`/
`CCA_SK_LEN`; the oracle-cloning labels 0x00/0x01/0x02/0x04/0x08 appear verbatim at
`cpapke.c:198,199,217,218,219`; sk = (ŝ ‖ pk ‖ z) with |z| = 64 as in Eq. (6);
implicit-rejection key `KDF(z, ct)` is present (`ccakem.c:84-85`).

Discrepancies:

- **(a) real deviation — partial re-encryption comparison.** `ccakem.c:17-22` `cmp()`
  advances `i += 4`, so Decaps validates only bytes ≡0 mod 4 of ct. The FO predicate
  of §1.4 requires full equality. Already recorded as `KEM-FO-PARTIAL-CIPHERTEXT-
  COMPARE` (confirmed) in `security_findings.md`.
- **(a) real deviation — explicit rejection / decryption-failure oracle.**
  `cpapke.c:444` returns −1 when `decode_ECC` reports a *detected* double error, and
  `ccakem.c:60-62` propagates that as `return -1` **before** any key is derived. §1.4
  and §2.3.2 say such a ciphertext is rejected, but the FO^⊥ transform demands the
  rejection be *implicit* (`KDF(z, ct)`). The implementation therefore distinguishes
  "ECC-undecodable" from "re-encryption mismatch" to the caller. Filed as kem-02-6 in
  `report.md`.
- **(a) real deviation — KeyGen seeds.** Algorithm 5 line 1 samples ρ and σ
  independently. `cpapke.c:407` draws a single 512-bit coin, and `CPAPKE_KeyGen_raw`
  derives ρ = `pseudoXOF(coin ‖ 0x00)` and σ = `pseudoXOF(coin ‖ 0x01)`
  (`cpapke.c:186-196`).
- **(a) memory safety in Encode/Decode.** `encode_MSB`/`decode_MSB` (`cpapke.c:165-182`)
  step 4 at a time over ℓ = 523 and touch index 523. Already recorded as
  `KEM-ECC-TAIL-STACK-OVERFLOW` (confirmed).
- **(b) spec ambiguity resolved ad hoc — `ID(pk)`.** §1.4 leaves `ID` undefined;
  `ccakem.c:13-15` sets `ID(pk) := pk[64..127]`, i.e. the first 64 bytes of the packed,
  d0-compressed `t` — a truncation, not a hash. Sound-looking but unspecified.
- **(b) `G` instantiation.** §1.4 requires a 1024-bit hash; the code builds `Kr` from
  four SM3-256 calls over `m ‖ ctr ‖ ID(pk)` with a 1-byte counter placed *between*
  m and ID (`ccakem.c:41-45`). A counter-mode expansion, not a single 1024-bit hash;
  the spec does not describe this construction.
- **(c) cosmetic.** `params.h:66` defines `RLWE_ECC_R 8`, contradicting spec §2.3.2's
  r = 10. It is dead: `hamming.h:13` defines the live `ECC_R 10`, and `RLWE_ECC_N` is
  the literal 523. No effect on behaviour, but it is a misleading constant.
- **build (not a spec issue).** `backend/ntt_2s3t.c` defines `split`/`compose`/
  `radix2`/`radix3(inv)` as bare C99 `inline` with no external definition, so the
  symbols are unresolved at −O2. `kem-02/patches/common/ntt_2s3t.c` is a `static
  inline` copy used for the NGCC build (see RESULTS.md:135).

Not verified within the time box: the `zetas.c` root tables, the exact radix-2/radix-3
butterfly schedule against §2.2.2 Eqs. (1)–(3), and the `syndrome_map`/`A_trunc`
tables of the shortened extended Hamming code.
