No: the only Critical findings are the Leads hash-32-2 and hash-32-3, which do not decide the answer, and the repair costed for them replaces shift offsets in θ and ρ without adding an operation, a round or a permutation call.

Impact: none
Scope: none
With leads: No; none; Function (small), specification and code
Assessed: 2026-11-23

## Attacks considered

- hash-32-2: Message modification gives a conditional 2^288 collision estimate (Design, Lead)
- hash-32-3: Sparse ZC-1280 trail gives a conditional 2^144 collision estimate (Design, Lead)

## Remediation

- **hash-32-2.** Root cause: the ZC-1536 linear layer fixes the difference 0x1111111111111111 in A0[0]. ρ leaves plane 0 in place, and the two θ parity shifts, 20 and 56 (physical p. 9), differ by 36, so the two rotated copies cancel on a lane pattern of period 4. report.md records the ZC-DM team's v1.1 notice, which revises the shift offsets of the shared permutations for ZC-DM only; no ZC-EDMC revision has been posted. The costed repair is a standard change of a constant: one θ parity shift is replaced so that the two copies no longer cancel on that pattern. θ still rotates each parity lane twice, so no operation is added. The round function is shared by both six-round halves, so the repair is the same for the specified mapping and the implemented one. Scope: Single line, specification and code. Impact: none. No replacement shift was chosen, and whether one leaves other sparse fixed differences was not checked.
- **hash-32-3.** Root cause: the ZC-1280 linear layer fixes a difference on eight χ columns. A difference of that shape follows from the plane-2 offset: ρ applied twice moves plane 2 by 2 × (3, 24) ≡ (2, 48) on the 4 × 64 lane grid, a shift of order 4, so four plane-2 bits return to themselves and share columns with four bits of the unmoved plane 0 in between. report.md records the ZC-DM team's v1.1 notice, which revises the shift offsets of the shared permutations for ZC-DM only; no ZC-EDMC revision has been posted. The costed repair is a standard change of constants: new ρ offsets for ZC-1280, at least on plane 2. Every lane of planes 1 to 4 is already rotated in each ρ step, so new offsets add no operation. The round function is shared by both six-round halves, so the repair is the same for the specified mapping and the implemented one. Scope: Function, specification and code. Impact: none. No offsets were chosen or checked. The attachment of issue #37 was not read, so the match between the shape above and the reported difference rests on the report's description.

## Evidence

- Baseline: Table 1.2 (physical p. 13) gives c = n + 64 and r = r′ = m − c, that is 704, 448 and 192 bits for ZC-1280 and 960, 704 and 448 bits for ZC-1536; control 1536 − (512 + 64) = 960. Each r-bit block takes two six-round halves with the capacity feed-forward between them, 12 rounds in all (physical p. 13; Algorithm 3, physical p. 15; `ZC-EDMC/Implementations/Implementations/Reference_Implementation/ZC-EDMC-1536-512/CryptHash_AlgorithmInstance.c:30,89,92,94,119,123`). No repair changes the rate, the rounds or the calls. A hash has no bandwidth.
- The specification gives batch run times only (Table 4.1, physical p. 31) and no breakdown by step, so the operation counts are the whole evidence. Nothing was measured.
- ZC-1536 round, Algorithm 2 (physical p. 10) and `ZC-EDMC/Implementations/lib/low/ZuD-1536/plain/ZuD1536-plain.c`: 73 XOR + 24 AND + 24 NOT + 32 ROTL64 = 153 logic operations (:155–229, ρ called twice at :267 and :269). θ has 16 ROTL64, two per parity lane (:163–170); another shift value leaves that count at 16.
- ZC-1280 round, Algorithm 1 (physical p. 10) and `ZC-EDMC/Implementations/lib/low/ZuD-1280/plain/ZuD1280-plain.c`: 61 XOR + 20 AND + 20 NOT + 40 ROTL64 = 141 logic operations (:155–221). ρ rotates all 16 lanes of planes 1 to 4 (:180–195) by 5, 24, 1 and 3 bits (physical p. 7), and is called twice (:252, :254); other nonzero offsets leave that count at 32 per round.
- Not costed as a repair: more rounds, which leave the fixed differences in place. At each finding's own per-round rate the collision target needs 32(nr − 3) ≥ 384, so nr ≥ 15 (hash-32-2 at -768), and 16(nr − 3) ≥ 256, so nr ≥ 19 (hash-32-3). That is +25% and +58% per byte against 12 rounds per block.
