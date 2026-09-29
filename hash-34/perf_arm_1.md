# hash-34 WChain Hash Function — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-34` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101533578757754880.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: WChain Hash Function
- Implementation versions measured: reference
- Parameter sets: `WChain-V1-512`, `WChain-V2-1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-34/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `WChain-V1-512` | guide | PASS |
| `WChain-V2-1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `WChain-V1-512` | 32 B | 1379 | 43.1 | 513 ns | 62.3 | 100000 (5 × 20000) |
| `WChain-V1-512` | 128 B | 1367 | 10.7 | 509 ns | 251.6 | 100000 (5 × 20000) |
| `WChain-V1-512` | 512 B | 3324 | 6.5 | 1.24 µs | 414.4 | 100000 (5 × 20000) |
| `WChain-V1-512` | 1024 B | 5967 | 5.8 | 2.22 µs | 462.1 | 100000 (5 × 20000) |
| `WChain-V1-512` | 4096 B | 19.8 k | 4.8 | 7.34 µs | 558.2 | 100000 (5 × 20000) |
| `WChain-V1-512` | 8192 B | 38.2 k | 4.7 | 14.2 µs | 577.9 | 100000 (5 × 20000) |
| `WChain-V1-512` | 16384 B | 75.6 k | 4.6 | 28.1 µs | 583.7 | 100000 (5 × 20000) |
| `WChain-V1-512` | 65536 B | 300.5 k | 4.6 | 112 µs | 587.6 | 28025 (5 × 5605) |
| `WChain-V2-1024` | 32 B | 5991 | 187.2 | 2.23 µs | 14.4 | 100000 (5 × 20000) |
| `WChain-V2-1024` | 128 B | 6062 | 47.4 | 2.25 µs | 56.9 | 100000 (5 × 20000) |
| `WChain-V2-1024` | 512 B | 9011 | 17.6 | 3.35 µs | 153.0 | 100000 (5 × 20000) |
| `WChain-V2-1024` | 1024 B | 14.9 k | 14.6 | 5.53 µs | 185.1 | 100000 (5 × 20000) |
| `WChain-V2-1024` | 4096 B | 47.3 k | 11.5 | 17.5 µs | 233.5 | 100000 (5 × 20000) |
| `WChain-V2-1024` | 8192 B | 88.5 k | 10.8 | 32.8 µs | 249.6 | 91955 (5 × 18391) |
| `WChain-V2-1024` | 16384 B | 181.0 k | 11.0 | 67.2 µs | 243.9 | 48100 (5 × 9620) |
| `WChain-V2-1024` | 65536 B | 677.0 k | 10.3 | 251 µs | 260.9 | 12550 (5 × 2510) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `WChain-V1-512` | hash_32 | 22368 | 1412 KiB | 1476 KiB |
| `WChain-V1-512` | hash_128 | 22368 | 1412 KiB | 1476 KiB |
| `WChain-V1-512` | hash_512 | 22368 | 1412 KiB | 1476 KiB |
| `WChain-V1-512` | hash_1024 | 22368 | 1416 KiB | 1480 KiB |
| `WChain-V1-512` | hash_4096 | 22368 | 1416 KiB | 1480 KiB |
| `WChain-V1-512` | hash_8192 | 22368 | 1420 KiB | 1484 KiB |
| `WChain-V1-512` | hash_16384 | 22368 | 1428 KiB | 1492 KiB |
| `WChain-V1-512` | hash_65536 | 22368 | 1476 KiB | 1540 KiB |
| `WChain-V2-1024` | hash_32 | 22368 | 1412 KiB | 1476 KiB |
| `WChain-V2-1024` | hash_128 | 22368 | 1412 KiB | 1476 KiB |
| `WChain-V2-1024` | hash_512 | 22368 | 1408 KiB | 1472 KiB |
| `WChain-V2-1024` | hash_1024 | 22368 | 1416 KiB | 1480 KiB |
| `WChain-V2-1024` | hash_4096 | 22368 | 1416 KiB | 1480 KiB |
| `WChain-V2-1024` | hash_8192 | 22368 | 1420 KiB | 1484 KiB |
| `WChain-V2-1024` | hash_16384 | 22368 | 1428 KiB | 1492 KiB |
| `WChain-V2-1024` | hash_65536 | 22368 | 1476 KiB | 1540 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `WChain-V1-512` | KAT log (sha256 `829596117a41745c…`) | `kat/hash-34/WChain-V1-512.log` |
| `WChain-V1-512` | timing hash_1024 | `records/hash-34/WChain-V1-512__hash_1024.json` |
| `WChain-V1-512` | timing hash_128 | `records/hash-34/WChain-V1-512__hash_128.json` |
| `WChain-V1-512` | timing hash_16384 | `records/hash-34/WChain-V1-512__hash_16384.json` |
| `WChain-V1-512` | timing hash_32 | `records/hash-34/WChain-V1-512__hash_32.json` |
| `WChain-V1-512` | timing hash_4096 | `records/hash-34/WChain-V1-512__hash_4096.json` |
| `WChain-V1-512` | timing hash_512 | `records/hash-34/WChain-V1-512__hash_512.json` |
| `WChain-V1-512` | timing hash_65536 | `records/hash-34/WChain-V1-512__hash_65536.json` |
| `WChain-V1-512` | timing hash_8192 | `records/hash-34/WChain-V1-512__hash_8192.json` |
| `WChain-V2-1024` | KAT log (sha256 `9f8ac22a5d0ed869…`) | `kat/hash-34/WChain-V2-1024.log` |
| `WChain-V2-1024` | timing hash_1024 | `records/hash-34/WChain-V2-1024__hash_1024.json` |
| `WChain-V2-1024` | timing hash_128 | `records/hash-34/WChain-V2-1024__hash_128.json` |
| `WChain-V2-1024` | timing hash_16384 | `records/hash-34/WChain-V2-1024__hash_16384.json` |
| `WChain-V2-1024` | timing hash_32 | `records/hash-34/WChain-V2-1024__hash_32.json` |
| `WChain-V2-1024` | timing hash_4096 | `records/hash-34/WChain-V2-1024__hash_4096.json` |
| `WChain-V2-1024` | timing hash_512 | `records/hash-34/WChain-V2-1024__hash_512.json` |
| `WChain-V2-1024` | timing hash_65536 | `records/hash-34/WChain-V2-1024__hash_65536.json` |
| `WChain-V2-1024` | timing hash_8192 | `records/hash-34/WChain-V2-1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

