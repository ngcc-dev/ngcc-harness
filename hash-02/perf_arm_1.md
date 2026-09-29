# hash-02 AXIS: Advanced eXpandable Iterative Stream-based Hash — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-02` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101540899009417216.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: AXIS: Advanced eXpandable Iterative Stream-based Hash
- Implementation versions measured: reference
- Parameter sets: `AXIS-512`, `AXIS-768`, `AXIS-1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-02/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `AXIS-512` | guide | PASS |
| `AXIS-768` | guide | PASS |
| `AXIS-1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `AXIS-512` | 32 B | 50.7 k | 1583.0 | 18.8 µs | 1.7 | 100000 (5 × 20000) |
| `AXIS-512` | 128 B | 52.8 k | 412.4 | 19.6 µs | 6.5 | 100000 (5 × 20000) |
| `AXIS-512` | 512 B | 61.4 k | 119.8 | 22.8 µs | 22.5 | 100000 (5 × 20000) |
| `AXIS-512` | 1024 B | 72.9 k | 71.1 | 27.1 µs | 37.8 | 100000 (5 × 20000) |
| `AXIS-512` | 4096 B | 141.3 k | 34.5 | 52.4 µs | 78.1 | 57975 (5 × 11595) |
| `AXIS-512` | 8192 B | 232.6 k | 28.4 | 86.3 µs | 94.9 | 35795 (5 × 7159) |
| `AXIS-512` | 16384 B | 415.3 k | 25.3 | 154 µs | 106.3 | 20255 (5 × 4051) |
| `AXIS-512` | 65536 B | 1.51 M | 23.1 | 561 µs | 116.8 | 5630 (5 × 1126) |
| `AXIS-768` | 32 B | 64.3 k | 2009.9 | 23.9 µs | 1.3 | 92665 (5 × 18533) |
| `AXIS-768` | 128 B | 70.7 k | 552.7 | 26.2 µs | 4.9 | 86335 (5 × 17267) |
| `AXIS-768` | 512 B | 96.8 k | 189.0 | 35.9 µs | 14.3 | 68670 (5 × 13734) |
| `AXIS-768` | 1024 B | 131.5 k | 128.4 | 48.8 µs | 21.0 | 56210 (5 × 11242) |
| `AXIS-768` | 4096 B | 339.9 k | 83.0 | 126 µs | 32.5 | 23670 (5 × 4734) |
| `AXIS-768` | 8192 B | 617.5 k | 75.4 | 229 µs | 35.7 | 13385 (5 × 2677) |
| `AXIS-768` | 16384 B | 1.17 M | 71.6 | 435 µs | 37.6 | 7165 (5 × 1433) |
| `AXIS-768` | 65536 B | 4.51 M | 68.7 | 1.67 ms | 39.2 | 1875 (5 × 375) |
| `AXIS-1024` | 32 B | 84.8 k | 2649.8 | 31.5 µs | 1.0 | 75830 (5 × 15166) |
| `AXIS-1024` | 128 B | 88.4 k | 690.6 | 32.8 µs | 3.9 | 74305 (5 × 14861) |
| `AXIS-1024` | 512 B | 102.4 k | 200.0 | 38 µs | 13.5 | 50690 (5 × 10138) |
| `AXIS-1024` | 1024 B | 120.9 k | 118.1 | 44.9 µs | 22.8 | 59260 (5 × 11852) |
| `AXIS-1024` | 4096 B | 232.5 k | 56.8 | 86.3 µs | 47.5 | 33900 (5 × 6780) |
| `AXIS-1024` | 8192 B | 381.0 k | 46.5 | 141 µs | 57.9 | 21225 (5 × 4245) |
| `AXIS-1024` | 16384 B | 678.5 k | 41.4 | 252 µs | 65.1 | 12235 (5 × 2447) |
| `AXIS-1024` | 65536 B | 2.46 M | 37.6 | 913 µs | 71.7 | 3270 (5 × 654) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `AXIS-512` | hash_32 | 32180 | 1420 KiB | 1484 KiB |
| `AXIS-512` | hash_128 | 32180 | 1420 KiB | 1484 KiB |
| `AXIS-512` | hash_512 | 32180 | 1420 KiB | 1484 KiB |
| `AXIS-512` | hash_1024 | 32180 | 1424 KiB | 1488 KiB |
| `AXIS-512` | hash_4096 | 32180 | 1424 KiB | 1488 KiB |
| `AXIS-512` | hash_8192 | 32180 | 1428 KiB | 1492 KiB |
| `AXIS-512` | hash_16384 | 32180 | 1436 KiB | 1500 KiB |
| `AXIS-512` | hash_65536 | 32180 | 1484 KiB | 1548 KiB |
| `AXIS-768` | hash_32 | 32180 | 1420 KiB | 1484 KiB |
| `AXIS-768` | hash_128 | 32180 | 1420 KiB | 1484 KiB |
| `AXIS-768` | hash_512 | 32180 | 1420 KiB | 1484 KiB |
| `AXIS-768` | hash_1024 | 32180 | 1424 KiB | 1488 KiB |
| `AXIS-768` | hash_4096 | 32180 | 1424 KiB | 1488 KiB |
| `AXIS-768` | hash_8192 | 32180 | 1428 KiB | 1492 KiB |
| `AXIS-768` | hash_16384 | 32180 | 1436 KiB | 1500 KiB |
| `AXIS-768` | hash_65536 | 32180 | 1484 KiB | 1548 KiB |
| `AXIS-1024` | hash_32 | 32196 | 1420 KiB | 1484 KiB |
| `AXIS-1024` | hash_128 | 32196 | 1420 KiB | 1484 KiB |
| `AXIS-1024` | hash_512 | 32196 | 1420 KiB | 1484 KiB |
| `AXIS-1024` | hash_1024 | 32196 | 1424 KiB | 1488 KiB |
| `AXIS-1024` | hash_4096 | 32196 | 1424 KiB | 1488 KiB |
| `AXIS-1024` | hash_8192 | 32196 | 1428 KiB | 1492 KiB |
| `AXIS-1024` | hash_16384 | 32196 | 1436 KiB | 1500 KiB |
| `AXIS-1024` | hash_65536 | 32196 | 1484 KiB | 1548 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `AXIS-512` | KAT log (sha256 `bc8a7db475fd7e61…`) | `kat/hash-02/AXIS-512.log` |
| `AXIS-512` | timing hash_1024 | `records/hash-02/AXIS-512__hash_1024.json` |
| `AXIS-512` | timing hash_128 | `records/hash-02/AXIS-512__hash_128.json` |
| `AXIS-512` | timing hash_16384 | `records/hash-02/AXIS-512__hash_16384.json` |
| `AXIS-512` | timing hash_32 | `records/hash-02/AXIS-512__hash_32.json` |
| `AXIS-512` | timing hash_4096 | `records/hash-02/AXIS-512__hash_4096.json` |
| `AXIS-512` | timing hash_512 | `records/hash-02/AXIS-512__hash_512.json` |
| `AXIS-512` | timing hash_65536 | `records/hash-02/AXIS-512__hash_65536.json` |
| `AXIS-512` | timing hash_8192 | `records/hash-02/AXIS-512__hash_8192.json` |
| `AXIS-768` | KAT log (sha256 `fb945c51403db489…`) | `kat/hash-02/AXIS-768.log` |
| `AXIS-768` | timing hash_1024 | `records/hash-02/AXIS-768__hash_1024.json` |
| `AXIS-768` | timing hash_128 | `records/hash-02/AXIS-768__hash_128.json` |
| `AXIS-768` | timing hash_16384 | `records/hash-02/AXIS-768__hash_16384.json` |
| `AXIS-768` | timing hash_32 | `records/hash-02/AXIS-768__hash_32.json` |
| `AXIS-768` | timing hash_4096 | `records/hash-02/AXIS-768__hash_4096.json` |
| `AXIS-768` | timing hash_512 | `records/hash-02/AXIS-768__hash_512.json` |
| `AXIS-768` | timing hash_65536 | `records/hash-02/AXIS-768__hash_65536.json` |
| `AXIS-768` | timing hash_8192 | `records/hash-02/AXIS-768__hash_8192.json` |
| `AXIS-1024` | KAT log (sha256 `81046c344b0a6f2e…`) | `kat/hash-02/AXIS-1024.log` |
| `AXIS-1024` | timing hash_1024 | `records/hash-02/AXIS-1024__hash_1024.json` |
| `AXIS-1024` | timing hash_128 | `records/hash-02/AXIS-1024__hash_128.json` |
| `AXIS-1024` | timing hash_16384 | `records/hash-02/AXIS-1024__hash_16384.json` |
| `AXIS-1024` | timing hash_32 | `records/hash-02/AXIS-1024__hash_32.json` |
| `AXIS-1024` | timing hash_4096 | `records/hash-02/AXIS-1024__hash_4096.json` |
| `AXIS-1024` | timing hash_512 | `records/hash-02/AXIS-1024__hash_512.json` |
| `AXIS-1024` | timing hash_65536 | `records/hash-02/AXIS-1024__hash_65536.json` |
| `AXIS-1024` | timing hash_8192 | `records/hash-02/AXIS-1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

