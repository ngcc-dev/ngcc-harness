# Constant-time audit — hash-02

Scope: archived reference implementation, fixed digest parameters and fixed message length. Message contents and intermediate state are secret; length-dependent block count is documented but not treated as a candidate-specific vulnerability. Alternate optimized implementations and microarchitectural measurements remain outside this pass.

Reference entry: `AXIS/Implementations/Reference_Implementation/AXIS-1024/CryptHash_AlgorithmInstance.c:6`.

`axis_core_hash_bits` selects a separate non-byte-aligned path (`axis_core.c:1698-1704`). Its nonlinear update reaches `axis_flip_bit`, whose condition depends on state/message bits (`axis_core.c:160,633-638`); GCC `-O2` retains conditional jumps in the generic path. Byte-aligned fast paths are bit-sliced (`axis_core.c:1592-1596`). Finding: hash-02-2.

Status: source-confirmed issue. See the cited source lines; do not infer a full key-recovery or forgery attack from this result alone.

