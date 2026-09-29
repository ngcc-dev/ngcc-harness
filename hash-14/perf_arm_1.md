# hash-14 Laurus — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-14` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101538290232020992.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: Laurus
- Implementation versions measured: reference
- Parameter sets: `Laurus-512`, `Laurus-768`, `Laurus-1024`, `Laurus-XOF`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-14/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Laurus-512` | guide | PASS |
| `Laurus-768` | guide | PASS |
| `Laurus-1024` | guide | PASS |
| `Laurus-XOF` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `Laurus-512` | 32 B | 3059 | 95.6 | 1.14 µs | 28.1 | 100000 (5 × 20000) |
| `Laurus-512` | 128 B | 6249 | 48.8 | 2.32 µs | 55.1 | 100000 (5 × 20000) |
| `Laurus-512` | 512 B | 15.3 k | 29.9 | 5.68 µs | 90.2 | 100000 (5 × 20000) |
| `Laurus-512` | 1024 B | 27.2 k | 26.6 | 10.1 µs | 101.4 | 100000 (5 × 20000) |
| `Laurus-512` | 4096 B | 99.5 k | 24.3 | 36.9 µs | 110.9 | 81775 (5 × 16355) |
| `Laurus-512` | 8192 B | 196.9 k | 24.0 | 73.1 µs | 112.1 | 42595 (5 × 8519) |
| `Laurus-512` | 16384 B | 389.7 k | 23.8 | 145 µs | 113.2 | 18935 (5 × 3787) |
| `Laurus-512` | 65536 B | 1.56 M | 23.8 | 580 µs | 113.1 | 5525 (5 × 1105) |
| `Laurus-768` | 32 B | 3086 | 96.4 | 1.15 µs | 27.9 | 100000 (5 × 20000) |
| `Laurus-768` | 128 B | 6059 | 47.3 | 2.25 µs | 56.9 | 100000 (5 × 20000) |
| `Laurus-768` | 512 B | 18.4 k | 36.0 | 6.85 µs | 74.8 | 100000 (5 × 20000) |
| `Laurus-768` | 1024 B | 33.8 k | 33.1 | 12.6 µs | 81.5 | 100000 (5 × 20000) |
| `Laurus-768` | 4096 B | 131.9 k | 32.2 | 48.9 µs | 83.7 | 59705 (5 × 11941) |
| `Laurus-768` | 8192 B | 263.2 k | 32.1 | 97.7 µs | 83.9 | 32150 (5 × 6430) |
| `Laurus-768` | 16384 B | 524.9 k | 32.0 | 195 µs | 84.1 | 16375 (5 × 3275) |
| `Laurus-768` | 65536 B | 2.09 M | 31.9 | 776 µs | 84.4 | 4090 (5 × 818) |
| `Laurus-1024` | 32 B | 3186 | 99.6 | 1.18 µs | 27.0 | 100000 (5 × 20000) |
| `Laurus-1024` | 128 B | 6036 | 47.2 | 2.24 µs | 57.1 | 100000 (5 × 20000) |
| `Laurus-1024` | 512 B | 24.8 k | 48.4 | 9.2 µs | 55.7 | 100000 (5 × 20000) |
| `Laurus-1024` | 1024 B | 48.7 k | 47.6 | 18.1 µs | 56.6 | 100000 (5 × 20000) |
| `Laurus-1024` | 4096 B | 197.0 k | 48.1 | 73.1 µs | 56.0 | 43325 (5 × 8665) |
| `Laurus-1024` | 8192 B | 388.5 k | 47.4 | 144 µs | 56.8 | 21770 (5 × 4354) |
| `Laurus-1024` | 16384 B | 777.5 k | 47.5 | 288 µs | 56.8 | 11050 (5 × 2210) |
| `Laurus-1024` | 65536 B | 3.11 M | 47.5 | 1.16 ms | 56.7 | 2425 (5 × 485) |
| `Laurus-XOF` | 32 B | 6000 | 187.5 | 2.23 µs | 14.4 | 100000 (5 × 20000) |
| `Laurus-XOF` | 128 B | 9108 | 71.2 | 3.38 µs | 37.9 | 100000 (5 × 20000) |
| `Laurus-XOF` | 512 B | 18.4 k | 36.0 | 6.84 µs | 74.9 | 100000 (5 × 20000) |
| `Laurus-XOF` | 1024 B | 30.7 k | 30.0 | 11.4 µs | 90.0 | 100000 (5 × 20000) |
| `Laurus-XOF` | 4096 B | 104.2 k | 25.4 | 38.7 µs | 105.9 | 67610 (5 × 13522) |
| `Laurus-XOF` | 8192 B | 202.9 k | 24.8 | 75.3 µs | 108.8 | 41455 (5 × 8291) |
| `Laurus-XOF` | 16384 B | 397.2 k | 24.2 | 147 µs | 111.2 | 21545 (5 × 4309) |
| `Laurus-XOF` | 65536 B | 1.58 M | 24.1 | 586 µs | 111.9 | 5350 (5 × 1070) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Laurus-512` | hash_32 | 17808 | 1404 KiB | 1468 KiB |
| `Laurus-512` | hash_128 | 17808 | 1404 KiB | 1468 KiB |
| `Laurus-512` | hash_512 | 17808 | 1404 KiB | 1468 KiB |
| `Laurus-512` | hash_1024 | 17808 | 1408 KiB | 1472 KiB |
| `Laurus-512` | hash_4096 | 17808 | 1408 KiB | 1472 KiB |
| `Laurus-512` | hash_8192 | 17808 | 3436 KiB | 3500 KiB |
| `Laurus-512` | hash_16384 | 17808 | 1420 KiB | 1484 KiB |
| `Laurus-512` | hash_65536 | 17808 | 1468 KiB | 1532 KiB |
| `Laurus-768` | hash_32 | 16852 | 1404 KiB | 1468 KiB |
| `Laurus-768` | hash_128 | 16852 | 1404 KiB | 1468 KiB |
| `Laurus-768` | hash_512 | 16852 | 1404 KiB | 1468 KiB |
| `Laurus-768` | hash_1024 | 16852 | 1408 KiB | 1472 KiB |
| `Laurus-768` | hash_4096 | 16852 | 1408 KiB | 1472 KiB |
| `Laurus-768` | hash_8192 | 16852 | 3444 KiB | 3508 KiB |
| `Laurus-768` | hash_16384 | 16852 | 1420 KiB | 1484 KiB |
| `Laurus-768` | hash_65536 | 16852 | 1468 KiB | 1532 KiB |
| `Laurus-1024` | hash_32 | 16708 | 1404 KiB | 1468 KiB |
| `Laurus-1024` | hash_128 | 16708 | 1404 KiB | 1468 KiB |
| `Laurus-1024` | hash_512 | 16708 | 1404 KiB | 1468 KiB |
| `Laurus-1024` | hash_1024 | 16708 | 1408 KiB | 1472 KiB |
| `Laurus-1024` | hash_4096 | 16708 | 1408 KiB | 1472 KiB |
| `Laurus-1024` | hash_8192 | 16708 | 1412 KiB | 1476 KiB |
| `Laurus-1024` | hash_16384 | 16708 | 1420 KiB | 1484 KiB |
| `Laurus-1024` | hash_65536 | 16708 | 1468 KiB | 1532 KiB |
| `Laurus-XOF` | hash_32 | 14704 | 1404 KiB | 1468 KiB |
| `Laurus-XOF` | hash_128 | 14704 | 1404 KiB | 1468 KiB |
| `Laurus-XOF` | hash_512 | 14704 | 1404 KiB | 1468 KiB |
| `Laurus-XOF` | hash_1024 | 14704 | 1408 KiB | 1472 KiB |
| `Laurus-XOF` | hash_4096 | 14704 | 1408 KiB | 1472 KiB |
| `Laurus-XOF` | hash_8192 | 14704 | 1412 KiB | 1476 KiB |
| `Laurus-XOF` | hash_16384 | 14704 | 3456 KiB | 3520 KiB |
| `Laurus-XOF` | hash_65536 | 14704 | 1468 KiB | 1532 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Laurus-512` | KAT log (sha256 `267d12f48778da29…`) | `kat/hash-14/Laurus-512.log` |
| `Laurus-512` | timing hash_1024 | `records/hash-14/Laurus-512__hash_1024.json` |
| `Laurus-512` | timing hash_128 | `records/hash-14/Laurus-512__hash_128.json` |
| `Laurus-512` | timing hash_16384 | `records/hash-14/Laurus-512__hash_16384.json` |
| `Laurus-512` | timing hash_32 | `records/hash-14/Laurus-512__hash_32.json` |
| `Laurus-512` | timing hash_4096 | `records/hash-14/Laurus-512__hash_4096.json` |
| `Laurus-512` | timing hash_512 | `records/hash-14/Laurus-512__hash_512.json` |
| `Laurus-512` | timing hash_65536 | `records/hash-14/Laurus-512__hash_65536.json` |
| `Laurus-512` | timing hash_8192 | `records/hash-14/Laurus-512__hash_8192.json` |
| `Laurus-768` | KAT log (sha256 `7756432630d6a1b5…`) | `kat/hash-14/Laurus-768.log` |
| `Laurus-768` | timing hash_1024 | `records/hash-14/Laurus-768__hash_1024.json` |
| `Laurus-768` | timing hash_128 | `records/hash-14/Laurus-768__hash_128.json` |
| `Laurus-768` | timing hash_16384 | `records/hash-14/Laurus-768__hash_16384.json` |
| `Laurus-768` | timing hash_32 | `records/hash-14/Laurus-768__hash_32.json` |
| `Laurus-768` | timing hash_4096 | `records/hash-14/Laurus-768__hash_4096.json` |
| `Laurus-768` | timing hash_512 | `records/hash-14/Laurus-768__hash_512.json` |
| `Laurus-768` | timing hash_65536 | `records/hash-14/Laurus-768__hash_65536.json` |
| `Laurus-768` | timing hash_8192 | `records/hash-14/Laurus-768__hash_8192.json` |
| `Laurus-1024` | KAT log (sha256 `52a563e471e9ca2a…`) | `kat/hash-14/Laurus-1024.log` |
| `Laurus-1024` | timing hash_1024 | `records/hash-14/Laurus-1024__hash_1024.json` |
| `Laurus-1024` | timing hash_128 | `records/hash-14/Laurus-1024__hash_128.json` |
| `Laurus-1024` | timing hash_16384 | `records/hash-14/Laurus-1024__hash_16384.json` |
| `Laurus-1024` | timing hash_32 | `records/hash-14/Laurus-1024__hash_32.json` |
| `Laurus-1024` | timing hash_4096 | `records/hash-14/Laurus-1024__hash_4096.json` |
| `Laurus-1024` | timing hash_512 | `records/hash-14/Laurus-1024__hash_512.json` |
| `Laurus-1024` | timing hash_65536 | `records/hash-14/Laurus-1024__hash_65536.json` |
| `Laurus-1024` | timing hash_8192 | `records/hash-14/Laurus-1024__hash_8192.json` |
| `Laurus-XOF` | KAT log (sha256 `65e5483fcde38c71…`) | `kat/hash-14/Laurus-XOF.log` |
| `Laurus-XOF` | timing hash_1024 | `records/hash-14/Laurus-XOF__hash_1024.json` |
| `Laurus-XOF` | timing hash_128 | `records/hash-14/Laurus-XOF__hash_128.json` |
| `Laurus-XOF` | timing hash_16384 | `records/hash-14/Laurus-XOF__hash_16384.json` |
| `Laurus-XOF` | timing hash_32 | `records/hash-14/Laurus-XOF__hash_32.json` |
| `Laurus-XOF` | timing hash_4096 | `records/hash-14/Laurus-XOF__hash_4096.json` |
| `Laurus-XOF` | timing hash_512 | `records/hash-14/Laurus-XOF__hash_512.json` |
| `Laurus-XOF` | timing hash_65536 | `records/hash-14/Laurus-XOF__hash_65536.json` |
| `Laurus-XOF` | timing hash_8192 | `records/hash-14/Laurus-XOF__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

