# Constant-time review — 22 Rhyme

Scope: reference implementation, representative instance only: `Implementations and Test_Vectors/Implementations/Reference_Implementation/Rhyme-SM3/Rhyme-SM3-128/src/sign.c`. Other instances and optimized implementations have not been proven source-equivalent. This is a source-level triage, not a constant-time certification.

Secret: signing-key polynomials/basis, per-signature masks and sampler state. Public: verification key, algorithm parameters, message and serialized signature.

- Branch/loop trace: `Implementations and Test_Vectors/Implementations/Reference_Implementation/Rhyme-SM3/Rhyme-SM3-128/src/sign.c:352` — The loop `while (t >= p) t -= p` has 0–6 iterations on a sum of NTT products involving the private signing basis and secret Gaussian masks.
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: No new vulnerability report is promoted from this representative source triage. A compiler/architecture-specific timing and cache test is still needed before claiming constant-time behavior or exploitable leakage.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.

Division follow-up: `src/sign.c:152-159` computes `a->coeffs[i] % zpntt_prime()`; calls at `:300-308` process the long-term short basis and `s_tail` on every signature. In the Rhyme-SM3-128 reference build, GCC `-O2` emits four `idiv` instructions in `sign.o`. These are secret-operand divides despite a public prime. The key-derived work is fixed across messages, so the source trace and instruction count alone do not show a chosen-message oracle, recoverable coefficient information, or a forgery. This is a high-priority measurement/cryptanalysis lead, not a proved EUF-CMA break.

Recheck from the repository root: set `ref='sign-22/Implementations and Test_Vectors/Implementations/Reference_Implementation/Rhyme-SM3/Rhyme-SM3-128'`, then run `cc -O2 -std=gnu11 -w -fwrapv -DRHYME_NO_AES -DRHYME_MODE=128 -DOUTPUT_BLANK_TEST_VECTORS=0 '-DALGORITHM_INSTANCE="Rhyme-SM3-128"' -I"$ref" -I"$ref/include" -I"$ref/src" -I"$ref/src/keygen" -Iapi -S "$ref/src/sign.c" -o - | rg -c '\bidiv'`; this build prints `4`.

Size-build lead: GCC `-Os` also emits division in `freeze`, `creduce`, and `freeze2q` (`src/reduce.h:20-35`) reached while computing signing intermediates. Distinguish their secret-derived invocations from public-only reduction before making a report.

Table follow-up: `src/sampler.c:343-357` reads `exp_lut[abs(y1[i])]` and later `exp_lut[abs(z1[i]-c[i])]` from a table with hundreds of entries. `y1` is a secret one-time signing mask, while the accepted signature exposes `z1` but not `y1`. A fine-grained table trace could therefore reveal information not simulatable from the signature; whether the observed granularity suffices to recover the mask and forge is open. This is a stronger cryptanalytic lead than the fixed per-key `idiv` timing alone.
