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
  (hash-31, hash-32, kem-03, kem-04, kem-34, kex-03, kex-08, sign-14, sign-19,
  sign-21, sign-22, sign-25 and sign-27).
  Some import a model from elsewhere in the repository:
  [`rank_mm_costs.py`](rank_mm_costs.py) for kem-03, kem-04 and kem-06,
  `kem-16/reproduce_model1_dfr.py`, `sign-33/reproduce_wedge_estimate.py`,
  the hash-31 helper for hash-32, and two kem-04 helpers for the
  BAG-Piglet-256 one.

Run times are for the default command, measured on 2026-11-23 or 2026-11-24
on a 28-core x86-64 machine, unless marked as stated by the helper.

| Helper | Prints | Run time |
|---|---|---|
| `hash-03/fixing_cost_tree_model.py` | hash-03-1: backward-tree cost exponents and call ratios when z bits of each 736-bit C-Hash-1024 block are reserved | under 1 s |
| `hash-31/fixing_cost_linear_layer.py` | hash-31-2, hash-31-3 and hash-31-4: whether the ZC-1536 and ZC-1280 linear layers, as submitted (offsets read from the sources) and with the ZC-DM v1.1 offsets, fix the findings' differences; θ cancellation periods, plane-2 orbit orders and fixed-space dimensions; with `--min-weight`, the lightest difference a v1.1 layer fixes | under 1 s; `--min-weight` 27 s |
| `hash-32/fixing_cost_linear_layer.py` | hash-32-2 and hash-32-3: the same checks with the offsets read from the ZC-EDMC sources, through the hash-31 helper | under 1 s; `--min-weight` 28 s |
| `kem-02/fixing_cost_g_work.py` | kem-02-8: SM3 compressions of Amoeba-1728 and -2304 encapsulation, as built and with the contest XOF idealized, and Keccak-f[1600] calls of G on a capacity-2λ sponge, in word operations | under 1 s |
| `kem-03/fixing_cost_decaps.py` | kem-03-3: field multiplications of reference decapsulation with the submitted support pool and with the specified random supports | under 1 s |
| `kem-03/fixing_cost_params.py` | kem-03-4 and kem-03-6: controls and the BAG-Loong parameter sets found, with sizes, failure rates and attack estimates | 1 s |
| `kem-04/fixing_cost_decaps_count.py` | kem-04-3: BAG-Piglet-128 decapsulation field multiplications and squarings (ring products, encoder, decoder) at the submitted code and the two costed sets, under four cost models | under 1 s |
| `kem-04/fixing_cost_enc_weights.py` | kem-04-3: the same with the encryption weights free, checked against the specification's message-instance and quantum formulas | 22 s |
| `kem-04/fixing_cost_params.py` | kem-04-3: the smallest BAG-Piglet-128 change that brings the maximal-minors estimates to 2^128 with the submitted encryption weights, with controls | 21 s |
| `kem-04/fixing_cost_piglet256.py` | kem-04-3: the smallest BAG-Piglet-256 change that brings the ordinary-MM and ZD-MM key-recovery estimates, unpacked and packed over `F_(2^d)`, and the specification's formulas to 2^256, with controls, sizes and decapsulation counts | 22 s |
| `kem-06/fixing_cost_attack3n.py` | kem-06-8: the specification's Attack-3n (Table 6) and AGHT and PRR (Table 7) controls, and their values at the low-end and high-end costed BRA sets | about 2.5 min on 9 cores |
| `kem-06/fixing_cost_params.py` | kem-06-8: the smallest BRA parameters at which the transposed maximal-minors attack reaches the target, with sizes and operation counts | about 12 min on 16 cores (stated); `--controls` 25 s |
| `kem-09/fixing_cost_failure_params.py` | kem-09-5: CheetahKEM failure-rate model and the parameter steps that bring the failure rate down | 34 s |
| `kem-11/fixing_cost_dfr.py` | kem-11-1: the failure rate of specification §3.3, as a union bound over 256 and over n coefficients, beside Table 3 | under 1 s |
| `kem-16/fixing_cost_dfr.py` | kem-16-2: the smallest HARE code length for a raised weight under the Model 1 failure bound, with sizes | 3 min |
| `kem-17/fixing_cost_xof_work.py` | HEP-QC sizes with widened seeds, and SM3 compression counts | under 1 s |
| `kem-18/fixing_cost_unstructured_block.py` | kem-18-3: size and failure-formula controls against Tables 3 and 2, and the unstructured block size that brings the public consistency equations to the level under the primal model and the recorded dual-hybrid estimate, with sizes, failure rates and work | under 1 s |
| `kem-19/fixing_cost_hash_work.py` | kem-19-2: Keccak-f[1600] permutations per Lore operation when hash calls move to a sponge of capacity 2λ, in three variants, and SM3-compression equivalents of the SM3 backend as built and idealized | under 1 s |
| `kem-26/fixing_cost_parameters.py` | kem-26-3, kem-26-5, kem-26-6 and kem-26-7: NSS-HQC weights, failure model and sizes, decapsulation with the decoder share, fixed-iteration Berlekamp–Massey decoder counts, and the 2^80 multi-instance bound | 18 s |
| `kem-27/fixing_cost_keygen_work.py` | kem-27-1: word operations of NTRE-512 key generation and its SM3, XOF and DRBG work with a 32- and a 64-byte seed, as built and idealized by calls and bytes | under 1 s |
| `kem-30/fixing_cost_count.py` | kem-30-1: work of PolarLAC decapsulation with a fixed-work screened sampler | under 1 s |
| `kem-34/fixing_cost_params.py` | kem-34-5: Rudraksh2 size formulas against Table 2, a Gaussian failure model against the six recorded values, and XOF and multiplication counts grouped to bound each whole operation | 18 s |
| `kem-36/fixing_cost_params.py` | kem-36-9: TRIKE sizes, inversion counts and failure-rate bounds from the specification's §3.1 law at the costed parameters, checked against Tables 3 and 8 | under 1 s |
| `kex-03/fixing_cost_hash_count.py` | kex-03-2, kex-03-4 and kex-03-9: bytes hashed and SM3 calls of the CreTAKE-S2K initiator's Der step, and the BiT-128 public-key and signature sizes stated for the BiT team's revision | under 1 s |
| `kex-08/fixing_cost_lv512_key_range.py` | kex-08-2: whether the two hard-coded NIIKE-lv512 secret vectors lie in the range that Algorithm 4 samples from | under 1 s |
| `sign-04/fixing_cost_encoding_count.py` | sign-04-1: constant-sum encoding counts and encoder work for the 160-bit CEDRUS-alpha sets | under 1 s |
| `sign-05/fixing_cost_counts.py` | Chinith signature-size control, bytes hashed per signature and Vistrutith multiplication counts | under 1 s |
| `sign-07/fixing_cost_challenge_weight.py` | sign-07-2: CS challenge weights and the repetition count of Theorem 3.2 | 29 s |
| `sign-11/fixing_cost_grinding.py` | sign-11-1: FlexTree signature sizes, forced-pruning acceptance probabilities and hash-call counts | 5 min |
| `sign-13/fixing_cost_keccak_count.py` | GreatWall signature-size control and Keccak-f[1600] calls per signature at a 1024-bit capacity | under 1 s |
| `sign-14/fixing_cost_tweak_encoding.py` | sign-14-2 and sign-14-3: free TCCR and PRG key and block bits per Lynxer set, tree size and index width, unchanged cipher-call counts, and tweak sharing when the position is XORed into the 120-bit ivsig | under 1 s |
| `sign-15/fixing_cost_params.py` | MORNING-ATLAS sizes, challenge spaces, rejection probabilities and work counts | under 1 s |
| `sign-19/fixing_cost_msg_hash.py` | sign-19-4 and sign-19-8: Phoenix-SM3 signature sizes, expected message-hash trials (sampled, fixed seed) and SM3 compression counts | 3 min |
| `sign-21/fixing_cost_tweak_encoding.py` | sign-21-1 and sign-21-2: the same for ReSolveD-α | under 1 s |
| `sign-22/fixing_cost_envelope.py` | sign-22-4 and sign-22-8: Rhyme's rejection-envelope condition, its closed form and the constant used in the sources | under 1 s |
| `sign-22/fixing_cost_hash_work.py` | sign-22-1: Keccak-f[1600] permutations of Rhyme-SHAKE signing and verification with a wider message hash | under 1 s |
| `sign-24/fixing_cost_rs_work.py` | sign-24-1: GF(2^16) multiplications, additions and table lookups of one Reed–Solomon encoding, chunked, global, by a radix-2 transform and by the team's order-51 coset transform, with table size and setup | under 1 s |
| `sign-25/fixing_cost_verify_work.py` | sign-25-1 and sign-25-3: field-operation counts of SQIsign2D² verification, and the steps added to the compact signer | under 1 s |
| `sign-26/fixing_cost_hash_work.py` | sign-26-2: hash calls added by the specified challenge grinding, against a lower bound on the field work that the verifier and the signer each run once | under 1 s |
| `sign-27/fixing_cost_params.py` | sign-27-2, sign-27-3 and sign-27-5: SQIsignTriangle prime and response sizes | under 1 s |
| `sign-28/fixing_cost_grind_work.py` | sign-28-1: SYDO grinding hash calls with the specified number of bits | under 1 s |
| `sign-29/fixing_cost_hash_work.py` | sign-29-1 and sign-29-7: Tins signature-size control and SM3 calls per signature and verification | under 1 s |
| `sign-30/fixing_cost_transcript_work.py` | sign-30-3: Keccak-f[1600] calls added by a 1024-bit-capacity TRINE-512 transcript hash, against a lower bound on the field and XOF work of signing and verification, in word operations | under 1 s |
| `sign-32/fixing_cost_sampler.py` | sign-32-3: UVW signature-size control, the weight-loop success probability of Theorem 2, and the matrix and loop work with a decoding gap, keeping one information set or drawing one per trial | 55 s |
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
