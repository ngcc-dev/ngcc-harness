# sign-08 DARTS — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-08` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561077176815616.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: DARTS
- Implementation versions measured: reference
- Parameter sets: `DARTS128`, `DARTS256`, `DARTS512`

## 2. Assessment environment

| item | value |
|---|---|
| processor | Qualcomm Oryon (CPU 2, one core) |
| machine | ASUS Vivobook S 15 |
| clock | fixed 2.71 GHz (governor performance, minimum = maximum), boost off, SMT none |
| memory | 30562 MiB |
| OS / kernel | Ubuntu 26.04.1 LTS / 7.0.0-34-generic |
| compiler / build tool | gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0 / cmake version 4.2.3 |
| campaign start / end (UTC) | 2026-09-28T15:05:46 / 2026-09-29T08:43:52 |

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
| `DARTS128` | keygen | 792.1 k | 294 µs | 3.4e+03 | 293 µs | 16755 (5 × 3351) |
| `DARTS128` | sign | 42.56 M | 15.8 ms | 63.3 | 15.7 ms | 205 (5 × 41) |
| `DARTS128` | verify | 316.0 k | 117 µs | 8.53e+03 | 117 µs | 25825 (5 × 5165) |
| `DARTS256` | keygen | 1.07 M | 396 µs | 2.52e+03 | 396 µs | 8395 (5 × 1679) |
| `DARTS256` | sign | 31.33 M | 11.6 ms | 86 | 11.6 ms | 270 (5 × 54) |
| `DARTS256` | verify | 798.8 k | 296 µs | 3.37e+03 | 293 µs | 10270 (5 × 2054) |
| `DARTS512` | keygen | 1.84 M | 683 µs | 1.46e+03 | 678 µs | 4415 (5 × 883) |
| `DARTS512` | sign | 9.97 M | 3.7 ms | 270 | 3.7 ms | 840 (5 × 168) |
| `DARTS512` | verify | 1.54 M | 571 µs | 1.75e+03 | 570 µs | 5430 (5 × 1086) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `DARTS128` | keygen | 50576 | 1444 KiB | 1540 KiB |
| `DARTS128` | sign | 50576 | 1472 KiB | 3624 KiB |
| `DARTS128` | verify | 50576 | 1536 KiB | 1600 KiB |
| `DARTS256` | keygen | 53360 | 3480 KiB | 3592 KiB |
| `DARTS256` | sign | 53360 | 1500 KiB | 1668 KiB |
| `DARTS256` | verify | 53360 | 1592 KiB | 1660 KiB |
| `DARTS512` | keygen | 59728 | 3492 KiB | 3656 KiB |
| `DARTS512` | sign | 59728 | 1568 KiB | 1828 KiB |
| `DARTS512` | verify | 59728 | 1756 KiB | 1820 KiB |

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
| `DARTS128` | keygen | 53% | 0.5% | drng 1, pseudoXOF 8.61, pseudohash 8.61 |
| `DARTS128` | sign | 90% | 0.0% | pseudoXOF 210, pseudohash 83 |
| `DARTS128` | verify | 68% | 0.0% | pseudoXOF 3, pseudohash 3 |
| `DARTS256` | keygen | 70% | 0.4% | drng 1, pseudoXOF 19.1, pseudohash 12 |
| `DARTS256` | sign | 89% | 0.0% | pseudoXOF 148, pseudohash 55 |
| `DARTS256` | verify | 75% | 0.0% | pseudoXOF 6, pseudohash 6 |
| `DARTS512` | keygen | 66% | 0.3% | drng 1, pseudoXOF 50, pseudohash 10 |
| `DARTS512` | sign | 88% | 0.0% | pseudoXOF 77, pseudohash 13 |
| `DARTS512` | verify | 78% | 0.0% | pseudoXOF 47, pseudohash 6 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `DARTS128` | KAT log (sha256 `90fe2a8656951957…`) | `kat/sign-08/DARTS128.log` |
| `DARTS128` | timing keygen | `records/sign-08/DARTS128__keygen.json` |
| `DARTS128` | timing sign | `records/sign-08/DARTS128__sign.json` |
| `DARTS128` | timing verify | `records/sign-08/DARTS128__verify.json` |
| `DARTS128` | hash profile keygen | `profile/sign-08/DARTS128__keygen.json` |
| `DARTS128` | hash profile sign | `profile/sign-08/DARTS128__sign.json` |
| `DARTS128` | hash profile verify | `profile/sign-08/DARTS128__verify.json` |
| `DARTS256` | KAT log (sha256 `303c94164bf10d95…`) | `kat/sign-08/DARTS256.log` |
| `DARTS256` | timing keygen | `records/sign-08/DARTS256__keygen.json` |
| `DARTS256` | timing sign | `records/sign-08/DARTS256__sign.json` |
| `DARTS256` | timing verify | `records/sign-08/DARTS256__verify.json` |
| `DARTS256` | hash profile keygen | `profile/sign-08/DARTS256__keygen.json` |
| `DARTS256` | hash profile sign | `profile/sign-08/DARTS256__sign.json` |
| `DARTS256` | hash profile verify | `profile/sign-08/DARTS256__verify.json` |
| `DARTS512` | KAT log (sha256 `b4b786dd37a6582c…`) | `kat/sign-08/DARTS512.log` |
| `DARTS512` | timing keygen | `records/sign-08/DARTS512__keygen.json` |
| `DARTS512` | timing sign | `records/sign-08/DARTS512__sign.json` |
| `DARTS512` | timing verify | `records/sign-08/DARTS512__verify.json` |
| `DARTS512` | hash profile keygen | `profile/sign-08/DARTS512__keygen.json` |
| `DARTS512` | hash profile sign | `profile/sign-08/DARTS512__sign.json` |
| `DARTS512` | hash profile verify | `profile/sign-08/DARTS512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

