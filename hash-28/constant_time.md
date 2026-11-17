# Constant-time audit — hash-28

Scope: archived reference implementation, fixed digest parameters and fixed message length. Message contents and intermediate state are secret; length-dependent block count is documented but not treated as a candidate-specific vulnerability. The optimized trees (bitsliced C `XRH1280` plus compiler-generated AVX2 assembly) were reviewed on 2026-11-17, and the reference was compiled with GCC and Clang 21 at `-O0` to `-O3` and `-Os`: there are no table lookups, and conditional jumps depend only on the round index and the message length. No microarchitectural measurement was made.

Reference entry: `XRH-1/Implementations/Reference_Implementation/XRH-1-1024/CryptHash_AlgorithmInstance.c:257`.

A fixed-length source pass found no direct secret-dependent branch, table index, or variable-latency divide/remainder in the representative core. Public-parameter and message-length dispatch remains; this is a static review, not a proof of constant time across compilers or architectures.

Status: no issue confirmed in static pass. See the cited source lines; do not infer a full key-recovery or forgery attack from this result alone.

