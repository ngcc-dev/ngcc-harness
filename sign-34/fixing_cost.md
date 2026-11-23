No: making the 1024 and 2048 code hash the salt before the message and public key, as Algorithm 8 specifies, with no 256-bit value between the message and the challenge at 2048, changes no transmitted object and adds less than 1% of hashing work to signing and verification.

Impact: none
Scope: Function (small), code
Assessed: 2026-11-23

## Attacks considered

- sign-34-5: An unsalted 256-bit prehash permits signature transfer at the higher levels (Implementation, Confirmed)

## Remediation

- **sign-34-5.** Make the code compute the target as `H(salt || M || pk)`, as Algorithm 8, line 4 and Algorithm 14, line 3 specify (physical pp. 15 and 20): the salt is absorbed first, inside the salt loop, in place of the fixed value `SM3(M || h)` that is computed before the salt is drawn. Under the finding's model a collision prepared in advance then does not apply, because the signer draws the salt of λ + 80 bits afterwards; the transfer needs a second preimage of the intermediate value under that salt, about 2^256 for a 256-bit value. That is the level of YuanYang.DSA-1024. YuanYang.DSA-2048 claims 512 bits and also needs the intermediate value to be 512 bits wide, or the XOF to absorb `salt || M || pk` itself. Scope: Function, code: the two hash helpers and their call order (`common.c:33–54,57–136`, `sign.c:596,619,628` and `vrfy.c:116,127` in the yuanyang-2048 reference tree, and the same code in the other trees). The specification already has this order and stays. The signature encoding does not change, since the salt is already sent and the stated signature sizes include it. Impact: none. The hash absorbs 42 or 72 more bytes on top of 2112 or 4160 (+2.0% and +1.7%), and the challenge expansion is unchanged. In signing, the hash of `M` moves into the salt loop and runs 1.1 times per signature instead of once; it is 66 SM3 compression calls at 2048 against 290 for one challenge expansion. Not counted: for a long message the hash of `M` dominates signing, and it then runs 1.1 times per signature instead of once, about +10%; verification hashes it once, as submitted. Not checked: the optimized trees beyond the call order.

## Evidence

- Baseline: Table 6 (physical p. 35) gives signatures of 1,150 and 2,364 bytes at 1024 and 2048, salt included. Control with `yuanyang_params.h:12–13,28,36`: 42 + 1108 = 1150 and 72 + 2292 = 2364. Table 3 (physical p. 26) gives the salt as λ + 80 = 336 and 592 bits; the code and Table 6 use 72 bytes at 2048.
- Submitted order: `seed = sm3hash(256, M || h)` (`common.c:50–52`), computed once before the loop (`sign.c:596`); the salt is drawn at `sign.c:619`; the challenge is `pseudoXOF(salt || seed)` (`common.c:83–95`, `sign.c:628`). Verification repeats it (`vrfy.c:116,127`).
- Hash work in verification of the 64-byte benchmark message (physical p. 35), in SM3 compression calls. Prehash: 64 + 2·1024 = 2112 bytes is 34 calls and 64 + 2·2048 = 4160 bytes is 66. Challenge: 2,268 and 4,626 stream bytes (`common.c:63–73`) are 71 and 145 KDF blocks of 2 calls each, 142 and 290. Totals: 176 and 356.
- With the salt first: 2154 bytes is still 34 calls, and 4232 bytes is 67. That is +0 of 176 at 1024 and +1 of 356 at 2048.
- Share of hashing: at 512 the specification attributes 11.29% of verification time to `SM3(m || h)` and 48.43% to the challenge expansion (physical p. 36); the call counts there are 18 and 70, a similar proportion. Lattice sampling is 85.29% of signing time at 512 (physical p. 36).
- Repetitions: Table 3 gives 1.1 (physical p. 26), so the hash of `M` runs 1.1 times per signature when it is inside the salt loop.
- The finding's cost for the submitted order, 2^128.24, is printed by `python3 -B sign-34/reproduce_prehash_ceiling.py`.
- Not counted: the code calls the contest's `sm3hash` and `pseudoXOF`, which the specification describes as placeholders to be replaced (physical p. 36), so calls and bytes are counted and an ideal hash of the needed width is assumed. The placeholders as built cost more for a wide value: the 512-bit `pseudohash` makes two SM3 passes over its input (`auxfunc.c:224–279`), and KDF-SM3 hashes its whole input for every 32 output bytes (`auxfunc.c:482–519`). A 512-bit intermediate value built from them would add one pass over `salt || M || h`, about 67 compression calls on 356 in YuanYang.DSA-2048 verification; the specification attributes 60% of verification time to hashing at 512.
- The public map's codomain has `q^d` elements, far above 2^512, so the usual check for hash-and-sign schemes adds nothing.
