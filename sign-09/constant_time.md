# Constant-time review — 09 DOVE

Scope: reference implementation, representative instance only: `Implementations/Digital_Signature-DOVE-x86-reference_implementation/API_PKC/Implementations/Reference_Implementation/DOVE_classic_ref/SIG_AlgorithmInstance.c`. Other instances and optimized implementations have not been proven source-equivalent. This is a source-level triage, not a constant-time certification.

Secret: trapdoor/master seed, hidden central map and signing vinegar/solver state. Public: verification key, message and final signature.

- Branch/loop trace: `Implementations/Digital_Signature-DOVE-x86-reference_implementation/API_PKC/Implementations/Reference_Implementation/DOVE_classic_ref/SIG_AlgorithmInstance.c:337` — The signer retries after `matrix_solve` on a hidden-key-derived linear system; its UOV Gaussian elimination uses masked conditional addition, so the wrapper retry is the main timing question.
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: No new vulnerability report is promoted from this representative source triage. A compiler/architecture-specific timing and cache test is still needed before claiming constant-time behavior or exploitable leakage.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.
