# hash-30 The ZC-DM Hash Function — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-30` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101534683994607616.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: The ZC-DM Hash Function
- Implementation versions measured: reference
- Parameter sets: `ZC-DM-1280-512`, `ZC-DM-1280-768`, `ZC-DM-1280-1024`, `ZC-DM-1536-512`, `ZC-DM-1536-768`, `ZC-DM-1536-1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-30/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `ZC-DM-1280-512` | guide | PASS |
| `ZC-DM-1280-768` | guide | PASS |
| `ZC-DM-1280-1024` | guide | PASS |
| `ZC-DM-1536-512` | guide | PASS |
| `ZC-DM-1536-768` | guide | PASS |
| `ZC-DM-1536-1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `ZC-DM-1280-512` | 32 B | 497 | 15.5 | 185 ns | 173.0 | 100000 (5 × 20000) |
| `ZC-DM-1280-512` | 128 B | 931 | 7.3 | 347 ns | 369.2 | 100000 (5 × 20000) |
| `ZC-DM-1280-512` | 512 B | 2675 | 5.2 | 994 ns | 514.9 | 100000 (5 × 20000) |
| `ZC-DM-1280-512` | 1024 B | 5284 | 5.2 | 1.96 µs | 521.7 | 100000 (5 × 20000) |
| `ZC-DM-1280-512` | 4096 B | 20.5 k | 5.0 | 7.59 µs | 539.6 | 100000 (5 × 20000) |
| `ZC-DM-1280-512` | 8192 B | 40.8 k | 5.0 | 15.2 µs | 540.6 | 100000 (5 × 20000) |
| `ZC-DM-1280-512` | 16384 B | 81.1 k | 5.0 | 30.1 µs | 544.1 | 100000 (5 × 20000) |
| `ZC-DM-1280-512` | 65536 B | 323.0 k | 4.9 | 120 µs | 546.8 | 25950 (5 × 5190) |
| `ZC-DM-1280-768` | 32 B | 906 | 28.3 | 337 ns | 94.9 | 100000 (5 × 20000) |
| `ZC-DM-1280-768` | 128 B | 1760 | 13.7 | 655 ns | 195.3 | 100000 (5 × 20000) |
| `ZC-DM-1280-768` | 512 B | 4758 | 9.3 | 1.77 µs | 289.5 | 100000 (5 × 20000) |
| `ZC-DM-1280-768` | 1024 B | 8594 | 8.4 | 3.19 µs | 320.9 | 100000 (5 × 20000) |
| `ZC-DM-1280-768` | 4096 B | 32.0 k | 7.8 | 11.9 µs | 343.9 | 100000 (5 × 20000) |
| `ZC-DM-1280-768` | 8192 B | 63.1 k | 7.7 | 23.4 µs | 349.8 | 100000 (5 × 20000) |
| `ZC-DM-1280-768` | 16384 B | 125.3 k | 7.6 | 46.5 µs | 352.3 | 66120 (5 × 13224) |
| `ZC-DM-1280-768` | 65536 B | 499.2 k | 7.6 | 185 µs | 353.7 | 16970 (5 × 3394) |
| `ZC-DM-1280-1024` | 32 B | 2909 | 90.9 | 1.08 µs | 29.6 | 100000 (5 × 20000) |
| `ZC-DM-1280-1024` | 128 B | 4633 | 36.2 | 1.72 µs | 74.4 | 100000 (5 × 20000) |
| `ZC-DM-1280-1024` | 512 B | 11.5 k | 22.5 | 4.28 µs | 119.6 | 100000 (5 × 20000) |
| `ZC-DM-1280-1024` | 1024 B | 20.6 k | 20.1 | 7.64 µs | 134.0 | 100000 (5 × 20000) |
| `ZC-DM-1280-1024` | 4096 B | 75.6 k | 18.5 | 28.1 µs | 146.0 | 100000 (5 × 20000) |
| `ZC-DM-1280-1024` | 8192 B | 149.5 k | 18.2 | 55.5 µs | 147.7 | 55495 (5 × 11099) |
| `ZC-DM-1280-1024` | 16384 B | 295.9 k | 18.1 | 110 µs | 149.2 | 28695 (5 × 5739) |
| `ZC-DM-1280-1024` | 65536 B | 1.18 M | 18.0 | 437 µs | 149.8 | 7150 (5 × 1430) |
| `ZC-DM-1536-512` | 32 B | 919 | 28.7 | 343 ns | 93.4 | 100000 (5 × 20000) |
| `ZC-DM-1536-512` | 128 B | 1903 | 14.9 | 708 ns | 180.8 | 100000 (5 × 20000) |
| `ZC-DM-1536-512` | 512 B | 4315 | 8.4 | 1.6 µs | 319.2 | 100000 (5 × 20000) |
| `ZC-DM-1536-512` | 1024 B | 7708 | 7.5 | 2.87 µs | 357.4 | 100000 (5 × 20000) |
| `ZC-DM-1536-512` | 4096 B | 29.8 k | 7.3 | 11.1 µs | 370.6 | 100000 (5 × 20000) |
| `ZC-DM-1536-512` | 8192 B | 58.6 k | 7.2 | 21.7 µs | 376.7 | 100000 (5 × 20000) |
| `ZC-DM-1536-512` | 16384 B | 116.3 k | 7.1 | 43.2 µs | 379.7 | 70800 (5 × 14160) |
| `ZC-DM-1536-512` | 65536 B | 464.2 k | 7.1 | 172 µs | 380.4 | 18170 (5 × 3634) |
| `ZC-DM-1536-768` | 32 B | 1885 | 58.9 | 701 ns | 45.7 | 100000 (5 × 20000) |
| `ZC-DM-1536-768` | 128 B | 2572 | 20.1 | 956 ns | 133.8 | 100000 (5 × 20000) |
| `ZC-DM-1536-768` | 512 B | 5955 | 11.6 | 2.21 µs | 231.5 | 100000 (5 × 20000) |
| `ZC-DM-1536-768` | 1024 B | 11.0 k | 10.8 | 4.1 µs | 249.7 | 100000 (5 × 20000) |
| `ZC-DM-1536-768` | 4096 B | 40.7 k | 9.9 | 15.1 µs | 271.2 | 100000 (5 × 20000) |
| `ZC-DM-1536-768` | 8192 B | 80.5 k | 9.8 | 29.9 µs | 274.3 | 98160 (5 × 19632) |
| `ZC-DM-1536-768` | 16384 B | 159.3 k | 9.7 | 59.1 µs | 277.1 | 51950 (5 × 10390) |
| `ZC-DM-1536-768` | 65536 B | 632.1 k | 9.6 | 235 µs | 279.5 | 13005 (5 × 2601) |
| `ZC-DM-1536-1024` | 32 B | 2558 | 79.9 | 951 ns | 33.6 | 100000 (5 × 20000) |
| `ZC-DM-1536-1024` | 128 B | 4235 | 33.1 | 1.57 µs | 81.3 | 100000 (5 × 20000) |
| `ZC-DM-1536-1024` | 512 B | 10.1 k | 19.8 | 3.77 µs | 135.8 | 100000 (5 × 20000) |
| `ZC-DM-1536-1024` | 1024 B | 17.8 k | 17.3 | 6.59 µs | 155.3 | 100000 (5 × 20000) |
| `ZC-DM-1536-1024` | 4096 B | 64.2 k | 15.7 | 23.8 µs | 172.0 | 100000 (5 × 20000) |
| `ZC-DM-1536-1024` | 8192 B | 125.7 k | 15.3 | 46.7 µs | 175.6 | 65575 (5 × 13115) |
| `ZC-DM-1536-1024` | 16384 B | 249.1 k | 15.2 | 92.4 µs | 177.2 | 33710 (5 × 6742) |
| `ZC-DM-1536-1024` | 65536 B | 989.4 k | 15.1 | 367 µs | 178.5 | 8580 (5 × 1716) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `ZC-DM-1280-512` | hash_32 | 12664 | 1400 KiB | 1464 KiB |
| `ZC-DM-1280-512` | hash_128 | 12664 | 1400 KiB | 1464 KiB |
| `ZC-DM-1280-512` | hash_512 | 12664 | 1400 KiB | 1464 KiB |
| `ZC-DM-1280-512` | hash_1024 | 12664 | 3444 KiB | 3508 KiB |
| `ZC-DM-1280-512` | hash_4096 | 12664 | 1404 KiB | 1468 KiB |
| `ZC-DM-1280-512` | hash_8192 | 12664 | 1408 KiB | 1472 KiB |
| `ZC-DM-1280-512` | hash_16384 | 12664 | 1416 KiB | 1480 KiB |
| `ZC-DM-1280-512` | hash_65536 | 12664 | 1464 KiB | 1528 KiB |
| `ZC-DM-1280-768` | hash_32 | 12664 | 1400 KiB | 1464 KiB |
| `ZC-DM-1280-768` | hash_128 | 12664 | 1400 KiB | 1464 KiB |
| `ZC-DM-1280-768` | hash_512 | 12664 | 1400 KiB | 1464 KiB |
| `ZC-DM-1280-768` | hash_1024 | 12664 | 1404 KiB | 1468 KiB |
| `ZC-DM-1280-768` | hash_4096 | 12664 | 1404 KiB | 1468 KiB |
| `ZC-DM-1280-768` | hash_8192 | 12664 | 1408 KiB | 1472 KiB |
| `ZC-DM-1280-768` | hash_16384 | 12664 | 3456 KiB | 3520 KiB |
| `ZC-DM-1280-768` | hash_65536 | 12664 | 1464 KiB | 1528 KiB |
| `ZC-DM-1280-1024` | hash_32 | 12664 | 1400 KiB | 1464 KiB |
| `ZC-DM-1280-1024` | hash_128 | 12664 | 1400 KiB | 1464 KiB |
| `ZC-DM-1280-1024` | hash_512 | 12664 | 1400 KiB | 1464 KiB |
| `ZC-DM-1280-1024` | hash_1024 | 12664 | 1404 KiB | 1468 KiB |
| `ZC-DM-1280-1024` | hash_4096 | 12664 | 1404 KiB | 1468 KiB |
| `ZC-DM-1280-1024` | hash_8192 | 12664 | 1408 KiB | 1472 KiB |
| `ZC-DM-1280-1024` | hash_16384 | 12664 | 1416 KiB | 1480 KiB |
| `ZC-DM-1280-1024` | hash_65536 | 12664 | 3484 KiB | 3548 KiB |
| `ZC-DM-1536-512` | hash_32 | 16720 | 1404 KiB | 1468 KiB |
| `ZC-DM-1536-512` | hash_128 | 16720 | 1404 KiB | 1468 KiB |
| `ZC-DM-1536-512` | hash_512 | 16720 | 1404 KiB | 1468 KiB |
| `ZC-DM-1536-512` | hash_1024 | 16720 | 1408 KiB | 1472 KiB |
| `ZC-DM-1536-512` | hash_4096 | 16720 | 1408 KiB | 1472 KiB |
| `ZC-DM-1536-512` | hash_8192 | 16720 | 1412 KiB | 1476 KiB |
| `ZC-DM-1536-512` | hash_16384 | 16720 | 1420 KiB | 1484 KiB |
| `ZC-DM-1536-512` | hash_65536 | 16720 | 1468 KiB | 1532 KiB |
| `ZC-DM-1536-768` | hash_32 | 16720 | 1404 KiB | 1468 KiB |
| `ZC-DM-1536-768` | hash_128 | 16720 | 1404 KiB | 1468 KiB |
| `ZC-DM-1536-768` | hash_512 | 16720 | 1404 KiB | 1468 KiB |
| `ZC-DM-1536-768` | hash_1024 | 16720 | 1408 KiB | 1472 KiB |
| `ZC-DM-1536-768` | hash_4096 | 16720 | 1408 KiB | 1472 KiB |
| `ZC-DM-1536-768` | hash_8192 | 16720 | 1412 KiB | 1476 KiB |
| `ZC-DM-1536-768` | hash_16384 | 16720 | 1420 KiB | 1484 KiB |
| `ZC-DM-1536-768` | hash_65536 | 16720 | 1468 KiB | 1532 KiB |
| `ZC-DM-1536-1024` | hash_32 | 16720 | 1404 KiB | 1468 KiB |
| `ZC-DM-1536-1024` | hash_128 | 16720 | 1404 KiB | 1468 KiB |
| `ZC-DM-1536-1024` | hash_512 | 16720 | 1404 KiB | 1468 KiB |
| `ZC-DM-1536-1024` | hash_1024 | 16720 | 1408 KiB | 1472 KiB |
| `ZC-DM-1536-1024` | hash_4096 | 16720 | 1408 KiB | 1472 KiB |
| `ZC-DM-1536-1024` | hash_8192 | 16720 | 1412 KiB | 1476 KiB |
| `ZC-DM-1536-1024` | hash_16384 | 16720 | 1420 KiB | 1484 KiB |
| `ZC-DM-1536-1024` | hash_65536 | 16720 | 3488 KiB | 3552 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `ZC-DM-1280-512` | KAT log (sha256 `fd498f3f2835d888…`) | `kat/hash-30/ZC-DM-1280-512.log` |
| `ZC-DM-1280-512` | timing hash_1024 | `records/hash-30/ZC-DM-1280-512__hash_1024.json` |
| `ZC-DM-1280-512` | timing hash_128 | `records/hash-30/ZC-DM-1280-512__hash_128.json` |
| `ZC-DM-1280-512` | timing hash_16384 | `records/hash-30/ZC-DM-1280-512__hash_16384.json` |
| `ZC-DM-1280-512` | timing hash_32 | `records/hash-30/ZC-DM-1280-512__hash_32.json` |
| `ZC-DM-1280-512` | timing hash_4096 | `records/hash-30/ZC-DM-1280-512__hash_4096.json` |
| `ZC-DM-1280-512` | timing hash_512 | `records/hash-30/ZC-DM-1280-512__hash_512.json` |
| `ZC-DM-1280-512` | timing hash_65536 | `records/hash-30/ZC-DM-1280-512__hash_65536.json` |
| `ZC-DM-1280-512` | timing hash_8192 | `records/hash-30/ZC-DM-1280-512__hash_8192.json` |
| `ZC-DM-1280-768` | KAT log (sha256 `d676ff6ee9dc2341…`) | `kat/hash-30/ZC-DM-1280-768.log` |
| `ZC-DM-1280-768` | timing hash_1024 | `records/hash-30/ZC-DM-1280-768__hash_1024.json` |
| `ZC-DM-1280-768` | timing hash_128 | `records/hash-30/ZC-DM-1280-768__hash_128.json` |
| `ZC-DM-1280-768` | timing hash_16384 | `records/hash-30/ZC-DM-1280-768__hash_16384.json` |
| `ZC-DM-1280-768` | timing hash_32 | `records/hash-30/ZC-DM-1280-768__hash_32.json` |
| `ZC-DM-1280-768` | timing hash_4096 | `records/hash-30/ZC-DM-1280-768__hash_4096.json` |
| `ZC-DM-1280-768` | timing hash_512 | `records/hash-30/ZC-DM-1280-768__hash_512.json` |
| `ZC-DM-1280-768` | timing hash_65536 | `records/hash-30/ZC-DM-1280-768__hash_65536.json` |
| `ZC-DM-1280-768` | timing hash_8192 | `records/hash-30/ZC-DM-1280-768__hash_8192.json` |
| `ZC-DM-1280-1024` | KAT log (sha256 `be7e7ccb9e73949d…`) | `kat/hash-30/ZC-DM-1280-1024.log` |
| `ZC-DM-1280-1024` | timing hash_1024 | `records/hash-30/ZC-DM-1280-1024__hash_1024.json` |
| `ZC-DM-1280-1024` | timing hash_128 | `records/hash-30/ZC-DM-1280-1024__hash_128.json` |
| `ZC-DM-1280-1024` | timing hash_16384 | `records/hash-30/ZC-DM-1280-1024__hash_16384.json` |
| `ZC-DM-1280-1024` | timing hash_32 | `records/hash-30/ZC-DM-1280-1024__hash_32.json` |
| `ZC-DM-1280-1024` | timing hash_4096 | `records/hash-30/ZC-DM-1280-1024__hash_4096.json` |
| `ZC-DM-1280-1024` | timing hash_512 | `records/hash-30/ZC-DM-1280-1024__hash_512.json` |
| `ZC-DM-1280-1024` | timing hash_65536 | `records/hash-30/ZC-DM-1280-1024__hash_65536.json` |
| `ZC-DM-1280-1024` | timing hash_8192 | `records/hash-30/ZC-DM-1280-1024__hash_8192.json` |
| `ZC-DM-1536-512` | KAT log (sha256 `f48fecda0086bc69…`) | `kat/hash-30/ZC-DM-1536-512.log` |
| `ZC-DM-1536-512` | timing hash_1024 | `records/hash-30/ZC-DM-1536-512__hash_1024.json` |
| `ZC-DM-1536-512` | timing hash_128 | `records/hash-30/ZC-DM-1536-512__hash_128.json` |
| `ZC-DM-1536-512` | timing hash_16384 | `records/hash-30/ZC-DM-1536-512__hash_16384.json` |
| `ZC-DM-1536-512` | timing hash_32 | `records/hash-30/ZC-DM-1536-512__hash_32.json` |
| `ZC-DM-1536-512` | timing hash_4096 | `records/hash-30/ZC-DM-1536-512__hash_4096.json` |
| `ZC-DM-1536-512` | timing hash_512 | `records/hash-30/ZC-DM-1536-512__hash_512.json` |
| `ZC-DM-1536-512` | timing hash_65536 | `records/hash-30/ZC-DM-1536-512__hash_65536.json` |
| `ZC-DM-1536-512` | timing hash_8192 | `records/hash-30/ZC-DM-1536-512__hash_8192.json` |
| `ZC-DM-1536-768` | KAT log (sha256 `01d1887987b6a755…`) | `kat/hash-30/ZC-DM-1536-768.log` |
| `ZC-DM-1536-768` | timing hash_1024 | `records/hash-30/ZC-DM-1536-768__hash_1024.json` |
| `ZC-DM-1536-768` | timing hash_128 | `records/hash-30/ZC-DM-1536-768__hash_128.json` |
| `ZC-DM-1536-768` | timing hash_16384 | `records/hash-30/ZC-DM-1536-768__hash_16384.json` |
| `ZC-DM-1536-768` | timing hash_32 | `records/hash-30/ZC-DM-1536-768__hash_32.json` |
| `ZC-DM-1536-768` | timing hash_4096 | `records/hash-30/ZC-DM-1536-768__hash_4096.json` |
| `ZC-DM-1536-768` | timing hash_512 | `records/hash-30/ZC-DM-1536-768__hash_512.json` |
| `ZC-DM-1536-768` | timing hash_65536 | `records/hash-30/ZC-DM-1536-768__hash_65536.json` |
| `ZC-DM-1536-768` | timing hash_8192 | `records/hash-30/ZC-DM-1536-768__hash_8192.json` |
| `ZC-DM-1536-1024` | KAT log (sha256 `81f68be63270df7f…`) | `kat/hash-30/ZC-DM-1536-1024.log` |
| `ZC-DM-1536-1024` | timing hash_1024 | `records/hash-30/ZC-DM-1536-1024__hash_1024.json` |
| `ZC-DM-1536-1024` | timing hash_128 | `records/hash-30/ZC-DM-1536-1024__hash_128.json` |
| `ZC-DM-1536-1024` | timing hash_16384 | `records/hash-30/ZC-DM-1536-1024__hash_16384.json` |
| `ZC-DM-1536-1024` | timing hash_32 | `records/hash-30/ZC-DM-1536-1024__hash_32.json` |
| `ZC-DM-1536-1024` | timing hash_4096 | `records/hash-30/ZC-DM-1536-1024__hash_4096.json` |
| `ZC-DM-1536-1024` | timing hash_512 | `records/hash-30/ZC-DM-1536-1024__hash_512.json` |
| `ZC-DM-1536-1024` | timing hash_65536 | `records/hash-30/ZC-DM-1536-1024__hash_65536.json` |
| `ZC-DM-1536-1024` | timing hash_8192 | `records/hash-30/ZC-DM-1536-1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

