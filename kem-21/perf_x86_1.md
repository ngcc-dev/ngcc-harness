# kem-21 MAMBA-Viper — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-21` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560854052425728.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: MAMBA-Viper
- Implementation versions measured: reference
- Parameter sets: `MAMBA-Viper-128`, `MAMBA-Viper-192`, `MAMBA-Viper-256`, `MAMBA-Viper-384`, `MAMBA-Viper-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-21/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `MAMBA-Viper-128` | guide | PASS |
| `MAMBA-Viper-192` | guide | PASS |
| `MAMBA-Viper-256` | guide | PASS |
| `MAMBA-Viper-384` | guide | PASS |
| `MAMBA-Viper-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `MAMBA-Viper-128` | keygen | 225.5 k | 108 µs | 9.27e+03 | 108 µs | 41265 (5 × 8253) |
| `MAMBA-Viper-128` | enc | 349.4 k | 167 µs | 5.99e+03 | 167 µs | 28820 (5 × 5764) |
| `MAMBA-Viper-128` | dec | 422.5 k | 202 µs | 4.95e+03 | 202 µs | 23745 (5 × 4749) |
| `MAMBA-Viper-192` | keygen | 479.3 k | 229 µs | 4.36e+03 | 229 µs | 20505 (5 × 4101) |
| `MAMBA-Viper-192` | enc | 654.3 k | 313 µs | 3.2e+03 | 313 µs | 13615 (5 × 2723) |
| `MAMBA-Viper-192` | dec | 747.5 k | 358 µs | 2.8e+03 | 358 µs | 13695 (5 × 2739) |
| `MAMBA-Viper-256` | keygen | 798.2 k | 382 µs | 2.62e+03 | 382 µs | 12555 (5 × 2511) |
| `MAMBA-Viper-256` | enc | 1.01 M | 485 µs | 2.06e+03 | 485 µs | 10130 (5 × 2026) |
| `MAMBA-Viper-256` | dec | 1.12 M | 537 µs | 1.86e+03 | 537 µs | 9165 (5 × 1833) |
| `MAMBA-Viper-384` | keygen | 2.35 M | 1.12 ms | 891 | 1.12 ms | 4325 (5 × 865) |
| `MAMBA-Viper-384` | enc | 2.70 M | 1.29 ms | 774 | 1.29 ms | 3870 (5 × 774) |
| `MAMBA-Viper-384` | dec | 2.88 M | 1.38 ms | 727 | 1.38 ms | 3590 (5 × 718) |
| `MAMBA-Viper-512` | keygen | 3.79 M | 1.81 ms | 552 | 1.81 ms | 2705 (5 × 541) |
| `MAMBA-Viper-512` | enc | 4.24 M | 2.03 ms | 494 | 2.03 ms | 2460 (5 × 492) |
| `MAMBA-Viper-512` | dec | 4.45 M | 2.13 ms | 470 | 2.13 ms | 2290 (5 × 458) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `MAMBA-Viper-128` | keygen | 39025 | 1700 KiB | 1796 KiB |
| `MAMBA-Viper-128` | enc | 39025 | 1732 KiB | 1796 KiB |
| `MAMBA-Viper-128` | dec | 39025 | 1732 KiB | 1796 KiB |
| `MAMBA-Viper-192` | keygen | 38849 | 1700 KiB | 1780 KiB |
| `MAMBA-Viper-192` | enc | 38849 | 1736 KiB | 1804 KiB |
| `MAMBA-Viper-192` | dec | 38849 | 1740 KiB | 1808 KiB |
| `MAMBA-Viper-256` | keygen | 39025 | 1704 KiB | 1816 KiB |
| `MAMBA-Viper-256` | enc | 39025 | 1736 KiB | 1804 KiB |
| `MAMBA-Viper-256` | dec | 39025 | 1744 KiB | 1836 KiB |
| `MAMBA-Viper-384` | keygen | 39297 | 1708 KiB | 1832 KiB |
| `MAMBA-Viper-384` | enc | 39297 | 1768 KiB | 1888 KiB |
| `MAMBA-Viper-384` | dec | 39297 | 1784 KiB | 1884 KiB |
| `MAMBA-Viper-512` | keygen | 39425 | 1716 KiB | 1900 KiB |
| `MAMBA-Viper-512` | enc | 39425 | 1800 KiB | 1924 KiB |
| `MAMBA-Viper-512` | dec | 39425 | 1812 KiB | 1884 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `MAMBA-Viper-128` | 608 | 1424 | 736 | 16 |
| `MAMBA-Viper-192` | 992 | 2200 | 1088 | 24 |
| `MAMBA-Viper-256` | 1312 | 2912 | 1472 | 32 |
| `MAMBA-Viper-384` | 2496 | 5488 | 2656 | 48 |
| `MAMBA-Viper-512` | 3200 | 7040 | 3456 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — `shake*` names are shims over pseudoXOF

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `MAMBA-Viper-128` | keygen | 46% | 6.1% | drng 3, pseudoXOF 8 |
| `MAMBA-Viper-128` | enc | 48% | 1.3% | drng 1, pseudoXOF 14 |
| `MAMBA-Viper-128` | dec | 46% | 0.0% | pseudoXOF 12 |
| `MAMBA-Viper-192` | keygen | 48% | 2.9% | drng 3, pseudoXOF 14 |
| `MAMBA-Viper-192` | enc | 47% | 0.7% | drng 1, pseudoXOF 20 |
| `MAMBA-Viper-192` | dec | 43% | 0.0% | pseudoXOF 18 |
| `MAMBA-Viper-256` | keygen | 47% | 1.7% | drng 3, pseudoXOF 22 |
| `MAMBA-Viper-256` | enc | 45% | 0.5% | drng 1, pseudoXOF 28 |
| `MAMBA-Viper-256` | dec | 42% | 0.0% | pseudoXOF 26 |
| `MAMBA-Viper-384` | keygen | 47% | 0.6% | drng 3, pseudoXOF 58 |
| `MAMBA-Viper-384` | enc | 46% | 0.2% | drng 1, pseudoXOF 64 |
| `MAMBA-Viper-384` | dec | 43% | 0.0% | pseudoXOF 62 |
| `MAMBA-Viper-512` | keygen | 46% | 0.4% | drng 3, pseudoXOF 92 |
| `MAMBA-Viper-512` | enc | 45% | 0.1% | drng 1, pseudoXOF 98 |
| `MAMBA-Viper-512` | dec | 43% | 0.0% | pseudoXOF 96 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `MAMBA-Viper-128` | KAT log (sha256 `c929a08da4556dde…`) | `kat/kem-21/MAMBA-Viper-128.log` |
| `MAMBA-Viper-128` | timing dec | `records/kem-21/MAMBA-Viper-128__dec.json` |
| `MAMBA-Viper-128` | timing enc | `records/kem-21/MAMBA-Viper-128__enc.json` |
| `MAMBA-Viper-128` | timing keygen | `records/kem-21/MAMBA-Viper-128__keygen.json` |
| `MAMBA-Viper-128` | hash profile dec | `profile/kem-21/MAMBA-Viper-128__dec.json` |
| `MAMBA-Viper-128` | hash profile enc | `profile/kem-21/MAMBA-Viper-128__enc.json` |
| `MAMBA-Viper-128` | hash profile keygen | `profile/kem-21/MAMBA-Viper-128__keygen.json` |
| `MAMBA-Viper-192` | KAT log (sha256 `7989ee0a5a1d29d4…`) | `kat/kem-21/MAMBA-Viper-192.log` |
| `MAMBA-Viper-192` | timing dec | `records/kem-21/MAMBA-Viper-192__dec.json` |
| `MAMBA-Viper-192` | timing enc | `records/kem-21/MAMBA-Viper-192__enc.json` |
| `MAMBA-Viper-192` | timing keygen | `records/kem-21/MAMBA-Viper-192__keygen.json` |
| `MAMBA-Viper-192` | hash profile dec | `profile/kem-21/MAMBA-Viper-192__dec.json` |
| `MAMBA-Viper-192` | hash profile enc | `profile/kem-21/MAMBA-Viper-192__enc.json` |
| `MAMBA-Viper-192` | hash profile keygen | `profile/kem-21/MAMBA-Viper-192__keygen.json` |
| `MAMBA-Viper-256` | KAT log (sha256 `4973cbbfe3b158d6…`) | `kat/kem-21/MAMBA-Viper-256.log` |
| `MAMBA-Viper-256` | timing dec | `records/kem-21/MAMBA-Viper-256__dec.json` |
| `MAMBA-Viper-256` | timing enc | `records/kem-21/MAMBA-Viper-256__enc.json` |
| `MAMBA-Viper-256` | timing keygen | `records/kem-21/MAMBA-Viper-256__keygen.json` |
| `MAMBA-Viper-256` | hash profile dec | `profile/kem-21/MAMBA-Viper-256__dec.json` |
| `MAMBA-Viper-256` | hash profile enc | `profile/kem-21/MAMBA-Viper-256__enc.json` |
| `MAMBA-Viper-256` | hash profile keygen | `profile/kem-21/MAMBA-Viper-256__keygen.json` |
| `MAMBA-Viper-384` | KAT log (sha256 `19a9309b0ce2dbaa…`) | `kat/kem-21/MAMBA-Viper-384.log` |
| `MAMBA-Viper-384` | timing dec | `records/kem-21/MAMBA-Viper-384__dec.json` |
| `MAMBA-Viper-384` | timing enc | `records/kem-21/MAMBA-Viper-384__enc.json` |
| `MAMBA-Viper-384` | timing keygen | `records/kem-21/MAMBA-Viper-384__keygen.json` |
| `MAMBA-Viper-384` | hash profile dec | `profile/kem-21/MAMBA-Viper-384__dec.json` |
| `MAMBA-Viper-384` | hash profile enc | `profile/kem-21/MAMBA-Viper-384__enc.json` |
| `MAMBA-Viper-384` | hash profile keygen | `profile/kem-21/MAMBA-Viper-384__keygen.json` |
| `MAMBA-Viper-512` | KAT log (sha256 `285cfc3817b9d9a8…`) | `kat/kem-21/MAMBA-Viper-512.log` |
| `MAMBA-Viper-512` | timing dec | `records/kem-21/MAMBA-Viper-512__dec.json` |
| `MAMBA-Viper-512` | timing enc | `records/kem-21/MAMBA-Viper-512__enc.json` |
| `MAMBA-Viper-512` | timing keygen | `records/kem-21/MAMBA-Viper-512__keygen.json` |
| `MAMBA-Viper-512` | hash profile dec | `profile/kem-21/MAMBA-Viper-512__dec.json` |
| `MAMBA-Viper-512` | hash profile enc | `profile/kem-21/MAMBA-Viper-512__enc.json` |
| `MAMBA-Viper-512` | hash profile keygen | `profile/kem-21/MAMBA-Viper-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

