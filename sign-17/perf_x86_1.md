# sign-17 OPS Digital Signature Algorithm — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-17` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561086848880640.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: OPS Digital Signature Algorithm
- Implementation versions measured: reference
- Parameter sets: `OPSsig-128`, `OPSsig-256`, `OPSsig-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-17/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `OPSsig-128` | guide | PASS |
| `OPSsig-256` | guide | PASS |
| `OPSsig-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `OPSsig-128` | keygen | 2.31 M | 1.1 ms | 906 | 1.1 ms | 4460 (5 × 892) |
| `OPSsig-128` | sign | 2.63 M | 1.26 ms | 793 | 1.26 ms | 3165 (5 × 633) |
| `OPSsig-128` | verify | 2.16 M | 1.03 ms | 969 | 1.03 ms | 4800 (5 × 960) |
| `OPSsig-256` | keygen | 3.90 M | 1.86 ms | 537 | 1.86 ms | 2600 (5 × 520) |
| `OPSsig-256` | sign | 5.14 M | 2.46 ms | 406 | 2.46 ms | 2525 (5 × 505) |
| `OPSsig-256` | verify | 3.64 M | 1.74 ms | 575 | 1.74 ms | 2845 (5 × 569) |
| `OPSsig-512` | keygen | 7.56 M | 3.61 ms | 277 | 3.61 ms | 1370 (5 × 274) |
| `OPSsig-512` | sign | 10.68 M | 5.11 ms | 196 | 5.11 ms | 1240 (5 × 248) |
| `OPSsig-512` | verify | 7.14 M | 3.41 ms | 293 | 3.41 ms | 1445 (5 × 289) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `OPSsig-128` | keygen | 38121 | 1720 KiB | 1880 KiB |
| `OPSsig-128` | sign | 38121 | 1756 KiB | 1868 KiB |
| `OPSsig-128` | verify | 38121 | 1816 KiB | 1912 KiB |
| `OPSsig-256` | keygen | 39145 | 1728 KiB | 1888 KiB |
| `OPSsig-256` | sign | 39145 | 1804 KiB | 1964 KiB |
| `OPSsig-256` | verify | 39145 | 1856 KiB | 1956 KiB |
| `OPSsig-512` | keygen | 41641 | 1744 KiB | 1980 KiB |
| `OPSsig-512` | sign | 41641 | 1900 KiB | 2108 KiB |
| `OPSsig-512` | verify | 41641 | 2024 KiB | 2152 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `OPSsig-128` | 2560 | 3840 | 4349 |
| `OPSsig-256` | 3392 | 5056 | 5540 |
| `OPSsig-512` | 6720 | 9920 | 12021 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — optimized AVX2 auxfunc.c adds sm3x4 / pseudoXOF_4x

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `OPSsig-128` | keygen | 91% | 0.7% | drng 2, pseudoXOF 17, pseudohash 1 |
| `OPSsig-128` | sign | 89% | 0.4% | drng 1, pseudoXOF 15.8, pseudohash 1, sm3hash 1.13 |
| `OPSsig-128` | verify | 86% | 0.0% | pseudoXOF 12, pseudohash 2, sm3hash 1 |
| `OPSsig-256` | keygen | 92% | 0.4% | drng 2, pseudoXOF 27.5, pseudohash 1 |
| `OPSsig-256` | sign | 88% | 0.2% | drng 1, pseudoXOF 27.7, pseudohash 2.95 |
| `OPSsig-256` | verify | 89% | 0.0% | pseudoXOF 18, pseudohash 3 |
| `OPSsig-512` | keygen | 92% | 0.2% | drng 2, pseudoXOF 27.1, pseudohash 1 |
| `OPSsig-512` | sign | 87% | 0.1% | drng 1, pseudoXOF 27.9, pseudohash 2.98 |
| `OPSsig-512` | verify | 89% | 0.0% | pseudoXOF 18, pseudohash 3 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `OPSsig-128` | KAT log (sha256 `1f09becd73a347df…`) | `kat/sign-17/OPSsig-128.log` |
| `OPSsig-128` | timing keygen | `records/sign-17/OPSsig-128__keygen.json` |
| `OPSsig-128` | timing sign | `records/sign-17/OPSsig-128__sign.json` |
| `OPSsig-128` | timing verify | `records/sign-17/OPSsig-128__verify.json` |
| `OPSsig-128` | hash profile keygen | `profile/sign-17/OPSsig-128__keygen.json` |
| `OPSsig-128` | hash profile sign | `profile/sign-17/OPSsig-128__sign.json` |
| `OPSsig-128` | hash profile verify | `profile/sign-17/OPSsig-128__verify.json` |
| `OPSsig-256` | KAT log (sha256 `87e11e7f8d4fcff8…`) | `kat/sign-17/OPSsig-256.log` |
| `OPSsig-256` | timing keygen | `records/sign-17/OPSsig-256__keygen.json` |
| `OPSsig-256` | timing sign | `records/sign-17/OPSsig-256__sign.json` |
| `OPSsig-256` | timing verify | `records/sign-17/OPSsig-256__verify.json` |
| `OPSsig-256` | hash profile keygen | `profile/sign-17/OPSsig-256__keygen.json` |
| `OPSsig-256` | hash profile sign | `profile/sign-17/OPSsig-256__sign.json` |
| `OPSsig-256` | hash profile verify | `profile/sign-17/OPSsig-256__verify.json` |
| `OPSsig-512` | KAT log (sha256 `018fcafd6513eeb6…`) | `kat/sign-17/OPSsig-512.log` |
| `OPSsig-512` | timing keygen | `records/sign-17/OPSsig-512__keygen.json` |
| `OPSsig-512` | timing sign | `records/sign-17/OPSsig-512__sign.json` |
| `OPSsig-512` | timing verify | `records/sign-17/OPSsig-512__verify.json` |
| `OPSsig-512` | hash profile keygen | `profile/sign-17/OPSsig-512__keygen.json` |
| `OPSsig-512` | hash profile sign | `profile/sign-17/OPSsig-512__sign.json` |
| `OPSsig-512` | hash profile verify | `profile/sign-17/OPSsig-512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

