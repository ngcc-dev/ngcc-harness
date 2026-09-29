# hash-22 Pavelor: A Highly Secure Hash Algorithm for Software Implementation — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-22` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101536571221692416.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: Pavelor: A Highly Secure Hash Algorithm for Software Implementation
- Implementation versions measured: reference
- Parameter sets: `Pavelor-512`, `Pavelor-768`, `Pavelor-1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-22/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Pavelor-512` | guide | PASS |
| `Pavelor-768` | guide | PASS |
| `Pavelor-1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `Pavelor-512` | 32 B | 36.9 k | 1151.6 | 13.7 µs | 2.3 | 100000 (5 × 20000) |
| `Pavelor-512` | 128 B | 39.4 k | 307.6 | 14.6 µs | 8.8 | 100000 (5 × 20000) |
| `Pavelor-512` | 512 B | 111.2 k | 217.2 | 41.3 µs | 12.4 | 68185 (5 × 13637) |
| `Pavelor-512` | 1024 B | 217.9 k | 212.8 | 80.8 µs | 12.7 | 37040 (5 × 7408) |
| `Pavelor-512` | 4096 B | 795.9 k | 194.3 | 295 µs | 13.9 | 10610 (5 × 2122) |
| `Pavelor-512` | 8192 B | 1.55 M | 189.6 | 576 µs | 14.2 | 4805 (5 × 961) |
| `Pavelor-512` | 16384 B | 3.09 M | 188.7 | 1.15 ms | 14.3 | 2740 (5 × 548) |
| `Pavelor-512` | 65536 B | 12.29 M | 187.5 | 4.56 ms | 14.4 | 680 (5 × 136) |
| `Pavelor-768` | 32 B | 36.7 k | 1147.9 | 13.6 µs | 2.3 | 100000 (5 × 20000) |
| `Pavelor-768` | 128 B | 72.2 k | 564.4 | 26.8 µs | 4.8 | 100000 (5 × 20000) |
| `Pavelor-768` | 512 B | 179.8 k | 351.2 | 66.7 µs | 7.7 | 45455 (5 × 9091) |
| `Pavelor-768` | 1024 B | 323.6 k | 316.0 | 120 µs | 8.5 | 25975 (5 × 5195) |
| `Pavelor-768` | 4096 B | 1.18 M | 288.8 | 439 µs | 9.3 | 7150 (5 × 1430) |
| `Pavelor-768` | 8192 B | 2.34 M | 285.1 | 867 µs | 9.5 | 3640 (5 × 728) |
| `Pavelor-768` | 16384 B | 4.65 M | 283.7 | 1.72 ms | 9.5 | 1820 (5 × 364) |
| `Pavelor-768` | 65536 B | 18.39 M | 280.6 | 6.82 ms | 9.6 | 465 (5 × 93) |
| `Pavelor-1024` | 32 B | 72.6 k | 2269.0 | 26.9 µs | 1.2 | 100000 (5 × 20000) |
| `Pavelor-1024` | 128 B | 143.0 k | 1117.4 | 53.1 µs | 2.4 | 57010 (5 × 11402) |
| `Pavelor-1024` | 512 B | 357.5 k | 698.3 | 133 µs | 3.9 | 23015 (5 × 4603) |
| `Pavelor-1024` | 1024 B | 643.8 k | 628.7 | 239 µs | 4.3 | 12695 (5 × 2539) |
| `Pavelor-1024` | 4096 B | 2.38 M | 580.1 | 882 µs | 4.6 | 3565 (5 × 713) |
| `Pavelor-1024` | 8192 B | 4.66 M | 569.4 | 1.73 ms | 4.7 | 1815 (5 × 363) |
| `Pavelor-1024` | 16384 B | 9.29 M | 566.9 | 3.45 ms | 4.8 | 915 (5 × 183) |
| `Pavelor-1024` | 65536 B | 36.92 M | 563.3 | 13.7 ms | 4.8 | 230 (5 × 46) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Pavelor-512` | hash_32 | 13692 | 1400 KiB | 1464 KiB |
| `Pavelor-512` | hash_128 | 13692 | 1400 KiB | 1464 KiB |
| `Pavelor-512` | hash_512 | 13692 | 1400 KiB | 1464 KiB |
| `Pavelor-512` | hash_1024 | 13692 | 1404 KiB | 1468 KiB |
| `Pavelor-512` | hash_4096 | 13692 | 1400 KiB | 1464 KiB |
| `Pavelor-512` | hash_8192 | 13692 | 1408 KiB | 1472 KiB |
| `Pavelor-512` | hash_16384 | 13692 | 1416 KiB | 1480 KiB |
| `Pavelor-512` | hash_65536 | 13692 | 1464 KiB | 1528 KiB |
| `Pavelor-768` | hash_32 | 13692 | 1400 KiB | 1464 KiB |
| `Pavelor-768` | hash_128 | 13692 | 1400 KiB | 1464 KiB |
| `Pavelor-768` | hash_512 | 13692 | 1400 KiB | 1464 KiB |
| `Pavelor-768` | hash_1024 | 13692 | 1404 KiB | 1468 KiB |
| `Pavelor-768` | hash_4096 | 13692 | 1404 KiB | 1468 KiB |
| `Pavelor-768` | hash_8192 | 13692 | 1408 KiB | 1472 KiB |
| `Pavelor-768` | hash_16384 | 13692 | 1416 KiB | 1480 KiB |
| `Pavelor-768` | hash_65536 | 13692 | 1464 KiB | 1528 KiB |
| `Pavelor-1024` | hash_32 | 13692 | 1400 KiB | 1464 KiB |
| `Pavelor-1024` | hash_128 | 13692 | 1400 KiB | 1464 KiB |
| `Pavelor-1024` | hash_512 | 13692 | 1400 KiB | 1464 KiB |
| `Pavelor-1024` | hash_1024 | 13692 | 1404 KiB | 1468 KiB |
| `Pavelor-1024` | hash_4096 | 13692 | 1404 KiB | 1468 KiB |
| `Pavelor-1024` | hash_8192 | 13692 | 1408 KiB | 1472 KiB |
| `Pavelor-1024` | hash_16384 | 13692 | 1416 KiB | 1480 KiB |
| `Pavelor-1024` | hash_65536 | 13692 | 1464 KiB | 1528 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Pavelor-512` | KAT log (sha256 `f8a4c5b36f24ca8f…`) | `kat/hash-22/Pavelor-512.log` |
| `Pavelor-512` | timing hash_1024 | `records/hash-22/Pavelor-512__hash_1024.json` |
| `Pavelor-512` | timing hash_128 | `records/hash-22/Pavelor-512__hash_128.json` |
| `Pavelor-512` | timing hash_16384 | `records/hash-22/Pavelor-512__hash_16384.json` |
| `Pavelor-512` | timing hash_32 | `records/hash-22/Pavelor-512__hash_32.json` |
| `Pavelor-512` | timing hash_4096 | `records/hash-22/Pavelor-512__hash_4096.json` |
| `Pavelor-512` | timing hash_512 | `records/hash-22/Pavelor-512__hash_512.json` |
| `Pavelor-512` | timing hash_65536 | `records/hash-22/Pavelor-512__hash_65536.json` |
| `Pavelor-512` | timing hash_8192 | `records/hash-22/Pavelor-512__hash_8192.json` |
| `Pavelor-768` | KAT log (sha256 `41ed86f622b25874…`) | `kat/hash-22/Pavelor-768.log` |
| `Pavelor-768` | timing hash_1024 | `records/hash-22/Pavelor-768__hash_1024.json` |
| `Pavelor-768` | timing hash_128 | `records/hash-22/Pavelor-768__hash_128.json` |
| `Pavelor-768` | timing hash_16384 | `records/hash-22/Pavelor-768__hash_16384.json` |
| `Pavelor-768` | timing hash_32 | `records/hash-22/Pavelor-768__hash_32.json` |
| `Pavelor-768` | timing hash_4096 | `records/hash-22/Pavelor-768__hash_4096.json` |
| `Pavelor-768` | timing hash_512 | `records/hash-22/Pavelor-768__hash_512.json` |
| `Pavelor-768` | timing hash_65536 | `records/hash-22/Pavelor-768__hash_65536.json` |
| `Pavelor-768` | timing hash_8192 | `records/hash-22/Pavelor-768__hash_8192.json` |
| `Pavelor-1024` | KAT log (sha256 `15a80bb7712bb206…`) | `kat/hash-22/Pavelor-1024.log` |
| `Pavelor-1024` | timing hash_1024 | `records/hash-22/Pavelor-1024__hash_1024.json` |
| `Pavelor-1024` | timing hash_128 | `records/hash-22/Pavelor-1024__hash_128.json` |
| `Pavelor-1024` | timing hash_16384 | `records/hash-22/Pavelor-1024__hash_16384.json` |
| `Pavelor-1024` | timing hash_32 | `records/hash-22/Pavelor-1024__hash_32.json` |
| `Pavelor-1024` | timing hash_4096 | `records/hash-22/Pavelor-1024__hash_4096.json` |
| `Pavelor-1024` | timing hash_512 | `records/hash-22/Pavelor-1024__hash_512.json` |
| `Pavelor-1024` | timing hash_65536 | `records/hash-22/Pavelor-1024__hash_65536.json` |
| `Pavelor-1024` | timing hash_8192 | `records/hash-22/Pavelor-1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

