# Constant-time audit — hash-11

Scope: archived reference implementation, fixed digest parameters and fixed message length. Message contents and intermediate state are secret; length-dependent block count is documented but not treated as a candidate-specific vulnerability. Alternate optimized implementations and microarchitectural measurements remain outside this pass.

Reference entry: `Garnet/Implementations/API_CryptHash/Implementations/Reference_Implementation/Garnet/CryptHash_Garnet.c:24`.

AES T-tables use state bytes as indices (`Garnet_1024.c:278-281`; `Garnet_512.c:322-325`). The GF reduction helper also indexes a two-entry table with a state bit (`Garnet_512_512.c:356,382`). The 1 KiB T-tables span cache lines. Finding: hash-11-1.

Status: source-confirmed issue. See the cited source lines; do not infer a full key-recovery or forgery attack from this result alone.

