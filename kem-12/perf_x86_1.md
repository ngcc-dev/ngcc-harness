# kem-12 CTL Algorithm — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-12` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560844426498048.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: CTL Algorithm
- Implementation versions measured: reference
- Parameter sets: `CTL-257-512`, `CTL-769-1024`, `CTL-3329-2048`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-12/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `CTL-257-512` | guide | PASS |
| `CTL-769-1024` | guide | PASS |
| `CTL-3329-2048` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `CTL-257-512` | keygen | 17.76 M | 8.48 ms | 118 | 8.49 ms | 595 (5 × 119) |
| `CTL-257-512` | enc | 61.0 k | 29.1 µs | 3.43e+04 | 29.1 µs | 100000 (5 × 20000) |
| `CTL-257-512` | dec | 349.0 k | 167 µs | 6e+03 | 167 µs | 28460 (5 × 5692) |
| `CTL-769-1024` | keygen | 79.80 M | 38.2 ms | 26.2 | 38.2 ms | 155 (5 × 31) |
| `CTL-769-1024` | enc | 113.7 k | 54.3 µs | 1.84e+04 | 54.3 µs | 80625 (5 × 16125) |
| `CTL-769-1024` | dec | 711.3 k | 340 µs | 2.94e+03 | 340 µs | 14345 (5 × 2869) |
| `CTL-3329-2048` | keygen | 2.15 G | 1.03 s | 0.967 | 1.04 s | 100 (5 × 20) |
| `CTL-3329-2048` | enc | 11.91 M | 5.72 ms | 175 | 5.72 ms | 880 (5 × 176) |
| `CTL-3329-2048` | dec | 79.73 M | 38.4 ms | 26 | 38.4 ms | 135 (5 × 27) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `CTL-257-512` | keygen | 391997 | 2496 KiB | 2940 KiB |
| `CTL-257-512` | enc | 391997 | 2768 KiB | 2944 KiB |
| `CTL-257-512` | dec | 391997 | 2780 KiB | 2952 KiB |
| `CTL-769-1024` | keygen | 392261 | 2504 KiB | 3012 KiB |
| `CTL-769-1024` | enc | 392261 | 2820 KiB | 3000 KiB |
| `CTL-769-1024` | dec | 392261 | 2888 KiB | 3032 KiB |
| `CTL-3329-2048` | keygen | 392317 | 2528 KiB | 6432 KiB |
| `CTL-3329-2048` | enc | 392317 | 4292 KiB | 4580 KiB |
| `CTL-3329-2048` | dec | 392317 | 4324 KiB | 4648 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

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
| `CTL-257-512` | keygen | 8.4% | 0.0% | drng 1, pseudoXOF 257 |
| `CTL-257-512` | enc | 12% | 10% | drng 1, pseudoXOF 3 |
| `CTL-257-512` | dec | 6.2% | 0.0% | pseudoXOF 4 |
| `CTL-769-1024` | keygen | 2.5% | 0.0% | drng 1, pseudoXOF 337 |
| `CTL-769-1024` | enc | 10% | 4.0% | drng 1, pseudoXOF 2, sm3hash 1 |
| `CTL-769-1024` | dec | 5.1% | 0.0% | pseudoXOF 3, sm3hash 1 |
| `CTL-3329-2048` | keygen | 0.1% | 0.0% | drng 1, pseudoXOF 260 |
| `CTL-3329-2048` | enc | 0.4% | 0.1% | drng 1, pseudoXOF 3 |
| `CTL-3329-2048` | dec | 0.3% | 0.0% | pseudoXOF 4 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `CTL-257-512` | KAT log (sha256 `6d4cbf596edd6540…`) | `kat/kem-12/CTL-257-512.log` |
| `CTL-257-512` | timing dec | `records/kem-12/CTL-257-512__dec.json` |
| `CTL-257-512` | timing enc | `records/kem-12/CTL-257-512__enc.json` |
| `CTL-257-512` | timing keygen | `records/kem-12/CTL-257-512__keygen.json` |
| `CTL-257-512` | hash profile dec | `profile/kem-12/CTL-257-512__dec.json` |
| `CTL-257-512` | hash profile enc | `profile/kem-12/CTL-257-512__enc.json` |
| `CTL-257-512` | hash profile keygen | `profile/kem-12/CTL-257-512__keygen.json` |
| `CTL-769-1024` | KAT log (sha256 `a90c0b008f11c9dd…`) | `kat/kem-12/CTL-769-1024.log` |
| `CTL-769-1024` | timing dec | `records/kem-12/CTL-769-1024__dec.json` |
| `CTL-769-1024` | timing enc | `records/kem-12/CTL-769-1024__enc.json` |
| `CTL-769-1024` | timing keygen | `records/kem-12/CTL-769-1024__keygen.json` |
| `CTL-769-1024` | hash profile dec | `profile/kem-12/CTL-769-1024__dec.json` |
| `CTL-769-1024` | hash profile enc | `profile/kem-12/CTL-769-1024__enc.json` |
| `CTL-769-1024` | hash profile keygen | `profile/kem-12/CTL-769-1024__keygen.json` |
| `CTL-3329-2048` | KAT log (sha256 `d6c583cccfe71aa0…`) | `kat/kem-12/CTL-3329-2048.log` |
| `CTL-3329-2048` | timing dec | `records/kem-12/CTL-3329-2048__dec.json` |
| `CTL-3329-2048` | timing enc | `records/kem-12/CTL-3329-2048__enc.json` |
| `CTL-3329-2048` | timing keygen | `records/kem-12/CTL-3329-2048__keygen.json` |
| `CTL-3329-2048` | hash profile dec | `profile/kem-12/CTL-3329-2048__dec.json` |
| `CTL-3329-2048` | hash profile enc | `profile/kem-12/CTL-3329-2048__enc.json` |
| `CTL-3329-2048` | hash profile keygen | `profile/kem-12/CTL-3329-2048__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

