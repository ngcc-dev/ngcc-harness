# Constant-time review — kem-23 Mito

Scope: representative reference instance `Implementations/Reference_Implementation/Mito-1-256/KEM_AlgorithmInstance.c`; apart from `parameters.h`, the source files are byte-identical in the nine instances, except that Mito-1-128 differs from the other eight by a trailing newline in `KAT_KEM.c` and `KEM_AlgorithmInstance.c` and by one extra `#include "parameters.h"` in `symmetric.h`; `Optimized_Implementation/` is byte-identical to the reference tree (`diff -rq`); compiled machine code is not certified by this source review.

Secret-bearing values: private decapsulation key, recovered message/error state, rejection secret and shared secret. Public values: public key, ciphertext and encoded lengths. A decoder result is not automatically public merely because its ciphertext input is public.

- Source trace: `Implementations/Reference_Implementation/Mito-1-128/vector.c:52` — `mito_kem.c:127-134` passes the persistent PKE seed into `mito_pke_decrypt`; `mito_pke.c:128-134` and `parsing.c:15-19` re-expand it on each decapsulation. The sampler's reject and duplicate branches at `vector.c:52-73` therefore have secret-dependent iteration count.
- Branches, indexed accesses and division/remainder: beyond the traced operand above, the same sampler runs three times in FO re-encryption with a seed derived from the decrypted message (`mito_kem.c:138-142` → `mito_pke.c:93-95` → `vector.c:56,63`), and the Reed–Muller peak search branches on the transform of the decrypted word (`reed_muller.c:147,149`), indexes it by the peak position (`:162-163`) and, in the Mito-E instances, branches on the erasure test (`:242`). Public-seed rejection and fixed-divisor arithmetic are not treated as leaks; absence of other leaks has not been proved.

Assessment: Source-confirmed secret-seed-dependent work, analogous to the TriQ/QUBE findings. A measured timing witness and key-recovery analysis remain open; no constant-time guarantee is made for the decoder.

Reproduction is source/dataflow inspection at the cited location. A remote timing claim needs repeated same-public-input tests with changed secret state and an independent public-value control.
