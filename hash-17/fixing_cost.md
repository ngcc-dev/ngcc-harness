No: the repair of hash-17-1 emits the second padding block that pad10*1 requires when one bit of the rate is left, and that block occurs only at message lengths of r − 1 modulo r in the three MasterCube instances.

Impact: none
Scope: Function (small), code
Assessed: 2026-11-23

## Attacks considered

- hash-17-1: Trivial collisions at every rate boundary (Implementation, Confirmed)

## Remediation

- **hash-17-1.** The root cause is that the code always pads inside one rate block. `pad10star1` ORs the first padding bit after the message and the last padding bit into the last bit of the block (`mastercube.h:25,28`), and `_sponge_absorb_helper` absorbs exactly one padded block after the full blocks (`MasterCube-512.c:311-317,329-338`). With r − 1 message bits in the last block, both padding bits fall on the same bit. Definition 1 requires both bits, so that length needs one more block. The repair is the team's proposed fix: emit an additional block when only one bit remains. Scope: Function, code; the padding helper and the tail of the absorb routine, repeated in each source tree. Definition 1 of the specification is sound; the team says it also added a sentence on the extra block to its document. Impact: none. The extra Cube-f call is needed only when |M| mod r = r − 1, one residue in r, and never for a byte-aligned message. An extra block that is needed only at particular lengths does not count as impact (`security/FIXING_COST_ANALYSIS.md`, Baseline). At every other length the block count is unchanged. The team states that the fix "can lead to an extra block after padding" and that, with this fix and its fix of the inverse round applied, re-benchmarking shows that "the performance variations remain within the normal experimental deviation range"; this is quoted as reported and was not evaluated. Not checked: the erratum and the revised package attached to the team's post were not read; the six Verilog cores under `Additional_Implementation`, of which the three round-sequential ones pad with the whole bytes `0x80` and `0x01` (`mastercube.v:166,173`); no repair was written or run.

## Evidence

- Baseline: a hash candidate has no bandwidth. Table 1 (physical p. 4) gives r = 960, 704 and 448 bits for MasterCube-512, -768 and -1024. The specification's timings were not used; the block count is the whole evidence.
- Specification: Definition 1 (physical p. 5): pad10*1 "appends a single bit 1 followed by the minimum number of bits 0 followed by a single bit 1 such that the length of the result is a multiple of the block length". The padded message has ⌈(|M| + 2)/r⌉ blocks.
- Count: the submitted code absorbs ⌊|M|/r⌋ + 1 blocks, one Cube-f call each (`MasterCube-512.c:319-338`). Write |M| = qr + s with 0 ≤ s < r. Then ⌈(|M| + 2)/r⌉ is q + 1 for s ≤ r − 2 and q + 2 for s = r − 1. The two counts differ only at s = r − 1, which is 1 of 960, 704 or 448 residues. Each r is a multiple of 8, so r − 1 is not a byte-aligned length.
- Source: the three reference files differ only in the function name, `digest_bits` and `block_bits` (`diff`). The body of `pad10star1` is the same in all 18 copies of `mastercube.h`, apart from line endings in three.
- Proposed fix: [MasterCube team's response](https://list.niccs.org.cn/archives/list/crypthashforum@list.niccs.org.cn/message/IG2TAQNA7YHL4JLOOWBTJGKBVVVESWZK/), read on 2026-11-23.
