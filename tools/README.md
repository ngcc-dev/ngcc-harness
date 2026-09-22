# Reproducers

Runnable witnesses for report findings that admit a low-cost experiment. Each
one drives a candidate's own reference implementation through the uniform ABI
described in `api/README.md`. No submission file is modified and no shipped
binary is executed; the libraries are compiled from the candidates' own sources
by the per-candidate Makefiles. Information-theoretic parameter ceilings use
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
make -C tools                  # build the reproducer
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

Facto-DSA's separate `sign-10/reproduce_forgery.py` demonstrates `sign-10-2`:
the public key exposes a universal signing trapdoor. This complete confirmed
forgery is independent of the submitted-size recovery claim in `sign-10-1`.

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
documented in the corresponding `report.md` and `pseudocode.md` instead.
