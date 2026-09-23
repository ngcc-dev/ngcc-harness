# Constant-time audit — hash-32

Scope: archived reference implementation, fixed digest parameters and fixed message length. Message contents and intermediate state are secret; length-dependent block count is documented but not treated as a candidate-specific vulnerability. Alternate optimized implementations and microarchitectural measurements remain outside this pass.

Reference entry: `ZC-EDMC/Implementations/Implementations/Reference_Implementation/ZC-EDMC-1280-1024/CryptHash_AlgorithmInstance.c:103`.

The wrapper absorbs full blocks bytewise (`CryptHash_AlgorithmInstance.c:128-132`). Its bit-append helper branches only on fixed domain/padding bits (`:53-68,158-187`), not message data; the included `ZuD1280-plain.c:77-92` loops over public byte counts. No secret-dependent lookup or divide was found in this path. Other variants are not proved constant time.

Status: no issue confirmed in static pass. See the cited source lines; do not infer a full key-recovery or forgery attack from this result alone.
