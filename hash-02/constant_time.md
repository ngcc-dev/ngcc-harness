# Constant-time audit — hash-02

Scope: archived reference implementation, fixed digest parameters and fixed message length. Message contents and intermediate state are secret; length-dependent block count is documented but not treated as a candidate-specific vulnerability. Alternate optimized implementations and microarchitectural measurements remain outside this pass.

Reference entry: `AXIS/Implementations/Reference_Implementation/AXIS-1024/CryptHash_AlgorithmInstance.c:6`.

`axis_core_hash_bits` selects a separate non-byte-aligned path (`axis_core.c:1698-1704`). Its nonlinear update reaches `axis_flip_bit`, whose condition depends on state/message bits (`axis_core.c:160,633-638`); GCC `-O2` retains conditional jumps in the generic path. Byte-aligned fast paths are bit-sliced (`axis_core.c:1592-1596`). Finding: hash-02-2.

Measured on 2026-11-17 with the reference AXIS-512 and the submission's flags: for 501-bit messages the instruction count varies with the message, and an all-zero message runs about 12,900 cycles faster than random ones. `axis_write_digest_bit` (`axis_core.c:1152-1158`) also branches on each digest bit in this path. A 504-bit control shows no dependence. With a measured channel hash-02-2 is rated Medium.

The optimized tree's byte-aligned path indexes 256-entry packing tables with message bytes when BMI2 is not used (`axis_pack_message64_reverse_block`), including the Makefile's portable and AVX2 targets. Finding: hash-02-5 (Low; no timing difference measured).

Status: measured issue (hash-02-2) and source-confirmed issue (hash-02-5). See the cited source lines; do not infer a full key-recovery or forgery attack from this result alone.

