# hash-01 AFS-TrEDM — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-01` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101541113569038336.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: AFS-TrEDM
- Implementation versions measured: reference
- Parameter sets: `AFS-TrEDM-512`, `AFS-TrEDM-768`, `AFS-TrEDM-1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-01/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `AFS-TrEDM-512` | guide | PASS |
| `AFS-TrEDM-768` | guide | PASS |
| `AFS-TrEDM-1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `AFS-TrEDM-512` | 32 B | 14.0 k | 436.1 | 5.18 µs | 6.2 | 100000 (5 × 20000) |
| `AFS-TrEDM-512` | 128 B | 27.4 k | 213.7 | 10.2 µs | 12.6 | 100000 (5 × 20000) |
| `AFS-TrEDM-512` | 512 B | 71.3 k | 139.3 | 26.5 µs | 19.3 | 100000 (5 × 20000) |
| `AFS-TrEDM-512` | 1024 B | 125.0 k | 122.1 | 46.4 µs | 22.1 | 62995 (5 × 12599) |
| `AFS-TrEDM-512` | 4096 B | 450.3 k | 109.9 | 167 µs | 24.5 | 18455 (5 × 3691) |
| `AFS-TrEDM-512` | 8192 B | 886.2 k | 108.2 | 329 µs | 24.9 | 8845 (5 × 1769) |
| `AFS-TrEDM-512` | 16384 B | 1.77 M | 108.0 | 657 µs | 24.9 | 4810 (5 × 962) |
| `AFS-TrEDM-512` | 65536 B | 7.01 M | 106.9 | 2.6 ms | 25.2 | 1210 (5 × 242) |
| `AFS-TrEDM-768` | 32 B | 23.0 k | 718.7 | 8.54 µs | 3.7 | 100000 (5 × 20000) |
| `AFS-TrEDM-768` | 128 B | 45.4 k | 354.6 | 16.8 µs | 7.6 | 100000 (5 × 20000) |
| `AFS-TrEDM-768` | 512 B | 136.1 k | 265.9 | 50.5 µs | 10.1 | 58255 (5 × 11651) |
| `AFS-TrEDM-768` | 1024 B | 249.3 k | 243.5 | 92.5 µs | 11.1 | 33405 (5 × 6681) |
| `AFS-TrEDM-768` | 4096 B | 982.0 k | 239.7 | 364 µs | 11.2 | 8725 (5 × 1745) |
| `AFS-TrEDM-768` | 8192 B | 1.98 M | 242.0 | 735 µs | 11.1 | 4295 (5 × 859) |
| `AFS-TrEDM-768` | 16384 B | 3.94 M | 240.5 | 1.46 ms | 11.2 | 2205 (5 × 441) |
| `AFS-TrEDM-768` | 65536 B | 15.48 M | 236.1 | 5.74 ms | 11.4 | 540 (5 × 108) |
| `AFS-TrEDM-1024` | 32 B | 27.8 k | 868.9 | 10.3 µs | 3.1 | 100000 (5 × 20000) |
| `AFS-TrEDM-1024` | 128 B | 82.8 k | 646.8 | 30.7 µs | 4.2 | 88565 (5 × 17713) |
| `AFS-TrEDM-1024` | 512 B | 249.9 k | 488.1 | 92.7 µs | 5.5 | 33520 (5 × 6704) |
| `AFS-TrEDM-1024` | 1024 B | 463.5 k | 452.7 | 172 µs | 6.0 | 18060 (5 × 3612) |
| `AFS-TrEDM-1024` | 4096 B | 1.80 M | 440.0 | 669 µs | 6.1 | 4835 (5 × 967) |
| `AFS-TrEDM-1024` | 8192 B | 3.56 M | 435.2 | 1.32 ms | 6.2 | 2230 (5 × 446) |
| `AFS-TrEDM-1024` | 16384 B | 7.00 M | 427.3 | 2.6 ms | 6.3 | 1180 (5 × 236) |
| `AFS-TrEDM-1024` | 65536 B | 27.94 M | 426.4 | 10.4 ms | 6.3 | 305 (5 × 61) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `AFS-TrEDM-512` | hash_32 | 14356 | 1404 KiB | 1468 KiB |
| `AFS-TrEDM-512` | hash_128 | 14356 | 1404 KiB | 1468 KiB |
| `AFS-TrEDM-512` | hash_512 | 14356 | 1404 KiB | 1468 KiB |
| `AFS-TrEDM-512` | hash_1024 | 14356 | 1408 KiB | 1472 KiB |
| `AFS-TrEDM-512` | hash_4096 | 14356 | 1408 KiB | 1472 KiB |
| `AFS-TrEDM-512` | hash_8192 | 14356 | 1412 KiB | 1476 KiB |
| `AFS-TrEDM-512` | hash_16384 | 14356 | 1420 KiB | 1484 KiB |
| `AFS-TrEDM-512` | hash_65536 | 14356 | 1468 KiB | 1532 KiB |
| `AFS-TrEDM-768` | hash_32 | 14356 | 1404 KiB | 1468 KiB |
| `AFS-TrEDM-768` | hash_128 | 14356 | 1400 KiB | 1464 KiB |
| `AFS-TrEDM-768` | hash_512 | 14356 | 1404 KiB | 1468 KiB |
| `AFS-TrEDM-768` | hash_1024 | 14356 | 1404 KiB | 1468 KiB |
| `AFS-TrEDM-768` | hash_4096 | 14356 | 1408 KiB | 1472 KiB |
| `AFS-TrEDM-768` | hash_8192 | 14356 | 1412 KiB | 1476 KiB |
| `AFS-TrEDM-768` | hash_16384 | 14356 | 1420 KiB | 1484 KiB |
| `AFS-TrEDM-768` | hash_65536 | 14356 | 1468 KiB | 1532 KiB |
| `AFS-TrEDM-1024` | hash_32 | 14420 | 1404 KiB | 1468 KiB |
| `AFS-TrEDM-1024` | hash_128 | 14420 | 1404 KiB | 1468 KiB |
| `AFS-TrEDM-1024` | hash_512 | 14420 | 1404 KiB | 1468 KiB |
| `AFS-TrEDM-1024` | hash_1024 | 14420 | 1408 KiB | 1472 KiB |
| `AFS-TrEDM-1024` | hash_4096 | 14420 | 1404 KiB | 1468 KiB |
| `AFS-TrEDM-1024` | hash_8192 | 14420 | 1412 KiB | 1476 KiB |
| `AFS-TrEDM-1024` | hash_16384 | 14420 | 1420 KiB | 1484 KiB |
| `AFS-TrEDM-1024` | hash_65536 | 14420 | 1468 KiB | 1532 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `AFS-TrEDM-512` | KAT log (sha256 `a139505b25242730…`) | `kat/hash-01/AFS-TrEDM-512.log` |
| `AFS-TrEDM-512` | timing hash_1024 | `records/hash-01/AFS-TrEDM-512__hash_1024.json` |
| `AFS-TrEDM-512` | timing hash_128 | `records/hash-01/AFS-TrEDM-512__hash_128.json` |
| `AFS-TrEDM-512` | timing hash_16384 | `records/hash-01/AFS-TrEDM-512__hash_16384.json` |
| `AFS-TrEDM-512` | timing hash_32 | `records/hash-01/AFS-TrEDM-512__hash_32.json` |
| `AFS-TrEDM-512` | timing hash_4096 | `records/hash-01/AFS-TrEDM-512__hash_4096.json` |
| `AFS-TrEDM-512` | timing hash_512 | `records/hash-01/AFS-TrEDM-512__hash_512.json` |
| `AFS-TrEDM-512` | timing hash_65536 | `records/hash-01/AFS-TrEDM-512__hash_65536.json` |
| `AFS-TrEDM-512` | timing hash_8192 | `records/hash-01/AFS-TrEDM-512__hash_8192.json` |
| `AFS-TrEDM-768` | KAT log (sha256 `a9e3e08821613236…`) | `kat/hash-01/AFS-TrEDM-768.log` |
| `AFS-TrEDM-768` | timing hash_1024 | `records/hash-01/AFS-TrEDM-768__hash_1024.json` |
| `AFS-TrEDM-768` | timing hash_128 | `records/hash-01/AFS-TrEDM-768__hash_128.json` |
| `AFS-TrEDM-768` | timing hash_16384 | `records/hash-01/AFS-TrEDM-768__hash_16384.json` |
| `AFS-TrEDM-768` | timing hash_32 | `records/hash-01/AFS-TrEDM-768__hash_32.json` |
| `AFS-TrEDM-768` | timing hash_4096 | `records/hash-01/AFS-TrEDM-768__hash_4096.json` |
| `AFS-TrEDM-768` | timing hash_512 | `records/hash-01/AFS-TrEDM-768__hash_512.json` |
| `AFS-TrEDM-768` | timing hash_65536 | `records/hash-01/AFS-TrEDM-768__hash_65536.json` |
| `AFS-TrEDM-768` | timing hash_8192 | `records/hash-01/AFS-TrEDM-768__hash_8192.json` |
| `AFS-TrEDM-1024` | KAT log (sha256 `6f126b5de12b8144…`) | `kat/hash-01/AFS-TrEDM-1024.log` |
| `AFS-TrEDM-1024` | timing hash_1024 | `records/hash-01/AFS-TrEDM-1024__hash_1024.json` |
| `AFS-TrEDM-1024` | timing hash_128 | `records/hash-01/AFS-TrEDM-1024__hash_128.json` |
| `AFS-TrEDM-1024` | timing hash_16384 | `records/hash-01/AFS-TrEDM-1024__hash_16384.json` |
| `AFS-TrEDM-1024` | timing hash_32 | `records/hash-01/AFS-TrEDM-1024__hash_32.json` |
| `AFS-TrEDM-1024` | timing hash_4096 | `records/hash-01/AFS-TrEDM-1024__hash_4096.json` |
| `AFS-TrEDM-1024` | timing hash_512 | `records/hash-01/AFS-TrEDM-1024__hash_512.json` |
| `AFS-TrEDM-1024` | timing hash_65536 | `records/hash-01/AFS-TrEDM-1024__hash_65536.json` |
| `AFS-TrEDM-1024` | timing hash_8192 | `records/hash-01/AFS-TrEDM-1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

