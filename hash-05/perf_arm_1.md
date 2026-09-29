# hash-05 Cryptographic Hash Algorithm uHash — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-05` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101540182026702848.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: Cryptographic Hash Algorithm uHash
- Implementation versions measured: reference
- Parameter sets: `uHash-512`, `uHash-768`, `uHash-1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-05/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `uHash-512` | guide | PASS |
| `uHash-768` | guide | PASS |
| `uHash-1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `uHash-512` | 32 B | 61.6 k | 1924.1 | 22.8 µs | 1.4 | 100000 (5 × 20000) |
| `uHash-512` | 128 B | 123.1 k | 961.8 | 45.7 µs | 2.8 | 63245 (5 × 12649) |
| `uHash-512` | 512 B | 366.4 k | 715.6 | 136 µs | 3.8 | 21930 (5 × 4386) |
| `uHash-512` | 1024 B | 671.7 k | 656.0 | 249 µs | 4.1 | 12665 (5 × 2533) |
| `uHash-512` | 4096 B | 2.63 M | 641.0 | 974 µs | 4.2 | 3210 (5 × 642) |
| `uHash-512` | 8192 B | 5.26 M | 641.9 | 1.95 ms | 4.2 | 1515 (5 × 303) |
| `uHash-512` | 16384 B | 10.43 M | 636.4 | 3.87 ms | 4.2 | 810 (5 × 162) |
| `uHash-512` | 65536 B | 41.79 M | 637.7 | 15.5 ms | 4.2 | 205 (5 × 41) |
| `uHash-768` | 32 B | 92.5 k | 2891.4 | 34.3 µs | 0.9 | 81220 (5 × 16244) |
| `uHash-768` | 128 B | 277.1 k | 2164.6 | 103 µs | 1.2 | 30135 (5 × 6027) |
| `uHash-768` | 512 B | 823.1 k | 1607.6 | 305 µs | 1.7 | 10265 (5 × 2053) |
| `uHash-768` | 1024 B | 1.55 M | 1516.4 | 576 µs | 1.8 | 5435 (5 × 1087) |
| `uHash-768` | 4096 B | 6.02 M | 1470.3 | 2.23 ms | 1.8 | 1445 (5 × 289) |
| `uHash-768` | 8192 B | 11.79 M | 1439.8 | 4.37 ms | 1.9 | 725 (5 × 145) |
| `uHash-768` | 16384 B | 23.48 M | 1433.3 | 8.71 ms | 1.9 | 365 (5 × 73) |
| `uHash-768` | 65536 B | 93.75 M | 1430.5 | 34.8 ms | 1.9 | 100 (5 × 20) |
| `uHash-1024` | 32 B | 244.5 k | 7640.6 | 90.7 µs | 0.4 | 32765 (5 × 6553) |
| `uHash-1024` | 128 B | 609.8 k | 4764.1 | 226 µs | 0.6 | 13510 (5 × 2702) |
| `uHash-1024` | 512 B | 2.07 M | 4047.2 | 769 µs | 0.7 | 4120 (5 × 824) |
| `uHash-1024` | 1024 B | 4.07 M | 3971.0 | 1.51 ms | 0.7 | 2090 (5 × 418) |
| `uHash-1024` | 4096 B | 15.84 M | 3866.4 | 5.88 ms | 0.7 | 540 (5 × 108) |
| `uHash-1024` | 8192 B | 32.27 M | 3939.3 | 12 ms | 0.7 | 275 (5 × 55) |
| `uHash-1024` | 16384 B | 62.58 M | 3819.8 | 23.2 ms | 0.7 | 140 (5 × 28) |
| `uHash-1024` | 65536 B | 253.45 M | 3867.4 | 94 ms | 0.7 | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `uHash-512` | hash_32 | 16756 | 1404 KiB | 1472 KiB |
| `uHash-512` | hash_128 | 16756 | 1404 KiB | 1472 KiB |
| `uHash-512` | hash_512 | 16756 | 1404 KiB | 1472 KiB |
| `uHash-512` | hash_1024 | 16756 | 1408 KiB | 1476 KiB |
| `uHash-512` | hash_4096 | 16756 | 1408 KiB | 1480 KiB |
| `uHash-512` | hash_8192 | 16756 | 1412 KiB | 1488 KiB |
| `uHash-512` | hash_16384 | 16756 | 1420 KiB | 1504 KiB |
| `uHash-512` | hash_65536 | 16756 | 1468 KiB | 1600 KiB |
| `uHash-768` | hash_32 | 16756 | 1404 KiB | 1472 KiB |
| `uHash-768` | hash_128 | 16756 | 1404 KiB | 1472 KiB |
| `uHash-768` | hash_512 | 16756 | 1404 KiB | 1472 KiB |
| `uHash-768` | hash_1024 | 16756 | 1408 KiB | 1476 KiB |
| `uHash-768` | hash_4096 | 16756 | 1408 KiB | 1480 KiB |
| `uHash-768` | hash_8192 | 16756 | 1412 KiB | 1488 KiB |
| `uHash-768` | hash_16384 | 16756 | 1420 KiB | 1504 KiB |
| `uHash-768` | hash_65536 | 16756 | 1468 KiB | 1600 KiB |
| `uHash-1024` | hash_32 | 16756 | 1404 KiB | 1472 KiB |
| `uHash-1024` | hash_128 | 16756 | 1404 KiB | 3516 KiB |
| `uHash-1024` | hash_512 | 16756 | 1404 KiB | 1476 KiB |
| `uHash-1024` | hash_1024 | 16756 | 1408 KiB | 1476 KiB |
| `uHash-1024` | hash_4096 | 16756 | 1408 KiB | 1480 KiB |
| `uHash-1024` | hash_8192 | 16756 | 1412 KiB | 1488 KiB |
| `uHash-1024` | hash_16384 | 16756 | 1416 KiB | 1500 KiB |
| `uHash-1024` | hash_65536 | 16756 | 1468 KiB | 1600 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `uHash-512` | KAT log (sha256 `b7d4c4787e42e33e…`) | `kat/hash-05/uHash-512.log` |
| `uHash-512` | timing hash_1024 | `records/hash-05/uHash-512__hash_1024.json` |
| `uHash-512` | timing hash_128 | `records/hash-05/uHash-512__hash_128.json` |
| `uHash-512` | timing hash_16384 | `records/hash-05/uHash-512__hash_16384.json` |
| `uHash-512` | timing hash_32 | `records/hash-05/uHash-512__hash_32.json` |
| `uHash-512` | timing hash_4096 | `records/hash-05/uHash-512__hash_4096.json` |
| `uHash-512` | timing hash_512 | `records/hash-05/uHash-512__hash_512.json` |
| `uHash-512` | timing hash_65536 | `records/hash-05/uHash-512__hash_65536.json` |
| `uHash-512` | timing hash_8192 | `records/hash-05/uHash-512__hash_8192.json` |
| `uHash-768` | KAT log (sha256 `9114a640daf9778b…`) | `kat/hash-05/uHash-768.log` |
| `uHash-768` | timing hash_1024 | `records/hash-05/uHash-768__hash_1024.json` |
| `uHash-768` | timing hash_128 | `records/hash-05/uHash-768__hash_128.json` |
| `uHash-768` | timing hash_16384 | `records/hash-05/uHash-768__hash_16384.json` |
| `uHash-768` | timing hash_32 | `records/hash-05/uHash-768__hash_32.json` |
| `uHash-768` | timing hash_4096 | `records/hash-05/uHash-768__hash_4096.json` |
| `uHash-768` | timing hash_512 | `records/hash-05/uHash-768__hash_512.json` |
| `uHash-768` | timing hash_65536 | `records/hash-05/uHash-768__hash_65536.json` |
| `uHash-768` | timing hash_8192 | `records/hash-05/uHash-768__hash_8192.json` |
| `uHash-1024` | KAT log (sha256 `17d5df0c84cc5f4e…`) | `kat/hash-05/uHash-1024.log` |
| `uHash-1024` | timing hash_1024 | `records/hash-05/uHash-1024__hash_1024.json` |
| `uHash-1024` | timing hash_128 | `records/hash-05/uHash-1024__hash_128.json` |
| `uHash-1024` | timing hash_16384 | `records/hash-05/uHash-1024__hash_16384.json` |
| `uHash-1024` | timing hash_32 | `records/hash-05/uHash-1024__hash_32.json` |
| `uHash-1024` | timing hash_4096 | `records/hash-05/uHash-1024__hash_4096.json` |
| `uHash-1024` | timing hash_512 | `records/hash-05/uHash-1024__hash_512.json` |
| `uHash-1024` | timing hash_65536 | `records/hash-05/uHash-1024__hash_65536.json` |
| `uHash-1024` | timing hash_8192 | `records/hash-05/uHash-1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

