# Constant-time audit — hash-08

Scope: archived reference implementation, fixed digest parameters and fixed message length. Message contents and intermediate state are secret; length-dependent block count is documented but not treated as a candidate-specific vulnerability. The optimized AVX2, AVX-512 and NEON trees were reviewed on 2026-11-17. Valgrind with the message marked undefined reported no secret-dependent branch or address in 90 GCC and Clang builds (reference, AVX2, and NEON through a scalar shim); AVX-512 was read only. Microarchitectural measurements remain outside this pass.

Reference entry: `Duet/Implementations/Reference_Implementation/Duet-1024/CryptHash_Duet-1024.c:412`.

A fixed-length source pass found no direct secret-dependent branch, table index, or variable-latency divide/remainder in the representative core. Public-parameter and message-length dispatch remains; this is a static review, not a proof of constant time across compilers or architectures.

Status: no issue confirmed in static pass. See the cited source lines; do not infer a full key-recovery or forgery attack from this result alone.

