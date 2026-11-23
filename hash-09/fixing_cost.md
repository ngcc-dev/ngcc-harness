No: the repair of hash-09-1 gives the padding bit one bit order at every message length, and it changes no block count and no permutation call in any of the five Eijen instances.

Impact: none
Scope: Function (small), specification and code
Assessed: 2026-11-23

## Attacks considered

- hash-09-1: Trivial collisions in all Eijen implementations (Implementation, Confirmed)

## Remediation

- **hash-09-1.** The root cause is that `eijen_final_partial` writes the padding bit in two bit orders: `0x01` after a byte-aligned message, and `0x80 >> partial_bits` after a partial byte (`eijen_ref.c:179-185`, `eijen_avx2.c:251-257`). After seven zero bits both branches write the byte `0x01`. The repair is the one that the finding names: one bit order in both branches, and a sentence in the specification that fixes it. Two forms are open. The partial byte can follow the LSB-first order of the specification's worked example, which keeps every byte-aligned digest. Or the byte-aligned branch can write `0x80`, which changes every byte-aligned digest and the worked example. Each changes one to three statements in the finalization code of each source file, and each is a standard bit-order convention. Scope: Function, specification and code. The specification changes because §2.3 defines `pad10*` on bit strings, and the order of bits within a byte appears only in the "abc" example of Appendix B. Impact: none. The padded length |M| + 1 + k and the number of blocks are the same in either form, so every message takes the same permutation calls as in the submitted code. The report records no proposed fix for this finding. Not checked: which of the two orders the submitters intend; the four-way one-block helper behind `eijen_hash4_same`, which takes byte-aligned messages only and writes its own `0x01` (`eijen_avx2.c:345,350`); no repair was written or run.

## Evidence

- Baseline: Table 1 (physical p. 5) gives b = 2048 and r = 1728, 1600, 1472, 1216 and 960 bits for Eijen-256, -384, -512, -768 and -1024. A hash candidate has no bandwidth. The specification's timings were not used; the count below is the whole evidence.
- Specification: §2.3 (physical p. 5) pads with M ∥ 1 ∥ 0^k, where k ≥ 0 is the smallest value with |M| + 1 + k ≡ 0 (mod r). The padded message has ⌈(|M| + 1)/r⌉ blocks. Appendix B.1 (physical p. 24) prints the first word of padded "abc" as `0000000001636261`, which is the byte string `61 62 63 01`. The same example prints the last of its 27 rate words as `8000000000000000`, which §2.3 does not describe and the submitted code does not write (it zero-fills the block after the padding byte, `eijen_ref.c:187`); this was not traced. Read as a second padding bit, it would add a block only when |M| ≡ r − 1 (mod r), and a block needed only at particular lengths does not count as impact.
- Count: every rate is a whole number of bytes (`rate_bytes` 216, 200, 184, 152 and 120 at `eijen_internal.h:10-14`; 8 × 216 = 1728, and so on). A partial final byte holds at most 7 message bits, so the padding bit fits in that byte in either order, and the padded message has ⌈(|M| + 1)/8⌉ bytes before the zero fill, as it has now.
- Skipped work: the submitted code absorbs ⌊⌊|M|/8⌋/(r/8)⌋ + 1 = ⌊|M|/r⌋ + 1 blocks (`eijen_ref.c:144-161,175-188`). This equals ⌈(|M| + 1)/r⌉, so the code skips no specified block, and the repair adds none.
- Source: the reference file is identical in the five instance directories, and so is the optimized file (checked with `md5sum`).
