# sign-06 COMPASS-SIG — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-06` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561076912574464.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: COMPASS-SIG
- Implementation versions measured: reference
- Parameter sets: `COMPASS-SIG-128`, `COMPASS-SIG-256`, `COMPASS-SIG-384`, `COMPASS-SIG-512`

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
| `COMPASS-SIG-128` | keygen | 1.31 M | 487 µs | 2.05e+03 | 487 µs | 6155 (5 × 1231) |
| `COMPASS-SIG-128` | sign | 1.69 M | 629 µs | 1.59e+03 | 628 µs | 4800 (5 × 960) |
| `COMPASS-SIG-128` | verify | 1.27 M | 472 µs | 2.12e+03 | 472 µs | 6480 (5 × 1296) |
| `COMPASS-SIG-256` | keygen | 4.92 M | 1.83 ms | 548 | 1.81 ms | 1645 (5 × 329) |
| `COMPASS-SIG-256` | sign | 6.33 M | 2.35 ms | 425 | 2.35 ms | 1295 (5 × 259) |
| `COMPASS-SIG-256` | verify | 4.75 M | 1.76 ms | 567 | 1.76 ms | 1740 (5 × 348) |
| `COMPASS-SIG-384` | keygen | 3.56 M | 1.32 ms | 755 | 1.32 ms | 2240 (5 × 448) |
| `COMPASS-SIG-384` | sign | 5.02 M | 1.86 ms | 537 | 1.86 ms | 1590 (5 × 318) |
| `COMPASS-SIG-384` | verify | 3.21 M | 1.19 ms | 838 | 1.19 ms | 2595 (5 × 519) |
| `COMPASS-SIG-512` | keygen | 5.56 M | 2.07 ms | 484 | 2.06 ms | 1470 (5 × 294) |
| `COMPASS-SIG-512` | sign | 7.60 M | 2.82 ms | 354 | 2.85 ms | 1095 (5 × 219) |
| `COMPASS-SIG-512` | verify | 5.15 M | 1.91 ms | 523 | 1.9 ms | 1640 (5 × 328) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `COMPASS-SIG-128` | keygen | 33200 | 1428 KiB | 34692 KiB |
| `COMPASS-SIG-128` | sign | 33200 | 1488 KiB | 31136 KiB |
| `COMPASS-SIG-128` | verify | 33200 | 3592 KiB | 35096 KiB |
| `COMPASS-SIG-256` | keygen | 33552 | 1436 KiB | 37200 KiB |
| `COMPASS-SIG-256` | sign | 33552 | 1628 KiB | 33648 KiB |
| `COMPASS-SIG-256` | verify | 33552 | 1820 KiB | 38456 KiB |
| `COMPASS-SIG-384` | keygen | 39024 | 1448 KiB | 32216 KiB |
| `COMPASS-SIG-384` | sign | 39024 | 1640 KiB | 28044 KiB |
| `COMPASS-SIG-384` | verify | 39024 | 1832 KiB | 35324 KiB |
| `COMPASS-SIG-512` | keygen | 34784 | 1452 KiB | 34280 KiB |
| `COMPASS-SIG-512` | sign | 34784 | 1736 KiB | 29880 KiB |
| `COMPASS-SIG-512` | verify | 34784 | 2016 KiB | 36180 KiB |

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
| `COMPASS-SIG-256` | keygen | 96% | 0.1% | drng 1, pseudoXOF 72 |
| `COMPASS-SIG-256` | sign | 90% | 0.0% | pseudoXOF 69 |
| `COMPASS-SIG-256` | verify | 95% | 0.0% | pseudoXOF 60 |
| `COMPASS-SIG-384` | keygen | 91% | 0.1% | drng 1, pseudoXOF 65 |
| `COMPASS-SIG-384` | sign | 81% | 0.0% | pseudoXOF 134 |
| `COMPASS-SIG-384` | verify | 90% | 0.0% | pseudoXOF 46 |
| `COMPASS-SIG-512` | keygen | 92% | 0.1% | drng 1, pseudoXOF 94 |
| `COMPASS-SIG-512` | sign | 82% | 0.0% | pseudoXOF 225 |
| `COMPASS-SIG-512` | verify | 91% | 0.0% | pseudoXOF 69 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `COMPASS-SIG-128` | KAT log (sha256 `0b9eb2f7d9b6a74a…`) | `kat/sign-06/COMPASS-SIG-128.log` |
| `COMPASS-SIG-128` | timing keygen | `records/sign-06/COMPASS-SIG-128__keygen.json` |
| `COMPASS-SIG-128` | timing sign | `records/sign-06/COMPASS-SIG-128__sign.json` |
| `COMPASS-SIG-128` | timing verify | `records/sign-06/COMPASS-SIG-128__verify.json` |
| `COMPASS-SIG-128` | hash profile keygen | `profile/sign-06/COMPASS-SIG-128__keygen.json` |
| `COMPASS-SIG-128` | hash profile sign | `profile/sign-06/COMPASS-SIG-128__sign.json` |
| `COMPASS-SIG-128` | hash profile verify | `profile/sign-06/COMPASS-SIG-128__verify.json` |
| `COMPASS-SIG-256` | KAT log (sha256 `60303efad5b30986…`) | `kat/sign-06/COMPASS-SIG-256.log` |
| `COMPASS-SIG-256` | timing keygen | `records/sign-06/COMPASS-SIG-256__keygen.json` |
| `COMPASS-SIG-256` | timing sign | `records/sign-06/COMPASS-SIG-256__sign.json` |
| `COMPASS-SIG-256` | timing verify | `records/sign-06/COMPASS-SIG-256__verify.json` |
| `COMPASS-SIG-256` | hash profile keygen | `profile/sign-06/COMPASS-SIG-256__keygen.json` |
| `COMPASS-SIG-256` | hash profile sign | `profile/sign-06/COMPASS-SIG-256__sign.json` |
| `COMPASS-SIG-256` | hash profile verify | `profile/sign-06/COMPASS-SIG-256__verify.json` |
| `COMPASS-SIG-384` | KAT log (sha256 `120820888e4c1e3f…`) | `kat/sign-06/COMPASS-SIG-384.log` |
| `COMPASS-SIG-384` | timing keygen | `records/sign-06/COMPASS-SIG-384__keygen.json` |
| `COMPASS-SIG-384` | timing sign | `records/sign-06/COMPASS-SIG-384__sign.json` |
| `COMPASS-SIG-384` | timing verify | `records/sign-06/COMPASS-SIG-384__verify.json` |
| `COMPASS-SIG-384` | hash profile keygen | `profile/sign-06/COMPASS-SIG-384__keygen.json` |
| `COMPASS-SIG-384` | hash profile sign | `profile/sign-06/COMPASS-SIG-384__sign.json` |
| `COMPASS-SIG-384` | hash profile verify | `profile/sign-06/COMPASS-SIG-384__verify.json` |
| `COMPASS-SIG-512` | KAT log (sha256 `9a1d5333d830ce56…`) | `kat/sign-06/COMPASS-SIG-512.log` |
| `COMPASS-SIG-512` | timing keygen | `records/sign-06/COMPASS-SIG-512__keygen.json` |
| `COMPASS-SIG-512` | timing sign | `records/sign-06/COMPASS-SIG-512__sign.json` |
| `COMPASS-SIG-512` | timing verify | `records/sign-06/COMPASS-SIG-512__verify.json` |
| `COMPASS-SIG-512` | hash profile keygen | `profile/sign-06/COMPASS-SIG-512__keygen.json` |
| `COMPASS-SIG-512` | hash profile sign | `profile/sign-06/COMPASS-SIG-512__sign.json` |
| `COMPASS-SIG-512` | hash profile verify | `profile/sign-06/COMPASS-SIG-512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

