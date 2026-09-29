# hash-27 The Vedak Hash Function Family — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-27` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101535609849466880.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: The Vedak Hash Function Family
- Implementation versions measured: reference
- Parameter sets: `Vedak-512`, `Vedak-768`, `Vedak-1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-27/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Vedak-512` | guide | PASS |
| `Vedak-768` | guide | PASS |
| `Vedak-1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `Vedak-512` | 32 B | 16.9 k | 527.9 | 6.27 µs | 5.1 | 100000 (5 × 20000) |
| `Vedak-512` | 128 B | 23.1 k | 180.4 | 8.57 µs | 14.9 | 100000 (5 × 20000) |
| `Vedak-512` | 512 B | 77.3 k | 151.1 | 28.7 µs | 17.8 | 99380 (5 × 19876) |
| `Vedak-512` | 1024 B | 154.7 k | 151.1 | 57.4 µs | 17.8 | 53160 (5 × 10632) |
| `Vedak-512` | 4096 B | 593.2 k | 144.8 | 220 µs | 18.6 | 14265 (5 × 2853) |
| `Vedak-512` | 8192 B | 1.17 M | 143.0 | 435 µs | 18.8 | 7105 (5 × 1421) |
| `Vedak-512` | 16384 B | 2.35 M | 143.1 | 870 µs | 18.8 | 3605 (5 × 721) |
| `Vedak-512` | 65536 B | 9.34 M | 142.5 | 3.47 ms | 18.9 | 880 (5 × 176) |
| `Vedak-768` | 32 B | 16.9 k | 527.4 | 6.26 µs | 5.1 | 100000 (5 × 20000) |
| `Vedak-768` | 128 B | 37.7 k | 294.9 | 14 µs | 9.1 | 100000 (5 × 20000) |
| `Vedak-768` | 512 B | 106.7 k | 208.4 | 39.6 µs | 12.9 | 74420 (5 × 14884) |
| `Vedak-768` | 1024 B | 198.4 k | 193.7 | 73.6 µs | 13.9 | 41635 (5 × 8327) |
| `Vedak-768` | 4096 B | 754.9 k | 184.3 | 280 µs | 14.6 | 10870 (5 × 2174) |
| `Vedak-768` | 8192 B | 1.49 M | 182.3 | 554 µs | 14.8 | 5605 (5 × 1121) |
| `Vedak-768` | 16384 B | 3.40 M | 207.8 | 1.26 ms | 13.0 | 2590 (5 × 518) |
| `Vedak-768` | 65536 B | 11.86 M | 181.0 | 4.4 ms | 14.9 | 715 (5 × 143) |
| `Vedak-1024` | 32 B | 31.6 k | 986.2 | 11.7 µs | 2.7 | 100000 (5 × 20000) |
| `Vedak-1024` | 128 B | 67.2 k | 525.0 | 24.9 µs | 5.1 | 100000 (5 × 20000) |
| `Vedak-1024` | 512 B | 180.1 k | 351.8 | 66.8 µs | 7.7 | 45370 (5 × 9074) |
| `Vedak-1024` | 1024 B | 330.9 k | 323.1 | 123 µs | 8.3 | 25160 (5 × 5032) |
| `Vedak-1024` | 4096 B | 1.24 M | 302.6 | 460 µs | 8.9 | 6850 (5 × 1370) |
| `Vedak-1024` | 8192 B | 2.45 M | 299.5 | 910 µs | 9.0 | 3310 (5 × 662) |
| `Vedak-1024` | 16384 B | 4.87 M | 297.3 | 1.81 ms | 9.1 | 1745 (5 × 349) |
| `Vedak-1024` | 65536 B | 19.41 M | 296.1 | 7.2 ms | 9.1 | 435 (5 × 87) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Vedak-512` | hash_32 | 12660 | 1396 KiB | 1460 KiB |
| `Vedak-512` | hash_128 | 12660 | 1400 KiB | 1464 KiB |
| `Vedak-512` | hash_512 | 12660 | 1400 KiB | 1468 KiB |
| `Vedak-512` | hash_1024 | 12660 | 1404 KiB | 1468 KiB |
| `Vedak-512` | hash_4096 | 12660 | 1404 KiB | 1472 KiB |
| `Vedak-512` | hash_8192 | 12660 | 1408 KiB | 1480 KiB |
| `Vedak-512` | hash_16384 | 12660 | 1416 KiB | 1496 KiB |
| `Vedak-512` | hash_65536 | 12660 | 3444 KiB | 3508 KiB |
| `Vedak-768` | hash_32 | 12660 | 1400 KiB | 1464 KiB |
| `Vedak-768` | hash_128 | 12660 | 1400 KiB | 1464 KiB |
| `Vedak-768` | hash_512 | 12660 | 1400 KiB | 3512 KiB |
| `Vedak-768` | hash_1024 | 12660 | 1404 KiB | 1468 KiB |
| `Vedak-768` | hash_4096 | 12660 | 1404 KiB | 1472 KiB |
| `Vedak-768` | hash_8192 | 12660 | 3432 KiB | 3496 KiB |
| `Vedak-768` | hash_16384 | 12660 | 1416 KiB | 1496 KiB |
| `Vedak-768` | hash_65536 | 12660 | 1464 KiB | 1592 KiB |
| `Vedak-1024` | hash_32 | 12660 | 1400 KiB | 1464 KiB |
| `Vedak-1024` | hash_128 | 12660 | 1400 KiB | 1464 KiB |
| `Vedak-1024` | hash_512 | 12660 | 1400 KiB | 1468 KiB |
| `Vedak-1024` | hash_1024 | 12660 | 1404 KiB | 1468 KiB |
| `Vedak-1024` | hash_4096 | 12660 | 1404 KiB | 1472 KiB |
| `Vedak-1024` | hash_8192 | 12660 | 1408 KiB | 1480 KiB |
| `Vedak-1024` | hash_16384 | 12660 | 1416 KiB | 1496 KiB |
| `Vedak-1024` | hash_65536 | 12660 | 3480 KiB | 3544 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Vedak-512` | KAT log (sha256 `4b66af292f3726a7…`) | `kat/hash-27/Vedak-512.log` |
| `Vedak-512` | timing hash_1024 | `records/hash-27/Vedak-512__hash_1024.json` |
| `Vedak-512` | timing hash_128 | `records/hash-27/Vedak-512__hash_128.json` |
| `Vedak-512` | timing hash_16384 | `records/hash-27/Vedak-512__hash_16384.json` |
| `Vedak-512` | timing hash_32 | `records/hash-27/Vedak-512__hash_32.json` |
| `Vedak-512` | timing hash_4096 | `records/hash-27/Vedak-512__hash_4096.json` |
| `Vedak-512` | timing hash_512 | `records/hash-27/Vedak-512__hash_512.json` |
| `Vedak-512` | timing hash_65536 | `records/hash-27/Vedak-512__hash_65536.json` |
| `Vedak-512` | timing hash_8192 | `records/hash-27/Vedak-512__hash_8192.json` |
| `Vedak-768` | KAT log (sha256 `fa4ab72b5a26343e…`) | `kat/hash-27/Vedak-768.log` |
| `Vedak-768` | timing hash_1024 | `records/hash-27/Vedak-768__hash_1024.json` |
| `Vedak-768` | timing hash_128 | `records/hash-27/Vedak-768__hash_128.json` |
| `Vedak-768` | timing hash_16384 | `records/hash-27/Vedak-768__hash_16384.json` |
| `Vedak-768` | timing hash_32 | `records/hash-27/Vedak-768__hash_32.json` |
| `Vedak-768` | timing hash_4096 | `records/hash-27/Vedak-768__hash_4096.json` |
| `Vedak-768` | timing hash_512 | `records/hash-27/Vedak-768__hash_512.json` |
| `Vedak-768` | timing hash_65536 | `records/hash-27/Vedak-768__hash_65536.json` |
| `Vedak-768` | timing hash_8192 | `records/hash-27/Vedak-768__hash_8192.json` |
| `Vedak-1024` | KAT log (sha256 `0e18588bb850b98f…`) | `kat/hash-27/Vedak-1024.log` |
| `Vedak-1024` | timing hash_1024 | `records/hash-27/Vedak-1024__hash_1024.json` |
| `Vedak-1024` | timing hash_128 | `records/hash-27/Vedak-1024__hash_128.json` |
| `Vedak-1024` | timing hash_16384 | `records/hash-27/Vedak-1024__hash_16384.json` |
| `Vedak-1024` | timing hash_32 | `records/hash-27/Vedak-1024__hash_32.json` |
| `Vedak-1024` | timing hash_4096 | `records/hash-27/Vedak-1024__hash_4096.json` |
| `Vedak-1024` | timing hash_512 | `records/hash-27/Vedak-1024__hash_512.json` |
| `Vedak-1024` | timing hash_65536 | `records/hash-27/Vedak-1024__hash_65536.json` |
| `Vedak-1024` | timing hash_8192 | `records/hash-27/Vedak-1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

