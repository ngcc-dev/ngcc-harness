# Constant-time audit — hash-15

Scope: archived reference implementation, fixed digest parameters and fixed message length. Message contents and intermediate state are secret; length-dependent block count is documented but not treated as a candidate-specific vulnerability. Alternate optimized implementations and microarchitectural measurements remain outside this pass.

Reference entry: `Litchi/Implementations/Reference_Implementation/Litchi-1024/CryptHash_AlgorithmInstance.c:14`.

A fixed-length source pass found no direct secret-dependent branch, table index, or variable-latency divide/remainder in the representative core. Public-parameter and message-length dispatch remains; this is a static review, not a proof of constant time across compilers or architectures.

Status: no issue confirmed in static pass. See the cited source lines; do not infer a full key-recovery or forgery attack from this result alone.

