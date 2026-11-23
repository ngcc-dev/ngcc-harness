No: the submitters' proposed word-dependent round constants change one step of the round function and add no operation per round, so CHIME-512 and CHIME-1024 keep their cost per byte.

Impact: none
Scope: Function (small), specification and code
Assessed: 2026-11-23

## Attacks considered

- hash-26-1: An invariant subspace reduces CHIME collision bounds to 64 and 160 bits (Design, Confirmed)

## Remediation

- **hash-26-1.** The root cause has two parts. Every layer of the round function maps a 256-bit array of four equal 64-bit words to an array of the same form, and each round constant `RC_i` is four copies of one word `X_i` (Sections 1.2 and 3.2). The costed repair is the submitters' proposal, which removes the second part: the four words of each round constant become word-dependent. Breaking a symmetry of the round function with asymmetric round constants is also the standard technique for an invariant subspace of this kind. The report records no attack against the proposal. The change is confined to the AddRoundConstant step and its table: Sections 1.2 and 3.2 and Table 7 in the specification, the `ARC` table and its XOR loop in the reference code, and `ARC64` and `init_arc` in the optimized code. Both instances share the permutation and the table, so the same change applies to both. Scope: Function, specification and code. Impact: none (count). AddRoundConstant already XORs four 64-bit words into `A0` in every round, so word-dependent values add no operation. The rate, the state and the round numbers stay, so the cost per byte and the cost of a one-block message do not change. The constant table grows from 20 to 80 words of 64 bits.

## Evidence

- Baseline: CHIME-512 is a sponge with rate 512, capacity 1024 and 15 rounds (Section 1.5.1, physical p. 4). CHIME-1024 is a sponge with feeding-forward with rate 448, capacity 1088 and 20 rounds (Section 1.5.2, physical p. 5). The code runs 15 and 20 rounds (`CHIME/Implementations/Reference_Implementation/CHIME-512/CryptHash_AlgorithmInstance.c:831`; `CHIME-1024/CryptHash_AlgorithmInstance.c:861`). The digest lengths are fixed and a hash has no bandwidth. Table 2 (physical p. 8) gives software throughput; it is context and was not used.
- Round constants as specified: `RC[i]` contains "four identical 64-bit words" (Section 1.2, physical p. 3); `X_i` is given by the formula in Section 3.2 (physical p. 9) and listed for 20 rounds in Table 7 (physical p. 10); AddRoundConstant is `A0 <- A0 xor RC_i` (Section 1.6.2, physical p. 6).
- Count, reference CHIME-512, 64-bit word operations per round, with the permutations of 32-bit and 64-bit words (`sh_0x39`, `sh_0x4e`, `pq_0x93`, `p128_0x01`) not counted. The linear layer has 11 array XORs and 5 shifts or rotations of four words each = 64 (Section 1.6.1, physical p. 5-6; `:359-410`). AddRoundConstant has 4 XOR (`:490-492`; CHIME-1024 `:491-493`). The nonlinear layer has 2 x 4 x 9 = 72 (`chi3`, `:292-304`). Total 140, of which AddRoundConstant is 4, or 3%, before and after the change.
- Optimized code: one 256-bit XOR per round, `s.A0 = X(s.A0, ARC[r])` (`CHIME/Implementations/Optimized_Implementation/CHIME-512/P.hpp:370`; `CHIME-1024/P.hpp:376`). The table is built once from `ARC64` with `_mm256_set1_epi64x` (`CHIME-512/P.cpp:18-22`; `CHIME-1024/P.cpp:22-26`). A word-dependent table changes that initialization and not the round.
- Proposed fix: the submission team's [erratum post](https://list.niccs.org.cn/archives/list/crypthashforum@list.niccs.org.cn/message/HLW3FPCIRT2UIR3YMYJJ5YW7SCGZENUL/) describes "a correction to the round-constant layout, where the four 64-bit words of each round constant are revised to be word-dependent rather than identical", as reported by the submitters and not evaluated.
- Not checked: the erratum PDF attached to the post was not read, so the new constant formula was not examined, including whether the new words have a symmetry of their own. The post says the erratum also holds specification clarifications and editorial corrections; these were not costed. Other invariant subspaces of the revised permutation were not searched for. The hardware figures of Tables 3 to 5 (physical p. 8-9) were not assessed, and the archive holds no RTL.
