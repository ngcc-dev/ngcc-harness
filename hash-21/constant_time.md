# Constant-time audit — hash-21

Scope: archived reference implementation, fixed digest parameters and fixed message length. Message contents and intermediate state are secret; length-dependent block count is documented but not treated as a candidate-specific vulnerability. Alternate optimized implementations and microarchitectural measurements remain outside this pass.

Reference entry: `Neulaser/Implementations and Test_Vectors/API_CryptHash/Implementations/Reference_Implementation/Neulaser-1024/CryptHash_AlgorithmInstance.c:341`.

`nl_sbox_word` reads a 256-byte S-box at four state-derived indices (`CryptHash_AlgorithmInstance.c:85-90`). `nl_redp64` computes a remainder of state-derived `x` modulo constant `2^32-5` (`:60-63`); constant-divisor code generation is platform-dependent. Finding: hash-21-3.

Status: source-confirmed issue. See the cited source lines; do not infer a full key-recovery or forgery attack from this result alone.

