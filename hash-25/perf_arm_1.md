# hash-25 TaiChi — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-25` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101536036577955840.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: TaiChi
- Implementation versions measured: reference
- Parameter sets: `TaiChi-512`, `TaiChi-768`, `TaiChi-1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-25/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `TaiChi-512` | guide | PASS |
| `TaiChi-768` | guide | PASS |
| `TaiChi-1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `TaiChi-512` | 32 B | 26.8 k | 836.2 | 9.93 µs | 3.2 | 100000 (5 × 20000) |
| `TaiChi-512` | 128 B | 24.7 k | 192.7 | 9.16 µs | 14.0 | 100000 (5 × 20000) |
| `TaiChi-512` | 512 B | 71.0 k | 138.6 | 26.3 µs | 19.4 | 73510 (5 × 14702) |
| `TaiChi-512` | 1024 B | 141.6 k | 138.2 | 52.5 µs | 19.5 | 54360 (5 × 10872) |
| `TaiChi-512` | 4096 B | 570.5 k | 139.3 | 212 µs | 19.3 | 14430 (5 × 2886) |
| `TaiChi-512` | 8192 B | 1.11 M | 134.9 | 410 µs | 20.0 | 7465 (5 × 1493) |
| `TaiChi-512` | 16384 B | 2.21 M | 135.1 | 821 µs | 19.9 | 3810 (5 × 762) |
| `TaiChi-512` | 65536 B | 8.76 M | 133.7 | 3.25 ms | 20.2 | 955 (5 × 191) |
| `TaiChi-768` | 32 B | 25.7 k | 804.7 | 9.56 µs | 3.3 | 100000 (5 × 20000) |
| `TaiChi-768` | 128 B | 23.9 k | 186.9 | 8.88 µs | 14.4 | 100000 (5 × 20000) |
| `TaiChi-768` | 512 B | 95.1 k | 185.8 | 35.3 µs | 14.5 | 74425 (5 × 14885) |
| `TaiChi-768` | 1024 B | 190.5 k | 186.1 | 70.7 µs | 14.5 | 39605 (5 × 7921) |
| `TaiChi-768` | 4096 B | 683.4 k | 166.8 | 254 µs | 16.2 | 12230 (5 × 2446) |
| `TaiChi-768` | 8192 B | 1.35 M | 164.8 | 501 µs | 16.4 | 6165 (5 × 1233) |
| `TaiChi-768` | 16384 B | 2.68 M | 163.6 | 995 µs | 16.5 | 3130 (5 × 626) |
| `TaiChi-768` | 65536 B | 10.74 M | 163.9 | 3.99 ms | 16.4 | 765 (5 × 153) |
| `TaiChi-1024` | 32 B | 48.4 k | 1513.9 | 18 µs | 1.8 | 100000 (5 × 20000) |
| `TaiChi-1024` | 128 B | 72.3 k | 564.6 | 26.8 µs | 4.8 | 89555 (5 × 17911) |
| `TaiChi-1024` | 512 B | 142.0 k | 277.3 | 52.8 µs | 9.7 | 52635 (5 × 10527) |
| `TaiChi-1024` | 1024 B | 261.0 k | 254.9 | 97.1 µs | 10.5 | 30675 (5 × 6135) |
| `TaiChi-1024` | 4096 B | 896.0 k | 218.8 | 333 µs | 12.3 | 9420 (5 × 1884) |
| `TaiChi-1024` | 8192 B | 1.76 M | 215.0 | 654 µs | 12.5 | 4770 (5 × 954) |
| `TaiChi-1024` | 16384 B | 3.48 M | 212.2 | 1.29 ms | 12.7 | 2335 (5 × 467) |
| `TaiChi-1024` | 65536 B | 13.83 M | 211.0 | 5.13 ms | 12.8 | 600 (5 × 120) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `TaiChi-512` | hash_32 | 12732 | 1400 KiB | 1464 KiB |
| `TaiChi-512` | hash_128 | 12732 | 1396 KiB | 1460 KiB |
| `TaiChi-512` | hash_512 | 12732 | 1400 KiB | 1468 KiB |
| `TaiChi-512` | hash_1024 | 12732 | 1404 KiB | 1468 KiB |
| `TaiChi-512` | hash_4096 | 12732 | 1404 KiB | 1472 KiB |
| `TaiChi-512` | hash_8192 | 12732 | 1408 KiB | 1480 KiB |
| `TaiChi-512` | hash_16384 | 12732 | 1416 KiB | 1496 KiB |
| `TaiChi-512` | hash_65536 | 12732 | 1464 KiB | 1592 KiB |
| `TaiChi-768` | hash_32 | 12732 | 1400 KiB | 1464 KiB |
| `TaiChi-768` | hash_128 | 12732 | 1400 KiB | 1464 KiB |
| `TaiChi-768` | hash_512 | 12732 | 1396 KiB | 1464 KiB |
| `TaiChi-768` | hash_1024 | 12732 | 1404 KiB | 1468 KiB |
| `TaiChi-768` | hash_4096 | 12732 | 1400 KiB | 1468 KiB |
| `TaiChi-768` | hash_8192 | 12732 | 1408 KiB | 1480 KiB |
| `TaiChi-768` | hash_16384 | 12732 | 3452 KiB | 3516 KiB |
| `TaiChi-768` | hash_65536 | 12732 | 3484 KiB | 3612 KiB |
| `TaiChi-1024` | hash_32 | 12764 | 1400 KiB | 5992 KiB |
| `TaiChi-1024` | hash_128 | 12764 | 1400 KiB | 7584 KiB |
| `TaiChi-1024` | hash_512 | 12764 | 1396 KiB | 8896 KiB |
| `TaiChi-1024` | hash_1024 | 12764 | 1404 KiB | 10032 KiB |
| `TaiChi-1024` | hash_4096 | 12764 | 1404 KiB | 10852 KiB |
| `TaiChi-1024` | hash_8192 | 12764 | 1408 KiB | 11164 KiB |
| `TaiChi-1024` | hash_16384 | 12764 | 1416 KiB | 10792 KiB |
| `TaiChi-1024` | hash_65536 | 12764 | 1464 KiB | 11380 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `TaiChi-512` | KAT log (sha256 `2bc8a362ec8802a4…`) | `kat/hash-25/TaiChi-512.log` |
| `TaiChi-512` | timing hash_1024 | `records/hash-25/TaiChi-512__hash_1024.json` |
| `TaiChi-512` | timing hash_128 | `records/hash-25/TaiChi-512__hash_128.json` |
| `TaiChi-512` | timing hash_16384 | `records/hash-25/TaiChi-512__hash_16384.json` |
| `TaiChi-512` | timing hash_32 | `records/hash-25/TaiChi-512__hash_32.json` |
| `TaiChi-512` | timing hash_4096 | `records/hash-25/TaiChi-512__hash_4096.json` |
| `TaiChi-512` | timing hash_512 | `records/hash-25/TaiChi-512__hash_512.json` |
| `TaiChi-512` | timing hash_65536 | `records/hash-25/TaiChi-512__hash_65536.json` |
| `TaiChi-512` | timing hash_8192 | `records/hash-25/TaiChi-512__hash_8192.json` |
| `TaiChi-768` | KAT log (sha256 `cbb02706e49c8446…`) | `kat/hash-25/TaiChi-768.log` |
| `TaiChi-768` | timing hash_1024 | `records/hash-25/TaiChi-768__hash_1024.json` |
| `TaiChi-768` | timing hash_128 | `records/hash-25/TaiChi-768__hash_128.json` |
| `TaiChi-768` | timing hash_16384 | `records/hash-25/TaiChi-768__hash_16384.json` |
| `TaiChi-768` | timing hash_32 | `records/hash-25/TaiChi-768__hash_32.json` |
| `TaiChi-768` | timing hash_4096 | `records/hash-25/TaiChi-768__hash_4096.json` |
| `TaiChi-768` | timing hash_512 | `records/hash-25/TaiChi-768__hash_512.json` |
| `TaiChi-768` | timing hash_65536 | `records/hash-25/TaiChi-768__hash_65536.json` |
| `TaiChi-768` | timing hash_8192 | `records/hash-25/TaiChi-768__hash_8192.json` |
| `TaiChi-1024` | KAT log (sha256 `dda0cfc32011b722…`) | `kat/hash-25/TaiChi-1024.log` |
| `TaiChi-1024` | timing hash_1024 | `records/hash-25/TaiChi-1024__hash_1024.json` |
| `TaiChi-1024` | timing hash_128 | `records/hash-25/TaiChi-1024__hash_128.json` |
| `TaiChi-1024` | timing hash_16384 | `records/hash-25/TaiChi-1024__hash_16384.json` |
| `TaiChi-1024` | timing hash_32 | `records/hash-25/TaiChi-1024__hash_32.json` |
| `TaiChi-1024` | timing hash_4096 | `records/hash-25/TaiChi-1024__hash_4096.json` |
| `TaiChi-1024` | timing hash_512 | `records/hash-25/TaiChi-1024__hash_512.json` |
| `TaiChi-1024` | timing hash_65536 | `records/hash-25/TaiChi-1024__hash_65536.json` |
| `TaiChi-1024` | timing hash_8192 | `records/hash-25/TaiChi-1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

