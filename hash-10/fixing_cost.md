No: the repair of hash-10-2 forms the compression domain from a bit count that includes the current block in the 1SC, 4SC and 8SC RTL wrappers, as the specification and the submitted 2SC wrapper do, and it adds no clock cycle per block.

Impact: none
Scope: Function (small), code
Assessed: 2026-11-23

## Attacks considered

- hash-10-2: Three FEILIAN RTL cores have trivial zero-extension collisions (Implementation, Confirmed)

## Remediation

- **hash-10-2.** The root cause is that the 1SC, 4SC and 8SC wrappers pass `hashed_bits_reg`, the count of completed blocks, to the compression function, and add the current block only when it has finished (`core.sv:135,157` in 1SC and 4SC; `:137,160` in 8SC). §2.3 adds the block length to t before the call. The repair adds the current block's bits before `make_domain`. The submitted 2SC wrapper already does this with one combinational adder (`FEILIAN_2SC/core.sv:120-124,141`), and it is the first part of the team's proposed fix. Scope: Function, code; two declarations, two assignments and one changed argument in each of three `core.sv` files. The specification is sound on this point and stays as it is. Impact: none. The finding exists only in the RTL, so it is costed in clock cycles per block. The new sum is combinational, and no state of `core.sv`, `compression.sv` or `round.sv` changes, so a block takes the same cycles as before. The C implementations are not affected. The team's fix also latches the block, the domain and the chaining state for each transaction. The report records no cycle or area figure for the revised RTL, and the revised RTL was not read. The 8SC compression module already registers all three at start (`FEILIAN_8SC/compression.sv:79-82`). If the latches cost one clock per block in the other two cores, that is 1 clock in about 40 at 4SC, or 2.5%, and less at 1SC (judgment), with up to 1024 + 256 register bits per core for the block and the domain. Not checked: the clock period with the adder ahead of the domain input; no synthesis or simulation was run; the team's erratum was not fetched.

## Evidence

- Baseline: a hash candidate has no bandwidth. Table 5.5 (physical p. 35) gives FPGA results for the compression function FEILIAN-f only. The text above it says "Message buffering, padding logic, bit counters, and output registers are outside the measured block". The defect and the repair are in the bit counter of the wrapper `core.sv`, outside that block.
- Specification: §2.3 (physical p. 14) iterates `t ← t + 1024`, `tw ← (ver, flag, t)`, `h_{i+1} ← FEILIAN-f(h_i, M_i, tw)`, and for the last block `t ← t + ∥M_ℓ∥` before the call. The zero padding of the last block is on physical p. 13.
- Source: `diff FEILIAN_1SC/core.sv FEILIAN_2SC/core.sv` shows two differences. One is the pre-compression sum (`FEILIAN_2SC/core.sv:120-124,141`). The other is the order of the two counter words in `make_domain` (`:93-94`), which belongs to hash-10-3 and is not costed. The 1SC and 4SC `core.sv` files are identical.
- Cycles: in 1SC, `compression.sv:50-61` passes `m` and `d` straight to the round module, and `core.sv:135` drives `d` from combinational logic. The repair changes that logic only.
- Cycles per compression call, from Table 5.5 (Virtex-5 rows), latency × maximum frequency: 1.58 us × 101.0 MHz = 159.6 at 1SC, 0.48 us × 84.3 MHz = 40.5 at 4SC, 0.35 us × 60.0 MHz = 21.0 at 8SC. One added clock in 40.5 is 2.5%.
