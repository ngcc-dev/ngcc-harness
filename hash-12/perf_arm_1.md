# hash-12 Iphe — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-12` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101538722543128576.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: Iphe
- Implementation versions measured: reference
- Parameter sets: `Iphe-512`, `Iphe-768`, `Iphe-1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-12/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Iphe-512` | guide | PASS |
| `Iphe-768` | guide | PASS |
| `Iphe-1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `Iphe-512` | 32 B | 13.1 k | 408.1 | 4.85 µs | 6.6 | 100000 (5 × 20000) |
| `Iphe-512` | 128 B | 13.5 k | 105.3 | 5 µs | 25.6 | 100000 (5 × 20000) |
| `Iphe-512` | 512 B | 34.6 k | 67.6 | 12.8 µs | 39.9 | 100000 (5 × 20000) |
| `Iphe-512` | 1024 B | 66.1 k | 64.6 | 24.5 µs | 41.7 | 97565 (5 × 19513) |
| `Iphe-512` | 4096 B | 244.9 k | 59.8 | 90.9 µs | 45.1 | 32635 (5 × 6527) |
| `Iphe-512` | 8192 B | 477.3 k | 58.3 | 177 µs | 46.2 | 17175 (5 × 3435) |
| `Iphe-512` | 16384 B | 951.5 k | 58.1 | 353 µs | 46.4 | 8840 (5 × 1768) |
| `Iphe-512` | 65536 B | 3.77 M | 57.5 | 1.4 ms | 46.9 | 2110 (5 × 422) |
| `Iphe-768` | 32 B | 12.6 k | 393.0 | 4.67 µs | 6.9 | 100000 (5 × 20000) |
| `Iphe-768` | 128 B | 13.0 k | 101.6 | 4.83 µs | 26.5 | 100000 (5 × 20000) |
| `Iphe-768` | 512 B | 43.9 k | 85.7 | 16.3 µs | 31.5 | 100000 (5 × 20000) |
| `Iphe-768` | 1024 B | 75.3 k | 73.6 | 28 µs | 36.6 | 92490 (5 × 18498) |
| `Iphe-768` | 4096 B | 283.6 k | 69.2 | 105 µs | 38.9 | 26715 (5 × 5343) |
| `Iphe-768` | 8192 B | 564.3 k | 68.9 | 209 µs | 39.1 | 14075 (5 × 2815) |
| `Iphe-768` | 16384 B | 1.13 M | 68.7 | 418 µs | 39.2 | 5935 (5 × 1187) |
| `Iphe-768` | 65536 B | 4.50 M | 68.6 | 1.67 ms | 39.3 | 1890 (5 × 378) |
| `Iphe-1024` | 32 B | 12.1 k | 378.7 | 4.5 µs | 7.1 | 100000 (5 × 20000) |
| `Iphe-1024` | 128 B | 22.3 k | 174.1 | 8.27 µs | 15.5 | 100000 (5 × 20000) |
| `Iphe-1024` | 512 B | 53.2 k | 103.9 | 19.7 µs | 25.9 | 100000 (5 × 20000) |
| `Iphe-1024` | 1024 B | 94.5 k | 92.3 | 35.1 µs | 29.2 | 78050 (5 × 15610) |
| `Iphe-1024` | 4096 B | 361.5 k | 88.3 | 134 µs | 30.5 | 11170 (5 × 2234) |
| `Iphe-1024` | 8192 B | 711.2 k | 86.8 | 264 µs | 31.0 | 11445 (5 × 2289) |
| `Iphe-1024` | 16384 B | 1.41 M | 86.0 | 523 µs | 31.3 | 6020 (5 × 1204) |
| `Iphe-1024` | 65536 B | 5.62 M | 85.8 | 2.09 ms | 31.4 | 1465 (5 × 293) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Iphe-512` | hash_32 | 11644 | 1400 KiB | 1464 KiB |
| `Iphe-512` | hash_128 | 11644 | 1400 KiB | 1464 KiB |
| `Iphe-512` | hash_512 | 11644 | 1400 KiB | 1464 KiB |
| `Iphe-512` | hash_1024 | 11644 | 1404 KiB | 1468 KiB |
| `Iphe-512` | hash_4096 | 11644 | 1404 KiB | 1468 KiB |
| `Iphe-512` | hash_8192 | 11644 | 1408 KiB | 1472 KiB |
| `Iphe-512` | hash_16384 | 11644 | 1416 KiB | 1480 KiB |
| `Iphe-512` | hash_65536 | 11644 | 1464 KiB | 1528 KiB |
| `Iphe-768` | hash_32 | 11644 | 1400 KiB | 1464 KiB |
| `Iphe-768` | hash_128 | 11644 | 1400 KiB | 1464 KiB |
| `Iphe-768` | hash_512 | 11644 | 1400 KiB | 1464 KiB |
| `Iphe-768` | hash_1024 | 11644 | 1404 KiB | 1468 KiB |
| `Iphe-768` | hash_4096 | 11644 | 1404 KiB | 1468 KiB |
| `Iphe-768` | hash_8192 | 11644 | 1408 KiB | 1472 KiB |
| `Iphe-768` | hash_16384 | 11644 | 1416 KiB | 1480 KiB |
| `Iphe-768` | hash_65536 | 11644 | 1464 KiB | 1528 KiB |
| `Iphe-1024` | hash_32 | 11668 | 1400 KiB | 1464 KiB |
| `Iphe-1024` | hash_128 | 11668 | 1400 KiB | 1464 KiB |
| `Iphe-1024` | hash_512 | 11668 | 1400 KiB | 1464 KiB |
| `Iphe-1024` | hash_1024 | 11668 | 1404 KiB | 1468 KiB |
| `Iphe-1024` | hash_4096 | 11668 | 1404 KiB | 1468 KiB |
| `Iphe-1024` | hash_8192 | 11668 | 1408 KiB | 1472 KiB |
| `Iphe-1024` | hash_16384 | 11668 | 1416 KiB | 1480 KiB |
| `Iphe-1024` | hash_65536 | 11668 | 1464 KiB | 1528 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Iphe-512` | KAT log (sha256 `6a14644a2ee8ce1e…`) | `kat/hash-12/Iphe-512.log` |
| `Iphe-512` | timing hash_1024 | `records/hash-12/Iphe-512__hash_1024.json` |
| `Iphe-512` | timing hash_128 | `records/hash-12/Iphe-512__hash_128.json` |
| `Iphe-512` | timing hash_16384 | `records/hash-12/Iphe-512__hash_16384.json` |
| `Iphe-512` | timing hash_32 | `records/hash-12/Iphe-512__hash_32.json` |
| `Iphe-512` | timing hash_4096 | `records/hash-12/Iphe-512__hash_4096.json` |
| `Iphe-512` | timing hash_512 | `records/hash-12/Iphe-512__hash_512.json` |
| `Iphe-512` | timing hash_65536 | `records/hash-12/Iphe-512__hash_65536.json` |
| `Iphe-512` | timing hash_8192 | `records/hash-12/Iphe-512__hash_8192.json` |
| `Iphe-768` | KAT log (sha256 `be5581c5a4fe5cbc…`) | `kat/hash-12/Iphe-768.log` |
| `Iphe-768` | timing hash_1024 | `records/hash-12/Iphe-768__hash_1024.json` |
| `Iphe-768` | timing hash_128 | `records/hash-12/Iphe-768__hash_128.json` |
| `Iphe-768` | timing hash_16384 | `records/hash-12/Iphe-768__hash_16384.json` |
| `Iphe-768` | timing hash_32 | `records/hash-12/Iphe-768__hash_32.json` |
| `Iphe-768` | timing hash_4096 | `records/hash-12/Iphe-768__hash_4096.json` |
| `Iphe-768` | timing hash_512 | `records/hash-12/Iphe-768__hash_512.json` |
| `Iphe-768` | timing hash_65536 | `records/hash-12/Iphe-768__hash_65536.json` |
| `Iphe-768` | timing hash_8192 | `records/hash-12/Iphe-768__hash_8192.json` |
| `Iphe-1024` | KAT log (sha256 `1e5305894210ba38…`) | `kat/hash-12/Iphe-1024.log` |
| `Iphe-1024` | timing hash_1024 | `records/hash-12/Iphe-1024__hash_1024.json` |
| `Iphe-1024` | timing hash_128 | `records/hash-12/Iphe-1024__hash_128.json` |
| `Iphe-1024` | timing hash_16384 | `records/hash-12/Iphe-1024__hash_16384.json` |
| `Iphe-1024` | timing hash_32 | `records/hash-12/Iphe-1024__hash_32.json` |
| `Iphe-1024` | timing hash_4096 | `records/hash-12/Iphe-1024__hash_4096.json` |
| `Iphe-1024` | timing hash_512 | `records/hash-12/Iphe-1024__hash_512.json` |
| `Iphe-1024` | timing hash_65536 | `records/hash-12/Iphe-1024__hash_65536.json` |
| `Iphe-1024` | timing hash_8192 | `records/hash-12/Iphe-1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

