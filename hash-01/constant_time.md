# Constant-time audit — hash-01

Scope: archived reference implementation, fixed digest parameters and fixed message length. Message contents and intermediate state are secret; length-dependent block count is documented but not treated as a candidate-specific vulnerability. Alternate optimized implementations and microarchitectural measurements remain outside this pass.

Reference entry: `AFS-TrEDM/Implementations/Reference_Implementation/AFS-TrEDM-1024/CryptHash_AlgorithmInstance.c:13`.

The partial-bit framing path calls `set_frame_bit` with each remaining message bit (`afs_tredm.c:215`); `set_lane_bit_msb` branches on that bit (`afs_tredm.c:99`). GCC `-O2` retains a `bt`/`jae` branch. Byte-aligned messages use the separate byte-copy path (`afs_tredm.c:245`). Finding: hash-01-1.

Measured on 2026-11-17 with the reference AFS-TrEDM-512 and the submission's flags: for 501-bit messages the instruction count is exactly a constant plus 11 per message 1-bit, the running time rises by about 2.3 cycles per message 1-bit, and an all-zero message runs about 880 cycles faster than random ones. A 504-bit control shows no dependence. The optimized tree has the same branch. With a measured channel hash-01-1 is rated Medium.

Status: measured issue. See the cited source lines; do not infer a full key-recovery or forgery attack from this result alone.

