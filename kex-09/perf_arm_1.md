# kex-09 TriQ-KEX — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kex-09` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560626335272960.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key exchange
- Algorithm: TriQ-KEX
- Implementation versions measured: reference
- Parameter sets: `TriQ-KEX-128`, `TriQ-KEX-256`, `TriQ-KEX-384`, `TriQ-KEX-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kex-09/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `TriQ-KEX-128` | guide | PASS |
| `TriQ-KEX-256` | guide | PASS |
| `TriQ-KEX-384` | guide | PASS |
| `TriQ-KEX-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `TriQ-KEX-128` | exchange | 37.66 M | 14 ms | 71.5 | 14 ms | 230 (5 × 46) |
| `TriQ-KEX-128` | init_a | 2.23 M | 827 µs | 1.21e+03 | 826 µs | 230 (5 × 46) |
| `TriQ-KEX-128` | init_b | 2.23 M | 826 µs | 1.21e+03 | 826 µs | 230 (5 × 46) |
| `TriQ-KEX-128` | pass1 | 6.60 M | 2.45 ms | 409 | 2.45 ms | 230 (5 × 46) |
| `TriQ-KEX-128` | pass2 | 15.46 M | 5.74 ms | 174 | 5.74 ms | 230 (5 × 46) |
| `TriQ-KEX-128` | derive_a | 10.06 M | 3.73 ms | 268 | 3.73 ms | 230 (5 × 46) |
| `TriQ-KEX-128` | derive_b | 1.11 M | 412 µs | 2.43e+03 | 412 µs | 230 (5 × 46) |
| `TriQ-KEX-256` | exchange | 217.58 M | 80.7 ms | 12.4 | 80.7 ms | 100 (5 × 20) |
| `TriQ-KEX-256` | init_a | 13.26 M | 4.92 ms | 203 | 4.92 ms | 100 (5 × 20) |
| `TriQ-KEX-256` | init_b | 13.25 M | 4.92 ms | 203 | 4.92 ms | 100 (5 × 20) |
| `TriQ-KEX-256` | pass1 | 39.63 M | 14.7 ms | 68 | 14.7 ms | 100 (5 × 20) |
| `TriQ-KEX-256` | pass2 | 92.24 M | 34.2 ms | 29.2 | 34.2 ms | 100 (5 × 20) |
| `TriQ-KEX-256` | derive_a | 55.80 M | 20.7 ms | 48.3 | 20.7 ms | 100 (5 × 20) |
| `TriQ-KEX-256` | derive_b | 3.39 M | 1.26 ms | 795 | 1.26 ms | 100 (5 × 20) |
| `TriQ-KEX-384` | exchange | 602.41 M | 223 ms | 4.47 | 223 ms | 100 (5 × 20) |
| `TriQ-KEX-384` | init_a | 36.09 M | 13.4 ms | 74.7 | 13.4 ms | 100 (5 × 20) |
| `TriQ-KEX-384` | init_b | 36.09 M | 13.4 ms | 74.7 | 13.4 ms | 100 (5 × 20) |
| `TriQ-KEX-384` | pass1 | 108.08 M | 40.1 ms | 24.9 | 40.1 ms | 100 (5 × 20) |
| `TriQ-KEX-384` | pass2 | 252.06 M | 93.5 ms | 10.7 | 93.5 ms | 100 (5 × 20) |
| `TriQ-KEX-384` | derive_a | 156.81 M | 58.2 ms | 17.2 | 58.1 ms | 100 (5 × 20) |
| `TriQ-KEX-384` | derive_b | 13.13 M | 4.89 ms | 205 | 4.87 ms | 100 (5 × 20) |
| `TriQ-KEX-512` | exchange | 1.28 G | 475 ms | 2.11 | 474 ms | 100 (5 × 20) |
| `TriQ-KEX-512` | init_a | 77.98 M | 28.9 ms | 34.6 | 28.9 ms | 100 (5 × 20) |
| `TriQ-KEX-512` | init_b | 78.04 M | 29 ms | 34.5 | 29 ms | 100 (5 × 20) |
| `TriQ-KEX-512` | pass1 | 232.89 M | 86.4 ms | 11.6 | 86.4 ms | 100 (5 × 20) |
| `TriQ-KEX-512` | pass2 | 540.80 M | 201 ms | 4.98 | 201 ms | 100 (5 × 20) |
| `TriQ-KEX-512` | derive_a | 327.84 M | 122 ms | 8.22 | 122 ms | 100 (5 × 20) |
| `TriQ-KEX-512` | derive_b | 21.19 M | 7.86 ms | 127 | 7.86 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `TriQ-KEX-128` | exchange | 44624 | 3836 KiB | 3900 KiB |
| `TriQ-KEX-128` | init_a | 44624 | 1796 KiB | 1876 KiB |
| `TriQ-KEX-128` | init_b | 44624 | 1796 KiB | 1876 KiB |
| `TriQ-KEX-128` | pass1 | 44624 | 3612 KiB | 3676 KiB |
| `TriQ-KEX-128` | pass2 | 44624 | 3728 KiB | 3792 KiB |
| `TriQ-KEX-128` | derive_a | 44624 | 3644 KiB | 3708 KiB |
| `TriQ-KEX-128` | derive_b | 44624 | 1796 KiB | 1876 KiB |
| `TriQ-KEX-256` | exchange | 47836 | 4476 KiB | 4540 KiB |
| `TriQ-KEX-256` | init_a | 47836 | 4380 KiB | 4444 KiB |
| `TriQ-KEX-256` | init_b | 47836 | 4356 KiB | 4420 KiB |
| `TriQ-KEX-256` | pass1 | 47836 | 4492 KiB | 4556 KiB |
| `TriQ-KEX-256` | pass2 | 47836 | 4280 KiB | 4344 KiB |
| `TriQ-KEX-256` | derive_a | 47836 | 4440 KiB | 4504 KiB |
| `TriQ-KEX-256` | derive_b | 47836 | 3788 KiB | 3852 KiB |
| `TriQ-KEX-384` | exchange | 58368 | 5132 KiB | 5196 KiB |
| `TriQ-KEX-384` | init_a | 58368 | 4476 KiB | 5584 KiB |
| `TriQ-KEX-384` | init_b | 58368 | 4848 KiB | 4912 KiB |
| `TriQ-KEX-384` | pass1 | 58368 | 5000 KiB | 5568 KiB |
| `TriQ-KEX-384` | pass2 | 58368 | 5328 KiB | 5392 KiB |
| `TriQ-KEX-384` | derive_a | 58368 | 5152 KiB | 5644 KiB |
| `TriQ-KEX-384` | derive_b | 58368 | 5384 KiB | 5636 KiB |
| `TriQ-KEX-512` | exchange | 64824 | 6284 KiB | 6348 KiB |
| `TriQ-KEX-512` | init_a | 64824 | 6320 KiB | 6384 KiB |
| `TriQ-KEX-512` | init_b | 64824 | 5960 KiB | 6024 KiB |
| `TriQ-KEX-512` | pass1 | 64824 | 5304 KiB | 5368 KiB |
| `TriQ-KEX-512` | pass2 | 64824 | 6064 KiB | 6128 KiB |
| `TriQ-KEX-512` | derive_a | 64824 | 5524 KiB | 5588 KiB |
| `TriQ-KEX-512` | derive_b | 64824 | 6012 KiB | 6076 KiB |

## 6. Transmission and storage overhead

| instance | passes | messages (bytes) | total | long-term pk / sk | shared secret |
|---|---|---|---|---|---|
| `TriQ-KEX-128` | 2 | 6148 / 8180 | 14328 | 2054 / 2134 | 16 |
| `TriQ-KEX-256` | 2 | 18808 / 24952 | 43760 | 6328 / 6488 | 32 |
| `TriQ-KEX-384` | 2 | 36598 / 48678 | 85276 | 12255 / 12495 | 48 |
| `TriQ-KEX-512` | 2 | 59096 / 78648 | 137744 | 19768 / 20088 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `TriQ-KEX-128` | exchange | 11% | 0.2% | drng 14, pseudoXOF 74.6 |
| `TriQ-KEX-256` | exchange | 5.3% | 0.0% | drng 14, pseudoXOF 65.9, pseudohash 9 |
| `TriQ-KEX-384` | exchange | 6.3% | 0.0% | drng 14, pseudoXOF 70.7, pseudohash 9 |
| `TriQ-KEX-512` | exchange | 5.6% | 0.0% | drng 14, pseudoXOF 63, pseudohash 17 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `TriQ-KEX-128` | KAT log (sha256 `b53a7b6ac1262b30…`) | `kat/kex-09/TriQ-KEX-128.log` |
| `TriQ-KEX-128` | timing derive_a | `records/kex-09/TriQ-KEX-128__derive_a.json` |
| `TriQ-KEX-128` | timing derive_b | `records/kex-09/TriQ-KEX-128__derive_b.json` |
| `TriQ-KEX-128` | timing exchange | `records/kex-09/TriQ-KEX-128__exchange.json` |
| `TriQ-KEX-128` | timing init_a | `records/kex-09/TriQ-KEX-128__init_a.json` |
| `TriQ-KEX-128` | timing init_b | `records/kex-09/TriQ-KEX-128__init_b.json` |
| `TriQ-KEX-128` | timing pass1 | `records/kex-09/TriQ-KEX-128__pass1.json` |
| `TriQ-KEX-128` | timing pass2 | `records/kex-09/TriQ-KEX-128__pass2.json` |
| `TriQ-KEX-128` | hash profile exchange | `profile/kex-09/TriQ-KEX-128__exchange.json` |
| `TriQ-KEX-256` | KAT log (sha256 `ff3859812595cf76…`) | `kat/kex-09/TriQ-KEX-256.log` |
| `TriQ-KEX-256` | timing derive_a | `records/kex-09/TriQ-KEX-256__derive_a.json` |
| `TriQ-KEX-256` | timing derive_b | `records/kex-09/TriQ-KEX-256__derive_b.json` |
| `TriQ-KEX-256` | timing exchange | `records/kex-09/TriQ-KEX-256__exchange.json` |
| `TriQ-KEX-256` | timing init_a | `records/kex-09/TriQ-KEX-256__init_a.json` |
| `TriQ-KEX-256` | timing init_b | `records/kex-09/TriQ-KEX-256__init_b.json` |
| `TriQ-KEX-256` | timing pass1 | `records/kex-09/TriQ-KEX-256__pass1.json` |
| `TriQ-KEX-256` | timing pass2 | `records/kex-09/TriQ-KEX-256__pass2.json` |
| `TriQ-KEX-256` | hash profile exchange | `profile/kex-09/TriQ-KEX-256__exchange.json` |
| `TriQ-KEX-384` | KAT log (sha256 `bb396809c1d4bb7c…`) | `kat/kex-09/TriQ-KEX-384.log` |
| `TriQ-KEX-384` | timing derive_a | `records/kex-09/TriQ-KEX-384__derive_a.json` |
| `TriQ-KEX-384` | timing derive_b | `records/kex-09/TriQ-KEX-384__derive_b.json` |
| `TriQ-KEX-384` | timing exchange | `records/kex-09/TriQ-KEX-384__exchange.json` |
| `TriQ-KEX-384` | timing init_a | `records/kex-09/TriQ-KEX-384__init_a.json` |
| `TriQ-KEX-384` | timing init_b | `records/kex-09/TriQ-KEX-384__init_b.json` |
| `TriQ-KEX-384` | timing pass1 | `records/kex-09/TriQ-KEX-384__pass1.json` |
| `TriQ-KEX-384` | timing pass2 | `records/kex-09/TriQ-KEX-384__pass2.json` |
| `TriQ-KEX-384` | hash profile exchange | `profile/kex-09/TriQ-KEX-384__exchange.json` |
| `TriQ-KEX-512` | KAT log (sha256 `2b6807a92bffa604…`) | `kat/kex-09/TriQ-KEX-512.log` |
| `TriQ-KEX-512` | timing derive_a | `records/kex-09/TriQ-KEX-512__derive_a.json` |
| `TriQ-KEX-512` | timing derive_b | `records/kex-09/TriQ-KEX-512__derive_b.json` |
| `TriQ-KEX-512` | timing exchange | `records/kex-09/TriQ-KEX-512__exchange.json` |
| `TriQ-KEX-512` | timing init_a | `records/kex-09/TriQ-KEX-512__init_a.json` |
| `TriQ-KEX-512` | timing init_b | `records/kex-09/TriQ-KEX-512__init_b.json` |
| `TriQ-KEX-512` | timing pass1 | `records/kex-09/TriQ-KEX-512__pass1.json` |
| `TriQ-KEX-512` | timing pass2 | `records/kex-09/TriQ-KEX-512__pass2.json` |
| `TriQ-KEX-512` | hash profile exchange | `profile/kex-09/TriQ-KEX-512__exchange.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

