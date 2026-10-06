# kem-30 PolarLAC — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-30` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560872180207616.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: PolarLAC
- Implementation versions measured: reference
- Parameter sets: `POLARLAC-128`, `POLARLAC-256`, `POLARLAC-512`, `POLARLAC-512-Star`, `POLARLAC-Light`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-30/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `POLARLAC-128` | guide | PASS |
| `POLARLAC-256` | guide | PASS |
| `POLARLAC-512` | guide | PASS |
| `POLARLAC-512-Star` | guide | PASS |
| `POLARLAC-Light` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `POLARLAC-128` | keygen | 160.5 k | 76.7 µs | 1.3e+04 | 76.7 µs | 56015 (5 × 11203) |
| `POLARLAC-128` | enc | 209.2 k | 99.9 µs | 1e+04 | 99.9 µs | 46970 (5 × 9394) |
| `POLARLAC-128` | dec | 252.7 k | 121 µs | 8.29e+03 | 121 µs | 39145 (5 × 7829) |
| `POLARLAC-256` | keygen | 285.9 k | 137 µs | 7.32e+03 | 137 µs | 33295 (5 × 6659) |
| `POLARLAC-256` | enc | 398.5 k | 190 µs | 5.25e+03 | 190 µs | 25965 (5 × 5193) |
| `POLARLAC-256` | dec | 496.0 k | 237 µs | 4.22e+03 | 237 µs | 20670 (5 × 4134) |
| `POLARLAC-512` | keygen | 980.5 k | 468 µs | 2.13e+03 | 469 µs | 10490 (5 × 2098) |
| `POLARLAC-512` | enc | 1.32 M | 632 µs | 1.58e+03 | 632 µs | 7955 (5 × 1591) |
| `POLARLAC-512` | dec | 1.60 M | 763 µs | 1.31e+03 | 763 µs | 6540 (5 × 1308) |
| `POLARLAC-512-Star` | keygen | 1.07 M | 511 µs | 1.96e+03 | 511 µs | 9555 (5 × 1911) |
| `POLARLAC-512-Star` | enc | 1.42 M | 678 µs | 1.48e+03 | 677 µs | 7035 (5 × 1407) |
| `POLARLAC-512-Star` | dec | 1.70 M | 813 µs | 1.23e+03 | 813 µs | 6265 (5 × 1253) |
| `POLARLAC-Light` | keygen | 149.0 k | 71.2 µs | 1.4e+04 | 71.2 µs | 59460 (5 × 11892) |
| `POLARLAC-Light` | enc | 195.5 k | 93.4 µs | 1.07e+04 | 93.4 µs | 49790 (5 × 9958) |
| `POLARLAC-Light` | dec | 237.9 k | 114 µs | 8.8e+03 | 114 µs | 41290 (5 × 8258) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `POLARLAC-128` | keygen | 61009 | 1740 KiB | 1840 KiB |
| `POLARLAC-128` | enc | 61009 | 1752 KiB | 1820 KiB |
| `POLARLAC-128` | dec | 61009 | 1744 KiB | 1808 KiB |
| `POLARLAC-256` | keygen | 69561 | 1748 KiB | 1828 KiB |
| `POLARLAC-256` | enc | 69561 | 1764 KiB | 1860 KiB |
| `POLARLAC-256` | dec | 69561 | 1768 KiB | 1832 KiB |
| `POLARLAC-512` | keygen | 83353 | 1760 KiB | 1896 KiB |
| `POLARLAC-512` | enc | 83353 | 1796 KiB | 1864 KiB |
| `POLARLAC-512` | dec | 83353 | 1776 KiB | 1904 KiB |
| `POLARLAC-512-Star` | keygen | 88513 | 1776 KiB | 1884 KiB |
| `POLARLAC-512-Star` | enc | 88513 | 1792 KiB | 1912 KiB |
| `POLARLAC-512-Star` | dec | 88513 | 1804 KiB | 1920 KiB |
| `POLARLAC-Light` | keygen | 61265 | 1736 KiB | 1816 KiB |
| `POLARLAC-Light` | enc | 61265 | 1700 KiB | 1828 KiB |
| `POLARLAC-Light` | dec | 61265 | 1712 KiB | 1836 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `POLARLAC-128` | 530 | 1570 | 640 | 16 |
| `POLARLAC-256` | 1060 | 3140 | 1280 | 32 |
| `POLARLAC-512` | 2116 | 6276 | 2560 | 64 |
| `POLARLAC-512-Star` | 2522 | 6682 | 2970 | 64 |
| `POLARLAC-Light` | 530 | 1570 | 608 | 16 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — default BIT_USE_SHAKE=0

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `POLARLAC-128` | keygen | 64% | 6.9% | drng 2, pseudoXOF 9.05 |
| `POLARLAC-128` | enc | 68% | 2.3% | drng 1, pseudoXOF 10 |
| `POLARLAC-128` | dec | 62% | 0.0% | pseudoXOF 11 |
| `POLARLAC-256` | keygen | 61% | 3.9% | drng 2, pseudoXOF 9.06 |
| `POLARLAC-256` | enc | 65% | 1.2% | drng 1, pseudoXOF 10 |
| `POLARLAC-256` | dec | 58% | 0.0% | pseudoXOF 11 |
| `POLARLAC-512` | keygen | 72% | 1.3% | drng 2, pseudoXOF 9.1 |
| `POLARLAC-512` | enc | 73% | 0.5% | drng 1, pseudoXOF 10.1 |
| `POLARLAC-512` | dec | 67% | 0.0% | pseudoXOF 11 |
| `POLARLAC-512-Star` | keygen | 65% | 1.2% | drng 2, pseudoXOF 13.1 |
| `POLARLAC-512-Star` | enc | 68% | 0.4% | drng 1, pseudoXOF 14.1 |
| `POLARLAC-512-Star` | dec | 64% | 0.0% | pseudoXOF 15 |
| `POLARLAC-Light` | keygen | 61% | 7.4% | drng 2, pseudoXOF 9.07 |
| `POLARLAC-Light` | enc | 66% | 2.5% | drng 1, pseudoXOF 10.1 |
| `POLARLAC-Light` | dec | 60% | 0.0% | pseudoXOF 11 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `POLARLAC-128` | KAT log (sha256 `e3254329a397c6e4…`) | `kat/kem-30/POLARLAC-128.log` |
| `POLARLAC-128` | timing dec | `records/kem-30/POLARLAC-128__dec.json` |
| `POLARLAC-128` | timing enc | `records/kem-30/POLARLAC-128__enc.json` |
| `POLARLAC-128` | timing keygen | `records/kem-30/POLARLAC-128__keygen.json` |
| `POLARLAC-128` | hash profile dec | `profile/kem-30/POLARLAC-128__dec.json` |
| `POLARLAC-128` | hash profile enc | `profile/kem-30/POLARLAC-128__enc.json` |
| `POLARLAC-128` | hash profile keygen | `profile/kem-30/POLARLAC-128__keygen.json` |
| `POLARLAC-256` | KAT log (sha256 `6c3c7fdbac0989e5…`) | `kat/kem-30/POLARLAC-256.log` |
| `POLARLAC-256` | timing dec | `records/kem-30/POLARLAC-256__dec.json` |
| `POLARLAC-256` | timing enc | `records/kem-30/POLARLAC-256__enc.json` |
| `POLARLAC-256` | timing keygen | `records/kem-30/POLARLAC-256__keygen.json` |
| `POLARLAC-256` | hash profile dec | `profile/kem-30/POLARLAC-256__dec.json` |
| `POLARLAC-256` | hash profile enc | `profile/kem-30/POLARLAC-256__enc.json` |
| `POLARLAC-256` | hash profile keygen | `profile/kem-30/POLARLAC-256__keygen.json` |
| `POLARLAC-512` | KAT log (sha256 `5012929efabba19a…`) | `kat/kem-30/POLARLAC-512.log` |
| `POLARLAC-512` | timing dec | `records/kem-30/POLARLAC-512__dec.json` |
| `POLARLAC-512` | timing enc | `records/kem-30/POLARLAC-512__enc.json` |
| `POLARLAC-512` | timing keygen | `records/kem-30/POLARLAC-512__keygen.json` |
| `POLARLAC-512` | hash profile dec | `profile/kem-30/POLARLAC-512__dec.json` |
| `POLARLAC-512` | hash profile enc | `profile/kem-30/POLARLAC-512__enc.json` |
| `POLARLAC-512` | hash profile keygen | `profile/kem-30/POLARLAC-512__keygen.json` |
| `POLARLAC-512-Star` | KAT log (sha256 `e960eef8180a2e43…`) | `kat/kem-30/POLARLAC-512-Star.log` |
| `POLARLAC-512-Star` | timing dec | `records/kem-30/POLARLAC-512-Star__dec.json` |
| `POLARLAC-512-Star` | timing enc | `records/kem-30/POLARLAC-512-Star__enc.json` |
| `POLARLAC-512-Star` | timing keygen | `records/kem-30/POLARLAC-512-Star__keygen.json` |
| `POLARLAC-512-Star` | hash profile dec | `profile/kem-30/POLARLAC-512-Star__dec.json` |
| `POLARLAC-512-Star` | hash profile enc | `profile/kem-30/POLARLAC-512-Star__enc.json` |
| `POLARLAC-512-Star` | hash profile keygen | `profile/kem-30/POLARLAC-512-Star__keygen.json` |
| `POLARLAC-Light` | KAT log (sha256 `530bcdd4951bf043…`) | `kat/kem-30/POLARLAC-Light.log` |
| `POLARLAC-Light` | timing dec | `records/kem-30/POLARLAC-Light__dec.json` |
| `POLARLAC-Light` | timing enc | `records/kem-30/POLARLAC-Light__enc.json` |
| `POLARLAC-Light` | timing keygen | `records/kem-30/POLARLAC-Light__keygen.json` |
| `POLARLAC-Light` | hash profile dec | `profile/kem-30/POLARLAC-Light__dec.json` |
| `POLARLAC-Light` | hash profile enc | `profile/kem-30/POLARLAC-Light__enc.json` |
| `POLARLAC-Light` | hash profile keygen | `profile/kem-30/POLARLAC-Light__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

