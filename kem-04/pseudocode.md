# kem-04 BAG-Piglet — algorithm summary

Rank-metric code-based KEM. An IND-CPA PKE (BAG-Piglet.PKE) built from ideal
(3,1)/(4,2)-block rank syndrome-decoding / rank-support-learning assumptions with
*disjoint-support block errors*, with an **augmented Gabidulin code** as the auxiliary
decodable code; the salted Fujisaki–Okamoto (SFO) transform with implicit rejection
gives the IND-CCA2 KEM (spec §1.1, §1.3, §1.4).

Specification: `kem-04-spec.pdf` (25 pages, English; Alg. 1 = PKE p.5, Alg. 2 = KEM p.6,
decoder §2.3 pp.10–11, parameters Tables 1–3 pp.7). The spec does **not** mention
BAG-Loong (kem-03); the only stated ancestor is Piglet-1 (§2.4, ref. [14]), over which
BAG-Piglet claims a vector (not matrix) public key `h`, 12 instead of 18 field
multiplications per encryption, disjoint block errors, and pk+ct of 6mn/8 instead of 8mn/8.

## Parameters

| parameter | 128 | 256 | 384 | 512 | meaning |
|---|---|---|---|---|---|
| q | 2 | 2 | 2 | 2 | base field |
| n | 47* | 78 | 106 | 125 | deg P(X), ring R = F_{q^m}[X]/(P) |
| m | 43* | 79 | 103 | 131 | field F_{q^m}, deg f(X) |
| n' | 43 | 78 | 103 | 130 | Gabidulin support length |
| n1 | 2 | 2 | 2 | 2 | blocks (code length nn1) |
| k | 3 | 4 | 4 | 4 | message dimension |
| ε | 30 | 54 | 77 | 92 | augmentation / tail-support dim |
| w(x,y) | (4,4,4) | (6,6,6) | (7,7,7) | (8,8,8) | key-error rank weights |
| w(r1,R2,e) | (2,3,3,3) | (3,3,4,4) | (4,4,4,4) | (4,4,5,5) | encryption rank weights |
| δ = ⌊(n'−k+ε)/2⌋ | 35 | 64 | 88 | 109 | decoding radius (§1.2) |
| claimed security | 128 | 256 | 384 | 512 | classical bits (80/128/192/256 quantum) |
| claimed DFR | 2^−130 | 2^−273 | 2^−394 | 2^−520 | Table 5 |

\* Table 1's `n` and `m` columns are **swapped in the 128 row only**: Table 2 gives
deg P = 43 and deg f = 47, and `parameters.h` has `PARAM_N 43`, `PARAM_M 47`. The
other three rows agree with Table 2 and the code. Sizes are unaffected (formula is 2mn).

Sizes (bytes), specification (Table 3) vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | ct spec | ct impl | ss spec | ss impl | match |
|---|---|---|---|---|---|---|---|---|---|
| bag_piglet_128 | 522 | 522 | 16 | 16 | 1011 | 1027 | 16 | 16 | ct +16 |
| bag_piglet_256 | 1573 | 1573 | 32 | 32 | 3081 | 3097 | 32 | 32 | ct +16 |
| bag_piglet_384 | 2778 | 2778 | 48 | 48 | 5459 | 5475 | 48 | 48 | ct +16 |
| bag_piglet_512 | 4158 | 4158 | 64 | 64 | 8188 | 8204 | 64 | 64 | ct +16 |

pk = ⌈2mn/8⌉ + λ/8 and ct = ⌈2mnn1/8⌉ (§1.5) both reproduce exactly; the +16 is the
128-bit `salt`, which Alg. 2 returns as a *separate* output but the code appends to `ct`.

## Pseudocode

### PKE.KeyGen (Algorithm 1)
```
ρ, σ ←$ {0,1}^λ
h = (h1,h2) ∈ R^2 := shake256(ρ)
(x, y=(y1,y2)) ∈ R × R^2 ← S_a^{3n}(F_{2^m}) := shake256(σ)   # disjoint block supports
s = (s1,s2) := x·h + y  mod P
pk := (ρ, s) ;  sk := x
```
### PKE.Enc(pk=(ρ,s), m ∈ F_{q^m}^k, θ)  (Algorithm 1)
```
h := shake256(ρ);  G := generator of augmented Gabidulin code G+_g(nn1, n', k, m)   # g also from ρ
(r1, e, R2) ∈ R^{n1} × R^{n1} × R^{n1×2} ← S_b^{4nn1}(F_{q^m}) := shake256(θ)
u := h·R2^T + r1
v := s·R2^T + e + Fold(m·G)
return c := (u, v)
```
### PKE.Dec(sk=x, c=(u,v))  (Algorithm 1)
```
return m := D_{G+_g}( Unfold(v − x·u) )      # over F_2, v − x·u = v + x·u
# correctness: v − x·u = Fold(mG) + y·R2^T + e − x·r1^T, and
#   wR(y·R2^T + e − x·r1^T) = Σ_i w_{yi}·w_{R2,i} + w_e + w_x·w_{r1} ≤ δ = ⌊(n'−k+ε)/2⌋
```
### Rank-metric decoder D_{G+_g} (spec §2.3, "new fast decoding algorithm")
```
input ȳ ∈ F_{q^m}^{nn1}, with c̄ = xG ∈ G+_g, w_R(ē) ≤ δ
1. split ȳ = (y | tail):  y = first n' coords, tail = last nn1−n' coords
2. by construction c̄ = (* | 0...0), so tail is exactly the last nn1−n' coords of ē
3. E2 := Supp(tail), dim E2 = ε ; V2 := unique monic q-linearized annihilator of E2
4. z_i := V2(y_i), 1 ≤ i ≤ n'                       # V2(y_i) = V2∘f(g_i) + V2(e_i)
5. (V2∘f(g_1),…,V2∘f(g_n')) is a codeword of the ordinary Gabidulin code G(g, n', k+ε),
   which has d' = n'−(k+ε)+1 and corrects up to δ−ε = ⌊(n'−k−ε)/2⌋ rank errors;
   decode z with it
6. recover f from V2∘f by left Euclidean division by V2; output its k coefficients
   complexity O(n^2) over F_{q^m} (Thm. 2.1); DFR = q^{δ(n'−nn1)}·Π… (§1.2)
```
### KEM (Algorithm 2), G = SHA3-512, K = SHAKE256, ID(·) = pk entropy extraction
```
KeyGen: (pk',sk') ← PKE.KeyGen; ξ ←$ {0,1}^512;  pk := pk', sk := (sk', pk', ξ)
Encap(pk): m ←$ F_{q^m}^k, salt ←$ {0,1}^128
           (k,θ) ← G(ID(pk), m, salt);  ct ← PKE.Enc(pk,m,θ);  K ← K(k, ct)
           return (ct, K, salt)
Decap(sk,ct,salt): m' ← PKE.Dec(sk',ct);  (k',θ') ← G(ID(pk'), m', salt)
           ct' ← PKE.Enc(pk', m', θ')
           if ct' = ct: K' ← K(k', ct')  else: K' ← K(ξ, ct')      # implicit rejection
```

## Implementation vs specification

Checked (reference implementation built by `kem-04/Makefile`): `src/common/ccakem.c`
(SFO transform), `src/scheme/bag_piglet.c` (PKE keygen/enc/dec), `src/common/augabidulin.c`
+ `gabidulin.c` + `q_polynomial.c` (decoder), `src/scheme/parameters.h`, `lib/api_pkc/auxfunc.c`.

Agreements. Parameter spot-check, all four instances, on `PARAM_N/PARAM_M/PARAM_N_/PARAM_K/
PARAM_epsilon/PARAM_N_1` and the seven `WEIGHT_*_VALUES`: every value matches Table 1
(modulo the 128-row n/m swap above). The weight budget is tight and exact — 128:
4·3+4·3+3+4·2 = 35 = δ; 512: 8·4+8·5+5+8·4 = 109 = δ. The decoder follows §2.3 step for
step: `augabidulin.c:69-140` extracts the tail, calls `ffi_vec_recover_part_error_support`
(`src/ffi/ffi_vec.c:206`) for a rank-ε basis of E2, interpolates V2
(`q_polynomial_set_interpolate`), applies V2 to the first n' coordinates, and decodes with
an inner Gabidulin code of dimension `PARAM_K_Gabidulin = k+ε` and radius
`PARAM_E_Gabidulin = (n'−k−ε)/2` (= δ−ε, matching Lemma 2.1) via `gabidulin_code_decode_3`.
Disjoint block supports are realised by slicing one rank-`WEIGHT_TOTAL_i` support into
non-overlapping segments (`bag_piglet.c:82-115`, `:202-256`). The FO re-encryption,
constant-time compare and masked fallback select are present (`ccakem.c:76-143`), and the
session key is derived from the *re-encrypted* `ct'` in both branches, exactly as Alg. 2 says.

Discrepancies.
- **(a) symmetric primitives.** The spec fixes `shake256` as PRG, G = SHA3-512,
  K = SHAKE256, and §4 claims an NTL/GMP/**OpenSSL SHAKE** C++ implementation. The
  shipped code is plain C and instantiates *every* one of these with an SM3-based KDF XOF
  `pseudoXOF` (`lib/api_pkc/auxfunc.c:482`), wrapped as `sm3_xof` in `ccakem.c:16` and
  `bag_piglet.c:17`. Function names still say `..._using_shake`. No SHA-3 anywhere.
- **(a) ciphertext/API shape.** `salt` is concatenated into `ct` (`ccakem.c:112`,
  `CCAKEM_CT_SIZE = CPAPKE_CT_SIZE + PARAMS_SALT_SIZE`), so the observed ct is Table 3's
  value +16 at every level. Alg. 2 returns salt separately; Table 3 omits it.
- **(b) hash output lengths.** Spec declares G, K : {0,1}* → {0,1}^512 and ξ ∈ {0,1}^512.
  The code uses |G output| = 2·λ/8, |K output| = λ/8 and |ξ| = λ/8 (16/32/48/64 bytes) —
  i.e. only the 512-bit instance matches the stated 512 bits.
- **(b/c) secret key.** Spec sk = (sk', pk', ξ); the implementation stores only a λ/8-byte
  master seed and re-derives (pke_seed ‖ ξ) = XOF(sk‖0x01), then *re-runs the whole PKE
  keygen on every decapsulation* (`ccakem.c:117-125`) to recover pk' and x. Functionally
  equivalent, but it explains the ~4× decaps cost and is not the spec's key format.
- **(c) domain separation.** Four domain bytes 0x01–0x04 are appended to every XOF input
  (`ccakem.c:11-14`); the spec specifies no such labels.
- **(c) two seeds.** Spec samples ρ, σ independently; the code derives sk_seed‖pk_seed
  from one `coin` via the XOF (`bag_piglet.c:56-59`).
- Cosmetic: `src/scheme/api.h` of the 128 instance names the algorithm `BAG-PIGLET-128`
  while the other three use `BAG_PIGLET-<lvl>`.

Not verified: the DFR values of Table 5, the attack cost estimates (§3.2–3.3), and
byte-level packing in `parsing.c`. The decoder is not constant time; see
`constant_time.md` and kem-04-1. KAT PASS for all
four instances (`RESULTS.md:373-376`).
