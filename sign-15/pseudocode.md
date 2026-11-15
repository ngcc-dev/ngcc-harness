# sign-15 ⌊MORNING−ATLAS⌉ — algorithm summary

ATLAS is a Fiat–Shamir-with-aborts lattice signature over **Module-LWR**: the
public key is the *rounded* product t = ⌈(p/q)·A·s₁⌋ ∈ R_p^k (no explicit error
vector — s₂ is recovered deterministically at signing time), with power-of-two
moduli q = 2^QBITS, p = 2^PBITS so that no NTT is used (Toom-Cook/Karatsuba
instead). The structure otherwise follows Dilithium (§2.2): compressed public
key via `decompose_keygen`, hint vector h, two rejection-sampling predicates.
Claimed EUF-CMA and SUF-CMA in the (Q)ROM from MLWR + SelfTargetMSIS.

Specification: `sign-15-spec.pdf` (61 pages), §2.2 (Algorithms 1–3), §2.3
(Algorithms 4–7), §2.7 (sizes, Table 3), §4.1 (Table 2), §4.7 (NGCC API).

## Parameters

Spec Table 2 (§4.1); Λ is the spec's own quantum claim.

| parameter | ATLAS-128 | ATLAS-192 | ATLAS-256 | ATLAS-512 | meaning |
|---|---|---|---|---|---|
| n | 128 | 128 | 256 | 512 | ring degree, R = Z[x]/(x^n+1) |
| (q, p) | (2²³, 2¹⁸) | (2²³, 2¹⁹) | (2²³, 2²⁰) | (2²⁵, 2²²) | moduli (rounding q→p) |
| (k, l) | (9, 6) | (13, 10) | (8, 7) | (8, 7) | module dimensions |
| η | 16 | 8 | 4 | 4 | secret bound, s₁ ∈ S_η^l |
| κ | 31 | 69 | 60 | 60 | challenge Hamming weight |
| (β₁, β₂) | (51, 8) | (33, 12) | (73, 12) | (73, 12) | rejection bounds |
| (γ₁, γ₂, γ̄₁, γ̄₂) | (2¹⁹,2¹⁸,2¹⁴,2¹³) | (2¹⁹,2¹⁸,2¹⁵,2¹⁴) | (2¹⁹,2¹⁸,2¹⁶,2¹⁵) | (2²¹,2²⁰,2¹⁸,2¹⁷) | mask / decomposition bounds |
| pkdrop (d) | 10 | 10 | 10 | 10 | public-key truncation bits |
| ω | 128 | 128 | 128 | 128 | max hint Hamming weight |
| claimed security | 128 (80 q) | 192 (96 q) | 256 (128 q) | 512 (256 q) | bits, classical (quantum) |

Sizes (bytes), specification (Table 1 / Table 3 formulae) vs the built library:

| instance | pk spec | pk impl | sk spec | sk impl | sig spec | sig impl | match |
|---|---|---|---|---|---|---|---|
| lwrdsa128 | 1328 | 1328 | 2128 | 2128 | 2081 | 2081 | yes |
| lwrdsa192 | 2112 | 2112 | 3152 | 3152 | 3365 | 3365 | yes |
| lwrdsa256 | 2848 | 2848 | 4016 | 4016 | 4656 | 4656 | yes |
| lwrdsa512 | 6688 | 6688 | 7920 | 7920 | 10081 | 10081 | yes |

The declared lengths all match. **But `sig_sign` writes only `sig spec` bytes
while *reporting* `sig spec + |M|`** — see discrepancy 1.

Spec formulae (§2.7, Table 3), all reproduced for 128/192/256:
`|pk| = (n/8)·k·(⌈log p⌉ − pkdrop + 1) + 32`,
`|sk| = 112 + (n/8)·(l·6 + pkdrop·k)`,
`|σ| = (n/8)·l·log(2γ₁) + ω + k + n/8 + 8`.

## Pseudocode

### KeyGen (spec Algorithm 1, §2.2)
```
ρ ←$ {0,1}^256 ;  K ←$ {0,1}^256
A ∈ R_q^{k×l} := expand_mat(ρ) ;  s1 ←$ S_η^l
t  := ⌈(p/q)·A·s1⌋ ∈ R_p^k                      # rounding, not noise
(t1, t0) := decompose_keygen(t, 2^d) ;  tr := CRH(ρ ‖ t1)
pk := (ρ, t1) ;  sk := (ρ, K, tr, s1, t0)
return (pk, sk)
```

### Sign (spec Algorithm 2, §2.2) — deterministic, rejection loop
```
A := expand_mat(ρ);  t := t1·2^d + t0;  s2 := t − (p/q)·A·s1
µ := CRH(tr ‖ M);  count := 0
repeat
  count ← count + 1
  y ∈ S_{γ1−1}^l := Sam(K ‖ µ ‖ count)          # deterministic mask
  w  := ⌈(p/q)·A·y⌋ ∈ R_p^k ;  ξ1 := ⌈(p/q)Ay⌋ − (p/q)Ay
  (w1, _) := decompose_sign(w, 2γ̄2)
  c  := H(µ ‖ w1) ∈ B_κ                         # κ coeffs in {−1,+1}
  z  := y + c·s1
  ξ2 := ⌈c·s2⌋ − c·s2 ;  ν := ⌈ξ2 − ξ1⌋
  (r1, r0) := decompose_sign(w − ⌈c·s2⌋ + ν, 2γ̄2)
  h  := make_hint(−c·t0, w − ⌈c·s2⌋ + ν + c·t0, 2γ̄2)
  accept ← ¬( ‖z‖∞ ≥ γ1−β1  ∨  ‖r0‖∞ ≥ γ̄2−β2  ∨  r1 ≠ w1 )
  accept ← accept ∧ ¬( HW(h) > ω  ∨  ‖c·t0‖∞ ≥ γ̄2 )
until accept
return σ = (z, h, c)
```

### Verify (spec Algorithm 3, §2.2)
```
A := expand_mat(ρ)
µ  ← CRH( CRH(pk) ‖ M )
w1' ← use_hint( h, ⌈(p/q)·A·z⌋ − c·t1·2^pkdrop, 2γ̄2 )
c'  := H(µ ‖ w1')
return [ ‖z‖∞ < γ1 − β1  ∧  c' = c  ∧  HW(h) ≤ ω ]
```

### Supporting routines (spec Algorithms 4–7, §2.3)
```
decompose_keygen(a): a1 := (a − 1 + 2^{pkdrop−1}) >> pkdrop ;  a0 := q + a − (a1 << pkdrop)
decompose_sign(a)  : a1 := (a − 1 + γ̄2) >> bitsγ̄1 ;  a0 := p + a − (a1 << bitsγ̄1)
                     a1 &= ((p >> bitsγ̄1) − 1)
make_hint(a, b)    : v := (a+b) & (p−1);  return [ high(a) ≠ high(v) ]
use_hint(a, hint)  : (a1,a0) := decompose_sign(a & (p−1)); if hint = 0 return a1
                     mask := (p >> bitsγ̄1) − 1
                     return (a0 > p) ? (a1+1)&mask : (a1−1)&mask
```

### Hash / XOF layer (spec §4.5–4.6)
```
CRH(·) = pseudoXOF(48 bytes out)              # ICCS pseudoXOF, 48-byte digests
A: a_{i,j} := SampleUniform( pseudoXOF(ρ ‖ i ‖ j) ), rejection v < q
c: absorb µ ‖ pack(w1) into pseudoXOF; place exactly κ nonzero coefficients
   in {−1,+1} by rejection sampling without replacement; signs from the stream
```

## Implementation vs specification

What was checked: `src/lwrdsa128/params.h`, `api.h`, `SIG_lwrdsa128.c`
(`sig_keygen`, `sig_sign`, `sig_verify`, `sig_get_*_len_bytes`), `packing.c`
(`pack_sig` / `unpack_sig`), `rounding.c`; plus `params.h`/`api.h` of
`lwrdsa192`, `lwrdsa256`, `lwrdsa512`. Each instance dir picks its set with a
`MODE` switch (0/1/2/3).

Agreements:
- **Parameter spot-check, all four instances** (n, k, l, q=2^QBITS, p=2^PBITS,
  η, κ, β₁, β₂, ω, d): every constant in the selected `MODE` block matches
  Table 2 **except κ for ATLAS-192** (discrepancy 2). η is not a literal but
  `ETA = 1<<(QBITS−PBITS−1)`, giving 16/8/4/4 — matches Table 2. Likewise
  `GAMMA1_BITS = QBITS−4` and `GAMMA1_BAR_BITS = GAMMA1_BITS+PBITS−QBITS`
  reproduce all four (γ₁, γ₂, γ̄₁, γ̄₂) rows exactly, including the 512 set
  (q = 2²⁵, p = 2²², γ₁ = 2²¹, γ̄₁ = 2¹⁸).
- `CRYPTO_PUBLICKEYBYTES / CRYPTO_SECRETKEYBYTES / CRYPTO_BYTES` in each
  `api.h`/`params.h` equal the Table 1 values, and `PK_SIZE_PACKED`,
  `SK_SIZE_PACKED`, `SIG_SIZE_PACKED` (`params.h:105-109`) reproduce the
  Table 3 formulae.
- The signing loop in `SIG_lwrdsa128.c` follows Algorithm 2 line for line,
  including both rejection predicates and the ν = ⌈ξ₂−ξ₁⌋ compensation, except
  for the mask step: the nonce starts at 0 and advances once per polynomial
  (`:273,327`) where Algorithm 2 increments `count` once per attempt, and the
  mask sampler it calls has the defects recorded as sign-15-4 (ATLAS-128/-192/-256)
  and sign-15-13 (ATLAS-512);
  `rounding.c` implements Algorithms 4–7 as written (bit-mask forms included).

Discrepancies:

1. **(a) real deviation — `sig_sign` overruns its own declared length (known
   finding: `security_findings.md` reports OVERFLOW on all four instances,
   2137 > 2081, 3421 > 3365, 4712 > 4656, 10137 > 10081; the excess is exactly
   the 56-byte KAT message, `KAT_SIG.c:56`).**
   *What the spec says.* §2.2 and Algorithm 2 line 27 define the signature as
   `σ = (z, h, c)`; §2.7.2 and Table 3 give its size as
   `(n/8)·l·log(2γ₁) + ω + k + n/8 + 8` bytes, and Table 1 fixes the four
   values 2081 / 3365 / 4656 / 10081. Separately, §4.7 ("NGCC API
   Implementation") states that `sig_sign` "outputs a signed message `sm`,
   which is the concatenation of the packed signature and the original
   message" — i.e. the spec itself describes a SUPERCOP-style `σ‖M` output
   while declaring the signature length as `|σ|` alone. Those two statements
   are incompatible under the NGCC API, where the caller sizes the output
   buffer from `sig_get_sn_len_bytes()`.
   *What the code does.* `SIG_lwrdsa128.c:33-35` returns `CRYPTO_BYTES`
   (= 2081) from `sig_get_sn_len_bytes()`. `sig_sign` writes exactly
   `SIG_SIZE_PACKED = CRYPTO_BYTES` bytes via `pack_sig(sn, &z, &h, &c)`
   (`:426`) and then sets
   `*sn_len_bytes = m_len_bytes + CRYPTO_BYTES;` (`:428`).
   **The message is never copied into `sn`** — the only message copy in
   `sig_sign` is into the local CRH scratch buffer `buf` (`:288-291`, whose
   comment "Copy message at the end of the sn buffer" is stale). So the
   function reports `|M|` more bytes than it wrote, into a buffer the caller
   allocated at `sig_get_sn_len_bytes()`; the candidate's own driver
   (`KAT_SIG.c:97-100,136,144`) then reads and prints `sn_len_bytes` bytes from
   a 2081-byte `calloc` — a heap over-read, and the tail of every KAT `Sn`
   field is uninitialised memory rather than the message.
   Same code in all four instances (`SIG_lwrdsa192.c` / `256` / `512`).
2. **(a) real deviation — ATLAS-192 challenge weight.** Table 2 specifies
   κ = 69 for ATLAS-192; `src/lwrdsa192/params.h:68` sets `#define KAPPA 64U`
   with the in-source comment *"KAPPA should be 69 not 64, due to limitation in
   the implementation, we are temperorily using 64"*. The challenge space
   C(n,κ)·2^κ is therefore smaller than the set the §3.3 security estimate is
   computed over. All other ATLAS-192 constants match. `challenge()` reads its
   position bytes from one 136-byte `pseudoXOF` output and, when that is used
   up, repeats the same call and restarts at position 0
   (`SIG_lwrdsa192.c:138-149`); sign-15-3 records that this happens in nearly
   every ATLAS-192 challenge.
3. **(a) real deviation — verification omits the hint checks of Algorithm 3.**
   Algorithm 3 line 5 requires `HW(h) ≤ ω`. `sig_verify` checks only
   `‖z‖∞ < γ1 − β1` (`SIG_lwrdsa128.c:470`) and `c' = c`; it never tests the
   hint weight, and `unpack_sig` (`packing.c:208-230`) is `void` and validates
   nothing: the per-polynomial cumulative counter `sig[OMEGA+i]` is used
   unchecked as a loop bound (`packing.c:222-226`), so a crafted signature with
   `sig[OMEGA+i] > OMEGA+K+N/8+8` makes the decoder read past the end of the
   signature buffer. (Dilithium's reference `unpack_sig` returns an error for
   exactly these cases.) Consequence: malformed-signature memory unsafety and
   loss of the canonical-encoding property the SUF-CMA claim of §1.1/§3.4
   depends on.
4. **(a) real deviation — `sig_verify` reads past the signature.** After a
   successful check it executes `for(i=0;i<m_len_bytes;++i) m[i] =
   sn[CRYPTO_BYTES + i];` (`SIG_lwrdsa128.c:525-526`), recovering the message
   from bytes `sig_sign` never wrote. It also only tests
   `sn_len_bytes < CRYPTO_BYTES` (`:462`), never that the length matches.
5. **(b) spec-internal inconsistency — 512-level signature formula.** §2.7.2 /
   Table 3 give `|σ| = (n/8)·l·log(2γ₁) + ω·log(n)/8 + k + n/8 + 8` for the
   512 level; substituting n=512, l=7, γ₁=2²¹, ω=128, k=8 yields **10080**,
   while Table 1 and the implementation both say **10081**. The extra byte is
   the `+ 1` in `src/lwrdsa512/params.h:165`
   (`SIG_SIZE_PACKED = L*POLZ_SIZE_PACKED + ((9*(OMEGA>>3)) + K + 1) + (N/8+8)`),
   i.e. the 512 level uses a 9-bits-per-index hint encoding (n = 512 > 256)
   plus one extra byte. The implementation is self-consistent; the spec's
   formula is off by one against its own table.
6. **(c) deliberate equivalent.** Algorithm 1 line 1 samples ρ and K
   independently; the implementation draws 32 bytes from the NGCC
   `drng_algorithm` and expands them with `pseudoXOF` into (ρ, ρ′, K)
   (`SIG_lwrdsa128.c:191-203`), sampling s₁ from ρ′. This is the standard
   Dilithium seed-expansion and is indistinguishable in the ROM, but it is not
   what Algorithm 1 states. For ATLAS-512 the single 256-bit root is recorded
   as sign-15-10.
7. **(a) real deviation — ATLAS-512 matrix expansion (sign-15-18).** §4.6.1
   derives `a_{i,j}` from `pseudoXOF(ρ ‖ i ‖ j)` with candidates accepted when
   below q. `expand_mat` appends the single byte `i + (j << 4)`
   (`SIG_lwrdsa128.c:64`) and builds each candidate from three bytes; in
   `SIG_lwrdsa512.c:73-76` the 24-bit value is masked with `0x1FFFFFF` and
   compared with q = 2²⁵, so every coefficient is below 2²⁴.

Not verified: the Toom-Cook/Karatsuba multiplication chain in `poly.c`
(§4.3 / Appendix B) against the spec's decomposition schedule, and the §2.5
failure-probability accounting.
