# sign-33 VDOO: Vinegar-Diagonal-Oil-Oil — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-33` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561114325766144.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: VDOO: Vinegar-Diagonal-Oil-Oil
- Implementation versions measured: reference
- Parameter sets: `vdoo_128`, `vdoo_256`, `vdoo_512`

## 2. Assessment environment

| item | value |
|---|---|
| processor | 12th Gen Intel(R) Core(TM) i7-12700 (CPU 2, one core) |
| clock | max 2.10 GHz, governor performance, turbo off, SMT off |
| memory | 31788 MiB |
| OS / kernel | Debian GNU/Linux 13 (trixie) / 6.12.107+deb13-amd64 |
| compiler / build tool | gcc (Debian 14.2.0-19) 14.2.0 / cmake version 3.31.6 |
| campaign start / end (UTC) | 2026-09-25T10:02:21 / 2026-09-28T10:51:21 |

## 3. Functional testing (KAT)

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-33/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `vdoo_128` | guide | PASS |
| `vdoo_256` | guide | PASS |
| `vdoo_512` | harness-default | TIMEOUT [1] |

[1] KAT TIMEOUT only: with a 20 MB public key, ten KAT records exceed the harness's 900 s limit (VDOO-128/256 pass). A separate one-record run with a 4-hour limit reproduced the first official KAT record byte for byte (81,408,662 bytes, compared against the verified archive); correctness is therefore established on record 0 (performance/data/x86_1/katcheck/vdoo_512-record0.log). These instances are timed anyway; their output is not validated.

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `vdoo_128` | keygen | 1.20 G | 574 ms | 1.74 | 572 ms | 100 (5 × 20) |
| `vdoo_128` | sign | 219.78 M | 105 ms | 9.49 | 105 ms | 190 (5 × 38) |
| `vdoo_128` | verify | 2.36 M | 1.13 ms | 886 | 1.13 ms | 4390 (5 × 878) |
| `vdoo_256` | keygen | 39.28 G | 18.9 s | 0.053 | 18.8 s | 45 (5 × 9) |
| `vdoo_256` | sign | 271.44 M | 130 ms | 7.7 | 130 ms | 100 (5 × 20) |
| `vdoo_256` | verify | 7.77 M | 3.74 ms | 267 | 3.74 ms | 1335 (5 × 267) |
| `vdoo_512` | keygen | 308.01 G | 148 s | 0.00677 | 148 s | 5 (5 × 1) |
| `vdoo_512` | sign | 1.69 G | 813 ms | 1.23 | 787 ms | 100 (5 × 20) |
| `vdoo_512` | verify | 28.78 M | 13.8 ms | 72.5 | 13.8 ms | 365 (5 × 73) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `vdoo_128` | keygen | 2024705 | 1736 KiB | 4460 KiB |
| `vdoo_128` | sign | 2024705 | 4352 KiB | 4416 KiB |
| `vdoo_128` | verify | 2024705 | 4368 KiB | 4452 KiB |
| `vdoo_256` | keygen | 25742425 | 1720 KiB | 35756 KiB |
| `vdoo_256` | sign | 25742425 | 35664 KiB | 35728 KiB |
| `vdoo_256` | verify | 25742425 | 35640 KiB | 35752 KiB |
| `vdoo_512` | keygen | – | 1736 KiB | 160780 KiB |
| `vdoo_512` | sign | – | 160700 KiB | 160824 KiB |
| `vdoo_512` | verify | – | 160704 KiB | 160768 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | signature |
|---|---|---|---|
| `vdoo_128` | 330855 | 342790 | 85 |
| `vdoo_256` | 4289250 | 4388307 | 316 |
| `vdoo_512` | 20229300 | 20474382 | 471 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `vdoo_128` | keygen | 0.0% | 10% | drng 2.39e+04 |
| `vdoo_128` | sign | 0.0% | 10% | drng 4.92e+03, pseudoXOF 2.37 |
| `vdoo_128` | verify | 0.3% | 0.0% | pseudoXOF 2 |
| `vdoo_256` | keygen | 0.0% | 1.7% | drng 9.94e+04 |
| `vdoo_256` | sign | 0.0% | 0.5% | drng 247, pseudoXOF 2 |
| `vdoo_256` | verify | 0.1% | 0.0% | pseudoXOF 2 |
| `vdoo_512` | keygen | 0.0% | 0.7% | drng 2.46e+05 |
| `vdoo_512` | sign | 0.0% | 0.2% | drng 434, pseudoXOF 2 |
| `vdoo_512` | verify | 0.1% | 0.0% | pseudoXOF 2 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `vdoo_128` | KAT log (sha256 `444ccd23bec52d2d…`) | `kat/sign-33/vdoo_128.log` |
| `vdoo_128` | timing keygen | `records/sign-33/vdoo_128__keygen.json` |
| `vdoo_128` | timing sign | `records/sign-33/vdoo_128__sign.json` |
| `vdoo_128` | timing verify | `records/sign-33/vdoo_128__verify.json` |
| `vdoo_128` | hash profile keygen | `profile/sign-33/vdoo_128__keygen.json` |
| `vdoo_128` | hash profile sign | `profile/sign-33/vdoo_128__sign.json` |
| `vdoo_128` | hash profile verify | `profile/sign-33/vdoo_128__verify.json` |
| `vdoo_256` | KAT log (sha256 `8d2c14b2ffacd1ec…`) | `kat/sign-33/vdoo_256.log` |
| `vdoo_256` | timing keygen | `records/sign-33/vdoo_256__keygen.json` |
| `vdoo_256` | timing sign | `records/sign-33/vdoo_256__sign.json` |
| `vdoo_256` | timing verify | `records/sign-33/vdoo_256__verify.json` |
| `vdoo_256` | hash profile keygen | `profile/sign-33/vdoo_256__keygen.json` |
| `vdoo_256` | hash profile sign | `profile/sign-33/vdoo_256__sign.json` |
| `vdoo_256` | hash profile verify | `profile/sign-33/vdoo_256__verify.json` |
| `vdoo_512` | KAT log (sha256 `0984f1d1cb098f94…`) | `kat/sign-33/vdoo_512.log` |
| `vdoo_512` | timing keygen | `records/sign-33/vdoo_512__keygen.json` |
| `vdoo_512` | timing sign | `records/sign-33/vdoo_512__sign.json` |
| `vdoo_512` | timing verify | `records/sign-33/vdoo_512__verify.json` |
| `vdoo_512` | hash profile keygen | `profile/sign-33/vdoo_512__keygen.json` |
| `vdoo_512` | hash profile sign | `profile/sign-33/vdoo_512__sign.json` |
| `vdoo_512` | hash profile verify | `profile/sign-33/vdoo_512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

