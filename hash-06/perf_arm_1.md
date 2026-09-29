# hash-06 Cuishen — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-06` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101539989910802432.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: Cuishen
- Implementation versions measured: reference
- Parameter sets: `Cuishen-512`, `Cuishen-768`, `Cuishen-1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-06/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Cuishen-512` | guide | PASS |
| `Cuishen-768` | guide | PASS |
| `Cuishen-1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `Cuishen-512` | 32 B | 1358 | 42.4 | 506 ns | 63.3 | 100000 (5 × 20000) |
| `Cuishen-512` | 128 B | 2685 | 21.0 | 998 ns | 128.3 | 100000 (5 × 20000) |
| `Cuishen-512` | 512 B | 6639 | 13.0 | 2.47 µs | 207.7 | 100000 (5 × 20000) |
| `Cuishen-512` | 1024 B | 12.0 k | 11.7 | 4.44 µs | 230.6 | 100000 (5 × 20000) |
| `Cuishen-512` | 4096 B | 43.9 k | 10.7 | 16.3 µs | 251.5 | 100000 (5 × 20000) |
| `Cuishen-512` | 8192 B | 86.6 k | 10.6 | 32.1 µs | 255.1 | 87595 (5 × 17519) |
| `Cuishen-512` | 16384 B | 172.8 k | 10.5 | 64.1 µs | 255.5 | 47810 (5 × 9562) |
| `Cuishen-512` | 65536 B | 679.2 k | 10.4 | 252 µs | 260.0 | 12255 (5 × 2451) |
| `Cuishen-768` | 32 B | 1780 | 55.6 | 662 ns | 48.3 | 100000 (5 × 20000) |
| `Cuishen-768` | 128 B | 3537 | 27.6 | 1.31 µs | 97.4 | 100000 (5 × 20000) |
| `Cuishen-768` | 512 B | 8809 | 17.2 | 3.27 µs | 156.5 | 100000 (5 × 20000) |
| `Cuishen-768` | 1024 B | 15.9 k | 15.5 | 5.89 µs | 174.0 | 100000 (5 × 20000) |
| `Cuishen-768` | 4096 B | 58.3 k | 14.2 | 21.6 µs | 189.4 | 100000 (5 × 20000) |
| `Cuishen-768` | 8192 B | 114.3 k | 13.9 | 42.4 µs | 193.2 | 67040 (5 × 13408) |
| `Cuishen-768` | 16384 B | 228.5 k | 13.9 | 84.8 µs | 193.2 | 35795 (5 × 7159) |
| `Cuishen-768` | 65536 B | 909.9 k | 13.9 | 338 µs | 194.1 | 9330 (5 × 1866) |
| `Cuishen-1024` | 32 B | 1823 | 57.0 | 678 ns | 47.2 | 100000 (5 × 20000) |
| `Cuishen-1024` | 128 B | 3539 | 27.6 | 1.31 µs | 97.4 | 100000 (5 × 20000) |
| `Cuishen-1024` | 512 B | 8869 | 17.3 | 3.29 µs | 155.5 | 100000 (5 × 20000) |
| `Cuishen-1024` | 1024 B | 15.9 k | 15.5 | 5.89 µs | 173.9 | 100000 (5 × 20000) |
| `Cuishen-1024` | 4096 B | 58.0 k | 14.2 | 21.5 µs | 190.4 | 100000 (5 × 20000) |
| `Cuishen-1024` | 8192 B | 115.0 k | 14.0 | 42.7 µs | 192.1 | 68280 (5 × 13656) |
| `Cuishen-1024` | 16384 B | 227.0 k | 13.9 | 84.2 µs | 194.5 | 35245 (5 × 7049) |
| `Cuishen-1024` | 65536 B | 904.2 k | 13.8 | 336 µs | 195.3 | 7225 (5 × 1445) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Cuishen-512` | hash_32 | 11924 | 1400 KiB | 1464 KiB |
| `Cuishen-512` | hash_128 | 11924 | 1400 KiB | 1464 KiB |
| `Cuishen-512` | hash_512 | 11924 | 1400 KiB | 1464 KiB |
| `Cuishen-512` | hash_1024 | 11924 | 1404 KiB | 1468 KiB |
| `Cuishen-512` | hash_4096 | 11924 | 1404 KiB | 1468 KiB |
| `Cuishen-512` | hash_8192 | 11924 | 1408 KiB | 1472 KiB |
| `Cuishen-512` | hash_16384 | 11924 | 1416 KiB | 1480 KiB |
| `Cuishen-512` | hash_65536 | 11924 | 3488 KiB | 3552 KiB |
| `Cuishen-768` | hash_32 | 12316 | 1400 KiB | 1464 KiB |
| `Cuishen-768` | hash_128 | 12316 | 1400 KiB | 1464 KiB |
| `Cuishen-768` | hash_512 | 12316 | 1400 KiB | 1464 KiB |
| `Cuishen-768` | hash_1024 | 12316 | 1400 KiB | 1468 KiB |
| `Cuishen-768` | hash_4096 | 12316 | 1404 KiB | 1472 KiB |
| `Cuishen-768` | hash_8192 | 12316 | 1408 KiB | 1472 KiB |
| `Cuishen-768` | hash_16384 | 12316 | 1416 KiB | 1480 KiB |
| `Cuishen-768` | hash_65536 | 12316 | 1464 KiB | 1532 KiB |
| `Cuishen-1024` | hash_32 | 13060 | 1400 KiB | 1464 KiB |
| `Cuishen-1024` | hash_128 | 13060 | 1400 KiB | 1464 KiB |
| `Cuishen-1024` | hash_512 | 13060 | 1400 KiB | 1468 KiB |
| `Cuishen-1024` | hash_1024 | 13060 | 1404 KiB | 1468 KiB |
| `Cuishen-1024` | hash_4096 | 13060 | 1404 KiB | 1468 KiB |
| `Cuishen-1024` | hash_8192 | 13060 | 1408 KiB | 1476 KiB |
| `Cuishen-1024` | hash_16384 | 13060 | 1412 KiB | 1480 KiB |
| `Cuishen-1024` | hash_65536 | 13060 | 3460 KiB | 3528 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Cuishen-512` | KAT log (sha256 `448d543dc1c5c7cb…`) | `kat/hash-06/Cuishen-512.log` |
| `Cuishen-512` | timing hash_1024 | `records/hash-06/Cuishen-512__hash_1024.json` |
| `Cuishen-512` | timing hash_128 | `records/hash-06/Cuishen-512__hash_128.json` |
| `Cuishen-512` | timing hash_16384 | `records/hash-06/Cuishen-512__hash_16384.json` |
| `Cuishen-512` | timing hash_32 | `records/hash-06/Cuishen-512__hash_32.json` |
| `Cuishen-512` | timing hash_4096 | `records/hash-06/Cuishen-512__hash_4096.json` |
| `Cuishen-512` | timing hash_512 | `records/hash-06/Cuishen-512__hash_512.json` |
| `Cuishen-512` | timing hash_65536 | `records/hash-06/Cuishen-512__hash_65536.json` |
| `Cuishen-512` | timing hash_8192 | `records/hash-06/Cuishen-512__hash_8192.json` |
| `Cuishen-768` | KAT log (sha256 `043bd47c642bb574…`) | `kat/hash-06/Cuishen-768.log` |
| `Cuishen-768` | timing hash_1024 | `records/hash-06/Cuishen-768__hash_1024.json` |
| `Cuishen-768` | timing hash_128 | `records/hash-06/Cuishen-768__hash_128.json` |
| `Cuishen-768` | timing hash_16384 | `records/hash-06/Cuishen-768__hash_16384.json` |
| `Cuishen-768` | timing hash_32 | `records/hash-06/Cuishen-768__hash_32.json` |
| `Cuishen-768` | timing hash_4096 | `records/hash-06/Cuishen-768__hash_4096.json` |
| `Cuishen-768` | timing hash_512 | `records/hash-06/Cuishen-768__hash_512.json` |
| `Cuishen-768` | timing hash_65536 | `records/hash-06/Cuishen-768__hash_65536.json` |
| `Cuishen-768` | timing hash_8192 | `records/hash-06/Cuishen-768__hash_8192.json` |
| `Cuishen-1024` | KAT log (sha256 `eab5dc799b3b17ea…`) | `kat/hash-06/Cuishen-1024.log` |
| `Cuishen-1024` | timing hash_1024 | `records/hash-06/Cuishen-1024__hash_1024.json` |
| `Cuishen-1024` | timing hash_128 | `records/hash-06/Cuishen-1024__hash_128.json` |
| `Cuishen-1024` | timing hash_16384 | `records/hash-06/Cuishen-1024__hash_16384.json` |
| `Cuishen-1024` | timing hash_32 | `records/hash-06/Cuishen-1024__hash_32.json` |
| `Cuishen-1024` | timing hash_4096 | `records/hash-06/Cuishen-1024__hash_4096.json` |
| `Cuishen-1024` | timing hash_512 | `records/hash-06/Cuishen-1024__hash_512.json` |
| `Cuishen-1024` | timing hash_65536 | `records/hash-06/Cuishen-1024__hash_65536.json` |
| `Cuishen-1024` | timing hash_8192 | `records/hash-06/Cuishen-1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

