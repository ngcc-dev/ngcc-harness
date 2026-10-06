# Performance evaluation

Independent measurements of the NGCC Round 1 implementations, following the
[NICCS self-assessment announcement](https://www.niccs.org.cn/niccs/Notice/pc/content/content_2066798866453762048.html)
and its locally preserved guides for [x86](../doc/perf-x86.pdf) and
[ARM](../doc/perf-arm.pdf), §§3.3–3.5. These are not submitter
self-assessments and not NICCS results.

Results are published per test system (`performance/systems.csv`):

| system | results | evidence |
|---|---|---|
| `x86_1` — Intel Core i7-12700, one performance core, fixed 2.1 GHz | [summary](summary_x86_1.md) · [method](method_x86_1.md) · `<id>/perf_x86_1.md` per candidate | [`data/x86_1/`](data/x86_1/) |
| `arm_1` — Qualcomm Snapdragon X Elite (Oryon), one core, fixed 2.71 GHz | [summary](summary_arm_1.md) · [method](method_arm_1.md) · `<id>/perf_arm_1.md` per candidate | [`data/arm_1/`](data/arm_1/) |

The [symmetric cryptography survey](symmetric-survey.md) records how each
public-key submission implements its hashing and randomness (from
`symmetric_survey.csv`), with the measured share of the ICCS placeholder
functions. The compact summary also times the ICCS helpers themselves at every
hash message length and gives each hash measurement relative to `pseudoXOF`
with the same message length and output width; `summary_notes.csv` supplies
brief per-instance interpretation notes.
The aggregate public-key tables omit non-ICCS variants whose measured
operations never call a placeholder hash helper; their measurements remain on
the per-candidate pages.

`security_targets.csv` assigns each measured instance to the NGCC
128-, 256- or 512-bit comparison target. A blank marks an intermediate or
otherwise supplementary parameter set. The mapping uses the complete NGCC
classical/quantum target: in particular, 160-bit signature sets intended to
meet the 128/80-bit requirement are in the 128-bit group, while SQI Level-1
sets that do not meet the associated 80-bit quantum target remain supplementary.
Hash functions are assigned by their claimed collision-security target, not by
digest size: for example, an ordinary 512-bit digest belongs to the 256-bit
table, while XOF instances follow the security level stated in their specification.

### Representative-instance policy

The ordered comparisons distinguish a parameter set from an implementation of
that parameter set:

- When a submission designates a recommended or primary parameter set for a
  target, that parameter set represents the candidate. Optional,
  experimental, compact, illustrative and DFR-oriented alternatives remain in
  the candidate's detailed performance report but have a blank comparison
  target; an alternative is not promoted merely because it is faster or
  smaller. If the submission makes no such designation, all parameter sets
  that claim the target remain comparison-eligible.
- Reference, architecture-specific and other optimized implementations of the
  *same* eligible parameter set are implementation alternatives. A comparison
  that includes more than one conforming, KAT-passing implementation uses the
  faster complete measurement; this does not change which parameter set is
  represented. The current cross-system ordered pages use the reference
  campaign because comparable optimized coverage is not yet available on both
  systems; optimized measurements remain visible in the detailed reports.

For NEV and NEV-AKE, Remark 5 and the parameter discussion designate the R
sets as the recommended standardization choices and describe the compressed C
sets as demonstrations of possible efficiency gains. Accordingly NEV-R1/R2/R3
and NEV-AKE-R1/R2/R3 are comparison-eligible; the C, compressed C and
DFR-oriented D alternatives remain measured but supplementary.

The same rule selects MAMBA-Frost's five stated default parameter sets rather
than its compact-ciphertext alternatives. For PolarLAC at the 512-bit target,
§5.5 recommends PolarLAC-512* for unrestricted query counts; the plain
PolarLAC-512 set remains measured as the bounded-query alternative.

## What is measured

Every candidate with a harness Makefile — KEMs, signatures, key exchange and
hash functions, 119 submissions — is built with the guide's reference flags
(falling back per instance to the harness defaults when a submission does not
build or pass its KATs that way) and checked against the submitted KAT vectors.
Instances whose vectors are not reproduced by the submitted code are still
timed, marked ⚠, and explained in `kat_issues.csv`. Each candidate page covers
the guide's report items 1–7: basic information, environment, KAT results,
cycles and time per operation, memory proxies, data sizes (and KEX messages
and passes), and an index of the raw evidence. It also gives the share of each
operation spent in `pseudohash`, `pseudoXOF`, `sm3hash` and the ICCS DRNG.

For key exchange, `kex_bandwidth.csv` calculates bandwidth from specified
protocol-message bytes plus each required public key once. These public keys
are transmitted bytes. KEM public-key size is reported separately:
encapsulation assumes the recipient's key is already available. Certificates,
identities supplied out of band, and transport framing are not counted. The
raw benchmark records are preserved. AFS-KEX specifies fresh composite keys in the first two
passes but pre-distributes them in its submitted API; either arrangement has
the same bandwidth. Its fourth pass emits no message but leaves its
output-length parameter unchanged, creating a phantom raw benchmark message.

KEM public-key/ciphertext and signature public-key/signature sizes come from
`external_sizes.csv`, a separate catalog of the frozen submissions' external
formats, not from the timing records or benchmark allocation sizes. The
[external-size audit](EXTERNAL_SIZE_AUDIT.md) identifies variable-length
signatures and discrepancies between specified sizes and submitted encodings.

Timing uses one performance core with turbo off, the `performance` governor,
SMT off and the hardware cycle counter (`perf_event_paranoid` ≤ 2). Each
operation is calibrated with one call, then measured in five trials, normally
each in a fresh process so memory-layout effects are sampled rather than fixed.
Work is bounded by iteration counts, not wall time: fast operations get at
least 100 timed calls, slow ones fewer but at least one per trial. The DRNG is
seeded with bytes `00..2f`, signatures use a 64-byte message, and hash inputs
are the guide's S1–S8 lengths. Static and peak memory are process-level proxies
(ELF image, VmHWM). The system's `method_<ID>.md` gives the details and
limitations.

The ICCS helpers `sm3hash`, `pseudohash` and `pseudoXOF` (from `api/auxfunc.c`,
unchanged) are built by `iccs/Makefile` as hash instances and timed exactly like
a hash candidate at the S1–S8 lengths (`campaign.py baseline`, candidate id
`iccs`); `pseudoXOF` is built at every digest width a hash candidate has, and
`iccs/selftest.py` checks each against an independent model on OpenSSL's SM3
first. The summary divides each hash candidate's cycles by those of `pseudoXOF`
with the same output width and message length. (A dataset without these
records falls back to the median profiled 32-byte `pseudoXOF` call.) This does
not estimate the cost of replacing every public-key helper call with that
candidate. Several
hash submissions are not yet constant-time (e.g. table-based S-boxes), so their
current timings are not production figures; brief notes flag known cases.

## Checking the published results

```sh
make -C performance check        # = python3 performance/validate.py
```

For every dataset under `data/<system>/` this recomputes each timing record's
statistics from its raw trials and each hash profile's shares from its calls,
checks every KAT log against its digest and result in `build.json`, rejects
local absolute paths, and regenerates all reports of that system with
`report.py --check`, which must find them byte-for-byte identical to the
committed pages.

## Running a campaign

On a Linux host with GCC, GNU make, Python 3, `taskset` (util-linux) and
`readelf` (binutils):

1. Register the machine in `systems.csv` as `<arch>_<n>` (e.g. `arm_1`).
2. Fix the host: one performance core, turbo off, `performance` governor,
   SMT off, `kernel.perf_event_paranoid=2`. `campaign.py check` lists what is
   still wrong; the measuring phases refuse to run on an unfixed host.
3. Run, publish and report:

```sh
python3 performance/campaign.py check --cpu 2
python3 performance/campaign.py all --cpu 2 --run-dir performance/runs/<run>
python3 performance/campaign.py publish --run-dir performance/runs/<run>
python3 performance/report.py performance/data/<ID>
make -C performance check
```

`campaign.py` phases are `build`, `baseline`, `calibrate`, `measure`, `profile`
and `hashcost`, each resumable; `baseline` added to an existing run refuses to
measure unless the host state (CPU, clock limit, governor, turbo, SMT, counter
access) matches that of the run's records; `--defer` / `--deferred-only` move very slow
instances out of the way. Run directories (`performance/runs/`) are Git-ignored;
`publish` copies the evidence that the reports cite — `campaign.json`,
`build.json`, `records/`, `profile/`, `kat/` and `katcheck/` logs — into
`data/<ID>/` with repository-relative paths. Calibration runs, build logs,
hash-cost tables and generated KAT text stay in the run directory. On AArch64
the same scripts select the ARM guide flags, and the hash wrappers use
`cntvct_el0` instead of `rdtsc`.

Several cores can share one run: `performance/campaign_parallel.sh RUN "CPUS" DEFERRED_CPU`
runs one `--shard K/N` per core (instances divided by prior run time, all
operations of an instance on one core) and the deferred instances on a further
core, keeping its own processes off the benchmark cores; `--trial-target` and
`--op-budget` shorten a campaign. Where cpufreq exposes no fixed limit,
`clockprobe` measures the clock and `check` requires it to be steady; on
AArch64 with cpufreq, `check` requires the minimum and maximum frequency to be
pinned to the same value. `publish --defer` leaves the deferred instances'
timings out of a preliminary dataset; `evidence` attaches the library and KAT
log of instances timed without passing their KATs to runs built before
`build` recorded them.

## Files

| file | purpose |
|---|---|
| `campaign.py` | build, KAT gate, calibration, measurement, profiling, publication |
| `ngcc_perf.c` | the timing driver: one API operation per process, fixed iteration counts, KEX steps |
| `campaign_parallel.sh` | one campaign on several cores: a shard per core plus the deferred instances |
| `clockprobe.c` | measured core clock (cycle counter over wall time) for hosts without a fixed cpufreq limit |
| `iccs/` | the ICCS helpers as hash instances, and their self-test (`campaign.py baseline`) |
| `hashprof/` | link-time wrappers and tools for the ICCS hash share ([README](hashprof/README.md)) |
| `report.py` | renders `<id>/perf_<ID>.md`, `summary_<ID>.md`, `method_<ID>.md`, `symmetric-survey.md` |
| `validate.py` | checks the published datasets and reports (above) and the source catalog |
| `systems.csv` | registered test systems |
| `kat_issues.csv` | cause and treatment of every instance whose KATs do not pass |
| `summary_notes.csv` | brief per-instance caveats shown in the summary table |
| `symmetric_survey.csv` | ICCS helper usage per public-key candidate |
| `security_targets.csv` | explicit 128/256/512 NGCC comparison target for every measured instance; blank marks a supplementary parameter set |
| `kex_bandwidth.csv` | per-instance protocol-message bytes and required public keys used to calculate bandwidth |
| `external_sizes.csv` | per-instance KEM/signature external sizes, independently curated from the submissions |
| `EXTERNAL_SIZE_AUDIT.md` | size-accounting rules and frozen specification/encoding disagreements |
| `smoke.py`, `Makefile` | optimized (AVX2) builds of four families and a no-record smoke test |
| `kem-35.mk` | Scloud+ AVX2 and NEON build rules |
| `source_catalog.csv`, `import_sources.py`, `import_dove.py`, `catalog.py` | source provenance (below) |

## Source provenance and builds

All 119 submission packages are represented by their software source/include
files. Local ZIP copies are optional and Git-ignored. The [source catalog](source_catalog.csv)
records per-package counts and archive hashes; path-name counts there are not
claims that a variant builds. `IDS=<id> ./download.sh` can fetch an official ZIP
into ignored `orig/` for re-auditing. `performance/import_sources.py` checks
its SHA-256 against `SOURCE_ARCHIVES.md`, then imports source files or package
license notices without overwriting any differing file. `performance/import_dove.py`
handles DOVE's nested RARs using `bsdtar`. Submitted build scripts and binaries
are not executed. Every candidate's KAT check runs from its committed
`kat.sha256` manifest, so no archive extraction is needed to reproduce the
results.

## Optimized builds

Four families have optimized (AVX2) build rules with the guide's performance
flags, and the campaign times them alongside the reference builds: Mithril
(all three levels), Scloud+ SHAKE-128, ZC-DMC (all six levels) and BiT (all
three levels; its 128/256 assembly needs only linker symbol aliases, all
submitted source files remain unchanged).

```sh
make -C performance build-mithril-avx2      # also: scloud, zcdmc, bit; -ref for reference
make -C performance smoke-x86               # rebuild, KAT-check and smoke-test all four
python3 performance/smoke.py --library sign-02/lib/libBiT-256-avx2.so \
    --kat-log sign-02/results/BiT-256-avx2.log
```

`smoke.py` checks KAT evidence and runs short functional timings without
writing or publishing numbers. Scloud+'s `neon` backend source is also
retained; `make -C kem-35 -f ../performance/kem-35.mk PERF_NEON=1 list` shows
the ARM target.
