# kem-23 Mito — algorithm summary

Code-based KEM in the HQC lineage: an IND-CPA PKE whose security rests on the
**Quasi-Dyadic Syndrome Decoding (QDSD)** problem over a 2^ℓ-adic algebra
(`Adic(h)` is the 2^ℓ-adic matrix of a vector sequence; products are polynomial
products in the base ring `R_0 = F_2[x]/(x^n − 1)` computed by recursive Karatsuba).
The message is protected by a concatenated **Reed–Solomon ∘ duplicated Reed–Muller
RM(1,7)** code; the KEM is an FO transform with **implicit rejection** and a salt.

Specification: `kem-23-spec.pdf` (56 pages), §3 "Algorithm Specification"
(Algorithms 5–10, pp. 26–31); parameters Tables 3.5–3.8 (pp. 32–33);
DFR analysis §4.2; IND-CCA §4.3.2.

## Parameters

`PARAM_L` in the code is `2^ℓ`, not ℓ. Constants below are `parameters.h`, verified
against the spec tables for all three families at λ = 128 and spot-checked at 256/512.

| parameter | Mito1-128 | Mito1-256 | Mito1-512 | Mito1-E-128 | Mito1-E-256 | Mito1-E-512 | Mito2-E-128 | Mito2-E-256 | Mito2-E-512 | meaning |
|---|---|---|---|---|---|---|---|---|---|---|
| ℓ (2^ℓ = PARAM_L) | 1 | 1 | 1 | 1 | 1 | 1 | 2 | 2 | 2 | adic number |
| n | 15373 | 33827 | 106261 | 14621 | 32261 | 102437 | 10253 | 21523 | 65293 | base-ring dimension |
| n′ = N_out·n_in | 15360 | 33792 | 106240 | 14592 | 32256 | 102400 | 10240 | 21504 | 65280 | concatenated code length |
| n_in | 384 | 384 | 640 | 384 | 384 | 640 | 256 | 256 | 384 | duplicated RM(1,7) length |
| N_out | 40 | 88 | 166 | 38 | 84 | 160 | 40 | 84 | 170 | RS length |
| k = K_out (bytes) | 16 | 32 | 64 | 16 | 32 | 64 | 16 | 32 | 64 | message length = λ/8 |
| (w_x, w_y) | 44,41,47,40 | 65,63,72,69 | 127,126,143,131 | 44,41,47,40 | 69,64,72,64 | 128,123,140,136 | (w1) 25,25,22,21 | 38,38,33,33 | 72,72,66,62 | key weights |
| (w_r, w_t) | 42,45,44,45 | 63,66,71,74 | 123,134,137,138 | 42,45,44,45 | 63,73,66,72 | 126,129,136,141 | (w2) 22,23,25,25 | 34,34,38,38 | 65,66,70,73 | enc. weights |
| w_e | 85 | 133 | 262 | 86 | 132 | 261 | 39 | 68 | 135 | error weight |
| E (erasure switch) | 0 | 0 | 0 | 1 | 1 | 1 | 1 | 1 | 1 | spec parm |
| claimed DFR (log2) | −173.34 | −272.79 | −516.22 | −167.99 | −265.48 | −517.00 | −181.39 | −269.97 | −547.37 | spec Tables 3.5–3.7 |
| claimed security λ / λ_Q | 128/80 | 256/128 | 512/256 | 128/80 | 256/128 | 512/256 | 128/80 | 256/128 | 512/256 | bits |

λ_seed = 64 B, λ_salt = 32 B, shared secret = 64 B for every instance.

Sizes (bytes), specification Table 3.8 vs the built reference library (OBSERVED):

| instance | ek spec | pk impl | dk spec | sk impl | ct spec | ct impl | ss impl | match |
|---|---|---|---|---|---|---|---|---|
| Mito1-128 | 3908 | 3908 | 4052 | 4052 | 5796 | 5796 | 64 | yes |
| Mito1-256 | 8522 | 8522 | 8682 | 8682 | 12714 | 12714 | 64 | yes |
| Mito1-512 | 26630 | 26630 | 26822 | 26822 | 39878 | 39878 | 64 | yes |
| Mito1-E-128 | 3720 | 3720 | 3864 | 3864 | 5512 | 5512 | 64 | yes |
| Mito1-E-256 | 8130 | 8130 | 8290 | 8290 | 12130 | 12130 | 64 | yes |
| Mito1-E-512 | 25674 | 25674 | 25866 | 25866 | 38442 | 38442 | 64 | yes |
| Mito2-E-128 | 5192 | 5192 | 5336 | 5336 | 6440 | 6440 | 64 | yes |
| Mito2-E-256 | 10828 | 10828 | 10988 | 10988 | 13484 | 13484 | 64 | yes |
| Mito2-E-512 | 32712 | 32712 | 32904 | 32904 | 40840 | 40840 | 64 | yes |

All nine KATs reproduce (PASS ×9).

## Pseudocode

### PKE.KeyGen (spec Algorithm 5) / KEM.KeyGen (Algorithm 8)
```
PKE.KeyGen(seed_PKE):
 1  (seed_sk, seed_pk) <- H_seed(seed_PKE)                 # 2*lambda_seed bytes
 2  state_sk <- XOFInit(seed_sk)
 3-7  for i in 0..2^l-1: y_i <- SampleDisWt(state_sk, w_y[i])
 8-12 for i in 0..2^l-1: x_i <- SampleDisWt(state_sk, w_x[i])
13    state_pk <- XOFInit(seed_pk)
14-16 for i in 0..2^l-1: h_i <- SampleUnif(state_pk)
17    s <- x + y * Adic(h)
18-19 sk <- seed_sk ;  pk <- (seed_pk, s)

KEM.KeyGen():
 2  seed_KEM <- $ B^{lambda_seed}
 3-5 state <- XOFInit(seed_KEM); seed_PKE <- XOFBytes(state); sigma <- XOFBytes(state)
 6  (pk, sk) <- PKE.KeyGen(seed_PKE)
 7-8 ek <- pk ;  dk <- (ek, sk, sigma)
```

### PKE.Encrypt (Algorithm 6) / KEM.Encaps (Algorithm 9)
```
PKE.Encrypt(pk=(seed_pk,s), m, theta):
 2-5  h_i <- SampleUnif(XOFInit(seed_pk))          # regenerate h
 6    state_theta <- XOFInit(theta)
 7-11 r_i <- SampleDisWt(state_theta, w_r[i])
12-16 t_i <- SampleDisWt(state_theta, w_t[i])
17-18 e   <- SampleDisWt(state_theta, w_e)
19    u <- r + t * Adic(h)
20    v <- Encode(m) + Trun( e + Pr(s * Adic(t)) )
21    c <- (u, v)

KEM.Encaps(ek):
 1-2 m <- $ B^k ;  salt <- $ B^{lambda_salt}
 3   (K, theta) <- H_gen( H_comp(ek) || m || salt )
 4   c  <- PKE.Encrypt(ek, m, theta)
 5   ct <- (c, salt);  return (K, ct)
```

### PKE.Decrypt (Algorithm 7) / KEM.Decaps (Algorithm 10)
```
PKE.Decrypt(sk=seed_sk, c=(u,v)):
 3-8  regenerate y from XOFInit(seed_sk) with weights w_y
 9    m <- Decode( E,  v - Trun( Pr(u * Adic(y)) ) )

KEM.Decaps(dk=(ek,sk,sigma), ct=(c,salt)):
 3  m'       <- PKE.Decrypt(sk, c)                       # may be _|_
 4  (K',th') <- H_gen( H_comp(ek) || m' || salt )
 5  c'       <- PKE.Encrypt(ek, m', th')
 6  Kbar     <- H_gen( H_comp(ek) || sigma || ct )
 7-8 if m' = _|_ or c' != c:  K' <- Kbar                 # constant time
 9  return K'
```

### Decoding step (the correctness / DFR boundary)
```
Decode(E, w):                       # w = noisy concatenated codeword, n' bits
  for each of the N_out inner blocks:
      expand_and_sum the MULTIPLICITY = n_in/128 copies
      Hadamard transform of the RM(1,7) block; find the two best peaks
      byte_i <- peak[0]
      if E = 1: if dist(peak[1]) - dist(peak[0]) <= alpha, mark block i as ERASED
  RS decode the N_out bytes over GF(2^8), g(x) of degree PARAM_G-1 = 2*delta:
      syndromes -> Berlekamp-Massey ELP -> roots (additive FFT) -> Forney values
  E = 0 : succeeds while  sigma <= delta = floor((N_out - K_out)/2)
  E = 1 : spec Thm 4.2.2 -- succeeds while  2*sigma + t <= N_out - K_out
```
Hashes are the official ICCS `pseudohash` (SM3-based): H_seed = `hash_i`
(1024-bit), H_comp = `hash_h` (512-bit), H_gen = `hash_g` (1024-bit),
H_mask = `hash_j` (512-bit). The XOF is the official DRNG in "expand a seed" mode
(`init_random_number` / `get_random_number`).

## Implementation vs specification

Built tree: `Implementations/Reference_Implementation/Mito-*`, all nine directories.
`parameters.h` differs in every instance. Of the other `.c`/`.h` files, all are
byte-identical across the nine instances except three, in which Mito-1-128 alone
differs from the other eight: `KAT_KEM.c` and `KEM_AlgorithmInstance.c` by a
trailing newline, and `symmetric.h` by one extra `#include "parameters.h"`
(verified by `md5sum`). None of the three differences changes code, so one code
review covers all nine.
`Optimized_Implementation/` is byte-identical to `Reference_Implementation/` for all
nine instances (`diff -rq`).

Agreements:

- All sampled parameter constants match the spec tables exactly: `PARAM_N`,
  `PARAM_N1` (= N_out), `PARAM_N2` (= n_in), `PARAM_OMEGA`, `PARAM_OMEGA_R`,
  `PARAM_OMEGA_E`, `PARAM_L = 2^ℓ`, for Mito1-128/256/512, Mito1-E-128 and
  Mito2-E-128/256. `PARAM_K_BYTES = lambda/8` matches k ∈ {16,32,64}.
- Sizes are derived from the parameters, not hard-wired
  (`parameters.h:199-201`), and evaluate to exactly Table 3.8.
- FO structure is present and correctly oriented (`mito_kem.c:111-157`):
  `mito_pke_decrypt` → `hash_g` → re-encrypt → `vect_compare` over `u`, `v`, `salt`
  → `result -= 1` yields the mask 0xFF (equal) / 0x00 (differ), and
  `ss[i] = (ss[i] & result) ^ (Kbar[i] & ~result)`. `vect_compare`
  (`vector.c:185-193`) returns strictly 0 or 1, so the OR-accumulated `result` is
  always 0 or 1 and the `-1` mask trick is safe. Condition sense is correct
  (not inverted).
- `Kbar = hash_j(H_comp(ek), sigma, ct)` binds the whole ciphertext including the
  salt (`symmetric.c:117-130`), as Algorithm 10 line 6 requires.
- Randomness for `seed_kem`, `m` and `salt` comes only from the official DRNG via
  `prng_get_bytes` → `get_random_number(&drng_algorithm, …)` (`symmetric.c:39-41`).
- Encrypt/Decrypt arithmetic follows Algorithms 6/7 literally, including adding `e`
  *before* `Trun` (`mito_pke.c:110-112`) and the weight-array split
  `&PARAM_OMEGA[0]` for x / `&PARAM_OMEGA[4-PARAM_L]` for y (and the same for
  r/t), which reproduces the spec's `(w_x, w_y)` and `(w_r, w_t)` ordering, and
  degenerates correctly to `w_x = w_y = w1` when 2^ℓ = 4.

Discrepancies:

- **(a) Deviation — the distance-informed erasure decoder (E = 1) is not
  implemented.** `code.c:34-41` is:
  `t = reed_muller_decode(tmp, pos, em); reed_solomon_decode(m, tmp);`
  The erasure count `t` and the erasure positions `pos` are computed
  (`reed_muller.c:242-248`, the `#else` branch that uses `PARAM_ALPHA`) and then
  **discarded**. `reed_solomon_decode` (`reed_solomon.c`) is the plain HQC
  errors-only Berlekamp–Massey decoder with the two-argument signature
  `void reed_solomon_decode(uint64_t *msg, uint64_t *cdw)` — it has no erasure
  input at all. Consequently the six instances whose spec parameter `E = 1`
  (Mito1-E-128/256/512, Mito2-E-128/256/512) run errors-only decoding
  (`2σ ≤ N_out − K_out`) while their parameters and their claimed DFRs
  (2^−167.99 … 2^−547.37) are derived in spec §4.2.2 / Theorem 4.2.2 from the
  erasure bound `2σ + t ≤ N_out − K_out`. The claimed DFR of all six E-instances is
  therefore not the DFR of the shipped code, and the DFR is the assumption the
  IND-CCA proof of the FO transform is conditioned on (spec §4.3.2). The same
  holds in `Optimized_Implementation/`. The spec's `Decode(E, ·)` interface and the
  parameter `E` have no counterpart anywhere in the source.
- **(a) Deviation — σ is k bytes, not λ_seed bytes.** Spec Algorithm 8 line 5 and
  Table 3.3 both say `σ ∈ B^{λ_seed}` = 64 bytes; `mito_kem.c:31,43` samples
  `sigma[PARAM_K_BYTES]` = 16 bytes at λ = 128 (32 at 256, 64 at 512). The spec's
  own size formula `|dk| = |ek| + 2λ_seed + k` (§3.4.2) implies the k-byte version,
  so the specification is internally inconsistent and the code follows the size
  formula. Effect: the implicit-rejection secret carries only λ bits of entropy —
  adequate at the claimed level, but not what Algorithm 8 states.
- **(b/c) The `m' = ⊥` branch of Algorithm 10 line 7 is unreachable.**
  `mito_pke_decrypt` (`mito_pke.c:128-151`) unconditionally `return 0;` — the RS
  decoder never reports failure. `crypto_kem_dec` seeds `result` from that return
  value (`mito_kem.c:134`), so the term is always 0 and only the re-encryption
  comparison drives rejection. This is behaviourally equivalent (a decoding failure
  yields a wrong `m'` and hence `c' ≠ c`), but it means decoder-failure detection is
  absent as an independent check.
- **(c) Latent offset bug, currently harmless.** `crypto_kem_enc` takes
  `theta = K_theta + SEED_BYTES` (`mito_kem.c:90`) whereas `crypto_kem_dec` takes
  `theta_prime = K_theta_prime + SHARED_SECRET_BYTES` (`mito_kem.c:140`). The two
  agree only because `SEED_BYTES == SHARED_SECRET_BYTES == 64` for every instance.
- **(c) `prng_init()` is dead code.** `symmetric.c:23-30` would reseed the shared
  `drng_algorithm` via `init_random_number()`, and the doc comments on
  `crypto_kem_keypair`/`crypto_kem_enc` declare it a precondition
  (`mito_kem.c:22,69`), but nothing in the submission calls it — the only
  references are those comments and the prototype in `symmetric.h:15`. The KEM
  therefore relies entirely on the harness/caller having seeded `drng_algorithm`.
  Benign under the NGCC KAT harness (which seeds it), but a caller following the
  doxygen would have to reach into an unused function, and the `main_mito.c`
  it points at (`syscall(SYS_getrandom, …)`) is not in the submitted tree.
- **(c) Domain separation is length-based only.** §3.1.2 claims the four hash roles
  are "rigorously instantiated via domain separation". In `symmetric.c` there is no
  domain tag: H_seed and H_gen are both `pseudohash(1024, …)` and H_comp and H_mask
  are both `pseudohash(512, …)`; they are distinguished only by their fixed,
  differing input lengths. (The 512/1024 split *is* separated inside `pseudohash`
  itself, which prefixes `0x0200` vs `0x0400`.) No practical collision follows from
  the fixed lengths, but the claim is stronger than the code.
- **(c)** The re-encryption check compares `salt` against a copy of itself
  (`mito_kem.c:143,150`), so that one term of the comparison is a no-op. Harmless:
  the salt is already bound through `hash_g` and `hash_j`.

Not verified: the ISD work factors, the DFR numbers themselves, and the Karatsuba
`vect_mul` adic-product identities beyond the fact that they reproduce the KATs.
