# hash-13 JuziHash — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-13` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101538489901862912.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: JuziHash
- Implementation versions measured: reference
- Parameter sets: `JuziHash-512`, `JuziHash-1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-13/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `JuziHash-512` | guide | PASS |
| `JuziHash-1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `JuziHash-512` | 32 B | 133.3 k | 4165.2 | 49.4 µs | 0.6 | 54485 (5 × 10897) |
| `JuziHash-512` | 128 B | 132.8 k | 1037.8 | 49.3 µs | 2.6 | 60915 (5 × 12183) |
| `JuziHash-512` | 512 B | 267.1 k | 521.7 | 99.1 µs | 5.2 | 30250 (5 × 6050) |
| `JuziHash-512` | 1024 B | 445.3 k | 434.9 | 165 µs | 6.2 | 18895 (5 × 3779) |
| `JuziHash-512` | 4096 B | 1.50 M | 367.4 | 558 µs | 7.3 | 5535 (5 × 1107) |
| `JuziHash-512` | 8192 B | 2.92 M | 356.4 | 1.08 ms | 7.6 | 2810 (5 × 562) |
| `JuziHash-512` | 16384 B | 5.75 M | 351.2 | 2.13 ms | 7.7 | 1470 (5 × 294) |
| `JuziHash-512` | 65536 B | 22.74 M | 347.0 | 8.44 ms | 7.8 | 375 (5 × 75) |
| `JuziHash-1024` | 32 B | 132.8 k | 4149.2 | 49.3 µs | 0.6 | 59045 (5 × 11809) |
| `JuziHash-1024` | 128 B | 132.8 k | 1037.2 | 49.3 µs | 2.6 | 61150 (5 × 12230) |
| `JuziHash-1024` | 512 B | 265.4 k | 518.4 | 98.5 µs | 5.2 | 30790 (5 × 6158) |
| `JuziHash-1024` | 1024 B | 443.9 k | 433.5 | 165 µs | 6.2 | 18870 (5 × 3774) |
| `JuziHash-1024` | 4096 B | 1.51 M | 368.6 | 560 µs | 7.3 | 5530 (5 × 1106) |
| `JuziHash-1024` | 8192 B | 2.92 M | 356.7 | 1.08 ms | 7.6 | 2895 (5 × 579) |
| `JuziHash-1024` | 16384 B | 5.75 M | 351.2 | 2.13 ms | 7.7 | 1385 (5 × 277) |
| `JuziHash-1024` | 65536 B | 22.77 M | 347.4 | 8.45 ms | 7.8 | 370 (5 × 74) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `JuziHash-512` | hash_32 | 11444 | 1400 KiB | 1464 KiB |
| `JuziHash-512` | hash_128 | 11444 | 1400 KiB | 1464 KiB |
| `JuziHash-512` | hash_512 | 11444 | 1400 KiB | 1464 KiB |
| `JuziHash-512` | hash_1024 | 11444 | 1404 KiB | 1468 KiB |
| `JuziHash-512` | hash_4096 | 11444 | 1404 KiB | 1468 KiB |
| `JuziHash-512` | hash_8192 | 11444 | 1404 KiB | 1468 KiB |
| `JuziHash-512` | hash_16384 | 11444 | 1416 KiB | 1480 KiB |
| `JuziHash-512` | hash_65536 | 11444 | 1464 KiB | 1528 KiB |
| `JuziHash-1024` | hash_32 | 11684 | 1400 KiB | 1464 KiB |
| `JuziHash-1024` | hash_128 | 11684 | 1400 KiB | 1464 KiB |
| `JuziHash-1024` | hash_512 | 11684 | 1400 KiB | 1464 KiB |
| `JuziHash-1024` | hash_1024 | 11684 | 1404 KiB | 1468 KiB |
| `JuziHash-1024` | hash_4096 | 11684 | 1404 KiB | 1468 KiB |
| `JuziHash-1024` | hash_8192 | 11684 | 1408 KiB | 1472 KiB |
| `JuziHash-1024` | hash_16384 | 11684 | 1416 KiB | 1480 KiB |
| `JuziHash-1024` | hash_65536 | 11684 | 1464 KiB | 1528 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `JuziHash-512` | KAT log (sha256 `adbcde90ac764a37…`) | `kat/hash-13/JuziHash-512.log` |
| `JuziHash-512` | timing hash_1024 | `records/hash-13/JuziHash-512__hash_1024.json` |
| `JuziHash-512` | timing hash_128 | `records/hash-13/JuziHash-512__hash_128.json` |
| `JuziHash-512` | timing hash_16384 | `records/hash-13/JuziHash-512__hash_16384.json` |
| `JuziHash-512` | timing hash_32 | `records/hash-13/JuziHash-512__hash_32.json` |
| `JuziHash-512` | timing hash_4096 | `records/hash-13/JuziHash-512__hash_4096.json` |
| `JuziHash-512` | timing hash_512 | `records/hash-13/JuziHash-512__hash_512.json` |
| `JuziHash-512` | timing hash_65536 | `records/hash-13/JuziHash-512__hash_65536.json` |
| `JuziHash-512` | timing hash_8192 | `records/hash-13/JuziHash-512__hash_8192.json` |
| `JuziHash-1024` | KAT log (sha256 `3c5899608dbd0b4b…`) | `kat/hash-13/JuziHash-1024.log` |
| `JuziHash-1024` | timing hash_1024 | `records/hash-13/JuziHash-1024__hash_1024.json` |
| `JuziHash-1024` | timing hash_128 | `records/hash-13/JuziHash-1024__hash_128.json` |
| `JuziHash-1024` | timing hash_16384 | `records/hash-13/JuziHash-1024__hash_16384.json` |
| `JuziHash-1024` | timing hash_32 | `records/hash-13/JuziHash-1024__hash_32.json` |
| `JuziHash-1024` | timing hash_4096 | `records/hash-13/JuziHash-1024__hash_4096.json` |
| `JuziHash-1024` | timing hash_512 | `records/hash-13/JuziHash-1024__hash_512.json` |
| `JuziHash-1024` | timing hash_65536 | `records/hash-13/JuziHash-1024__hash_65536.json` |
| `JuziHash-1024` | timing hash_8192 | `records/hash-13/JuziHash-1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

