# kem-14 DTRU — algorithm summary

NTRU-style KEM ("dual" in the sense that security rests on *both* the NTRU
assumption for h = g/f and Ring-LWE for the ciphertext). The message is first
mapped into a **scaled double-E8 lattice code** (two interleaved copies of the
E8 Reed–Muller [8,4] code, one per half of the ring), so decryption is a
minimum-distance decode rather than a coefficient-wise rounding; the ciphertext
is then compressed from modulus q to q2. IND-CCA comes from the
`FO^{not-perp}_{ID(pk),m}` transform of [DHK+21] — implicit rejection with the
public key bound into the hash.

Specification: `kem-14-spec.pdf` (23 pages), §1.1 (Algorithms 1–2, Encode /
Decode), §1.2 Fig. 1 (PKE), §1.3 Fig. 2 (KEM), §1.4 Tables 1–2.

## Parameters

Spec Table 1 (p. 6). B_k = centred binomial with parameter k.

| parameter | 648 | 768 | 1024 | 1536 | 2048 | Light | Prime |
|---|---|---|---|---|---|---|---|
| n | 648 | 768 | 1024 | 1536 | 2048 | 512 | 1087 |
| q | 3457 | 3457 | 3457 | 3457 | 3457 | 769 | 2017 |
| q2 | 2^9 | 2^10 | 2^10 | 2^10 | 2^10 | 2^8 | 2^10 |
| (kg, kf) | (B9,B2) | (B4,B3) | (B5,B2) | (B3,B2) | (B5,B1) | (B2,B1) | (B2,B1) |
| (kr, ke) | (B2,B2) | (B4,B2) | (B3,B2) | (B2,B1) | (B2,B1) | (B1,B2) | (B2,B1) |
| LWE (C,Q) | 164,144 | 198,174 | 271,238 | 411,361 | 567,498 | 136,120 | 288,253 |
| NTRU (C,Q) | 164,144 | 195,171 | 270,237 | 416,365 | 571,501 | 132,116 | 280,246 |
| delta (DFR) | 2^-146.79 | 2^-185.56 | 2^-190.48 | 2^-195.50 | 2^-204.10 | 2^-143.92 | 2^-180.72 |
| claimed level | 128 | 192 | 256 | 384 | 512 | 128 | 256 |

Sizes (bytes), spec Table 2 vs the built reference library. Table 2 has no
secret-key column; the "sk spec" entries below are derived from Fig. 2's
sk = (sk', pk, s) and the code's own packing widths.

| instance | pk spec | pk impl | sk impl | ct spec | ct impl | ss impl | match |
|---|---|---|---|---|---|---|---|
| DTRU-648 | 972 (954 packed) | 972 / 954 | 1328 / 1310 | 729 | 729 | 32 | yes |
| DTRU-768 | 1152 (1130) | 1152 / 1130 | 1568 / 1546 | 960 | 960 | 32 | yes |
| DTRU-1024 | 1536 (1506) | 1536 / 1506 | 2080 / 2050 | 1280 | 1280 | 32 | yes |
| DTRU-1536 | 2304 (2259) | 2304 / 2259 | 3136 / 3091 | 1920 | 1920 | 64 | yes |
| DTRU-2048 | 3072 (3012) | 3072 / 3012 | 3904 / 3844 | 2560 | 2560 | 64 | yes |
| DTRU-Light | 640 (615) | 640 | 864 | 512 | 512 | 32 | yes* |
| DTRU-Prime | 1495 | 1495 | 1935 | 1359 | 1359 | 32 | yes |

The second number in each pair is the `-DPK_PACK_OPT=1` build (lossless pk
compression, second KAT set). *DTRU-Light's compressed 615-byte figure is
printed in Table 2 even though the text says Light has no compressed version;
the code has no `PK_PACK_OPT` branch for Light, so only 640 is reachable.

## Pseudocode

### Encode (spec Algorithm 1) — double E8, l <= n/4 message bits
```
for i = 0 .. n/16-1:
    (w_{8i}, ..., w_{8i+7}) <- (m_{4i}, ..., m_{4i+3}) * H  (mod 2)   // [8,4] RM code
w <- (w_0 + w_1 x + ... + w_{n/2-1} x^{n/2-1}) * (1 + x^{n/2})   in R_q
```
### Decode (spec Algorithm 2)
```
for i = 0 .. n/16-1:
    v <- (t_{8i..8i+7}, t_{8i+n/2 .. 8i+7+n/2})  in Z_q^16      // pair the two halves
    for j = 0..7:
        dis[j][0] = ||v_j||^2_{q,2} + ||v_{j+8}||^2_{q,2}
        dis[j][1] = ||v_j - (q+1)/2||^2 + ||v_{j+8} - (q+1)/2||^2
    for j = 0..15:
        (u_0..u_7) <- (j_0,j_1,j_2,j_3) * H
        cost[j] = sum_k dis[k][u_k]
    k <- argmin cost;   (m_{4i}..m_{4i+3}) <- (k_0,k_1,k_2,k_3)
```
Decoding is correct whenever ||v - ((q+1)/2)(u,u)||_{q,2} < (q-1)/sqrt(2)
(minimum distance sqrt(2)(q-1) of the scaled double-E8 code). There is no
failure symbol — Decode always returns the nearest codeword; a wrong result is
caught only by the KEM's re-encryption check. The residual probability is the
delta row of Table 1.

### DTRU.PKE (spec Fig. 1)
```
KeyGen:                              Enc(pk = h, m):
 1. f', g <- (Psi_f, Psi_g)           1. r, e <- (Psi_r, Psi_e)
 2. f <- p*f' + 1                     2. w <- Encode(m)
 3. if f not invertible in R_q:       3. c <- round( (q2/q) * (h r + e + ((q+1)/2) w) ) mod q2
 4.    resample (goto 1)              4. return c
 6. h <- g f^-1 in R_q
 7. return (pk, sk) = (h, f)         Dec(sk = f, c):
                                      1. c' <- round( (q/q2) * c ) mod q
p = 2 for tricyclotomic rings and     2. m' <- Decode(c' * f)
LPPNF; p = 1 - x^{n/2} for            3. return m'
power-of-two cyclotomic rings.
```
### DTRU.KEM (spec Fig. 2) — FO^{not-perp}_{ID(pk),m}
```
KeyGen:  (pk, sk') <- PKE.KeyGen(1^kappa);  s <-$ {0,1}^kappa
         sk = (sk', pk, s)

Encaps:  m <-$ M
         (K, rho) <- H(ID(pk), m)
         c <- PKE.Enc(pk, m; rho)
         return (c, K)

Decaps:  m'      <- PKE.Dec(sk', c)
         (K',rho') <- H(ID(pk), m')
         Ktilde  <- H1(ID(pk), s, c)
         if PKE.Enc(pk, m'; rho') != c: return Ktilde   else: return K'
```
ID : PK -> {0,1}^gamma with gamma >= 256 "maps public keys to fixed-length
prefixes"; H : {0,1}* -> {0,1}^kappa x {0,1}^coins.

## Implementation vs specification

Checked: `src/DTRU-*/params.h` (all seven instances), and
`KEM_AlgorithmInstance.c`, `dtru.c`, `poly.c`, `coding.c` for DTRU-1024 with
spot checks of DTRU-1536/2048/Light/Prime.

Agreements:
- n, q, q2 and both noise pairs match Table 1 for **all seven** instances. The
  noise pairs are encoded as the coin-buffer split, e.g. DTRU-648
  `COINBYTES_KEYGEN = CBD2 + CBD9` and `COINBYTES_ENC = CBD2 + CBD2`
  ((kg,kf) = (B9,B2), (kr,ke) = (B2,B2)); DTRU-768 `CBD3 + CBD4` / `CBD4 +
  CBD2`; DTRU-1024 `CBD2 + CBD5` / `CBD3 + CBD2`; DTRU-1536 `CBD2 + CBD3` /
  `CBD2 + CBD1`; DTRU-2048 `CBD1 + CBD5` / `CBD2 + CBD1`; Light and Prime
  `CBD1 + CBD2` / matching their (B2,B1),(B1,B2) and (B2,B1),(B2,B1) rows.
- Every pk and ct size in Table 2 is reproduced by
  `DTRU_PKE_PUBLICKEYBYTES = n*log2(q)/8` and
  `DTRU_PKE_CIPHERTEXTBYTES = n*log2(q2)/8`, and each compressed pk value
  (954 / 1130 / 1506 / 2259 / 3012) is hard-coded under `#if PK_PACK_OPT`.
- `pke_keygen` (`dtru.c:5`) is Fig. 1 line for line, including `f = p*f' + 1`
  (`poly_multi_p` then `f.coeffs[0] += 1`) and the resample loop driven from
  `kem_keygen`'s `do { coins } while (pke_keygen(...))` on a non-invertible f.
  DTRU-Prime is the exception: its `pke_keygen` (`dtru.c:7`) returns `void`
  and `kem_keygen` calls it once, with no invertibility check or resample loop.
- `poly_encode_compress` (`poly.c:102`) implements Algorithm 1 exactly: each
  message nibble goes through `encode_e8`, the result is written to both
  `mh[2i]` and `mh[2i + n/16]` (the `*(1 + x^{n/2})` factor), then
  `c = ((sigma + mask & (q+1)/2) << log q2 + q/2) / q` mod q2, i.e.
  round((q2/q)(hr + e + ((q+1)/2)w)). DTRU-648 and DTRU-Prime do not use the
  n/2 offset: DTRU-648 repeats at `mh[2i + 40]` and decodes 40 blocks paired at
  coefficient offset 320 (`poly.c:124,154-159`), and DTRU-Prime repeats at
  `mh[2i + 64]` and decodes 64 blocks paired at offset 512
  (`poly.c:66,99-104`).
- The FO check and key selection are constant-time: an OR-accumulated byte
  difference, `fail = (-fail) >> 31`, and a masked XOR select of K vs Ktilde.
- All randomness (keygen coins, s, and m) comes from
  `get_random_number(&drng_algorithm, ...)`; the commented-out `randombytes`
  calls are dead.

Discrepancies:
- **(a) the rejection key omits ID(pk).** Fig. 2 Decaps line 3 is
  `Ktilde = H1(ID(pk), s, c)`. The implementation computes
  `pseudohash(512, c || z)` (`KEM_AlgorithmInstance.c:144` in DTRU-1024,
  `:153` in DTRU-1536/2048) — the public-key prefix is not an input, the
  operand order is reversed, and there is no separate H1: the same
  `pseudohash(512, ...)` that plays the role of H is reused with no domain tag.
  Only the differing input length separates the two oracles. Consequences are
  limited (the reject key still binds c and the per-key secret s), but the
  multi-target property the `FO_{ID(pk),m}` variant was chosen for is lost on
  the reject path.
- **(a) the rejection flag is returned to the caller.** `kem_dec` ends with
  `return fail;` in **all seven** instances. The whole point of the
  not-perp / implicit-rejection variant in Fig. 2 is that decapsulation always
  returns a key and never signals which branch was taken; here the NGCC API
  return value is 1 exactly when re-encryption failed. Any caller that checks
  the return code gets a decryption-failure oracle for free. The shared-secret
  buffer itself is still selected in constant time, so this is an API-level
  leak rather than a timing one.
- **(b)/(a) ID(pk) is a literal truncation, not a hash.** The code copies the
  first `DTRU_PREFIXHASHBYTES` = 33 bytes (65 at the 64-byte-key levels) of the
  public key: `for (i = 0; i < DTRU_PREFIXHASHBYTES && i < pk_len_bytes; ++i)
  buf2[i] = pk[i];`. gamma = 264 bits satisfies the spec's gamma >= 256, and
  the spec does say "fixed-length prefixes", but two public keys agreeing on
  their first 33 bytes are indistinguishable to H — a collision-resistant hash
  is what the DHK+21 transform assumes. Worth confirming with the authors
  whether "prefix" was meant literally.
- **(c) equivalent, level-dependent H.** For the 32-byte-key instances H is one
  `pseudohash(512, ID(pk)||m)` split as `K = buf[0..31]`,
  `rho = pseudoXOF(buf[32..63])`. For DTRU-1536/2048 (64-byte keys) a
  domain-separation byte is appended (`0` for K, `1` for the rho seed) and two
  512-bit hashes are taken. Both realise `(K, rho) <- H(ID(pk), m)`; the spec
  does not fix the instantiation.
- **Robustness note.** In `kem_dec`, the loop that assembles `ct2 = c || z`
  uses the caller-supplied `ct_len_bytes` rather than
  `DTRU_PKE_CIPHERTEXTBYTES`, and `z` is then written at offset
  `ct_len_bytes`, so the buffer has only `DTRU_PREFIXHASHBYTES` (33 or 65)
  bytes of slack. An oversized declared
  ciphertext length overflows the stack buffer. Not reachable through the KAT
  flow, and the already-recorded harness length-metadata tests stay inside the
  slack.

Not verified: the E8 `decode_e8` / `poly_decode` cost tables against
Algorithm 2's `dis_matrix`, the NTT/inverse code, the `PK_PACK_OPT` packing
routine, and the Table 1 DFR and security figures.
