# hash-21 Neulaser — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-21` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101536800629149696.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: Neulaser
- Implementation versions measured: reference
- Parameter sets: `Neulaser-512`, `Neulaser-768`, `Neulaser-1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-21/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Neulaser-512` | guide | PASS |
| `Neulaser-768` | guide | PASS |
| `Neulaser-1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `Neulaser-512` | 32 B | 8781 | 274.4 | 3.26 µs | 9.8 | 100000 (5 × 20000) |
| `Neulaser-512` | 128 B | 16.2 k | 126.8 | 6.03 µs | 21.2 | 100000 (5 × 20000) |
| `Neulaser-512` | 512 B | 38.7 k | 75.5 | 14.3 µs | 35.7 | 100000 (5 × 20000) |
| `Neulaser-512` | 1024 B | 68.5 k | 66.9 | 25.4 µs | 40.3 | 100000 (5 × 20000) |
| `Neulaser-512` | 4096 B | 261.9 k | 64.0 | 97.2 µs | 42.1 | 29945 (5 × 5989) |
| `Neulaser-512` | 8192 B | 557.3 k | 68.0 | 207 µs | 39.6 | 16445 (5 × 3289) |
| `Neulaser-512` | 16384 B | 1.02 M | 62.5 | 380 µs | 43.2 | 8440 (5 × 1688) |
| `Neulaser-512` | 65536 B | 4.09 M | 62.4 | 1.52 ms | 43.2 | 1905 (5 × 381) |
| `Neulaser-768` | 32 B | 11.2 k | 348.8 | 4.14 µs | 7.7 | 100000 (5 × 20000) |
| `Neulaser-768` | 128 B | 11.2 k | 87.3 | 4.15 µs | 30.9 | 100000 (5 × 20000) |
| `Neulaser-768` | 512 B | 39.6 k | 77.3 | 14.7 µs | 34.9 | 100000 (5 × 20000) |
| `Neulaser-768` | 1024 B | 71.7 k | 70.0 | 26.6 µs | 38.5 | 100000 (5 × 20000) |
| `Neulaser-768` | 4096 B | 266.3 k | 65.0 | 98.8 µs | 41.5 | 31500 (5 × 6300) |
| `Neulaser-768` | 8192 B | 511.7 k | 62.5 | 190 µs | 43.1 | 16495 (5 × 3299) |
| `Neulaser-768` | 16384 B | 1.02 M | 62.4 | 379 µs | 43.2 | 8310 (5 × 1662) |
| `Neulaser-768` | 65536 B | 4.23 M | 64.5 | 1.57 ms | 41.8 | 2075 (5 × 415) |
| `Neulaser-1024` | 32 B | 14.3 k | 447.6 | 5.32 µs | 6.0 | 100000 (5 × 20000) |
| `Neulaser-1024` | 128 B | 14.1 k | 110.3 | 5.24 µs | 24.4 | 100000 (5 × 20000) |
| `Neulaser-1024` | 512 B | 37.2 k | 72.7 | 13.8 µs | 37.0 | 100000 (5 × 20000) |
| `Neulaser-1024` | 1024 B | 75.4 k | 73.6 | 28 µs | 36.6 | 100000 (5 × 20000) |
| `Neulaser-1024` | 4096 B | 273.8 k | 66.9 | 102 µs | 40.3 | 30950 (5 × 6190) |
| `Neulaser-1024` | 8192 B | 524.3 k | 64.0 | 195 µs | 42.1 | 16165 (5 × 3233) |
| `Neulaser-1024` | 16384 B | 1.05 M | 63.8 | 388 µs | 42.2 | 7985 (5 × 1597) |
| `Neulaser-1024` | 65536 B | 4.14 M | 63.2 | 1.54 ms | 42.7 | 2045 (5 × 409) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Neulaser-512` | hash_32 | 12692 | 1400 KiB | 1464 KiB |
| `Neulaser-512` | hash_128 | 12692 | 1400 KiB | 1464 KiB |
| `Neulaser-512` | hash_512 | 12692 | 1400 KiB | 1464 KiB |
| `Neulaser-512` | hash_1024 | 12692 | 1404 KiB | 1468 KiB |
| `Neulaser-512` | hash_4096 | 12692 | 1404 KiB | 1468 KiB |
| `Neulaser-512` | hash_8192 | 12692 | 1408 KiB | 1472 KiB |
| `Neulaser-512` | hash_16384 | 12692 | 3436 KiB | 3500 KiB |
| `Neulaser-512` | hash_65536 | 12692 | 1464 KiB | 1528 KiB |
| `Neulaser-768` | hash_32 | 12724 | 1400 KiB | 1464 KiB |
| `Neulaser-768` | hash_128 | 12724 | 1400 KiB | 1464 KiB |
| `Neulaser-768` | hash_512 | 12724 | 1400 KiB | 1464 KiB |
| `Neulaser-768` | hash_1024 | 12724 | 1404 KiB | 1468 KiB |
| `Neulaser-768` | hash_4096 | 12724 | 1404 KiB | 1468 KiB |
| `Neulaser-768` | hash_8192 | 12724 | 1408 KiB | 1472 KiB |
| `Neulaser-768` | hash_16384 | 12724 | 1416 KiB | 1480 KiB |
| `Neulaser-768` | hash_65536 | 12724 | 1464 KiB | 1528 KiB |
| `Neulaser-1024` | hash_32 | 12820 | 1400 KiB | 1464 KiB |
| `Neulaser-1024` | hash_128 | 12820 | 1400 KiB | 1464 KiB |
| `Neulaser-1024` | hash_512 | 12820 | 1400 KiB | 1464 KiB |
| `Neulaser-1024` | hash_1024 | 12820 | 1404 KiB | 1468 KiB |
| `Neulaser-1024` | hash_4096 | 12820 | 1404 KiB | 1468 KiB |
| `Neulaser-1024` | hash_8192 | 12820 | 3444 KiB | 3508 KiB |
| `Neulaser-1024` | hash_16384 | 12820 | 1416 KiB | 1480 KiB |
| `Neulaser-1024` | hash_65536 | 12820 | 1464 KiB | 1528 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Neulaser-512` | KAT log (sha256 `981a5351552eeb9c…`) | `kat/hash-21/Neulaser-512.log` |
| `Neulaser-512` | timing hash_1024 | `records/hash-21/Neulaser-512__hash_1024.json` |
| `Neulaser-512` | timing hash_128 | `records/hash-21/Neulaser-512__hash_128.json` |
| `Neulaser-512` | timing hash_16384 | `records/hash-21/Neulaser-512__hash_16384.json` |
| `Neulaser-512` | timing hash_32 | `records/hash-21/Neulaser-512__hash_32.json` |
| `Neulaser-512` | timing hash_4096 | `records/hash-21/Neulaser-512__hash_4096.json` |
| `Neulaser-512` | timing hash_512 | `records/hash-21/Neulaser-512__hash_512.json` |
| `Neulaser-512` | timing hash_65536 | `records/hash-21/Neulaser-512__hash_65536.json` |
| `Neulaser-512` | timing hash_8192 | `records/hash-21/Neulaser-512__hash_8192.json` |
| `Neulaser-768` | KAT log (sha256 `09892f2364b5b3d7…`) | `kat/hash-21/Neulaser-768.log` |
| `Neulaser-768` | timing hash_1024 | `records/hash-21/Neulaser-768__hash_1024.json` |
| `Neulaser-768` | timing hash_128 | `records/hash-21/Neulaser-768__hash_128.json` |
| `Neulaser-768` | timing hash_16384 | `records/hash-21/Neulaser-768__hash_16384.json` |
| `Neulaser-768` | timing hash_32 | `records/hash-21/Neulaser-768__hash_32.json` |
| `Neulaser-768` | timing hash_4096 | `records/hash-21/Neulaser-768__hash_4096.json` |
| `Neulaser-768` | timing hash_512 | `records/hash-21/Neulaser-768__hash_512.json` |
| `Neulaser-768` | timing hash_65536 | `records/hash-21/Neulaser-768__hash_65536.json` |
| `Neulaser-768` | timing hash_8192 | `records/hash-21/Neulaser-768__hash_8192.json` |
| `Neulaser-1024` | KAT log (sha256 `4df66d57a8e87e06…`) | `kat/hash-21/Neulaser-1024.log` |
| `Neulaser-1024` | timing hash_1024 | `records/hash-21/Neulaser-1024__hash_1024.json` |
| `Neulaser-1024` | timing hash_128 | `records/hash-21/Neulaser-1024__hash_128.json` |
| `Neulaser-1024` | timing hash_16384 | `records/hash-21/Neulaser-1024__hash_16384.json` |
| `Neulaser-1024` | timing hash_32 | `records/hash-21/Neulaser-1024__hash_32.json` |
| `Neulaser-1024` | timing hash_4096 | `records/hash-21/Neulaser-1024__hash_4096.json` |
| `Neulaser-1024` | timing hash_512 | `records/hash-21/Neulaser-1024__hash_512.json` |
| `Neulaser-1024` | timing hash_65536 | `records/hash-21/Neulaser-1024__hash_65536.json` |
| `Neulaser-1024` | timing hash_8192 | `records/hash-21/Neulaser-1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

