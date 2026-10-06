# kem-40 YuanYang.KEM — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-40` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560890442207232.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: YuanYang.KEM
- Implementation versions measured: reference
- Parameter sets: `yuanyang-512`, `yuanyang-1024`, `yuanyang-2048`

## 2. Assessment environment

| item | value |
|---|---|
| processor | 12th Gen Intel(R) Core(TM) i7-12700 (CPU 2, one core) |
| clock | max 2.10 GHz, governor performance, turbo off, SMT off |
| memory | 31788 MiB |
| OS / kernel | Debian GNU/Linux 13 (trixie) / 6.12.107+deb13-amd64 |
| compiler / build tool | gcc (Debian 14.2.0-19) 14.2.0 / cmake version 3.31.6 |
| campaign start / end (UTC) | 2026-09-25T10:02:21 / 2026-09-28T10:51:21 |

## 3. Functional testing (KAT)

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-40/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `yuanyang-512` | guide | PASS |
| `yuanyang-1024` | guide | PASS |
| `yuanyang-2048` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `yuanyang-512` | keygen | 505.6 k | 242 µs | 4.14e+03 | 242 µs | 16905 (5 × 3381) |
| `yuanyang-512` | enc | 288.3 k | 138 µs | 7.26e+03 | 138 µs | 34725 (5 × 6945) |
| `yuanyang-512` | dec | 433.4 k | 207 µs | 4.83e+03 | 207 µs | 23490 (5 × 4698) |
| `yuanyang-1024` | keygen | 1.07 M | 509 µs | 1.96e+03 | 509 µs | 8990 (5 × 1798) |
| `yuanyang-1024` | enc | 495.2 k | 237 µs | 4.23e+03 | 235 µs | 20480 (5 × 4096) |
| `yuanyang-1024` | dec | 808.6 k | 386 µs | 2.59e+03 | 386 µs | 12740 (5 × 2548) |
| `yuanyang-2048` | keygen | 2.06 M | 984 µs | 1.02e+03 | 984 µs | 4375 (5 × 875) |
| `yuanyang-2048` | enc | 991.5 k | 474 µs | 2.11e+03 | 474 µs | 10450 (5 × 2090) |
| `yuanyang-2048` | dec | 1.68 M | 801 µs | 1.25e+03 | 801 µs | 6135 (5 × 1227) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `yuanyang-512` | keygen | 48461 | 2040 KiB | 2296 KiB |
| `yuanyang-512` | enc | 48461 | 2244 KiB | 2308 KiB |
| `yuanyang-512` | dec | 48461 | 2228 KiB | 2348 KiB |
| `yuanyang-1024` | keygen | 53077 | 2036 KiB | 2320 KiB |
| `yuanyang-1024` | enc | 53077 | 2260 KiB | 2324 KiB |
| `yuanyang-1024` | dec | 53077 | 2248 KiB | 2360 KiB |
| `yuanyang-2048` | keygen | 61945 | 2052 KiB | 2380 KiB |
| `yuanyang-2048` | enc | 61945 | 2288 KiB | 2356 KiB |
| `yuanyang-2048` | dec | 61945 | 2320 KiB | 2388 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `yuanyang-512` | 736 | 1536 | 656 | 16 |
| `yuanyang-1024` | 1568 | 3168 | 1344 | 32 |
| `yuanyang-2048` | 3328 | 6528 | 2880 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `yuanyang-512` | keygen | 8.1% | 23% | drng 2.35, pseudohash 1 |
| `yuanyang-512` | enc | 42% | 21% | drng 3, pseudohash 4 |
| `yuanyang-512` | dec | 21% | 13% | drng 2, pseudohash 4 |
| `yuanyang-1024` | keygen | 7.2% | 19% | drng 4, pseudohash 1 |
| `yuanyang-1024` | enc | 38% | 22% | drng 4, pseudohash 4 |
| `yuanyang-1024` | dec | 15% | 13% | drng 3, pseudohash 4 |
| `yuanyang-2048` | keygen | 7.5% | 17% | drng 6.79, pseudohash 1 |
| `yuanyang-2048` | enc | 34% | 21% | drng 6, pseudohash 4 |
| `yuanyang-2048` | dec | 12% | 12% | drng 5, pseudohash 4 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `yuanyang-512` | KAT log (sha256 `57521d656ee2765d…`) | `kat/kem-40/yuanyang-512.log` |
| `yuanyang-512` | timing dec | `records/kem-40/yuanyang-512__dec.json` |
| `yuanyang-512` | timing enc | `records/kem-40/yuanyang-512__enc.json` |
| `yuanyang-512` | timing keygen | `records/kem-40/yuanyang-512__keygen.json` |
| `yuanyang-512` | hash profile dec | `profile/kem-40/yuanyang-512__dec.json` |
| `yuanyang-512` | hash profile enc | `profile/kem-40/yuanyang-512__enc.json` |
| `yuanyang-512` | hash profile keygen | `profile/kem-40/yuanyang-512__keygen.json` |
| `yuanyang-1024` | KAT log (sha256 `c2901c070c99f13e…`) | `kat/kem-40/yuanyang-1024.log` |
| `yuanyang-1024` | timing dec | `records/kem-40/yuanyang-1024__dec.json` |
| `yuanyang-1024` | timing enc | `records/kem-40/yuanyang-1024__enc.json` |
| `yuanyang-1024` | timing keygen | `records/kem-40/yuanyang-1024__keygen.json` |
| `yuanyang-1024` | hash profile dec | `profile/kem-40/yuanyang-1024__dec.json` |
| `yuanyang-1024` | hash profile enc | `profile/kem-40/yuanyang-1024__enc.json` |
| `yuanyang-1024` | hash profile keygen | `profile/kem-40/yuanyang-1024__keygen.json` |
| `yuanyang-2048` | KAT log (sha256 `a6f936079f75c299…`) | `kat/kem-40/yuanyang-2048.log` |
| `yuanyang-2048` | timing dec | `records/kem-40/yuanyang-2048__dec.json` |
| `yuanyang-2048` | timing enc | `records/kem-40/yuanyang-2048__enc.json` |
| `yuanyang-2048` | timing keygen | `records/kem-40/yuanyang-2048__keygen.json` |
| `yuanyang-2048` | hash profile dec | `profile/kem-40/yuanyang-2048__dec.json` |
| `yuanyang-2048` | hash profile enc | `profile/kem-40/yuanyang-2048__enc.json` |
| `yuanyang-2048` | hash profile keygen | `profile/kem-40/yuanyang-2048__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

