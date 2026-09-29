# hash-26 The Hash Function CHIME — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-26` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101535839617634304.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: The Hash Function CHIME
- Implementation versions measured: reference
- Parameter sets: `CHIME-512`, `CHIME-1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-26/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `CHIME-512` | guide | PASS |
| `CHIME-1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `CHIME-512` | 32 B | 1690 | 52.8 | 629 ns | 50.9 | 100000 (5 × 20000) |
| `CHIME-512` | 128 B | 5404 | 42.2 | 2.01 µs | 63.8 | 100000 (5 × 20000) |
| `CHIME-512` | 512 B | 19.2 k | 37.4 | 7.11 µs | 72.0 | 100000 (5 × 20000) |
| `CHIME-512` | 1024 B | 32.1 k | 31.3 | 11.9 µs | 86.0 | 100000 (5 × 20000) |
| `CHIME-512` | 4096 B | 140.2 k | 34.2 | 52 µs | 78.7 | 74770 (5 × 14954) |
| `CHIME-512` | 8192 B | 247.9 k | 30.3 | 92 µs | 89.1 | 38835 (5 × 7767) |
| `CHIME-512` | 16384 B | 489.4 k | 29.9 | 182 µs | 90.2 | 17415 (5 × 3483) |
| `CHIME-512` | 65536 B | 1.96 M | 29.9 | 727 µs | 90.2 | 4255 (5 × 851) |
| `CHIME-1024` | 32 B | 2237 | 69.9 | 832 ns | 38.4 | 100000 (5 × 20000) |
| `CHIME-1024` | 128 B | 7206 | 56.3 | 2.68 µs | 47.8 | 100000 (5 × 20000) |
| `CHIME-1024` | 512 B | 23.4 k | 45.8 | 8.7 µs | 58.9 | 100000 (5 × 20000) |
| `CHIME-1024` | 1024 B | 44.7 k | 43.6 | 16.6 µs | 61.8 | 100000 (5 × 20000) |
| `CHIME-1024` | 4096 B | 171.8 k | 41.9 | 63.7 µs | 64.3 | 48835 (5 × 9767) |
| `CHIME-1024` | 8192 B | 339.5 k | 41.4 | 126 µs | 65.0 | 24540 (5 × 4908) |
| `CHIME-1024` | 16384 B | 743.7 k | 45.4 | 276 µs | 59.4 | 11880 (5 × 2376) |
| `CHIME-1024` | 65536 B | 2.67 M | 40.7 | 991 µs | 66.1 | 3200 (5 × 640) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `CHIME-512` | hash_32 | 12108 | 1400 KiB | 1464 KiB |
| `CHIME-512` | hash_128 | 12108 | 1400 KiB | 1464 KiB |
| `CHIME-512` | hash_512 | 12108 | 1396 KiB | 1464 KiB |
| `CHIME-512` | hash_1024 | 12108 | 1404 KiB | 1468 KiB |
| `CHIME-512` | hash_4096 | 12108 | 1400 KiB | 1468 KiB |
| `CHIME-512` | hash_8192 | 12108 | 3420 KiB | 3484 KiB |
| `CHIME-512` | hash_16384 | 12108 | 1416 KiB | 1496 KiB |
| `CHIME-512` | hash_65536 | 12108 | 1460 KiB | 1588 KiB |
| `CHIME-1024` | hash_32 | 12404 | 1400 KiB | 1464 KiB |
| `CHIME-1024` | hash_128 | 12404 | 1400 KiB | 1464 KiB |
| `CHIME-1024` | hash_512 | 12404 | 1400 KiB | 1468 KiB |
| `CHIME-1024` | hash_1024 | 12404 | 1404 KiB | 1468 KiB |
| `CHIME-1024` | hash_4096 | 12404 | 1400 KiB | 1468 KiB |
| `CHIME-1024` | hash_8192 | 12404 | 1408 KiB | 1484 KiB |
| `CHIME-1024` | hash_16384 | 12404 | 1416 KiB | 1500 KiB |
| `CHIME-1024` | hash_65536 | 12404 | 1464 KiB | 1604 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `CHIME-512` | KAT log (sha256 `9f5443bb43a1bce7…`) | `kat/hash-26/CHIME-512.log` |
| `CHIME-512` | timing hash_1024 | `records/hash-26/CHIME-512__hash_1024.json` |
| `CHIME-512` | timing hash_128 | `records/hash-26/CHIME-512__hash_128.json` |
| `CHIME-512` | timing hash_16384 | `records/hash-26/CHIME-512__hash_16384.json` |
| `CHIME-512` | timing hash_32 | `records/hash-26/CHIME-512__hash_32.json` |
| `CHIME-512` | timing hash_4096 | `records/hash-26/CHIME-512__hash_4096.json` |
| `CHIME-512` | timing hash_512 | `records/hash-26/CHIME-512__hash_512.json` |
| `CHIME-512` | timing hash_65536 | `records/hash-26/CHIME-512__hash_65536.json` |
| `CHIME-512` | timing hash_8192 | `records/hash-26/CHIME-512__hash_8192.json` |
| `CHIME-1024` | KAT log (sha256 `7f5dbe036176b9fb…`) | `kat/hash-26/CHIME-1024.log` |
| `CHIME-1024` | timing hash_1024 | `records/hash-26/CHIME-1024__hash_1024.json` |
| `CHIME-1024` | timing hash_128 | `records/hash-26/CHIME-1024__hash_128.json` |
| `CHIME-1024` | timing hash_16384 | `records/hash-26/CHIME-1024__hash_16384.json` |
| `CHIME-1024` | timing hash_32 | `records/hash-26/CHIME-1024__hash_32.json` |
| `CHIME-1024` | timing hash_4096 | `records/hash-26/CHIME-1024__hash_4096.json` |
| `CHIME-1024` | timing hash_512 | `records/hash-26/CHIME-1024__hash_512.json` |
| `CHIME-1024` | timing hash_65536 | `records/hash-26/CHIME-1024__hash_65536.json` |
| `CHIME-1024` | timing hash_8192 | `records/hash-26/CHIME-1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

