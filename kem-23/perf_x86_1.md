# kem-23 Mito — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-23` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560862776578048.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: Mito
- Implementation versions measured: reference
- Parameter sets: `Mito-1-128`, `Mito-1-256`, `Mito-1-512`, `Mito-1-E-128`, `Mito-1-E-256`, `Mito-1-E-512`, `Mito-2-E-128`, `Mito-2-E-256`, `Mito-2-E-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-23/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Mito-1-128` | guide | PASS |
| `Mito-1-256` | guide | PASS |
| `Mito-1-512` | guide | PASS |
| `Mito-1-E-128` | guide | PASS |
| `Mito-1-E-256` | guide | PASS |
| `Mito-1-E-512` | guide | PASS |
| `Mito-2-E-128` | guide | PASS |
| `Mito-2-E-256` | guide | PASS |
| `Mito-2-E-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Mito-1-128` | keygen | 12.63 M | 6.03 ms | 166 | 6.03 ms | 825 (5 × 165) |
| `Mito-1-128` | enc | 20.92 M | 10 ms | 100 | 10 ms | 505 (5 × 101) |
| `Mito-1-128` | dec | 29.58 M | 14.1 ms | 70.8 | 14.1 ms | 355 (5 × 71) |
| `Mito-1-256` | keygen | 38.14 M | 18.4 ms | 54.3 | 18.2 ms | 275 (5 × 55) |
| `Mito-1-256` | enc | 63.41 M | 30.3 ms | 33 | 30.3 ms | 165 (5 × 33) |
| `Mito-1-256` | dec | 89.90 M | 42.9 ms | 23.3 | 42.9 ms | 120 (5 × 24) |
| `Mito-1-512` | keygen | 234.79 M | 112 ms | 8.91 | 112 ms | 100 (5 × 20) |
| `Mito-1-512` | enc | 391.13 M | 187 ms | 5.35 | 187 ms | 100 (5 × 20) |
| `Mito-1-512` | dec | 550.81 M | 265 ms | 3.78 | 263 ms | 100 (5 × 20) |
| `Mito-1-E-128` | keygen | 11.69 M | 5.58 ms | 179 | 5.58 ms | 890 (5 × 178) |
| `Mito-1-E-128` | enc | 19.36 M | 9.25 ms | 108 | 9.25 ms | 540 (5 × 108) |
| `Mito-1-E-128` | dec | 27.35 M | 13.1 ms | 76.5 | 13.1 ms | 380 (5 × 76) |
| `Mito-1-E-256` | keygen | 39.69 M | 19 ms | 52.7 | 19 ms | 265 (5 × 53) |
| `Mito-1-E-256` | enc | 65.98 M | 31.5 ms | 31.7 | 31.5 ms | 160 (5 × 32) |
| `Mito-1-E-256` | dec | 93.36 M | 44.6 ms | 22.4 | 44.6 ms | 115 (5 × 23) |
| `Mito-1-E-512` | keygen | 224.83 M | 107 ms | 9.31 | 107 ms | 100 (5 × 20) |
| `Mito-1-E-512` | enc | 374.52 M | 180 ms | 5.55 | 179 ms | 100 (5 × 20) |
| `Mito-1-E-512` | dec | 527.42 M | 253 ms | 3.95 | 252 ms | 100 (5 × 20) |
| `Mito-2-E-128` | keygen | 17.27 M | 8.25 ms | 121 | 8.25 ms | 605 (5 × 121) |
| `Mito-2-E-128` | enc | 24.84 M | 11.9 ms | 84.3 | 11.9 ms | 425 (5 × 85) |
| `Mito-2-E-128` | dec | 32.99 M | 15.8 ms | 63.5 | 15.8 ms | 320 (5 × 64) |
| `Mito-2-E-256` | keygen | 55.07 M | 26.3 ms | 38 | 26.3 ms | 190 (5 × 38) |
| `Mito-2-E-256` | enc | 79.54 M | 38 ms | 26.3 | 38 ms | 135 (5 × 27) |
| `Mito-2-E-256` | dec | 105.35 M | 50.3 ms | 19.9 | 50.3 ms | 100 (5 × 20) |
| `Mito-2-E-512` | keygen | 346.96 M | 167 ms | 5.98 | 166 ms | 100 (5 × 20) |
| `Mito-2-E-512` | enc | 501.43 M | 240 ms | 4.17 | 240 ms | 100 (5 × 20) |
| `Mito-2-E-512` | dec | 660.21 M | 317 ms | 3.16 | 315 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Mito-1-128` | keygen | 55177 | 1724 KiB | 1888 KiB |
| `Mito-1-128` | enc | 55177 | 1816 KiB | 1888 KiB |
| `Mito-1-128` | dec | 55177 | 1800 KiB | 1924 KiB |
| `Mito-1-256` | keygen | 56001 | 1744 KiB | 1992 KiB |
| `Mito-1-256` | enc | 56001 | 1924 KiB | 2016 KiB |
| `Mito-1-256` | dec | 56001 | 1936 KiB | 2008 KiB |
| `Mito-1-512` | keygen | 56361 | 1808 KiB | 2252 KiB |
| `Mito-1-512` | enc | 56361 | 2268 KiB | 2408 KiB |
| `Mito-1-512` | dec | 56361 | 2432 KiB | 2524 KiB |
| `Mito-1-E-128` | keygen | 55537 | 1724 KiB | 1888 KiB |
| `Mito-1-E-128` | enc | 55537 | 1776 KiB | 1904 KiB |
| `Mito-1-E-128` | dec | 55537 | 1776 KiB | 1908 KiB |
| `Mito-1-E-256` | keygen | 56441 | 1752 KiB | 1980 KiB |
| `Mito-1-E-256` | enc | 56441 | 1900 KiB | 2000 KiB |
| `Mito-1-E-256` | dec | 56441 | 1932 KiB | 2000 KiB |
| `Mito-1-E-512` | keygen | 57289 | 1820 KiB | 2252 KiB |
| `Mito-1-E-512` | enc | 57289 | 2292 KiB | 2376 KiB |
| `Mito-1-E-512` | dec | 57289 | 2388 KiB | 2460 KiB |
| `Mito-2-E-128` | keygen | 55289 | 1744 KiB | 1872 KiB |
| `Mito-2-E-128` | enc | 55289 | 1796 KiB | 1896 KiB |
| `Mito-2-E-128` | dec | 55289 | 1804 KiB | 1916 KiB |
| `Mito-2-E-256` | keygen | 56073 | 1744 KiB | 1936 KiB |
| `Mito-2-E-256` | enc | 56073 | 1924 KiB | 2020 KiB |
| `Mito-2-E-256` | dec | 56073 | 1924 KiB | 2056 KiB |
| `Mito-2-E-512` | keygen | 56881 | 1828 KiB | 2308 KiB |
| `Mito-2-E-512` | enc | 56881 | 2328 KiB | 2408 KiB |
| `Mito-2-E-512` | dec | 56881 | 2396 KiB | 2464 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `Mito-1-128` | 3908 | 4052 | 5796 | 64 |
| `Mito-1-256` | 8522 | 8682 | 12714 | 64 |
| `Mito-1-512` | 26630 | 26822 | 39878 | 64 |
| `Mito-1-E-128` | 3720 | 3864 | 5512 | 64 |
| `Mito-1-E-256` | 8130 | 8290 | 12130 | 64 |
| `Mito-1-E-512` | 25674 | 25866 | 38442 | 64 |
| `Mito-2-E-128` | 5192 | 5336 | 6440 | 64 |
| `Mito-2-E-256` | 10828 | 10988 | 13484 | 64 |
| `Mito-2-E-512` | 32712 | 32904 | 40840 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Mito-1-128` | keygen | 0.3% | 7.8% | drng 177, pseudohash 1 |
| `Mito-1-128` | enc | 1.0% | 6.6% | drng 266, pseudohash 2 |
| `Mito-1-128` | dec | 1.6% | 6.0% | drng 351, pseudohash 3 |
| `Mito-1-256` | keygen | 0.1% | 4.3% | drng 275, pseudohash 1 |
| `Mito-1-256` | enc | 0.6% | 3.6% | drng 413, pseudohash 2 |
| `Mito-1-256` | dec | 1.1% | 3.2% | drng 552, pseudohash 3 |
| `Mito-1-512` | keygen | 0.0% | 1.6% | drng 536, pseudohash 1 |
| `Mito-1-512` | enc | 0.3% | 1.2% | drng 803, pseudohash 2 |
| `Mito-1-512` | dec | 0.5% | 1.1% | drng 1.08e+03, pseudohash 3 |
| `Mito-1-E-128` | keygen | 0.3% | 8.3% | drng 178, pseudohash 1 |
| `Mito-1-E-128` | enc | 1.0% | 7.1% | drng 267, pseudohash 2 |
| `Mito-1-E-128` | dec | 1.6% | 6.4% | drng 351, pseudohash 3 |
| `Mito-1-E-256` | keygen | 0.1% | 4.1% | drng 275, pseudohash 1 |
| `Mito-1-E-256` | enc | 0.6% | 3.4% | drng 411, pseudohash 2 |
| `Mito-1-E-256` | dec | 1.0% | 3.0% | drng 545, pseudohash 3 |
| `Mito-1-E-512` | keygen | 0.0% | 1.6% | drng 536, pseudohash 1 |
| `Mito-1-E-512` | enc | 0.3% | 1.3% | drng 801, pseudohash 2 |
| `Mito-1-E-512` | dec | 0.5% | 1.2% | drng 1.08e+03, pseudohash 3 |
| `Mito-2-E-128` | keygen | 0.2% | 6.4% | drng 194, pseudohash 1 |
| `Mito-2-E-128` | enc | 1.1% | 5.2% | drng 236, pseudohash 2 |
| `Mito-2-E-128` | dec | 1.7% | 5.2% | drng 327, pseudohash 3 |
| `Mito-2-E-256` | keygen | 0.1% | 3.3% | drng 292, pseudohash 1 |
| `Mito-2-E-256` | enc | 0.6% | 2.7% | drng 363, pseudohash 2 |
| `Mito-2-E-256` | dec | 1.0% | 2.6% | drng 504, pseudohash 3 |
| `Mito-2-E-512` | keygen | 0.0% | 1.2% | drng 555, pseudohash 1 |
| `Mito-2-E-512` | enc | 0.3% | 0.9% | drng 693, pseudohash 2 |
| `Mito-2-E-512` | dec | 0.5% | 0.9% | drng 964, pseudohash 3 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Mito-1-128` | KAT log (sha256 `48b126c7832d690a…`) | `kat/kem-23/Mito-1-128.log` |
| `Mito-1-128` | timing dec | `records/kem-23/Mito-1-128__dec.json` |
| `Mito-1-128` | timing enc | `records/kem-23/Mito-1-128__enc.json` |
| `Mito-1-128` | timing keygen | `records/kem-23/Mito-1-128__keygen.json` |
| `Mito-1-128` | hash profile dec | `profile/kem-23/Mito-1-128__dec.json` |
| `Mito-1-128` | hash profile enc | `profile/kem-23/Mito-1-128__enc.json` |
| `Mito-1-128` | hash profile keygen | `profile/kem-23/Mito-1-128__keygen.json` |
| `Mito-1-256` | KAT log (sha256 `bcde43e95d77dff2…`) | `kat/kem-23/Mito-1-256.log` |
| `Mito-1-256` | timing dec | `records/kem-23/Mito-1-256__dec.json` |
| `Mito-1-256` | timing enc | `records/kem-23/Mito-1-256__enc.json` |
| `Mito-1-256` | timing keygen | `records/kem-23/Mito-1-256__keygen.json` |
| `Mito-1-256` | hash profile dec | `profile/kem-23/Mito-1-256__dec.json` |
| `Mito-1-256` | hash profile enc | `profile/kem-23/Mito-1-256__enc.json` |
| `Mito-1-256` | hash profile keygen | `profile/kem-23/Mito-1-256__keygen.json` |
| `Mito-1-512` | KAT log (sha256 `4aae9274e08975a2…`) | `kat/kem-23/Mito-1-512.log` |
| `Mito-1-512` | timing dec | `records/kem-23/Mito-1-512__dec.json` |
| `Mito-1-512` | timing enc | `records/kem-23/Mito-1-512__enc.json` |
| `Mito-1-512` | timing keygen | `records/kem-23/Mito-1-512__keygen.json` |
| `Mito-1-512` | hash profile dec | `profile/kem-23/Mito-1-512__dec.json` |
| `Mito-1-512` | hash profile enc | `profile/kem-23/Mito-1-512__enc.json` |
| `Mito-1-512` | hash profile keygen | `profile/kem-23/Mito-1-512__keygen.json` |
| `Mito-1-E-128` | KAT log (sha256 `cf86a7383f9f5906…`) | `kat/kem-23/Mito-1-E-128.log` |
| `Mito-1-E-128` | timing dec | `records/kem-23/Mito-1-E-128__dec.json` |
| `Mito-1-E-128` | timing enc | `records/kem-23/Mito-1-E-128__enc.json` |
| `Mito-1-E-128` | timing keygen | `records/kem-23/Mito-1-E-128__keygen.json` |
| `Mito-1-E-128` | hash profile dec | `profile/kem-23/Mito-1-E-128__dec.json` |
| `Mito-1-E-128` | hash profile enc | `profile/kem-23/Mito-1-E-128__enc.json` |
| `Mito-1-E-128` | hash profile keygen | `profile/kem-23/Mito-1-E-128__keygen.json` |
| `Mito-1-E-256` | KAT log (sha256 `f64c50c13782ab87…`) | `kat/kem-23/Mito-1-E-256.log` |
| `Mito-1-E-256` | timing dec | `records/kem-23/Mito-1-E-256__dec.json` |
| `Mito-1-E-256` | timing enc | `records/kem-23/Mito-1-E-256__enc.json` |
| `Mito-1-E-256` | timing keygen | `records/kem-23/Mito-1-E-256__keygen.json` |
| `Mito-1-E-256` | hash profile dec | `profile/kem-23/Mito-1-E-256__dec.json` |
| `Mito-1-E-256` | hash profile enc | `profile/kem-23/Mito-1-E-256__enc.json` |
| `Mito-1-E-256` | hash profile keygen | `profile/kem-23/Mito-1-E-256__keygen.json` |
| `Mito-1-E-512` | KAT log (sha256 `2ed32a87f5edc9dc…`) | `kat/kem-23/Mito-1-E-512.log` |
| `Mito-1-E-512` | timing dec | `records/kem-23/Mito-1-E-512__dec.json` |
| `Mito-1-E-512` | timing enc | `records/kem-23/Mito-1-E-512__enc.json` |
| `Mito-1-E-512` | timing keygen | `records/kem-23/Mito-1-E-512__keygen.json` |
| `Mito-1-E-512` | hash profile dec | `profile/kem-23/Mito-1-E-512__dec.json` |
| `Mito-1-E-512` | hash profile enc | `profile/kem-23/Mito-1-E-512__enc.json` |
| `Mito-1-E-512` | hash profile keygen | `profile/kem-23/Mito-1-E-512__keygen.json` |
| `Mito-2-E-128` | KAT log (sha256 `dc8fde4fdd876d2c…`) | `kat/kem-23/Mito-2-E-128.log` |
| `Mito-2-E-128` | timing dec | `records/kem-23/Mito-2-E-128__dec.json` |
| `Mito-2-E-128` | timing enc | `records/kem-23/Mito-2-E-128__enc.json` |
| `Mito-2-E-128` | timing keygen | `records/kem-23/Mito-2-E-128__keygen.json` |
| `Mito-2-E-128` | hash profile dec | `profile/kem-23/Mito-2-E-128__dec.json` |
| `Mito-2-E-128` | hash profile enc | `profile/kem-23/Mito-2-E-128__enc.json` |
| `Mito-2-E-128` | hash profile keygen | `profile/kem-23/Mito-2-E-128__keygen.json` |
| `Mito-2-E-256` | KAT log (sha256 `acd8cf343c1e580d…`) | `kat/kem-23/Mito-2-E-256.log` |
| `Mito-2-E-256` | timing dec | `records/kem-23/Mito-2-E-256__dec.json` |
| `Mito-2-E-256` | timing enc | `records/kem-23/Mito-2-E-256__enc.json` |
| `Mito-2-E-256` | timing keygen | `records/kem-23/Mito-2-E-256__keygen.json` |
| `Mito-2-E-256` | hash profile dec | `profile/kem-23/Mito-2-E-256__dec.json` |
| `Mito-2-E-256` | hash profile enc | `profile/kem-23/Mito-2-E-256__enc.json` |
| `Mito-2-E-256` | hash profile keygen | `profile/kem-23/Mito-2-E-256__keygen.json` |
| `Mito-2-E-512` | KAT log (sha256 `121de948af972264…`) | `kat/kem-23/Mito-2-E-512.log` |
| `Mito-2-E-512` | timing dec | `records/kem-23/Mito-2-E-512__dec.json` |
| `Mito-2-E-512` | timing enc | `records/kem-23/Mito-2-E-512__enc.json` |
| `Mito-2-E-512` | timing keygen | `records/kem-23/Mito-2-E-512__keygen.json` |
| `Mito-2-E-512` | hash profile dec | `profile/kem-23/Mito-2-E-512__dec.json` |
| `Mito-2-E-512` | hash profile enc | `profile/kem-23/Mito-2-E-512__enc.json` |
| `Mito-2-E-512` | hash profile keygen | `profile/kem-23/Mito-2-E-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

