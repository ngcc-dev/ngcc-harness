# Constant-time audit — hash-27

Scope: archived reference implementation, fixed digest parameters and fixed message length. Message contents and intermediate state are secret; length-dependent block count is documented but not treated as a candidate-specific vulnerability. Alternate optimized implementations and microarchitectural measurements remain outside this pass.

Reference entry: `Vedak/Implementations/Reference_Implementation/Vedak-1024/CryptHash_AlgorithmInstance.c:310`.

`apply_S` reads the 256-byte `S_BOX_8` at eight state-derived byte indices per word (`CryptHash_AlgorithmInstance.c:57,167-180` in Vedak-512; `:165-178` in Vedak-768/1024), giving cache-line-dependent addresses. Separately, `set_bit_msb_first` has an `if (bit)` on each input bit (`:160-164,276` in Vedak-512), but GCC `-O2` compiles that path to `cmovne` and fixed-address loads/stores. Finding: hash-27-1 rests on the S-box, not this branch.

Status: source-confirmed issue. See the cited source lines; do not infer a full key-recovery or forgery attack from this result alone.
