# hash-31 The ZC-DMC Hash Function — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-31` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101534457934204928.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: The ZC-DMC Hash Function
- Implementation versions measured: reference
- Parameter sets: `ZC-DMC-1280-512`, `ZC-DMC-1280-768`, `ZC-DMC-1280-1024`, `ZC-DMC-1536-512`, `ZC-DMC-1536-768`, `ZC-DMC-1536-1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-31/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `ZC-DMC-1280-512` | guide | PASS |
| `ZC-DMC-1280-768` | guide | PASS |
| `ZC-DMC-1280-1024` | guide | PASS |
| `ZC-DMC-1536-512` | guide | PASS |
| `ZC-DMC-1536-768` | guide | PASS |
| `ZC-DMC-1536-1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `ZC-DMC-1280-512` | 32 B | 3303 | 103.2 | 1.23 µs | 26.1 | 100000 (5 × 20000) |
| `ZC-DMC-1280-512` | 128 B | 3723 | 29.1 | 1.38 µs | 92.5 | 100000 (5 × 20000) |
| `ZC-DMC-1280-512` | 512 B | 5495 | 10.7 | 2.04 µs | 250.7 | 100000 (5 × 20000) |
| `ZC-DMC-1280-512` | 1024 B | 10.9 k | 10.7 | 4.05 µs | 252.8 | 100000 (5 × 20000) |
| `ZC-DMC-1280-512` | 4096 B | 23.5 k | 5.7 | 8.7 µs | 470.6 | 100000 (5 × 20000) |
| `ZC-DMC-1280-512` | 8192 B | 44.0 k | 5.4 | 16.3 µs | 501.6 | 70075 (5 × 14015) |
| `ZC-DMC-1280-512` | 16384 B | 84.7 k | 5.2 | 31.4 µs | 521.3 | 36365 (5 × 7273) |
| `ZC-DMC-1280-512` | 65536 B | 328.8 k | 5.0 | 122 µs | 537.0 | 25645 (5 × 5129) |
| `ZC-DMC-1280-768` | 32 B | 4886 | 152.7 | 1.82 µs | 17.6 | 100000 (5 × 20000) |
| `ZC-DMC-1280-768` | 128 B | 5749 | 44.9 | 2.14 µs | 59.9 | 100000 (5 × 20000) |
| `ZC-DMC-1280-768` | 512 B | 11.6 k | 22.7 | 4.32 µs | 118.5 | 100000 (5 × 20000) |
| `ZC-DMC-1280-768` | 1024 B | 17.3 k | 16.8 | 6.4 µs | 159.9 | 100000 (5 × 20000) |
| `ZC-DMC-1280-768` | 4096 B | 36.7 k | 9.0 | 13.6 µs | 300.3 | 100000 (5 × 20000) |
| `ZC-DMC-1280-768` | 8192 B | 68.6 k | 8.4 | 25.5 µs | 321.7 | 100000 (5 × 20000) |
| `ZC-DMC-1280-768` | 16384 B | 132.6 k | 8.1 | 49.2 µs | 333.0 | 62500 (5 × 12500) |
| `ZC-DMC-1280-768` | 65536 B | 515.3 k | 7.9 | 191 µs | 342.6 | 16430 (5 × 3286) |
| `ZC-DMC-1280-1024` | 32 B | 6892 | 215.4 | 2.56 µs | 12.5 | 100000 (5 × 20000) |
| `ZC-DMC-1280-1024` | 128 B | 8642 | 67.5 | 3.21 µs | 39.9 | 100000 (5 × 20000) |
| `ZC-DMC-1280-1024` | 512 B | 15.6 k | 30.5 | 5.79 µs | 88.4 | 100000 (5 × 20000) |
| `ZC-DMC-1280-1024` | 1024 B | 24.8 k | 24.2 | 9.19 µs | 111.4 | 100000 (5 × 20000) |
| `ZC-DMC-1280-1024` | 4096 B | 80.5 k | 19.7 | 29.9 µs | 137.0 | 98765 (5 × 19753) |
| `ZC-DMC-1280-1024` | 8192 B | 209.0 k | 25.5 | 77.5 µs | 105.7 | 53160 (5 × 10632) |
| `ZC-DMC-1280-1024` | 16384 B | 303.3 k | 18.5 | 113 µs | 145.6 | 27635 (5 × 5527) |
| `ZC-DMC-1280-1024` | 65536 B | 1.20 M | 18.2 | 443 µs | 147.8 | 6985 (5 × 1397) |
| `ZC-DMC-1536-512` | 32 B | 6910 | 215.9 | 2.57 µs | 12.5 | 100000 (5 × 20000) |
| `ZC-DMC-1536-512` | 128 B | 12.5 k | 97.4 | 4.62 µs | 27.7 | 100000 (5 × 20000) |
| `ZC-DMC-1536-512` | 512 B | 18.2 k | 35.5 | 6.74 µs | 76.0 | 100000 (5 × 20000) |
| `ZC-DMC-1536-512` | 1024 B | 22.8 k | 22.3 | 8.47 µs | 120.9 | 100000 (5 × 20000) |
| `ZC-DMC-1536-512` | 4096 B | 36.4 k | 8.9 | 13.5 µs | 302.8 | 100000 (5 × 20000) |
| `ZC-DMC-1536-512` | 8192 B | 67.2 k | 8.2 | 24.9 µs | 328.7 | 100000 (5 × 20000) |
| `ZC-DMC-1536-512` | 16384 B | 166.1 k | 10.1 | 61.6 µs | 265.8 | 65845 (5 × 13169) |
| `ZC-DMC-1536-512` | 65536 B | 823.4 k | 12.6 | 306 µs | 214.5 | 9980 (5 × 1996) |
| `ZC-DMC-1536-768` | 32 B | 10.3 k | 323.0 | 3.84 µs | 8.3 | 100000 (5 × 20000) |
| `ZC-DMC-1536-768` | 128 B | 11.1 k | 87.1 | 4.14 µs | 30.9 | 100000 (5 × 20000) |
| `ZC-DMC-1536-768` | 512 B | 14.8 k | 28.9 | 5.49 µs | 93.3 | 100000 (5 × 20000) |
| `ZC-DMC-1536-768` | 1024 B | 19.7 k | 19.3 | 7.33 µs | 139.8 | 100000 (5 × 20000) |
| `ZC-DMC-1536-768` | 4096 B | 50.4 k | 12.3 | 18.7 µs | 218.9 | 100000 (5 × 20000) |
| `ZC-DMC-1536-768` | 8192 B | 92.1 k | 11.2 | 34.2 µs | 239.7 | 88400 (5 × 17680) |
| `ZC-DMC-1536-768` | 16384 B | 172.2 k | 10.5 | 63.9 µs | 256.3 | 47340 (5 × 9468) |
| `ZC-DMC-1536-768` | 65536 B | 663.3 k | 10.1 | 246 µs | 266.2 | 3190 (5 × 638) |
| `ZC-DMC-1536-1024` | 32 B | 13.7 k | 427.5 | 5.08 µs | 6.3 | 100000 (5 × 20000) |
| `ZC-DMC-1536-1024` | 128 B | 15.5 k | 121.0 | 5.75 µs | 22.3 | 100000 (5 × 20000) |
| `ZC-DMC-1536-1024` | 512 B | 21.6 k | 42.3 | 8.04 µs | 63.7 | 100000 (5 × 20000) |
| `ZC-DMC-1536-1024` | 1024 B | 29.6 k | 28.9 | 11 µs | 93.2 | 100000 (5 × 20000) |
| `ZC-DMC-1536-1024` | 4096 B | 78.1 k | 19.1 | 29 µs | 141.2 | 100000 (5 × 20000) |
| `ZC-DMC-1536-1024` | 8192 B | 187.6 k | 22.9 | 69.6 µs | 117.7 | 58540 (5 × 11708) |
| `ZC-DMC-1536-1024` | 16384 B | 357.4 k | 21.8 | 133 µs | 123.5 | 31050 (5 × 6210) |
| `ZC-DMC-1536-1024` | 65536 B | 1.37 M | 20.9 | 509 µs | 128.8 | 8195 (5 × 1639) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `ZC-DMC-1280-512` | hash_32 | 12616 | 1400 KiB | 1464 KiB |
| `ZC-DMC-1280-512` | hash_128 | 12616 | 1400 KiB | 1464 KiB |
| `ZC-DMC-1280-512` | hash_512 | 12616 | 1400 KiB | 1464 KiB |
| `ZC-DMC-1280-512` | hash_1024 | 12616 | 1404 KiB | 1468 KiB |
| `ZC-DMC-1280-512` | hash_4096 | 12616 | 1404 KiB | 1468 KiB |
| `ZC-DMC-1280-512` | hash_8192 | 12616 | 1408 KiB | 1472 KiB |
| `ZC-DMC-1280-512` | hash_16384 | 12616 | 1416 KiB | 1480 KiB |
| `ZC-DMC-1280-512` | hash_65536 | 12616 | 1464 KiB | 1528 KiB |
| `ZC-DMC-1280-768` | hash_32 | 12616 | 1400 KiB | 1464 KiB |
| `ZC-DMC-1280-768` | hash_128 | 12616 | 1400 KiB | 1464 KiB |
| `ZC-DMC-1280-768` | hash_512 | 12616 | 1396 KiB | 1460 KiB |
| `ZC-DMC-1280-768` | hash_1024 | 12616 | 1404 KiB | 1468 KiB |
| `ZC-DMC-1280-768` | hash_4096 | 12616 | 1404 KiB | 1468 KiB |
| `ZC-DMC-1280-768` | hash_8192 | 12616 | 1408 KiB | 1472 KiB |
| `ZC-DMC-1280-768` | hash_16384 | 12616 | 1416 KiB | 1480 KiB |
| `ZC-DMC-1280-768` | hash_65536 | 12616 | 3500 KiB | 3564 KiB |
| `ZC-DMC-1280-1024` | hash_32 | 12584 | 1400 KiB | 1464 KiB |
| `ZC-DMC-1280-1024` | hash_128 | 12584 | 1400 KiB | 1464 KiB |
| `ZC-DMC-1280-1024` | hash_512 | 12584 | 1400 KiB | 1464 KiB |
| `ZC-DMC-1280-1024` | hash_1024 | 12584 | 1404 KiB | 1468 KiB |
| `ZC-DMC-1280-1024` | hash_4096 | 12584 | 1404 KiB | 1468 KiB |
| `ZC-DMC-1280-1024` | hash_8192 | 12584 | 1408 KiB | 1472 KiB |
| `ZC-DMC-1280-1024` | hash_16384 | 12584 | 1412 KiB | 1476 KiB |
| `ZC-DMC-1280-1024` | hash_65536 | 12584 | 1464 KiB | 1528 KiB |
| `ZC-DMC-1536-512` | hash_32 | 16640 | 1400 KiB | 1464 KiB |
| `ZC-DMC-1536-512` | hash_128 | 16640 | 1404 KiB | 1468 KiB |
| `ZC-DMC-1536-512` | hash_512 | 16640 | 1404 KiB | 1468 KiB |
| `ZC-DMC-1536-512` | hash_1024 | 16640 | 1404 KiB | 1468 KiB |
| `ZC-DMC-1536-512` | hash_4096 | 16640 | 1408 KiB | 1472 KiB |
| `ZC-DMC-1536-512` | hash_8192 | 16640 | 1412 KiB | 1476 KiB |
| `ZC-DMC-1536-512` | hash_16384 | 16640 | 1420 KiB | 1484 KiB |
| `ZC-DMC-1536-512` | hash_65536 | 16640 | 1468 KiB | 1532 KiB |
| `ZC-DMC-1536-768` | hash_32 | 16640 | 1404 KiB | 1468 KiB |
| `ZC-DMC-1536-768` | hash_128 | 16640 | 1404 KiB | 1468 KiB |
| `ZC-DMC-1536-768` | hash_512 | 16640 | 1404 KiB | 1468 KiB |
| `ZC-DMC-1536-768` | hash_1024 | 16640 | 1408 KiB | 1472 KiB |
| `ZC-DMC-1536-768` | hash_4096 | 16640 | 1408 KiB | 1472 KiB |
| `ZC-DMC-1536-768` | hash_8192 | 16640 | 1412 KiB | 1476 KiB |
| `ZC-DMC-1536-768` | hash_16384 | 16640 | 1420 KiB | 1484 KiB |
| `ZC-DMC-1536-768` | hash_65536 | 16640 | 1468 KiB | 1532 KiB |
| `ZC-DMC-1536-1024` | hash_32 | 16608 | 1404 KiB | 1468 KiB |
| `ZC-DMC-1536-1024` | hash_128 | 16608 | 1404 KiB | 1468 KiB |
| `ZC-DMC-1536-1024` | hash_512 | 16608 | 1404 KiB | 1468 KiB |
| `ZC-DMC-1536-1024` | hash_1024 | 16608 | 1408 KiB | 1472 KiB |
| `ZC-DMC-1536-1024` | hash_4096 | 16608 | 1408 KiB | 1472 KiB |
| `ZC-DMC-1536-1024` | hash_8192 | 16608 | 1412 KiB | 1476 KiB |
| `ZC-DMC-1536-1024` | hash_16384 | 16608 | 1420 KiB | 1484 KiB |
| `ZC-DMC-1536-1024` | hash_65536 | 16608 | 1468 KiB | 1532 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `ZC-DMC-1280-512` | KAT log (sha256 `8ae155b2b165a546…`) | `kat/hash-31/ZC-DMC-1280-512.log` |
| `ZC-DMC-1280-512` | timing hash_1024 | `records/hash-31/ZC-DMC-1280-512__hash_1024.json` |
| `ZC-DMC-1280-512` | timing hash_128 | `records/hash-31/ZC-DMC-1280-512__hash_128.json` |
| `ZC-DMC-1280-512` | timing hash_16384 | `records/hash-31/ZC-DMC-1280-512__hash_16384.json` |
| `ZC-DMC-1280-512` | timing hash_32 | `records/hash-31/ZC-DMC-1280-512__hash_32.json` |
| `ZC-DMC-1280-512` | timing hash_4096 | `records/hash-31/ZC-DMC-1280-512__hash_4096.json` |
| `ZC-DMC-1280-512` | timing hash_512 | `records/hash-31/ZC-DMC-1280-512__hash_512.json` |
| `ZC-DMC-1280-512` | timing hash_65536 | `records/hash-31/ZC-DMC-1280-512__hash_65536.json` |
| `ZC-DMC-1280-512` | timing hash_8192 | `records/hash-31/ZC-DMC-1280-512__hash_8192.json` |
| `ZC-DMC-1280-768` | KAT log (sha256 `0e9cd601a0424d55…`) | `kat/hash-31/ZC-DMC-1280-768.log` |
| `ZC-DMC-1280-768` | timing hash_1024 | `records/hash-31/ZC-DMC-1280-768__hash_1024.json` |
| `ZC-DMC-1280-768` | timing hash_128 | `records/hash-31/ZC-DMC-1280-768__hash_128.json` |
| `ZC-DMC-1280-768` | timing hash_16384 | `records/hash-31/ZC-DMC-1280-768__hash_16384.json` |
| `ZC-DMC-1280-768` | timing hash_32 | `records/hash-31/ZC-DMC-1280-768__hash_32.json` |
| `ZC-DMC-1280-768` | timing hash_4096 | `records/hash-31/ZC-DMC-1280-768__hash_4096.json` |
| `ZC-DMC-1280-768` | timing hash_512 | `records/hash-31/ZC-DMC-1280-768__hash_512.json` |
| `ZC-DMC-1280-768` | timing hash_65536 | `records/hash-31/ZC-DMC-1280-768__hash_65536.json` |
| `ZC-DMC-1280-768` | timing hash_8192 | `records/hash-31/ZC-DMC-1280-768__hash_8192.json` |
| `ZC-DMC-1280-1024` | KAT log (sha256 `3b72c02f90d751c0…`) | `kat/hash-31/ZC-DMC-1280-1024.log` |
| `ZC-DMC-1280-1024` | timing hash_1024 | `records/hash-31/ZC-DMC-1280-1024__hash_1024.json` |
| `ZC-DMC-1280-1024` | timing hash_128 | `records/hash-31/ZC-DMC-1280-1024__hash_128.json` |
| `ZC-DMC-1280-1024` | timing hash_16384 | `records/hash-31/ZC-DMC-1280-1024__hash_16384.json` |
| `ZC-DMC-1280-1024` | timing hash_32 | `records/hash-31/ZC-DMC-1280-1024__hash_32.json` |
| `ZC-DMC-1280-1024` | timing hash_4096 | `records/hash-31/ZC-DMC-1280-1024__hash_4096.json` |
| `ZC-DMC-1280-1024` | timing hash_512 | `records/hash-31/ZC-DMC-1280-1024__hash_512.json` |
| `ZC-DMC-1280-1024` | timing hash_65536 | `records/hash-31/ZC-DMC-1280-1024__hash_65536.json` |
| `ZC-DMC-1280-1024` | timing hash_8192 | `records/hash-31/ZC-DMC-1280-1024__hash_8192.json` |
| `ZC-DMC-1536-512` | KAT log (sha256 `f2b697c086e3b070…`) | `kat/hash-31/ZC-DMC-1536-512.log` |
| `ZC-DMC-1536-512` | timing hash_1024 | `records/hash-31/ZC-DMC-1536-512__hash_1024.json` |
| `ZC-DMC-1536-512` | timing hash_128 | `records/hash-31/ZC-DMC-1536-512__hash_128.json` |
| `ZC-DMC-1536-512` | timing hash_16384 | `records/hash-31/ZC-DMC-1536-512__hash_16384.json` |
| `ZC-DMC-1536-512` | timing hash_32 | `records/hash-31/ZC-DMC-1536-512__hash_32.json` |
| `ZC-DMC-1536-512` | timing hash_4096 | `records/hash-31/ZC-DMC-1536-512__hash_4096.json` |
| `ZC-DMC-1536-512` | timing hash_512 | `records/hash-31/ZC-DMC-1536-512__hash_512.json` |
| `ZC-DMC-1536-512` | timing hash_65536 | `records/hash-31/ZC-DMC-1536-512__hash_65536.json` |
| `ZC-DMC-1536-512` | timing hash_8192 | `records/hash-31/ZC-DMC-1536-512__hash_8192.json` |
| `ZC-DMC-1536-768` | KAT log (sha256 `d3a96f7037e9a30a…`) | `kat/hash-31/ZC-DMC-1536-768.log` |
| `ZC-DMC-1536-768` | timing hash_1024 | `records/hash-31/ZC-DMC-1536-768__hash_1024.json` |
| `ZC-DMC-1536-768` | timing hash_128 | `records/hash-31/ZC-DMC-1536-768__hash_128.json` |
| `ZC-DMC-1536-768` | timing hash_16384 | `records/hash-31/ZC-DMC-1536-768__hash_16384.json` |
| `ZC-DMC-1536-768` | timing hash_32 | `records/hash-31/ZC-DMC-1536-768__hash_32.json` |
| `ZC-DMC-1536-768` | timing hash_4096 | `records/hash-31/ZC-DMC-1536-768__hash_4096.json` |
| `ZC-DMC-1536-768` | timing hash_512 | `records/hash-31/ZC-DMC-1536-768__hash_512.json` |
| `ZC-DMC-1536-768` | timing hash_65536 | `records/hash-31/ZC-DMC-1536-768__hash_65536.json` |
| `ZC-DMC-1536-768` | timing hash_8192 | `records/hash-31/ZC-DMC-1536-768__hash_8192.json` |
| `ZC-DMC-1536-1024` | KAT log (sha256 `8eba759aea5a109c…`) | `kat/hash-31/ZC-DMC-1536-1024.log` |
| `ZC-DMC-1536-1024` | timing hash_1024 | `records/hash-31/ZC-DMC-1536-1024__hash_1024.json` |
| `ZC-DMC-1536-1024` | timing hash_128 | `records/hash-31/ZC-DMC-1536-1024__hash_128.json` |
| `ZC-DMC-1536-1024` | timing hash_16384 | `records/hash-31/ZC-DMC-1536-1024__hash_16384.json` |
| `ZC-DMC-1536-1024` | timing hash_32 | `records/hash-31/ZC-DMC-1536-1024__hash_32.json` |
| `ZC-DMC-1536-1024` | timing hash_4096 | `records/hash-31/ZC-DMC-1536-1024__hash_4096.json` |
| `ZC-DMC-1536-1024` | timing hash_512 | `records/hash-31/ZC-DMC-1536-1024__hash_512.json` |
| `ZC-DMC-1536-1024` | timing hash_65536 | `records/hash-31/ZC-DMC-1536-1024__hash_65536.json` |
| `ZC-DMC-1536-1024` | timing hash_8192 | `records/hash-31/ZC-DMC-1536-1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

