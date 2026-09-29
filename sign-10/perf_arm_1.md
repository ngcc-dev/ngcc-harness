# sign-10 Facto-DSA — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-10` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561077457833984.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: Facto-DSA
- Implementation versions measured: reference
- Parameter sets: `Facto-DSA-128`, `Facto-DSA-256`, `Facto-DSA-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-10/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Facto-DSA-128` | guide | PASS |
| `Facto-DSA-256` | guide | PASS |
| `Facto-DSA-512` | harness-default | NOKAT [1] |

[1] NOKAT: the submission contains no KAT file for Facto-DSA-512, so its output cannot be checked against submitted vectors; the other Facto-DSA instances pass. These instances are timed anyway; their output is not validated.

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Facto-DSA-128` | keygen | 6.14 M | 2.28 ms | 439 | 2.28 ms | 1345 (5 × 269) |
| `Facto-DSA-128` | sign | 1.91 M | 708 µs | 1.41e+03 | 708 µs | 7380 (5 × 1476) |
| `Facto-DSA-128` | verify | 909.9 k | 338 µs | 2.96e+03 | 338 µs | 9320 (5 × 1864) |
| `Facto-DSA-256` | keygen | 115.14 M | 42.7 ms | 23.4 | 42.8 ms | 100 (5 × 20) |
| `Facto-DSA-256` | sign | 10.61 M | 3.93 ms | 254 | 3.93 ms | 655 (5 × 131) |
| `Facto-DSA-256` | verify | 10.20 M | 3.79 ms | 264 | 3.77 ms | 840 (5 × 168) |
| `Facto-DSA-512` | keygen | 2.03 G | 754 ms | 1.33 | 754 ms | 100 (5 × 20) |
| `Facto-DSA-512` | sign | 132.53 M | 49.2 ms | 20.3 | 49.2 ms | 115 (5 × 23) |
| `Facto-DSA-512` | verify | 127.13 M | 47.2 ms | 21.2 | 46.7 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Facto-DSA-128` | keygen | 38436 | 3460 KiB | 3528 KiB |
| `Facto-DSA-128` | sign | 38436 | 3556 KiB | 3624 KiB |
| `Facto-DSA-128` | verify | 38436 | 3520 KiB | 3584 KiB |
| `Facto-DSA-256` | keygen | 39700 | 1888 KiB | 4576 KiB |
| `Facto-DSA-256` | sign | 39700 | 4016 KiB | 4100 KiB |
| `Facto-DSA-256` | verify | 39700 | 2492 KiB | 2556 KiB |
| `Facto-DSA-512` | keygen | 40956 | 1492 KiB | 15512 KiB |
| `Facto-DSA-512` | sign | 40956 | 7848 KiB | 15308 KiB |
| `Facto-DSA-512` | verify | 40956 | 7928 KiB | 13264 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | signature |
|---|---|---|---|
| `Facto-DSA-128` | 40040 | 3094 | 40 |
| `Facto-DSA-256` | 456960 | 11662 | 68 |
| `Facto-DSA-512` | 5674240 | 61922 | 128 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Facto-DSA-128` | keygen | 13% | 2.9% | drng 1, sm3hash 1 |
| `Facto-DSA-128` | sign | 0.6% | 9.2% | drng 1, pseudoXOF 1 |
| `Facto-DSA-128` | verify | 88% | 0.0% | pseudoXOF 1, sm3hash 1 |
| `Facto-DSA-256` | keygen | 7.9% | 0.5% | drng 3, sm3hash 1 |
| `Facto-DSA-256` | sign | 0.1% | 1.8% | drng 1, pseudoXOF 1 |
| `Facto-DSA-256` | verify | 89% | 0.0% | pseudoXOF 1, sm3hash 1 |
| `Facto-DSA-512` | keygen | 5.6% | 0.1% | drng 16, sm3hash 1 |
| `Facto-DSA-512` | sign | 0.0% | 0.1% | drng 1, pseudoXOF 1 |
| `Facto-DSA-512` | verify | 89% | 0.0% | pseudoXOF 1, sm3hash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Facto-DSA-128` | KAT log (sha256 `f375c0bbeb2acc01…`) | `kat/sign-10/Facto-DSA-128.log` |
| `Facto-DSA-128` | timing keygen | `records/sign-10/Facto-DSA-128__keygen.json` |
| `Facto-DSA-128` | timing sign | `records/sign-10/Facto-DSA-128__sign.json` |
| `Facto-DSA-128` | timing verify | `records/sign-10/Facto-DSA-128__verify.json` |
| `Facto-DSA-128` | hash profile keygen | `profile/sign-10/Facto-DSA-128__keygen.json` |
| `Facto-DSA-128` | hash profile sign | `profile/sign-10/Facto-DSA-128__sign.json` |
| `Facto-DSA-128` | hash profile verify | `profile/sign-10/Facto-DSA-128__verify.json` |
| `Facto-DSA-256` | KAT log (sha256 `4268b6c5028e6366…`) | `kat/sign-10/Facto-DSA-256.log` |
| `Facto-DSA-256` | timing keygen | `records/sign-10/Facto-DSA-256__keygen.json` |
| `Facto-DSA-256` | timing sign | `records/sign-10/Facto-DSA-256__sign.json` |
| `Facto-DSA-256` | timing verify | `records/sign-10/Facto-DSA-256__verify.json` |
| `Facto-DSA-256` | hash profile keygen | `profile/sign-10/Facto-DSA-256__keygen.json` |
| `Facto-DSA-256` | hash profile sign | `profile/sign-10/Facto-DSA-256__sign.json` |
| `Facto-DSA-256` | hash profile verify | `profile/sign-10/Facto-DSA-256__verify.json` |
| `Facto-DSA-512` | KAT log (sha256 `65e40dfec44d2542…`) | `kat/sign-10/Facto-DSA-512.log` |
| `Facto-DSA-512` | timing keygen | `records/sign-10/Facto-DSA-512__keygen.json` |
| `Facto-DSA-512` | timing sign | `records/sign-10/Facto-DSA-512__sign.json` |
| `Facto-DSA-512` | timing verify | `records/sign-10/Facto-DSA-512__verify.json` |
| `Facto-DSA-512` | hash profile keygen | `profile/sign-10/Facto-DSA-512__keygen.json` |
| `Facto-DSA-512` | hash profile sign | `profile/sign-10/Facto-DSA-512__sign.json` |
| `Facto-DSA-512` | hash profile verify | `profile/sign-10/Facto-DSA-512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

