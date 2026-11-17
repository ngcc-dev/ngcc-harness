# Constant-time audit — hash-16

Scope: archived reference implementation, fixed digest parameters and fixed message length. Message contents and intermediate state are secret; length-dependent block count is documented but not treated as a candidate-specific vulnerability. The reference and optimized AVX2 code of all four instances were run on 2026-11-17 under Valgrind with the message marked undefined (GCC 15 and Clang 21 at `-O0`, `-O2`, `-Os` and `-O3`): no secret-dependent branch or address. The AVX-512 path was read only. Microarchitectural measurements remain outside this pass.

Reference entry: `LLH/Implementations and Test_Vectors/LLH_C/API_CryptHash/Implementations/Reference_Implementation/LLH-1024/CryptHash_AlgorithmInstance.c:50`.

A fixed-length source pass found no direct secret-dependent branch, table index, or variable-latency divide/remainder in the representative core. Public-parameter and message-length dispatch remains; this is a static review, not a proof of constant time across compilers or architectures.

Status: no issue confirmed in static pass. See the cited source lines; do not infer a full key-recovery or forgery attack from this result alone.

