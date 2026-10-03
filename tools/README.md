# Reproducers

Runnable witnesses for report findings that admit a low-cost experiment. Most
drive a candidate's reference implementation through the uniform ABI described
in `api/README.md`; some compile a focused source-built witness or check a
specified parameter relation. No submission file is modified and no shipped
binary is executed; candidate code is compiled from the submitted sources.
Information-theoretic parameter ceilings use
`security/design_parameter_audit.py` instead and do not attempt their generic
`2^128` or `2^256` attacks.

Each issue has a stable ID of the form `xxx-yy-z`: `xxx-yy` is the candidate
algorithm ID and `z` is that candidate's sequential report number. The runner
prints the relevant ID beside each runtime witness. The complete list, including
findings without a runnable witness, is `security/vulnerabilities.csv`; run
`make check-vulnerabilities` to validate its IDs, statuses, and checker coverage.
Every issue has its own `Severity`, `Status`, `Layer`, `Affected`, `Discovery`,
`Exploitation`, `Credit`, and `Date` metadata in the corresponding report.

## Running everything

```sh
make -C api harness            # once
make tools                     # build ngcc_attack and the security witnesses
make -C hash-09 && make -C kem-01 && ...   # build the candidates you want
tools/reproduce.sh             # run every reproducer
tools/reproduce.sh hash-09     # or just one candidate
```

`reproduce.sh` exits 0 when every supported runtime witness reproduced and every control
stayed clean. A candidate whose libraries are not built is reported as `SKIP`
and makes the run fail, so an incomplete build cannot look successful.

## Reading the output

```
ATTACK <check> <instance> CONFIRMED|NOT-CONFIRMED <detail>
```

Rows marked `[control]` run the *same* check against a candidate that does not
have the defect, and are expected to print `NOT-CONFIRMED`. They are there so a
reader can see the test distinguishes broken from sound implementations rather
than always firing.

## Checks

| check | defect | candidates |
|---|---|---|
| `hash-collide-zeropad` | `H(M) == H(M‖0000000)` because byte-aligned input follows the specification's LSB-first `0x01` convention while partial-byte API input is handled MSB-first | Eijen (hash-09) |
| `hash-collide-rate` | `pad10*1` puts both padding bits in one position when `\|M\| mod r == r-1` | MasterCube (hash-17) |
| `hash-prefix` | no domain separation, so the short digest is a byte-exact prefix of the long one | Megascon (hash-18), Mozi (hash-20) |
| `kem-ct-flip` | the FO implicit-rejection branch is dead code, so modified ciphertexts still return the original shared secret | Aigis-Enc+ (kem-01) |
| `kem-reject-mask` | the rejection mask is not normalised to all-ones, so the returned value retains the low 7 bits of every byte of the valid secret | CheetahKEM (kem-09), LoongKEM (kem-18) |
| `trike-threshold` | the PDF's maximum threshold rejects every honest ciphertext in the paired whole-KEM test, while the shipped minimum threshold succeeds | TRIKE (kem-36) |
| `kem-failure-oracle` | ciphertext mutations distinguish list-decoder failure from later validation failure by return code and timing | UVW-KEM (kem-38) |
| `kex-pfs-recovery` | recorded ciphertexts plus later compromise of the API long-term keys recover the exact completed-session key | AFS-KEX (kex-02) |
| `honest-failure` | a deterministic honest four-pass exchange aborts when the initiator cannot decapsulate the responder's ciphertext | LoomKEX-256 (kex-05) |
| `state-rollback-key-recovery` | chosen pass-2 queries against a restored pass-1 state recover the complete ephemeral KEM secret and predict the final AKE secret | LoomKEX-256 (kex-05) |
| `sign-fors-forgery` | repeated two-bit FORS addressing permits an adaptive chosen-message signature forgery | CEDRUS+C 160f (sign-03) |
| `sig-malleable` | non-canonical trailing encoding bytes yield a distinct valid signature (SUF-CMA); malformed Aigis hint counts also exercise its verifier stack write | Aigis-Sig+ (sign-01), CS (sign-07) |
| `sig-forge-support-grind` | challenge signs are invisible to verification, reducing a universal forgery to a grind over supports; the identical scaled attack completes at tau=3 | CS (sign-07) |
| `sig-pors-padding` | unused PORS authentication-node padding is unchecked and can be changed without invalidating a signature | FlexTree (sign-11) |
| `sig-transcript-leak` | row/column Gram confusion leaves key-dependent variance and covariance in public signatures | YuanYang.DSA (sign-34) |
| `pk-noncanonical` | radix-q packing accepts distinct public-key byte strings that decode to the same coefficients | YuanYang.DSA (sign-34) |
| `sig-hint-padding` | unused fixed-size hint slots are not checked, so a distinct encoding verifies for the same message (SUF-CMA) | MORNING-ATLAS (sign-15) |
| `sig-accept-all` | the verifier discards its result and accepts anything; the guarded all-zero call also records the UVW-128/-256 crash | UVW (sign-32) |
| `sig-uninit-verdict` | with `NDEBUG`, required verifier checks disappear and an all-zero signature's verdict depends on stale stack contents | SQIsign2D2 Level2-eff uncompressed (sign-25) |
| `keygen-determinism` | key generation ignores the seeded DRNG, so two different seeds give the same key | Galas (sign-12) |
| `keygen-fresh` | the seed is ignored but an internal generator advances within a process, so the defect shows as an identical *first* key in every fresh process | HEP-QC (kem-17), VDOO (sign-33) |
| `kem-enc-fresh` | the first key, ciphertext, and shared secret are identical across fresh processes despite different API seeds | HEP-QC (kem-17) |
| `sig-random-fresh` | different messages in fresh processes receive the same 16-byte signing salt despite different API seeds | VDOO (sign-33) |

VDOO needs `keygen-fresh` rather than `keygen-determinism`: its unseeded
generator carries a counter, so two keys made in one process differ and an
in-process test would wrongly clear it.
`sig-random-fresh` additionally changes the message between processes; the
repeated signature tail is VDOO's encoded salt.

Polar-KEM has its own reproducer, `kem-29/reproduce_public_recovery.py`, because
the break is specific: the submission ships `polarkem_recover_message(pk, ct, mu)`
and `polarkem_derive_valid_secret(mu, ct, ss)`, which together recover the
session key from public data alone. `reproduce.sh` runs it.

The latest candidate-local witnesses are also wired into the runner:

- `kem-33/reproduce_unseeded_optimized.py` certifies QUBE's never-seeded
  optimized PRNG path; its shell companion downloads a commit- and hash-pinned
  artifact and requires full key and sender-session recovery on all four sets.
- `kem-36/reproduce_multitarget.c` links TRIKE's submitted encapsulation and
  decapsulation for a scaled multi-target recovery. The bounds certificate
  checks both implementation families and the published full-size cycle costs.
- `kem-16/reproduce_multi_instance.py` hash-pins the DS-DOOM estimator, while
  its native companion verifies HARE's ciphertext-to-session-key mapping and
  controls on all four shipped sets.
- Qing Luan's `sign-20` scripts provide a scaled concatenated-hash witness, a
  submitted-source sampler trace, a pinned rollback forgery replay, and static
  certificates for the 256-bit random-generation state ceilings.
- `kem-11/reproduce_biased_mlwr_gap.py` checks COMPASS-KEM's missing proof
  bridge. `kem-14/reproduce_dtru_covariance.py` certifies DTRU's ring-paired
  decoder covariance and query-bound proof loss, not the cited paper's
  extreme-tail fit or a full key recovery.
- `sign-12/reproduce_long_message.c` sparsely maps a `2^32 + 38`-byte Galas
  message and shows that the reference verifier hashes only its 38-byte prefix.
- `kem-18/reproduce_loong256.sh` downloads Tianyuan Xie's pinned LoongKEM PoC,
  generates a fresh transcript, and runs the 337-dimensional public recovery;
  set `PYTHON` to an interpreter providing `fpylll`.
- `kem-05/reproduce_multi_instance.py` and
  `kem-32/reproduce_multi_instance.py` hash-pin the May–Sá Diogo estimator and
  check the first below-target same-key session counts plus controls.
- `kem-26/reproduce_isd_estimate.py` pins
  `cryptographic-estimators==2.1.1` and reproduces NSS-HQC's ephemeral-decoding
  work factors and public message-recovery certificate. The broader comparative
  audit used to identify these parameters remains internal.
- Rudraksh2's `kem-34` targets check its message space, Cortex-M4 decoder
  constant, caller-declared lengths, and `-II` NTT moduli. The length witness
  uses AddressSanitizer.
- The `kem-36` certificates cover TRIKE's weak-key test, missing free,
  secret-support addresses, and proof gaps. `kem-38/reproduce_dfr_reaction.py`
  checks UVW's final-failure reaction and retry-matrix leak. The `kem-39`
  scripts compare Weaver's BCH decoders and specified parsers.
- The `kem-37` and `kex-09` scripts trace TriQ's decrypted message into
  rejection-sampler timing. They are source/dataflow witnesses, not completed
  key recoveries.
- OPS-SIG's pinned `sign-17/reproduce_spec_findings.sh` checks its fixed
  challenge support, reduced public-key-only forgeries, and rounding mismatch.
  AFS-KEX's pinned `kex-02/reproduce_protocol_findings.sh` exercises its three
  protocol findings at every level.
- Chinith's `sign-05/reproduce_em_constraints.sh` checks the uBlockith-EM
  constraint alignment and Vistrutith composition. YuanYang.DSA's new `sign-34`
  scripts check the sampler constants and signature-mean effect.

The September 28 additions are also wired into `tools/reproduce.sh`:

- `hash-24-3` checks explicit full-mode QSH free-start collisions.
- `kem-02-6` recovers all 576 Amoeba-576 secret coefficients through the
  pre-FO decoder-failure oracle and checks a fresh honest shared secret.
- `kem-18-2` verifies the LoongKEM ring factorizations and pinned estimator
  inputs; it remains a Lead because full covariance-aware recovery is open.
- `kem-29-3` and `kem-29-4` certify the Polar-KEM radius contradiction and
  one-query alias probabilities directly from the retained specification PDF.
- `sign-22-3` provides both an exact Rhyme-SM3 parity-bias calculation and an
  accepted-signature runtime sample.
- `kem-06-1`, `kem-30-1`, `kex-06-2`, and `sign-25-3` use commit-pinned public
  attack repositories for, respectively, ciphertext-reachable BRA crashes,
  PolarLAC timing classes, the MAMBA raw-reconciliation reaction path, and
  compact SQIsign2D2 message-retargeting forgeries.

The September 27 findings are available through `tools/reproduce.sh` by
candidate ID. `kem-03-3` checks the archived BAG-Loong sampler at all four
levels and recovers the secret PKE matrix from five fresh 128-bit public keys;
`kem-29-2` checks the aligned-basis assignments in the archived Polar-KEM
specification. The Polar-KEM paper reports full-size recovery experiments.
`sign-01-5` checks the submitted Aigis-Sig+-I and -II reference, AVX2,
NEON and AArch64 samplers, including their uneven challenge probabilities.
`sign-14-1` fetches a pinned public Lynxer reproducer, verifies the official
archive hash and runs all six full-size forgery and changed-message checks; it
needs Git, curl, unzip and a C compiler. `sign-15-5` compiles all four submitted
ATLAS decoders with AddressSanitizer. `sign-15-6` is withdrawn because its
undefined rotation comes from the shared official NICCS DRNG file.
`sign-15-7` rebuilds an equivalent signing key from stored recovered values
and verifies a fresh-message forgery. Set `ATLAS_FULL=1` and
`NGCC_SAGE_PYTHON` to a Python with NumPy, SciPy and fpylll to generate three
million signatures and repeat the full ATLAS recovery; allow about two hours.
`sign-27-5` fetches Feussner's hash-pinned public reproducer and transforms one
eligible genuine response into a signature on a fresh message by rescaling its
response integer and auxiliary basis. It runs three fresh-key trials at each of
the four submitted levels and checks both source/target rejection controls.

`kem-14-2` and `kem-14-3` use DTRU-Light to expose the invalid-ciphertext return
code and the rejection KDF's missing public-key binding. The latter gives two
distinct keys the same rejection seed and includes a different-seed control.

Amoeba-576's `kem-02/recover_amoeba576.py` uses NumPy and SciPy to recover all
576 secret coefficients through the submitted decapsulation path, then rebuilds
a key and checks fresh honest shared secrets. Set `AMOEBA_PYTHON` when running
`tools/reproduce.sh kem-02` if those dependencies live in a separate Python or
Sage environment.

Cheetah's `kem-09/reproduce_pk_compression_noise.py` is the static check for
`kem-09-3`. Run it directly with `python3`; it enumerates public-key rounding
errors and checks a nonzero omitted noise term, but does not estimate the full
decryption-failure rate.

DKEM's `kem-13-1` witness changes two `c1` components while keeping `c2` fixed
and checks that both rejected ciphertexts return the same key. Its changed-`c2`
control returns a different key. The `kem-13-2` witness encapsulates 64 times
to a zero-vector public key and finds distinct ciphertexts with one repeated
key; an honestly generated public key yields distinct keys. Both witnesses run
on DKEM-128, DKEM-256, and DKEM-512 with `tools/reproduce.sh kem-13` after
`make -C kem-13`. They demonstrate key-binding and contributiveness failures,
without claiming an IND-CCA break for honestly generated keys.

The September 25 additions include malformed-input and ciphertext-alias
witnesses for Amoeba, BAG-Loong, BAG-Piglet, BRA, BRQC, C-Multi-UR-AG,
OAEP-NTRU, QIMEN-PIKE, UVW-KEM, and NIIKE. Run each through
`tools/reproduce.sh <id>` after building that candidate. The process-crash
witnesses isolate their malformed calls in child processes. `kem-28-1` tests
`+q` ciphertext aliases and non-congruent controls at all three levels.

`sign-23-1` builds a source-linked Shuttle covariance recovery driver and
tests fresh-message forgeries at all three levels; allow several minutes for
its ordinary signature samples. `sign-24-1` recovers Sigurd witnesses and
forges at all three levels with two keys each.

The BiT `sign-02-2` wrapper downloads and hash-checks both the attack package
and official submission archive, then streams 200,000 oracle signatures. It
requires equivalent-key recovery, an accepted
fresh-message forgery and a rejecting changed-message control. Because this
takes about three minutes, the full runner executes it only for
`tools/reproduce.sh sign-02` or with `NGCC_SLOW=1`.

The UVW `sign-32-3` wrapper recomputes pair candidates from 300 public error
vectors, recovers an equivalent decoder and forges against the intended
verifier; it does not use the separate accept-all wrapper bug. The VDOO
`sign-33-6` wrapper replays a public-key-only forgery and reject controls by
default; `VDOO_FULL=1` repeats the complete uncached public-map recovery
through the attack's public-key-only `--pk` path. Its posted key is regenerated
from public API seed `00..2f`; the attack receives serialized public-key bytes
and no secret-key input. Both require Python with NumPy.

`sign-05-1` has one source-linked Chinith forgery witness per parameter set.
`make -C sign-05 reproduce` builds and runs all 14; each creates a victim key,
wipes the secret key, signs with a false zero-key witness, then checks acceptance
and a one-bit message-change rejection. `tools/reproduce.sh sign-05` runs the
same set.

The September 26 import adds `hash-02-3`, `hash-05-3`, `sign-08-2`, and
`sign-27-3`/`sign-27-4` to the runner. AXIS checks that its
submitted beat can be inverted for known message bits; this does not perform
the estimated 2^768 second-preimage attack. uHash checks full-round collisions
at every output size with canonical-input controls. The DARTS witness
fetches Bing Shi's attack at a pinned commit, replays the published
20-million-signature accumulators, and signs a fresh message with the recovered
algebraic key; it requires Git and Python with NumPy. The SQIsignTriangle
witness scans the submitted source for verifier equations and runs a scaled
Python challenge model, not a full-size forgery or a call into the submitted
verifier. The `hash-05-2` and `hash-24-2` second-preimage
findings are design analyses with no feasible full-size witness. Set
`NGCC_SAGE_PYTHON` to a NumPy-enabled Python when running the DARTS witness if
the default `python3` lacks NumPy.

The newer focused witnesses include Amoeba's two-error failure-tail calculation
(`kem-02-3`), Mithril-256's honest shared-secret mismatch (`kem-22-1`),
WeaverKEM-256's omitted BCH correction (`kem-39-2`), and the Aigis-Sig+ key-length
and signer-write AddressSanitizer checks (`sign-01-3`/`sign-01-4`). The Lore
script (`kem-19-1`) verifies only its ring projection and CRT reconstruction;
the published lattice-cost estimate remains an unconfirmed lead. All are
invoked by `tools/reproduce.sh`; the Aigis-Sig+ checks require GCC
AddressSanitizer and `rg`.

FEILIAN's `hash-10-2` witness compiles the submitted 1SC, 2SC, 4SC and 8SC
SystemVerilog cores with Verilator. It confirms the `0x61`/`0x6100` collision
in 1SC/4SC/8SC and checks 2SC as a negative control. Run
`make -C hash-10 rtl-exploit`; `hash-10-4` uses
`make -C hash-10 unused-bits` to compare two encodings of the same one-bit
message through the three reference C libraries. `hash-10-3` and `hash-10-5`
are source and specification reviews. For QIMEN-PIKE,
`make -C kem-31 reproduce-hint` checks all four negative-hint positions at each level against
honest decapsulation controls.

AFS-KEX has candidate-local C128/C256/C512 `reproduce_pfs_break*` drivers. Each
records an honest exchange, erases both live session states, then treats the API
long-term secret keys as compromised. Their first halves contain the composite
KEM secret keys, allowing the driver to decapsulate both recorded ciphertexts
and reproduce the exact old session key. A static-key-only control derives a
different value.

Loom has a scalar-reference `kex-05/reproduce_failure` driver. It replays the
known deterministic index 136129 through the complete four-pass exchange and
requires the normalized full-witness SHA-256 to match. Build it with
`make -C kex-05 replay`; no optimized implementation is required. The retained
witness and search methodology are in `security/LOOM_FAILURE_SEARCH.md`.

The separate `kex-05/reproduce_state_rollback_key_recovery` exploit restores a
saved pass-1 state before each chosen pass-2 query. It recovers all 1,024 secret
coefficients and predicts an honest exchange's final shared secret. This is a
conditional rollback/cloning/concurrent-evaluation attack, not a claim about a
strictly linear deployment that irrevocably consumes state. Build it with
`make -C kex-05 exploit`.

CS has a candidate-local universal-forgery driver. At the submitted parameter
sets it verifies the free-transcript construction and runs bounded negative
controls; the full support grind is intentionally infeasible. The same attack
completes on a scaled CS-128 instance that reduces `tau` from 23 to 3 and makes
the documented companion encoding/bound changes. The scaled verifier accepts
the forged signature. Build it with `make -C sign-07 exploit`.

YuanYang.DSA's public-data witness decodes ordinary signatures and measures the
key-dependent per-slot dispersion left by the faulty covariance calculation. A
synthetic spherical transcript is the negative control. The same executable
also constructs a byte-distinct public-key alias and verifies the same
signature under it. Its 4,000-signature sample is needed for the `sign-34-1`
statistical test, not for the `sign-34-2` encoding witness. It does not claim
complete signing-key recovery. Build it with `make -C sign-34 exploit`.

The FactoDSA code under `sign-10/cryptanalysis/` demonstrates reduced-size
algebraic recovery of the hidden zero subspace and subsequent central-map
structure. Full-size costs remain extrapolated and no submitted-size forgery is
claimed, so `sign-10-1` remains a review-classified Lead.

The newer candidate-local witnesses are invoked by the same runner: CHAMP,
Laurus, MasterCube, MoFang, Neulaser, QSH and CHIME under their `hash-*`
directories; BRA and HEP-QC under `kem-06` and `kem-17`; and CEDRUS-alpha,
Facto-DSA, Origami and Tins under their `sign-*` directories. Static-only
validators for MEGASCON, MOZI, ZC-EDMC, Amoeba, YuanYang.KEM and DOVE are listed
as `static` in `security/vulnerabilities.csv` and checked by
`security/check_vulnerability_ids.py`.

`security/rbg_protocol_dependency.py` is the shared source certificate for the
Low external-RBG dependency findings.  It checks the submitted copies in which
the ICCS DRBG is reset from protocol data and then used as a deterministic
PRG, KDF or XOF for key expansion, encryption/re-encryption, signing or
verification.  Ordinary fresh-randomness and KAT-only seeding sites are outside
that check.

Facto-DSA's separate `sign-10/reproduce_forgery.py` demonstrates `sign-10-2`:
the public key exposes a universal signing trapdoor. This complete confirmed
forgery is independent of the submitted-size recovery claim in `sign-10-1`.

Origami's `sign-18/reproduce_public_forgery.py` checks `sign-18-5` against a
freshly seeded Origami-128 library built from the submitted reference source.
It fetches Pébereau's attack at a pinned commit (or accepts that checkout as
an argument), uses only the public key to forge, and requires the submitted
verifier to reject the same signature on a changed message. The higher sets
have not been runtime-forged in this harness.

CEDRUS+C has a candidate-local `sign-03/reproduce_forgery` driver. It obtains
1,000 signatures on distinct chosen messages, catalogs the disclosed FORS
leaves at the implementation's four reachable bottom addresses, and grinds a
digest for a message never sent to the signing oracle. It assembles the new
FORS signature from disclosures belonging to different oracle signatures,
reuses the fixed address's hypertree suffix, and requires the submitted
verifier to accept the result.

The SQIsign2D2 witness deliberately verifies the identical all-zero signature
twice: once after a genuine verification has primed the verifier's stack frame,
and once after that stack region has been zeroed. The uncompressed Level2-eff
instance accepts only the primed call. Its compressed counterpart is included
as a control and rejects both calls.

## Crashes during a sweep

Several candidate verifiers fault on malformed input, which is a reported
defect in its own right. `ngcc_attack` traps the fault, counts it and carries
on, so one bad input cannot hide the rest of the sweep. Aigis-Sig+ reports both
its malleable bits and the flips that crash verification in the same line.
Jumping out of a fault handler is not strictly portable; it is reliable on
Linux and is confined to this test tool.

## Scope

These demonstrate the reported behaviour against source-built libraries. They
do not attack the candidates' underlying hardness assumptions; the CEDRUS+C
driver instead exploits broken composition to produce a complete chosen-message
forgery. Findings that are real but have no cheap runnable witness (for example
CreTAKE's 64-bit ephemeral secret, which needs about 2^64 offline work) are
documented in the public reports at <https://ngcc.dev/reports/> and, where
available, candidate `pseudocode.md` files instead.
