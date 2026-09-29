# hash-19 MoFang Hash Function — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-19` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101537228439769088.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: MoFang Hash Function
- Implementation versions measured: reference
- Parameter sets: `MoFang-256`, `MoFang-256-XOF`, `MoFang-512`, `MoFang-768`, `MoFang-768-XOF`, `MoFang-1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-19/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `MoFang-256` | guide | PASS |
| `MoFang-256-XOF` | guide | PASS |
| `MoFang-512` | guide | PASS |
| `MoFang-768` | guide | PASS |
| `MoFang-768-XOF` | guide | PASS |
| `MoFang-1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `MoFang-256` | 32 B | 1665 | 52.0 | 619 ns | 51.7 | 100000 (5 × 20000) |
| `MoFang-256` | 128 B | 1662 | 13.0 | 619 ns | 206.9 | 100000 (5 × 20000) |
| `MoFang-256` | 512 B | 6423 | 12.5 | 2.39 µs | 214.7 | 100000 (5 × 20000) |
| `MoFang-256` | 1024 B | 12.9 k | 12.6 | 4.78 µs | 214.2 | 100000 (5 × 20000) |
| `MoFang-256` | 4096 B | 55.1 k | 13.5 | 20.5 µs | 200.2 | 100000 (5 × 20000) |
| `MoFang-256` | 8192 B | 91.9 k | 11.2 | 34.1 µs | 240.1 | 85870 (5 × 17174) |
| `MoFang-256` | 16384 B | 183.0 k | 11.2 | 67.9 µs | 241.3 | 43960 (5 × 8792) |
| `MoFang-256` | 65536 B | 735.9 k | 11.2 | 273 µs | 239.9 | 11130 (5 × 2226) |
| `MoFang-256-XOF` | 32 B | 1775 | 55.5 | 660 ns | 48.5 | 100000 (5 × 20000) |
| `MoFang-256-XOF` | 128 B | 1881 | 14.7 | 700 ns | 182.9 | 100000 (5 × 20000) |
| `MoFang-256-XOF` | 512 B | 6581 | 12.9 | 2.44 µs | 209.5 | 100000 (5 × 20000) |
| `MoFang-256-XOF` | 1024 B | 12.9 k | 12.6 | 4.8 µs | 213.3 | 100000 (5 × 20000) |
| `MoFang-256-XOF` | 4096 B | 47.0 k | 11.5 | 17.4 µs | 235.0 | 100000 (5 × 20000) |
| `MoFang-256-XOF` | 8192 B | 92.3 k | 11.3 | 34.2 µs | 239.3 | 84215 (5 × 16843) |
| `MoFang-256-XOF` | 16384 B | 184.3 k | 11.2 | 68.4 µs | 239.6 | 43480 (5 × 8696) |
| `MoFang-256-XOF` | 65536 B | 861.0 k | 13.1 | 319 µs | 205.1 | 10925 (5 × 2185) |
| `MoFang-512` | 32 B | 1706 | 53.3 | 635 ns | 50.4 | 100000 (5 × 20000) |
| `MoFang-512` | 128 B | 1870 | 14.6 | 695 ns | 184.0 | 100000 (5 × 20000) |
| `MoFang-512` | 512 B | 6446 | 12.6 | 2.39 µs | 213.9 | 100000 (5 × 20000) |
| `MoFang-512` | 1024 B | 12.9 k | 12.6 | 4.79 µs | 213.8 | 100000 (5 × 20000) |
| `MoFang-512` | 4096 B | 46.6 k | 11.4 | 17.3 µs | 237.0 | 100000 (5 × 20000) |
| `MoFang-512` | 8192 B | 91.3 k | 11.2 | 33.9 µs | 241.6 | 73510 (5 × 14702) |
| `MoFang-512` | 16384 B | 183.4 k | 11.2 | 68 µs | 240.8 | 21325 (5 × 4265) |
| `MoFang-512` | 65536 B | 733.3 k | 11.2 | 272 µs | 240.8 | 10655 (5 × 2131) |
| `MoFang-768` | 32 B | 3485 | 108.9 | 1.3 µs | 24.7 | 100000 (5 × 20000) |
| `MoFang-768` | 128 B | 3497 | 27.3 | 1.3 µs | 98.4 | 100000 (5 × 20000) |
| `MoFang-768` | 512 B | 13.7 k | 26.7 | 5.08 µs | 100.8 | 100000 (5 × 20000) |
| `MoFang-768` | 1024 B | 26.8 k | 26.1 | 9.94 µs | 103.1 | 100000 (5 × 20000) |
| `MoFang-768` | 4096 B | 99.1 k | 24.2 | 36.8 µs | 111.4 | 73620 (5 × 14724) |
| `MoFang-768` | 8192 B | 192.5 k | 23.5 | 71.4 µs | 114.7 | 42555 (5 × 8511) |
| `MoFang-768` | 16384 B | 378.9 k | 23.1 | 141 µs | 116.5 | 20920 (5 × 4184) |
| `MoFang-768` | 65536 B | 1.55 M | 23.6 | 574 µs | 114.1 | 5470 (5 × 1094) |
| `MoFang-768-XOF` | 32 B | 3828 | 119.6 | 1.43 µs | 22.4 | 100000 (5 × 20000) |
| `MoFang-768-XOF` | 128 B | 3817 | 29.8 | 1.42 µs | 90.3 | 100000 (5 × 20000) |
| `MoFang-768-XOF` | 512 B | 13.8 k | 26.9 | 5.11 µs | 100.1 | 100000 (5 × 20000) |
| `MoFang-768-XOF` | 1024 B | 28.6 k | 27.9 | 10.6 µs | 96.4 | 100000 (5 × 20000) |
| `MoFang-768-XOF` | 4096 B | 96.9 k | 23.6 | 35.9 µs | 114.0 | 81495 (5 × 16299) |
| `MoFang-768-XOF` | 8192 B | 189.8 k | 23.2 | 70.4 µs | 116.3 | 38130 (5 × 7626) |
| `MoFang-768-XOF` | 16384 B | 378.7 k | 23.1 | 141 µs | 116.6 | 21900 (5 × 4380) |
| `MoFang-768-XOF` | 65536 B | 1.52 M | 23.1 | 563 µs | 116.4 | 5505 (5 × 1101) |
| `MoFang-1024` | 32 B | 3534 | 110.4 | 1.31 µs | 24.4 | 100000 (5 × 20000) |
| `MoFang-1024` | 128 B | 3527 | 27.6 | 1.31 µs | 97.7 | 100000 (5 × 20000) |
| `MoFang-1024` | 512 B | 13.5 k | 26.4 | 5.01 µs | 102.1 | 100000 (5 × 20000) |
| `MoFang-1024` | 1024 B | 28.5 k | 27.8 | 10.6 µs | 97.0 | 100000 (5 × 20000) |
| `MoFang-1024` | 4096 B | 100.0 k | 24.4 | 37.1 µs | 110.4 | 82335 (5 × 16467) |
| `MoFang-1024` | 8192 B | 188.7 k | 23.0 | 70 µs | 117.0 | 39900 (5 × 7980) |
| `MoFang-1024` | 16384 B | 476.6 k | 29.1 | 177 µs | 92.7 | 20900 (5 × 4180) |
| `MoFang-1024` | 65536 B | 1.59 M | 24.3 | 591 µs | 110.9 | 5390 (5 × 1078) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `MoFang-256` | hash_32 | 11620 | 1400 KiB | 1464 KiB |
| `MoFang-256` | hash_128 | 11620 | 1400 KiB | 1464 KiB |
| `MoFang-256` | hash_512 | 11620 | 1400 KiB | 1468 KiB |
| `MoFang-256` | hash_1024 | 11620 | 1404 KiB | 1468 KiB |
| `MoFang-256` | hash_4096 | 11620 | 1404 KiB | 1472 KiB |
| `MoFang-256` | hash_8192 | 11620 | 1408 KiB | 1480 KiB |
| `MoFang-256` | hash_16384 | 11620 | 1416 KiB | 1496 KiB |
| `MoFang-256` | hash_65536 | 11620 | 1464 KiB | 1592 KiB |
| `MoFang-256-XOF` | hash_32 | 11756 | 1400 KiB | 1464 KiB |
| `MoFang-256-XOF` | hash_128 | 11756 | 1400 KiB | 1464 KiB |
| `MoFang-256-XOF` | hash_512 | 11756 | 3432 KiB | 3496 KiB |
| `MoFang-256-XOF` | hash_1024 | 11756 | 1404 KiB | 1468 KiB |
| `MoFang-256-XOF` | hash_4096 | 11756 | 1404 KiB | 1472 KiB |
| `MoFang-256-XOF` | hash_8192 | 11756 | 1408 KiB | 1480 KiB |
| `MoFang-256-XOF` | hash_16384 | 11756 | 1416 KiB | 1496 KiB |
| `MoFang-256-XOF` | hash_65536 | 11756 | 3436 KiB | 3500 KiB |
| `MoFang-512` | hash_32 | 11620 | 1400 KiB | 1464 KiB |
| `MoFang-512` | hash_128 | 11620 | 1400 KiB | 1464 KiB |
| `MoFang-512` | hash_512 | 11620 | 1400 KiB | 1468 KiB |
| `MoFang-512` | hash_1024 | 11620 | 1404 KiB | 1468 KiB |
| `MoFang-512` | hash_4096 | 11620 | 1404 KiB | 1472 KiB |
| `MoFang-512` | hash_8192 | 11620 | 1408 KiB | 1480 KiB |
| `MoFang-512` | hash_16384 | 11620 | 1412 KiB | 1492 KiB |
| `MoFang-512` | hash_65536 | 11620 | 3504 KiB | 3568 KiB |
| `MoFang-768` | hash_32 | 12396 | 1400 KiB | 1464 KiB |
| `MoFang-768` | hash_128 | 12396 | 1400 KiB | 1464 KiB |
| `MoFang-768` | hash_512 | 12396 | 1400 KiB | 1468 KiB |
| `MoFang-768` | hash_1024 | 12396 | 1404 KiB | 1468 KiB |
| `MoFang-768` | hash_4096 | 12396 | 1404 KiB | 1472 KiB |
| `MoFang-768` | hash_8192 | 12396 | 1408 KiB | 1480 KiB |
| `MoFang-768` | hash_16384 | 12396 | 1416 KiB | 1496 KiB |
| `MoFang-768` | hash_65536 | 12396 | 1464 KiB | 1592 KiB |
| `MoFang-768-XOF` | hash_32 | 12660 | 1400 KiB | 1464 KiB |
| `MoFang-768-XOF` | hash_128 | 12660 | 1400 KiB | 1464 KiB |
| `MoFang-768-XOF` | hash_512 | 12660 | 1400 KiB | 1468 KiB |
| `MoFang-768-XOF` | hash_1024 | 12660 | 1404 KiB | 1468 KiB |
| `MoFang-768-XOF` | hash_4096 | 12660 | 1404 KiB | 1472 KiB |
| `MoFang-768-XOF` | hash_8192 | 12660 | 1408 KiB | 1480 KiB |
| `MoFang-768-XOF` | hash_16384 | 12660 | 1416 KiB | 1496 KiB |
| `MoFang-768-XOF` | hash_65536 | 12660 | 1464 KiB | 1592 KiB |
| `MoFang-1024` | hash_32 | 12396 | 1400 KiB | 1464 KiB |
| `MoFang-1024` | hash_128 | 12396 | 1400 KiB | 1464 KiB |
| `MoFang-1024` | hash_512 | 12396 | 1400 KiB | 1468 KiB |
| `MoFang-1024` | hash_1024 | 12396 | 1404 KiB | 1468 KiB |
| `MoFang-1024` | hash_4096 | 12396 | 1404 KiB | 1472 KiB |
| `MoFang-1024` | hash_8192 | 12396 | 1408 KiB | 1480 KiB |
| `MoFang-1024` | hash_16384 | 12396 | 1416 KiB | 1496 KiB |
| `MoFang-1024` | hash_65536 | 12396 | 1464 KiB | 1592 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `MoFang-256` | KAT log (sha256 `1301ce73ce65c4f8…`) | `kat/hash-19/MoFang-256.log` |
| `MoFang-256` | timing hash_1024 | `records/hash-19/MoFang-256__hash_1024.json` |
| `MoFang-256` | timing hash_128 | `records/hash-19/MoFang-256__hash_128.json` |
| `MoFang-256` | timing hash_16384 | `records/hash-19/MoFang-256__hash_16384.json` |
| `MoFang-256` | timing hash_32 | `records/hash-19/MoFang-256__hash_32.json` |
| `MoFang-256` | timing hash_4096 | `records/hash-19/MoFang-256__hash_4096.json` |
| `MoFang-256` | timing hash_512 | `records/hash-19/MoFang-256__hash_512.json` |
| `MoFang-256` | timing hash_65536 | `records/hash-19/MoFang-256__hash_65536.json` |
| `MoFang-256` | timing hash_8192 | `records/hash-19/MoFang-256__hash_8192.json` |
| `MoFang-256-XOF` | KAT log (sha256 `b84814b3f6993d53…`) | `kat/hash-19/MoFang-256-XOF.log` |
| `MoFang-256-XOF` | timing hash_1024 | `records/hash-19/MoFang-256-XOF__hash_1024.json` |
| `MoFang-256-XOF` | timing hash_128 | `records/hash-19/MoFang-256-XOF__hash_128.json` |
| `MoFang-256-XOF` | timing hash_16384 | `records/hash-19/MoFang-256-XOF__hash_16384.json` |
| `MoFang-256-XOF` | timing hash_32 | `records/hash-19/MoFang-256-XOF__hash_32.json` |
| `MoFang-256-XOF` | timing hash_4096 | `records/hash-19/MoFang-256-XOF__hash_4096.json` |
| `MoFang-256-XOF` | timing hash_512 | `records/hash-19/MoFang-256-XOF__hash_512.json` |
| `MoFang-256-XOF` | timing hash_65536 | `records/hash-19/MoFang-256-XOF__hash_65536.json` |
| `MoFang-256-XOF` | timing hash_8192 | `records/hash-19/MoFang-256-XOF__hash_8192.json` |
| `MoFang-512` | KAT log (sha256 `3da7e7b15c02bceb…`) | `kat/hash-19/MoFang-512.log` |
| `MoFang-512` | timing hash_1024 | `records/hash-19/MoFang-512__hash_1024.json` |
| `MoFang-512` | timing hash_128 | `records/hash-19/MoFang-512__hash_128.json` |
| `MoFang-512` | timing hash_16384 | `records/hash-19/MoFang-512__hash_16384.json` |
| `MoFang-512` | timing hash_32 | `records/hash-19/MoFang-512__hash_32.json` |
| `MoFang-512` | timing hash_4096 | `records/hash-19/MoFang-512__hash_4096.json` |
| `MoFang-512` | timing hash_512 | `records/hash-19/MoFang-512__hash_512.json` |
| `MoFang-512` | timing hash_65536 | `records/hash-19/MoFang-512__hash_65536.json` |
| `MoFang-512` | timing hash_8192 | `records/hash-19/MoFang-512__hash_8192.json` |
| `MoFang-768` | KAT log (sha256 `ed0b2cea1412ce09…`) | `kat/hash-19/MoFang-768.log` |
| `MoFang-768` | timing hash_1024 | `records/hash-19/MoFang-768__hash_1024.json` |
| `MoFang-768` | timing hash_128 | `records/hash-19/MoFang-768__hash_128.json` |
| `MoFang-768` | timing hash_16384 | `records/hash-19/MoFang-768__hash_16384.json` |
| `MoFang-768` | timing hash_32 | `records/hash-19/MoFang-768__hash_32.json` |
| `MoFang-768` | timing hash_4096 | `records/hash-19/MoFang-768__hash_4096.json` |
| `MoFang-768` | timing hash_512 | `records/hash-19/MoFang-768__hash_512.json` |
| `MoFang-768` | timing hash_65536 | `records/hash-19/MoFang-768__hash_65536.json` |
| `MoFang-768` | timing hash_8192 | `records/hash-19/MoFang-768__hash_8192.json` |
| `MoFang-768-XOF` | KAT log (sha256 `75e0208400114d1e…`) | `kat/hash-19/MoFang-768-XOF.log` |
| `MoFang-768-XOF` | timing hash_1024 | `records/hash-19/MoFang-768-XOF__hash_1024.json` |
| `MoFang-768-XOF` | timing hash_128 | `records/hash-19/MoFang-768-XOF__hash_128.json` |
| `MoFang-768-XOF` | timing hash_16384 | `records/hash-19/MoFang-768-XOF__hash_16384.json` |
| `MoFang-768-XOF` | timing hash_32 | `records/hash-19/MoFang-768-XOF__hash_32.json` |
| `MoFang-768-XOF` | timing hash_4096 | `records/hash-19/MoFang-768-XOF__hash_4096.json` |
| `MoFang-768-XOF` | timing hash_512 | `records/hash-19/MoFang-768-XOF__hash_512.json` |
| `MoFang-768-XOF` | timing hash_65536 | `records/hash-19/MoFang-768-XOF__hash_65536.json` |
| `MoFang-768-XOF` | timing hash_8192 | `records/hash-19/MoFang-768-XOF__hash_8192.json` |
| `MoFang-1024` | KAT log (sha256 `2c581322d1c6aa95…`) | `kat/hash-19/MoFang-1024.log` |
| `MoFang-1024` | timing hash_1024 | `records/hash-19/MoFang-1024__hash_1024.json` |
| `MoFang-1024` | timing hash_128 | `records/hash-19/MoFang-1024__hash_128.json` |
| `MoFang-1024` | timing hash_16384 | `records/hash-19/MoFang-1024__hash_16384.json` |
| `MoFang-1024` | timing hash_32 | `records/hash-19/MoFang-1024__hash_32.json` |
| `MoFang-1024` | timing hash_4096 | `records/hash-19/MoFang-1024__hash_4096.json` |
| `MoFang-1024` | timing hash_512 | `records/hash-19/MoFang-1024__hash_512.json` |
| `MoFang-1024` | timing hash_65536 | `records/hash-19/MoFang-1024__hash_65536.json` |
| `MoFang-1024` | timing hash_8192 | `records/hash-19/MoFang-1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

