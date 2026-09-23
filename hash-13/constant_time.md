# Constant-time audit — hash-13

Scope: archived reference implementation, fixed digest parameters and fixed message length. Message contents and intermediate state are secret; length-dependent block count is documented but not treated as a candidate-specific vulnerability. Alternate optimized implementations and microarchitectural measurements remain outside this pass.

Reference entry: `Juzi/Implementations/Implementation/Reference_Implementation/JuziHash-1024/CryptHash_AlgorithmInstance.c:312`.

A fixed-length source pass finds `JUZI_S4[r0]`, `JUZI_S4[r1]`, and `JUZI_S4[r2]` in `juzi_sbox8` (`CryptHash_AlgorithmInstance.c:86,95-97`): all indices derive from secret state. This is the same small-table class as hash-03. A one-cache-line table is not automatically safe against sub-line observation, but no candidate-specific observation or message-recovery channel was established. Public-parameter and message-length dispatch is separate; this is a static review, not a proof of constant time across compilers or architectures.

Status: small secret-indexed table lead, not a demonstrated attack. See the cited source lines; do not infer a full preimage attack from this result alone.
