# hash-03 C Hash — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-03` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101540728942972928.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: C Hash
- Implementation versions measured: reference
- Parameter sets: `CHash_512`, `CHash_1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-03/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `CHash_512` | guide | PASS |
| `CHash_1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `CHash_512` | 32 B | 210.0 k | 6563.4 | 77.9 µs | 0.4 | 35375 (5 × 7075) |
| `CHash_512` | 128 B | 210.1 k | 1641.8 | 78 µs | 1.6 | 38525 (5 × 7705) |
| `CHash_512` | 512 B | 842.9 k | 1646.2 | 313 µs | 1.6 | 9690 (5 × 1938) |
| `CHash_512` | 1024 B | 1.47 M | 1440.0 | 547 µs | 1.9 | 5785 (5 × 1157) |
| `CHash_512` | 4096 B | 4.95 M | 1208.3 | 1.84 ms | 2.2 | 1720 (5 × 344) |
| `CHash_512` | 8192 B | 9.69 M | 1182.6 | 3.59 ms | 2.3 | 875 (5 × 175) |
| `CHash_512` | 16384 B | 19.06 M | 1163.6 | 7.07 ms | 2.3 | 450 (5 × 90) |
| `CHash_512` | 65536 B | 75.29 M | 1148.9 | 27.9 ms | 2.3 | 115 (5 × 23) |
| `CHash_1024` | 32 B | 209.9 k | 6560.3 | 77.9 µs | 0.4 | 36310 (5 × 7262) |
| `CHash_1024` | 128 B | 210.0 k | 1640.5 | 77.9 µs | 1.6 | 37385 (5 × 7477) |
| `CHash_1024` | 512 B | 843.0 k | 1646.4 | 313 µs | 1.6 | 9840 (5 × 1968) |
| `CHash_1024` | 1024 B | 1.48 M | 1441.1 | 548 µs | 1.9 | 5675 (5 × 1135) |
| `CHash_1024` | 4096 B | 4.95 M | 1208.6 | 1.84 ms | 2.2 | 1715 (5 × 343) |
| `CHash_1024` | 8192 B | 9.69 M | 1183.0 | 3.6 ms | 2.3 | 875 (5 × 175) |
| `CHash_1024` | 16384 B | 19.08 M | 1164.4 | 7.08 ms | 2.3 | 445 (5 × 89) |
| `CHash_1024` | 65536 B | 75.33 M | 1149.4 | 27.9 ms | 2.3 | 115 (5 × 23) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `CHash_512` | hash_32 | 14484 | 1404 KiB | 1468 KiB |
| `CHash_512` | hash_128 | 14484 | 1404 KiB | 1468 KiB |
| `CHash_512` | hash_512 | 14484 | 1404 KiB | 1472 KiB |
| `CHash_512` | hash_1024 | 14484 | 1408 KiB | 1472 KiB |
| `CHash_512` | hash_4096 | 14484 | 1404 KiB | 1472 KiB |
| `CHash_512` | hash_8192 | 14484 | 1412 KiB | 1484 KiB |
| `CHash_512` | hash_16384 | 14484 | 1420 KiB | 1500 KiB |
| `CHash_512` | hash_65536 | 14484 | 1468 KiB | 1596 KiB |
| `CHash_1024` | hash_32 | 14484 | 1404 KiB | 1468 KiB |
| `CHash_1024` | hash_128 | 14484 | 1404 KiB | 1468 KiB |
| `CHash_1024` | hash_512 | 14484 | 1404 KiB | 1472 KiB |
| `CHash_1024` | hash_1024 | 14484 | 1408 KiB | 1472 KiB |
| `CHash_1024` | hash_4096 | 14484 | 1408 KiB | 1476 KiB |
| `CHash_1024` | hash_8192 | 14484 | 1412 KiB | 1484 KiB |
| `CHash_1024` | hash_16384 | 14484 | 1420 KiB | 1500 KiB |
| `CHash_1024` | hash_65536 | 14484 | 1468 KiB | 3624 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `CHash_512` | KAT log (sha256 `71a608714d05cd9f…`) | `kat/hash-03/CHash_512.log` |
| `CHash_512` | timing hash_1024 | `records/hash-03/CHash_512__hash_1024.json` |
| `CHash_512` | timing hash_128 | `records/hash-03/CHash_512__hash_128.json` |
| `CHash_512` | timing hash_16384 | `records/hash-03/CHash_512__hash_16384.json` |
| `CHash_512` | timing hash_32 | `records/hash-03/CHash_512__hash_32.json` |
| `CHash_512` | timing hash_4096 | `records/hash-03/CHash_512__hash_4096.json` |
| `CHash_512` | timing hash_512 | `records/hash-03/CHash_512__hash_512.json` |
| `CHash_512` | timing hash_65536 | `records/hash-03/CHash_512__hash_65536.json` |
| `CHash_512` | timing hash_8192 | `records/hash-03/CHash_512__hash_8192.json` |
| `CHash_1024` | KAT log (sha256 `9bd96d2af2cf58f4…`) | `kat/hash-03/CHash_1024.log` |
| `CHash_1024` | timing hash_1024 | `records/hash-03/CHash_1024__hash_1024.json` |
| `CHash_1024` | timing hash_128 | `records/hash-03/CHash_1024__hash_128.json` |
| `CHash_1024` | timing hash_16384 | `records/hash-03/CHash_1024__hash_16384.json` |
| `CHash_1024` | timing hash_32 | `records/hash-03/CHash_1024__hash_32.json` |
| `CHash_1024` | timing hash_4096 | `records/hash-03/CHash_1024__hash_4096.json` |
| `CHash_1024` | timing hash_512 | `records/hash-03/CHash_1024__hash_512.json` |
| `CHash_1024` | timing hash_65536 | `records/hash-03/CHash_1024__hash_65536.json` |
| `CHash_1024` | timing hash_8192 | `records/hash-03/CHash_1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

