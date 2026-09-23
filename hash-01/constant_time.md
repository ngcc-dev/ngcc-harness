# Constant-time audit — hash-01

Scope: archived reference implementation, fixed digest parameters and fixed message length. Message contents and intermediate state are secret; length-dependent block count is documented but not treated as a candidate-specific vulnerability. Alternate optimized implementations and microarchitectural measurements remain outside this pass.

Reference entry: `AFS-TrEDM/Implementations/Reference_Implementation/AFS-TrEDM-1024/CryptHash_AlgorithmInstance.c:13`.

The partial-bit framing path calls `set_frame_bit` with each remaining message bit (`afs_tredm.c:215`); `set_lane_bit_msb` branches on that bit (`afs_tredm.c:99`). GCC `-O2` retains a `bt`/`jae` branch. Byte-aligned messages use the separate byte-copy path (`afs_tredm.c:245`). Finding: hash-01-1.

Status: source-confirmed issue. See the cited source lines; do not infer a full key-recovery or forgery attack from this result alone.

