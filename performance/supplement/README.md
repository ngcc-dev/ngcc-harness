# Supplementary measurements

Two measurements that sit next to the published campaign
(`performance/data/<ID>`) without changing it:

1. **NIST reference points.** ML-KEM-512/768/1024 (FIPS 203) and
   ML-DSA-44/65/87 (FIPS 204), the pq-crystals reference implementations
   (`upstream/`, pinned in [PROVENANCE.md](PROVENANCE.md)), are built behind the
   ICCS API exactly like a candidate's reference build: the same
   `api/link_rules.mk`, the guide's reference flags, the same driver
   (`ngcc_perf`), the seeded ICCS DRNG for all randomness, a 64-byte message and
   the same iteration planning. `hashprof` (with `HASHPROF_SHA3=1`) measures
   the share of each operation spent in SHA-3/SHAKE and in the DRNG, the
   counterpart of the candidates' ICCS placeholder share.
2. **XOF output-length test.** The published hash test varies the message
   length at a fixed output width. Here the message is a short seed, the
   security level in bytes, and the output length runs over the same eight
   values (32 B, 128 B, 512 B, 1 KiB, 4 KiB, 8 KiB, 16 KiB, 64 KiB), requested
   through `CryptHash`'s length argument (`ngcc_perf` operation `xof-SEED`).

## Arm results, 22 November 2026

Run `vivo-supplement-20261122T205546Z` measured 18 public-key operations
and 224 XOF seed/output combinations on CPU 2. All 1,210 retained trials
used hardware cycle counters with the CPU frequency pinned at 2.7072 GHz.
The run used Linux `7.0.0-44-generic`. The original Arm campaign used
`7.0.0-34-generic`. The pre-run NEV encapsulation control averaged 71,373
cycles per call across five trials, compared with 71,503 in the archive.
Each fresh process ran 20,000 encapsulations after three warmups. The trial
means in run order were 71,324.6, 71,343.1, 71,371.1, 71,341.2 and 71,485.9
cycles per call.
[Environment and UTC timestamps](data/arm_1/campaign.json) ·
[Archived NEV control](../data/arm_1/records/kem-25/NEV_512_1409_ICCS__enc.json)

| Output | Contents |
| --- | --- |
| [Arm summary](summary_arm_1.md) | Tables, units, adapted XOFs and test status |
| [ML-KEM and ML-DSA CSV](reference_arm_1.csv) | Operation means, SHA-3 and DRNG shares, external sizes |
| [XOF CSV](xof_arm_1.csv) | Whole-call cycles and cycles per output byte at each seed and output length |
| [Timing records](data/arm_1/records/) and [profiles](data/arm_1/profile/) | Individual trials, measured calls and instrumented public-key operations |
| [Build manifest](data/arm_1/build.json), [test logs](data/arm_1/kat/) and [XOF probes](data/arm_1/xofprobe.json) | Binary hashes, self-tests, accepted lengths and equivalent instances |

The base revision was `a47f07692a8ea6888bce1e3ed1e3ea11c440668b`.
The working copy of `supplement.py` used for this run had SHA-256
`68c4cd8b810ed01274611c7306bd95f6d28c4f554ec2be25ad1c37ffc25eb95c`.
The local working directory `performance/runs/vivo-supplement-20261122T205546Z/`
also contains the calibration results, build logs and chronological campaign
log. To regenerate the summary and CSVs from the recorded dataset, run
`python3 performance/supplement/supplement.py report --system arm_1` from
the harness root.

## What is measured

| rows | source | seed |
|---|---|---|
| Dragon-XOF-256/384/512 (`hash-07`), MEGASCON-384/512-XOF (`hash-18`), MoFang-256/768-XOF (`hash-19`), MOZI-384/512-XOF (`hash-20`), Thunder-XOF-256/384/512 (`hash-33`) | the submitted instances, built and KAT-checked by their harness Makefiles | the level in the name, in bytes; MoFang names the underlying digest length, so 16 and 48 B |
| Laurus-XOF (`hash-14`), litchi_xof (`hash-15`) | the submitted XOF function through `adapter/fixed_xof.c`: the submitted `CryptHash` ignores the length argument and always returns `DIGEST_BIT_LENGTH` bits | 32, 64 B |
| CHash LDXOF-512/1024 (`hash-03`) | `C_HASH_LDXOF()` from the submitted reference code (specification Appendix B), which the ICCS interface does not expose (`adapter/chash_ldxof.c`) | 32, 64 B |
| ICCS `pseudoXOF`, SHAKE128, SHAKE256 | `api/auxfunc.c` unchanged; pq-crystals `fips202.c` | every seed length above |

Instances that differ only in their default output width (MEGASCON and MOZI
`-256/-1024/-2048`) give identical outputs for every requested length and are
timed once. Before timing, `supplement.py` probes each library in a child
process: return code, complete output, nothing written past the requested
length (4 KiB guard), and whether shorter outputs are prefixes of longer ones
(reported, not required: LDXOF binds the output length). Only accepted lengths
are timed. With `--probe-all-hashes` every hash instance is probed and the
fixed-length instances that also honour other output lengths are listed, not
timed.

Self-tests: ML-KEM/ML-DSA reproduce the upstream test-vector digests with the
same flags and pass an API round trip (sizes, implicit rejection, rejection of
a modified signature or message); SHAKE matches `hashlib`, `pseudoXOF` matches
the SM3 model of `performance/iccs/selftest.py`; the two adapted submitted XOFs
equal the submitted (KAT-passing) library at its fixed width. LDXOF has no
reference vectors: it is timed with status `NOKAT` after functional checks.

## Running

On a registered system whose campaign is published (`performance/data/<ID>`),
with the host fixed as for that campaign and the same benchmark CPU:

```sh
python3 performance/supplement/supplement.py check --cpu 2
python3 performance/supplement/supplement.py all   --cpu 2 --run-dir performance/runs/<host>-supplement-<date>
python3 performance/supplement/supplement.py publish --run-dir performance/runs/<host>-supplement-<date>
python3 performance/supplement/supplement.py report
```

`check` compares the host state (clock, governor, boost, SMT, counter access,
hostname, CPU model) with the published records measured on the same CPU; the
measuring phases refuse to run if they differ. The trial target and operation
budget are read from the published `campaign.json`. `all` runs `build`,
`measure` (calibrate and measure, resumable) and `profile`. `publish` copies the
evidence to `performance/supplement/data/<ID>/`; `report` writes
`summary_<ID>.md`, `reference_<ID>.csv` and `xof_<ID>.csv`. Expect about half an
hour of measurement per system.

### x86 supplement on `blk`

Run `blk-supplement-20261123T0830Z` completed on 23 November 2026.
The 242 means cover 1,210 hardware-counter trials on CPU 2. The NEV
encapsulation control averaged 96,753 cycles per call, 1.65% above the
archived mean. Timing and profiling took 12 minutes.
[x86 summary](summary_x86_1.md) · [Reference CSV](reference_x86_1.csv) ·
[XOF CSV](xof_x86_1.csv) · [Campaign manifest](data/x86_1/campaign.json)

To regenerate the tables from the saved records, run
`python3 performance/supplement/supplement.py report --system x86_1`.

Use CPU 2 of the registered `x86_1` host. The archived run used GCC 14.2.0,
the `performance` governor with an 800 MHz minimum and a 2.10 GHz ceiling,
turbo off, SMT off, and `kernel.perf_event_paranoid=2`. The preflight lists
any differences. A kernel update produces a warning. [Archived CPU 2 record](../data/x86_1/records/kem-25/NEV_512_1409_ICCS__enc.json)

The table gives the settings used for this run and the desktop values
verified on `blk` afterward. For another run, record the starting values
and restore them after profiling. With SMT off, CPU 2 had private L1 and L2
caches but shared L3 and memory bandwidth with the other cores. The run
did not measure cache contention.

| Control on `blk` | During timing | Desktop after the run |
| --- | --- | --- |
| CPU 2 governor | `performance` | `powersave` |
| CPU 2 frequency limits | 800–2,100 MHz | 800–4,800 MHz |
| Intel turbo (`no_turbo`) | off (`1`) | on (`0`) |
| SMT | off | on |
| `kernel.perf_event_paranoid` | 2 | 3 |

From the harness root, the build and functional tests can run before the
host is configured for timing. Choose the run directory once and reuse it
for all phases. The default build covers the submitted XOFs, adapters and
reference functions used in the Arm timing tables.

```sh
run_dir="performance/runs/blk-supplement-$(date -u +%Y%m%dT%H%M%SZ)"
python3 performance/supplement/supplement.py build --system x86_1 --cpu 2 --run-dir "$run_dir"
```

The build-phase host problems in the 23 November manifest reflect the
ordinary desktop settings during functional checks. For a new run, resume
from the harness root after restoring the archived settings. If the machine
was rebooted, set `run_dir` to the directory created by the build above.

```sh
python3 performance/supplement/supplement.py check --system x86_1 --cpu 2 &&
python3 performance/supplement/supplement.py measure --system x86_1 --cpu 2 --run-dir "$run_dir" &&
python3 performance/supplement/supplement.py profile --system x86_1 --cpu 2 --run-dir "$run_dir" &&
python3 performance/supplement/supplement.py publish --run-dir "$run_dir" &&
python3 performance/supplement/supplement.py report --system x86_1
```

Each `measure` invocation rechecks the SHA-256 of every library in
`build.json`. Before calibration it verifies the archived NEV library and
repeats its encapsulation control in five fresh processes, with three
warmups and 19,280 calls per trial. The measurement entry in
`campaign.json` records the command, raw trials, mean cycles and percentage
difference from the archived 95,181 cycles per call. Review the difference
when comparing the campaigns. The trials for the supplement then use the
x86 campaign's one-second target.

Each new build, measurement and profiling entry records the host settings,
UTC time, harness revision, Git status, tracked diff digest when present,
and SHA-256 digests of the scripts and drivers. The run directory retains
the tracked patch. The original 23 November build predates this logging.
Publication checks every timing record against the archived host settings
for its CPU and rejects measurement or profiling runs with a failed host
check or the test override.

The paired datasets should cover the same 18 public-key operations and 224
XOF seed/output combinations. The x86 evidence goes in `data/x86_1/` under
this supplement, and the report command writes `reference_x86_1.csv`,
`xof_x86_1.csv` and `summary_x86_1.md` alongside the Arm tables.

## Files

| file | purpose |
|---|---|
| `supplement.py` | build, self-tests, probes, measurement, SHA-3 profile, publication, report |
| `ml-kem/Makefile`, `ml-dsa/Makefile` | ML-KEM / ML-DSA instances and upstream test-vector programs |
| `xof/Makefile` | `pseudoXOF`, SHAKE128/256, LDXOF and the two adapted XOF instances |
| `adapter/` | ICCS API adapters and `randombytes()` over the ICCS DRNG |
| `upstream/` | vendored pq-crystals reference code ([PROVENANCE.md](PROVENANCE.md)) |

Changes outside this directory: `ngcc_perf.c` gained the `xof-SEED`
operation; `hashprof/wrap.c`, `hashprof.c` and `relink.sh` can wrap the
pq-crystals SHA-3 entry points when `HASHPROF_SHA3=1` (never for candidates);
`campaign.py`'s `operations()` honours an instance's own operation list. The
published datasets and pages are unchanged (`make -C performance check`).
