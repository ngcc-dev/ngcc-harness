# Pinned upstream SNOVA LPN artifact

[Author repository](https://github.com/WardBeullens/SNOVA_LPN_Attack/tree/553f28040fd325892f3003afc5833d5e6f0b7b58),
commit `553f28040fd325892f3003afc5833d5e6f0b7b58`. Exact archive and source-file
hashes, pinned signer source, experiment profiles, local outputs and scope are
in [SNOVA-LPN.json](SNOVA-LPN.json).

Run from the repository root:

```sh
python3 security/reproduce_snova_upstream.py verify
python3 security/reproduce_snova_upstream.py sample --samples 64
python3 security/reproduce_snova_upstream.py recipe --set A --workers 4 --recovery-jobs 2 --fft-threads 1
```

Python 3.12+ is required for filtered archive extraction. Missing optional compiler/FFTW dependencies report `SKIP` and exit 77.

`verify` explicitly compiles the pinned author's C++17 solver against
single-precision FFTW and runs its self-test plus
`check_generic_lf22_synthetic.py`. The deterministic regression recovers exact
planted secret rows for q=19,13,11, checks shared grouping, pair accounting and
one-harmonic decoding. Both passed locally with GCC 15.2 and FFTW 3.3.10.
This is a solver regression on synthetic LPN data.

`sample` also explicitly compiles the pinned upstream honest reference signer
and author collector for alternative `(v,o,q,l,r,m1)=(27,4,19,4,6,5)`.
A cached collector copy replaces its 48-byte `/dev/urandom` entropy read with
bytes `00..2f`, preserving its domain mixing, and receives a fixed `time()` shim
(value 1700000000). This makes the small witness deterministic; the pinned
collector and candidate signing/verification sources are unchanged. It passed the NIST
API self-test, generated 64 honest message/signature pairs with every pair
verified, and extracted 64 LPN records while verifying every pair again.
Recorded public-key/raw-pair/LPN SHA-256 hashes are compared on repeat 64-pair
runs. No recovered signing key is claimed for these 64 records.

The pinned upstream archive contains a `nist/generic/snova_ref.c` copy that is
byte-identical to the NIST ZIP. The separate `src/snova_ref.c` used by the
small witness differs in namespacing and other details; its `expand_gf` body
equals the local NIST generic implementation after whitespace removal, recorded
in the JSON. The exact local sampler's bias
and controls are separately reproduced by
[SNOVA/reproduce_snova_1.py](../../SNOVA/reproduce_snova_1.py) and analyzed in
[SNOVA-1.md](../../SNOVA/SNOVA-1.md). The 18 recommended q=16 APIs are outside
the odd-prime attack's demonstrated scope.

`recipe` fetches pinned HTTP archives and prints the reviewed upstream staged
commands. It performs no large collection. The full set-A recipe retains 9M
signatures; sets B/C/D retain 16.5M/9M/180M respectively. These GB-scale
experiments were **not** run locally. Workers, simultaneous recovery rows and
FFTW threads are explicit controls. Successful full recovery must validate the
public equations and a fresh reference-format signature; guessing a private
seed or observing bias alone does not meet that success criterion.

The upstream `prepare_official_source.sh` invokes Git clone/checkout and is
never executed here. All source retrieval uses pinned GitHub HTTP archives.
The printed full setup uses reviewed Makefile rules on disposable cached
copies and its validation-only imported-T12 hook; the honest sampling path
and public verification path have no such hook. Archives, compiled programs,
keys and retained pair/shard binaries remain below ignored `.cache/security/`.
