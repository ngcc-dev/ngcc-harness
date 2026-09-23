# Constant-time audit — hash-33

Scope: archived reference implementation, fixed digest parameters and fixed message length. Message contents and intermediate state are secret; length-dependent block count is documented but not treated as a candidate-specific vulnerability. Alternate optimized implementations and microarchitectural measurements remain outside this pass.

Reference entry: `Thunder/Implementations and Test_vector/Thunder-ARM/Implementations/Reference_Implementation/Thunder-1024/CryptHash_AlgorithmInstance.c:175`.

The XOF output-bit copy has `if (src_bit)` (`Thunder-X86/Implementations/Reference_Implementation/Thunder-XOF-256/CryptHash_AlgorithmInstance.c:187`). GCC `-O2` compiles this to a `cmovne` with fixed-address accesses, so that source branch is not a compiled branch in the reviewed build. Other compilers and targets still require checking.

Status: no report for the reviewed GCC build. See the cited source lines; do not infer a full preimage attack from this result alone.
