# Constant-time audit — hash-33

Scope: archived reference implementation, fixed digest parameters and fixed message length. Message contents and intermediate state are secret; length-dependent block count is documented but not treated as a candidate-specific vulnerability. Microarchitectural measurements remain outside this pass. The optimized and additional implementations were added on 2026-11-17: they are straight-line 64-bit word operations with no tables.

Reference entry: `Thunder/Implementations and Test_vector/Thunder-ARM/Implementations/Reference_Implementation/Thunder-1024/CryptHash_AlgorithmInstance.c:175`.

The XOF output-bit copy has `if (src_bit)` (`Thunder-X86/Implementations/Reference_Implementation/Thunder-XOF-256/CryptHash_AlgorithmInstance.c:187`). GCC `-O2` compiles this to a `cmovne` with fixed-address accesses, so that source branch is not a compiled branch in the GCC build. Clang 21 at `-O2` keeps `bt`/`jae` on x86-64 and `tbz` on AArch64, and the GitHub issue reports the same for Apple Clang 17. Filed as `hash-33-1` (Low, Side-channel).

A taint run on 2026-11-17 marked the message as undefined under Valgrind memcheck and hashed 17 lengths from 0 to 3,000 bits. The fixed-output reference and optimized builds reported no secret-dependent branch or address with GCC 15 and Clang 21 at `-O0` and `-O2`. The XOF builds reported exactly one, in `copy_bits_msb`, with Clang at both levels and with GCC at `-O0`; GCC `-O2` was clean.

Status: `hash-33-1` for Clang builds and unoptimized GCC builds; no other issue. See the cited source lines; do not infer a full preimage attack from this result alone.
