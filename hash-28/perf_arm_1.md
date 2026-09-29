# hash-28 The XRH-1 Hash Function — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-28` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101535400742440960.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: The XRH-1 Hash Function
- Implementation versions measured: reference
- Parameter sets: `XRH-1-512`, `XRH-1-768`, `XRH-1-1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-28/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `XRH-1-512` | guide | PASS |
| `XRH-1-768` | guide | PASS |
| `XRH-1-1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `XRH-1-512` | 32 B | 1308 | 40.9 | 487 ns | 65.8 | 100000 (5 × 20000) |
| `XRH-1-512` | 128 B | 2588 | 20.2 | 963 ns | 133.0 | 100000 (5 × 20000) |
| `XRH-1-512` | 512 B | 7727 | 15.1 | 2.87 µs | 178.5 | 100000 (5 × 20000) |
| `XRH-1-512` | 1024 B | 15.4 k | 15.1 | 5.73 µs | 178.8 | 100000 (5 × 20000) |
| `XRH-1-512` | 4096 B | 64.5 k | 15.8 | 24 µs | 171.0 | 100000 (5 × 20000) |
| `XRH-1-512` | 8192 B | 120.3 k | 14.7 | 44.6 µs | 183.5 | 67230 (5 × 13446) |
| `XRH-1-512` | 16384 B | 239.2 k | 14.6 | 88.8 µs | 184.5 | 34960 (5 × 6992) |
| `XRH-1-512` | 65536 B | 953.0 k | 14.5 | 354 µs | 185.3 | 8920 (5 × 1784) |
| `XRH-1-768` | 32 B | 2555 | 79.9 | 950 ns | 33.7 | 100000 (5 × 20000) |
| `XRH-1-768` | 128 B | 5127 | 40.1 | 1.9 µs | 67.2 | 100000 (5 × 20000) |
| `XRH-1-768` | 512 B | 14.2 k | 27.7 | 5.27 µs | 97.2 | 100000 (5 × 20000) |
| `XRH-1-768` | 1024 B | 25.8 k | 25.2 | 9.58 µs | 106.9 | 100000 (5 × 20000) |
| `XRH-1-768` | 4096 B | 96.8 k | 23.6 | 35.9 µs | 114.0 | 81495 (5 × 16299) |
| `XRH-1-768` | 8192 B | 191.0 k | 23.3 | 70.9 µs | 115.6 | 43365 (5 × 8673) |
| `XRH-1-768` | 16384 B | 379.7 k | 23.2 | 141 µs | 116.3 | 22175 (5 × 4435) |
| `XRH-1-768` | 65536 B | 1.51 M | 23.1 | 561 µs | 116.7 | 5630 (5 × 1126) |
| `XRH-1-1024` | 32 B | 8857 | 276.8 | 3.29 µs | 9.7 | 100000 (5 × 20000) |
| `XRH-1-1024` | 128 B | 14.1 k | 109.8 | 5.22 µs | 24.5 | 100000 (5 × 20000) |
| `XRH-1-1024` | 512 B | 34.8 k | 68.1 | 12.9 µs | 39.6 | 100000 (5 × 20000) |
| `XRH-1-1024` | 1024 B | 62.1 k | 60.7 | 23.1 µs | 44.4 | 100000 (5 × 20000) |
| `XRH-1-1024` | 4096 B | 244.4 k | 59.7 | 90.7 µs | 45.2 | 36285 (5 × 7257) |
| `XRH-1-1024` | 8192 B | 450.7 k | 55.0 | 167 µs | 49.0 | 18715 (5 × 3743) |
| `XRH-1-1024` | 16384 B | 894.0 k | 54.6 | 332 µs | 49.4 | 9515 (5 × 1903) |
| `XRH-1-1024` | 65536 B | 3.56 M | 54.3 | 1.32 ms | 49.7 | 2390 (5 × 478) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `XRH-1-512` | hash_32 | 13156 | 1400 KiB | 1464 KiB |
| `XRH-1-512` | hash_128 | 13156 | 1400 KiB | 1464 KiB |
| `XRH-1-512` | hash_512 | 13156 | 1400 KiB | 1464 KiB |
| `XRH-1-512` | hash_1024 | 13156 | 1404 KiB | 1468 KiB |
| `XRH-1-512` | hash_4096 | 13156 | 1404 KiB | 1468 KiB |
| `XRH-1-512` | hash_8192 | 13156 | 1404 KiB | 1468 KiB |
| `XRH-1-512` | hash_16384 | 13156 | 1416 KiB | 1480 KiB |
| `XRH-1-512` | hash_65536 | 13156 | 3504 KiB | 3568 KiB |
| `XRH-1-768` | hash_32 | 13156 | 1400 KiB | 1464 KiB |
| `XRH-1-768` | hash_128 | 13156 | 1400 KiB | 1464 KiB |
| `XRH-1-768` | hash_512 | 13156 | 1400 KiB | 1464 KiB |
| `XRH-1-768` | hash_1024 | 13156 | 1404 KiB | 1468 KiB |
| `XRH-1-768` | hash_4096 | 13156 | 1404 KiB | 1468 KiB |
| `XRH-1-768` | hash_8192 | 13156 | 1408 KiB | 1472 KiB |
| `XRH-1-768` | hash_16384 | 13156 | 1416 KiB | 1480 KiB |
| `XRH-1-768` | hash_65536 | 13156 | 1464 KiB | 1528 KiB |
| `XRH-1-1024` | hash_32 | 13156 | 3436 KiB | 3500 KiB |
| `XRH-1-1024` | hash_128 | 13156 | 1400 KiB | 1464 KiB |
| `XRH-1-1024` | hash_512 | 13156 | 1400 KiB | 1464 KiB |
| `XRH-1-1024` | hash_1024 | 13156 | 1400 KiB | 1464 KiB |
| `XRH-1-1024` | hash_4096 | 13156 | 1404 KiB | 1468 KiB |
| `XRH-1-1024` | hash_8192 | 13156 | 1408 KiB | 1472 KiB |
| `XRH-1-1024` | hash_16384 | 13156 | 1416 KiB | 1480 KiB |
| `XRH-1-1024` | hash_65536 | 13156 | 1464 KiB | 1528 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `XRH-1-512` | KAT log (sha256 `c2d321d39c32dc1f…`) | `kat/hash-28/XRH-1-512.log` |
| `XRH-1-512` | timing hash_1024 | `records/hash-28/XRH-1-512__hash_1024.json` |
| `XRH-1-512` | timing hash_128 | `records/hash-28/XRH-1-512__hash_128.json` |
| `XRH-1-512` | timing hash_16384 | `records/hash-28/XRH-1-512__hash_16384.json` |
| `XRH-1-512` | timing hash_32 | `records/hash-28/XRH-1-512__hash_32.json` |
| `XRH-1-512` | timing hash_4096 | `records/hash-28/XRH-1-512__hash_4096.json` |
| `XRH-1-512` | timing hash_512 | `records/hash-28/XRH-1-512__hash_512.json` |
| `XRH-1-512` | timing hash_65536 | `records/hash-28/XRH-1-512__hash_65536.json` |
| `XRH-1-512` | timing hash_8192 | `records/hash-28/XRH-1-512__hash_8192.json` |
| `XRH-1-768` | KAT log (sha256 `5726f02d2d738612…`) | `kat/hash-28/XRH-1-768.log` |
| `XRH-1-768` | timing hash_1024 | `records/hash-28/XRH-1-768__hash_1024.json` |
| `XRH-1-768` | timing hash_128 | `records/hash-28/XRH-1-768__hash_128.json` |
| `XRH-1-768` | timing hash_16384 | `records/hash-28/XRH-1-768__hash_16384.json` |
| `XRH-1-768` | timing hash_32 | `records/hash-28/XRH-1-768__hash_32.json` |
| `XRH-1-768` | timing hash_4096 | `records/hash-28/XRH-1-768__hash_4096.json` |
| `XRH-1-768` | timing hash_512 | `records/hash-28/XRH-1-768__hash_512.json` |
| `XRH-1-768` | timing hash_65536 | `records/hash-28/XRH-1-768__hash_65536.json` |
| `XRH-1-768` | timing hash_8192 | `records/hash-28/XRH-1-768__hash_8192.json` |
| `XRH-1-1024` | KAT log (sha256 `88fcb6eeab5eb833…`) | `kat/hash-28/XRH-1-1024.log` |
| `XRH-1-1024` | timing hash_1024 | `records/hash-28/XRH-1-1024__hash_1024.json` |
| `XRH-1-1024` | timing hash_128 | `records/hash-28/XRH-1-1024__hash_128.json` |
| `XRH-1-1024` | timing hash_16384 | `records/hash-28/XRH-1-1024__hash_16384.json` |
| `XRH-1-1024` | timing hash_32 | `records/hash-28/XRH-1-1024__hash_32.json` |
| `XRH-1-1024` | timing hash_4096 | `records/hash-28/XRH-1-1024__hash_4096.json` |
| `XRH-1-1024` | timing hash_512 | `records/hash-28/XRH-1-1024__hash_512.json` |
| `XRH-1-1024` | timing hash_65536 | `records/hash-28/XRH-1-1024__hash_65536.json` |
| `XRH-1-1024` | timing hash_8192 | `records/hash-28/XRH-1-1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

