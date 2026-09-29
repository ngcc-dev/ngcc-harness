# hash-10 FEILIAN — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-10` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101539111250251776.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: FEILIAN
- Implementation versions measured: reference
- Parameter sets: `FEILIAN512`, `FEILIAN768`, `FEILIAN1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-10/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `FEILIAN512` | guide | PASS |
| `FEILIAN768` | guide | PASS |
| `FEILIAN1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `FEILIAN512` | 32 B | 2323 | 72.6 | 865 ns | 37.0 | 100000 (5 × 20000) |
| `FEILIAN512` | 128 B | 2328 | 18.2 | 866 ns | 147.9 | 100000 (5 × 20000) |
| `FEILIAN512` | 512 B | 8972 | 17.5 | 3.33 µs | 153.7 | 100000 (5 × 20000) |
| `FEILIAN512` | 1024 B | 17.9 k | 17.4 | 6.63 µs | 154.4 | 100000 (5 × 20000) |
| `FEILIAN512` | 4096 B | 70.9 k | 17.3 | 26.3 µs | 155.6 | 87915 (5 × 17583) |
| `FEILIAN512` | 8192 B | 141.9 k | 17.3 | 52.7 µs | 155.6 | 49285 (5 × 9857) |
| `FEILIAN512` | 16384 B | 284.5 k | 17.4 | 106 µs | 155.2 | 26895 (5 × 5379) |
| `FEILIAN512` | 65536 B | 1.13 M | 17.3 | 421 µs | 155.7 | 7155 (5 × 1431) |
| `FEILIAN768` | 32 B | 2362 | 73.8 | 878 ns | 36.4 | 100000 (5 × 20000) |
| `FEILIAN768` | 128 B | 2351 | 18.4 | 875 ns | 146.2 | 100000 (5 × 20000) |
| `FEILIAN768` | 512 B | 8970 | 17.5 | 3.33 µs | 153.6 | 100000 (5 × 20000) |
| `FEILIAN768` | 1024 B | 17.9 k | 17.4 | 6.63 µs | 154.4 | 100000 (5 × 20000) |
| `FEILIAN768` | 4096 B | 71.1 k | 17.3 | 26.4 µs | 155.2 | 100000 (5 × 20000) |
| `FEILIAN768` | 8192 B | 141.8 k | 17.3 | 52.6 µs | 155.7 | 56340 (5 × 11268) |
| `FEILIAN768` | 16384 B | 285.0 k | 17.4 | 106 µs | 154.9 | 28490 (5 × 5698) |
| `FEILIAN768` | 65536 B | 1.14 M | 17.4 | 423 µs | 155.0 | 7200 (5 × 1440) |
| `FEILIAN1024` | 32 B | 2384 | 74.5 | 887 ns | 36.1 | 100000 (5 × 20000) |
| `FEILIAN1024` | 128 B | 2375 | 18.6 | 883 ns | 144.9 | 100000 (5 × 20000) |
| `FEILIAN1024` | 512 B | 9043 | 17.7 | 3.36 µs | 152.5 | 100000 (5 × 20000) |
| `FEILIAN1024` | 1024 B | 18.0 k | 17.6 | 6.67 µs | 153.5 | 100000 (5 × 20000) |
| `FEILIAN1024` | 4096 B | 71.0 k | 17.3 | 26.3 µs | 155.5 | 100000 (5 × 20000) |
| `FEILIAN1024` | 8192 B | 141.8 k | 17.3 | 52.6 µs | 155.7 | 56540 (5 × 11308) |
| `FEILIAN1024` | 16384 B | 283.3 k | 17.3 | 105 µs | 155.9 | 28640 (5 × 5728) |
| `FEILIAN1024` | 65536 B | 1.13 M | 17.3 | 421 µs | 155.7 | 7185 (5 × 1437) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `FEILIAN512` | hash_32 | 11644 | 1400 KiB | 1464 KiB |
| `FEILIAN512` | hash_128 | 11644 | 1400 KiB | 1464 KiB |
| `FEILIAN512` | hash_512 | 11644 | 1396 KiB | 1464 KiB |
| `FEILIAN512` | hash_1024 | 11644 | 1400 KiB | 1464 KiB |
| `FEILIAN512` | hash_4096 | 11644 | 1404 KiB | 1472 KiB |
| `FEILIAN512` | hash_8192 | 11644 | 1408 KiB | 1480 KiB |
| `FEILIAN512` | hash_16384 | 11644 | 1412 KiB | 1492 KiB |
| `FEILIAN512` | hash_65536 | 11644 | 1464 KiB | 1592 KiB |
| `FEILIAN768` | hash_32 | 11580 | 1400 KiB | 1464 KiB |
| `FEILIAN768` | hash_128 | 11580 | 1400 KiB | 1464 KiB |
| `FEILIAN768` | hash_512 | 11580 | 1396 KiB | 1464 KiB |
| `FEILIAN768` | hash_1024 | 11580 | 1404 KiB | 1468 KiB |
| `FEILIAN768` | hash_4096 | 11580 | 1404 KiB | 1472 KiB |
| `FEILIAN768` | hash_8192 | 11580 | 1404 KiB | 1476 KiB |
| `FEILIAN768` | hash_16384 | 11580 | 1416 KiB | 1496 KiB |
| `FEILIAN768` | hash_65536 | 11580 | 1464 KiB | 1592 KiB |
| `FEILIAN1024` | hash_32 | 11580 | 1400 KiB | 1464 KiB |
| `FEILIAN1024` | hash_128 | 11580 | 1400 KiB | 1464 KiB |
| `FEILIAN1024` | hash_512 | 11580 | 1400 KiB | 1468 KiB |
| `FEILIAN1024` | hash_1024 | 11580 | 1404 KiB | 1468 KiB |
| `FEILIAN1024` | hash_4096 | 11580 | 1404 KiB | 1472 KiB |
| `FEILIAN1024` | hash_8192 | 11580 | 1408 KiB | 1480 KiB |
| `FEILIAN1024` | hash_16384 | 11580 | 1416 KiB | 1496 KiB |
| `FEILIAN1024` | hash_65536 | 11580 | 1464 KiB | 1592 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `FEILIAN512` | KAT log (sha256 `8dd4b4ba1e6d4a36…`) | `kat/hash-10/FEILIAN512.log` |
| `FEILIAN512` | timing hash_1024 | `records/hash-10/FEILIAN512__hash_1024.json` |
| `FEILIAN512` | timing hash_128 | `records/hash-10/FEILIAN512__hash_128.json` |
| `FEILIAN512` | timing hash_16384 | `records/hash-10/FEILIAN512__hash_16384.json` |
| `FEILIAN512` | timing hash_32 | `records/hash-10/FEILIAN512__hash_32.json` |
| `FEILIAN512` | timing hash_4096 | `records/hash-10/FEILIAN512__hash_4096.json` |
| `FEILIAN512` | timing hash_512 | `records/hash-10/FEILIAN512__hash_512.json` |
| `FEILIAN512` | timing hash_65536 | `records/hash-10/FEILIAN512__hash_65536.json` |
| `FEILIAN512` | timing hash_8192 | `records/hash-10/FEILIAN512__hash_8192.json` |
| `FEILIAN768` | KAT log (sha256 `6ad347aa936ebe15…`) | `kat/hash-10/FEILIAN768.log` |
| `FEILIAN768` | timing hash_1024 | `records/hash-10/FEILIAN768__hash_1024.json` |
| `FEILIAN768` | timing hash_128 | `records/hash-10/FEILIAN768__hash_128.json` |
| `FEILIAN768` | timing hash_16384 | `records/hash-10/FEILIAN768__hash_16384.json` |
| `FEILIAN768` | timing hash_32 | `records/hash-10/FEILIAN768__hash_32.json` |
| `FEILIAN768` | timing hash_4096 | `records/hash-10/FEILIAN768__hash_4096.json` |
| `FEILIAN768` | timing hash_512 | `records/hash-10/FEILIAN768__hash_512.json` |
| `FEILIAN768` | timing hash_65536 | `records/hash-10/FEILIAN768__hash_65536.json` |
| `FEILIAN768` | timing hash_8192 | `records/hash-10/FEILIAN768__hash_8192.json` |
| `FEILIAN1024` | KAT log (sha256 `2b7aecd13402053b…`) | `kat/hash-10/FEILIAN1024.log` |
| `FEILIAN1024` | timing hash_1024 | `records/hash-10/FEILIAN1024__hash_1024.json` |
| `FEILIAN1024` | timing hash_128 | `records/hash-10/FEILIAN1024__hash_128.json` |
| `FEILIAN1024` | timing hash_16384 | `records/hash-10/FEILIAN1024__hash_16384.json` |
| `FEILIAN1024` | timing hash_32 | `records/hash-10/FEILIAN1024__hash_32.json` |
| `FEILIAN1024` | timing hash_4096 | `records/hash-10/FEILIAN1024__hash_4096.json` |
| `FEILIAN1024` | timing hash_512 | `records/hash-10/FEILIAN1024__hash_512.json` |
| `FEILIAN1024` | timing hash_65536 | `records/hash-10/FEILIAN1024__hash_65536.json` |
| `FEILIAN1024` | timing hash_8192 | `records/hash-10/FEILIAN1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

