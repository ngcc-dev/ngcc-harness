# hash-23 QILIN — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-23` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101536372126470144.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: QILIN
- Implementation versions measured: reference
- Parameter sets: `QILIN-512`, `QILIN-768`, `QILIN-1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-23/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `QILIN-512` | guide | PASS |
| `QILIN-768` | guide | PASS |
| `QILIN-1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `QILIN-512` | 32 B | 13.0 k | 406.2 | 4.82 µs | 6.6 | 100000 (5 × 20000) |
| `QILIN-512` | 128 B | 13.0 k | 101.6 | 4.83 µs | 26.5 | 100000 (5 × 20000) |
| `QILIN-512` | 512 B | 26.0 k | 50.8 | 9.65 µs | 53.1 | 100000 (5 × 20000) |
| `QILIN-512` | 1024 B | 51.9 k | 50.7 | 19.3 µs | 53.2 | 100000 (5 × 20000) |
| `QILIN-512` | 4096 B | 207.4 k | 50.6 | 76.9 µs | 53.2 | 40205 (5 × 8041) |
| `QILIN-512` | 8192 B | 415.1 k | 50.7 | 154 µs | 53.2 | 20255 (5 × 4051) |
| `QILIN-512` | 16384 B | 817.4 k | 49.9 | 303 µs | 54.0 | 10400 (5 × 2080) |
| `QILIN-512` | 65536 B | 3.23 M | 49.2 | 1.2 ms | 54.7 | 2470 (5 × 494) |
| `QILIN-768` | 32 B | 15.1 k | 473.3 | 5.62 µs | 5.7 | 100000 (5 × 20000) |
| `QILIN-768` | 128 B | 15.1 k | 118.3 | 5.62 µs | 22.8 | 100000 (5 × 20000) |
| `QILIN-768` | 512 B | 45.3 k | 88.4 | 16.8 µs | 30.5 | 100000 (5 × 20000) |
| `QILIN-768` | 1024 B | 90.7 k | 88.6 | 33.7 µs | 30.4 | 89385 (5 × 17877) |
| `QILIN-768` | 4096 B | 316.4 k | 77.3 | 117 µs | 34.9 | 26535 (5 × 5307) |
| `QILIN-768` | 8192 B | 616.9 k | 75.3 | 229 µs | 35.8 | 13805 (5 × 2761) |
| `QILIN-768` | 16384 B | 1.23 M | 75.3 | 457 µs | 35.8 | 6915 (5 × 1383) |
| `QILIN-768` | 65536 B | 4.95 M | 75.5 | 1.84 ms | 35.7 | 1640 (5 × 328) |
| `QILIN-1024` | 32 B | 17.2 k | 537.9 | 6.39 µs | 5.0 | 100000 (5 × 20000) |
| `QILIN-1024` | 128 B | 17.2 k | 134.7 | 6.4 µs | 20.0 | 100000 (5 × 20000) |
| `QILIN-1024` | 512 B | 68.9 k | 134.6 | 25.6 µs | 20.0 | 100000 (5 × 20000) |
| `QILIN-1024` | 1024 B | 137.3 k | 134.1 | 50.9 µs | 20.1 | 58900 (5 × 11780) |
| `QILIN-1024` | 4096 B | 531.5 k | 129.8 | 197 µs | 20.8 | 15920 (5 × 3184) |
| `QILIN-1024` | 8192 B | 1.05 M | 127.7 | 388 µs | 21.1 | 8130 (5 × 1626) |
| `QILIN-1024` | 16384 B | 2.07 M | 126.6 | 770 µs | 21.3 | 4045 (5 × 809) |
| `QILIN-1024` | 65536 B | 8.28 M | 126.4 | 3.07 ms | 21.3 | 1005 (5 × 201) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `QILIN-512` | hash_32 | 11860 | 1400 KiB | 1464 KiB |
| `QILIN-512` | hash_128 | 11860 | 1400 KiB | 1464 KiB |
| `QILIN-512` | hash_512 | 11860 | 1400 KiB | 1464 KiB |
| `QILIN-512` | hash_1024 | 11860 | 1400 KiB | 1464 KiB |
| `QILIN-512` | hash_4096 | 11860 | 1404 KiB | 1468 KiB |
| `QILIN-512` | hash_8192 | 11860 | 1408 KiB | 1472 KiB |
| `QILIN-512` | hash_16384 | 11860 | 3448 KiB | 3512 KiB |
| `QILIN-512` | hash_65536 | 11860 | 1464 KiB | 1528 KiB |
| `QILIN-768` | hash_32 | 11860 | 1400 KiB | 1464 KiB |
| `QILIN-768` | hash_128 | 11860 | 1400 KiB | 1464 KiB |
| `QILIN-768` | hash_512 | 11860 | 1400 KiB | 1464 KiB |
| `QILIN-768` | hash_1024 | 11860 | 1404 KiB | 1468 KiB |
| `QILIN-768` | hash_4096 | 11860 | 1400 KiB | 1464 KiB |
| `QILIN-768` | hash_8192 | 11860 | 1408 KiB | 1472 KiB |
| `QILIN-768` | hash_16384 | 11860 | 3428 KiB | 3492 KiB |
| `QILIN-768` | hash_65536 | 11860 | 1464 KiB | 1528 KiB |
| `QILIN-1024` | hash_32 | 11796 | 1400 KiB | 1464 KiB |
| `QILIN-1024` | hash_128 | 11796 | 1400 KiB | 1464 KiB |
| `QILIN-1024` | hash_512 | 11796 | 1400 KiB | 1464 KiB |
| `QILIN-1024` | hash_1024 | 11796 | 1404 KiB | 1468 KiB |
| `QILIN-1024` | hash_4096 | 11796 | 1400 KiB | 1464 KiB |
| `QILIN-1024` | hash_8192 | 11796 | 1408 KiB | 1472 KiB |
| `QILIN-1024` | hash_16384 | 11796 | 1416 KiB | 1480 KiB |
| `QILIN-1024` | hash_65536 | 11796 | 1464 KiB | 1528 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `QILIN-512` | KAT log (sha256 `b9364416b131c647…`) | `kat/hash-23/QILIN-512.log` |
| `QILIN-512` | timing hash_1024 | `records/hash-23/QILIN-512__hash_1024.json` |
| `QILIN-512` | timing hash_128 | `records/hash-23/QILIN-512__hash_128.json` |
| `QILIN-512` | timing hash_16384 | `records/hash-23/QILIN-512__hash_16384.json` |
| `QILIN-512` | timing hash_32 | `records/hash-23/QILIN-512__hash_32.json` |
| `QILIN-512` | timing hash_4096 | `records/hash-23/QILIN-512__hash_4096.json` |
| `QILIN-512` | timing hash_512 | `records/hash-23/QILIN-512__hash_512.json` |
| `QILIN-512` | timing hash_65536 | `records/hash-23/QILIN-512__hash_65536.json` |
| `QILIN-512` | timing hash_8192 | `records/hash-23/QILIN-512__hash_8192.json` |
| `QILIN-768` | KAT log (sha256 `76bceb630c950d62…`) | `kat/hash-23/QILIN-768.log` |
| `QILIN-768` | timing hash_1024 | `records/hash-23/QILIN-768__hash_1024.json` |
| `QILIN-768` | timing hash_128 | `records/hash-23/QILIN-768__hash_128.json` |
| `QILIN-768` | timing hash_16384 | `records/hash-23/QILIN-768__hash_16384.json` |
| `QILIN-768` | timing hash_32 | `records/hash-23/QILIN-768__hash_32.json` |
| `QILIN-768` | timing hash_4096 | `records/hash-23/QILIN-768__hash_4096.json` |
| `QILIN-768` | timing hash_512 | `records/hash-23/QILIN-768__hash_512.json` |
| `QILIN-768` | timing hash_65536 | `records/hash-23/QILIN-768__hash_65536.json` |
| `QILIN-768` | timing hash_8192 | `records/hash-23/QILIN-768__hash_8192.json` |
| `QILIN-1024` | KAT log (sha256 `df58e3c7abfc8432…`) | `kat/hash-23/QILIN-1024.log` |
| `QILIN-1024` | timing hash_1024 | `records/hash-23/QILIN-1024__hash_1024.json` |
| `QILIN-1024` | timing hash_128 | `records/hash-23/QILIN-1024__hash_128.json` |
| `QILIN-1024` | timing hash_16384 | `records/hash-23/QILIN-1024__hash_16384.json` |
| `QILIN-1024` | timing hash_32 | `records/hash-23/QILIN-1024__hash_32.json` |
| `QILIN-1024` | timing hash_4096 | `records/hash-23/QILIN-1024__hash_4096.json` |
| `QILIN-1024` | timing hash_512 | `records/hash-23/QILIN-1024__hash_512.json` |
| `QILIN-1024` | timing hash_65536 | `records/hash-23/QILIN-1024__hash_65536.json` |
| `QILIN-1024` | timing hash_8192 | `records/hash-23/QILIN-1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

