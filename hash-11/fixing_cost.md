No: the repair of hash-11-3 keeps the byte counts and loop indices of the Garnet reference code in 64-bit types, and it changes no digest length and no work per block for Garnet-512, Garnet-768 or Garnet-1024.

Impact: none
Scope: Function (small), code
Assessed: 2026-11-23

## Attacks considered

- hash-11-3: 32-bit byte counts give collisions between 4 GiB messages and crash on 2 GiB messages (Implementation, Confirmed)

## Remediation

- **hash-11-3.** The root cause is 32-bit length arithmetic. `pad_message` holds the message byte count in an `int`, and the absorption loops index with `int i` and compare against `(int)byte_num` (`Garnet_512_896.c:578,582,584,628,664,682`, and the same in the other four Garnet-512 rate files). Garnet-768 and Garnet-1024 have the `int` count in `pad_message` and an `int` loop index against a 64-bit count (`Garnet_768.c:661,756,787`; `Garnet_1024.c:688,783,814`). The specification covers messages of up to 2^64 − 1 bits. The repair is 64-bit length arithmetic: `uint64_t` or `size_t` counts and indices in `pad_message`, `pad_message2` where a file has it, and the hash routine of each file, with the `(int)` casts removed. This is a standard change that makes the code follow the specification. Scope: Function, code; declarations and casts in two or three routines of each of the seven reference files, and the `int` count at `hash_garnet_universal.c:442` in the additional implementation. The specification stays as it is. Impact: none. Only types change. The same rate blocks are absorbed with one P^b call each, and a message short enough for the `int` counts to be exact keeps its digest. The report records no proposed fix for this finding. Not checked: the optimized assembly implementations and their `main.c` drivers; the other defects that the report notes in `hash_garnet_universal.c` (it does not compile as shipped and pads the caller's buffer in place); no repair was written or run.

## Evidence

- Baseline: a hash candidate has no bandwidth. The specification's timings were not used; the block count is the whole evidence.
- Specification: the maximal message length is 2^64 − 1 bits (§1, physical p. 1, and §2, physical p. 3). The Sponge absorbing update is S^(j+1) = P^b(S^j ⊕ I_r(M'_j)), one P^b call per rate block (§2.5, physical p. 9).
- Source: the wrapper already holds the byte length in `size_t` (`Garnet_512_896.c:735`), and the hash routine holds `byte_num` in `uint64_t` (`:635`; `u64` at `Garnet_768.c:760` and `Garnet_1024.c:787`). The 32-bit values are the `int byte_num` of `pad_message` and `pad_message2` (`Garnet_512_896.c:578,606`), the loop index `int i` (`:628`) and the casts (`:664,682`).
- Arithmetic, from the report: 2^35 + 128 bits is 2^32 + 16 bytes, which an `int` holds as 16.
- Work per block: the absorb loop calls the round function 10 times per 112-byte block at the 896-bit rate of Garnet-512 (`Garnet_512_896.c:664-677`). The repair changes the type of `i` in that loop and nothing inside it.
- Skipped work: below 2^31 bytes the block count is the same as now. From 2^31 bytes on, the submitted code crashes or miscounts, and at 2^32 + 16 bytes Garnet-512 absorbs 16 bytes only (report). The corrected code absorbs every block of such a message at the same work per block, so the cost per byte of a long message does not change.
