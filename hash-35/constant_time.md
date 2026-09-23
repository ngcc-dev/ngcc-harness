# Constant-time audit — hash-35

Scope: archived reference implementation, fixed digest parameters and fixed message length. Message contents and intermediate state are secret; length-dependent block count is documented but not treated as a candidate-specific vulnerability. Alternate optimized implementations and microarchitectural measurements remain outside this pass.

Reference entry: `Wish/Implementations/Reference_Implementation/Wish1024_reference/CryptHash_AlgorithmInstance.c:16`.

`SubBytes` indexes the 256-byte `S_Box` by secret state (`wish.c:96-99`); `GF_Mul` has a state-dependent loop bound and branch (`wish.c:86-91`). Both are reached in every permutation. Finding: hash-35-1.

Status: source-confirmed issue. See the cited source lines; do not infer a full key-recovery or forgery attack from this result alone.

