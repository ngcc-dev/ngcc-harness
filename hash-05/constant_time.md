# Constant-time audit — hash-05

Scope: archived reference implementation, fixed digest parameters and fixed message length. Message contents and intermediate state are secret; length-dependent block count is documented but not treated as a candidate-specific vulnerability. Alternate optimized implementations and microarchitectural measurements remain outside this pass.

Reference entry: `uHash/Implementations/3 实现代码/Implementations/Reference_Implementation/uHash-1024/CryptHash_AlgorithmInstance.c:1057`.

`RoundFunction` loads `S[X[i]]` and `S[Y[i]]` (`CryptHash_AlgorithmInstance.c:173-174`) after xoring the evolving state with a round key; `S` is a 256-byte table (`:36`). These secret-dependent lookup addresses can select different cache lines. Finding: hash-05-1.

Status: source-confirmed issue. See the cited source lines; do not infer a full key-recovery or forgery attack from this result alone.
