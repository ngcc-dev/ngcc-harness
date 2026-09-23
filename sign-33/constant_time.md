# Constant-time review — 33 VDOO

Scope: reference implementation, representative instance only: `Implementation/Reference_Implementation/vdoo_256/vdoo_sign.c`. Other instances and optimized implementations have not been proven source-equivalent. This is a source-level triage, not a constant-time certification.

Secret: trapdoor/master seed, hidden central map and signing vinegar/solver state. Public: verification key, message and final signature.

- Branch/loop trace: `Implementation/Reference_Implementation/vdoo_256/vdoo_sign.c:22` — `if (coeff != 0)` branches directly on `gfv_get_ele(sk->F, …)`; repeated at lines 34, 86 and 102, with additional secret pivots/retries.
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: Source-confirmed direct key-coefficient branch. This is a side-channel lead; no full-key extraction from timing has yet been demonstrated.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.
