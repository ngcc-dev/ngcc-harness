# hash-35 Wish Hash Function — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-35` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101533368094642176.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: Wish Hash Function
- Implementation versions measured: reference
- Parameter sets: `Wish512`, `Wish1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-35/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Wish512` | guide | PASS |
| `Wish1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `Wish512` | 32 B | 208.6 k | 6517.5 | 77.4 µs | 0.4 | 32745 (5 × 6549) |
| `Wish512` | 128 B | 707.6 k | 5528.1 | 263 µs | 0.5 | 11330 (5 × 2266) |
| `Wish512` | 512 B | 2.16 M | 4228.1 | 803 µs | 0.6 | 3870 (5 × 774) |
| `Wish512` | 1024 B | 4.10 M | 4002.9 | 1.52 ms | 0.7 | 2045 (5 × 409) |
| `Wish512` | 4096 B | 15.68 M | 3828.8 | 5.82 ms | 0.7 | 545 (5 × 109) |
| `Wish512` | 8192 B | 31.17 M | 3805.1 | 11.6 ms | 0.7 | 270 (5 × 54) |
| `Wish512` | 16384 B | 62.09 M | 3789.5 | 23 ms | 0.7 | 140 (5 × 28) |
| `Wish512` | 65536 B | 247.65 M | 3778.8 | 91.9 ms | 0.7 | 100 (5 × 20) |
| `Wish1024` | 32 B | 608.8 k | 19024.3 | 226 µs | 0.1 | 12915 (5 × 2583) |
| `Wish1024` | 128 B | 1.27 M | 9925.1 | 471 µs | 0.3 | 6420 (5 × 1284) |
| `Wish1024` | 512 B | 3.19 M | 6228.9 | 1.18 ms | 0.4 | 2630 (5 × 526) |
| `Wish1024` | 1024 B | 5.76 M | 5624.5 | 2.14 ms | 0.5 | 1435 (5 × 287) |
| `Wish1024` | 4096 B | 21.16 M | 5166.0 | 7.85 ms | 0.5 | 405 (5 × 81) |
| `Wish1024` | 8192 B | 41.88 M | 5112.4 | 15.5 ms | 0.5 | 205 (5 × 41) |
| `Wish1024` | 16384 B | 82.80 M | 5053.7 | 30.7 ms | 0.5 | 105 (5 × 21) |
| `Wish1024` | 65536 B | 329.38 M | 5025.9 | 122 ms | 0.5 | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Wish512` | hash_32 | 17624 | 1404 KiB | 1468 KiB |
| `Wish512` | hash_128 | 17624 | 1404 KiB | 1468 KiB |
| `Wish512` | hash_512 | 17624 | 1404 KiB | 1472 KiB |
| `Wish512` | hash_1024 | 17624 | 1408 KiB | 1472 KiB |
| `Wish512` | hash_4096 | 17624 | 1408 KiB | 1476 KiB |
| `Wish512` | hash_8192 | 17624 | 1412 KiB | 1484 KiB |
| `Wish512` | hash_16384 | 17624 | 3460 KiB | 3524 KiB |
| `Wish512` | hash_65536 | 17624 | 1464 KiB | 3620 KiB |
| `Wish1024` | hash_32 | 17624 | 1404 KiB | 1472 KiB |
| `Wish1024` | hash_128 | 17624 | 1404 KiB | 1472 KiB |
| `Wish1024` | hash_512 | 17624 | 1404 KiB | 1472 KiB |
| `Wish1024` | hash_1024 | 17624 | 1404 KiB | 1468 KiB |
| `Wish1024` | hash_4096 | 17624 | 1408 KiB | 1480 KiB |
| `Wish1024` | hash_8192 | 17624 | 1408 KiB | 1484 KiB |
| `Wish1024` | hash_16384 | 17624 | 1420 KiB | 1504 KiB |
| `Wish1024` | hash_65536 | 17624 | 3436 KiB | 3500 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Wish512` | KAT log (sha256 `92385670a4469f23…`) | `kat/hash-35/Wish512.log` |
| `Wish512` | timing hash_1024 | `records/hash-35/Wish512__hash_1024.json` |
| `Wish512` | timing hash_128 | `records/hash-35/Wish512__hash_128.json` |
| `Wish512` | timing hash_16384 | `records/hash-35/Wish512__hash_16384.json` |
| `Wish512` | timing hash_32 | `records/hash-35/Wish512__hash_32.json` |
| `Wish512` | timing hash_4096 | `records/hash-35/Wish512__hash_4096.json` |
| `Wish512` | timing hash_512 | `records/hash-35/Wish512__hash_512.json` |
| `Wish512` | timing hash_65536 | `records/hash-35/Wish512__hash_65536.json` |
| `Wish512` | timing hash_8192 | `records/hash-35/Wish512__hash_8192.json` |
| `Wish1024` | KAT log (sha256 `82944577edf2bb91…`) | `kat/hash-35/Wish1024.log` |
| `Wish1024` | timing hash_1024 | `records/hash-35/Wish1024__hash_1024.json` |
| `Wish1024` | timing hash_128 | `records/hash-35/Wish1024__hash_128.json` |
| `Wish1024` | timing hash_16384 | `records/hash-35/Wish1024__hash_16384.json` |
| `Wish1024` | timing hash_32 | `records/hash-35/Wish1024__hash_32.json` |
| `Wish1024` | timing hash_4096 | `records/hash-35/Wish1024__hash_4096.json` |
| `Wish1024` | timing hash_512 | `records/hash-35/Wish1024__hash_512.json` |
| `Wish1024` | timing hash_65536 | `records/hash-35/Wish1024__hash_65536.json` |
| `Wish1024` | timing hash_8192 | `records/hash-35/Wish1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

