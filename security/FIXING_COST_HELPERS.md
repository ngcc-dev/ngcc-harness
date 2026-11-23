# Fixing-cost helpers

These scripts compute the figures behind the candidates' `fixing_cost.md`
notes, which follow [`FIXING_COST_ANALYSIS.md`](FIXING_COST_ANALYSIS.md). Each
helper sits beside the note it supports and is named `fixing_cost_<topic>.py`.

All of them follow the same conventions:

- Python standard library only. Run them from the repository root with
  `python3 -B <path>`; none writes a file.
- They print counts, sizes, failure rates and cost estimates. None implements
  a candidate, a repaired scheme or an attack.
- Parameter values they print are costing aids for the note, not recommended
  parameters.
- Some read the candidate's submitted sources, headers or data files
  (kem-03, kem-34, kex-03, kex-08, sign-19, sign-22, sign-25 and sign-27).
  Some import a model from elsewhere in the repository:
  [`rank_mm_costs.py`](rank_mm_costs.py) for kem-03, kem-04 and kem-06,
  `kem-16/reproduce_model1_dfr.py`, and `sign-33/reproduce_wedge_estimate.py`.

Run times are for the default command, measured on 2026-11-23 on a 28-core
x86-64 machine, unless marked as stated by the helper.

| Helper | Prints | Run time |
|---|---|---|
| `hash-03/fixing_cost_tree_model.py` | hash-03-1: backward-tree cost exponents and call ratios when z bits of each 736-bit C-Hash-1024 block are reserved | under 1 s |
| `kem-03/fixing_cost_decaps.py` | kem-03-3: field multiplications of reference decapsulation with the submitted support pool and with the specified random supports | under 1 s |
| `kem-03/fixing_cost_params.py` | kem-03-4 and kem-03-6: controls and the BAG-Loong parameter sets found, with sizes, failure rates and attack estimates | 1 s |
| `kem-04/fixing_cost_params.py` | kem-04-3: the smallest BAG-Piglet-128 change that brings the maximal-minors estimates to 2^128 with the submitted encryption weights, with controls | 21 s |
| `kem-04/fixing_cost_enc_weights.py` | kem-04-3: the same with the encryption weights free, checked against the specification's message-instance and quantum formulas | 22 s |
| `kem-06/fixing_cost_params.py` | kem-06-8: the smallest BRA parameters at which the transposed maximal-minors attack reaches the target, with sizes and operation counts | about 12 min on 16 cores (stated); `--controls` 25 s |
| `kem-09/fixing_cost_failure_params.py` | kem-09-5: CheetahKEM failure-rate model and the parameter steps that bring the failure rate down | 34 s |
| `kem-11/fixing_cost_dfr.py` | kem-11-1: the failure rate of specification §3.3, as a union bound over 256 and over n coefficients, beside Table 3 | under 1 s |
| `kem-16/fixing_cost_dfr.py` | kem-16-2: the smallest HARE code length for a raised weight under the Model 1 failure bound, with sizes | 3 min |
| `kem-17/fixing_cost_xof_work.py` | HEP-QC sizes with widened seeds, and SM3 compression counts | under 1 s |
| `kem-18/fixing_cost_unstructured_block.py` | kem-18-3: the unstructured block size that brings the public consistency equations to the level, with sizes and work | under 1 s |
| `kem-19/fixing_cost_hash_work.py` | kem-19-2: Keccak-f[1600] permutations per Lore operation when hash calls move to a sponge of capacity 2λ, in three variants | under 1 s |
| `kem-26/fixing_cost_parameters.py` | kem-26-3 and kem-26-7: NSS-HQC weights, failure model and sizes | 14 s |
| `kem-30/fixing_cost_count.py` | kem-30-1: work of PolarLAC decapsulation with a fixed-work screened sampler | under 1 s |
| `kem-34/fixing_cost_params.py` | kem-34-5: Rudraksh2 size formulas against Table 2, a Gaussian failure model against the six recorded values, and XOF and multiplication counts | 17 s |
| `kem-36/fixing_cost_params.py` | kem-36-9: TRIKE sizes and inversion counts at the costed parameters, checked against Table 3 | under 1 s |
| `kex-03/fixing_cost_hash_count.py` | kex-03-2 and kex-03-4: bytes hashed and SM3 calls of the CreTAKE-S2K initiator's Der step | under 1 s |
| `kex-08/fixing_cost_lv512_key_range.py` | kex-08-2: whether the two hard-coded NIIKE-lv512 secret vectors lie in the range that Algorithm 4 samples from | under 1 s |
| `sign-04/fixing_cost_encoding_count.py` | sign-04-1: constant-sum encoding counts and encoder work for the 160-bit CEDRUS-alpha sets | under 1 s |
| `sign-05/fixing_cost_counts.py` | Chinith signature-size control, bytes hashed per signature and Vistrutith multiplication counts | under 1 s |
| `sign-07/fixing_cost_challenge_weight.py` | sign-07-2: CS challenge weights and the repetition count of Theorem 3.2 | 29 s |
| `sign-11/fixing_cost_grinding.py` | sign-11-1: FlexTree signature sizes, forced-pruning acceptance probabilities and hash-call counts | 5 min |
| `sign-13/fixing_cost_keccak_count.py` | GreatWall signature-size control and Keccak-f[1600] calls per signature at a 1024-bit capacity | under 1 s |
| `sign-15/fixing_cost_params.py` | MORNING-ATLAS sizes, challenge spaces, rejection probabilities and work counts | under 1 s |
| `sign-19/fixing_cost_msg_hash.py` | sign-19-4 and sign-19-8: Phoenix-SM3 signature sizes, expected message-hash trials (sampled, fixed seed) and SM3 compression counts | 3 min |
| `sign-22/fixing_cost_envelope.py` | sign-22-4 and sign-22-8: Rhyme's rejection-envelope condition, its closed form and the constant used in the sources | under 1 s |
| `sign-22/fixing_cost_hash_work.py` | sign-22-1: Keccak-f[1600] permutations of Rhyme-SHAKE signing and verification with a wider message hash | under 1 s |
| `sign-24/fixing_cost_rs_work.py` | sign-24-1: GF(2^16) multiplications of one Reed–Solomon encoding, chunked, global and by a radix-2 transform | under 1 s |
| `sign-25/fixing_cost_verify_work.py` | sign-25-1 and sign-25-3: field-operation counts of SQIsign2D² verification | under 1 s |
| `sign-26/fixing_cost_hash_work.py` | sign-26-2: hash calls added by the specified challenge grinding, against the verifier's field work | under 1 s |
| `sign-27/fixing_cost_params.py` | sign-27-2, sign-27-3 and sign-27-5: SQIsignTriangle prime and response sizes | under 1 s |
| `sign-28/fixing_cost_grind_work.py` | sign-28-1: SYDO grinding hash calls with the specified number of bits | under 1 s |
| `sign-29/fixing_cost_hash_work.py` | sign-29-1 and sign-29-7: Tins signature-size control and SM3 calls per signature and verification | under 1 s |
| `sign-32/fixing_cost_sampler.py` | sign-32-3: UVW signature-size control, the weight-loop success probability of Theorem 2, and the matrix and loop work with a decoding gap | 52 s |
| `sign-33/fixing_cost_sign_work.py` | sign-33-6: expected VDOO signing work with the submitted and the specified oil-term filter | under 1 s |
| `sign-33/fixing_cost_wedge_params.py` | sign-33-9: the smallest VDOO-512 change that brings the wedge estimate to 2^512 | 12 s |

Further modes, cited in the notes:

- `kem-03/fixing_cost_params.py --search 128|256|384|512` redoes one level's
  search, with `--theorem-only` or `--partial-overlap` as variants; the helper
  states about 2, 6, 10 and 25 minutes. The 2026-11-23 assessment stopped the
  512-bit searches before they finished.
- `kem-04/fixing_cost_params.py --search` redoes the BAG-Piglet-128 search
  (about 2 minutes, stated); `kem-04/fixing_cost_enc_weights.py --search` was
  stopped before it finished.
- `kem-06/fixing_cost_params.py --aligned`, `--we 1` and `--wr1`/`--wr2`
  give the alternative ciphertext weights and the size floor (8 to 9 minutes
  each, stated).
- `kem-16/fixing_cost_dfr.py --ceiling` gives the alternative at 2^80
  ciphertexts (several minutes, stated).
