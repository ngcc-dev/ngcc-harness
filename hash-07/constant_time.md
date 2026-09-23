# Constant-time audit — hash-07

Scope: archived reference implementation, fixed digest parameters and fixed message length. Message contents and intermediate state are secret; length-dependent block count is documented but not treated as a candidate-specific vulnerability. Alternate optimized implementations and microarchitectural measurements remain outside this pass.

Reference entry: `Dragon/Implementations and Test_vector/Dragon-ARM/Implementations/Reference_Implementation/Dragon-1024/CryptHash_AlgorithmInstance.c:94`.

The XOF `copy_bits_msb` helper branches in C on each squeezed output bit (`Dragon-x86/Implementations/Reference_Implementation/Dragon-XOF-256/CryptHash_AlgorithmInstance.c:101`), but GCC `-O2` compiles the branch to `cmovne` in the inspected x86 reference variant. No issue promoted without a data-dependent machine branch or address.

Status: lead/no report. See the cited source lines; do not infer a full key-recovery or forgery attack from this result alone.
