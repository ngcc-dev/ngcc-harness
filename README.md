# NGCC Round 1 reproduction harness

Build tooling and runnable reproducers for the findings published at
<https://ngcc.dev>. Where a candidate's **reference implementation** is
included, it is compiled from the submitter's own sources into one shared
library per parameter set, driven through the official ICCS API as the
official `KAT_*.c` generators do. Runtime findings are checked by the
candidate-local reproducers or `tools/ngcc_attack`; other findings use static
checks or documented source review.

The required candidate reference sources and FEILIAN RTL witness sources are
included in this repository. No candidate Makefile, CMake script, other
submitted script or prebuilt binary is run. Each `<id>/Makefile` lists the
sources explicitly and compiles them with fixed flags (see `api/README.md`,
"Rules that every candidate Makefile follows").

This repository is not affiliated with NICCS. `SOURCE_ARCHIVES.md` records the
official submission archives from which the included source files were taken.

## Quick start

```sh
make -C api harness                  # bin/ngcc_kat, the KAT harness
make tools                           # generic runtime witness programs
make -C kem-01 && make -C sign-07    # <id>/lib/lib<instance>.so
make -C kem-01 test                  # reproduce the submitted KATs (kat.sha256)
tools/reproduce.sh                   # run every reproducer and its controls
tools/reproduce.sh kem-01            # or one candidate
tools/reproduce.sh kem-13            # DKEM's two findings, all three parameter sets
make design-audit                    # static specification/parameter findings
make check-vulnerabilities           # validate all stable vulnerability IDs
make check-reference-data            # validate all specs and parameter records
```

`make -j8 all test` builds and KAT-tests every included candidate. Run
`make reproduce` after the libraries have been built. Base requirements are
gcc, GNU make, Python 3, CMake, and `pdftotext`; some candidate Makefiles also
need GMP or OpenSSL development libraries. The complete reproduction suite
additionally needs clang for the kem-06 ASan check, NumPy and SciPy for the
Python witnesses, and SageMath for kex-08-1, sign-15-4, and kem-09-2.
The FEILIAN `hash-10-2` RTL witness needs Verilator and a C++ compiler. The
sign-15-4 optimized build needs an AVX2-capable CPU. The sign-10-2 and
sign-16-2 witnesses fetch separately published, SHA-256-checked artifacts
over the network. The kem-09-2 estimate needs the pinned lattice-estimator
checkout documented in its report. The sign-18-5 witness fetches Pébereau's
pinned attack source unless given an existing local checkout; it loads only
the Origami library built by this harness and needs Python SM3 support.

If the default Python lacks NumPy or Sage, point the witnesses to an
appropriate environment, for example:

```sh
AMOEBA_PYTHON=/path/to/sage/bin/python \
NIIKE_PYTHON=/path/to/sage/bin/python \
NGCC_SAGE_PYTHON=/path/to/sage/bin/python \
  tools/reproduce.sh
```

The QUBE reference tree has a documented mismatch against its submitted
top-level KAT files: `make -C kem-33 test` is expected to report four
`MISMATCH` results and one `NOKAT`. See `kem-33/README.md`; this does not
affect the separate `kem-33-1` witness.

## Layout

```
api/            KAT harness (bin/ngcc_kat), link shim, generic make rules; api/README.md
tools/          ngcc_attack.c reproducer, reproduce.sh runner; tools/README.md
security/       vulnerability inventory, focused validators and crash-safe witnesses
data/           machine-readable parameters, candidate metadata and spec provenance
<id>/           specification and extracted pseudocode/parameters for every candidate;
                constant_time.md source review for every candidate;
                included reference sources and a Makefile where needed by this harness,
                kat.sha256 manifest, and patches/ where shipped source cannot
                compile as-is; kex-02, kex-05, sign-03 and kem-29 also have
                candidate-local reproducer source
downloads.csv   candidate list with archive URLs from niccs.org.cn
download.sh     optional: fetch original archives into orig/<id>/orig.zip
extract.sh      optional: unpack an original archive for provenance checks
SOURCE_ARCHIVES.md  archive sizes and SHA-256 digests for included candidates
```

Candidate ids (`sign-NN`, `kem-NN`, `kex-NN`, `hash-NN`) follow the numbering
of the official Round 1 lists. Every candidate has its submitted specification
and a human-readable extraction of its algorithms and parameter tables. Every
submitted implementation instance is also represented in `data/parameters.csv`.
Only candidates covered by a published report, used as a reproducer control, or
needed by the static audit have reference source files here; a directory without
a Makefile is therefore a reference-data entry rather than a build target.
Some `constant_time.md` source reviews cite files not retained in this compact
harness. To inspect those source paths, use `IDS=<id> ./download.sh` followed
by `./extract.sh <id>`; verify the ZIP against `SOURCE_ARCHIVES.md` first.
Submitted test-vector files are not included: the compact `kat.sha256`
manifests let `make test` compare freshly generated vectors to every required
reference digest without retaining multi-gigabyte text files.

Every published issue is identified in `security/vulnerabilities.csv` by its
stable `xxx-yy-z` ID. The verification field distinguishes runtime witnesses,
static checks, findings covered by both, and review findings for which no cheap
automated witness is claimed. The public report prose is maintained at
<https://ngcc.dev/reports/>; this harness does not carry partial report copies.

`api/drng.c`, `api/auxfunc.c` and the `api/API_PKC`, `api/API_CryptHash`
trees are the official NICCS API package files, unmodified; the harness links
the official DRNG into any instance directory that ships without its own copy.
