# LoomKEX whole-scheme failure search

Tracking ID: `kex-05-1`

## Result

A deterministic honest LoomKEX-256 exchange fails at pass 3. The initiator
cannot decapsulate the responder's honestly generated KEM ciphertext, so the
four-pass AKE produces neither messages 3/4 nor either party's shared secret.
This is a whole-scheme correctness failure, not a direct call to the underlying
KEM and not a malformed-input experiment.

The first witness found in this deterministic search was:

| Field | Value |
|---|---|
| Instance | LoomKEX-256 |
| Global trial index | `136129` |
| 64-byte DRNG seed | `5acbd14167a442dae45ef6569e11f0088275695fe473b0599a1013adc0e1997a4029abc1c26ea58ec49c4d7aaffc33e4454ed8719d4360d392e244bcfd8b42bd` |
| Failed API pass | `kex_generate_pass3_msg_a` |
| Return code | `-3` (rigid KEM decapsulation failure) |
| Message lengths | m1 = 1352, m2 = 1384, m3 = 0, m4 = 0 bytes |
| Shared-secret lengths | initiator = 0, responder = 0 bytes |
| Optimized witness SHA-256 | `b957dac32f2b3dfd0e689c0ea47bf7281cdc1bebbf48303e491be052a9067168` |

The full 46,933-byte witness is
[`loom256_failure_witness.txt`](loom256_failure_witness.txt). It records the
seed, both generated long-term key pairs, both serialized protocol states, all
wire messages that exist at the point of failure, the absent later messages and
secrets, and the precise failing stage/code.

The parallel run used 28 interleaved deterministic workers over a requested
2,000,000-index interval. Worker 21 reached index
`21 + 28 * 4861 = 136129` and reported the failure after 4,862 of its own
trials; the wall-clock search took 63.58 seconds on this host. This single find
is a constructive correctness witness, not a statistically sound measurement
of the failure probability. The specification itself reports DFR `2^-18.9`
for LoomKEX-256 (Table 5).

## Submitted optimized implementation

The original package does include an x86-64 optimized implementation for all
three parameter sets under
`Implementations/Optimized_Implementation`. Its own README specifies AVX2,
BMI2, POPCNT and FMA and says that its KAT path has bit-exact output with the
reference implementation. The search therefore links the submitted optimized
KEX wrapper and its KAT-compatible `kem3_kat` and `sig3_kat` libraries, using
the official ICCS DRNG and symmetric path. The build does not substitute a
locally rewritten KEM or protocol.

On this 28-worker host the optimized LoomKEX-256 driver sustained about 1,960
fresh-key whole-exchange attempts per second before the witness was found.

## Reference build and replay

From the repository root:

```sh
make -C kex-05 replay
kex-05/reproduce_failure 136129 1 1 /tmp/loom-ref.txt 0
```

This uses only the submitted scalar reference implementation and replays the
known failing exchange directly. A full deterministic search can use the same
binary with `security/run_loom_failure_search.py`; it is not necessary for
verifying the retained witness.

The index space is independent of worker count: index `i` always expands by
SplitMix64 to the same 64-byte seed. A worker is invoked as
`START STRIDE TRIALS WITNESS [PROGRESS]`, while the Python launcher assigns
interleaved indices to workers and stops at the first witness.

## Exact replay and optimized cross-check

Expected output includes:

```text
**Error:3 crypto_loom_auth_init [-3]
FOUND instance=LoomKEX-256 index=136129 stage=pass3 code=-3 trials=1
```

The retained witness was originally produced by the submitted optimized tier;
its digest is
`b957dac32f2b3dfd0e689c0ea47bf7281cdc1bebbf48303e491be052a9067168`.
The optimized tier is not included or needed by this reproduction repository.

The package's separately submitted scalar reference tier independently
reproduces the same exchange:

```sh
make -C kex-05 replay
kex-05/reproduce_failure 136129 1 1 /tmp/loom-ref.txt 0
```

Its witness is byte-for-byte identical to the optimized witness after removing
only the `implementation=` metadata line. This confirms the same keys, states,
honest messages and pass-3 failure across the two implementation tiers.

## Why return code `-3` proves the AKE abort

The submitted `kex_generate_pass3_msg_a` unpacks the initiator state after pass
1 and calls `crypto_loom_auth_init` on the honest pass-2 message. That routine
extracts the peer ciphertext and invokes `crypto_kem_dec_rigid`. It maps a
nonzero result to `-3` with the source comment `kem failure`; the wrapper then
wipes/destroys the context and returns without setting the message-3 length.
Consequently the responder can never execute pass 4 and neither party can call
its successful shared-secret path for this exchange.

This validates the specified high DFR as an observable protocol-level abort. It
is a reliability/design defect and possible failure-oracle surface; by itself,
this witness is not a key-recovery or authentication attack.
