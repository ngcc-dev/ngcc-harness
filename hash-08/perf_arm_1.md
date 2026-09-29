# hash-08 Duet — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-08` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101539583977672704.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: Duet
- Implementation versions measured: reference
- Parameter sets: `Duet-512`, `Duet-768`, `Duet-1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-08/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Duet-512` | guide | PASS |
| `Duet-768` | guide | PASS |
| `Duet-1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `Duet-512` | 32 B | 4034 | 126.1 | 1.5 µs | 21.3 | 100000 (5 × 20000) |
| `Duet-512` | 128 B | 12.0 k | 93.6 | 4.45 µs | 28.8 | 100000 (5 × 20000) |
| `Duet-512` | 512 B | 43.8 k | 85.5 | 16.2 µs | 31.5 | 100000 (5 × 20000) |
| `Duet-512` | 1024 B | 87.4 k | 85.3 | 32.4 µs | 31.6 | 90910 (5 × 18182) |
| `Duet-512` | 4096 B | 341.8 k | 83.4 | 127 µs | 32.3 | 24505 (5 × 4901) |
| `Duet-512` | 8192 B | 692.4 k | 84.5 | 257 µs | 31.9 | 12430 (5 × 2486) |
| `Duet-512` | 16384 B | 1.36 M | 82.8 | 503 µs | 32.6 | 6060 (5 × 1212) |
| `Duet-512` | 65536 B | 5.43 M | 82.8 | 2.01 ms | 32.6 | 1570 (5 × 314) |
| `Duet-768` | 32 B | 14.3 k | 445.7 | 5.29 µs | 6.0 | 100000 (5 × 20000) |
| `Duet-768` | 128 B | 14.2 k | 111.3 | 5.29 µs | 24.2 | 100000 (5 × 20000) |
| `Duet-768` | 512 B | 57.2 k | 111.6 | 21.2 µs | 24.1 | 100000 (5 × 20000) |
| `Duet-768` | 1024 B | 113.6 k | 111.0 | 42.2 µs | 24.3 | 69270 (5 × 13854) |
| `Duet-768` | 4096 B | 439.4 k | 107.3 | 163 µs | 25.1 | 19185 (5 × 3837) |
| `Duet-768` | 8192 B | 868.8 k | 106.1 | 322 µs | 25.4 | 9810 (5 × 1962) |
| `Duet-768` | 16384 B | 1.72 M | 104.7 | 636 µs | 25.8 | 4945 (5 × 989) |
| `Duet-768` | 65536 B | 6.88 M | 105.1 | 2.55 ms | 25.7 | 1215 (5 × 243) |
| `Duet-1024` | 32 B | 14.4 k | 450.0 | 5.34 µs | 6.0 | 100000 (5 × 20000) |
| `Duet-1024` | 128 B | 28.4 k | 222.3 | 10.6 µs | 12.1 | 100000 (5 × 20000) |
| `Duet-1024` | 512 B | 85.6 k | 167.1 | 31.8 µs | 16.1 | 94120 (5 × 18824) |
| `Duet-1024` | 1024 B | 156.8 k | 153.2 | 58.2 µs | 17.6 | 53100 (5 × 10620) |
| `Duet-1024` | 4096 B | 609.4 k | 148.8 | 226 µs | 18.1 | 13895 (5 × 2779) |
| `Duet-1024` | 8192 B | 1.23 M | 149.7 | 455 µs | 18.0 | 6985 (5 × 1397) |
| `Duet-1024` | 16384 B | 2.48 M | 151.5 | 921 µs | 17.8 | 3490 (5 × 698) |
| `Duet-1024` | 65536 B | 9.68 M | 147.7 | 3.59 ms | 18.3 | 840 (5 × 168) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Duet-512` | hash_32 | 12884 | 1400 KiB | 1464 KiB |
| `Duet-512` | hash_128 | 12884 | 1400 KiB | 1464 KiB |
| `Duet-512` | hash_512 | 12884 | 1400 KiB | 1464 KiB |
| `Duet-512` | hash_1024 | 12884 | 1404 KiB | 1468 KiB |
| `Duet-512` | hash_4096 | 12884 | 1404 KiB | 1468 KiB |
| `Duet-512` | hash_8192 | 12884 | 1408 KiB | 1472 KiB |
| `Duet-512` | hash_16384 | 12884 | 1416 KiB | 1480 KiB |
| `Duet-512` | hash_65536 | 12884 | 1464 KiB | 1528 KiB |
| `Duet-768` | hash_32 | 13500 | 1400 KiB | 1464 KiB |
| `Duet-768` | hash_128 | 13500 | 1400 KiB | 1464 KiB |
| `Duet-768` | hash_512 | 13500 | 1400 KiB | 1464 KiB |
| `Duet-768` | hash_1024 | 13500 | 1404 KiB | 1468 KiB |
| `Duet-768` | hash_4096 | 13500 | 1404 KiB | 1468 KiB |
| `Duet-768` | hash_8192 | 13500 | 1408 KiB | 1472 KiB |
| `Duet-768` | hash_16384 | 13500 | 1416 KiB | 1480 KiB |
| `Duet-768` | hash_65536 | 13500 | 1464 KiB | 1528 KiB |
| `Duet-1024` | hash_32 | 14164 | 1404 KiB | 1468 KiB |
| `Duet-1024` | hash_128 | 14164 | 1404 KiB | 1468 KiB |
| `Duet-1024` | hash_512 | 14164 | 1404 KiB | 1468 KiB |
| `Duet-1024` | hash_1024 | 14164 | 1408 KiB | 1472 KiB |
| `Duet-1024` | hash_4096 | 14164 | 1408 KiB | 1472 KiB |
| `Duet-1024` | hash_8192 | 14164 | 1412 KiB | 1476 KiB |
| `Duet-1024` | hash_16384 | 14164 | 1420 KiB | 1484 KiB |
| `Duet-1024` | hash_65536 | 14164 | 1468 KiB | 1532 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Duet-512` | KAT log (sha256 `098cbd7321ce065e…`) | `kat/hash-08/Duet-512.log` |
| `Duet-512` | timing hash_1024 | `records/hash-08/Duet-512__hash_1024.json` |
| `Duet-512` | timing hash_128 | `records/hash-08/Duet-512__hash_128.json` |
| `Duet-512` | timing hash_16384 | `records/hash-08/Duet-512__hash_16384.json` |
| `Duet-512` | timing hash_32 | `records/hash-08/Duet-512__hash_32.json` |
| `Duet-512` | timing hash_4096 | `records/hash-08/Duet-512__hash_4096.json` |
| `Duet-512` | timing hash_512 | `records/hash-08/Duet-512__hash_512.json` |
| `Duet-512` | timing hash_65536 | `records/hash-08/Duet-512__hash_65536.json` |
| `Duet-512` | timing hash_8192 | `records/hash-08/Duet-512__hash_8192.json` |
| `Duet-768` | KAT log (sha256 `91f8c69b051a42a8…`) | `kat/hash-08/Duet-768.log` |
| `Duet-768` | timing hash_1024 | `records/hash-08/Duet-768__hash_1024.json` |
| `Duet-768` | timing hash_128 | `records/hash-08/Duet-768__hash_128.json` |
| `Duet-768` | timing hash_16384 | `records/hash-08/Duet-768__hash_16384.json` |
| `Duet-768` | timing hash_32 | `records/hash-08/Duet-768__hash_32.json` |
| `Duet-768` | timing hash_4096 | `records/hash-08/Duet-768__hash_4096.json` |
| `Duet-768` | timing hash_512 | `records/hash-08/Duet-768__hash_512.json` |
| `Duet-768` | timing hash_65536 | `records/hash-08/Duet-768__hash_65536.json` |
| `Duet-768` | timing hash_8192 | `records/hash-08/Duet-768__hash_8192.json` |
| `Duet-1024` | KAT log (sha256 `0df3b189acc23d1a…`) | `kat/hash-08/Duet-1024.log` |
| `Duet-1024` | timing hash_1024 | `records/hash-08/Duet-1024__hash_1024.json` |
| `Duet-1024` | timing hash_128 | `records/hash-08/Duet-1024__hash_128.json` |
| `Duet-1024` | timing hash_16384 | `records/hash-08/Duet-1024__hash_16384.json` |
| `Duet-1024` | timing hash_32 | `records/hash-08/Duet-1024__hash_32.json` |
| `Duet-1024` | timing hash_4096 | `records/hash-08/Duet-1024__hash_4096.json` |
| `Duet-1024` | timing hash_512 | `records/hash-08/Duet-1024__hash_512.json` |
| `Duet-1024` | timing hash_65536 | `records/hash-08/Duet-1024__hash_65536.json` |
| `Duet-1024` | timing hash_8192 | `records/hash-08/Duet-1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

