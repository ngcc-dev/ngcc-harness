# sign-08 DARTS — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-08` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561077176815616.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: DARTS
- Implementation versions measured: reference
- Parameter sets: `DARTS128`, `DARTS256`, `DARTS512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-08/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `DARTS128` | guide | PASS |
| `DARTS256` | guide | PASS |
| `DARTS512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `DARTS128` | keygen | 953.3 k | 455 µs | 2.2e+03 | 455 µs | 17240 (5 × 3448) |
| `DARTS128` | sign | 45.51 M | 21.7 ms | 46 | 21.7 ms | 230 (5 × 46) |
| `DARTS128` | verify | 353.5 k | 169 µs | 5.92e+03 | 169 µs | 27380 (5 × 5476) |
| `DARTS256` | keygen | 1.24 M | 594 µs | 1.68e+03 | 594 µs | 9060 (5 × 1812) |
| `DARTS256` | sign | 33.85 M | 16.2 ms | 61.8 | 16.2 ms | 310 (5 × 62) |
| `DARTS256` | verify | 875.1 k | 418 µs | 2.39e+03 | 418 µs | 11530 (5 × 2306) |
| `DARTS512` | keygen | 2.08 M | 994 µs | 1.01e+03 | 993 µs | 4700 (5 × 940) |
| `DARTS512` | sign | 10.74 M | 5.15 ms | 194 | 5.15 ms | 975 (5 × 195) |
| `DARTS512` | verify | 1.64 M | 785 µs | 1.27e+03 | 785 µs | 6160 (5 × 1232) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `DARTS128` | keygen | 55669 | 1728 KiB | 1856 KiB |
| `DARTS128` | sign | 55669 | 1740 KiB | 1924 KiB |
| `DARTS128` | verify | 55669 | 1816 KiB | 1920 KiB |
| `DARTS256` | keygen | 58957 | 1696 KiB | 1876 KiB |
| `DARTS256` | sign | 58957 | 1788 KiB | 1948 KiB |
| `DARTS256` | verify | 58957 | 1880 KiB | 1960 KiB |
| `DARTS512` | keygen | 64709 | 1760 KiB | 1928 KiB |
| `DARTS512` | sign | 64709 | 1868 KiB | 2132 KiB |
| `DARTS512` | verify | 64709 | 2052 KiB | 2136 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `DARTS128` | 1120 | 1536 | 1449 |
| `DARTS256` | 2208 | 2880 | 2489 |
| `DARTS512` | 4672 | 6016 | 5851 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — x4 shake names loop over the ICCS XOF

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `DARTS128` | keygen | 46% | 0.5% | drng 1, pseudoXOF 8.61, pseudohash 8.61 |
| `DARTS128` | sign | 88% | 0.0% | pseudoXOF 210, pseudohash 83 |
| `DARTS128` | verify | 64% | 0.0% | pseudoXOF 3, pseudohash 3 |
| `DARTS256` | keygen | 64% | 0.4% | drng 1, pseudoXOF 19.1, pseudohash 12 |
| `DARTS256` | sign | 87% | 0.0% | pseudoXOF 148, pseudohash 55 |
| `DARTS256` | verify | 72% | 0.0% | pseudoXOF 6, pseudohash 6 |
| `DARTS512` | keygen | 62% | 0.3% | drng 1, pseudoXOF 50, pseudohash 10 |
| `DARTS512` | sign | 87% | 0.0% | pseudoXOF 77, pseudohash 13 |
| `DARTS512` | verify | 77% | 0.0% | pseudoXOF 47, pseudohash 6 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `DARTS128` | KAT log (sha256 `4c3aba0cff37de07…`) | `kat/sign-08/DARTS128.log` |
| `DARTS128` | timing keygen | `records/sign-08/DARTS128__keygen.json` |
| `DARTS128` | timing sign | `records/sign-08/DARTS128__sign.json` |
| `DARTS128` | timing verify | `records/sign-08/DARTS128__verify.json` |
| `DARTS128` | hash profile keygen | `profile/sign-08/DARTS128__keygen.json` |
| `DARTS128` | hash profile sign | `profile/sign-08/DARTS128__sign.json` |
| `DARTS128` | hash profile verify | `profile/sign-08/DARTS128__verify.json` |
| `DARTS256` | KAT log (sha256 `1065552e8693eea5…`) | `kat/sign-08/DARTS256.log` |
| `DARTS256` | timing keygen | `records/sign-08/DARTS256__keygen.json` |
| `DARTS256` | timing sign | `records/sign-08/DARTS256__sign.json` |
| `DARTS256` | timing verify | `records/sign-08/DARTS256__verify.json` |
| `DARTS256` | hash profile keygen | `profile/sign-08/DARTS256__keygen.json` |
| `DARTS256` | hash profile sign | `profile/sign-08/DARTS256__sign.json` |
| `DARTS256` | hash profile verify | `profile/sign-08/DARTS256__verify.json` |
| `DARTS512` | KAT log (sha256 `0e8b13a8e44fe5d8…`) | `kat/sign-08/DARTS512.log` |
| `DARTS512` | timing keygen | `records/sign-08/DARTS512__keygen.json` |
| `DARTS512` | timing sign | `records/sign-08/DARTS512__sign.json` |
| `DARTS512` | timing verify | `records/sign-08/DARTS512__verify.json` |
| `DARTS512` | hash profile keygen | `profile/sign-08/DARTS512__keygen.json` |
| `DARTS512` | hash profile sign | `profile/sign-08/DARTS512__sign.json` |
| `DARTS512` | hash profile verify | `profile/sign-08/DARTS512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

