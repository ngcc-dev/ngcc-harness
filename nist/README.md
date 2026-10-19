# NIST additional-signature publication and reproduction package

This directory is the public, reproducible source for the independent
`https://ngcc.dev/nist/` report listing. It is separate from the NGCC Round 1
inventory and classification. The checked-in snapshot contains the nine NIST
additional-signature candidates, the NIST-specific classification policy, the
numbered finding registry, primary-source registry, normalized report prose,
and every publishable finding witness.

`candidates.csv` links each candidate to the official NIST page, specification
PDF, and submission ZIP. HAWK is marked withdrawn and links to its last
NIST-hosted Round 2 package because NIST publishes no On-Ramp Round 3 HAWK package.
Archive and specification hashes pin those official files.

`findings.csv` is the machine-readable report index. `reports/<ID>.md` holds
the corresponding publication text. Links to specifications use official NIST
PDFs. Links to checked-in evidence point into this public harness; references
to submitted code use plain paths that appear after `make fetch`.
`classification-public.md` is the website-facing NIST classification policy;
it contains the substantive rating, evidence, constant-time, entropy, and
malleability rules without maintainer workflow or local tooling instructions.
`classification.md` retains the fuller harness methodology.
`source-manifest.csv` pins each imported input and generated output. The
dispatcher validates the complete generated publication and witness-input tree
before it executes a witness, rejects unlisted Python files, and starts Python
in isolated mode. Standard-library and installed-package paths remain ahead of
the harness root and witness directory. The dispatcher sends the authenticated
entry-script bytes directly to that interpreter, so it never reopens the
script between verification and execution. Package-control
files such as the dispatcher, checker, importer, fetcher, test suite, README,
and top-level Makefile
remain trusted as part of the public harness checkout rather than claiming to
self-authenticate through that generated manifest.
`snapshot.json` records a digest of the complete imported input set without
publishing private repository metadata or revision identifiers.

Run the checked-in validation and a witness directly:

```sh
make -C nist check
python3 nist/reproduce.py --list
python3 nist/reproduce.py UOV-2
```

Pure-model witnesses run immediately. Witnesses that inspect or execute a
submission first need the official packages and, in some cases, compiled
libraries:

```sh
make -C nist fetch
make -C nist/QR-UOV libs
python3 nist/reproduce.py QR-UOV-3
```

`make fetch` downloads the pinned official NIST ZIPs and PDFs, verifies their
published hashes, and extracts the source/build files used by the witnesses.
ZIPs, PDFs, extracted submissions, libraries, binaries, and other build output
are ignored. The fetcher requires the Python `requests` package. Failed hash or
size checks remove the rejected download and its partial file so a later fetch
starts cleanly. Individual witnesses list their additional dependencies in
`reproducers.csv`.
`reproducers.csv` records whether official source is needed, any library build
directory and target, and human-readable prerequisites. The dispatcher reports
an actionable SKIP when fetched source is absent. A requested build that fails
is a reproduction failure and returns a nonzero failure status. For
library-backed witnesses it invokes the recorded Make target, which builds a
fresh shared object from the hash-pinned official package. Shared objects are
environment-dependent build products and are neither checked in nor assigned
a purported portable binary hash; their trust boundary is the pinned source,
build recipe, and local compiler environment.

Maintainers refresh the publication snapshot from the private analysis tree
with an explicit local path. By default, the importer refuses staged,
unstaged, or untracked source changes and checks that the Git state remains
unchanged while inputs are read. It performs no network requests and publishes
neither the local path nor any repository URL or revision:

```sh
make -C nist sync PRIVATE_SOURCE=/path/to/analysis-checkout
make -C nist check-sync PRIVATE_SOURCE=/path/to/analysis-checkout
```

An intentionally reviewed but uncommitted worktree, including reviewed new
files, requires an explicit opt-in. That snapshot pins the complete imported
input tree by content and makes no claim that it matches the source checkout's
`HEAD`:

```sh
make -C nist sync PRIVATE_SOURCE=/path/to/analysis-checkout ALLOW_DIRTY=1
make -C nist check-sync PRIVATE_SOURCE=/path/to/analysis-checkout ALLOW_DIRTY=1
```

Research-source harvesting writes new logs and downloaded objects under the
ignored `.cache/security/` directory. The checked-in
`security/harvests.jsonl` remains an immutable publication snapshot, so a
harvest does not invalidate witness hashes. Offline verification and the
HAWK-1 witness read both logs.

The website generator consumes this directory through this stable contract:

- `candidates.csv`: one row per candidate, in canonical display order;
- `findings.csv`: one row per stable numbered finding, including canonical
  `Credit` attribution matching the corresponding report header;
- `classification-public.md`: website-facing NIST-call-based policy;
- `classification.md`: fuller harness methodology and evidence policy;
- `reproducers.csv`: finding-to-public-script mapping and pinned hashes;
- `reports/<ID>.md`: normalized Markdown report bodies;
- `sources.json`: registered primary sources used by findings;
- `snapshot.json` and `source-manifest.csv`: content-addressed provenance.

Submitted source is always obtained from the official NIST links. The harness
does not check in submission archives, specifications, full KAT files, IP
statements, or build products.

## Rights and licence status

See [`LICENSE.md`](LICENSE.md). Submitted and external material retains its
upstream notices and terms. This package currently grants no additional licence
for the original harness code, generated inventories, or report text.
