# hash-29 The XRH-2 Hash Function — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-29` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101534893223268352.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: The XRH-2 Hash Function
- Implementation versions measured: reference
- Parameter sets: `XRH-2-512`, `XRH-2-768`, `XRH-2-1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-29/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `XRH-2-512` | guide | PASS |
| `XRH-2-768` | guide | PASS |
| `XRH-2-1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `XRH-2-512` | 32 B | 1310 | 40.9 | 487 ns | 65.6 | 100000 (5 × 20000) |
| `XRH-2-512` | 128 B | 2597 | 20.3 | 965 ns | 132.6 | 100000 (5 × 20000) |
| `XRH-2-512` | 512 B | 7719 | 15.1 | 2.87 µs | 178.6 | 100000 (5 × 20000) |
| `XRH-2-512` | 1024 B | 15.4 k | 15.0 | 5.71 µs | 179.3 | 100000 (5 × 20000) |
| `XRH-2-512` | 4096 B | 60.1 k | 14.7 | 22.3 µs | 183.7 | 100000 (5 × 20000) |
| `XRH-2-512` | 8192 B | 120.1 k | 14.7 | 44.6 µs | 183.8 | 68970 (5 × 13794) |
| `XRH-2-512` | 16384 B | 255.4 k | 15.6 | 94.8 µs | 172.9 | 23845 (5 × 4769) |
| `XRH-2-512` | 65536 B | 951.7 k | 14.5 | 353 µs | 185.6 | 8705 (5 × 1741) |
| `XRH-2-768` | 32 B | 2556 | 79.9 | 950 ns | 33.7 | 100000 (5 × 20000) |
| `XRH-2-768` | 128 B | 5110 | 39.9 | 1.9 µs | 67.4 | 100000 (5 × 20000) |
| `XRH-2-768` | 512 B | 14.1 k | 27.5 | 5.23 µs | 97.8 | 100000 (5 × 20000) |
| `XRH-2-768` | 1024 B | 25.7 k | 25.1 | 9.55 µs | 107.2 | 100000 (5 × 20000) |
| `XRH-2-768` | 4096 B | 96.1 k | 23.5 | 35.7 µs | 114.9 | 84810 (5 × 16962) |
| `XRH-2-768` | 8192 B | 189.6 k | 23.1 | 70.4 µs | 116.4 | 43920 (5 × 8784) |
| `XRH-2-768` | 16384 B | 376.6 k | 23.0 | 140 µs | 117.2 | 22330 (5 × 4466) |
| `XRH-2-768` | 65536 B | 1.60 M | 24.5 | 595 µs | 110.1 | 5675 (5 × 1135) |
| `XRH-2-1024` | 32 B | 8842 | 276.3 | 3.28 µs | 9.7 | 100000 (5 × 20000) |
| `XRH-2-1024` | 128 B | 13.9 k | 108.9 | 5.17 µs | 24.7 | 100000 (5 × 20000) |
| `XRH-2-1024` | 512 B | 34.4 k | 67.2 | 12.8 µs | 40.1 | 100000 (5 × 20000) |
| `XRH-2-1024` | 1024 B | 61.2 k | 59.8 | 22.7 µs | 45.1 | 100000 (5 × 20000) |
| `XRH-2-1024` | 4096 B | 224.8 k | 54.9 | 83.4 µs | 49.1 | 37300 (5 × 7460) |
| `XRH-2-1024` | 8192 B | 443.3 k | 54.1 | 165 µs | 49.8 | 18630 (5 × 3726) |
| `XRH-2-1024` | 16384 B | 879.1 k | 53.7 | 326 µs | 50.2 | 9675 (5 × 1935) |
| `XRH-2-1024` | 65536 B | 3.50 M | 53.4 | 1.3 ms | 50.5 | 2290 (5 × 458) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `XRH-2-512` | hash_32 | 13156 | 1400 KiB | 1464 KiB |
| `XRH-2-512` | hash_128 | 13156 | 1400 KiB | 1464 KiB |
| `XRH-2-512` | hash_512 | 13156 | 1400 KiB | 1464 KiB |
| `XRH-2-512` | hash_1024 | 13156 | 1404 KiB | 1468 KiB |
| `XRH-2-512` | hash_4096 | 13156 | 3428 KiB | 3492 KiB |
| `XRH-2-512` | hash_8192 | 13156 | 1408 KiB | 1472 KiB |
| `XRH-2-512` | hash_16384 | 13156 | 1416 KiB | 1480 KiB |
| `XRH-2-512` | hash_65536 | 13156 | 1464 KiB | 1528 KiB |
| `XRH-2-768` | hash_32 | 13156 | 1400 KiB | 1464 KiB |
| `XRH-2-768` | hash_128 | 13156 | 1400 KiB | 1464 KiB |
| `XRH-2-768` | hash_512 | 13156 | 1400 KiB | 1464 KiB |
| `XRH-2-768` | hash_1024 | 13156 | 1404 KiB | 1468 KiB |
| `XRH-2-768` | hash_4096 | 13156 | 1404 KiB | 1468 KiB |
| `XRH-2-768` | hash_8192 | 13156 | 1408 KiB | 1472 KiB |
| `XRH-2-768` | hash_16384 | 13156 | 1416 KiB | 1480 KiB |
| `XRH-2-768` | hash_65536 | 13156 | 1464 KiB | 1528 KiB |
| `XRH-2-1024` | hash_32 | 13124 | 1400 KiB | 1464 KiB |
| `XRH-2-1024` | hash_128 | 13124 | 1400 KiB | 1464 KiB |
| `XRH-2-1024` | hash_512 | 13124 | 1396 KiB | 1460 KiB |
| `XRH-2-1024` | hash_1024 | 13124 | 1404 KiB | 1468 KiB |
| `XRH-2-1024` | hash_4096 | 13124 | 1400 KiB | 1464 KiB |
| `XRH-2-1024` | hash_8192 | 13124 | 1408 KiB | 1472 KiB |
| `XRH-2-1024` | hash_16384 | 13124 | 1416 KiB | 1480 KiB |
| `XRH-2-1024` | hash_65536 | 13124 | 1464 KiB | 1528 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `XRH-2-512` | KAT log (sha256 `84ab941944d54dc5…`) | `kat/hash-29/XRH-2-512.log` |
| `XRH-2-512` | timing hash_1024 | `records/hash-29/XRH-2-512__hash_1024.json` |
| `XRH-2-512` | timing hash_128 | `records/hash-29/XRH-2-512__hash_128.json` |
| `XRH-2-512` | timing hash_16384 | `records/hash-29/XRH-2-512__hash_16384.json` |
| `XRH-2-512` | timing hash_32 | `records/hash-29/XRH-2-512__hash_32.json` |
| `XRH-2-512` | timing hash_4096 | `records/hash-29/XRH-2-512__hash_4096.json` |
| `XRH-2-512` | timing hash_512 | `records/hash-29/XRH-2-512__hash_512.json` |
| `XRH-2-512` | timing hash_65536 | `records/hash-29/XRH-2-512__hash_65536.json` |
| `XRH-2-512` | timing hash_8192 | `records/hash-29/XRH-2-512__hash_8192.json` |
| `XRH-2-768` | KAT log (sha256 `91f4327d0b583e92…`) | `kat/hash-29/XRH-2-768.log` |
| `XRH-2-768` | timing hash_1024 | `records/hash-29/XRH-2-768__hash_1024.json` |
| `XRH-2-768` | timing hash_128 | `records/hash-29/XRH-2-768__hash_128.json` |
| `XRH-2-768` | timing hash_16384 | `records/hash-29/XRH-2-768__hash_16384.json` |
| `XRH-2-768` | timing hash_32 | `records/hash-29/XRH-2-768__hash_32.json` |
| `XRH-2-768` | timing hash_4096 | `records/hash-29/XRH-2-768__hash_4096.json` |
| `XRH-2-768` | timing hash_512 | `records/hash-29/XRH-2-768__hash_512.json` |
| `XRH-2-768` | timing hash_65536 | `records/hash-29/XRH-2-768__hash_65536.json` |
| `XRH-2-768` | timing hash_8192 | `records/hash-29/XRH-2-768__hash_8192.json` |
| `XRH-2-1024` | KAT log (sha256 `f49e8677b0d8cb7f…`) | `kat/hash-29/XRH-2-1024.log` |
| `XRH-2-1024` | timing hash_1024 | `records/hash-29/XRH-2-1024__hash_1024.json` |
| `XRH-2-1024` | timing hash_128 | `records/hash-29/XRH-2-1024__hash_128.json` |
| `XRH-2-1024` | timing hash_16384 | `records/hash-29/XRH-2-1024__hash_16384.json` |
| `XRH-2-1024` | timing hash_32 | `records/hash-29/XRH-2-1024__hash_32.json` |
| `XRH-2-1024` | timing hash_4096 | `records/hash-29/XRH-2-1024__hash_4096.json` |
| `XRH-2-1024` | timing hash_512 | `records/hash-29/XRH-2-1024__hash_512.json` |
| `XRH-2-1024` | timing hash_65536 | `records/hash-29/XRH-2-1024__hash_65536.json` |
| `XRH-2-1024` | timing hash_8192 | `records/hash-29/XRH-2-1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

