No: passing the whole 512-bit encryption seed to the generator in YuanYang.KEM-2048 is a one-argument code change that alters no size and no hash or sampling count.

Impact: none
Scope: Single line (small), code
Assessed: 2026-11-24

## Attacks considered

- kem-40-5: Truncated encryption seed permits a 2^440 challenge-key distinguisher at the 512-bit level (Implementation, Confirmed)

## Remediation

- **kem-40-5.** The root cause is the length argument in `yy_encrypt`: it hands `min(1 + YY_SEC/8, SEEDLEN)` = 55 of the 65 seed bytes to `init_random_number`, although Algorithm 3 gives Encrypt a seed `ρ` of λ = 512 bits. The costed repair is to pass all 65 bytes, the 64-byte seed and its domain byte. The generator's instantiate routine takes an input of any length and hashes all of it, so nothing else has to change, and the specification is sound on the seed width and stays as it is. This is the cheapest credible repair because it restores the specified seed in the one affected set and leaves the scheme, its parameters and its encodings alone; the 512 and 1024 sets pass 17 and 33 bytes and are not truncated. Scope: Single line, code: one argument in the reference `kem.c`, and in the optimized copy, which is byte-identical. Impact: none. The seed is a secret intermediate value, so the public key and the ciphertext are unchanged. The seed derivation hashes 60 bytes before the repair and 70 after it, two SM3 compression calls per digest in both cases, and the generator then returns the same number of bytes, so encapsulation and the re-encryption in decapsulation do the same work. Key generation does not use this path. The message, the blinding string `msg′` and the shared key are 512 bits each in the code, so no plaintext width has to grow. The generator is the contest's ICCS SM3 DRBG (`drng.c`, `init_random_number` and `get_random_number`), not an expander of the scheme's own: Algorithm 3 draws `msg′`, `s` and `e` from `ρ` without naming one. By the standing NGCC convention that DRBG is a placeholder for a perfect DRBG: its 55-byte (440-bit) working state (`drng.h:15`) is not a weakness, while seeding it with less input than the level is. The finding places the truncation at this caller, and that seeding is what the repair changes; the generator's state is not costed. Using the external RBG as the deterministic encryption generator at all is kem-40-3 (Low) and is not costed here. The report records no proposed fix. Nothing was built or run.

## Evidence

- Baseline: Table 4 (physical p. 20) gives a public key of 3328 bytes and a ciphertext of 2880 bytes for YuanYang.KEM-2048. Neither carries the seed.
- Specification steps: Algorithm 3 input `ρ ∈ {0,1}^λ` and lines 1 and 3, which draw `msg′`, `s` and `e` from `ρ` (physical p. 10); λ = d/4 = 512 (physical p. 9); Algorithm 8 line 2 derives `(K, ρ)` (physical p. 12).
- Source, `Implementations/Reference_Implementation/yuanyang-2048`: the capped call at `kem.c:111`, with `YY_SEC` = `YUANYANG_D/4` at `kem.c:32` and `SEEDLEN` = 55 at `drng.h:15`; the 65-byte seed is `tmphash + YY_SEC/8` at `kem.c:165`. The instantiate routine copies `nonce_len_bytes` bytes and hashes them in `SM3_df` (`drng.c:244–252`, `drng.c:200–235`).
- Count, seeding: `SM3_df` computes two digests over 1 + 4 + L bytes. For L = 55 that is 60 bytes and for L = 65 it is 70 bytes; with SM3's 9 bytes of minimum padding both fill two 64-byte blocks, so 4 compression calls before and after.
- Count, expansion: the generator returns 64 bytes for `msg′` (`kem.c:119`) and, through the buffered `prng`, one byte per coefficient of `s` and `e` (`kem.c:80–95`), 2·2048 bytes, whatever the seed length.
- Widths: `msg` is `YY_SEC/8` = 64 bytes (`kem.c:157–158`), `msg′` is `YY_SEC` = 512 bits (`kem.c:119`), and the shared key is 64 bytes (`kem.c:169–170`).
- Generator: `drng.c` and `drng.h` carry the ICCS header and are the contest's DRBG. Its state is `V[SEEDLEN]` at `drng.h:24`; output block i is SM3 of the 55-byte value `V + i` (`drng.c:276–300`). The convention above sets this state aside; what remains is the caller's cap of the seed length at `SEEDLEN` (`kem.c:111`).
- Controls: `1 + YY_SEC/8` is 17 and 33 for the 512 and 1024 sets (`kem.c:111` in each tree), below the cap.
