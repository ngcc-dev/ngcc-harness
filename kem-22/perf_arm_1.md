# kem-22 Mithril — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-22` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560862650748928.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: Mithril
- Implementation versions measured: reference
- Parameter sets: `Mithril-128`, `Mithril-256`, `Mithril-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-22/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Mithril-128` | guide | PASS |
| `Mithril-256` | guide | PASS |
| `Mithril-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Mithril-128` | keygen | 271.6 k | 101 µs | 9.92e+03 | 101 µs | 29060 (5 × 5812) |
| `Mithril-128` | enc | 294.6 k | 109 µs | 9.15e+03 | 109 µs | 28575 (5 × 5715) |
| `Mithril-128` | dec | 317.1 k | 118 µs | 8.5e+03 | 117 µs | 24810 (5 × 4962) |
| `Mithril-256` | keygen | 647.1 k | 240 µs | 4.16e+03 | 240 µs | 12365 (5 × 2473) |
| `Mithril-256` | enc | 736.9 k | 273 µs | 3.66e+03 | 273 µs | 11550 (5 × 2310) |
| `Mithril-256` | dec | 828.5 k | 307 µs | 3.25e+03 | 307 µs | 10185 (5 × 2037) |
| `Mithril-512` | keygen | 1.95 M | 724 µs | 1.38e+03 | 727 µs | 4305 (5 × 861) |
| `Mithril-512` | enc | 2.28 M | 846 µs | 1.18e+03 | 843 µs | 3725 (5 × 745) |
| `Mithril-512` | dec | 2.66 M | 988 µs | 1.01e+03 | 991 µs | 3245 (5 × 649) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Mithril-128` | keygen | 27924 | 1420 KiB | 1488 KiB |
| `Mithril-128` | enc | 27924 | 1424 KiB | 1488 KiB |
| `Mithril-128` | dec | 27924 | 1428 KiB | 1492 KiB |
| `Mithril-256` | keygen | 28412 | 1424 KiB | 1496 KiB |
| `Mithril-256` | enc | 28412 | 1436 KiB | 1500 KiB |
| `Mithril-256` | dec | 28412 | 1440 KiB | 1504 KiB |
| `Mithril-512` | keygen | 28404 | 1428 KiB | 1512 KiB |
| `Mithril-512` | enc | 28404 | 3464 KiB | 3528 KiB |
| `Mithril-512` | dec | 28404 | 1464 KiB | 1528 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `Mithril-128` | 944 | 1136 | 928 | 16 |
| `Mithril-256` | 1648 | 2000 | 1680 | 32 |
| `Mithril-512` | 3056 | 3728 | 3504 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Mithril-128` | keygen | 48% | 5.7% | drng 3, pseudoXOF 7 |
| `Mithril-128` | enc | 47% | 1.5% | drng 1, pseudoXOF 8 |
| `Mithril-128` | dec | 43% | 0.0% | pseudoXOF 8 |
| `Mithril-256` | keygen | 36% | 2.4% | drng 3, pseudoXOF 11 |
| `Mithril-256` | enc | 32% | 0.6% | drng 1, pseudoXOF 12 |
| `Mithril-256` | dec | 29% | 0.0% | pseudoXOF 12 |
| `Mithril-512` | keygen | 26% | 0.9% | drng 3, pseudoXOF 19 |
| `Mithril-512` | enc | 22% | 0.2% | drng 1, pseudoXOF 20 |
| `Mithril-512` | dec | 20% | 0.0% | pseudoXOF 20 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Mithril-128` | KAT log (sha256 `1f2624beb63c59a5…`) | `kat/kem-22/Mithril-128.log` |
| `Mithril-128` | timing dec | `records/kem-22/Mithril-128__dec.json` |
| `Mithril-128` | timing enc | `records/kem-22/Mithril-128__enc.json` |
| `Mithril-128` | timing keygen | `records/kem-22/Mithril-128__keygen.json` |
| `Mithril-128` | hash profile dec | `profile/kem-22/Mithril-128__dec.json` |
| `Mithril-128` | hash profile enc | `profile/kem-22/Mithril-128__enc.json` |
| `Mithril-128` | hash profile keygen | `profile/kem-22/Mithril-128__keygen.json` |
| `Mithril-256` | KAT log (sha256 `e2262a019cf345bc…`) | `kat/kem-22/Mithril-256.log` |
| `Mithril-256` | timing dec | `records/kem-22/Mithril-256__dec.json` |
| `Mithril-256` | timing enc | `records/kem-22/Mithril-256__enc.json` |
| `Mithril-256` | timing keygen | `records/kem-22/Mithril-256__keygen.json` |
| `Mithril-256` | hash profile dec | `profile/kem-22/Mithril-256__dec.json` |
| `Mithril-256` | hash profile enc | `profile/kem-22/Mithril-256__enc.json` |
| `Mithril-256` | hash profile keygen | `profile/kem-22/Mithril-256__keygen.json` |
| `Mithril-512` | KAT log (sha256 `733eb706f528fc3a…`) | `kat/kem-22/Mithril-512.log` |
| `Mithril-512` | timing dec | `records/kem-22/Mithril-512__dec.json` |
| `Mithril-512` | timing enc | `records/kem-22/Mithril-512__enc.json` |
| `Mithril-512` | timing keygen | `records/kem-22/Mithril-512__keygen.json` |
| `Mithril-512` | hash profile dec | `profile/kem-22/Mithril-512__dec.json` |
| `Mithril-512` | hash profile enc | `profile/kem-22/Mithril-512__enc.json` |
| `Mithril-512` | hash profile keygen | `profile/kem-22/Mithril-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

