# sign-07 CS — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-07` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561077046792192.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: CS
- Implementation versions measured: reference
- Parameter sets: `CS-128`, `CS-256`, `CS-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-07/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `CS-128` | guide | PASS |
| `CS-256` | guide | PASS |
| `CS-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `CS-128` | keygen | 546.7 k | 261 µs | 3.83e+03 | 261 µs | 17560 (5 × 3512) |
| `CS-128` | sign | 4.42 M | 2.11 ms | 474 | 2.11 ms | 3660 (5 × 732) |
| `CS-128` | verify | 593.9 k | 284 µs | 3.52e+03 | 284 µs | 16885 (5 × 3377) |
| `CS-256` | keygen | 896.1 k | 428 µs | 2.34e+03 | 428 µs | 10830 (5 × 2166) |
| `CS-256` | sign | 13.25 M | 6.33 ms | 158 | 6.33 ms | 745 (5 × 149) |
| `CS-256` | verify | 1.13 M | 538 µs | 1.86e+03 | 538 µs | 9090 (5 × 1818) |
| `CS-512` | keygen | 2.88 M | 1.37 ms | 728 | 1.37 ms | 3485 (5 × 697) |
| `CS-512` | sign | 52.45 M | 25.1 ms | 39.9 | 25.1 ms | 125 (5 × 25) |
| `CS-512` | verify | 3.62 M | 1.73 ms | 578 | 1.73 ms | 2850 (5 × 570) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `CS-128` | keygen | 99733 | 1744 KiB | 1896 KiB |
| `CS-128` | sign | 99733 | 1800 KiB | 1892 KiB |
| `CS-128` | verify | 99733 | 1816 KiB | 1900 KiB |
| `CS-256` | keygen | 131581 | 1764 KiB | 1908 KiB |
| `CS-256` | sign | 131581 | 1820 KiB | 2024 KiB |
| `CS-256` | verify | 131581 | 1928 KiB | 2004 KiB |
| `CS-512` | keygen | 261549 | 1784 KiB | 1992 KiB |
| `CS-512` | sign | 261549 | 1892 KiB | 2272 KiB |
| `CS-512` | verify | 261549 | 2152 KiB | 2268 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `CS-128` | 976 | 1888 | 1548 |
| `CS-256` | 1760 | 3968 | 3164 |
| `CS-512` | 4288 | 7808 | 5975 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `CS-128` | keygen | 4.7% | 58% | drng 16, pseudoXOF 2 |
| `CS-128` | sign | 56% | 12% | drng 75.3, pseudoXOF 16.4 |
| `CS-128` | verify | 6.3% | 58% | drng 33, pseudoXOF 3 |
| `CS-256` | keygen | 10% | 60% | drng 16, pseudoXOF 2 |
| `CS-256` | sign | 49% | 8.4% | drng 154, pseudoXOF 17.7 |
| `CS-256` | verify | 11% | 59% | drng 54, pseudoXOF 3 |
| `CS-512` | keygen | 14% | 58% | drng 42.1, pseudoXOF 2 |
| `CS-512` | sign | 62% | 6.3% | drng 430, pseudoXOF 32.7 |
| `CS-512` | verify | 16% | 60% | drng 171, pseudoXOF 3 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `CS-128` | KAT log (sha256 `d54361d3e5dd46ad…`) | `kat/sign-07/CS-128.log` |
| `CS-128` | timing keygen | `records/sign-07/CS-128__keygen.json` |
| `CS-128` | timing sign | `records/sign-07/CS-128__sign.json` |
| `CS-128` | timing verify | `records/sign-07/CS-128__verify.json` |
| `CS-128` | hash profile keygen | `profile/sign-07/CS-128__keygen.json` |
| `CS-128` | hash profile sign | `profile/sign-07/CS-128__sign.json` |
| `CS-128` | hash profile verify | `profile/sign-07/CS-128__verify.json` |
| `CS-256` | KAT log (sha256 `a41ece74a4dc846c…`) | `kat/sign-07/CS-256.log` |
| `CS-256` | timing keygen | `records/sign-07/CS-256__keygen.json` |
| `CS-256` | timing sign | `records/sign-07/CS-256__sign.json` |
| `CS-256` | timing verify | `records/sign-07/CS-256__verify.json` |
| `CS-256` | hash profile keygen | `profile/sign-07/CS-256__keygen.json` |
| `CS-256` | hash profile sign | `profile/sign-07/CS-256__sign.json` |
| `CS-256` | hash profile verify | `profile/sign-07/CS-256__verify.json` |
| `CS-512` | KAT log (sha256 `9d01f80e3bb7c8fd…`) | `kat/sign-07/CS-512.log` |
| `CS-512` | timing keygen | `records/sign-07/CS-512__keygen.json` |
| `CS-512` | timing sign | `records/sign-07/CS-512__sign.json` |
| `CS-512` | timing verify | `records/sign-07/CS-512__verify.json` |
| `CS-512` | hash profile keygen | `profile/sign-07/CS-512__keygen.json` |
| `CS-512` | hash profile sign | `profile/sign-07/CS-512__sign.json` |
| `CS-512` | hash profile verify | `profile/sign-07/CS-512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

