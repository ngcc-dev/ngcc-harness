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
