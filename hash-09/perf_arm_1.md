# hash-09 Eijen Hash Function — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-09` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101539361927024640.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: Eijen Hash Function
- Implementation versions measured: reference
- Parameter sets: `Eijen-256`, `Eijen-384`, `Eijen-512`, `Eijen-768`, `Eijen-1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-09/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Eijen-256` | guide | PASS |
| `Eijen-384` | guide | PASS |
| `Eijen-512` | guide | PASS |
| `Eijen-768` | guide | PASS |
| `Eijen-1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `Eijen-256` | 32 B | 1419 | 44.3 | 529 ns | 60.5 | 100000 (5 × 20000) |
| `Eijen-256` | 128 B | 1432 | 11.2 | 533 ns | 240.0 | 100000 (5 × 20000) |
| `Eijen-256` | 512 B | 4155 | 8.1 | 1.54 µs | 331.6 | 100000 (5 × 20000) |
| `Eijen-256` | 1024 B | 6864 | 6.7 | 2.55 µs | 401.6 | 100000 (5 × 20000) |
| `Eijen-256` | 4096 B | 25.8 k | 6.3 | 9.59 µs | 427.2 | 100000 (5 × 20000) |
| `Eijen-256` | 8192 B | 51.6 k | 6.3 | 19.2 µs | 427.6 | 100000 (5 × 20000) |
| `Eijen-256` | 16384 B | 103.1 k | 6.3 | 38.3 µs | 428.2 | 79605 (5 × 15921) |
| `Eijen-256` | 65536 B | 412.0 k | 6.3 | 153 µs | 428.6 | 20490 (5 × 4098) |
| `Eijen-384` | 32 B | 1422 | 44.5 | 529 ns | 60.5 | 100000 (5 × 20000) |
| `Eijen-384` | 128 B | 1439 | 11.2 | 535 ns | 239.3 | 100000 (5 × 20000) |
| `Eijen-384` | 512 B | 4167 | 8.1 | 1.55 µs | 330.8 | 100000 (5 × 20000) |
| `Eijen-384` | 1024 B | 8221 | 8.0 | 3.05 µs | 335.5 | 100000 (5 × 20000) |
| `Eijen-384` | 4096 B | 28.6 k | 7.0 | 10.6 µs | 386.6 | 100000 (5 × 20000) |
| `Eijen-384` | 8192 B | 55.7 k | 6.8 | 20.7 µs | 396.6 | 100000 (5 × 20000) |
| `Eijen-384` | 16384 B | 111.3 k | 6.8 | 41.3 µs | 396.7 | 72840 (5 × 14568) |
| `Eijen-384` | 65536 B | 444.7 k | 6.8 | 165 µs | 397.1 | 19005 (5 × 3801) |
| `Eijen-512` | 32 B | 1424 | 44.5 | 530 ns | 60.4 | 100000 (5 × 20000) |
| `Eijen-512` | 128 B | 1420 | 11.1 | 529 ns | 242.1 | 100000 (5 × 20000) |
| `Eijen-512` | 512 B | 4162 | 8.1 | 1.55 µs | 331.0 | 100000 (5 × 20000) |
| `Eijen-512` | 1024 B | 8234 | 8.0 | 3.06 µs | 335.0 | 100000 (5 × 20000) |
| `Eijen-512` | 4096 B | 31.3 k | 7.6 | 11.6 µs | 352.7 | 100000 (5 × 20000) |
| `Eijen-512` | 8192 B | 61.1 k | 7.5 | 22.7 µs | 361.1 | 100000 (5 × 20000) |
| `Eijen-512` | 16384 B | 122.2 k | 7.5 | 45.3 µs | 361.3 | 67045 (5 × 13409) |
| `Eijen-512` | 65536 B | 484.4 k | 7.4 | 180 µs | 364.6 | 17490 (5 × 3498) |
| `Eijen-768` | 32 B | 1431 | 44.7 | 533 ns | 60.0 | 100000 (5 × 20000) |
| `Eijen-768` | 128 B | 1427 | 11.1 | 531 ns | 241.1 | 100000 (5 × 20000) |
| `Eijen-768` | 512 B | 5538 | 10.8 | 2.06 µs | 248.8 | 100000 (5 × 20000) |
| `Eijen-768` | 1024 B | 9623 | 9.4 | 3.57 µs | 286.5 | 100000 (5 × 20000) |
| `Eijen-768` | 4096 B | 36.8 k | 9.0 | 13.7 µs | 299.6 | 100000 (5 × 20000) |
| `Eijen-768` | 8192 B | 73.6 k | 9.0 | 27.4 µs | 299.5 | 100000 (5 × 20000) |
| `Eijen-768` | 16384 B | 147.0 k | 9.0 | 54.6 µs | 300.3 | 56210 (5 × 11242) |
| `Eijen-768` | 65536 B | 587.8 k | 9.0 | 218 µs | 300.4 | 14405 (5 × 2881) |
| `Eijen-1024` | 32 B | 1433 | 44.8 | 533 ns | 60.0 | 100000 (5 × 20000) |
| `Eijen-1024` | 128 B | 2815 | 22.0 | 1.05 µs | 122.3 | 100000 (5 × 20000) |
| `Eijen-1024` | 512 B | 6907 | 13.5 | 2.56 µs | 199.7 | 100000 (5 × 20000) |
| `Eijen-1024` | 1024 B | 12.4 k | 12.1 | 4.59 µs | 223.2 | 100000 (5 × 20000) |
| `Eijen-1024` | 4096 B | 47.8 k | 11.7 | 17.7 µs | 231.1 | 100000 (5 × 20000) |
| `Eijen-1024` | 8192 B | 94.1 k | 11.5 | 34.9 µs | 234.6 | 86175 (5 × 17235) |
| `Eijen-1024` | 16384 B | 186.7 k | 11.4 | 69.3 µs | 236.5 | 44695 (5 × 8939) |
| `Eijen-1024` | 65536 B | 745.3 k | 11.4 | 277 µs | 237.0 | 11380 (5 × 2276) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Eijen-256` | hash_32 | 12440 | 1400 KiB | 1464 KiB |
| `Eijen-256` | hash_128 | 12440 | 1396 KiB | 1460 KiB |
| `Eijen-256` | hash_512 | 12440 | 1400 KiB | 1464 KiB |
| `Eijen-256` | hash_1024 | 12440 | 1404 KiB | 1468 KiB |
| `Eijen-256` | hash_4096 | 12440 | 1404 KiB | 1468 KiB |
| `Eijen-256` | hash_8192 | 12440 | 1408 KiB | 1472 KiB |
| `Eijen-256` | hash_16384 | 12440 | 1416 KiB | 1480 KiB |
| `Eijen-256` | hash_65536 | 12440 | 1464 KiB | 1528 KiB |
| `Eijen-384` | hash_32 | 12440 | 1400 KiB | 1464 KiB |
| `Eijen-384` | hash_128 | 12440 | 1400 KiB | 1464 KiB |
| `Eijen-384` | hash_512 | 12440 | 1400 KiB | 1464 KiB |
| `Eijen-384` | hash_1024 | 12440 | 1404 KiB | 1468 KiB |
| `Eijen-384` | hash_4096 | 12440 | 1404 KiB | 1468 KiB |
| `Eijen-384` | hash_8192 | 12440 | 1408 KiB | 1472 KiB |
| `Eijen-384` | hash_16384 | 12440 | 1416 KiB | 1480 KiB |
| `Eijen-384` | hash_65536 | 12440 | 1464 KiB | 1528 KiB |
| `Eijen-512` | hash_32 | 12440 | 1400 KiB | 1464 KiB |
| `Eijen-512` | hash_128 | 12440 | 3444 KiB | 3508 KiB |
| `Eijen-512` | hash_512 | 12440 | 1400 KiB | 1464 KiB |
| `Eijen-512` | hash_1024 | 12440 | 1404 KiB | 1468 KiB |
| `Eijen-512` | hash_4096 | 12440 | 1404 KiB | 1468 KiB |
| `Eijen-512` | hash_8192 | 12440 | 1408 KiB | 1472 KiB |
| `Eijen-512` | hash_16384 | 12440 | 1416 KiB | 1480 KiB |
| `Eijen-512` | hash_65536 | 12440 | 1464 KiB | 1528 KiB |
| `Eijen-768` | hash_32 | 12440 | 1400 KiB | 1464 KiB |
| `Eijen-768` | hash_128 | 12440 | 1400 KiB | 1464 KiB |
| `Eijen-768` | hash_512 | 12440 | 1400 KiB | 1464 KiB |
| `Eijen-768` | hash_1024 | 12440 | 1404 KiB | 1468 KiB |
| `Eijen-768` | hash_4096 | 12440 | 3432 KiB | 3496 KiB |
| `Eijen-768` | hash_8192 | 12440 | 1408 KiB | 1472 KiB |
| `Eijen-768` | hash_16384 | 12440 | 3452 KiB | 3516 KiB |
| `Eijen-768` | hash_65536 | 12440 | 1464 KiB | 1528 KiB |
| `Eijen-1024` | hash_32 | 12440 | 1400 KiB | 1464 KiB |
| `Eijen-1024` | hash_128 | 12440 | 1400 KiB | 1464 KiB |
| `Eijen-1024` | hash_512 | 12440 | 1400 KiB | 1464 KiB |
| `Eijen-1024` | hash_1024 | 12440 | 1404 KiB | 1468 KiB |
| `Eijen-1024` | hash_4096 | 12440 | 1404 KiB | 1468 KiB |
| `Eijen-1024` | hash_8192 | 12440 | 1408 KiB | 1472 KiB |
| `Eijen-1024` | hash_16384 | 12440 | 1416 KiB | 1480 KiB |
| `Eijen-1024` | hash_65536 | 12440 | 1460 KiB | 1524 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Eijen-256` | KAT log (sha256 `75fd74e5b83ee9bd…`) | `kat/hash-09/Eijen-256.log` |
| `Eijen-256` | timing hash_1024 | `records/hash-09/Eijen-256__hash_1024.json` |
| `Eijen-256` | timing hash_128 | `records/hash-09/Eijen-256__hash_128.json` |
| `Eijen-256` | timing hash_16384 | `records/hash-09/Eijen-256__hash_16384.json` |
| `Eijen-256` | timing hash_32 | `records/hash-09/Eijen-256__hash_32.json` |
| `Eijen-256` | timing hash_4096 | `records/hash-09/Eijen-256__hash_4096.json` |
| `Eijen-256` | timing hash_512 | `records/hash-09/Eijen-256__hash_512.json` |
| `Eijen-256` | timing hash_65536 | `records/hash-09/Eijen-256__hash_65536.json` |
| `Eijen-256` | timing hash_8192 | `records/hash-09/Eijen-256__hash_8192.json` |
| `Eijen-384` | KAT log (sha256 `a4c96dfa6b2b5786…`) | `kat/hash-09/Eijen-384.log` |
| `Eijen-384` | timing hash_1024 | `records/hash-09/Eijen-384__hash_1024.json` |
| `Eijen-384` | timing hash_128 | `records/hash-09/Eijen-384__hash_128.json` |
| `Eijen-384` | timing hash_16384 | `records/hash-09/Eijen-384__hash_16384.json` |
| `Eijen-384` | timing hash_32 | `records/hash-09/Eijen-384__hash_32.json` |
| `Eijen-384` | timing hash_4096 | `records/hash-09/Eijen-384__hash_4096.json` |
| `Eijen-384` | timing hash_512 | `records/hash-09/Eijen-384__hash_512.json` |
| `Eijen-384` | timing hash_65536 | `records/hash-09/Eijen-384__hash_65536.json` |
| `Eijen-384` | timing hash_8192 | `records/hash-09/Eijen-384__hash_8192.json` |
| `Eijen-512` | KAT log (sha256 `b2861bb7b8a4d8cc…`) | `kat/hash-09/Eijen-512.log` |
| `Eijen-512` | timing hash_1024 | `records/hash-09/Eijen-512__hash_1024.json` |
| `Eijen-512` | timing hash_128 | `records/hash-09/Eijen-512__hash_128.json` |
| `Eijen-512` | timing hash_16384 | `records/hash-09/Eijen-512__hash_16384.json` |
| `Eijen-512` | timing hash_32 | `records/hash-09/Eijen-512__hash_32.json` |
| `Eijen-512` | timing hash_4096 | `records/hash-09/Eijen-512__hash_4096.json` |
| `Eijen-512` | timing hash_512 | `records/hash-09/Eijen-512__hash_512.json` |
| `Eijen-512` | timing hash_65536 | `records/hash-09/Eijen-512__hash_65536.json` |
| `Eijen-512` | timing hash_8192 | `records/hash-09/Eijen-512__hash_8192.json` |
| `Eijen-768` | KAT log (sha256 `17bca75b514aed08…`) | `kat/hash-09/Eijen-768.log` |
| `Eijen-768` | timing hash_1024 | `records/hash-09/Eijen-768__hash_1024.json` |
| `Eijen-768` | timing hash_128 | `records/hash-09/Eijen-768__hash_128.json` |
| `Eijen-768` | timing hash_16384 | `records/hash-09/Eijen-768__hash_16384.json` |
| `Eijen-768` | timing hash_32 | `records/hash-09/Eijen-768__hash_32.json` |
| `Eijen-768` | timing hash_4096 | `records/hash-09/Eijen-768__hash_4096.json` |
| `Eijen-768` | timing hash_512 | `records/hash-09/Eijen-768__hash_512.json` |
| `Eijen-768` | timing hash_65536 | `records/hash-09/Eijen-768__hash_65536.json` |
| `Eijen-768` | timing hash_8192 | `records/hash-09/Eijen-768__hash_8192.json` |
| `Eijen-1024` | KAT log (sha256 `0bb0da6e7c0ebded…`) | `kat/hash-09/Eijen-1024.log` |
| `Eijen-1024` | timing hash_1024 | `records/hash-09/Eijen-1024__hash_1024.json` |
| `Eijen-1024` | timing hash_128 | `records/hash-09/Eijen-1024__hash_128.json` |
| `Eijen-1024` | timing hash_16384 | `records/hash-09/Eijen-1024__hash_16384.json` |
| `Eijen-1024` | timing hash_32 | `records/hash-09/Eijen-1024__hash_32.json` |
| `Eijen-1024` | timing hash_4096 | `records/hash-09/Eijen-1024__hash_4096.json` |
| `Eijen-1024` | timing hash_512 | `records/hash-09/Eijen-1024__hash_512.json` |
| `Eijen-1024` | timing hash_65536 | `records/hash-09/Eijen-1024__hash_65536.json` |
| `Eijen-1024` | timing hash_8192 | `records/hash-09/Eijen-1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

