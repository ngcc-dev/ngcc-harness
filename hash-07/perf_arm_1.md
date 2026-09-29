# hash-07 Dragon Hash Family — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-07` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101539781751689216.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: Dragon Hash Family
- Implementation versions measured: reference
- Parameter sets: `Dragon-512`, `Dragon-768`, `Dragon-1024`, `Dragon-XOF-256`, `Dragon-XOF-384`, `Dragon-XOF-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-07/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Dragon-512` | guide | PASS |
| `Dragon-768` | guide | PASS |
| `Dragon-1024` | guide | PASS |
| `Dragon-XOF-256` | guide | PASS |
| `Dragon-XOF-384` | guide | PASS |
| `Dragon-XOF-512` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `Dragon-512` | 32 B | 3862 | 120.7 | 1.44 µs | 22.3 | 100000 (5 × 20000) |
| `Dragon-512` | 128 B | 8117 | 63.4 | 3.01 µs | 42.5 | 100000 (5 × 20000) |
| `Dragon-512` | 512 B | 19.2 k | 37.5 | 7.12 µs | 71.9 | 100000 (5 × 20000) |
| `Dragon-512` | 1024 B | 34.3 k | 33.5 | 12.7 µs | 80.5 | 100000 (5 × 20000) |
| `Dragon-512` | 4096 B | 126.9 k | 31.0 | 47.1 µs | 87.0 | 61070 (5 × 12214) |
| `Dragon-512` | 8192 B | 249.0 k | 30.4 | 92.4 µs | 88.7 | 34265 (5 × 6853) |
| `Dragon-512` | 16384 B | 492.3 k | 30.0 | 183 µs | 89.7 | 17880 (5 × 3576) |
| `Dragon-512` | 65536 B | 1.97 M | 30.0 | 730 µs | 89.8 | 4215 (5 × 843) |
| `Dragon-768` | 32 B | 3896 | 121.7 | 1.45 µs | 22.1 | 100000 (5 × 20000) |
| `Dragon-768` | 128 B | 7686 | 60.0 | 2.85 µs | 44.8 | 100000 (5 × 20000) |
| `Dragon-768` | 512 B | 23.0 k | 44.9 | 8.54 µs | 60.0 | 100000 (5 × 20000) |
| `Dragon-768` | 1024 B | 42.1 k | 41.1 | 15.6 µs | 65.5 | 100000 (5 × 20000) |
| `Dragon-768` | 4096 B | 164.5 k | 40.2 | 61.1 µs | 67.1 | 48390 (5 × 9678) |
| `Dragon-768` | 8192 B | 329.2 k | 40.2 | 122 µs | 67.1 | 25605 (5 × 5121) |
| `Dragon-768` | 16384 B | 657.1 k | 40.1 | 244 µs | 67.2 | 12400 (5 × 2480) |
| `Dragon-768` | 65536 B | 2.60 M | 39.7 | 966 µs | 67.9 | 3110 (5 × 622) |
| `Dragon-1024` | 32 B | 3927 | 122.7 | 1.46 µs | 21.9 | 100000 (5 × 20000) |
| `Dragon-1024` | 128 B | 11.6 k | 90.8 | 4.32 µs | 29.7 | 100000 (5 × 20000) |
| `Dragon-1024` | 512 B | 34.7 k | 67.7 | 12.9 µs | 39.8 | 100000 (5 × 20000) |
| `Dragon-1024` | 1024 B | 65.2 k | 63.6 | 24.2 µs | 42.4 | 100000 (5 × 20000) |
| `Dragon-1024` | 4096 B | 250.3 k | 61.1 | 92.8 µs | 44.1 | 32175 (5 × 6435) |
| `Dragon-1024` | 8192 B | 495.5 k | 60.5 | 184 µs | 44.6 | 16565 (5 × 3313) |
| `Dragon-1024` | 16384 B | 984.7 k | 60.1 | 365 µs | 44.8 | 8475 (5 × 1695) |
| `Dragon-1024` | 65536 B | 3.94 M | 60.2 | 1.46 ms | 44.8 | 2055 (5 × 411) |
| `Dragon-XOF-256` | 32 B | 14.1 k | 441.7 | 5.25 µs | 6.1 | 100000 (5 × 20000) |
| `Dragon-XOF-256` | 128 B | 18.0 k | 140.4 | 6.67 µs | 19.2 | 100000 (5 × 20000) |
| `Dragon-XOF-256` | 512 B | 29.4 k | 57.5 | 10.9 µs | 46.9 | 100000 (5 × 20000) |
| `Dragon-XOF-256` | 1024 B | 44.6 k | 43.6 | 16.6 µs | 61.8 | 100000 (5 × 20000) |
| `Dragon-XOF-256` | 4096 B | 136.7 k | 33.4 | 50.7 µs | 80.8 | 62180 (5 × 12436) |
| `Dragon-XOF-256` | 8192 B | 259.2 k | 31.6 | 96.2 µs | 85.2 | 32110 (5 × 6422) |
| `Dragon-XOF-256` | 16384 B | 502.5 k | 30.7 | 186 µs | 87.9 | 16990 (5 × 3398) |
| `Dragon-XOF-256` | 65536 B | 1.96 M | 29.9 | 728 µs | 90.0 | 4050 (5 × 810) |
| `Dragon-XOF-384` | 32 B | 13.1 k | 409.2 | 4.86 µs | 6.6 | 100000 (5 × 20000) |
| `Dragon-XOF-384` | 128 B | 16.9 k | 132.3 | 6.28 µs | 20.4 | 100000 (5 × 20000) |
| `Dragon-XOF-384` | 512 B | 32.3 k | 63.0 | 12 µs | 42.7 | 100000 (5 × 20000) |
| `Dragon-XOF-384` | 1024 B | 51.6 k | 50.4 | 19.1 µs | 53.5 | 100000 (5 × 20000) |
| `Dragon-XOF-384` | 4096 B | 172.7 k | 42.2 | 64.1 µs | 63.9 | 46830 (5 × 9366) |
| `Dragon-XOF-384` | 8192 B | 338.3 k | 41.3 | 126 µs | 65.3 | 25535 (5 × 5107) |
| `Dragon-XOF-384` | 16384 B | 661.7 k | 40.4 | 246 µs | 66.7 | 12265 (5 × 2453) |
| `Dragon-XOF-384` | 65536 B | 2.61 M | 39.9 | 969 µs | 67.6 | 3195 (5 × 639) |
| `Dragon-XOF-512` | 32 B | 12.1 k | 378.8 | 4.5 µs | 7.1 | 100000 (5 × 20000) |
| `Dragon-XOF-512` | 128 B | 19.8 k | 155.0 | 7.36 µs | 17.4 | 100000 (5 × 20000) |
| `Dragon-XOF-512` | 512 B | 42.6 k | 83.3 | 15.8 µs | 32.4 | 100000 (5 × 20000) |
| `Dragon-XOF-512` | 1024 B | 73.3 k | 71.6 | 27.2 µs | 37.7 | 100000 (5 × 20000) |
| `Dragon-XOF-512` | 4096 B | 257.3 k | 62.8 | 95.5 µs | 42.9 | 31875 (5 × 6375) |
| `Dragon-XOF-512` | 8192 B | 500.3 k | 61.1 | 186 µs | 44.1 | 17430 (5 × 3486) |
| `Dragon-XOF-512` | 16384 B | 997.5 k | 60.9 | 370 µs | 44.3 | 8165 (5 × 1633) |
| `Dragon-XOF-512` | 65536 B | 3.96 M | 60.4 | 1.47 ms | 44.6 | 1940 (5 × 388) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Dragon-512` | hash_32 | 11676 | 3440 KiB | 3504 KiB |
| `Dragon-512` | hash_128 | 11676 | 1400 KiB | 1464 KiB |
| `Dragon-512` | hash_512 | 11676 | 1400 KiB | 1464 KiB |
| `Dragon-512` | hash_1024 | 11676 | 1404 KiB | 1468 KiB |
| `Dragon-512` | hash_4096 | 11676 | 1404 KiB | 1468 KiB |
| `Dragon-512` | hash_8192 | 11676 | 1408 KiB | 1472 KiB |
| `Dragon-512` | hash_16384 | 11676 | 1416 KiB | 1480 KiB |
| `Dragon-512` | hash_65536 | 11676 | 3432 KiB | 3496 KiB |
| `Dragon-768` | hash_32 | 11692 | 1400 KiB | 1464 KiB |
| `Dragon-768` | hash_128 | 11692 | 1400 KiB | 1464 KiB |
| `Dragon-768` | hash_512 | 11692 | 1400 KiB | 1464 KiB |
| `Dragon-768` | hash_1024 | 11692 | 1404 KiB | 1468 KiB |
| `Dragon-768` | hash_4096 | 11692 | 1404 KiB | 1468 KiB |
| `Dragon-768` | hash_8192 | 11692 | 1408 KiB | 1472 KiB |
| `Dragon-768` | hash_16384 | 11692 | 1416 KiB | 1480 KiB |
| `Dragon-768` | hash_65536 | 11692 | 1464 KiB | 1528 KiB |
| `Dragon-1024` | hash_32 | 11620 | 1400 KiB | 1464 KiB |
| `Dragon-1024` | hash_128 | 11620 | 1396 KiB | 1460 KiB |
| `Dragon-1024` | hash_512 | 11620 | 3432 KiB | 3496 KiB |
| `Dragon-1024` | hash_1024 | 11620 | 1404 KiB | 1468 KiB |
| `Dragon-1024` | hash_4096 | 11620 | 1400 KiB | 1464 KiB |
| `Dragon-1024` | hash_8192 | 11620 | 1408 KiB | 1472 KiB |
| `Dragon-1024` | hash_16384 | 11620 | 1416 KiB | 1480 KiB |
| `Dragon-1024` | hash_65536 | 11620 | 1464 KiB | 1528 KiB |
| `Dragon-XOF-256` | hash_32 | 12252 | 1400 KiB | 1464 KiB |
| `Dragon-XOF-256` | hash_128 | 12252 | 1400 KiB | 1464 KiB |
| `Dragon-XOF-256` | hash_512 | 12252 | 1400 KiB | 1464 KiB |
| `Dragon-XOF-256` | hash_1024 | 12252 | 1404 KiB | 1468 KiB |
| `Dragon-XOF-256` | hash_4096 | 12252 | 1400 KiB | 1464 KiB |
| `Dragon-XOF-256` | hash_8192 | 12252 | 1408 KiB | 1472 KiB |
| `Dragon-XOF-256` | hash_16384 | 12252 | 1416 KiB | 1480 KiB |
| `Dragon-XOF-256` | hash_65536 | 12252 | 1464 KiB | 1528 KiB |
| `Dragon-XOF-384` | hash_32 | 12300 | 1400 KiB | 1464 KiB |
| `Dragon-XOF-384` | hash_128 | 12300 | 1396 KiB | 1460 KiB |
| `Dragon-XOF-384` | hash_512 | 12300 | 1400 KiB | 1464 KiB |
| `Dragon-XOF-384` | hash_1024 | 12300 | 1404 KiB | 1468 KiB |
| `Dragon-XOF-384` | hash_4096 | 12300 | 1404 KiB | 1468 KiB |
| `Dragon-XOF-384` | hash_8192 | 12300 | 1408 KiB | 1472 KiB |
| `Dragon-XOF-384` | hash_16384 | 12300 | 1416 KiB | 1480 KiB |
| `Dragon-XOF-384` | hash_65536 | 12300 | 1464 KiB | 1528 KiB |
| `Dragon-XOF-512` | hash_32 | 12116 | 1400 KiB | 1464 KiB |
| `Dragon-XOF-512` | hash_128 | 12116 | 1400 KiB | 1464 KiB |
| `Dragon-XOF-512` | hash_512 | 12116 | 1400 KiB | 1464 KiB |
| `Dragon-XOF-512` | hash_1024 | 12116 | 1404 KiB | 1468 KiB |
| `Dragon-XOF-512` | hash_4096 | 12116 | 1404 KiB | 1468 KiB |
| `Dragon-XOF-512` | hash_8192 | 12116 | 3444 KiB | 3508 KiB |
| `Dragon-XOF-512` | hash_16384 | 12116 | 1416 KiB | 1480 KiB |
| `Dragon-XOF-512` | hash_65536 | 12116 | 1464 KiB | 1528 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Dragon-512` | KAT log (sha256 `28055ad2fd219a2a…`) | `kat/hash-07/Dragon-512.log` |
| `Dragon-512` | timing hash_1024 | `records/hash-07/Dragon-512__hash_1024.json` |
| `Dragon-512` | timing hash_128 | `records/hash-07/Dragon-512__hash_128.json` |
| `Dragon-512` | timing hash_16384 | `records/hash-07/Dragon-512__hash_16384.json` |
| `Dragon-512` | timing hash_32 | `records/hash-07/Dragon-512__hash_32.json` |
| `Dragon-512` | timing hash_4096 | `records/hash-07/Dragon-512__hash_4096.json` |
| `Dragon-512` | timing hash_512 | `records/hash-07/Dragon-512__hash_512.json` |
| `Dragon-512` | timing hash_65536 | `records/hash-07/Dragon-512__hash_65536.json` |
| `Dragon-512` | timing hash_8192 | `records/hash-07/Dragon-512__hash_8192.json` |
| `Dragon-768` | KAT log (sha256 `d55d9cdd6b8272d5…`) | `kat/hash-07/Dragon-768.log` |
| `Dragon-768` | timing hash_1024 | `records/hash-07/Dragon-768__hash_1024.json` |
| `Dragon-768` | timing hash_128 | `records/hash-07/Dragon-768__hash_128.json` |
| `Dragon-768` | timing hash_16384 | `records/hash-07/Dragon-768__hash_16384.json` |
| `Dragon-768` | timing hash_32 | `records/hash-07/Dragon-768__hash_32.json` |
| `Dragon-768` | timing hash_4096 | `records/hash-07/Dragon-768__hash_4096.json` |
| `Dragon-768` | timing hash_512 | `records/hash-07/Dragon-768__hash_512.json` |
| `Dragon-768` | timing hash_65536 | `records/hash-07/Dragon-768__hash_65536.json` |
| `Dragon-768` | timing hash_8192 | `records/hash-07/Dragon-768__hash_8192.json` |
| `Dragon-1024` | KAT log (sha256 `4cdf4487d79d9233…`) | `kat/hash-07/Dragon-1024.log` |
| `Dragon-1024` | timing hash_1024 | `records/hash-07/Dragon-1024__hash_1024.json` |
| `Dragon-1024` | timing hash_128 | `records/hash-07/Dragon-1024__hash_128.json` |
| `Dragon-1024` | timing hash_16384 | `records/hash-07/Dragon-1024__hash_16384.json` |
| `Dragon-1024` | timing hash_32 | `records/hash-07/Dragon-1024__hash_32.json` |
| `Dragon-1024` | timing hash_4096 | `records/hash-07/Dragon-1024__hash_4096.json` |
| `Dragon-1024` | timing hash_512 | `records/hash-07/Dragon-1024__hash_512.json` |
| `Dragon-1024` | timing hash_65536 | `records/hash-07/Dragon-1024__hash_65536.json` |
| `Dragon-1024` | timing hash_8192 | `records/hash-07/Dragon-1024__hash_8192.json` |
| `Dragon-XOF-256` | KAT log (sha256 `53b8bf6a38d67f7b…`) | `kat/hash-07/Dragon-XOF-256.log` |
| `Dragon-XOF-256` | timing hash_1024 | `records/hash-07/Dragon-XOF-256__hash_1024.json` |
| `Dragon-XOF-256` | timing hash_128 | `records/hash-07/Dragon-XOF-256__hash_128.json` |
| `Dragon-XOF-256` | timing hash_16384 | `records/hash-07/Dragon-XOF-256__hash_16384.json` |
| `Dragon-XOF-256` | timing hash_32 | `records/hash-07/Dragon-XOF-256__hash_32.json` |
| `Dragon-XOF-256` | timing hash_4096 | `records/hash-07/Dragon-XOF-256__hash_4096.json` |
| `Dragon-XOF-256` | timing hash_512 | `records/hash-07/Dragon-XOF-256__hash_512.json` |
| `Dragon-XOF-256` | timing hash_65536 | `records/hash-07/Dragon-XOF-256__hash_65536.json` |
| `Dragon-XOF-256` | timing hash_8192 | `records/hash-07/Dragon-XOF-256__hash_8192.json` |
| `Dragon-XOF-384` | KAT log (sha256 `db401e0f4f78d8f1…`) | `kat/hash-07/Dragon-XOF-384.log` |
| `Dragon-XOF-384` | timing hash_1024 | `records/hash-07/Dragon-XOF-384__hash_1024.json` |
| `Dragon-XOF-384` | timing hash_128 | `records/hash-07/Dragon-XOF-384__hash_128.json` |
| `Dragon-XOF-384` | timing hash_16384 | `records/hash-07/Dragon-XOF-384__hash_16384.json` |
| `Dragon-XOF-384` | timing hash_32 | `records/hash-07/Dragon-XOF-384__hash_32.json` |
| `Dragon-XOF-384` | timing hash_4096 | `records/hash-07/Dragon-XOF-384__hash_4096.json` |
| `Dragon-XOF-384` | timing hash_512 | `records/hash-07/Dragon-XOF-384__hash_512.json` |
| `Dragon-XOF-384` | timing hash_65536 | `records/hash-07/Dragon-XOF-384__hash_65536.json` |
| `Dragon-XOF-384` | timing hash_8192 | `records/hash-07/Dragon-XOF-384__hash_8192.json` |
| `Dragon-XOF-512` | KAT log (sha256 `745084dcce2602e2…`) | `kat/hash-07/Dragon-XOF-512.log` |
| `Dragon-XOF-512` | timing hash_1024 | `records/hash-07/Dragon-XOF-512__hash_1024.json` |
| `Dragon-XOF-512` | timing hash_128 | `records/hash-07/Dragon-XOF-512__hash_128.json` |
| `Dragon-XOF-512` | timing hash_16384 | `records/hash-07/Dragon-XOF-512__hash_16384.json` |
| `Dragon-XOF-512` | timing hash_32 | `records/hash-07/Dragon-XOF-512__hash_32.json` |
| `Dragon-XOF-512` | timing hash_4096 | `records/hash-07/Dragon-XOF-512__hash_4096.json` |
| `Dragon-XOF-512` | timing hash_512 | `records/hash-07/Dragon-XOF-512__hash_512.json` |
| `Dragon-XOF-512` | timing hash_65536 | `records/hash-07/Dragon-XOF-512__hash_65536.json` |
| `Dragon-XOF-512` | timing hash_8192 | `records/hash-07/Dragon-XOF-512__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

