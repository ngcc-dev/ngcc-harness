# kem-33 QUBE — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-33` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560881000828928.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: QUBE
- Implementation versions measured: reference
- Parameter sets: `qube-128`, `qube-192`, `qube-256`, `qube-384`, `qube-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-33/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `qube-128` | harness-default | MISMATCH [1] |
| `qube-192` | harness-default | NOKAT [1] |
| `qube-256` | harness-default | MISMATCH [1] |
| `qube-384` | harness-default | MISMATCH [1] |
| `qube-512` | harness-default | MISMATCH [1] |

[1] The reference sources reproduce their own in-tree KAT files byte for byte, but the top-level Test_Vectors carry the optimized implementation's key and ciphertext sizes at four levels, and no top-level qube-192 vector was submitted (kem-33/README.md, pseudocode.md). These instances are timed anyway; their output is not validated.

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `qube-128` | keygen | 616.9 k | 295 µs | 3.39e+03 | 295 µs | 15775 (5 × 3155) |
| `qube-128` | enc | 657.8 k | 314 µs | 3.18e+03 | 314 µs | 15475 (5 × 3095) |
| `qube-128` | dec | 1.18 M | 561 µs | 1.78e+03 | 561 µs | 8695 (5 × 1739) |
| `qube-192` | keygen | 1.41 M | 672 µs | 1.49e+03 | 672 µs | 7125 (5 × 1425) |
| `qube-192` | enc | 1.48 M | 705 µs | 1.42e+03 | 704 µs | 7035 (5 × 1407) |
| `qube-192` | dec | 2.83 M | 1.35 ms | 740 | 1.35 ms | 3500 (5 × 700) |
| `qube-256` | keygen | 2.89 M | 1.38 ms | 725 | 1.38 ms | 3510 (5 × 702) |
| `qube-256` | enc | 2.92 M | 1.4 ms | 717 | 1.4 ms | 3645 (5 × 729) |
| `qube-256` | dec | 5.95 M | 2.84 ms | 352 | 2.84 ms | 1740 (5 × 348) |
| `qube-384` | keygen | 7.21 M | 3.44 ms | 290 | 3.44 ms | 1420 (5 × 284) |
| `qube-384` | enc | 7.17 M | 3.43 ms | 292 | 3.43 ms | 1460 (5 × 292) |
| `qube-384` | dec | 13.37 M | 6.4 ms | 156 | 6.41 ms | 780 (5 × 156) |
| `qube-512` | keygen | 19.03 M | 9.09 ms | 110 | 9.09 ms | 545 (5 × 109) |
| `qube-512` | enc | 18.67 M | 8.93 ms | 112 | 8.93 ms | 560 (5 × 112) |
| `qube-512` | dec | 29.26 M | 14 ms | 71.3 | 14 ms | 360 (5 × 72) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `qube-128` | keygen | – | 1732 KiB | 1844 KiB |
| `qube-128` | enc | – | 1752 KiB | 1844 KiB |
| `qube-128` | dec | – | 1740 KiB | 1852 KiB |
| `qube-192` | keygen | – | 1720 KiB | 1864 KiB |
| `qube-192` | enc | – | 1792 KiB | 1868 KiB |
| `qube-192` | dec | – | 1784 KiB | 1860 KiB |
| `qube-256` | keygen | – | 1748 KiB | 1888 KiB |
| `qube-256` | enc | – | 1844 KiB | 1920 KiB |
| `qube-256` | dec | – | 1856 KiB | 1956 KiB |
| `qube-384` | keygen | – | 1756 KiB | 2004 KiB |
| `qube-384` | enc | – | 1976 KiB | 2068 KiB |
| `qube-384` | dec | – | 2012 KiB | 2132 KiB |
| `qube-512` | keygen | – | 1816 KiB | 2164 KiB |
| `qube-512` | enc | – | 2124 KiB | 2256 KiB |
| `qube-512` | dec | – | 2212 KiB | 2300 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `qube-128` | 3294 | 3326 | 3287 | 16 |
| `qube-192` | 6364 | 6412 | 6362 | 24 |
| `qube-256` | 10446 | 10510 | 10423 | 32 |
| `qube-384` | 20738 | 20834 | 20633 | 48 |
| `qube-512` | 34028 | 34156 | 33846 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `qube-128` | keygen | 55% | 1.5% | drng 2, pseudoXOF 7.07, sm3hash 1 |
| `qube-128` | enc | 62% | 1.4% | drng 2, pseudoXOF 7, sm3hash 2 |
| `qube-128` | dec | 49% | 0.0% | pseudoXOF 11, sm3hash 4 |
| `qube-192` | keygen | 49% | 0.7% | drng 2, pseudoXOF 8, pseudohash 1 |
| `qube-192` | enc | 56% | 0.6% | drng 2, pseudoXOF 8, pseudohash 1, sm3hash 1 |
| `qube-192` | dec | 41% | 0.0% | pseudoXOF 13, pseudohash 1, sm3hash 3 |
| `qube-256` | keygen | 47% | 0.3% | drng 2, pseudoXOF 9, pseudohash 1 |
| `qube-256` | enc | 53% | 0.3% | drng 2, pseudoXOF 8.59, pseudohash 1, sm3hash 1 |
| `qube-256` | dec | 35% | 0.0% | pseudoXOF 14, pseudohash 1, sm3hash 3 |
| `qube-384` | keygen | 38% | 0.2% | drng 2, pseudoXOF 10, pseudohash 1 |
| `qube-384` | enc | 48% | 0.2% | drng 2, pseudoXOF 9, pseudohash 2 |
| `qube-384` | dec | 40% | 0.0% | pseudoXOF 15, pseudohash 4 |
| `qube-512` | keygen | 51% | 0.1% | drng 2, pseudoXOF 10, pseudohash 1 |
| `qube-512` | enc | 60% | 0.1% | drng 2, pseudoXOF 10, pseudohash 2 |
| `qube-512` | dec | 49% | 0.0% | pseudoXOF 17, pseudohash 4 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `qube-128` | KAT log (sha256 `d42d6cf140962d85…`) | `kat/kem-33/qube-128.log` |
| `qube-128` | timing dec | `records/kem-33/qube-128__dec.json` |
| `qube-128` | timing enc | `records/kem-33/qube-128__enc.json` |
| `qube-128` | timing keygen | `records/kem-33/qube-128__keygen.json` |
| `qube-128` | hash profile dec | `profile/kem-33/qube-128__dec.json` |
| `qube-128` | hash profile enc | `profile/kem-33/qube-128__enc.json` |
| `qube-128` | hash profile keygen | `profile/kem-33/qube-128__keygen.json` |
| `qube-192` | KAT log (sha256 `e582d4539fa49366…`) | `kat/kem-33/qube-192.log` |
| `qube-192` | timing dec | `records/kem-33/qube-192__dec.json` |
| `qube-192` | timing enc | `records/kem-33/qube-192__enc.json` |
| `qube-192` | timing keygen | `records/kem-33/qube-192__keygen.json` |
| `qube-192` | hash profile dec | `profile/kem-33/qube-192__dec.json` |
| `qube-192` | hash profile enc | `profile/kem-33/qube-192__enc.json` |
| `qube-192` | hash profile keygen | `profile/kem-33/qube-192__keygen.json` |
| `qube-256` | KAT log (sha256 `5ae74b28d91e7bcc…`) | `kat/kem-33/qube-256.log` |
| `qube-256` | timing dec | `records/kem-33/qube-256__dec.json` |
| `qube-256` | timing enc | `records/kem-33/qube-256__enc.json` |
| `qube-256` | timing keygen | `records/kem-33/qube-256__keygen.json` |
| `qube-256` | hash profile dec | `profile/kem-33/qube-256__dec.json` |
| `qube-256` | hash profile enc | `profile/kem-33/qube-256__enc.json` |
| `qube-256` | hash profile keygen | `profile/kem-33/qube-256__keygen.json` |
| `qube-384` | KAT log (sha256 `575ed37bd6a07bfb…`) | `kat/kem-33/qube-384.log` |
| `qube-384` | timing dec | `records/kem-33/qube-384__dec.json` |
| `qube-384` | timing enc | `records/kem-33/qube-384__enc.json` |
| `qube-384` | timing keygen | `records/kem-33/qube-384__keygen.json` |
| `qube-384` | hash profile dec | `profile/kem-33/qube-384__dec.json` |
| `qube-384` | hash profile enc | `profile/kem-33/qube-384__enc.json` |
| `qube-384` | hash profile keygen | `profile/kem-33/qube-384__keygen.json` |
| `qube-512` | KAT log (sha256 `735bd1cc5ccd2967…`) | `kat/kem-33/qube-512.log` |
| `qube-512` | timing dec | `records/kem-33/qube-512__dec.json` |
| `qube-512` | timing enc | `records/kem-33/qube-512__enc.json` |
| `qube-512` | timing keygen | `records/kem-33/qube-512__keygen.json` |
| `qube-512` | hash profile dec | `profile/kem-33/qube-512__dec.json` |
| `qube-512` | hash profile enc | `profile/kem-33/qube-512__enc.json` |
| `qube-512` | hash profile keygen | `profile/kem-33/qube-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

