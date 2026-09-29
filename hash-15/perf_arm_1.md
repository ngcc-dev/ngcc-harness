# hash-15 Litchi — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-15` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101538083238924288.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: Litchi
- Implementation versions measured: reference
- Parameter sets: `litchi_512`, `litchi_768`, `litchi_1024`, `litchi_xof`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-15/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `litchi_512` | guide | PASS |
| `litchi_768` | guide | PASS |
| `litchi_1024` | guide | PASS |
| `litchi_xof` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `litchi_512` | 32 B | 3061 | 95.7 | 1.14 µs | 28.1 | 100000 (5 × 20000) |
| `litchi_512` | 128 B | 6084 | 47.5 | 2.26 µs | 56.6 | 100000 (5 × 20000) |
| `litchi_512` | 512 B | 15.4 k | 30.0 | 5.71 µs | 89.7 | 100000 (5 × 20000) |
| `litchi_512` | 1024 B | 27.3 k | 26.7 | 10.1 µs | 100.9 | 100000 (5 × 20000) |
| `litchi_512` | 4096 B | 100.1 k | 24.4 | 37.1 µs | 110.3 | 82195 (5 × 16439) |
| `litchi_512` | 8192 B | 196.9 k | 24.0 | 73.1 µs | 112.1 | 42330 (5 × 8466) |
| `litchi_512` | 16384 B | 400.6 k | 24.4 | 149 µs | 110.2 | 21980 (5 × 4396) |
| `litchi_512` | 65536 B | 1.55 M | 23.7 | 576 µs | 113.8 | 5450 (5 × 1090) |
| `litchi_768` | 32 B | 3067 | 95.9 | 1.14 µs | 28.1 | 100000 (5 × 20000) |
| `litchi_768` | 128 B | 6078 | 47.5 | 2.26 µs | 56.7 | 100000 (5 × 20000) |
| `litchi_768` | 512 B | 18.2 k | 35.5 | 6.74 µs | 75.9 | 100000 (5 × 20000) |
| `litchi_768` | 1024 B | 33.3 k | 32.5 | 12.4 µs | 82.9 | 100000 (5 × 20000) |
| `litchi_768` | 4096 B | 129.7 k | 31.7 | 48.1 µs | 85.1 | 64780 (5 × 12956) |
| `litchi_768` | 8192 B | 259.1 k | 31.6 | 96.1 µs | 85.2 | 32415 (5 × 6483) |
| `litchi_768` | 16384 B | 515.4 k | 31.5 | 191 µs | 85.7 | 8535 (5 × 1707) |
| `litchi_768` | 65536 B | 2.06 M | 31.5 | 765 µs | 85.7 | 4210 (5 × 842) |
| `litchi_1024` | 32 B | 3042 | 95.1 | 1.13 µs | 28.3 | 100000 (5 × 20000) |
| `litchi_1024` | 128 B | 8959 | 70.0 | 3.33 µs | 38.5 | 100000 (5 × 20000) |
| `litchi_1024` | 512 B | 27.0 k | 52.7 | 10 µs | 51.1 | 100000 (5 × 20000) |
| `litchi_1024` | 1024 B | 50.5 k | 49.3 | 18.7 µs | 54.6 | 100000 (5 × 20000) |
| `litchi_1024` | 4096 B | 194.6 k | 47.5 | 72.2 µs | 56.7 | 43600 (5 × 8720) |
| `litchi_1024` | 8192 B | 381.9 k | 46.6 | 142 µs | 57.8 | 22320 (5 × 4464) |
| `litchi_1024` | 16384 B | 777.2 k | 47.4 | 288 µs | 56.8 | 11115 (5 × 2223) |
| `litchi_1024` | 65536 B | 3.06 M | 46.6 | 1.13 ms | 57.8 | 2815 (5 × 563) |
| `litchi_xof` | 32 B | 3054 | 95.5 | 1.14 µs | 28.2 | 100000 (5 × 20000) |
| `litchi_xof` | 128 B | 6029 | 47.1 | 2.24 µs | 57.2 | 100000 (5 × 20000) |
| `litchi_xof` | 512 B | 24.2 k | 47.3 | 8.99 µs | 57.0 | 100000 (5 × 20000) |
| `litchi_xof` | 1024 B | 45.2 k | 44.2 | 16.8 µs | 61.0 | 100000 (5 × 20000) |
| `litchi_xof` | 4096 B | 172.4 k | 42.1 | 64 µs | 64.0 | 48585 (5 × 9717) |
| `litchi_xof` | 8192 B | 341.4 k | 41.7 | 127 µs | 64.7 | 25080 (5 × 5016) |
| `litchi_xof` | 16384 B | 689.8 k | 42.1 | 256 µs | 64.0 | 12680 (5 × 2536) |
| `litchi_xof` | 65536 B | 2.75 M | 42.0 | 1.02 ms | 64.2 | 2685 (5 × 537) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `litchi_512` | hash_32 | 16648 | 1404 KiB | 1468 KiB |
| `litchi_512` | hash_128 | 16648 | 1404 KiB | 1468 KiB |
| `litchi_512` | hash_512 | 16648 | 1404 KiB | 1468 KiB |
| `litchi_512` | hash_1024 | 16648 | 1408 KiB | 1472 KiB |
| `litchi_512` | hash_4096 | 16648 | 1408 KiB | 1472 KiB |
| `litchi_512` | hash_8192 | 16648 | 1412 KiB | 1476 KiB |
| `litchi_512` | hash_16384 | 16648 | 1420 KiB | 1484 KiB |
| `litchi_512` | hash_65536 | 16648 | 1468 KiB | 1532 KiB |
| `litchi_768` | hash_32 | 15580 | 1400 KiB | 1464 KiB |
| `litchi_768` | hash_128 | 15580 | 1404 KiB | 1468 KiB |
| `litchi_768` | hash_512 | 15580 | 1404 KiB | 1468 KiB |
| `litchi_768` | hash_1024 | 15580 | 1408 KiB | 1472 KiB |
| `litchi_768` | hash_4096 | 15580 | 1408 KiB | 1472 KiB |
| `litchi_768` | hash_8192 | 15580 | 1412 KiB | 1476 KiB |
| `litchi_768` | hash_16384 | 15580 | 1420 KiB | 1484 KiB |
| `litchi_768` | hash_65536 | 15580 | 1468 KiB | 1532 KiB |
| `litchi_1024` | hash_32 | 16152 | 1404 KiB | 1468 KiB |
| `litchi_1024` | hash_128 | 16152 | 1400 KiB | 1464 KiB |
| `litchi_1024` | hash_512 | 16152 | 3440 KiB | 3504 KiB |
| `litchi_1024` | hash_1024 | 16152 | 1404 KiB | 1468 KiB |
| `litchi_1024` | hash_4096 | 16152 | 1408 KiB | 1472 KiB |
| `litchi_1024` | hash_8192 | 16152 | 1412 KiB | 1476 KiB |
| `litchi_1024` | hash_16384 | 16152 | 1420 KiB | 1484 KiB |
| `litchi_1024` | hash_65536 | 16152 | 1468 KiB | 1532 KiB |
| `litchi_xof` | hash_32 | 13116 | 1400 KiB | 1464 KiB |
| `litchi_xof` | hash_128 | 13116 | 1400 KiB | 1464 KiB |
| `litchi_xof` | hash_512 | 13116 | 1396 KiB | 1460 KiB |
| `litchi_xof` | hash_1024 | 13116 | 1404 KiB | 1468 KiB |
| `litchi_xof` | hash_4096 | 13116 | 1404 KiB | 1468 KiB |
| `litchi_xof` | hash_8192 | 13116 | 1408 KiB | 1472 KiB |
| `litchi_xof` | hash_16384 | 13116 | 3448 KiB | 3512 KiB |
| `litchi_xof` | hash_65536 | 13116 | 1464 KiB | 1528 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `litchi_512` | KAT log (sha256 `c5bd8caeb1f991d0…`) | `kat/hash-15/litchi_512.log` |
| `litchi_512` | timing hash_1024 | `records/hash-15/litchi_512__hash_1024.json` |
| `litchi_512` | timing hash_128 | `records/hash-15/litchi_512__hash_128.json` |
| `litchi_512` | timing hash_16384 | `records/hash-15/litchi_512__hash_16384.json` |
| `litchi_512` | timing hash_32 | `records/hash-15/litchi_512__hash_32.json` |
| `litchi_512` | timing hash_4096 | `records/hash-15/litchi_512__hash_4096.json` |
| `litchi_512` | timing hash_512 | `records/hash-15/litchi_512__hash_512.json` |
| `litchi_512` | timing hash_65536 | `records/hash-15/litchi_512__hash_65536.json` |
| `litchi_512` | timing hash_8192 | `records/hash-15/litchi_512__hash_8192.json` |
| `litchi_768` | KAT log (sha256 `3cc212c30f16bbcd…`) | `kat/hash-15/litchi_768.log` |
| `litchi_768` | timing hash_1024 | `records/hash-15/litchi_768__hash_1024.json` |
| `litchi_768` | timing hash_128 | `records/hash-15/litchi_768__hash_128.json` |
| `litchi_768` | timing hash_16384 | `records/hash-15/litchi_768__hash_16384.json` |
| `litchi_768` | timing hash_32 | `records/hash-15/litchi_768__hash_32.json` |
| `litchi_768` | timing hash_4096 | `records/hash-15/litchi_768__hash_4096.json` |
| `litchi_768` | timing hash_512 | `records/hash-15/litchi_768__hash_512.json` |
| `litchi_768` | timing hash_65536 | `records/hash-15/litchi_768__hash_65536.json` |
| `litchi_768` | timing hash_8192 | `records/hash-15/litchi_768__hash_8192.json` |
| `litchi_1024` | KAT log (sha256 `28667c9a4b2ca22d…`) | `kat/hash-15/litchi_1024.log` |
| `litchi_1024` | timing hash_1024 | `records/hash-15/litchi_1024__hash_1024.json` |
| `litchi_1024` | timing hash_128 | `records/hash-15/litchi_1024__hash_128.json` |
| `litchi_1024` | timing hash_16384 | `records/hash-15/litchi_1024__hash_16384.json` |
| `litchi_1024` | timing hash_32 | `records/hash-15/litchi_1024__hash_32.json` |
| `litchi_1024` | timing hash_4096 | `records/hash-15/litchi_1024__hash_4096.json` |
| `litchi_1024` | timing hash_512 | `records/hash-15/litchi_1024__hash_512.json` |
| `litchi_1024` | timing hash_65536 | `records/hash-15/litchi_1024__hash_65536.json` |
| `litchi_1024` | timing hash_8192 | `records/hash-15/litchi_1024__hash_8192.json` |
| `litchi_xof` | KAT log (sha256 `2747759c349e881a…`) | `kat/hash-15/litchi_xof.log` |
| `litchi_xof` | timing hash_1024 | `records/hash-15/litchi_xof__hash_1024.json` |
| `litchi_xof` | timing hash_128 | `records/hash-15/litchi_xof__hash_128.json` |
| `litchi_xof` | timing hash_16384 | `records/hash-15/litchi_xof__hash_16384.json` |
| `litchi_xof` | timing hash_32 | `records/hash-15/litchi_xof__hash_32.json` |
| `litchi_xof` | timing hash_4096 | `records/hash-15/litchi_xof__hash_4096.json` |
| `litchi_xof` | timing hash_512 | `records/hash-15/litchi_xof__hash_512.json` |
| `litchi_xof` | timing hash_65536 | `records/hash-15/litchi_xof__hash_65536.json` |
| `litchi_xof` | timing hash_8192 | `records/hash-15/litchi_xof__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

