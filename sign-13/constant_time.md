# Constant-time review — 13 GreatWall

Scope: reference implementation, representative instance only: `Implementations/Reference_Implementation/GreatWall256s/SIG_AlgorithmInstance.c`. Other instances and optimized implementations have not been proven source-equivalent. This is a source-level triage, not a constant-time certification.

Secret: witness/secret key and per-proof random tapes until revealed. Public: verification key, message, Fiat–Shamir challenge and opened transcript.

- Branch/loop trace: `Implementations/Reference_Implementation/GreatWall256s/SIG_AlgorithmInstance.c:54` — The wrapper checks a signing return code; the actual Pylon/Faest witness path is deeper in the reference tree and is not certified by this check.
- Secret-dependent field arithmetic: `faest_sign` (`GreatWall256s/faest.c:518`) recomputes the witness from the secret key on every call through `faest_unpack_secret_key` → `faest_compute_witness` (`faest.c:73,146`), which raises key-derived states to fixed exponents (`faest.c:261,277`). The reference multiplication `ref_gf_mul` branches on each bit of one operand (`block_impl.h:93`) and `ref_reduce_poly` branches on each high product bit (`block_impl.h:74`). `block_impl.h` is identical in all eight reference trees; the optimized `block_impl.h` uses `_mm_clmulepi64_si128` and has no `ref_gf_mul`. This is a source-level observation; it has not been measured.
- Table S-box: the reference AES (`GreatWall256s/aes_impl.c:6`) indexes `aes_sbox` with key bytes in key expansion (`aes_impl.c:81–88`) and with state bytes in `sub_bytes_nb` (`aes_impl.c:39`); the AES-CTR PRG is keyed with GGM seeds (`prgs.h:68`).
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: No new vulnerability report is promoted from this representative source triage. A compiler/architecture-specific timing and cache test is still needed before claiming constant-time behavior or exploitable leakage.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.
