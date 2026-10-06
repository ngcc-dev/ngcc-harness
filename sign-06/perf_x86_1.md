# sign-06 COMPASS-SIG — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-06` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561076912574464.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: COMPASS-SIG
- Implementation versions measured: reference
- Parameter sets: `COMPASS-SIG-128`, `COMPASS-SIG-256`, `COMPASS-SIG-384`, `COMPASS-SIG-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-06/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `COMPASS-SIG-128` | guide | PASS |
| `COMPASS-SIG-256` | guide | PASS |
| `COMPASS-SIG-384` | guide | PASS |
| `COMPASS-SIG-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `COMPASS-SIG-128` | keygen | 1.41 M | 681 µs | 1.47e+03 | 681 µs | 7015 (5 × 1403) |
| `COMPASS-SIG-128` | sign | 1.81 M | 876 µs | 1.14e+03 | 875 µs | 5580 (5 × 1116) |
| `COMPASS-SIG-128` | verify | 1.37 M | 663 µs | 1.51e+03 | 663 µs | 7435 (5 × 1487) |
| `COMPASS-SIG-256` | keygen | 5.22 M | 2.53 ms | 395 | 2.53 ms | 1940 (5 × 388) |
| `COMPASS-SIG-256` | sign | 6.81 M | 3.3 ms | 303 | 3.29 ms | 1510 (5 × 302) |
| `COMPASS-SIG-256` | verify | 5.10 M | 2.47 ms | 404 | 2.47 ms | 2020 (5 × 404) |
| `COMPASS-SIG-384` | keygen | 3.87 M | 1.87 ms | 535 | 1.87 ms | 2585 (5 × 517) |
| `COMPASS-SIG-384` | sign | 5.58 M | 2.7 ms | 371 | 2.7 ms | 1800 (5 × 360) |
| `COMPASS-SIG-384` | verify | 3.51 M | 1.7 ms | 589 | 1.7 ms | 2930 (5 × 586) |
| `COMPASS-SIG-512` | keygen | 5.97 M | 2.89 ms | 346 | 2.89 ms | 1670 (5 × 334) |
| `COMPASS-SIG-512` | sign | 8.24 M | 3.98 ms | 251 | 3.98 ms | 1255 (5 × 251) |
| `COMPASS-SIG-512` | verify | 5.54 M | 2.68 ms | 373 | 2.68 ms | 1860 (5 × 372) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `COMPASS-SIG-128` | keygen | 35945 | 1704 KiB | 37500 KiB |
| `COMPASS-SIG-128` | sign | 35945 | 1756 KiB | 34132 KiB |
| `COMPASS-SIG-128` | verify | 35945 | 1840 KiB | 38644 KiB |
| `COMPASS-SIG-256` | keygen | 36097 | 1720 KiB | 41440 KiB |
| `COMPASS-SIG-256` | sign | 36097 | 1896 KiB | 36820 KiB |
| `COMPASS-SIG-256` | verify | 36097 | 2104 KiB | 42576 KiB |
| `COMPASS-SIG-384` | keygen | 37929 | 1736 KiB | 35068 KiB |
| `COMPASS-SIG-384` | sign | 37929 | 1908 KiB | 29744 KiB |
| `COMPASS-SIG-384` | verify | 37929 | 2096 KiB | 38084 KiB |
| `COMPASS-SIG-512` | keygen | 37353 | 1732 KiB | 36684 KiB |
| `COMPASS-SIG-512` | sign | 37353 | 2012 KiB | 32400 KiB |
| `COMPASS-SIG-512` | verify | 37353 | 2284 KiB | 39600 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `COMPASS-SIG-128` | 1664 | 960 | 2080 |
| `COMPASS-SIG-256` | 3616 | 2144 | 4080 |
| `COMPASS-SIG-384` | 5792 | 3136 | 7344 |
| `COMPASS-SIG-512` | 7648 | 4608 | 9024 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — `shake*` names are shims over pseudoXOF

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `COMPASS-SIG-128` | keygen | 93% | 0.3% | drng 1, pseudoXOF 24 |
| `COMPASS-SIG-128` | sign | 88% | 0.0% | pseudoXOF 20 |
| `COMPASS-SIG-128` | verify | 93% | 0.0% | pseudoXOF 19 |
| `COMPASS-SIG-256` | keygen | 95% | 0.1% | drng 1, pseudoXOF 72 |
| `COMPASS-SIG-256` | sign | 89% | 0.0% | pseudoXOF 69 |
| `COMPASS-SIG-256` | verify | 95% | 0.0% | pseudoXOF 60 |
| `COMPASS-SIG-384` | keygen | 90% | 0.1% | drng 1, pseudoXOF 65 |
| `COMPASS-SIG-384` | sign | 77% | 0.0% | pseudoXOF 134 |
| `COMPASS-SIG-384` | verify | 88% | 0.0% | pseudoXOF 46 |
| `COMPASS-SIG-512` | keygen | 91% | 0.1% | drng 1, pseudoXOF 94 |
| `COMPASS-SIG-512` | sign | 79% | 0.0% | pseudoXOF 225 |
| `COMPASS-SIG-512` | verify | 90% | 0.0% | pseudoXOF 69 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `COMPASS-SIG-128` | KAT log (sha256 `1df83a242cadc1fa…`) | `kat/sign-06/COMPASS-SIG-128.log` |
| `COMPASS-SIG-128` | timing keygen | `records/sign-06/COMPASS-SIG-128__keygen.json` |
| `COMPASS-SIG-128` | timing sign | `records/sign-06/COMPASS-SIG-128__sign.json` |
| `COMPASS-SIG-128` | timing verify | `records/sign-06/COMPASS-SIG-128__verify.json` |
| `COMPASS-SIG-128` | hash profile keygen | `profile/sign-06/COMPASS-SIG-128__keygen.json` |
| `COMPASS-SIG-128` | hash profile sign | `profile/sign-06/COMPASS-SIG-128__sign.json` |
| `COMPASS-SIG-128` | hash profile verify | `profile/sign-06/COMPASS-SIG-128__verify.json` |
| `COMPASS-SIG-256` | KAT log (sha256 `9aaa6083b36535c9…`) | `kat/sign-06/COMPASS-SIG-256.log` |
| `COMPASS-SIG-256` | timing keygen | `records/sign-06/COMPASS-SIG-256__keygen.json` |
| `COMPASS-SIG-256` | timing sign | `records/sign-06/COMPASS-SIG-256__sign.json` |
| `COMPASS-SIG-256` | timing verify | `records/sign-06/COMPASS-SIG-256__verify.json` |
| `COMPASS-SIG-256` | hash profile keygen | `profile/sign-06/COMPASS-SIG-256__keygen.json` |
| `COMPASS-SIG-256` | hash profile sign | `profile/sign-06/COMPASS-SIG-256__sign.json` |
| `COMPASS-SIG-256` | hash profile verify | `profile/sign-06/COMPASS-SIG-256__verify.json` |
| `COMPASS-SIG-384` | KAT log (sha256 `67fe953b09a0e02b…`) | `kat/sign-06/COMPASS-SIG-384.log` |
| `COMPASS-SIG-384` | timing keygen | `records/sign-06/COMPASS-SIG-384__keygen.json` |
| `COMPASS-SIG-384` | timing sign | `records/sign-06/COMPASS-SIG-384__sign.json` |
| `COMPASS-SIG-384` | timing verify | `records/sign-06/COMPASS-SIG-384__verify.json` |
| `COMPASS-SIG-384` | hash profile keygen | `profile/sign-06/COMPASS-SIG-384__keygen.json` |
| `COMPASS-SIG-384` | hash profile sign | `profile/sign-06/COMPASS-SIG-384__sign.json` |
| `COMPASS-SIG-384` | hash profile verify | `profile/sign-06/COMPASS-SIG-384__verify.json` |
| `COMPASS-SIG-512` | KAT log (sha256 `5072e8f1e995163e…`) | `kat/sign-06/COMPASS-SIG-512.log` |
| `COMPASS-SIG-512` | timing keygen | `records/sign-06/COMPASS-SIG-512__keygen.json` |
| `COMPASS-SIG-512` | timing sign | `records/sign-06/COMPASS-SIG-512__sign.json` |
| `COMPASS-SIG-512` | timing verify | `records/sign-06/COMPASS-SIG-512__verify.json` |
| `COMPASS-SIG-512` | hash profile keygen | `profile/sign-06/COMPASS-SIG-512__keygen.json` |
| `COMPASS-SIG-512` | hash profile sign | `profile/sign-06/COMPASS-SIG-512__sign.json` |
| `COMPASS-SIG-512` | hash profile verify | `profile/sign-06/COMPASS-SIG-512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

