No: the one Critical finding is a wrong destination buffer in decapsulation, and correcting it in the code changes no size and adds no work.

Impact: none
Scope: Single line (small), code
Assessed: 2026-11-23

## Attacks considered

- kem-01-1: Ineffective implicit rejection breaks IND-CCA security (Implementation, Confirmed)

## Remediation

- **kem-01-1.** Make `mkem_dec` follow Algorithm 20: on a failed comparison the constant-time move must write the rejection key into the returned secret `ss`, not into the local `buf` (`kem.c:80`). That is the root cause the report states. The specification is sound on the point, so only the code changes. Scope: Single line, code. The same line is in all twelve `kem.c` files (reference, AVX2, NEON and AArch64, three levels each). Impact: none. The submitted code already runs the re-encryption, the comparison of all `CT_BYTES` and the rejection hash over `SEED_BYTES + CT_BYTES` bytes on every call, so the corrected code does the same work, and no transmitted object changes. The report records no proposed fix. The reference and AArch64 trees also read the rejection secret from the wrong address (`kem.c:76`, finding kem-01-2, Low). That is a second one-argument correction in the same function. It adds no work and is not costed here.

## Evidence

- Specification: Algorithm 20 (Aigis-Enc+.Decap), physical p. 25, lines 7 to 11: `c′ := Enc_internal(pk, M′, ρ″)`, `K := H3(s, c)`, and `K := K′` only if `c = c′`. Table 1 (physical p. 15) states public keys of 656, 1312 and 2624 bytes and ciphertexts of 896, 1664 and 3584 bytes. The repair touches neither.
- Source, `Implementations/Reference_Implementation/Aigis-Enc+-I/kem.c`: line 71 copies the candidate key to `ss`; line 72 re-encrypts; line 74 is `verify(ct, cmp, CT_BYTES)`; line 78 is `KDF(buf2, SEED_BYTES, buf2, SEED_BYTES + CT_BYTES)`; line 80 is `cmov(buf, buf2, SEED_BYTES, fail)`. Lines 71 to 80 hold the same calls in the other eleven `kem.c` files; line 76 reads `sk-SEED_BYTES` in the reference and AArch64 trees and `sk + SK_BYTES - SEED_BYTES` in the AVX2 and NEON trees.
- Not checked: nothing was built or run for this note, and the assembly and vector code below `kem.c` were not read.
