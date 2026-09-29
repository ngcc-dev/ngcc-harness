# hash-32 The ZC-EDMC Hash Function — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-32` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101534201922277376.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: The ZC-EDMC Hash Function
- Implementation versions measured: reference
- Parameter sets: `ZC-EDMC-1280-512`, `ZC-EDMC-1280-768`, `ZC-EDMC-1280-1024`, `ZC-EDMC-1536-512`, `ZC-EDMC-1536-768`, `ZC-EDMC-1536-1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-32/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `ZC-EDMC-1280-512` | guide | PASS |
| `ZC-EDMC-1280-768` | guide | PASS |
| `ZC-EDMC-1280-1024` | guide | PASS |
| `ZC-EDMC-1536-512` | guide | PASS |
| `ZC-EDMC-1536-768` | guide | PASS |
| `ZC-EDMC-1536-1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `ZC-EDMC-1280-512` | 32 B | 513 | 16.0 | 191 ns | 167.3 | 100000 (5 × 20000) |
| `ZC-EDMC-1280-512` | 128 B | 965 | 7.5 | 360 ns | 355.1 | 100000 (5 × 20000) |
| `ZC-EDMC-1280-512` | 512 B | 2792 | 5.5 | 1.04 µs | 493.0 | 100000 (5 × 20000) |
| `ZC-EDMC-1280-512` | 1024 B | 5535 | 5.4 | 2.06 µs | 497.9 | 100000 (5 × 20000) |
| `ZC-EDMC-1280-512` | 4096 B | 21.5 k | 5.2 | 7.98 µs | 513.4 | 100000 (5 × 20000) |
| `ZC-EDMC-1280-512` | 8192 B | 43.0 k | 5.3 | 16 µs | 513.1 | 100000 (5 × 20000) |
| `ZC-EDMC-1280-512` | 16384 B | 85.5 k | 5.2 | 31.8 µs | 515.7 | 94490 (5 × 18898) |
| `ZC-EDMC-1280-512` | 65536 B | 340.4 k | 5.2 | 126 µs | 518.7 | 24630 (5 × 4926) |
| `ZC-EDMC-1280-768` | 32 B | 913 | 28.5 | 340 ns | 94.1 | 100000 (5 × 20000) |
| `ZC-EDMC-1280-768` | 128 B | 1828 | 14.3 | 680 ns | 188.2 | 100000 (5 × 20000) |
| `ZC-EDMC-1280-768` | 512 B | 5053 | 9.9 | 1.88 µs | 272.8 | 100000 (5 × 20000) |
| `ZC-EDMC-1280-768` | 1024 B | 9203 | 9.0 | 3.42 µs | 299.7 | 100000 (5 × 20000) |
| `ZC-EDMC-1280-768` | 4096 B | 34.6 k | 8.5 | 12.9 µs | 318.7 | 100000 (5 × 20000) |
| `ZC-EDMC-1280-768` | 8192 B | 68.2 k | 8.3 | 25.3 µs | 323.8 | 100000 (5 × 20000) |
| `ZC-EDMC-1280-768` | 16384 B | 178.9 k | 10.9 | 66.4 µs | 246.8 | 51785 (5 × 10357) |
| `ZC-EDMC-1280-768` | 65536 B | 539.7 k | 8.2 | 200 µs | 327.2 | 15110 (5 × 3022) |
| `ZC-EDMC-1280-1024` | 32 B | 3891 | 121.6 | 1.45 µs | 22.1 | 100000 (5 × 20000) |
| `ZC-EDMC-1280-1024` | 128 B | 4821 | 37.7 | 1.79 µs | 71.4 | 100000 (5 × 20000) |
| `ZC-EDMC-1280-1024` | 512 B | 16.4 k | 32.0 | 6.08 µs | 84.2 | 100000 (5 × 20000) |
| `ZC-EDMC-1280-1024` | 1024 B | 21.9 k | 21.4 | 8.13 µs | 126.0 | 100000 (5 × 20000) |
| `ZC-EDMC-1280-1024` | 4096 B | 81.2 k | 19.8 | 30.2 µs | 135.8 | 100000 (5 × 20000) |
| `ZC-EDMC-1280-1024` | 8192 B | 160.0 k | 19.5 | 59.4 µs | 137.9 | 52575 (5 × 10515) |
| `ZC-EDMC-1280-1024` | 16384 B | 394.3 k | 24.1 | 146 µs | 112.0 | 26190 (5 × 5238) |
| `ZC-EDMC-1280-1024` | 65536 B | 1.26 M | 19.2 | 468 µs | 140.1 | 6775 (5 × 1355) |
| `ZC-EDMC-1536-512` | 32 B | 946 | 29.6 | 353 ns | 90.7 | 100000 (5 × 20000) |
| `ZC-EDMC-1536-512` | 128 B | 1798 | 14.0 | 669 ns | 191.2 | 100000 (5 × 20000) |
| `ZC-EDMC-1536-512` | 512 B | 4374 | 8.5 | 1.63 µs | 315.0 | 100000 (5 × 20000) |
| `ZC-EDMC-1536-512` | 1024 B | 7811 | 7.6 | 2.9 µs | 353.0 | 100000 (5 × 20000) |
| `ZC-EDMC-1536-512` | 4096 B | 30.2 k | 7.4 | 11.2 µs | 365.7 | 100000 (5 × 20000) |
| `ZC-EDMC-1536-512` | 8192 B | 59.3 k | 7.2 | 22 µs | 372.0 | 100000 (5 × 20000) |
| `ZC-EDMC-1536-512` | 16384 B | 195.8 k | 12.0 | 72.7 µs | 225.3 | 68575 (5 × 13715) |
| `ZC-EDMC-1536-512` | 65536 B | 474.1 k | 7.2 | 176 µs | 372.1 | 18020 (5 × 3604) |
| `ZC-EDMC-1536-768` | 32 B | 2379 | 74.3 | 885 ns | 36.2 | 100000 (5 × 20000) |
| `ZC-EDMC-1536-768` | 128 B | 2642 | 20.6 | 983 ns | 130.3 | 100000 (5 × 20000) |
| `ZC-EDMC-1536-768` | 512 B | 6103 | 11.9 | 2.27 µs | 225.8 | 100000 (5 × 20000) |
| `ZC-EDMC-1536-768` | 1024 B | 19.1 k | 18.6 | 7.09 µs | 144.4 | 100000 (5 × 20000) |
| `ZC-EDMC-1536-768` | 4096 B | 41.2 k | 10.1 | 15.3 µs | 267.6 | 100000 (5 × 20000) |
| `ZC-EDMC-1536-768` | 8192 B | 108.8 k | 13.3 | 40.4 µs | 202.8 | 96390 (5 × 19278) |
| `ZC-EDMC-1536-768` | 16384 B | 161.0 k | 9.8 | 59.8 µs | 274.0 | 50055 (5 × 10011) |
| `ZC-EDMC-1536-768` | 65536 B | 1.10 M | 16.8 | 410 µs | 160.0 | 13380 (5 × 2676) |
| `ZC-EDMC-1536-1024` | 32 B | 2662 | 83.2 | 990 ns | 32.3 | 100000 (5 × 20000) |
| `ZC-EDMC-1536-1024` | 128 B | 4368 | 34.1 | 1.62 µs | 78.9 | 100000 (5 × 20000) |
| `ZC-EDMC-1536-1024` | 512 B | 10.4 k | 20.2 | 3.84 µs | 133.2 | 100000 (5 × 20000) |
| `ZC-EDMC-1536-1024` | 1024 B | 18.0 k | 17.6 | 6.69 µs | 153.0 | 100000 (5 × 20000) |
| `ZC-EDMC-1536-1024` | 4096 B | 65.0 k | 15.9 | 24.1 µs | 170.0 | 100000 (5 × 20000) |
| `ZC-EDMC-1536-1024` | 8192 B | 127.4 k | 15.5 | 47.3 µs | 173.2 | 62585 (5 × 12517) |
| `ZC-EDMC-1536-1024` | 16384 B | 251.6 k | 15.4 | 93.3 µs | 175.5 | 33130 (5 × 6626) |
| `ZC-EDMC-1536-1024` | 65536 B | 1.76 M | 26.8 | 653 µs | 100.4 | 8505 (5 × 1701) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `ZC-EDMC-1280-512` | hash_32 | 12680 | 1400 KiB | 1464 KiB |
| `ZC-EDMC-1280-512` | hash_128 | 12680 | 1400 KiB | 1464 KiB |
| `ZC-EDMC-1280-512` | hash_512 | 12680 | 1400 KiB | 1464 KiB |
| `ZC-EDMC-1280-512` | hash_1024 | 12680 | 1404 KiB | 1468 KiB |
| `ZC-EDMC-1280-512` | hash_4096 | 12680 | 1404 KiB | 1468 KiB |
| `ZC-EDMC-1280-512` | hash_8192 | 12680 | 1408 KiB | 1472 KiB |
| `ZC-EDMC-1280-512` | hash_16384 | 12680 | 1416 KiB | 1480 KiB |
| `ZC-EDMC-1280-512` | hash_65536 | 12680 | 1464 KiB | 1528 KiB |
| `ZC-EDMC-1280-768` | hash_32 | 12680 | 1400 KiB | 1464 KiB |
| `ZC-EDMC-1280-768` | hash_128 | 12680 | 1400 KiB | 1464 KiB |
| `ZC-EDMC-1280-768` | hash_512 | 12680 | 1400 KiB | 1464 KiB |
| `ZC-EDMC-1280-768` | hash_1024 | 12680 | 1400 KiB | 1464 KiB |
| `ZC-EDMC-1280-768` | hash_4096 | 12680 | 1404 KiB | 1468 KiB |
| `ZC-EDMC-1280-768` | hash_8192 | 12680 | 1408 KiB | 1472 KiB |
| `ZC-EDMC-1280-768` | hash_16384 | 12680 | 1416 KiB | 1480 KiB |
| `ZC-EDMC-1280-768` | hash_65536 | 12680 | 1464 KiB | 1528 KiB |
| `ZC-EDMC-1280-1024` | hash_32 | 12680 | 1400 KiB | 1464 KiB |
| `ZC-EDMC-1280-1024` | hash_128 | 12680 | 1400 KiB | 1464 KiB |
| `ZC-EDMC-1280-1024` | hash_512 | 12680 | 1400 KiB | 1464 KiB |
| `ZC-EDMC-1280-1024` | hash_1024 | 12680 | 1404 KiB | 1468 KiB |
| `ZC-EDMC-1280-1024` | hash_4096 | 12680 | 1404 KiB | 1468 KiB |
| `ZC-EDMC-1280-1024` | hash_8192 | 12680 | 1404 KiB | 1468 KiB |
| `ZC-EDMC-1280-1024` | hash_16384 | 12680 | 1416 KiB | 1480 KiB |
| `ZC-EDMC-1280-1024` | hash_65536 | 12680 | 1464 KiB | 1528 KiB |
| `ZC-EDMC-1536-512` | hash_32 | 16704 | 1404 KiB | 1468 KiB |
| `ZC-EDMC-1536-512` | hash_128 | 16704 | 1404 KiB | 1468 KiB |
| `ZC-EDMC-1536-512` | hash_512 | 16704 | 1404 KiB | 1468 KiB |
| `ZC-EDMC-1536-512` | hash_1024 | 16704 | 1408 KiB | 1472 KiB |
| `ZC-EDMC-1536-512` | hash_4096 | 16704 | 1404 KiB | 1468 KiB |
| `ZC-EDMC-1536-512` | hash_8192 | 16704 | 1412 KiB | 1476 KiB |
| `ZC-EDMC-1536-512` | hash_16384 | 16704 | 1420 KiB | 1484 KiB |
| `ZC-EDMC-1536-512` | hash_65536 | 16704 | 1468 KiB | 1532 KiB |
| `ZC-EDMC-1536-768` | hash_32 | 16704 | 1404 KiB | 1468 KiB |
| `ZC-EDMC-1536-768` | hash_128 | 16704 | 1404 KiB | 1468 KiB |
| `ZC-EDMC-1536-768` | hash_512 | 16704 | 1404 KiB | 1468 KiB |
| `ZC-EDMC-1536-768` | hash_1024 | 16704 | 1408 KiB | 1472 KiB |
| `ZC-EDMC-1536-768` | hash_4096 | 16704 | 1408 KiB | 1472 KiB |
| `ZC-EDMC-1536-768` | hash_8192 | 16704 | 1412 KiB | 1476 KiB |
| `ZC-EDMC-1536-768` | hash_16384 | 16704 | 1420 KiB | 1484 KiB |
| `ZC-EDMC-1536-768` | hash_65536 | 16704 | 3476 KiB | 3540 KiB |
| `ZC-EDMC-1536-1024` | hash_32 | 16704 | 1404 KiB | 1468 KiB |
| `ZC-EDMC-1536-1024` | hash_128 | 16704 | 1404 KiB | 1468 KiB |
| `ZC-EDMC-1536-1024` | hash_512 | 16704 | 1404 KiB | 1468 KiB |
| `ZC-EDMC-1536-1024` | hash_1024 | 16704 | 1404 KiB | 1468 KiB |
| `ZC-EDMC-1536-1024` | hash_4096 | 16704 | 3436 KiB | 3500 KiB |
| `ZC-EDMC-1536-1024` | hash_8192 | 16704 | 1412 KiB | 1476 KiB |
| `ZC-EDMC-1536-1024` | hash_16384 | 16704 | 1420 KiB | 1484 KiB |
| `ZC-EDMC-1536-1024` | hash_65536 | 16704 | 1468 KiB | 1532 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `ZC-EDMC-1280-512` | KAT log (sha256 `fd0e6077f885c07b…`) | `kat/hash-32/ZC-EDMC-1280-512.log` |
| `ZC-EDMC-1280-512` | timing hash_1024 | `records/hash-32/ZC-EDMC-1280-512__hash_1024.json` |
| `ZC-EDMC-1280-512` | timing hash_128 | `records/hash-32/ZC-EDMC-1280-512__hash_128.json` |
| `ZC-EDMC-1280-512` | timing hash_16384 | `records/hash-32/ZC-EDMC-1280-512__hash_16384.json` |
| `ZC-EDMC-1280-512` | timing hash_32 | `records/hash-32/ZC-EDMC-1280-512__hash_32.json` |
| `ZC-EDMC-1280-512` | timing hash_4096 | `records/hash-32/ZC-EDMC-1280-512__hash_4096.json` |
| `ZC-EDMC-1280-512` | timing hash_512 | `records/hash-32/ZC-EDMC-1280-512__hash_512.json` |
| `ZC-EDMC-1280-512` | timing hash_65536 | `records/hash-32/ZC-EDMC-1280-512__hash_65536.json` |
| `ZC-EDMC-1280-512` | timing hash_8192 | `records/hash-32/ZC-EDMC-1280-512__hash_8192.json` |
| `ZC-EDMC-1280-768` | KAT log (sha256 `a93bee8c2b50618b…`) | `kat/hash-32/ZC-EDMC-1280-768.log` |
| `ZC-EDMC-1280-768` | timing hash_1024 | `records/hash-32/ZC-EDMC-1280-768__hash_1024.json` |
| `ZC-EDMC-1280-768` | timing hash_128 | `records/hash-32/ZC-EDMC-1280-768__hash_128.json` |
| `ZC-EDMC-1280-768` | timing hash_16384 | `records/hash-32/ZC-EDMC-1280-768__hash_16384.json` |
| `ZC-EDMC-1280-768` | timing hash_32 | `records/hash-32/ZC-EDMC-1280-768__hash_32.json` |
| `ZC-EDMC-1280-768` | timing hash_4096 | `records/hash-32/ZC-EDMC-1280-768__hash_4096.json` |
| `ZC-EDMC-1280-768` | timing hash_512 | `records/hash-32/ZC-EDMC-1280-768__hash_512.json` |
| `ZC-EDMC-1280-768` | timing hash_65536 | `records/hash-32/ZC-EDMC-1280-768__hash_65536.json` |
| `ZC-EDMC-1280-768` | timing hash_8192 | `records/hash-32/ZC-EDMC-1280-768__hash_8192.json` |
| `ZC-EDMC-1280-1024` | KAT log (sha256 `18371e21e166ef2d…`) | `kat/hash-32/ZC-EDMC-1280-1024.log` |
| `ZC-EDMC-1280-1024` | timing hash_1024 | `records/hash-32/ZC-EDMC-1280-1024__hash_1024.json` |
| `ZC-EDMC-1280-1024` | timing hash_128 | `records/hash-32/ZC-EDMC-1280-1024__hash_128.json` |
| `ZC-EDMC-1280-1024` | timing hash_16384 | `records/hash-32/ZC-EDMC-1280-1024__hash_16384.json` |
| `ZC-EDMC-1280-1024` | timing hash_32 | `records/hash-32/ZC-EDMC-1280-1024__hash_32.json` |
| `ZC-EDMC-1280-1024` | timing hash_4096 | `records/hash-32/ZC-EDMC-1280-1024__hash_4096.json` |
| `ZC-EDMC-1280-1024` | timing hash_512 | `records/hash-32/ZC-EDMC-1280-1024__hash_512.json` |
| `ZC-EDMC-1280-1024` | timing hash_65536 | `records/hash-32/ZC-EDMC-1280-1024__hash_65536.json` |
| `ZC-EDMC-1280-1024` | timing hash_8192 | `records/hash-32/ZC-EDMC-1280-1024__hash_8192.json` |
| `ZC-EDMC-1536-512` | KAT log (sha256 `06ccca8fce871ce4…`) | `kat/hash-32/ZC-EDMC-1536-512.log` |
| `ZC-EDMC-1536-512` | timing hash_1024 | `records/hash-32/ZC-EDMC-1536-512__hash_1024.json` |
| `ZC-EDMC-1536-512` | timing hash_128 | `records/hash-32/ZC-EDMC-1536-512__hash_128.json` |
| `ZC-EDMC-1536-512` | timing hash_16384 | `records/hash-32/ZC-EDMC-1536-512__hash_16384.json` |
| `ZC-EDMC-1536-512` | timing hash_32 | `records/hash-32/ZC-EDMC-1536-512__hash_32.json` |
| `ZC-EDMC-1536-512` | timing hash_4096 | `records/hash-32/ZC-EDMC-1536-512__hash_4096.json` |
| `ZC-EDMC-1536-512` | timing hash_512 | `records/hash-32/ZC-EDMC-1536-512__hash_512.json` |
| `ZC-EDMC-1536-512` | timing hash_65536 | `records/hash-32/ZC-EDMC-1536-512__hash_65536.json` |
| `ZC-EDMC-1536-512` | timing hash_8192 | `records/hash-32/ZC-EDMC-1536-512__hash_8192.json` |
| `ZC-EDMC-1536-768` | KAT log (sha256 `91a7f06e7ba69db3…`) | `kat/hash-32/ZC-EDMC-1536-768.log` |
| `ZC-EDMC-1536-768` | timing hash_1024 | `records/hash-32/ZC-EDMC-1536-768__hash_1024.json` |
| `ZC-EDMC-1536-768` | timing hash_128 | `records/hash-32/ZC-EDMC-1536-768__hash_128.json` |
| `ZC-EDMC-1536-768` | timing hash_16384 | `records/hash-32/ZC-EDMC-1536-768__hash_16384.json` |
| `ZC-EDMC-1536-768` | timing hash_32 | `records/hash-32/ZC-EDMC-1536-768__hash_32.json` |
| `ZC-EDMC-1536-768` | timing hash_4096 | `records/hash-32/ZC-EDMC-1536-768__hash_4096.json` |
| `ZC-EDMC-1536-768` | timing hash_512 | `records/hash-32/ZC-EDMC-1536-768__hash_512.json` |
| `ZC-EDMC-1536-768` | timing hash_65536 | `records/hash-32/ZC-EDMC-1536-768__hash_65536.json` |
| `ZC-EDMC-1536-768` | timing hash_8192 | `records/hash-32/ZC-EDMC-1536-768__hash_8192.json` |
| `ZC-EDMC-1536-1024` | KAT log (sha256 `c1b8b5b4cd603e41…`) | `kat/hash-32/ZC-EDMC-1536-1024.log` |
| `ZC-EDMC-1536-1024` | timing hash_1024 | `records/hash-32/ZC-EDMC-1536-1024__hash_1024.json` |
| `ZC-EDMC-1536-1024` | timing hash_128 | `records/hash-32/ZC-EDMC-1536-1024__hash_128.json` |
| `ZC-EDMC-1536-1024` | timing hash_16384 | `records/hash-32/ZC-EDMC-1536-1024__hash_16384.json` |
| `ZC-EDMC-1536-1024` | timing hash_32 | `records/hash-32/ZC-EDMC-1536-1024__hash_32.json` |
| `ZC-EDMC-1536-1024` | timing hash_4096 | `records/hash-32/ZC-EDMC-1536-1024__hash_4096.json` |
| `ZC-EDMC-1536-1024` | timing hash_512 | `records/hash-32/ZC-EDMC-1536-1024__hash_512.json` |
| `ZC-EDMC-1536-1024` | timing hash_65536 | `records/hash-32/ZC-EDMC-1536-1024__hash_65536.json` |
| `ZC-EDMC-1536-1024` | timing hash_8192 | `records/hash-32/ZC-EDMC-1536-1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

