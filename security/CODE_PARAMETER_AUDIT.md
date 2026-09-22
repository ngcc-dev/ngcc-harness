# Code-based parameter and decoder audit

Run date: 2026-09-21 UTC. This pass followed the lattice audit with targeted
correctness, entropy, decoder-conformance, failure-oracle, and structured-rank
checks. A clean result below means that the named cheap failure mode was not
found; it is not a proof of the underlying assumption.

## Promoted findings

| ID | Candidate | Result |
|---|---|---|
| `kem-23-1` | Mito-E | All six E variants discard the RM erasure count and locations before errors-only RS decoding; the specified E-variant DFR does not apply to the shipped decoder. |
| `kem-36-1` | TRIKE | The PDF's `max` threshold failed 1,000/1,000 deterministic honest TRIKE-2 trials; the shipped `min` threshold passed 1,000/1,000. |
| `kem-38-1` | UVW-512 | The implemented H1 image is generated from only 256 bits, capping a 512-bit claim at `2^256` seed trials. |
| `kem-38-2` | UVW | Ciphertext mutations reproducibly separate decoder failure (`-2`, about 49.3 s) from validation failure (`-1`, about 0.81 s). |

## Tracks not promoted

- **BAG-Loong, BAG-Piglet, BRA, BRQC, C-Multi-UR-AG.** All parameter sets passed
  deterministic whole-KEM round trips. Key-generation roots scale to at least
  the claimed classical level: BAG-Loong uses 32/64/96/128 bytes,
  BAG-Piglet 16/32/48/64 bytes, and the 512-bit BRA/BRQC/C-Multi-UR-AG sets use
  64 bytes. No deterministic-keyspace ceiling was found.
- **C-Multi-UR-AG RSL thresholds.** For the public-key RSL instance, the
  submitted `(n,w1,N1)` values `(35,8,11)`, `(45,10,15)`, `(67,12,19)` satisfy
  `N1 < n-w1` with margins 16, 20, and 36, and are far below `N1 > n*w1`, the
  cited polynomial threshold. For the ciphertext RSL instance, `N2` is also
  below `n+N1-w2` with margins 21, 33, and 51. These checks rule out the known
  immediate many-sample collapse, not newer blockwise-rank attacks in general.
- **BIKE_MLThre.** The specification and shipped decoder differ in threshold
  coefficients, architecture, arithmetic, action set, and iteration use; the
  512-bit build disables its ML correction. This is a substantial conformance
  problem, but the audit did not obtain a practical DFR or key-recovery witness,
  so no vulnerability report was added.
- **TriQ-KEM.** The PDF explicitly targets post-rejection CDFR bounds only
  `2^-32, 2^-64, 2^-96, 2^-128` and reports `2^-34.32` for TriQ-128 under a
  hard `2^80` query cap. Removing that cap makes the submitted upper-bound
  calculation non-informative at the full ISD budget, but the table supplies
  only an upper bound on actual DFR and the cited recovery needs many failures.
  This remains a parameter/proof concern, not a demonstrated break.
- **UVW correctness.** The PDF's roughly 1.2% value is per randomized decoder
  attempt. The implementation retries up to 1,000 times; 160 deterministic
  complete UVW-128 round trips all succeeded. The per-attempt figure is not an
  honest-KEM failure rate.

## Reproduction

```sh
python3 security/design_parameter_audit.py
python3 security/trike_threshold_differential.py --trials 1000
python3 security/kem_mutation_oracle.py kem-38/lib/libUVW-KEM-128.so --bits 0,846
python3 security/kem_roundtrip_sweep.py kem-38/lib/libUVW-KEM-128.so --trials 160
```
