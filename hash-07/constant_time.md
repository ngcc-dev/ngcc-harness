# Constant-time audit — hash-07

Scope: archived reference implementation, fixed digest parameters and fixed message length. Message contents and intermediate state are secret; length-dependent block count is documented but not treated as a candidate-specific vulnerability. Alternate optimized implementations and microarchitectural measurements remain outside this pass.

Reference entry: `Dragon/Implementations and Test_vector/Dragon-ARM/Implementations/Reference_Implementation/Dragon-1024/CryptHash_AlgorithmInstance.c:94`.

The XOF `copy_bits_msb` helper branches in C on each squeezed output bit (`Dragon-x86/Implementations/Reference_Implementation/Dragon-XOF-256/CryptHash_AlgorithmInstance.c:101`), and GCC `-O2` compiles the branch to `cmovne` in the inspected x86 reference variant. Clang does not: Clang 21 at `-O2` keeps `bt`/`jae` on x86-64 and `tbz` on AArch64, and the GitHub issue reports the same for Apple Clang 17. Filed as `hash-07-1` (Low, Side-channel).

Status: `hash-07-1` for Clang builds. See the cited source lines; do not infer a full key-recovery or forgery attack from this result alone.
