# kex-02 AFS-KEX — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kex-02` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560625362194432.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key exchange
- Algorithm: AFS-KEX
- Implementation versions measured: reference
- Parameter sets: `AFS_KEX_C128`, `AFS_KEX_C256`, `AFS_KEX_C512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kex-02/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `AFS_KEX_C128` | guide | PASS |
| `AFS_KEX_C256` | guide | PASS |
| `AFS_KEX_C512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `AFS_KEX_C128` | exchange | 2.38 M | 883 µs | 1.13e+03 | 884 µs | 3595 (5 × 719) |
| `AFS_KEX_C128` | init_a | 439.7 k | 163 µs | 6.13e+03 | 163 µs | 3595 (5 × 719) |
| `AFS_KEX_C128` | init_b | 441.8 k | 164 µs | 6.1e+03 | 164 µs | 3595 (5 × 719) |
| `AFS_KEX_C128` | pass1 | 246.6 k | 91.5 µs | 1.09e+04 | 91.5 µs | 3595 (5 × 719) |
| `AFS_KEX_C128` | pass2 | 537.4 k | 199 µs | 5.02e+03 | 199 µs | 3595 (5 × 719) |
| `AFS_KEX_C128` | pass3 | 503.7 k | 187 µs | 5.35e+03 | 187 µs | 3595 (5 × 719) |
| `AFS_KEX_C128` | pass4 | 212.2 k | 78.7 µs | 1.27e+04 | 78.4 µs | 3595 (5 × 719) |
| `AFS_KEX_C128` | derive_a | 197 | 24 ns | 4.17e+07 | 22.8 ns | 3595 (5 × 719) |
| `AFS_KEX_C128` | derive_b | 326 | 27.1 ns | 3.69e+07 | 28.3 ns | 3595 (5 × 719) |
| `AFS_KEX_C256` | exchange | 5.84 M | 2.17 ms | 462 | 2.16 ms | 1410 (5 × 282) |
| `AFS_KEX_C256` | init_a | 1.12 M | 417 µs | 2.4e+03 | 416 µs | 1410 (5 × 282) |
| `AFS_KEX_C256` | init_b | 1.13 M | 418 µs | 2.39e+03 | 415 µs | 1410 (5 × 282) |
| `AFS_KEX_C256` | pass1 | 579.9 k | 215 µs | 4.65e+03 | 215 µs | 1410 (5 × 282) |
| `AFS_KEX_C256` | pass2 | 1.26 M | 467 µs | 2.14e+03 | 466 µs | 1410 (5 × 282) |
| `AFS_KEX_C256` | pass3 | 1.21 M | 450 µs | 2.22e+03 | 449 µs | 1410 (5 × 282) |
| `AFS_KEX_C256` | pass4 | 537.9 k | 199 µs | 5.01e+03 | 199 µs | 1410 (5 × 282) |
| `AFS_KEX_C256` | derive_a | 190 | 23.9 ns | 4.19e+07 | 24 ns | 1410 (5 × 282) |
| `AFS_KEX_C256` | derive_b | 207 | 24.1 ns | 4.15e+07 | 24.4 ns | 1410 (5 × 282) |
| `AFS_KEX_C512` | exchange | 20.05 M | 7.44 ms | 134 | 7.44 ms | 425 (5 × 85) |
| `AFS_KEX_C512` | init_a | 3.85 M | 1.43 ms | 701 | 1.43 ms | 425 (5 × 85) |
| `AFS_KEX_C512` | init_b | 3.86 M | 1.43 ms | 699 | 1.43 ms | 425 (5 × 85) |
| `AFS_KEX_C512` | pass1 | 1.99 M | 739 µs | 1.35e+03 | 739 µs | 425 (5 × 85) |
| `AFS_KEX_C512` | pass2 | 4.29 M | 1.59 ms | 629 | 1.59 ms | 425 (5 × 85) |
| `AFS_KEX_C512` | pass3 | 4.19 M | 1.55 ms | 644 | 1.55 ms | 425 (5 × 85) |
| `AFS_KEX_C512` | pass4 | 1.88 M | 699 µs | 1.43e+03 | 699 µs | 425 (5 × 85) |
| `AFS_KEX_C512` | derive_a | 189 | 23.5 ns | 4.25e+07 | 21.4 ns | 425 (5 × 85) |
| `AFS_KEX_C512` | derive_b | 207 | 25.4 ns | 3.94e+07 | 27 ns | 425 (5 × 85) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `AFS_KEX_C128` | exchange | 29872 | 3484 KiB | 3548 KiB |
| `AFS_KEX_C128` | init_a | 29872 | 1468 KiB | 1532 KiB |
| `AFS_KEX_C128` | init_b | 29872 | 1472 KiB | 1536 KiB |
| `AFS_KEX_C128` | pass1 | 29872 | 1472 KiB | 1536 KiB |
| `AFS_KEX_C128` | pass2 | 29872 | 1472 KiB | 1536 KiB |
| `AFS_KEX_C128` | pass3 | 29872 | 1472 KiB | 1536 KiB |
| `AFS_KEX_C128` | pass4 | 29872 | 1472 KiB | 1536 KiB |
| `AFS_KEX_C128` | derive_a | 29872 | 1472 KiB | 1536 KiB |
| `AFS_KEX_C128` | derive_b | 29872 | 1472 KiB | 1536 KiB |
| `AFS_KEX_C256` | exchange | 36336 | 1528 KiB | 1596 KiB |
| `AFS_KEX_C256` | init_a | 36336 | 1528 KiB | 1596 KiB |
| `AFS_KEX_C256` | init_b | 36336 | 1528 KiB | 1596 KiB |
| `AFS_KEX_C256` | pass1 | 36336 | 1528 KiB | 1596 KiB |
| `AFS_KEX_C256` | pass2 | 36336 | 3568 KiB | 3632 KiB |
| `AFS_KEX_C256` | pass3 | 36336 | 1528 KiB | 1596 KiB |
| `AFS_KEX_C256` | pass4 | 36336 | 3520 KiB | 3584 KiB |
| `AFS_KEX_C256` | derive_a | 36336 | 1528 KiB | 1596 KiB |
| `AFS_KEX_C256` | derive_b | 36336 | 1528 KiB | 1596 KiB |
| `AFS_KEX_C512` | exchange | 37936 | 1636 KiB | 1704 KiB |
| `AFS_KEX_C512` | init_a | 37936 | 1636 KiB | 1704 KiB |
| `AFS_KEX_C512` | init_b | 37936 | 1636 KiB | 1704 KiB |
| `AFS_KEX_C512` | pass1 | 37936 | 3640 KiB | 3704 KiB |
| `AFS_KEX_C512` | pass2 | 37936 | 1636 KiB | 1704 KiB |
| `AFS_KEX_C512` | pass3 | 37936 | 1636 KiB | 1704 KiB |
| `AFS_KEX_C512` | pass4 | 37936 | 3540 KiB | 3604 KiB |
| `AFS_KEX_C512` | derive_a | 37936 | 1636 KiB | 1704 KiB |
| `AFS_KEX_C512` | derive_b | 37936 | 1636 KiB | 1704 KiB |

## 6. Transmission and storage overhead

| instance | passes | messages (bytes) | total | long-term pk / sk | shared secret |
|---|---|---|---|---|---|
| `AFS_KEX_C128` | 4 | 768 / 784 / 16 / 1568 | 3136 | 1568 / 3170 | 16 |
| `AFS_KEX_C256` | 4 | 1440 / 1472 / 32 / 2944 | 5888 | 3136 / 6338 | 32 |
| `AFS_KEX_C512` | 4 | 2944 / 3008 / 64 / 6016 | 12032 | 6272 / 12674 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — optimized AVX2 auxfunc.c adds an SM3 counter-block fast path

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `AFS_KEX_C128` | exchange | 69% | 1.1% | drng 6, pseudoXOF 88.3, sm3hash 6 |
| `AFS_KEX_C256` | exchange | 76% | 0.5% | drng 6, pseudoXOF 249, pseudohash 6 |
| `AFS_KEX_C512` | exchange | 81% | 0.2% | drng 6, pseudoXOF 248, pseudohash 6 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `AFS_KEX_C128` | KAT log (sha256 `64aabe0de73942cc…`) | `kat/kex-02/AFS_KEX_C128.log` |
| `AFS_KEX_C128` | timing derive_a | `records/kex-02/AFS_KEX_C128__derive_a.json` |
| `AFS_KEX_C128` | timing derive_b | `records/kex-02/AFS_KEX_C128__derive_b.json` |
| `AFS_KEX_C128` | timing exchange | `records/kex-02/AFS_KEX_C128__exchange.json` |
| `AFS_KEX_C128` | timing init_a | `records/kex-02/AFS_KEX_C128__init_a.json` |
| `AFS_KEX_C128` | timing init_b | `records/kex-02/AFS_KEX_C128__init_b.json` |
| `AFS_KEX_C128` | timing pass1 | `records/kex-02/AFS_KEX_C128__pass1.json` |
| `AFS_KEX_C128` | timing pass2 | `records/kex-02/AFS_KEX_C128__pass2.json` |
| `AFS_KEX_C128` | timing pass3 | `records/kex-02/AFS_KEX_C128__pass3.json` |
| `AFS_KEX_C128` | timing pass4 | `records/kex-02/AFS_KEX_C128__pass4.json` |
| `AFS_KEX_C128` | hash profile exchange | `profile/kex-02/AFS_KEX_C128__exchange.json` |
| `AFS_KEX_C256` | KAT log (sha256 `89e367f06ce79d34…`) | `kat/kex-02/AFS_KEX_C256.log` |
| `AFS_KEX_C256` | timing derive_a | `records/kex-02/AFS_KEX_C256__derive_a.json` |
| `AFS_KEX_C256` | timing derive_b | `records/kex-02/AFS_KEX_C256__derive_b.json` |
| `AFS_KEX_C256` | timing exchange | `records/kex-02/AFS_KEX_C256__exchange.json` |
| `AFS_KEX_C256` | timing init_a | `records/kex-02/AFS_KEX_C256__init_a.json` |
| `AFS_KEX_C256` | timing init_b | `records/kex-02/AFS_KEX_C256__init_b.json` |
| `AFS_KEX_C256` | timing pass1 | `records/kex-02/AFS_KEX_C256__pass1.json` |
| `AFS_KEX_C256` | timing pass2 | `records/kex-02/AFS_KEX_C256__pass2.json` |
| `AFS_KEX_C256` | timing pass3 | `records/kex-02/AFS_KEX_C256__pass3.json` |
| `AFS_KEX_C256` | timing pass4 | `records/kex-02/AFS_KEX_C256__pass4.json` |
| `AFS_KEX_C256` | hash profile exchange | `profile/kex-02/AFS_KEX_C256__exchange.json` |
| `AFS_KEX_C512` | KAT log (sha256 `0d55876061394ba8…`) | `kat/kex-02/AFS_KEX_C512.log` |
| `AFS_KEX_C512` | timing derive_a | `records/kex-02/AFS_KEX_C512__derive_a.json` |
| `AFS_KEX_C512` | timing derive_b | `records/kex-02/AFS_KEX_C512__derive_b.json` |
| `AFS_KEX_C512` | timing exchange | `records/kex-02/AFS_KEX_C512__exchange.json` |
| `AFS_KEX_C512` | timing init_a | `records/kex-02/AFS_KEX_C512__init_a.json` |
| `AFS_KEX_C512` | timing init_b | `records/kex-02/AFS_KEX_C512__init_b.json` |
| `AFS_KEX_C512` | timing pass1 | `records/kex-02/AFS_KEX_C512__pass1.json` |
| `AFS_KEX_C512` | timing pass2 | `records/kex-02/AFS_KEX_C512__pass2.json` |
| `AFS_KEX_C512` | timing pass3 | `records/kex-02/AFS_KEX_C512__pass3.json` |
| `AFS_KEX_C512` | timing pass4 | `records/kex-02/AFS_KEX_C512__pass4.json` |
| `AFS_KEX_C512` | hash profile exchange | `profile/kex-02/AFS_KEX_C512__exchange.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

