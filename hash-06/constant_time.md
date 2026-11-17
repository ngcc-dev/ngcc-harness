# Constant-time audit — hash-06

Scope: archived reference implementation, fixed digest parameters and fixed message length. Message contents and intermediate state are secret; length-dependent block count is documented but not treated as a candidate-specific vulnerability. All trees were checked on 2026-11-17. The reference (GCC and Clang 21 at `-O0` to `-O3` and `-Os`), the x86 assembly and the ARM builds are clean under Valgrind with the message marked undefined. The AVX-512 code, which Valgrind cannot run, was read: every conditional depends on lengths only. There are no lookup tables. Microarchitectural measurements remain outside this pass.

Reference entry: `Cuishen/Implementations/Reference_Implementation/Cuishen-1024/CryptHash_Cuishen-1024.c:358`.

A fixed-length source pass found no direct secret-dependent branch, table index, or variable-latency divide/remainder in the representative core. Public-parameter and message-length dispatch remains; this is a static review, not a proof of constant time across compilers or architectures.

Status: no issue confirmed in static pass. See the cited source lines; do not infer a full key-recovery or forgery attack from this result alone.

