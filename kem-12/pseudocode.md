# kem-12 CTL ("Chase the Light") — algorithm summary

NTRU-lattice KEM. The public key is an NTRU quotient h = f^-1 g mod q (hardness:
decisional NTRU / DSPR); encryption is **Ring-LWR** — c1 = round((h·s mod q)/k)
with a binary secret s — and decryption uses the NTRU trapdoor basis B_{f,g}
together with a precomputed integer auxiliary vector w to solve a 2x2 linear
system (a fully integer Babai/GGH-style decoder, no floating point). The KEM is
an FO-style construction: m is recovered, s and c1 are re-derived and compared.
Structurally this is the BAT KEM family with SM3/pseudoXOF replacing BLAKE2.
Key generation needs GMP (+ libquadmath) for the NTRU-equation solver.

Specification: `kem-12-spec.pdf` (58 pages), §4 (Algorithms 1–6) and §5
(Algorithms 7–9); parameters §3.4 Table 1, security Table 2, sizes §3.5.5
Table 3. Written in English with some Chinese-typeset formulae.

## Parameters

Spec Table 1 (p. 24). R = Z[x]/(x^n+1).

| parameter | CTL_128 | CTL_256 | CTL_512 | meaning |
|---|---|---|---|---|
| n | 512 | 1024 | 2048 | ring degree |
| q | 257 | 769 | 3329 | ciphertext/public-key modulus |
| k | 2 | 4 | 8 | LWR rounding divisor, c1 = round(hs/k) |
| b | 128 | 192 | 416 | listed in Table 1, never defined in the text |
| sigma_f | 0.48 | 0.47 | 0.64 | std. dev. of the trapdoor sampler |
| alpha | 1.17 | 1.23 | 1.40 | trapdoor quality parameter |
| q' | 64513 | 64513 | 64513 | auxiliary decoding modulus |
| tau | 11.83 | 14.76 | 18.88 | failure-bound parameter |
| DFR | 2^-196 | 2^-308 | 2^-426.1 | decryption failure probability |
| classical security (key recovery) | 2^137.6 | 2^256.3 | 2^512.5 | spec Table 2 |
| quantum security (key recovery) | 2^122.3 | 2^227.2 | 2^452.5 | spec Table 2 |
| claimed level | 128 | 256 | 512 | bits |

Sizes (bytes), spec Table 3 vs the built reference library (long-format sk,
which is what the API exposes; all encodings carry a 1-byte type/level header):

| instance | pk spec | pk impl | sk spec | sk impl | ct spec | ct impl | ss impl | match |
|---|---|---|---|---|---|---|---|---|
| CTL-257-512 | 521 | 521 | 2953 | 2953 | 473 | 473 | 16 | yes |
| CTL-769-1024 | 1230 | 1230 | 6030 | 6030 | 1006 | 1006 | 32 | yes |
| CTL-3329-2048 | 3009 | 3009 | 15617 | 15617 | **2305** | **2353** | 48 | **no** |

The spec also lists short-format private keys (417 / 801 / 3105 bytes; seed +
F only, everything else recomputed) — not exposed through the built API.

## Pseudocode

### KeyGen (spec Algorithm 1)
```
1. gamma = sqrt((k^2-1)/3)                       // distribution balancing
2. (f, g) <- Sampler_fg(R, q, alpha, xi)         // Algorithm 7, Fourier-domain
                                                 //   annulus sampling
3. if f or g not invertible mod q: goto 2
4. if || q*gamma * ( -gamma*fbar/(gamma^2 f fbar + g gbar),
                      gbar/(gamma^2 f fbar + g gbar) ) ||
        > q*gamma*sqrt(I(gamma,r,R)): goto 2     // trapdoor quality check
5. h = f^-1 g mod q
6. solve gF - fG = q for (F, G) in R             // NTRU equation
7. B_{f,g} = [[g, G], [f, F]]
8. w = ComputeVec(B_{f,g}, q', gamma)            // Algorithm 8
9. return (h, (B_{f,g}, w))
```
### Encrypt_base(h, q, k, s) (spec Algorithm 2)
```
1. c1 = round((h*s mod q) / k)
2. e  = (h*s mod q) - k*c1
3. gamma = sqrt((k^2-1)/3)
4. if ||(gamma*s, e)|| > 1.08*sqrt(n(k^2-1)/6): return _|_
5. return c1
```
### Encrypt(h, q, k, m, seed) (spec Algorithm 3)
```
1. s  <- Sample(U(R mod 2); seed)                // binary ring element
2. c1 <- Encrypt_base(h, q, k, s);  if c1 = _|_ return _|_
3. c2 = Hash_m(s) XOR m
4. return (c1, c2)
```
### Decrypt((c1,c2), h, (B_{f,g}, w), q, k, q') (spec Algorithm 4)
```
1. c' = c1 * k
2. (e, s) <- Decode(B_{f,g}, w, q, q', 2, c')    // Algorithm 9
3. gamma = sqrt((k^2-1)/3)
4. if ||(gamma*s, e)|| > 1.08*sqrt(n(k^2-1)/6): return _|_
5. if c1 == round((h*s mod q)/k): m = Hash_m(s) XOR c2  else return _|_
6. return m
```
### Decode (spec Algorithm 9) — the trapdoor decoder
```
1. (Gd, Fd) = (q'G - g*w,  q'F - f*w)
2. c'  = (Q f c - f(Q mu_e 1) - g(Q mu_s 1))            mod qQ
3. c'' = (q'Q F c - F(q'Q mu_e 1) - G(q'Q mu_s 1) - c'w) mod q q' Q
4. solve [[f, g],[Fd, Gd]] (e'; s') = (1/Q)(c'; c'')
5. return e = e' + mu_e*1,  s = s' + mu_s*1
```
All steps are integer arithmetic modulo q, q' = 64513 and Q; no floating point.
There is no separate "decoding failure" branch — a wrong result is caught by
the norm test (step 4) and the re-encryption test (step 5) of Decrypt.

### Encapsulate (spec Algorithm 5)
```
1. m <- Sample(U(M))
2. (c1, c2) <- Encrypt(h, q, k, m, Hash_s(m))
3. if (c1,c2) = _|_ : restart from 1
4. K = Hash_k(m)
5. return ((c1,c2), K)
```
### Decapsulate (spec Algorithm 6)
```
1. m' <- Decrypt((c1,c2), h, (B_{f,g}, w), q, k, q')
2. if m' != _|_ and (c1,c2) == Encrypt(h, q, k, m', Hash_s(m')):
       return K = Hash_k(m')
   else: return _|_                      // spec: EXPLICIT rejection
```
Hashes (implementation instantiation, `api.c`): `HASH`/`hash_m` = `sm3hash(256)`
for 32-byte outputs, otherwise `pseudoXOF`; `EXPAND(seed, label)` prefixes an
8-byte little-endian label built from `q | logn<<16 | tag<<24` and 8 zero
bytes (`api.c:63-64`), giving domain
separation between rr-derivation (0x72), Hash_s/Sample_s (0x73), the bad-KDF
seed (0x62), and the final KDF (0x66 / 0x67 for reject / accept).

## Implementation vs specification

Checked: `Implementations/CTL-*/`: `api.c` (the whole KEM layer, included by
`api_<q>_<n>.c`), `ctl.h` (structures and encodings), `kem257.c` / `kem3329.c`
(encrypt/decrypt cores), `prng.c` (randomness), `keygen.c` + `ng_*.c` +
`ntru_solver.c` (trapdoor generation).

Agreements:
- The three (q, n) pairs and LVLBYTES are set in `api_257_512.c`,
  `api_769_1024.c`, `api_3329_2048.c` and match Table 1's (n, q): (512,257),
  (1024,769), (2048,3329). k is folded into the rounding constants
  (`kem257.c:90` uses `(x+129)>>1` i.e. k = 2, `kem3329.c:472` uses
  `((y+1668)>>3)-208` i.e. k = 8). q' = 64513 is the modulus of the
  `inner.h:1152` polynomial layer, matching Table 1.
- The Encrypt_base norm bound is implemented: `kem257.c:92-101` argues it can
  never be exceeded for q = 257 (hence encapsulation never loops there, as the
  spec's Table 1 gamma = 1 implies), and `kem3329.c:461-499` computes the sum
  of squares and compares against `(21 << logn) + e2norm`, i.e. the
  1.08*sqrt(n(k^2-1)/6) bound of Algorithms 2/4.
- The ciphertext is `(c1, c2)` with `c2 = Hash_m(s) XOR m`
  (`api.c` encapsulate), exactly Algorithm 3 step 3, and decapsulation
  recovers m by XORing `hash_m(sbuf)` back, exactly Algorithm 4 step 5.
- Randomness comes from the NGCC seeded DRNG: `ctl_get_seed` (`prng.c:14`)
  calls `get_random_number(&drng_algorithm, ...)`, despite the inherited BAT
  comments in `api.c` that say "get a random message m from the OS". For
  encapsulation, `kem_enc` draws the seed with `get_random_number` itself
  (`KEM_AlgorithmInstance.c:95` in CTL-257-512, `:84` in the other two;
  64/32/48 bytes, of which the first LVLBYTES are used as m) and passes it to
  `encapsulate_explicit_seed`.
- pk/sk sizes match Table 3 to the byte for all three instances, in both the
  long and (by the `get_privkey_length` formula) the short format.

Discrepancies:
- **(a) real deviation — explicit vs implicit rejection.** Spec Algorithm 6
  step 2 returns the failure symbol ⊥ when re-encryption disagrees. The
  implementation never fails: `Zn(decapsulate)` (`api.c`) computes
  `make_kdf_seed_bad(m_alt, sk->rr, ct)` = F(rr, c1, c2), conditionally
  replaces m with m_alt in constant time, and derives the shared secret with a
  *different* KDF label (`good + 0x66`), returning 0 in all cases. `ctl.h:175`
  documents this: "invalid ciphertext values lead to a recovered shared secret
  which is deterministic from the ciphertext and private key ... this function
  reports a success (0)". Implicit rejection is the stronger and more standard
  choice (the spec's own §6 IND-CCA claim is easier to support with it), but
  the spec text and the code disagree, and the private key has an extra field
  `rr` (32 bytes, derived from `seed` by `make_rr`) that no specification
  algorithm mentions.
- **(a) spec Table 3 ciphertext size is wrong for CTL-512.** Impl ct =
  1 (tag) + `ctl_encode_ciphertext_3329(c)` + `sizeof c2` = 1 + 2304 + 48 =
  2353, which is what the library reports. The spec's 2305 = 1 + 2304 omits
  the 48-byte `c2` field entirely, even though Table 3 is explicitly the
  "practical deployment form with the FO transform". The other two levels
  (473 = 1+456+16, 1006 = 1+973+32) include c2 and match, so this is an
  arithmetic slip in one table cell rather than a code bug. Separately,
  §3.5.3 (printed p. 28) fixes `c2` at 64 bytes for CTL-512, which would give
  2369; the implementation's 48-byte `c2` is filed as kem-12-2 in `report.md`.
- **(b) spec ambiguity — the parameter b.** Table 1's second column tuple is
  (b, k, q) with b = 128/192/416, and §3.4 lists b among the parameters to be
  determined, but b is never defined anywhere in the document and has no
  counterpart in the implementation. It is not the shared-secret length
  (16/32/48 bytes = 128/256/384 bits) and not the BKZ blocksize implied by
  Table 2.
- **(b) the shared-secret length is not specified.** The spec writes
  K = Hash_k(m) with "output space K" but never fixes |K|. The implementation
  uses LVLBYTES = 16 / 32 / 48 (`ctl.h:247-249`), confirmed by the observed
  ss sizes. A 16-byte shared secret for the "128-bit security" set is a
  deliberate choice, not a deviation, but it is undocumented.
- **(c) equivalent optimisation.** Key generation loops internally
  (`Zn(keygen)`: `continue` on a failed fg sample, a failed NTRU-equation
  solve, or a failed w computation) rather than having the spec's explicit
  "go back to step 2"; the trapdoor-quality test of Algorithm 1 step 4 lives
  inside `ctl_keygen_make_fg`. `encapsulate_explicit_seed` re-hashes m instead
  of drawing a fresh one on the rare Encrypt_base failure — a determinism
  convenience the spec's Algorithm 5 step 3 ("restart the entire process")
  does not describe; it is the path `kem_enc` uses.

Not verified: `Sampler_fg` (Algorithm 7) and `ComputeVec` (Algorithm 8) were
not checked line-by-line against the spec's formulae, the ntrugen-derived
`ng_*.c` / `ntru_solver.c` NTRU-equation solver was not reviewed, and the DFR
and security numbers were not recomputed. The whole tree is compiled with `-w`
in the harness because it is not warning-clean.
