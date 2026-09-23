# Constant-time audit — hash-22

Scope: archived reference implementation, fixed digest parameters and fixed message length. Message contents and intermediate state are secret; length-dependent block count is documented but not treated as a candidate-specific vulnerability. Alternate optimized implementations and microarchitectural measurements remain outside this pass.

Reference entry: `Pavelor/Implementations and Test_Vectors/API_CryptHash/Implementations/Reference_Implementation/Pavelor-1024/CryptHash_AlgorithmInstance.c:290`.

`aes_round_no_key` uses `AES_SBOX[in[i]]` on the evolving state (`CryptHash_AlgorithmInstance.c:77`). The final partial block also calls `xor_bit` on message bits (`:199-200`), and GCC `-O2` retains its conditional branch (`:177`). Finding: hash-22-1.

Status: source-confirmed issue. See the cited source lines; do not infer a full key-recovery or forgery attack from this result alone.
