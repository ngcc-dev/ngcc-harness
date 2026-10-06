# sign-16 Octarine — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-16` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561078258946048.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: Octarine
- Implementation versions measured: reference
- Parameter sets: `Octarine-128`, `Octarine-256`, `Octarine-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-16/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Octarine-128` | guide | PASS |
| `Octarine-256` | guide | PASS |
| `Octarine-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Octarine-128` | keygen | 684.0 k | 254 µs | 3.94e+03 | 253 µs | 11265 (5 × 2253) |
| `Octarine-128` | sign | 1.84 M | 684 µs | 1.46e+03 | 680 µs | 4675 (5 × 935) |
| `Octarine-128` | verify | 767.8 k | 285 µs | 3.51e+03 | 285 µs | 10730 (5 × 2146) |
| `Octarine-256` | keygen | 1.32 M | 489 µs | 2.04e+03 | 487 µs | 6180 (5 × 1236) |
| `Octarine-256` | sign | 4.01 M | 1.49 ms | 672 | 1.48 ms | 3630 (5 × 726) |
| `Octarine-256` | verify | 1.50 M | 559 µs | 1.79e+03 | 558 µs | 5620 (5 × 1124) |
| `Octarine-512` | keygen | 3.40 M | 1.26 ms | 792 | 1.26 ms | 2340 (5 × 468) |
| `Octarine-512` | sign | 15.20 M | 5.64 ms | 177 | 5.64 ms | 905 (5 × 181) |
| `Octarine-512` | verify | 3.85 M | 1.43 ms | 700 | 1.43 ms | 2205 (5 × 441) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Octarine-128` | keygen | 54108 | 1452 KiB | 1560 KiB |
| `Octarine-128` | sign | 54108 | 1496 KiB | 1588 KiB |
| `Octarine-128` | verify | 54108 | 1524 KiB | 1588 KiB |
| `Octarine-256` | keygen | 51660 | 1456 KiB | 1600 KiB |
| `Octarine-256` | sign | 51660 | 1536 KiB | 1648 KiB |
| `Octarine-256` | verify | 51660 | 3608 KiB | 3672 KiB |
| `Octarine-512` | keygen | 52308 | 1476 KiB | 1732 KiB |
| `Octarine-512` | sign | 52308 | 1664 KiB | 1820 KiB |
| `Octarine-512` | verify | 52308 | 1756 KiB | 1828 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `Octarine-128` | 1344 | 2432 | 2564 |
| `Octarine-256` | 2368 | 4608 | 5449 |
| `Octarine-512` | 5184 | 11776 | 14713 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Octarine-128` | keygen | 64% | 0.9% | drng 1, pseudoXOF 7 |
| `Octarine-128` | sign | 56% | 0.3% | drng 1, pseudoXOF 21.3 |
| `Octarine-128` | verify | 55% | 0.0% | pseudoXOF 9 |
| `Octarine-256` | keygen | 61% | 0.4% | drng 1, pseudoXOF 12 |
| `Octarine-256` | sign | 57% | 0.1% | drng 1, pseudoXOF 36.8 |
| `Octarine-256` | verify | 54% | 0.0% | pseudoXOF 13 |
| `Octarine-512` | keygen | 56% | 0.2% | drng 1, pseudoXOF 27 |
| `Octarine-512` | sign | 58% | 0.0% | drng 1, pseudoXOF 114 |
| `Octarine-512` | verify | 50% | 0.0% | pseudoXOF 26 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Octarine-128` | KAT log (sha256 `28ac09e7cbb93f95…`) | `kat/sign-16/Octarine-128.log` |
| `Octarine-128` | timing keygen | `records/sign-16/Octarine-128__keygen.json` |
| `Octarine-128` | timing sign | `records/sign-16/Octarine-128__sign.json` |
| `Octarine-128` | timing verify | `records/sign-16/Octarine-128__verify.json` |
| `Octarine-128` | hash profile keygen | `profile/sign-16/Octarine-128__keygen.json` |
| `Octarine-128` | hash profile sign | `profile/sign-16/Octarine-128__sign.json` |
| `Octarine-128` | hash profile verify | `profile/sign-16/Octarine-128__verify.json` |
| `Octarine-256` | KAT log (sha256 `284c6461568dcbd2…`) | `kat/sign-16/Octarine-256.log` |
| `Octarine-256` | timing keygen | `records/sign-16/Octarine-256__keygen.json` |
| `Octarine-256` | timing sign | `records/sign-16/Octarine-256__sign.json` |
| `Octarine-256` | timing verify | `records/sign-16/Octarine-256__verify.json` |
| `Octarine-256` | hash profile keygen | `profile/sign-16/Octarine-256__keygen.json` |
| `Octarine-256` | hash profile sign | `profile/sign-16/Octarine-256__sign.json` |
| `Octarine-256` | hash profile verify | `profile/sign-16/Octarine-256__verify.json` |
| `Octarine-512` | KAT log (sha256 `e08ca5eefdab1447…`) | `kat/sign-16/Octarine-512.log` |
| `Octarine-512` | timing keygen | `records/sign-16/Octarine-512__keygen.json` |
| `Octarine-512` | timing sign | `records/sign-16/Octarine-512__sign.json` |
| `Octarine-512` | timing verify | `records/sign-16/Octarine-512__verify.json` |
| `Octarine-512` | hash profile keygen | `profile/sign-16/Octarine-512__keygen.json` |
| `Octarine-512` | hash profile sign | `profile/sign-16/Octarine-512__sign.json` |
| `Octarine-512` | hash profile verify | `profile/sign-16/Octarine-512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

