# Constant-time audit — hash-12

Scope: archived reference implementation, fixed digest parameters and fixed message length. Message contents and intermediate state are secret; length-dependent block count is documented but not treated as a candidate-specific vulnerability. Alternate optimized implementations and microarchitectural measurements remain outside this pass.

Reference entry: `Iphe/Implementations/Reference_Implementation/Iphe-1024/CryptHash_AlgorithmInstance.c:136`.

An earlier fixed-length source pass of the permutation found no direct secret-dependent branch, table index, or variable-latency divide/remainder. It missed the final-block loader: `load_final_word` (`CryptHash_AlgorithmInstance.c:110-126`) branches on each message bit of the final block in all three trees, and the effect is measurable (instruction count exactly a constant plus 3 per message 1-bit at GCC `-O2`; timing differences of up to 64 standard deviations). Finding: hash-12-2 (Medium). Public-parameter and message-length dispatch remains; this is a static review, not a proof of constant time across compilers or architectures.

Status: measured issue (hash-12-2). See the cited source lines; do not infer a full key-recovery or forgery attack from this result alone.

