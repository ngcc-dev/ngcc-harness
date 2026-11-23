No: the repair costed for the one Critical finding widens the TSUOV-512 message digest µ from 512 to 1024 bits, and µ is only hashed, so no size changes and the added work is far below 1% of signing or verification.

Impact: none
Scope: Function (small), specification and code
Assessed: 2026-11-23

## Attacks considered

- sign-31-1: A 512-bit prehash caps forgery security at 256 bits (Design, Confirmed)

## Remediation

- **sign-31-1.** Root cause: Expand_µ(seed_pk || M) is defined as a 512-bit `pseudohash` at every level (physical p. 19), and Sign and Verify hash only µ || salt to the target (Algorithm 2 lines 1 and 27, Algorithm 3 lines 3–4, physical pp. 20–21), so a collision in µ costs about 2^256 at TSUOV-512. The costed repair is the usual one for a narrow prehash: at TSUOV-512, Expand_µ outputs 1024 bits. The contest `pseudohash` that the code calls has a 1024-bit form (`auxfunc.c:470`), and the specification's proof already carries the digest length as a parameter, `digest_bytes` (physical pp. 27–29). Scope: Function, specification and code: the definition of Expand_µ, and in the code the call and the length constant (`tsuov_core.c:423` and `tsuov.c:143`, `tsuov_params.h:81`), which size the buffers of `Hash`, `TSUOV_Sign` and `TSUOV_Verify`. TSUOV-128 and TSUOV-256 keep 512 bits, which is at least twice their level. Impact: none. µ is not transmitted: the public key is seed_pk and the P_i,3, and the signature is the salt and s (physical p. 18), so no object grows. The target space F_31^240 has about 1189 bits and the salt is 512 bits, so neither has to grow for this finding. The message is absorbed by one hash call as before; `Hash` absorbs 192 bytes where it absorbed 128, once per salt trial in signing and once in verification, against about 7.7×10^6 bytes of PRG output that each verification expands from seed_pk. The specification attributes over 80% of the cycles of the optimized implementation to that expansion (physical pp. 23–24). The report records no proposed fix. Not checked: nothing was built or run. A 1024-bit hash may cost more per message byte than a 512-bit one; this is not counted, because the contest hash is a placeholder and the stated timings use a 64-byte message (physical p. 22).

## Evidence

- Baseline, Table 1 (physical p. 22): TSUOV-512 has (q, ℓ) = (31, 2), n = 230, o = 12, m1 = 218, k = 10, m2 = 240; secret key 128, public key 21,319 and signature 2,939 bytes. Control with the formulas of physical p. 18, public key ⌈log2 q⌉·m1·ℓ·o(o+1)/2 + λ bits and signature ⌈log2 q⌉·nℓk + λ bits: 5·218·2·78 = 170,040 bits = 21,255 bytes, plus 64 = 21,319; 5·230·2·10 = 23,000 bits = 2,875 bytes, plus 64 = 2,939. TSUOV-256: 5·114·2·15 = 17,100 bits → 2,138 + 32 = 2,170; 5·119·2·16 = 19,040 bits = 2,380 + 32 = 2,412. TSUOV-128: 5·64·2·11 = 7,040 bits = 880 + 16 = 896 for the signature; the public-key formula gives 5·61·2·10 = 6,100 bits → 763 + 16 = 779 bytes, where Table 1 states 778 (the code's `CRYPTO_PUBLICKEYBYTES` gives 779). The repair touches none of these.
- Target space: 240·log2(31) = 1189.0 bits.
- Hashed bytes: `Hash` absorbs `TSUOV_MU_LEN + TSUOV_SALT_LEN` bytes (`tsuov_core.c:427-434`), 64 + 64 = 128 as submitted and 128 + 64 = 192 with the wider µ. Verification expands P_i,1 and P_i,2 for every i < m1 (`tsuov_core.c:1011-1012`, `544-548`), each by one `pseudoXOF` call of `TSUOV_Pi1_RS_LEN` = 31,536 and `TSUOV_Pi2_RS_LEN` = 3,665 bytes (`tsuov_core.c:403-412`, `tsuov_params.h:52-53`): 218·(31,536 + 3,665) = 7,673,818 bytes.
- Source paths are under `Implementations/Digital_Signature-TSUOV-x86-Reference_Implementation/API_PKC/Implementations/`: `Reference_Implementation/TSUOV_512/tsuov_core.c` and `tsuov_params.h`, `Optimized_Implementation/TSUOV_512/tsuov.c`. The same call and constant are in the trees of the performance-optimized package.
