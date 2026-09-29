# hash-24 QuantaSylva Hash — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-24` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101536223673274368.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: QuantaSylva Hash
- Implementation versions measured: reference
- Parameter sets: `QSH-512`, `QSH-768`, `QSH-1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-24/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `QSH-512` | guide | PASS |
| `QSH-768` | guide | PASS |
| `QSH-1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `QSH-512` | 32 B | 66.8 k | 2087.8 | 24.8 µs | 1.3 | 100000 (5 × 20000) |
| `QSH-512` | 128 B | 89.0 k | 695.5 | 33 µs | 3.9 | 85260 (5 × 17052) |
| `QSH-512` | 512 B | 155.5 k | 303.7 | 57.7 µs | 8.9 | 51615 (5 × 10323) |
| `QSH-512` | 1024 B | 245.0 k | 239.2 | 90.9 µs | 11.3 | 33640 (5 × 6728) |
| `QSH-512` | 4096 B | 905.4 k | 221.0 | 336 µs | 12.2 | 9545 (5 × 1909) |
| `QSH-512` | 8192 B | 1.66 M | 202.9 | 617 µs | 13.3 | 4990 (5 × 998) |
| `QSH-512` | 16384 B | 3.27 M | 199.3 | 1.21 ms | 13.5 | 2565 (5 × 513) |
| `QSH-512` | 65536 B | 12.84 M | 195.9 | 4.76 ms | 13.8 | 645 (5 × 129) |
| `QSH-768` | 32 B | 131.0 k | 4094.0 | 48.6 µs | 0.7 | 60455 (5 × 12091) |
| `QSH-768` | 128 B | 130.5 k | 1019.6 | 48.4 µs | 2.6 | 60000 (5 × 12000) |
| `QSH-768` | 512 B | 218.5 k | 426.7 | 81.1 µs | 6.3 | 37210 (5 × 7442) |
| `QSH-768` | 1024 B | 319.3 k | 311.8 | 119 µs | 8.6 | 27030 (5 × 5406) |
| `QSH-768` | 4096 B | 913.7 k | 223.1 | 339 µs | 12.1 | 9215 (5 × 1843) |
| `QSH-768` | 8192 B | 1.70 M | 207.2 | 630 µs | 13.0 | 4945 (5 × 989) |
| `QSH-768` | 16384 B | 3.26 M | 198.9 | 1.21 ms | 13.6 | 2595 (5 × 519) |
| `QSH-768` | 65536 B | 12.63 M | 192.7 | 4.69 ms | 14.0 | 660 (5 × 132) |
| `QSH-1024` | 32 B | 131.2 k | 4099.2 | 48.7 µs | 0.7 | 59045 (5 × 11809) |
| `QSH-1024` | 128 B | 130.7 k | 1020.9 | 48.5 µs | 2.6 | 59410 (5 × 11882) |
| `QSH-1024` | 512 B | 217.7 k | 425.2 | 80.8 µs | 6.3 | 30365 (5 × 6073) |
| `QSH-1024` | 1024 B | 305.5 k | 298.3 | 113 µs | 9.0 | 27135 (5 × 5427) |
| `QSH-1024` | 4096 B | 912.9 k | 222.9 | 339 µs | 12.1 | 9270 (5 × 1854) |
| `QSH-1024` | 8192 B | 1.69 M | 206.8 | 629 µs | 13.0 | 4870 (5 × 974) |
| `QSH-1024` | 16384 B | 3.26 M | 199.0 | 1.21 ms | 13.5 | 2615 (5 × 523) |
| `QSH-1024` | 65536 B | 12.64 M | 192.9 | 4.69 ms | 14.0 | 670 (5 × 134) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `QSH-512` | hash_32 | 13420 | 1400 KiB | 1464 KiB |
| `QSH-512` | hash_128 | 13420 | 1396 KiB | 1460 KiB |
| `QSH-512` | hash_512 | 13420 | 1400 KiB | 1464 KiB |
| `QSH-512` | hash_1024 | 13420 | 1404 KiB | 1468 KiB |
| `QSH-512` | hash_4096 | 13420 | 3440 KiB | 3504 KiB |
| `QSH-512` | hash_8192 | 13420 | 3444 KiB | 3512 KiB |
| `QSH-512` | hash_16384 | 13420 | 1416 KiB | 1492 KiB |
| `QSH-512` | hash_65536 | 13420 | 1464 KiB | 1544 KiB |
| `QSH-768` | hash_32 | 13420 | 1400 KiB | 1464 KiB |
| `QSH-768` | hash_128 | 13420 | 1400 KiB | 1464 KiB |
| `QSH-768` | hash_512 | 13420 | 1400 KiB | 1464 KiB |
| `QSH-768` | hash_1024 | 13420 | 1404 KiB | 1468 KiB |
| `QSH-768` | hash_4096 | 13420 | 1404 KiB | 1468 KiB |
| `QSH-768` | hash_8192 | 13420 | 1408 KiB | 1476 KiB |
| `QSH-768` | hash_16384 | 13420 | 3440 KiB | 3512 KiB |
| `QSH-768` | hash_65536 | 13420 | 3476 KiB | 3548 KiB |
| `QSH-1024` | hash_32 | 13404 | 1400 KiB | 1464 KiB |
| `QSH-1024` | hash_128 | 13404 | 1400 KiB | 1464 KiB |
| `QSH-1024` | hash_512 | 13404 | 1400 KiB | 1464 KiB |
| `QSH-1024` | hash_1024 | 13404 | 1404 KiB | 1468 KiB |
| `QSH-1024` | hash_4096 | 13404 | 1404 KiB | 1468 KiB |
| `QSH-1024` | hash_8192 | 13404 | 1408 KiB | 1476 KiB |
| `QSH-1024` | hash_16384 | 13404 | 1416 KiB | 1484 KiB |
| `QSH-1024` | hash_65536 | 13404 | 3480 KiB | 3552 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `QSH-512` | KAT log (sha256 `f2bd5effe140b5a9…`) | `kat/hash-24/QSH-512.log` |
| `QSH-512` | timing hash_1024 | `records/hash-24/QSH-512__hash_1024.json` |
| `QSH-512` | timing hash_128 | `records/hash-24/QSH-512__hash_128.json` |
| `QSH-512` | timing hash_16384 | `records/hash-24/QSH-512__hash_16384.json` |
| `QSH-512` | timing hash_32 | `records/hash-24/QSH-512__hash_32.json` |
| `QSH-512` | timing hash_4096 | `records/hash-24/QSH-512__hash_4096.json` |
| `QSH-512` | timing hash_512 | `records/hash-24/QSH-512__hash_512.json` |
| `QSH-512` | timing hash_65536 | `records/hash-24/QSH-512__hash_65536.json` |
| `QSH-512` | timing hash_8192 | `records/hash-24/QSH-512__hash_8192.json` |
| `QSH-768` | KAT log (sha256 `9345d9fb706dd2c2…`) | `kat/hash-24/QSH-768.log` |
| `QSH-768` | timing hash_1024 | `records/hash-24/QSH-768__hash_1024.json` |
| `QSH-768` | timing hash_128 | `records/hash-24/QSH-768__hash_128.json` |
| `QSH-768` | timing hash_16384 | `records/hash-24/QSH-768__hash_16384.json` |
| `QSH-768` | timing hash_32 | `records/hash-24/QSH-768__hash_32.json` |
| `QSH-768` | timing hash_4096 | `records/hash-24/QSH-768__hash_4096.json` |
| `QSH-768` | timing hash_512 | `records/hash-24/QSH-768__hash_512.json` |
| `QSH-768` | timing hash_65536 | `records/hash-24/QSH-768__hash_65536.json` |
| `QSH-768` | timing hash_8192 | `records/hash-24/QSH-768__hash_8192.json` |
| `QSH-1024` | KAT log (sha256 `d12a55414cdc4bab…`) | `kat/hash-24/QSH-1024.log` |
| `QSH-1024` | timing hash_1024 | `records/hash-24/QSH-1024__hash_1024.json` |
| `QSH-1024` | timing hash_128 | `records/hash-24/QSH-1024__hash_128.json` |
| `QSH-1024` | timing hash_16384 | `records/hash-24/QSH-1024__hash_16384.json` |
| `QSH-1024` | timing hash_32 | `records/hash-24/QSH-1024__hash_32.json` |
| `QSH-1024` | timing hash_4096 | `records/hash-24/QSH-1024__hash_4096.json` |
| `QSH-1024` | timing hash_512 | `records/hash-24/QSH-1024__hash_512.json` |
| `QSH-1024` | timing hash_65536 | `records/hash-24/QSH-1024__hash_65536.json` |
| `QSH-1024` | timing hash_8192 | `records/hash-24/QSH-1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

