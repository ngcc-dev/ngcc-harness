# kem-12 CTL Algorithm — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-12` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560844426498048.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: CTL Algorithm
- Implementation versions measured: reference
- Parameter sets: `CTL-257-512`, `CTL-769-1024`, `CTL-3329-2048`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-12/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `CTL-257-512` | guide | PASS |
| `CTL-769-1024` | guide | PASS |
| `CTL-3329-2048` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `CTL-257-512` | keygen | 14.17 M | 5.26 ms | 190 | 5.26 ms | 475 (5 × 95) |
| `CTL-257-512` | enc | 49.2 k | 18.2 µs | 5.48e+04 | 18.2 µs | 100000 (5 × 20000) |
| `CTL-257-512` | dec | 265.8 k | 98.6 µs | 1.01e+04 | 98.6 µs | 24565 (5 × 4913) |
| `CTL-769-1024` | keygen | 70.49 M | 26.2 ms | 38.2 | 26.2 ms | 120 (5 × 24) |
| `CTL-769-1024` | enc | 87.0 k | 32.3 µs | 3.1e+04 | 32.3 µs | 59705 (5 × 11941) |
| `CTL-769-1024` | dec | 539.7 k | 200 µs | 4.99e+03 | 200 µs | 11860 (5 × 2372) |
| `CTL-3329-2048` | keygen | 2.82 G | 1.05 s | 0.954 | 1.04 s | 100 (5 × 20) |
| `CTL-3329-2048` | enc | 9.01 M | 3.34 ms | 299 | 3.34 ms | 745 (5 × 149) |
| `CTL-3329-2048` | dec | 75.43 M | 28 ms | 35.7 | 28 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `CTL-257-512` | keygen | 348024 | 1796 KiB | 2312 KiB |
| `CTL-257-512` | enc | 348024 | 2232 KiB | 2296 KiB |
| `CTL-257-512` | dec | 348024 | 2240 KiB | 2304 KiB |
| `CTL-769-1024` | keygen | 352504 | 1804 KiB | 2360 KiB |
| `CTL-769-1024` | enc | 352504 | 2280 KiB | 2344 KiB |
| `CTL-769-1024` | dec | 352504 | 2284 KiB | 2352 KiB |
| `CTL-3329-2048` | keygen | 348592 | 3860 KiB | 8236 KiB |
| `CTL-3329-2048` | enc | 348592 | 5780 KiB | 6232 KiB |
| `CTL-3329-2048` | dec | 348592 | 6180 KiB | 6244 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `CTL-257-512` | 521 | 2953 | 473 | 16 |
| `CTL-769-1024` | 1230 | 6030 | 1006 | 32 |
| `CTL-3329-2048` | 3009 | 15617 | 2353 | 48 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — sha3.c shim has no permutation and is unused

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `CTL-257-512` | keygen | 10% | 0.0% | drng 1, pseudoXOF 266 |
| `CTL-257-512` | enc | 14% | 12% | drng 1, pseudoXOF 3 |
| `CTL-257-512` | dec | 7.6% | 0.0% | pseudoXOF 4 |
| `CTL-769-1024` | keygen | 2.9% | 0.0% | drng 1, pseudoXOF 342 |
| `CTL-769-1024` | enc | 13% | 4.9% | drng 1, pseudoXOF 2, sm3hash 1 |
| `CTL-769-1024` | dec | 6.4% | 0.0% | pseudoXOF 3, sm3hash 1 |
| `CTL-3329-2048` | keygen | 0.1% | 0.0% | drng 1, pseudoXOF 260 |
| `CTL-3329-2048` | enc | 0.4% | 0.1% | drng 1, pseudoXOF 3 |
| `CTL-3329-2048` | dec | 0.3% | 0.0% | pseudoXOF 4 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `CTL-257-512` | KAT log (sha256 `1898ce8c54d0ab69…`) | `kat/kem-12/CTL-257-512.log` |
| `CTL-257-512` | timing dec | `records/kem-12/CTL-257-512__dec.json` |
| `CTL-257-512` | timing enc | `records/kem-12/CTL-257-512__enc.json` |
| `CTL-257-512` | timing keygen | `records/kem-12/CTL-257-512__keygen.json` |
| `CTL-257-512` | hash profile dec | `profile/kem-12/CTL-257-512__dec.json` |
| `CTL-257-512` | hash profile enc | `profile/kem-12/CTL-257-512__enc.json` |
| `CTL-257-512` | hash profile keygen | `profile/kem-12/CTL-257-512__keygen.json` |
| `CTL-769-1024` | KAT log (sha256 `e3752c5c8a065cce…`) | `kat/kem-12/CTL-769-1024.log` |
| `CTL-769-1024` | timing dec | `records/kem-12/CTL-769-1024__dec.json` |
| `CTL-769-1024` | timing enc | `records/kem-12/CTL-769-1024__enc.json` |
| `CTL-769-1024` | timing keygen | `records/kem-12/CTL-769-1024__keygen.json` |
| `CTL-769-1024` | hash profile dec | `profile/kem-12/CTL-769-1024__dec.json` |
| `CTL-769-1024` | hash profile enc | `profile/kem-12/CTL-769-1024__enc.json` |
| `CTL-769-1024` | hash profile keygen | `profile/kem-12/CTL-769-1024__keygen.json` |
| `CTL-3329-2048` | KAT log (sha256 `568dfa032b2af9b2…`) | `kat/kem-12/CTL-3329-2048.log` |
| `CTL-3329-2048` | timing dec | `records/kem-12/CTL-3329-2048__dec.json` |
| `CTL-3329-2048` | timing enc | `records/kem-12/CTL-3329-2048__enc.json` |
| `CTL-3329-2048` | timing keygen | `records/kem-12/CTL-3329-2048__keygen.json` |
| `CTL-3329-2048` | hash profile dec | `profile/kem-12/CTL-3329-2048__dec.json` |
| `CTL-3329-2048` | hash profile enc | `profile/kem-12/CTL-3329-2048__enc.json` |
| `CTL-3329-2048` | hash profile keygen | `profile/kem-12/CTL-3329-2048__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

