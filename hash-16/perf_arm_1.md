# hash-16 LLH — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-16` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101537750001471488.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: LLH
- Implementation versions measured: reference
- Parameter sets: `LLH-256`, `LLH-512`, `LLH-768`, `LLH-1024`

## 2. Assessment environment

| item | value |
|---|---|
| processor | Qualcomm Oryon (CPU 2, one core) |
| machine | ASUS Vivobook S 15 |
| clock | fixed 2.71 GHz (governor performance, minimum = maximum), boost off, SMT none |
| memory | 30562 MiB |
| OS / kernel | Ubuntu 26.04.1 LTS / 7.0.0-34-generic |
| compiler / build tool | gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0 / cmake version 4.2.3 |
| campaign start / end (UTC) | 2026-09-28T15:05:46 / 2026-09-29T02:47:54 |

## 3. Functional testing (KAT)

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-16/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `LLH-256` | guide | PASS |
| `LLH-512` | guide | PASS |
| `LLH-768` | guide | PASS |
| `LLH-1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `LLH-256` | 32 B | 1204 | 37.6 | 448 ns | 71.4 | 100000 (5 × 20000) |
| `LLH-256` | 128 B | 3289 | 25.7 | 1.22 µs | 104.7 | 100000 (5 × 20000) |
| `LLH-256` | 512 B | 11.7 k | 22.9 | 4.35 µs | 117.7 | 100000 (5 × 20000) |
| `LLH-256` | 1024 B | 22.9 k | 22.4 | 8.5 µs | 120.4 | 100000 (5 × 20000) |
| `LLH-256` | 4096 B | 90.3 k | 22.0 | 33.5 µs | 122.2 | 91780 (5 × 18356) |
| `LLH-256` | 8192 B | 180.1 k | 22.0 | 66.8 µs | 122.6 | 46245 (5 × 9249) |
| `LLH-256` | 16384 B | 360.3 k | 22.0 | 134 µs | 122.5 | 23225 (5 × 4645) |
| `LLH-256` | 65536 B | 1.44 M | 21.9 | 533 µs | 122.9 | 6015 (5 × 1203) |
| `LLH-512` | 32 B | 1692 | 52.9 | 629 ns | 50.8 | 100000 (5 × 20000) |
| `LLH-512` | 128 B | 3053 | 23.9 | 1.13 µs | 112.8 | 100000 (5 × 20000) |
| `LLH-512` | 512 B | 10.0 k | 19.6 | 3.72 µs | 137.5 | 100000 (5 × 20000) |
| `LLH-512` | 1024 B | 19.4 k | 18.9 | 7.18 µs | 142.6 | 100000 (5 × 20000) |
| `LLH-512` | 4096 B | 75.6 k | 18.4 | 28 µs | 146.1 | 100000 (5 × 20000) |
| `LLH-512` | 8192 B | 150.3 k | 18.4 | 55.8 µs | 146.9 | 53635 (5 × 10727) |
| `LLH-512` | 16384 B | 299.4 k | 18.3 | 111 µs | 147.5 | 26940 (5 × 5388) |
| `LLH-512` | 65536 B | 1.19 M | 18.2 | 443 µs | 148.0 | 7060 (5 × 1412) |
| `LLH-768` | 32 B | 5436 | 169.9 | 2.02 µs | 15.8 | 100000 (5 × 20000) |
| `LLH-768` | 128 B | 8699 | 68.0 | 3.23 µs | 39.6 | 100000 (5 × 20000) |
| `LLH-768` | 512 B | 21.8 k | 42.6 | 8.09 µs | 63.3 | 100000 (5 × 20000) |
| `LLH-768` | 1024 B | 38.4 k | 37.5 | 14.2 µs | 71.9 | 100000 (5 × 20000) |
| `LLH-768` | 4096 B | 143.3 k | 35.0 | 53.2 µs | 77.0 | 56405 (5 × 11281) |
| `LLH-768` | 8192 B | 283.9 k | 34.6 | 105 µs | 77.8 | 29415 (5 × 5883) |
| `LLH-768` | 16384 B | 562.0 k | 34.3 | 209 µs | 78.6 | 14970 (5 × 2994) |
| `LLH-768` | 65536 B | 2.24 M | 34.1 | 830 µs | 78.9 | 3810 (5 × 762) |
| `LLH-1024` | 32 B | 5170 | 161.6 | 1.92 µs | 16.7 | 100000 (5 × 20000) |
| `LLH-1024` | 128 B | 5838 | 45.6 | 2.17 µs | 59.0 | 100000 (5 × 20000) |
| `LLH-1024` | 512 B | 16.0 k | 31.2 | 5.93 µs | 86.3 | 100000 (5 × 20000) |
| `LLH-1024` | 1024 B | 29.5 k | 28.8 | 11 µs | 93.5 | 100000 (5 × 20000) |
| `LLH-1024` | 4096 B | 110.5 k | 27.0 | 41 µs | 99.9 | 73735 (5 × 14747) |
| `LLH-1024` | 8192 B | 218.4 k | 26.7 | 81.1 µs | 101.1 | 38190 (5 × 7638) |
| `LLH-1024` | 16384 B | 433.9 k | 26.5 | 161 µs | 101.8 | 19460 (5 × 3892) |
| `LLH-1024` | 65536 B | 1.73 M | 26.4 | 642 µs | 102.1 | 4925 (5 × 985) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `LLH-256` | hash_32 | 11676 | 1400 KiB | 1464 KiB |
| `LLH-256` | hash_128 | 11676 | 1400 KiB | 1464 KiB |
| `LLH-256` | hash_512 | 11676 | 1400 KiB | 1464 KiB |
| `LLH-256` | hash_1024 | 11676 | 1404 KiB | 1468 KiB |
| `LLH-256` | hash_4096 | 11676 | 1404 KiB | 1468 KiB |
| `LLH-256` | hash_8192 | 11676 | 1408 KiB | 1472 KiB |
| `LLH-256` | hash_16384 | 11676 | 1416 KiB | 1480 KiB |
| `LLH-256` | hash_65536 | 11676 | 1464 KiB | 1528 KiB |
| `LLH-512` | hash_32 | 11852 | 1400 KiB | 1464 KiB |
| `LLH-512` | hash_128 | 11852 | 1400 KiB | 1464 KiB |
| `LLH-512` | hash_512 | 11852 | 1400 KiB | 1464 KiB |
| `LLH-512` | hash_1024 | 11852 | 1404 KiB | 1468 KiB |
| `LLH-512` | hash_4096 | 11852 | 1404 KiB | 1468 KiB |
| `LLH-512` | hash_8192 | 11852 | 1408 KiB | 1472 KiB |
| `LLH-512` | hash_16384 | 11852 | 1412 KiB | 1476 KiB |
| `LLH-512` | hash_65536 | 11852 | 3504 KiB | 3568 KiB |
| `LLH-768` | hash_32 | 13884 | 1404 KiB | 1468 KiB |
| `LLH-768` | hash_128 | 13884 | 1404 KiB | 1468 KiB |
| `LLH-768` | hash_512 | 13884 | 1404 KiB | 1468 KiB |
| `LLH-768` | hash_1024 | 13884 | 1408 KiB | 1472 KiB |
| `LLH-768` | hash_4096 | 13884 | 1408 KiB | 1472 KiB |
| `LLH-768` | hash_8192 | 13884 | 1412 KiB | 1476 KiB |
| `LLH-768` | hash_16384 | 13884 | 1420 KiB | 1484 KiB |
| `LLH-768` | hash_65536 | 13884 | 1468 KiB | 1532 KiB |
| `LLH-1024` | hash_32 | 13316 | 1400 KiB | 1464 KiB |
| `LLH-1024` | hash_128 | 13316 | 1400 KiB | 1464 KiB |
| `LLH-1024` | hash_512 | 13316 | 1400 KiB | 1464 KiB |
| `LLH-1024` | hash_1024 | 13316 | 1404 KiB | 1468 KiB |
| `LLH-1024` | hash_4096 | 13316 | 1404 KiB | 1468 KiB |
| `LLH-1024` | hash_8192 | 13316 | 1408 KiB | 1472 KiB |
| `LLH-1024` | hash_16384 | 13316 | 1416 KiB | 1480 KiB |
| `LLH-1024` | hash_65536 | 13316 | 1464 KiB | 1528 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `LLH-256` | KAT log (sha256 `599f712903ca0ab8…`) | `kat/hash-16/LLH-256.log` |
| `LLH-256` | timing hash_1024 | `records/hash-16/LLH-256__hash_1024.json` |
| `LLH-256` | timing hash_128 | `records/hash-16/LLH-256__hash_128.json` |
| `LLH-256` | timing hash_16384 | `records/hash-16/LLH-256__hash_16384.json` |
| `LLH-256` | timing hash_32 | `records/hash-16/LLH-256__hash_32.json` |
| `LLH-256` | timing hash_4096 | `records/hash-16/LLH-256__hash_4096.json` |
| `LLH-256` | timing hash_512 | `records/hash-16/LLH-256__hash_512.json` |
| `LLH-256` | timing hash_65536 | `records/hash-16/LLH-256__hash_65536.json` |
| `LLH-256` | timing hash_8192 | `records/hash-16/LLH-256__hash_8192.json` |
| `LLH-512` | KAT log (sha256 `c84a84388ddd2127…`) | `kat/hash-16/LLH-512.log` |
| `LLH-512` | timing hash_1024 | `records/hash-16/LLH-512__hash_1024.json` |
| `LLH-512` | timing hash_128 | `records/hash-16/LLH-512__hash_128.json` |
| `LLH-512` | timing hash_16384 | `records/hash-16/LLH-512__hash_16384.json` |
| `LLH-512` | timing hash_32 | `records/hash-16/LLH-512__hash_32.json` |
| `LLH-512` | timing hash_4096 | `records/hash-16/LLH-512__hash_4096.json` |
| `LLH-512` | timing hash_512 | `records/hash-16/LLH-512__hash_512.json` |
| `LLH-512` | timing hash_65536 | `records/hash-16/LLH-512__hash_65536.json` |
| `LLH-512` | timing hash_8192 | `records/hash-16/LLH-512__hash_8192.json` |
| `LLH-768` | KAT log (sha256 `2a2f458d943729e8…`) | `kat/hash-16/LLH-768.log` |
| `LLH-768` | timing hash_1024 | `records/hash-16/LLH-768__hash_1024.json` |
| `LLH-768` | timing hash_128 | `records/hash-16/LLH-768__hash_128.json` |
| `LLH-768` | timing hash_16384 | `records/hash-16/LLH-768__hash_16384.json` |
| `LLH-768` | timing hash_32 | `records/hash-16/LLH-768__hash_32.json` |
| `LLH-768` | timing hash_4096 | `records/hash-16/LLH-768__hash_4096.json` |
| `LLH-768` | timing hash_512 | `records/hash-16/LLH-768__hash_512.json` |
| `LLH-768` | timing hash_65536 | `records/hash-16/LLH-768__hash_65536.json` |
| `LLH-768` | timing hash_8192 | `records/hash-16/LLH-768__hash_8192.json` |
| `LLH-1024` | KAT log (sha256 `a521d0ca53249f0e…`) | `kat/hash-16/LLH-1024.log` |
| `LLH-1024` | timing hash_1024 | `records/hash-16/LLH-1024__hash_1024.json` |
| `LLH-1024` | timing hash_128 | `records/hash-16/LLH-1024__hash_128.json` |
| `LLH-1024` | timing hash_16384 | `records/hash-16/LLH-1024__hash_16384.json` |
| `LLH-1024` | timing hash_32 | `records/hash-16/LLH-1024__hash_32.json` |
| `LLH-1024` | timing hash_4096 | `records/hash-16/LLH-1024__hash_4096.json` |
| `LLH-1024` | timing hash_512 | `records/hash-16/LLH-1024__hash_512.json` |
| `LLH-1024` | timing hash_65536 | `records/hash-16/LLH-1024__hash_65536.json` |
| `LLH-1024` | timing hash_8192 | `records/hash-16/LLH-1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

