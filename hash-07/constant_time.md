# Constant-time audit — hash-07

Scope: archived reference implementation, fixed digest parameters and fixed message length. Message contents and intermediate state are secret; length-dependent block count is documented but not treated as a candidate-specific vulnerability. Microarchitectural measurements remain outside this pass. The optimized and additional implementations were added on 2026-11-17: they are straight-line 64-bit word operations with no tables.

Reference entry: `Dragon/Implementations and Test_vector/Dragon-ARM/Implementations/Reference_Implementation/Dragon-1024/CryptHash_AlgorithmInstance.c:94`.

The XOF `copy_bits_msb` helper branches in C on each squeezed output bit (`Dragon-x86/Implementations/Reference_Implementation/Dragon-XOF-256/CryptHash_AlgorithmInstance.c:101`), and GCC `-O2` compiles the branch to `cmovne` in the inspected x86 reference variant. Clang does not: Clang 21 at `-O2` keeps `bt`/`jae` on x86-64 and `tbz` on AArch64, and the GitHub issue reports the same for Apple Clang 17. Filed as `hash-07-1` (Side-channel).

A taint run on 2026-11-17 marked the message as undefined under Valgrind memcheck and hashed 17 lengths from 0 to 3,000 bits. The fixed-output reference and optimized builds reported no secret-dependent branch or address with GCC 15 and Clang 21 at `-O0` and `-O2`. The XOF builds reported exactly one, in `copy_bits_msb`, with Clang at both levels and with GCC at `-O0`; GCC `-O2` was clean.

The channel was then measured on the 256-bit XOF: with GCC `-O0` the instruction count of a call is exactly a constant minus twice the output's Hamming weight, and the running time correlates with that weight for GCC `-O0` and Clang `-O2` but not for GCC `-O2`. With a measured channel `hash-07-1` is rated Medium.

Status: `hash-07-1` (Medium) for Clang builds and unoptimized GCC builds; no other issue. See the cited source lines; do not infer a full key-recovery or forgery attack from this result alone.
