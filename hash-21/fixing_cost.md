Yes: hash-21-1 and hash-21-2 with hash-21-4 are two separate mergers in a state update that is not injective, and removing them means replacing the round function of the PRNG primitive of Neulaser-512, -768 and -1024; that is a Construction change, no fix is proposed, and its impact on hashing is not bounded.

Impact: not bounded
Scope: Construction (large), specification and code
Assessed: 2026-11-23

## Attacks considered

- hash-21-1: A local feedback collision merges complete Neulaser states (Design, Confirmed)
- hash-21-2: Reduction modulo 2^32-5 creates reachable state mergers (Design, Confirmed)
- hash-21-4: Long random messages admit second preimages in one scan (Design, Confirmed)

## Remediation

- **hash-21-1.** Root cause: the word that a module shifts out, `s0`, reaches the next state only through the local feedback `Φ`, and it enters `Φ` twice: with coefficient 8193 in the linear core modulo `p`, and as input `R3` of the nonlinear word `F`. The two contributions cancel for some pairs, so the round update `U_r` maps distinct states to one state. A repair must make the new word a bijection of `s0` when the other words are fixed. That changes how `Φ` is formed (§3.1.2) and how it is inserted (§3.2), and it needs one word domain for the register, because `s0` is loaded as a 32-bit word while `Φ` is reduced modulo `p`. `report.md` records no proposed fix. A change of constants, schedule or round count does not remove the cause: the states are equal after one round, whatever follows. Scope: Construction, specification and code; the state update of the PRNG primitive is the compression function of the hash. Impact: not bounded.
- **hash-21-2, hash-21-4.** Root cause: `red_p` is applied to a 32-bit XOR result when `t3`, `t7`, `t11` and `t15` are formed, so `y` and `y + p` merge for `y` in 0..4, while `V || K` is loaded as unrestricted 32-bit words and the old stages 8 and 12, which feed `t7` and `t11`, are read by nothing else in the round. hash-21-4 is the same merger met by chance in a long message. Loading canonical words alone would not remove it, since the XOR of a word below `p` with the mask can still reach `p` or more. A repair must combine each tap with its mask by an operation that is a bijection on the word domain, and must load the block injectively into that domain. The 16µ words of F_p cannot hold `v + k = 512µ` bits, so either the block length `k` of Table 1 or the field arithmetic of the register changes as well. This is the same rewrite of `U_r` as for hash-21-1 and is counted once. Scope: Construction, specification and code. Impact: not bounded. Not checked: whether an injective `U_r` exists with the same 2µ calls of `F` per round and the same 32 blank rounds; none is proposed and none is constructed here. The findings concern how words are combined and not the amount of work per round, so the answer follows from the rule that an unbounded impact counts as 10% or more, and not from a count of added work.

## Evidence

- Baseline: Table 1 (physical p. 5) gives `(n, k, v, µ)` = (512, 960, 576, 3), (768, 1216, 832, 4), (1024, 1472, 1088, 5). A non-final block costs 32 blank rounds plus `⌈v/32µ⌉` = 6, 7, 7 output rounds (§3.3, physical p. 9), each with 2µ calls of `F` (§3.1.2 and §3.2, physical pp. 6–8): 38·6 = 228, 39·8 = 312 and 39·10 = 390 calls of `F` per block of 120, 152 and 184 bytes. Control: for a 65,536-byte message these rules give 20,792, 16,854 and 13,930 update calls, the values of Table 7 (physical p. 26). Table 8 (physical p. 26) puts over 98% of the profiled time in the update.
- Specification steps that the root causes touch: the linear core with `(2^13 + 1)·s0`, and `R3 = s0` (§3.1.1–3.1.2, physical pp. 5–6); `Φ = red_p(ℓ_lin + F)` (physical p. 7); `Γ` reads stages 14, 9, 6 and 1 only (§3.2, physical p. 7); `t3`, `t7`, `t11` and `t15` are each `red_p` of an XOR (physical p. 8); the state is initialized "from (V, K)" with no word map given (§3.3, physical p. 9), and `v + k = 512µ` so that the block and chaining value fill the whole state with no conversion layer (§4.2, physical p. 12).
- Source: `Neulaser/Implementations and Test_Vectors/API_CryptHash/Implementations/Reference_Implementation/Neulaser-512/CryptHash_AlgorithmInstance.c:140–149` (feedback), `:166–174` (taps, with `nl_redp32`), `:237–250` (words loaded without reduction). The three reference instance files are byte-identical (`cmp`).
- `python3 -B hash-21/reproduce_long_second_preimage.py` prints 4, 5 and 6 message-loaded merge positions per block and success rates 60.60%, 60.11% and 59.79% for the 12 GB target of hash-21-4.
- Proposed fixes: `report.md` has no "Proposed fixes" subsection. The analysis it cites for hash-21-1, ePrint 2026/2152 Section 4, names the non-injective state update and gives no fix.
