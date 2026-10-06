# kem-09 CheetahKEM — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-09` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560844002873344.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: CheetahKEM
- Implementation versions measured: reference
- Parameter sets: `Cheetah128`, `Cheetah256`, `Cheetah384`, `Cheetah512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-09/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Cheetah128` | guide | PASS |
| `Cheetah256` | guide | PASS |
| `Cheetah384` | guide | PASS |
| `Cheetah512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Cheetah128` | keygen | 283.6 k | 135 µs | 7.38e+03 | 136 µs | 22335 (5 × 4467) |
| `Cheetah128` | enc | 367.7 k | 176 µs | 5.69e+03 | 176 µs | 27265 (5 × 5453) |
| `Cheetah128` | dec | 425.3 k | 203 µs | 4.92e+03 | 203 µs | 23000 (5 × 4600) |
| `Cheetah256` | keygen | 748.1 k | 357 µs | 2.8e+03 | 357 µs | 13295 (5 × 2659) |
| `Cheetah256` | enc | 859.1 k | 410 µs | 2.44e+03 | 410 µs | 12045 (5 × 2409) |
| `Cheetah256` | dec | 973.5 k | 465 µs | 2.15e+03 | 465 µs | 10615 (5 × 2123) |
| `Cheetah384` | keygen | 2.25 M | 1.08 ms | 930 | 1.08 ms | 4555 (5 × 911) |
| `Cheetah384` | enc | 2.39 M | 1.14 ms | 875 | 1.14 ms | 4315 (5 × 863) |
| `Cheetah384` | dec | 2.63 M | 1.26 ms | 797 | 1.26 ms | 3960 (5 × 792) |
| `Cheetah512` | keygen | 3.60 M | 1.72 ms | 582 | 1.72 ms | 2835 (5 × 567) |
| `Cheetah512` | enc | 3.77 M | 1.8 ms | 555 | 1.8 ms | 2775 (5 × 555) |
| `Cheetah512` | dec | 4.09 M | 1.96 ms | 511 | 1.96 ms | 2555 (5 × 511) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Cheetah128` | keygen | 38029 | 1712 KiB | 1788 KiB |
| `Cheetah128` | enc | 38029 | 1728 KiB | 1796 KiB |
| `Cheetah128` | dec | 38029 | 1728 KiB | 1792 KiB |
| `Cheetah256` | keygen | 38445 | 1716 KiB | 1816 KiB |
| `Cheetah256` | enc | 38445 | 1756 KiB | 1824 KiB |
| `Cheetah256` | dec | 38445 | 1764 KiB | 1832 KiB |
| `Cheetah384` | keygen | 39293 | 1724 KiB | 1880 KiB |
| `Cheetah384` | enc | 39293 | 1796 KiB | 1884 KiB |
| `Cheetah384` | dec | 39293 | 1800 KiB | 1912 KiB |
| `Cheetah512` | keygen | 39585 | 1732 KiB | 1908 KiB |
| `Cheetah512` | enc | 39585 | 1836 KiB | 1940 KiB |
| `Cheetah512` | dec | 39585 | 1776 KiB | 1912 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `Cheetah128` | 832 | 1936 | 864 | 16 |
| `Cheetah256` | 1648 | 3808 | 1728 | 32 |
| `Cheetah384` | 2704 | 5920 | 2832 | 48 |
| `Cheetah512` | 3600 | 7872 | 4032 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Cheetah128` | keygen | 64% | 4.3% | drng 2, pseudoXOF 3, sm3hash 1 |
| `Cheetah128` | enc | 61% | 1.3% | drng 1, pseudoXOF 5, sm3hash 1 |
| `Cheetah128` | dec | 54% | 0.0% | pseudoXOF 6 |
| `Cheetah256` | keygen | 68% | 2.3% | drng 2, pseudoXOF 3, sm3hash 1 |
| `Cheetah256` | enc | 65% | 0.6% | drng 1, pseudoXOF 5, sm3hash 1 |
| `Cheetah256` | dec | 57% | 0.0% | pseudoXOF 6 |
| `Cheetah384` | keygen | 82% | 0.8% | drng 2, pseudoXOF 3, sm3hash 1 |
| `Cheetah384` | enc | 79% | 0.3% | drng 1, pseudoXOF 5, sm3hash 1 |
| `Cheetah384` | dec | 74% | 0.0% | pseudoXOF 6 |
| `Cheetah512` | keygen | 83% | 0.6% | drng 2, pseudoXOF 3, sm3hash 1 |
| `Cheetah512` | enc | 81% | 0.2% | drng 1, pseudoXOF 5, sm3hash 1 |
| `Cheetah512` | dec | 76% | 0.0% | pseudoXOF 6 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Cheetah128` | KAT log (sha256 `958ef11f017bbe68…`) | `kat/kem-09/Cheetah128.log` |
| `Cheetah128` | timing dec | `records/kem-09/Cheetah128__dec.json` |
| `Cheetah128` | timing enc | `records/kem-09/Cheetah128__enc.json` |
| `Cheetah128` | timing keygen | `records/kem-09/Cheetah128__keygen.json` |
| `Cheetah128` | hash profile dec | `profile/kem-09/Cheetah128__dec.json` |
| `Cheetah128` | hash profile enc | `profile/kem-09/Cheetah128__enc.json` |
| `Cheetah128` | hash profile keygen | `profile/kem-09/Cheetah128__keygen.json` |
| `Cheetah256` | KAT log (sha256 `eb289ad531491e44…`) | `kat/kem-09/Cheetah256.log` |
| `Cheetah256` | timing dec | `records/kem-09/Cheetah256__dec.json` |
| `Cheetah256` | timing enc | `records/kem-09/Cheetah256__enc.json` |
| `Cheetah256` | timing keygen | `records/kem-09/Cheetah256__keygen.json` |
| `Cheetah256` | hash profile dec | `profile/kem-09/Cheetah256__dec.json` |
| `Cheetah256` | hash profile enc | `profile/kem-09/Cheetah256__enc.json` |
| `Cheetah256` | hash profile keygen | `profile/kem-09/Cheetah256__keygen.json` |
| `Cheetah384` | KAT log (sha256 `253b3a9cf85a3563…`) | `kat/kem-09/Cheetah384.log` |
| `Cheetah384` | timing dec | `records/kem-09/Cheetah384__dec.json` |
| `Cheetah384` | timing enc | `records/kem-09/Cheetah384__enc.json` |
| `Cheetah384` | timing keygen | `records/kem-09/Cheetah384__keygen.json` |
| `Cheetah384` | hash profile dec | `profile/kem-09/Cheetah384__dec.json` |
| `Cheetah384` | hash profile enc | `profile/kem-09/Cheetah384__enc.json` |
| `Cheetah384` | hash profile keygen | `profile/kem-09/Cheetah384__keygen.json` |
| `Cheetah512` | KAT log (sha256 `272ca8b8f7831b9b…`) | `kat/kem-09/Cheetah512.log` |
| `Cheetah512` | timing dec | `records/kem-09/Cheetah512__dec.json` |
| `Cheetah512` | timing enc | `records/kem-09/Cheetah512__enc.json` |
| `Cheetah512` | timing keygen | `records/kem-09/Cheetah512__keygen.json` |
| `Cheetah512` | hash profile dec | `profile/kem-09/Cheetah512__dec.json` |
| `Cheetah512` | hash profile enc | `profile/kem-09/Cheetah512__enc.json` |
| `Cheetah512` | hash profile keygen | `profile/kem-09/Cheetah512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

