# hash-04 CHAMP — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-04` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101540375996485632.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: CHAMP
- Implementation versions measured: reference
- Parameter sets: `CHAMP-512`, `CHAMP-1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-04/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `CHAMP-512` | guide | PASS |
| `CHAMP-1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `CHAMP-512` | 32 B | 87.8 k | 2742.4 | 32.6 µs | 1.0 | 9790 (5 × 1958) |
| `CHAMP-512` | 128 B | 132.0 k | 1031.4 | 49 µs | 2.6 | 7775 (5 × 1555) |
| `CHAMP-512` | 512 B | 341.9 k | 667.8 | 127 µs | 4.0 | 7980 (5 × 1596) |
| `CHAMP-512` | 1024 B | 687.1 k | 671.0 | 255 µs | 4.0 | 6555 (5 × 1311) |
| `CHAMP-512` | 4096 B | 2.72 M | 664.3 | 1.01 ms | 4.1 | 3065 (5 × 613) |
| `CHAMP-512` | 8192 B | 5.19 M | 633.8 | 1.93 ms | 4.3 | 1135 (5 × 227) |
| `CHAMP-512` | 16384 B | 10.58 M | 645.5 | 3.92 ms | 4.2 | 1025 (5 × 205) |
| `CHAMP-512` | 65536 B | 37.40 M | 570.7 | 13.9 ms | 4.7 | 275 (5 × 55) |
| `CHAMP-1024` | 32 B | 324.1 k | 10128.7 | 120 µs | 0.3 | 3020 (5 × 604) |
| `CHAMP-1024` | 128 B | 441.2 k | 3447.0 | 164 µs | 0.8 | 2905 (5 × 581) |
| `CHAMP-1024` | 512 B | 902.9 k | 1763.6 | 335 µs | 1.5 | 2515 (5 × 503) |
| `CHAMP-1024` | 1024 B | 1.52 M | 1483.8 | 564 µs | 1.8 | 2130 (5 × 426) |
| `CHAMP-1024` | 4096 B | 5.23 M | 1275.7 | 1.94 ms | 2.1 | 1030 (5 × 206) |
| `CHAMP-1024` | 8192 B | 10.16 M | 1240.8 | 3.77 ms | 2.2 | 665 (5 × 133) |
| `CHAMP-1024` | 16384 B | 20.04 M | 1223.2 | 7.43 ms | 2.2 | 380 (5 × 76) |
| `CHAMP-1024` | 65536 B | 85.04 M | 1297.5 | 31.5 ms | 2.1 | 105 (5 × 21) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `CHAMP-512` | hash_32 | 28420 | 1400 KiB | 1480 KiB |
| `CHAMP-512` | hash_128 | 28420 | 1400 KiB | 1480 KiB |
| `CHAMP-512` | hash_512 | 28420 | 1396 KiB | 1476 KiB |
| `CHAMP-512` | hash_1024 | 28420 | 1404 KiB | 1484 KiB |
| `CHAMP-512` | hash_4096 | 28420 | 1404 KiB | 1484 KiB |
| `CHAMP-512` | hash_8192 | 28420 | 1408 KiB | 1488 KiB |
| `CHAMP-512` | hash_16384 | 28420 | 1416 KiB | 1496 KiB |
| `CHAMP-512` | hash_65536 | 28420 | 3496 KiB | 3576 KiB |
| `CHAMP-1024` | hash_32 | 45316 | 1400 KiB | 1496 KiB |
| `CHAMP-1024` | hash_128 | 45316 | 1400 KiB | 1496 KiB |
| `CHAMP-1024` | hash_512 | 45316 | 1400 KiB | 1496 KiB |
| `CHAMP-1024` | hash_1024 | 45316 | 1404 KiB | 1500 KiB |
| `CHAMP-1024` | hash_4096 | 45316 | 1404 KiB | 1500 KiB |
| `CHAMP-1024` | hash_8192 | 45316 | 1404 KiB | 1500 KiB |
| `CHAMP-1024` | hash_16384 | 45316 | 1416 KiB | 1512 KiB |
| `CHAMP-1024` | hash_65536 | 45316 | 1464 KiB | 1560 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `CHAMP-512` | KAT log (sha256 `a02431baee612899…`) | `kat/hash-04/CHAMP-512.log` |
| `CHAMP-512` | timing hash_1024 | `records/hash-04/CHAMP-512__hash_1024.json` |
| `CHAMP-512` | timing hash_128 | `records/hash-04/CHAMP-512__hash_128.json` |
| `CHAMP-512` | timing hash_16384 | `records/hash-04/CHAMP-512__hash_16384.json` |
| `CHAMP-512` | timing hash_32 | `records/hash-04/CHAMP-512__hash_32.json` |
| `CHAMP-512` | timing hash_4096 | `records/hash-04/CHAMP-512__hash_4096.json` |
| `CHAMP-512` | timing hash_512 | `records/hash-04/CHAMP-512__hash_512.json` |
| `CHAMP-512` | timing hash_65536 | `records/hash-04/CHAMP-512__hash_65536.json` |
| `CHAMP-512` | timing hash_8192 | `records/hash-04/CHAMP-512__hash_8192.json` |
| `CHAMP-1024` | KAT log (sha256 `3da577f333dc5ebc…`) | `kat/hash-04/CHAMP-1024.log` |
| `CHAMP-1024` | timing hash_1024 | `records/hash-04/CHAMP-1024__hash_1024.json` |
| `CHAMP-1024` | timing hash_128 | `records/hash-04/CHAMP-1024__hash_128.json` |
| `CHAMP-1024` | timing hash_16384 | `records/hash-04/CHAMP-1024__hash_16384.json` |
| `CHAMP-1024` | timing hash_32 | `records/hash-04/CHAMP-1024__hash_32.json` |
| `CHAMP-1024` | timing hash_4096 | `records/hash-04/CHAMP-1024__hash_4096.json` |
| `CHAMP-1024` | timing hash_512 | `records/hash-04/CHAMP-1024__hash_512.json` |
| `CHAMP-1024` | timing hash_65536 | `records/hash-04/CHAMP-1024__hash_65536.json` |
| `CHAMP-1024` | timing hash_8192 | `records/hash-04/CHAMP-1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

