# HAWK-1 — Critical: public-key-only signing-key recovery through reduced-dimension SVP

Severity: Critical
Status: Confirmed
Layer: Design
Affected: Pinned HAWK v1.1; HAWK-256 attacked practically; HAWK-512 and HAWK-1024 security margins invalidated by the structural result
Discovery: Hard
Exploitation: Published public-key-only end-to-end equivalent-key recovery for HAWK-256; local reproduction covers algebra and cost-model controls
Remediation: Architectural
Credit: Zygimantas Straznickas and Stephen A. Weis, with Claude Mythos Preview assistance; independent local verification by Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Candidate disposition: Withdrawn from the NIST competition on 2026-07-29
Date: 2026-10-18

The Anthropic attack compromises HAWK's proposed security margins and recovers
a working equivalent signing key from public information. The HAWK team
confirmed the improvement and withdrew the candidate on **29 July 2026**;
NIST confirmed the withdrawal that day. The current [NIST On-Ramp Round 3
page](https://csrc.nist.gov/Projects/pqc-dig-sig/round-3-additional-signatures)
links that announcement and marks HAWK withdrawn. The announcement,
confirmation and dates are in the [primary pqc-forum
thread](https://groups.google.com/a/list.nist.gov/g/pqc-forum/c/2r2u6SbHun4).
This rating describes the candidate's cryptanalytic failure; it does not mean
that all its parameter sizes have a cheap practical attack.

## Affected version and attacker access

The evaluated package is NIST's **Round 2 HAWK v1.1, 5 February 2025**,
because the On-Ramp Round 3 page supplies no HAWK package. See the pinned
[provenance](https://github.com/ngcc-dev/ngcc-harness/blob/main/nist/HAWK/provenance.json), [specification](https://csrc.nist.gov/csrc/media/Projects/pqc-dig-sig/documents/round-2/spec-files/hawk-spec-round2-web.pdf), and
[external parameter inventory](https://github.com/ngcc-dev/ngcc-harness/blob/main/nist/HAWK/parameters.csv). The structural result covers
HAWK-256, HAWK-512 and HAWK-1024. HAWK-256 is a challenge set with no NIST
category; HAWK-512 and HAWK-1024 claimed categories 1 and 5 respectively.
This is not an assertion that a separate On-Ramp Round 3 HAWK implementation was
downloaded or tested here.

The attacker needs the public key, not a signing oracle, leaked randomness,
fault access or a side channel. An equivalent signing basis suffices for
fresh-message forgery; recovery of the original compact-key seed is
unnecessary. The [released attack
implementation](https://github.com/anthropics/cryptography-research-demo/blob/fa01c398b42bb7d94eadff75a42dfb45da484457/HAWK/README.md)
outputs a decoded 592-byte HAWK-256 signing key, rather than the original
96-byte seed-based NIST secret-key encoding. It verifies that key using the
alternate signing entry point in the `hawk-sign/dev` reference implementation
at the commit pinned by upstream's `nix/hawk256-ref.nix`. This review has not
established source identity between that development revision and the pinned
NIST v1.1 `Reference_Implementation`.

## Mechanism and concrete impact

From public Gram matrix `Q = B* B`, the attack constructs a rank-`n`
cocycle lattice. The involution `tau(zeta) = -zeta` exposes
`B^-1 tau(B)` as a shortest vector. Recovering an equivalent key takes
polynomially many exact-SVP calls in dimension `n/2 + 1`; the resulting attack
is still exponential. [Straznickas and Weis, Theorem 6.1 and
§6](https://anthropic.com/document/hawk_key_recovery.pdf).

| Set | Previous estimated BKZ block size | Exact-SVP oracle dimension | Previous total AGPS20 gate estimate | New per-call AGPS20 estimate | New total AGPS20 gate bound |
|---|---:|---:|---:|---:|---:|
| HAWK-256 | 211 | 129 | not specified for this challenge set | practical recovery reported | practical recovery reported |
| HAWK-512 | 452 | 257 | 2^150 | 2^86.0 | at most 2^108 |
| HAWK-1024 | 940 | 513 | 2^288 | 2^157.8 | at most 2^182 |

The final column prices the **complete oracle-call budget**; the preceding
column is per SVP call. These are model-based classical gate estimates, not measured
runtimes or NIST category bit counts. The paper additionally gives more
aggressive heuristic GSA-intersect estimates of about `2^80.8` gates at block
size 205 for HAWK-512 and `2^146.5` at block size 432 for HAWK-1024. Its
HAWK-256 experiments recovered working keys in hours on a 96-core server; it
did not attempt HAWK-512 recovery. [Paper, Table 1 and §6.1 text following the
table](https://anthropic.com/document/hawk_key_recovery.pdf).

The [authors' announcement](https://www.anthropic.com/research/discovering-cryptographic-weaknesses)
dates public disclosure to **28 July 2026**, following disclosure to the
HAWK team in June. Its approximately `2^38` HAWK-256 headline and the
AGPS20 totals above use different cost accounting. The result is specific
to HAWK's structure; this report does not transfer it to Falcon, ML-DSA or
other lattice signatures. The team considered straightforward parameter
doubling or higher module rank uncompetitive, rather than publishing a
drop-in repair. [Team response](https://groups.google.com/a/list.nist.gov/g/pqc-forum/c/2r2u6SbHun4).

## Local reproduction and limits

```sh
python3 security/reproduce_hawk_1.py
python3 security/harvest.py nist-round3 hawk-pqc-forum hawk-key-recovery-paper hawk-demo-archive
python3 security/reproduce_hawk_1.py --require-cache
```

The cheap control passes locally. It checks cocycle identities using a known
triangular basis in two toy rings, computes oracle dimensions 129/257/513,
recomputes the published gate-bound arithmetic, and verifies available
harvested evidence against its recorded hashes. The toy basis is supplied
to the control: **this is not unknown-key recovery or a proof of the full
reduction**. Cached-source checks are visibly SKIP on a fresh clone until
harvested; `--require-cache` makes missing artifacts return exit 77.

The full external attack is **not reproduced locally**. SageMath and Nix are
absent in this environment. The pinned upstream revision is
`fa01c398b42bb7d94eadff75a42dfb45da484457`; its source archive SHA-256 is
`3f318c7124e1fe93e6450e77d3702c3c4555b3cd24470dbc93006ed6b7364fed`.
The harvested paper SHA-256 is
`056faead316ea9a8eb01000cb9c548e79e21ad09a0684c3d52bbc1093183ac89`.

An optional external recovery requires the pinned upstream dependencies
and reviewed setup, including Sage/fpylll and the sieve's AVX512-VNNI CPU
support. Once those dependencies are available, its documented recovery
entry point is:

```sh
# In the reviewed, pinned upstream HAWK directory, with its dependencies configured:
sage -python -m hawk_recover /path/to/owned-hawk256-public-key.hex --out recovered_sk.bin
```

Retain final reference-signature verification; do not use `--skip-verify`
as evidence of successful recovery. The output's decoded-key format cannot
be passed directly to the ordinary NIST seed-key API wrapper. Neither
the upstream setup script nor the attack pipeline has been executed here.
Passing ordinary KATs is compatible with this finding:
functional agreement does not restore the lost security margin.

Retrieval statuses, times, URLs and artifact hashes are recorded in
[the append-only harvest log](https://github.com/ngcc-dev/ngcc-harness/blob/main/nist/security/harvests.jsonl). No candidate source
is patched by this report. Withdrawal is the published disposition; there is
no reviewed fixed HAWK variant in this pinned study.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py HAWK-1
```

The retained script's algebra and cost-model subchecks can pass, but the dispatcher returns SKIP (77) because it does not execute the claimed end-to-end SVP key-recovery pipeline.
