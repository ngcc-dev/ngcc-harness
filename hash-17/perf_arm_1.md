# hash-17 MasterCube — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-17` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101537586067099648.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: MasterCube
- Implementation versions measured: reference
- Parameter sets: `MasterCube-512`, `MasterCube-768`, `MasterCube-1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-17/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `MasterCube-512` | guide | PASS |
| `MasterCube-768` | guide | PASS |
| `MasterCube-1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `MasterCube-512` | 32 B | 14.8 k | 463.0 | 5.5 µs | 5.8 | 100000 (5 × 20000) |
| `MasterCube-512` | 128 B | 23.8 k | 185.9 | 8.83 µs | 14.5 | 100000 (5 × 20000) |
| `MasterCube-512` | 512 B | 44.2 k | 86.3 | 16.4 µs | 31.2 | 100000 (5 × 20000) |
| `MasterCube-512` | 1024 B | 73.4 k | 71.7 | 27.2 µs | 37.6 | 100000 (5 × 20000) |
| `MasterCube-512` | 4096 B | 263.3 k | 64.3 | 97.7 µs | 41.9 | 30535 (5 × 6107) |
| `MasterCube-512` | 8192 B | 548.8 k | 67.0 | 204 µs | 40.2 | 16390 (5 × 3278) |
| `MasterCube-512` | 16384 B | 1.08 M | 65.9 | 400 µs | 40.9 | 8365 (5 × 1673) |
| `MasterCube-512` | 65536 B | 4.00 M | 61.0 | 1.48 ms | 44.2 | 2125 (5 × 425) |
| `MasterCube-768` | 32 B | 22.1 k | 691.8 | 8.22 µs | 3.9 | 100000 (5 × 20000) |
| `MasterCube-768` | 128 B | 29.5 k | 230.8 | 11 µs | 11.7 | 100000 (5 × 20000) |
| `MasterCube-768` | 512 B | 58.8 k | 114.9 | 21.8 µs | 23.4 | 100000 (5 × 20000) |
| `MasterCube-768` | 1024 B | 102.7 k | 100.3 | 38.1 µs | 26.9 | 76560 (5 × 15312) |
| `MasterCube-768` | 4096 B | 358.1 k | 87.4 | 133 µs | 30.8 | 23430 (5 × 4686) |
| `MasterCube-768` | 8192 B | 701.0 k | 85.6 | 260 µs | 31.5 | 12075 (5 × 2415) |
| `MasterCube-768` | 16384 B | 1.38 M | 84.2 | 512 µs | 32.0 | 6150 (5 × 1230) |
| `MasterCube-768` | 65536 B | 5.45 M | 83.2 | 2.02 ms | 32.4 | 1540 (5 × 308) |
| `MasterCube-1024` | 32 B | 29.5 k | 922.0 | 11 µs | 2.9 | 100000 (5 × 20000) |
| `MasterCube-1024` | 128 B | 44.2 k | 345.6 | 16.4 µs | 7.8 | 100000 (5 × 20000) |
| `MasterCube-1024` | 512 B | 95.5 k | 186.4 | 35.4 µs | 14.5 | 84360 (5 × 16872) |
| `MasterCube-1024` | 1024 B | 161.2 k | 157.4 | 59.8 µs | 17.1 | 45200 (5 × 9040) |
| `MasterCube-1024` | 4096 B | 562.9 k | 137.4 | 209 µs | 19.6 | 14510 (5 × 2902) |
| `MasterCube-1024` | 8192 B | 1.10 M | 133.8 | 407 µs | 20.2 | 6870 (5 × 1374) |
| `MasterCube-1024` | 16384 B | 2.16 M | 131.9 | 802 µs | 20.4 | 3940 (5 × 788) |
| `MasterCube-1024` | 65536 B | 8.57 M | 130.8 | 3.18 ms | 20.6 | 965 (5 × 193) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `MasterCube-512` | hash_32 | 15244 | 1404 KiB | 1468 KiB |
| `MasterCube-512` | hash_128 | 15244 | 1400 KiB | 1464 KiB |
| `MasterCube-512` | hash_512 | 15244 | 1404 KiB | 1468 KiB |
| `MasterCube-512` | hash_1024 | 15244 | 1408 KiB | 1472 KiB |
| `MasterCube-512` | hash_4096 | 15244 | 1408 KiB | 1472 KiB |
| `MasterCube-512` | hash_8192 | 15244 | 1412 KiB | 1476 KiB |
| `MasterCube-512` | hash_16384 | 15244 | 1416 KiB | 1480 KiB |
| `MasterCube-512` | hash_65536 | 15244 | 3496 KiB | 3560 KiB |
| `MasterCube-768` | hash_32 | 15308 | 1404 KiB | 1468 KiB |
| `MasterCube-768` | hash_128 | 15308 | 1404 KiB | 1468 KiB |
| `MasterCube-768` | hash_512 | 15308 | 1404 KiB | 1468 KiB |
| `MasterCube-768` | hash_1024 | 15308 | 1408 KiB | 1472 KiB |
| `MasterCube-768` | hash_4096 | 15308 | 1408 KiB | 1472 KiB |
| `MasterCube-768` | hash_8192 | 15308 | 1412 KiB | 1476 KiB |
| `MasterCube-768` | hash_16384 | 15308 | 1420 KiB | 1484 KiB |
| `MasterCube-768` | hash_65536 | 15308 | 1468 KiB | 1532 KiB |
| `MasterCube-1024` | hash_32 | 15308 | 1404 KiB | 1468 KiB |
| `MasterCube-1024` | hash_128 | 15308 | 1404 KiB | 1468 KiB |
| `MasterCube-1024` | hash_512 | 15308 | 1404 KiB | 1468 KiB |
| `MasterCube-1024` | hash_1024 | 15308 | 3444 KiB | 3508 KiB |
| `MasterCube-1024` | hash_4096 | 15308 | 1408 KiB | 1472 KiB |
| `MasterCube-1024` | hash_8192 | 15308 | 1412 KiB | 1476 KiB |
| `MasterCube-1024` | hash_16384 | 15308 | 1420 KiB | 1484 KiB |
| `MasterCube-1024` | hash_65536 | 15308 | 1468 KiB | 1532 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `MasterCube-512` | KAT log (sha256 `88712a2ac590c107…`) | `kat/hash-17/MasterCube-512.log` |
| `MasterCube-512` | timing hash_1024 | `records/hash-17/MasterCube-512__hash_1024.json` |
| `MasterCube-512` | timing hash_128 | `records/hash-17/MasterCube-512__hash_128.json` |
| `MasterCube-512` | timing hash_16384 | `records/hash-17/MasterCube-512__hash_16384.json` |
| `MasterCube-512` | timing hash_32 | `records/hash-17/MasterCube-512__hash_32.json` |
| `MasterCube-512` | timing hash_4096 | `records/hash-17/MasterCube-512__hash_4096.json` |
| `MasterCube-512` | timing hash_512 | `records/hash-17/MasterCube-512__hash_512.json` |
| `MasterCube-512` | timing hash_65536 | `records/hash-17/MasterCube-512__hash_65536.json` |
| `MasterCube-512` | timing hash_8192 | `records/hash-17/MasterCube-512__hash_8192.json` |
| `MasterCube-768` | KAT log (sha256 `3ff090fadd703982…`) | `kat/hash-17/MasterCube-768.log` |
| `MasterCube-768` | timing hash_1024 | `records/hash-17/MasterCube-768__hash_1024.json` |
| `MasterCube-768` | timing hash_128 | `records/hash-17/MasterCube-768__hash_128.json` |
| `MasterCube-768` | timing hash_16384 | `records/hash-17/MasterCube-768__hash_16384.json` |
| `MasterCube-768` | timing hash_32 | `records/hash-17/MasterCube-768__hash_32.json` |
| `MasterCube-768` | timing hash_4096 | `records/hash-17/MasterCube-768__hash_4096.json` |
| `MasterCube-768` | timing hash_512 | `records/hash-17/MasterCube-768__hash_512.json` |
| `MasterCube-768` | timing hash_65536 | `records/hash-17/MasterCube-768__hash_65536.json` |
| `MasterCube-768` | timing hash_8192 | `records/hash-17/MasterCube-768__hash_8192.json` |
| `MasterCube-1024` | KAT log (sha256 `3c10275f56940ee2…`) | `kat/hash-17/MasterCube-1024.log` |
| `MasterCube-1024` | timing hash_1024 | `records/hash-17/MasterCube-1024__hash_1024.json` |
| `MasterCube-1024` | timing hash_128 | `records/hash-17/MasterCube-1024__hash_128.json` |
| `MasterCube-1024` | timing hash_16384 | `records/hash-17/MasterCube-1024__hash_16384.json` |
| `MasterCube-1024` | timing hash_32 | `records/hash-17/MasterCube-1024__hash_32.json` |
| `MasterCube-1024` | timing hash_4096 | `records/hash-17/MasterCube-1024__hash_4096.json` |
| `MasterCube-1024` | timing hash_512 | `records/hash-17/MasterCube-1024__hash_512.json` |
| `MasterCube-1024` | timing hash_65536 | `records/hash-17/MasterCube-1024__hash_65536.json` |
| `MasterCube-1024` | timing hash_8192 | `records/hash-17/MasterCube-1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

