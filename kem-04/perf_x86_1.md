# kem-04 BAG-Piglet — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-04` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560843373727744.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: BAG-Piglet
- Implementation versions measured: reference
- Parameter sets: `bag_piglet_128`, `bag_piglet_256`, `bag_piglet_384`, `bag_piglet_512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-04/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `bag_piglet_128` | guide | PASS |
| `bag_piglet_256` | guide | PASS |
| `bag_piglet_384` | guide | PASS |
| `bag_piglet_512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `bag_piglet_128` | keygen | 1.61 M | 770 µs | 1.3e+03 | 770 µs | 6190 (5 × 1238) |
| `bag_piglet_128` | enc | 3.45 M | 1.65 ms | 607 | 1.65 ms | 3015 (5 × 603) |
| `bag_piglet_128` | dec | 12.12 M | 5.79 ms | 173 | 5.79 ms | 865 (5 × 173) |
| `bag_piglet_256` | keygen | 7.19 M | 3.45 ms | 290 | 3.44 ms | 1310 (5 × 262) |
| `bag_piglet_256` | enc | 22.75 M | 10.9 ms | 91.9 | 10.9 ms | 460 (5 × 92) |
| `bag_piglet_256` | dec | 81.52 M | 39 ms | 25.6 | 39 ms | 130 (5 × 26) |
| `bag_piglet_384` | keygen | 20.60 M | 9.86 ms | 101 | 9.86 ms | 520 (5 × 104) |
| `bag_piglet_384` | enc | 55.27 M | 26.4 ms | 37.9 | 26.4 ms | 190 (5 × 38) |
| `bag_piglet_384` | dec | 221.57 M | 106 ms | 9.44 | 106 ms | 100 (5 × 20) |
| `bag_piglet_512` | keygen | 92.32 M | 44.2 ms | 22.6 | 44.2 ms | 115 (5 × 23) |
| `bag_piglet_512` | enc | 193.81 M | 92.6 ms | 10.8 | 92.6 ms | 100 (5 × 20) |
| `bag_piglet_512` | dec | 725.11 M | 348 ms | 2.87 | 347 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `bag_piglet_128` | keygen | 72473 | 1744 KiB | 1876 KiB |
| `bag_piglet_128` | enc | 72473 | 1776 KiB | 1880 KiB |
| `bag_piglet_128` | dec | 72473 | 1804 KiB | 1896 KiB |
| `bag_piglet_256` | keygen | 73905 | 1784 KiB | 1900 KiB |
| `bag_piglet_256` | enc | 73905 | 1792 KiB | 1860 KiB |
| `bag_piglet_256` | dec | 73905 | 1932 KiB | 2000 KiB |
| `bag_piglet_384` | keygen | 74385 | 1756 KiB | 2000 KiB |
| `bag_piglet_384` | enc | 74385 | 1896 KiB | 1960 KiB |
| `bag_piglet_384` | dec | 74385 | 1972 KiB | 2088 KiB |
| `bag_piglet_512` | keygen | 78705 | 1768 KiB | 2444 KiB |
| `bag_piglet_512` | enc | 78705 | 2168 KiB | 2236 KiB |
| `bag_piglet_512` | dec | 78705 | 2276 KiB | 2552 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `bag_piglet_128` | 522 | 16 | 1027 | 16 |
| `bag_piglet_256` | 1573 | 32 | 3097 | 32 |
| `bag_piglet_384` | 2778 | 48 | 5475 | 48 |
| `bag_piglet_512` | 4158 | 64 | 8204 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — `*_using_shake` names parse pseudoXOF output; AES-256-CTR dead; DRNG only with -DBAG_PIGLET_KAT

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `bag_piglet_128` | keygen | 53% | 0.3% | drng 1, pseudoXOF 4 |
| `bag_piglet_128` | enc | 17% | 0.5% | drng 4, pseudoXOF 5 |
| `bag_piglet_128` | dec | 14% | 0.0% | pseudoXOF 10 |
| `bag_piglet_256` | keygen | 24% | 0.1% | drng 1, pseudoXOF 4 |
| `bag_piglet_256` | enc | 5.6% | 0.1% | drng 5, pseudoXOF 5 |
| `bag_piglet_256` | dec | 4.4% | 0.0% | pseudoXOF 10 |
| `bag_piglet_384` | keygen | 33% | 0.0% | drng 1, pseudoXOF 4 |
| `bag_piglet_384` | enc | 8.9% | 0.0% | drng 5, pseudoXOF 5 |
| `bag_piglet_384` | dec | 6.3% | 0.0% | pseudoXOF 10 |
| `bag_piglet_512` | keygen | 56% | 0.0% | drng 1, pseudoXOF 4 |
| `bag_piglet_512` | enc | 18% | 0.0% | drng 5, pseudoXOF 5 |
| `bag_piglet_512` | dec | 14% | 0.0% | pseudoXOF 10 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `bag_piglet_128` | KAT log (sha256 `dc96d9827839f937…`) | `kat/kem-04/bag_piglet_128.log` |
| `bag_piglet_128` | timing dec | `records/kem-04/bag_piglet_128__dec.json` |
| `bag_piglet_128` | timing enc | `records/kem-04/bag_piglet_128__enc.json` |
| `bag_piglet_128` | timing keygen | `records/kem-04/bag_piglet_128__keygen.json` |
| `bag_piglet_128` | hash profile dec | `profile/kem-04/bag_piglet_128__dec.json` |
| `bag_piglet_128` | hash profile enc | `profile/kem-04/bag_piglet_128__enc.json` |
| `bag_piglet_128` | hash profile keygen | `profile/kem-04/bag_piglet_128__keygen.json` |
| `bag_piglet_256` | KAT log (sha256 `98dcd2b80e466788…`) | `kat/kem-04/bag_piglet_256.log` |
| `bag_piglet_256` | timing dec | `records/kem-04/bag_piglet_256__dec.json` |
| `bag_piglet_256` | timing enc | `records/kem-04/bag_piglet_256__enc.json` |
| `bag_piglet_256` | timing keygen | `records/kem-04/bag_piglet_256__keygen.json` |
| `bag_piglet_256` | hash profile dec | `profile/kem-04/bag_piglet_256__dec.json` |
| `bag_piglet_256` | hash profile enc | `profile/kem-04/bag_piglet_256__enc.json` |
| `bag_piglet_256` | hash profile keygen | `profile/kem-04/bag_piglet_256__keygen.json` |
| `bag_piglet_384` | KAT log (sha256 `6fe1b7d17156c860…`) | `kat/kem-04/bag_piglet_384.log` |
| `bag_piglet_384` | timing dec | `records/kem-04/bag_piglet_384__dec.json` |
| `bag_piglet_384` | timing enc | `records/kem-04/bag_piglet_384__enc.json` |
| `bag_piglet_384` | timing keygen | `records/kem-04/bag_piglet_384__keygen.json` |
| `bag_piglet_384` | hash profile dec | `profile/kem-04/bag_piglet_384__dec.json` |
| `bag_piglet_384` | hash profile enc | `profile/kem-04/bag_piglet_384__enc.json` |
| `bag_piglet_384` | hash profile keygen | `profile/kem-04/bag_piglet_384__keygen.json` |
| `bag_piglet_512` | KAT log (sha256 `43dad9934e0dc8c5…`) | `kat/kem-04/bag_piglet_512.log` |
| `bag_piglet_512` | timing dec | `records/kem-04/bag_piglet_512__dec.json` |
| `bag_piglet_512` | timing enc | `records/kem-04/bag_piglet_512__enc.json` |
| `bag_piglet_512` | timing keygen | `records/kem-04/bag_piglet_512__keygen.json` |
| `bag_piglet_512` | hash profile dec | `profile/kem-04/bag_piglet_512__dec.json` |
| `bag_piglet_512` | hash profile enc | `profile/kem-04/bag_piglet_512__enc.json` |
| `bag_piglet_512` | hash profile keygen | `profile/kem-04/bag_piglet_512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

