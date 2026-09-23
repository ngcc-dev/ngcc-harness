# Constant-time audit — hash-04

Scope: archived reference implementation, fixed digest parameters and fixed message length. Message contents and intermediate state are secret; length-dependent block count is documented but not treated as a candidate-specific vulnerability. Alternate optimized implementations and microarchitectural measurements remain outside this pass.

Reference entry: `CHAMP/Implementations and Test_Vectors/API_CryptHash/Implementations/Reference_Implementation/CHAMP-1024/CryptHash_AlgorithmInstance.c:289`.

The absorption loop indexes a 256-entry matrix table directly with `msg[i]` (`CryptHash_AlgorithmInstance.c:308`); field reduction and inverse dispatch also branch on state (`:186,195,328-331`). Table entries span multiple cache lines. Finding: hash-04-5.

Status: source-confirmed issue. See the cited source lines; do not infer a full key-recovery or forgery attack from this result alone.

