# Constant-time audit — hash-03

Scope: archived reference implementation, fixed digest parameters and fixed message length. Message contents and intermediate state are secret; length-dependent block count is documented but not treated as a candidate-specific vulnerability. Alternate optimized implementations and microarchitectural measurements remain outside this pass.

Reference entry: `C Hash/Implementations/Reference_Implementation/CHash_1024/CryptHash_AlgorithmInstance.c:585`.

The core has `SBOX[s[k] & 0x0f]` (`CryptHash_AlgorithmInstance.c:127,144`). This is a secret-dependent address into a 16-byte table. A shared cache line does not prove constant-time behavior: sub-line address channels may exist, and alignment/target architecture matter. No candidate-specific observation or message-recovery channel was established, so this remains an unpromoted lead rather than an exemption for small tables.

Status: lead/no report. See the cited source lines; do not infer a full key-recovery or forgery attack from this result alone.
