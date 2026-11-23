Yes: the specified key generation publishes a basis that exposes the secret isometry (kem-29-2) and the submitted code implements a different scheme whose decoding transform is public (kem-29-1), so repair needs a new key-generation mechanism and a new implementation, and its impact is not bounded at any of the three parameter sets.

Impact: not bounded
Scope: Construction (large), specification and code
Assessed: 2026-11-23

## Attacks considered

- kem-29-1: The submission ships a complete public-key-only break (Implementation, Confirmed)
- kem-29-2: The specified aligned basis directly exposes the secret isometry (Design, Confirmed)
- kem-29-4: Norm-only validation gives one-query ciphertext aliases (Design, Confirmed)

## Remediation

- **kem-29-1.** The submitted code has no trapdoor. The signed permutation that decodes a ciphertext is expanded from the seed stored in the public key (`polarkem_ct.c:419–420`), and encapsulation needs the same permutation (`polarkem_ct.c:351–352`), so the seed cannot simply be made secret. The costed repair is to implement the specified construction (Algorithms 1 to 7): a lattice basis, LLL reduction, an orthogonal matrix and a matrix-vector product. The code contains none of these. Scope: Construction, code. Impact: not bounded. The specification's own figures show the direction. Its size formulas give a public key of 229,376 bytes and a ciphertext of 896 bytes at PolarKEM-128, against the stated 1,024 and 768, and its Table 7 gives O(N^3) key generation and O(N^2) encapsulation, where the code does at most O(N log N) work plus hashing. These are context and not an impact figure: the public-key formula assumes an HNF encoding that Algorithm 1 does not define, and the kem-29-2 repair changes the public key again.
- **kem-29-2.** Algorithm 1 publishes `B_pk = O·B_red`, where `B_red` is computed from public parameters and no other secret is sampled. The report records no proposed fix. The specification's own problem statement has a unimodular change of basis `U` (Definition 2.6, physical p. 9; Definition 5.1, physical p. 18) that Algorithm 1 never samples. A repair has to hide the basis correspondence in some such way. That changes key generation, the public-key representation and `Embed`, and the decoding and size analysis must be redone. The report notes that the HNF remark of §6.1.1 defines no conversion and does not apply to a real orthogonal `O`. Scope: Construction, specification and code. Impact: not bounded. No repaired construction is specified, so none was costed.
- **kem-29-4.** Algorithm 7 accepts on the norm of `ê` alone and derives `K = Extract(m̂)` without the ciphertext. The costed repair is the one the report states: rebuild the encapsulation coins in decapsulation, compare the received ciphertext byte for byte with the re-encryption, and bind the ciphertext into the key. It changes Algorithm 6 (lines 2 and 6, so that the coins follow from the recovered value) and Algorithm 7 (lines 3 to 7). Scope: Function, specification. Impact: none. The submitted code already contains the check: it re-encrypts and compares every ciphertext byte (`polarkem_core.c:96–108`, `polarkem_ct.c:511–516`) and derives the key from `mu` and a hash of the ciphertext (`polarkem_ct.c:443–475`), so the baseline work and the stated ciphertext size include it. In the specified construction, Algorithm 7 line 2 already computes `Embed(m̂, pk)`. The added error sampling, hash and comparison are O(N) beside the O(N^2) matrix-vector product of `Embed` (§6.2.2, physical p. 23). The report's text also derives the key from a hash of the public key. The submitted code hashes its 1,024-byte public key in every encapsulation and decapsulation (`polarkem_ct.c:347`); for a matrix public key of the specified size that hash was not costed.

## Evidence

- Baseline sizes, Table 6 (physical p. 22): public key 1,024 / 2,048 / 4,096 bytes and ciphertext 768 / 1,280 / 2,304 bytes for PolarKEM-128 / -256 / -512. The built constants are the same (`polarkem_params.h:23–25` of each set). In the code, 976 of the 1,024 public-key bytes of PolarKEM-128 are padding (`polarkem_params.h:41–43,68`).
- Control: the size formulas of §6.1.1 and §6.1.3 (physical p. 23) do not reproduce Table 6. With Table 1 (physical p. 13), N = 512, 1024, 2048 and ⌈log2 q⌉ = 14, 14, 15 for q = 12289, 12289, 18433:
  - `|pk| = N^2/2 · ⌈log2 q⌉` bits = 229,376 / 917,504 / 3,932,160 bytes.
  - `|c| = N · ⌈log2 q⌉` bits = 896 / 1,792 / 3,840 bytes.
- Operation counts: Table 7 (physical p. 23) gives KeyGen O(N^3), Encaps O(N^2) and Decaps O(N log N). The specification states no timings. In the code, key generation draws two 32-byte seeds and fills padding (`polarkem_core.c:23–32`).
- Algorithm 1 is on physical p. 13 and Algorithms 5 to 7 on physical p. 17. Source paths are under `Submission_Package/Implementations/Reference_Implementation/PolarKEM-128/`.
- `python3 -B kem-29/reproduce_spec_alignment.py` and `python3 -B kem-29/reproduce_spec_defects.py` were run and confirm the Algorithm 1 assignments and the norm-only acceptance of Algorithm 7.
- Not checked: the optimized tree, beyond its use of the same public seed (`Submission_Package/Implementations/Optimized_Implementation/PolarKEM-128/polarkem_ct.c:356`); the two Markdown documents shipped beside the PDF; the cited ePrint papers, which were not read for repair proposals.
