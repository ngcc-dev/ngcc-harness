# kex-01 ADKEX (Authenticated Ding Key Exchange) — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kex-01` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560625202810880.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key exchange
- Algorithm: ADKEX (Authenticated Ding Key Exchange)
- Implementation versions measured: reference
- Parameter sets: `ADKEX-128`, `ADKEX-256`, `ADKEX-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kex-01/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `ADKEX-128` | guide | PASS |
| `ADKEX-256` | guide | PASS |
| `ADKEX-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `ADKEX-128` | exchange | 1.33 M | 494 µs | 2.02e+03 | 494 µs | 6405 (5 × 1281) |
| `ADKEX-128` | init_a | 186 | 24.4 ns | 4.11e+07 | 24.5 ns | 6405 (5 × 1281) |
| `ADKEX-128` | init_b | 175.2 k | 65 µs | 1.54e+04 | 64.7 µs | 6405 (5 × 1281) |
| `ADKEX-128` | pass1 | 372.9 k | 138 µs | 7.23e+03 | 138 µs | 6405 (5 × 1281) |
| `ADKEX-128` | pass2 | 488.5 k | 181 µs | 5.52e+03 | 181 µs | 6405 (5 × 1281) |
| `ADKEX-128` | derive_a | 290.5 k | 108 µs | 9.28e+03 | 108 µs | 6405 (5 × 1281) |
| `ADKEX-128` | derive_b | 2845 | 1.01 µs | 9.94e+05 | 1.01 µs | 6405 (5 × 1281) |
| `ADKEX-256` | exchange | 3.64 M | 1.35 ms | 742 | 1.35 ms | 2285 (5 × 457) |
| `ADKEX-256` | init_a | 184 | 24.6 ns | 4.07e+07 | 23.7 ns | 2285 (5 × 457) |
| `ADKEX-256` | init_b | 532.7 k | 198 µs | 5.06e+03 | 198 µs | 2285 (5 × 457) |
| `ADKEX-256` | pass1 | 1.10 M | 407 µs | 2.46e+03 | 405 µs | 2285 (5 × 457) |
| `ADKEX-256` | pass2 | 1.29 M | 477 µs | 2.1e+03 | 476 µs | 2285 (5 × 457) |
| `ADKEX-256` | derive_a | 724.0 k | 269 µs | 3.72e+03 | 269 µs | 2285 (5 × 457) |
| `ADKEX-256` | derive_b | 2828 | 1 µs | 9.99e+05 | 999 ns | 2285 (5 × 457) |
| `ADKEX-512` | exchange | 12.84 M | 4.76 ms | 210 | 4.76 ms | 650 (5 × 130) |
| `ADKEX-512` | init_a | 191 | 27.6 ns | 3.63e+07 | 28.8 ns | 650 (5 × 130) |
| `ADKEX-512` | init_b | 1.87 M | 693 µs | 1.44e+03 | 693 µs | 650 (5 × 130) |
| `ADKEX-512` | pass1 | 3.84 M | 1.43 ms | 701 | 1.42 ms | 650 (5 × 130) |
| `ADKEX-512` | pass2 | 4.57 M | 1.69 ms | 590 | 1.68 ms | 650 (5 × 130) |
| `ADKEX-512` | derive_a | 2.60 M | 964 µs | 1.04e+03 | 960 µs | 650 (5 × 130) |
| `ADKEX-512` | derive_b | 10.6 k | 3.91 µs | 2.55e+05 | 3.89 µs | 650 (5 × 130) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `ADKEX-128` | exchange | 30100 | 1472 KiB | 1536 KiB |
| `ADKEX-128` | init_a | 30100 | 1472 KiB | 1536 KiB |
| `ADKEX-128` | init_b | 30100 | 1472 KiB | 1536 KiB |
| `ADKEX-128` | pass1 | 30100 | 1468 KiB | 1532 KiB |
| `ADKEX-128` | pass2 | 30100 | 1472 KiB | 1536 KiB |
| `ADKEX-128` | derive_a | 30100 | 1468 KiB | 1532 KiB |
| `ADKEX-128` | derive_b | 30100 | 1472 KiB | 1536 KiB |
| `ADKEX-256` | exchange | 30908 | 3572 KiB | 3636 KiB |
| `ADKEX-256` | init_a | 30908 | 1528 KiB | 1592 KiB |
| `ADKEX-256` | init_b | 30908 | 1528 KiB | 1592 KiB |
| `ADKEX-256` | pass1 | 30908 | 3564 KiB | 3628 KiB |
| `ADKEX-256` | pass2 | 30908 | 1528 KiB | 1592 KiB |
| `ADKEX-256` | derive_a | 30908 | 3532 KiB | 3596 KiB |
| `ADKEX-256` | derive_b | 30908 | 3556 KiB | 3620 KiB |
| `ADKEX-512` | exchange | 32092 | 3688 KiB | 3752 KiB |
| `ADKEX-512` | init_a | 32092 | 1652 KiB | 1728 KiB |
| `ADKEX-512` | init_b | 32092 | 3640 KiB | 3704 KiB |
| `ADKEX-512` | pass1 | 32092 | 3532 KiB | 3596 KiB |
| `ADKEX-512` | pass2 | 32092 | 3560 KiB | 3624 KiB |
| `ADKEX-512` | derive_a | 32092 | 1652 KiB | 1728 KiB |
| `ADKEX-512` | derive_b | 32092 | 1652 KiB | 1728 KiB |

## 6. Transmission and storage overhead

| instance | passes | messages (bytes) | total | long-term pk / sk | shared secret |
|---|---|---|---|---|---|
| `ADKEX-128` | 2 | 1600 / 800 | 2400 | 800 / 1600 | 32 |
| `ADKEX-256` | 2 | 3168 / 1600 | 4768 | 1568 / 3136 | 32 |
| `ADKEX-512` | 2 | 6528 / 3136 | 9664 | 3392 / 6784 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — default DKE_HASH=0; own SM3/HMAC compiled but unreachable; DKE_HASH=2 would switch to SHAKE

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `ADKEX-128` | exchange | 69% | 1.2% | drng 3, pseudoXOF 6, sm3hash 563 |
| `ADKEX-256` | exchange | 73% | 0.4% | drng 3, pseudoXOF 6, sm3hash 1.76e+03 |
| `ADKEX-512` | exchange | 84% | 0.2% | drng 3, pseudoXOF 10, pseudohash 6, sm3hash 3.7e+03 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `ADKEX-128` | KAT log (sha256 `c3c89d89e65aa8b9…`) | `kat/kex-01/ADKEX-128.log` |
| `ADKEX-128` | timing derive_a | `records/kex-01/ADKEX-128__derive_a.json` |
| `ADKEX-128` | timing derive_b | `records/kex-01/ADKEX-128__derive_b.json` |
| `ADKEX-128` | timing exchange | `records/kex-01/ADKEX-128__exchange.json` |
| `ADKEX-128` | timing init_a | `records/kex-01/ADKEX-128__init_a.json` |
| `ADKEX-128` | timing init_b | `records/kex-01/ADKEX-128__init_b.json` |
| `ADKEX-128` | timing pass1 | `records/kex-01/ADKEX-128__pass1.json` |
| `ADKEX-128` | timing pass2 | `records/kex-01/ADKEX-128__pass2.json` |
| `ADKEX-128` | hash profile exchange | `profile/kex-01/ADKEX-128__exchange.json` |
| `ADKEX-256` | KAT log (sha256 `78e3512e964d6163…`) | `kat/kex-01/ADKEX-256.log` |
| `ADKEX-256` | timing derive_a | `records/kex-01/ADKEX-256__derive_a.json` |
| `ADKEX-256` | timing derive_b | `records/kex-01/ADKEX-256__derive_b.json` |
| `ADKEX-256` | timing exchange | `records/kex-01/ADKEX-256__exchange.json` |
| `ADKEX-256` | timing init_a | `records/kex-01/ADKEX-256__init_a.json` |
| `ADKEX-256` | timing init_b | `records/kex-01/ADKEX-256__init_b.json` |
| `ADKEX-256` | timing pass1 | `records/kex-01/ADKEX-256__pass1.json` |
| `ADKEX-256` | timing pass2 | `records/kex-01/ADKEX-256__pass2.json` |
| `ADKEX-256` | hash profile exchange | `profile/kex-01/ADKEX-256__exchange.json` |
| `ADKEX-512` | KAT log (sha256 `6638a9c454b67760…`) | `kat/kex-01/ADKEX-512.log` |
| `ADKEX-512` | timing derive_a | `records/kex-01/ADKEX-512__derive_a.json` |
| `ADKEX-512` | timing derive_b | `records/kex-01/ADKEX-512__derive_b.json` |
| `ADKEX-512` | timing exchange | `records/kex-01/ADKEX-512__exchange.json` |
| `ADKEX-512` | timing init_a | `records/kex-01/ADKEX-512__init_a.json` |
| `ADKEX-512` | timing init_b | `records/kex-01/ADKEX-512__init_b.json` |
| `ADKEX-512` | timing pass1 | `records/kex-01/ADKEX-512__pass1.json` |
| `ADKEX-512` | timing pass2 | `records/kex-01/ADKEX-512__pass2.json` |
| `ADKEX-512` | hash profile exchange | `profile/kex-01/ADKEX-512__exchange.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

