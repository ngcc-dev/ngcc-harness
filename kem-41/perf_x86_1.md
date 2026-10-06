# kem-41 ZEN — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-41` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560890563842048.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: ZEN
- Implementation versions measured: reference
- Parameter sets: `ZEN_128`, `ZEN_256`, `ZEN_512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-41/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `ZEN_128` | guide | PASS |
| `ZEN_256` | guide | PASS |
| `ZEN_512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `ZEN_128` | keygen | 222.8 k | 106 µs | 9.4e+03 | 106 µs | 38060 (5 × 7612) |
| `ZEN_128` | enc | 143.8 k | 68.7 µs | 1.46e+04 | 68.7 µs | 65455 (5 × 13091) |
| `ZEN_128` | dec | 191.9 k | 91.6 µs | 1.09e+04 | 91.6 µs | 50210 (5 × 10042) |
| `ZEN_256` | keygen | 391.5 k | 187 µs | 5.35e+03 | 187 µs | 24105 (5 × 4821) |
| `ZEN_256` | enc | 200.6 k | 95.8 µs | 1.04e+04 | 95.9 µs | 48975 (5 × 9795) |
| `ZEN_256` | dec | 327.4 k | 156 µs | 6.39e+03 | 156 µs | 30765 (5 × 6153) |
| `ZEN_512` | keygen | 970.2 k | 463 µs | 2.16e+03 | 463 µs | 11600 (5 × 2320) |
| `ZEN_512` | enc | 501.9 k | 240 µs | 4.17e+03 | 240 µs | 19830 (5 × 3966) |
| `ZEN_512` | dec | 788.0 k | 376 µs | 2.66e+03 | 377 µs | 12735 (5 × 2547) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `ZEN_128` | keygen | 37285 | 1720 KiB | 1812 KiB |
| `ZEN_128` | enc | 37285 | 1712 KiB | 1776 KiB |
| `ZEN_128` | dec | 37285 | 1728 KiB | 1792 KiB |
| `ZEN_256` | keygen | 43773 | 1716 KiB | 1832 KiB |
| `ZEN_256` | enc | 43773 | 1712 KiB | 1776 KiB |
| `ZEN_256` | dec | 43773 | 1716 KiB | 1832 KiB |
| `ZEN_512` | keygen | 54309 | 1728 KiB | 1860 KiB |
| `ZEN_512` | enc | 54309 | 1768 KiB | 1848 KiB |
| `ZEN_512` | dec | 54309 | 1776 KiB | 1840 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `ZEN_128` | 615 | 1303 | 512 | 16 |
| `ZEN_256` | 1229 | 2605 | 1024 | 32 |
| `ZEN_512` | 2458 | 5210 | 2048 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `ZEN_128` | keygen | 29% | 4.8% | drng 2, pseudoXOF 3.05, sm3hash 1 |
| `ZEN_128` | enc | 67% | 3.2% | drng 1, pseudoXOF 2, pseudohash 1, sm3hash 1 |
| `ZEN_128` | dec | 49% | 0.0% | pseudoXOF 2, pseudohash 1, sm3hash 1 |
| `ZEN_256` | keygen | 36% | 2.8% | drng 2, pseudoXOF 3.02, sm3hash 1 |
| `ZEN_256` | enc | 47% | 2.3% | drng 1, pseudoXOF 2, pseudohash 1, sm3hash 1 |
| `ZEN_256` | dec | 28% | 0.0% | pseudoXOF 2, pseudohash 1, sm3hash 1 |
| `ZEN_512` | keygen | 46% | 1.3% | drng 2, pseudoXOF 2.93, pseudohash 1 |
| `ZEN_512` | enc | 57% | 1.2% | drng 1, pseudoXOF 2, pseudohash 2 |
| `ZEN_512` | dec | 34% | 0.0% | pseudoXOF 3, pseudohash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `ZEN_128` | KAT log (sha256 `8a3a3eca0c641b15…`) | `kat/kem-41/ZEN_128.log` |
| `ZEN_128` | timing dec | `records/kem-41/ZEN_128__dec.json` |
| `ZEN_128` | timing enc | `records/kem-41/ZEN_128__enc.json` |
| `ZEN_128` | timing keygen | `records/kem-41/ZEN_128__keygen.json` |
| `ZEN_128` | hash profile dec | `profile/kem-41/ZEN_128__dec.json` |
| `ZEN_128` | hash profile enc | `profile/kem-41/ZEN_128__enc.json` |
| `ZEN_128` | hash profile keygen | `profile/kem-41/ZEN_128__keygen.json` |
| `ZEN_256` | KAT log (sha256 `1845f0d8c0b37b47…`) | `kat/kem-41/ZEN_256.log` |
| `ZEN_256` | timing dec | `records/kem-41/ZEN_256__dec.json` |
| `ZEN_256` | timing enc | `records/kem-41/ZEN_256__enc.json` |
| `ZEN_256` | timing keygen | `records/kem-41/ZEN_256__keygen.json` |
| `ZEN_256` | hash profile dec | `profile/kem-41/ZEN_256__dec.json` |
| `ZEN_256` | hash profile enc | `profile/kem-41/ZEN_256__enc.json` |
| `ZEN_256` | hash profile keygen | `profile/kem-41/ZEN_256__keygen.json` |
| `ZEN_512` | KAT log (sha256 `1cf6340204719a3f…`) | `kat/kem-41/ZEN_512.log` |
| `ZEN_512` | timing dec | `records/kem-41/ZEN_512__dec.json` |
| `ZEN_512` | timing enc | `records/kem-41/ZEN_512__enc.json` |
| `ZEN_512` | timing keygen | `records/kem-41/ZEN_512__keygen.json` |
| `ZEN_512` | hash profile dec | `profile/kem-41/ZEN_512__dec.json` |
| `ZEN_512` | hash profile enc | `profile/kem-41/ZEN_512__enc.json` |
| `ZEN_512` | hash profile keygen | `profile/kem-41/ZEN_512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

