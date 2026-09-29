# hash-33 Thunder Hash Family — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-33` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101533797931110400.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: Thunder Hash Family
- Implementation versions measured: reference
- Parameter sets: `Thunder-512`, `Thunder-768`, `Thunder-1024`, `Thunder-XOF-256`, `Thunder-XOF-384`, `Thunder-XOF-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-33/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Thunder-512` | guide | PASS |
| `Thunder-768` | guide | PASS |
| `Thunder-1024` | guide | PASS |
| `Thunder-XOF-256` | guide | PASS |
| `Thunder-XOF-384` | guide | PASS |
| `Thunder-XOF-512` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `Thunder-512` | 32 B | 2695 | 84.2 | 1 µs | 32.0 | 100000 (5 × 20000) |
| `Thunder-512` | 128 B | 5346 | 41.8 | 1.99 µs | 64.4 | 100000 (5 × 20000) |
| `Thunder-512` | 512 B | 13.3 k | 26.0 | 4.94 µs | 103.6 | 100000 (5 × 20000) |
| `Thunder-512` | 1024 B | 23.9 k | 23.4 | 8.88 µs | 115.3 | 100000 (5 × 20000) |
| `Thunder-512` | 4096 B | 87.6 k | 21.4 | 32.5 µs | 126.0 | 90740 (5 × 18148) |
| `Thunder-512` | 8192 B | 172.5 k | 21.1 | 64 µs | 128.0 | 47905 (5 × 9581) |
| `Thunder-512` | 16384 B | 369.9 k | 22.6 | 137 µs | 119.4 | 24655 (5 × 4931) |
| `Thunder-512` | 65536 B | 1.36 M | 20.8 | 505 µs | 129.7 | 6265 (5 × 1253) |
| `Thunder-768` | 32 B | 2689 | 84.0 | 999 ns | 32.0 | 100000 (5 × 20000) |
| `Thunder-768` | 128 B | 5355 | 41.8 | 1.99 µs | 64.3 | 100000 (5 × 20000) |
| `Thunder-768` | 512 B | 16.0 k | 31.2 | 5.92 µs | 86.5 | 100000 (5 × 20000) |
| `Thunder-768` | 1024 B | 29.2 k | 28.5 | 10.8 µs | 94.5 | 100000 (5 × 20000) |
| `Thunder-768` | 4096 B | 120.1 k | 29.3 | 44.5 µs | 92.0 | 70485 (5 × 14097) |
| `Thunder-768` | 8192 B | 227.8 k | 27.8 | 84.5 µs | 96.9 | 36840 (5 × 7368) |
| `Thunder-768` | 16384 B | 477.4 k | 29.1 | 177 µs | 92.5 | 18695 (5 × 3739) |
| `Thunder-768` | 65536 B | 1.81 M | 27.6 | 671 µs | 97.7 | 4710 (5 × 942) |
| `Thunder-1024` | 32 B | 2724 | 85.1 | 1.01 µs | 31.6 | 100000 (5 × 20000) |
| `Thunder-1024` | 128 B | 8041 | 62.8 | 2.99 µs | 42.9 | 100000 (5 × 20000) |
| `Thunder-1024` | 512 B | 23.9 k | 46.6 | 8.85 µs | 57.8 | 100000 (5 × 20000) |
| `Thunder-1024` | 1024 B | 45.0 k | 43.9 | 16.7 µs | 61.4 | 100000 (5 × 20000) |
| `Thunder-1024` | 4096 B | 171.9 k | 42.0 | 63.8 µs | 64.2 | 48145 (5 × 9629) |
| `Thunder-1024` | 8192 B | 341.1 k | 41.6 | 127 µs | 64.7 | 24745 (5 × 4949) |
| `Thunder-1024` | 16384 B | 678.5 k | 41.4 | 252 µs | 65.1 | 12530 (5 × 2506) |
| `Thunder-1024` | 65536 B | 2.71 M | 41.3 | 1 ms | 65.2 | 3135 (5 × 627) |
| `Thunder-XOF-256` | 32 B | 13.0 k | 406.4 | 4.83 µs | 6.6 | 100000 (5 × 20000) |
| `Thunder-XOF-256` | 128 B | 15.7 k | 122.4 | 5.82 µs | 22.0 | 100000 (5 × 20000) |
| `Thunder-XOF-256` | 512 B | 24.7 k | 48.2 | 9.16 µs | 55.9 | 100000 (5 × 20000) |
| `Thunder-XOF-256` | 1024 B | 34.3 k | 33.4 | 12.7 µs | 80.6 | 100000 (5 × 20000) |
| `Thunder-XOF-256` | 4096 B | 97.9 k | 23.9 | 36.3 µs | 112.8 | 83770 (5 × 16754) |
| `Thunder-XOF-256` | 8192 B | 182.7 k | 22.3 | 67.8 µs | 120.8 | 45415 (5 × 9083) |
| `Thunder-XOF-256` | 16384 B | 352.9 k | 21.5 | 131 µs | 125.1 | 15135 (5 × 3027) |
| `Thunder-XOF-256` | 65536 B | 1.37 M | 20.9 | 509 µs | 128.7 | 6200 (5 × 1240) |
| `Thunder-XOF-384` | 32 B | 12.0 k | 375.4 | 4.46 µs | 7.2 | 100000 (5 × 20000) |
| `Thunder-XOF-384` | 128 B | 14.7 k | 114.6 | 5.44 µs | 23.5 | 100000 (5 × 20000) |
| `Thunder-XOF-384` | 512 B | 26.2 k | 51.1 | 9.71 µs | 52.7 | 100000 (5 × 20000) |
| `Thunder-XOF-384` | 1024 B | 38.5 k | 37.6 | 14.3 µs | 71.6 | 100000 (5 × 20000) |
| `Thunder-XOF-384` | 4096 B | 123.2 k | 30.1 | 45.7 µs | 89.5 | 66300 (5 × 13260) |
| `Thunder-XOF-384` | 8192 B | 255.3 k | 31.2 | 94.8 µs | 86.4 | 35295 (5 × 7059) |
| `Thunder-XOF-384` | 16384 B | 461.8 k | 28.2 | 171 µs | 95.6 | 18260 (5 × 3652) |
| `Thunder-XOF-384` | 65536 B | 1.82 M | 27.7 | 674 µs | 97.3 | 4620 (5 × 924) |
| `Thunder-XOF-512` | 32 B | 11.1 k | 347.7 | 4.13 µs | 7.7 | 100000 (5 × 20000) |
| `Thunder-XOF-512` | 128 B | 16.3 k | 127.1 | 6.04 µs | 21.2 | 100000 (5 × 20000) |
| `Thunder-XOF-512` | 512 B | 32.1 k | 62.7 | 11.9 µs | 43.0 | 100000 (5 × 20000) |
| `Thunder-XOF-512` | 1024 B | 56.5 k | 55.2 | 21 µs | 48.8 | 100000 (5 × 20000) |
| `Thunder-XOF-512` | 4096 B | 179.9 k | 43.9 | 66.7 µs | 61.4 | 46110 (5 × 9222) |
| `Thunder-XOF-512` | 8192 B | 398.4 k | 48.6 | 148 µs | 55.4 | 24210 (5 × 4842) |
| `Thunder-XOF-512` | 16384 B | 686.6 k | 41.9 | 255 µs | 64.3 | 11980 (5 × 2396) |
| `Thunder-XOF-512` | 65536 B | 2.72 M | 41.4 | 1.01 ms | 65.0 | 2890 (5 × 578) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Thunder-512` | hash_32 | 12172 | 1400 KiB | 1464 KiB |
| `Thunder-512` | hash_128 | 12172 | 1400 KiB | 1464 KiB |
| `Thunder-512` | hash_512 | 12172 | 1400 KiB | 1464 KiB |
| `Thunder-512` | hash_1024 | 12172 | 1404 KiB | 1468 KiB |
| `Thunder-512` | hash_4096 | 12172 | 1404 KiB | 1468 KiB |
| `Thunder-512` | hash_8192 | 12172 | 1404 KiB | 1468 KiB |
| `Thunder-512` | hash_16384 | 12172 | 1412 KiB | 1476 KiB |
| `Thunder-512` | hash_65536 | 12172 | 3428 KiB | 3492 KiB |
| `Thunder-768` | hash_32 | 12196 | 1400 KiB | 1464 KiB |
| `Thunder-768` | hash_128 | 12196 | 1400 KiB | 1464 KiB |
| `Thunder-768` | hash_512 | 12196 | 1400 KiB | 1464 KiB |
| `Thunder-768` | hash_1024 | 12196 | 1404 KiB | 1468 KiB |
| `Thunder-768` | hash_4096 | 12196 | 1404 KiB | 1468 KiB |
| `Thunder-768` | hash_8192 | 12196 | 1404 KiB | 1468 KiB |
| `Thunder-768` | hash_16384 | 12196 | 1416 KiB | 1480 KiB |
| `Thunder-768` | hash_65536 | 12196 | 1464 KiB | 1528 KiB |
| `Thunder-1024` | hash_32 | 12860 | 1400 KiB | 1464 KiB |
| `Thunder-1024` | hash_128 | 12860 | 1400 KiB | 1464 KiB |
| `Thunder-1024` | hash_512 | 12860 | 1400 KiB | 1464 KiB |
| `Thunder-1024` | hash_1024 | 12860 | 1404 KiB | 1468 KiB |
| `Thunder-1024` | hash_4096 | 12860 | 1404 KiB | 1468 KiB |
| `Thunder-1024` | hash_8192 | 12860 | 1408 KiB | 1472 KiB |
| `Thunder-1024` | hash_16384 | 12860 | 1416 KiB | 1480 KiB |
| `Thunder-1024` | hash_65536 | 12860 | 1464 KiB | 1528 KiB |
| `Thunder-XOF-256` | hash_32 | 13484 | 1400 KiB | 1464 KiB |
| `Thunder-XOF-256` | hash_128 | 13484 | 1400 KiB | 1464 KiB |
| `Thunder-XOF-256` | hash_512 | 13484 | 1396 KiB | 1460 KiB |
| `Thunder-XOF-256` | hash_1024 | 13484 | 1404 KiB | 1468 KiB |
| `Thunder-XOF-256` | hash_4096 | 13484 | 1400 KiB | 1464 KiB |
| `Thunder-XOF-256` | hash_8192 | 13484 | 1408 KiB | 1472 KiB |
| `Thunder-XOF-256` | hash_16384 | 13484 | 3436 KiB | 3500 KiB |
| `Thunder-XOF-256` | hash_65536 | 13484 | 1464 KiB | 1528 KiB |
| `Thunder-XOF-384` | hash_32 | 13612 | 3436 KiB | 3500 KiB |
| `Thunder-XOF-384` | hash_128 | 13612 | 1400 KiB | 1464 KiB |
| `Thunder-XOF-384` | hash_512 | 13612 | 1400 KiB | 1464 KiB |
| `Thunder-XOF-384` | hash_1024 | 13612 | 1404 KiB | 1468 KiB |
| `Thunder-XOF-384` | hash_4096 | 13612 | 1404 KiB | 1468 KiB |
| `Thunder-XOF-384` | hash_8192 | 13612 | 1408 KiB | 1472 KiB |
| `Thunder-XOF-384` | hash_16384 | 13612 | 1416 KiB | 1480 KiB |
| `Thunder-XOF-384` | hash_65536 | 13612 | 1464 KiB | 1528 KiB |
| `Thunder-XOF-512` | hash_32 | 13468 | 1400 KiB | 1464 KiB |
| `Thunder-XOF-512` | hash_128 | 13468 | 1400 KiB | 1464 KiB |
| `Thunder-XOF-512` | hash_512 | 13468 | 1400 KiB | 1464 KiB |
| `Thunder-XOF-512` | hash_1024 | 13468 | 1400 KiB | 1464 KiB |
| `Thunder-XOF-512` | hash_4096 | 13468 | 1404 KiB | 1468 KiB |
| `Thunder-XOF-512` | hash_8192 | 13468 | 1408 KiB | 1472 KiB |
| `Thunder-XOF-512` | hash_16384 | 13468 | 1416 KiB | 1480 KiB |
| `Thunder-XOF-512` | hash_65536 | 13468 | 1464 KiB | 1528 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Thunder-512` | KAT log (sha256 `351b19a2a1681c03…`) | `kat/hash-33/Thunder-512.log` |
| `Thunder-512` | timing hash_1024 | `records/hash-33/Thunder-512__hash_1024.json` |
| `Thunder-512` | timing hash_128 | `records/hash-33/Thunder-512__hash_128.json` |
| `Thunder-512` | timing hash_16384 | `records/hash-33/Thunder-512__hash_16384.json` |
| `Thunder-512` | timing hash_32 | `records/hash-33/Thunder-512__hash_32.json` |
| `Thunder-512` | timing hash_4096 | `records/hash-33/Thunder-512__hash_4096.json` |
| `Thunder-512` | timing hash_512 | `records/hash-33/Thunder-512__hash_512.json` |
| `Thunder-512` | timing hash_65536 | `records/hash-33/Thunder-512__hash_65536.json` |
| `Thunder-512` | timing hash_8192 | `records/hash-33/Thunder-512__hash_8192.json` |
| `Thunder-768` | KAT log (sha256 `935ad71230ef058b…`) | `kat/hash-33/Thunder-768.log` |
| `Thunder-768` | timing hash_1024 | `records/hash-33/Thunder-768__hash_1024.json` |
| `Thunder-768` | timing hash_128 | `records/hash-33/Thunder-768__hash_128.json` |
| `Thunder-768` | timing hash_16384 | `records/hash-33/Thunder-768__hash_16384.json` |
| `Thunder-768` | timing hash_32 | `records/hash-33/Thunder-768__hash_32.json` |
| `Thunder-768` | timing hash_4096 | `records/hash-33/Thunder-768__hash_4096.json` |
| `Thunder-768` | timing hash_512 | `records/hash-33/Thunder-768__hash_512.json` |
| `Thunder-768` | timing hash_65536 | `records/hash-33/Thunder-768__hash_65536.json` |
| `Thunder-768` | timing hash_8192 | `records/hash-33/Thunder-768__hash_8192.json` |
| `Thunder-1024` | KAT log (sha256 `e081b435ef4e5dd4…`) | `kat/hash-33/Thunder-1024.log` |
| `Thunder-1024` | timing hash_1024 | `records/hash-33/Thunder-1024__hash_1024.json` |
| `Thunder-1024` | timing hash_128 | `records/hash-33/Thunder-1024__hash_128.json` |
| `Thunder-1024` | timing hash_16384 | `records/hash-33/Thunder-1024__hash_16384.json` |
| `Thunder-1024` | timing hash_32 | `records/hash-33/Thunder-1024__hash_32.json` |
| `Thunder-1024` | timing hash_4096 | `records/hash-33/Thunder-1024__hash_4096.json` |
| `Thunder-1024` | timing hash_512 | `records/hash-33/Thunder-1024__hash_512.json` |
| `Thunder-1024` | timing hash_65536 | `records/hash-33/Thunder-1024__hash_65536.json` |
| `Thunder-1024` | timing hash_8192 | `records/hash-33/Thunder-1024__hash_8192.json` |
| `Thunder-XOF-256` | KAT log (sha256 `8cfe9a17e8a28f7b…`) | `kat/hash-33/Thunder-XOF-256.log` |
| `Thunder-XOF-256` | timing hash_1024 | `records/hash-33/Thunder-XOF-256__hash_1024.json` |
| `Thunder-XOF-256` | timing hash_128 | `records/hash-33/Thunder-XOF-256__hash_128.json` |
| `Thunder-XOF-256` | timing hash_16384 | `records/hash-33/Thunder-XOF-256__hash_16384.json` |
| `Thunder-XOF-256` | timing hash_32 | `records/hash-33/Thunder-XOF-256__hash_32.json` |
| `Thunder-XOF-256` | timing hash_4096 | `records/hash-33/Thunder-XOF-256__hash_4096.json` |
| `Thunder-XOF-256` | timing hash_512 | `records/hash-33/Thunder-XOF-256__hash_512.json` |
| `Thunder-XOF-256` | timing hash_65536 | `records/hash-33/Thunder-XOF-256__hash_65536.json` |
| `Thunder-XOF-256` | timing hash_8192 | `records/hash-33/Thunder-XOF-256__hash_8192.json` |
| `Thunder-XOF-384` | KAT log (sha256 `da6d68ab5cbbc6e1…`) | `kat/hash-33/Thunder-XOF-384.log` |
| `Thunder-XOF-384` | timing hash_1024 | `records/hash-33/Thunder-XOF-384__hash_1024.json` |
| `Thunder-XOF-384` | timing hash_128 | `records/hash-33/Thunder-XOF-384__hash_128.json` |
| `Thunder-XOF-384` | timing hash_16384 | `records/hash-33/Thunder-XOF-384__hash_16384.json` |
| `Thunder-XOF-384` | timing hash_32 | `records/hash-33/Thunder-XOF-384__hash_32.json` |
| `Thunder-XOF-384` | timing hash_4096 | `records/hash-33/Thunder-XOF-384__hash_4096.json` |
| `Thunder-XOF-384` | timing hash_512 | `records/hash-33/Thunder-XOF-384__hash_512.json` |
| `Thunder-XOF-384` | timing hash_65536 | `records/hash-33/Thunder-XOF-384__hash_65536.json` |
| `Thunder-XOF-384` | timing hash_8192 | `records/hash-33/Thunder-XOF-384__hash_8192.json` |
| `Thunder-XOF-512` | KAT log (sha256 `27b7d525cb082e8b…`) | `kat/hash-33/Thunder-XOF-512.log` |
| `Thunder-XOF-512` | timing hash_1024 | `records/hash-33/Thunder-XOF-512__hash_1024.json` |
| `Thunder-XOF-512` | timing hash_128 | `records/hash-33/Thunder-XOF-512__hash_128.json` |
| `Thunder-XOF-512` | timing hash_16384 | `records/hash-33/Thunder-XOF-512__hash_16384.json` |
| `Thunder-XOF-512` | timing hash_32 | `records/hash-33/Thunder-XOF-512__hash_32.json` |
| `Thunder-XOF-512` | timing hash_4096 | `records/hash-33/Thunder-XOF-512__hash_4096.json` |
| `Thunder-XOF-512` | timing hash_512 | `records/hash-33/Thunder-XOF-512__hash_512.json` |
| `Thunder-XOF-512` | timing hash_65536 | `records/hash-33/Thunder-XOF-512__hash_65536.json` |
| `Thunder-XOF-512` | timing hash_8192 | `records/hash-33/Thunder-XOF-512__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

