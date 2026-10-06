# sign-20 Qing Luan — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-20` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561087243145216.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: Qing Luan
- Implementation versions measured: reference
- Parameter sets: `QingLuan-128`, `QingLuan-256`, `QingLuan-384`, `QingLuan-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-20/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `QingLuan-128` | guide | PASS |
| `QingLuan-256` | guide | PASS |
| `QingLuan-384` | guide | PASS |
| `QingLuan-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `QingLuan-128` | keygen | 272.2 k | 130 µs | 7.69e+03 | 130 µs | 35075 (5 × 7015) |
| `QingLuan-128` | sign | 16.41 M | 7.84 ms | 128 | 7.84 ms | 635 (5 × 127) |
| `QingLuan-128` | verify | 7.15 M | 3.42 ms | 293 | 3.42 ms | 1450 (5 × 290) |
| `QingLuan-256` | keygen | 1.67 M | 796 µs | 1.26e+03 | 796 µs | 5750 (5 × 1150) |
| `QingLuan-256` | sign | 103.95 M | 49.8 ms | 20.1 | 49.8 ms | 105 (5 × 21) |
| `QingLuan-256` | verify | 45.53 M | 21.8 ms | 46 | 21.8 ms | 230 (5 × 46) |
| `QingLuan-384` | keygen | 3.58 M | 1.71 ms | 585 | 1.71 ms | 2885 (5 × 577) |
| `QingLuan-384` | sign | 257.81 M | 123 ms | 8.1 | 123 ms | 100 (5 × 20) |
| `QingLuan-384` | verify | 115.85 M | 55.5 ms | 18 | 55.5 ms | 100 (5 × 20) |
| `QingLuan-512` | keygen | 8.80 M | 4.2 ms | 238 | 4.2 ms | 1170 (5 × 234) |
| `QingLuan-512` | sign | 612.04 M | 294 ms | 3.4 | 293 ms | 100 (5 × 20) |
| `QingLuan-512` | verify | 274.42 M | 131 ms | 7.61 | 131 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `QingLuan-128` | keygen | 34189 | 1712 KiB | 1824 KiB |
| `QingLuan-128` | sign | 34189 | 1724 KiB | 1940 KiB |
| `QingLuan-128` | verify | 34189 | 1832 KiB | 1928 KiB |
| `QingLuan-256` | keygen | 34789 | 1780 KiB | 1860 KiB |
| `QingLuan-256` | sign | 34789 | 1796 KiB | 2436 KiB |
| `QingLuan-256` | verify | 34789 | 2228 KiB | 2328 KiB |
| `QingLuan-384` | keygen | 34853 | 1684 KiB | 1824 KiB |
| `QingLuan-384` | sign | 34853 | 1732 KiB | 3184 KiB |
| `QingLuan-384` | verify | 34853 | 2308 KiB | 2920 KiB |
| `QingLuan-512` | keygen | 35285 | 1720 KiB | 1844 KiB |
| `QingLuan-512` | sign | 35285 | 1752 KiB | 4348 KiB |
| `QingLuan-512` | verify | 35285 | 2804 KiB | 3852 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `QingLuan-128` | 77 | 32 | 18720 |
| `QingLuan-256` | 153 | 64 | 74248 |
| `QingLuan-384` | 227 | 96 | 164940 |
| `QingLuan-512` | 302 | 128 | 292816 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — builds wide hash and counter XOF on sm3hash, not pseudohash/pseudoXOF

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `QingLuan-128` | keygen | 69% | 1.6% | drng 1, sm3hash 133 |
| `QingLuan-128` | sign | 62% | 0.1% | drng 2, sm3hash 5.93e+03 |
| `QingLuan-128` | verify | 66% | 0.0% | sm3hash 2.54e+03 |
| `QingLuan-256` | keygen | 82% | 0.4% | drng 1, sm3hash 497 |
| `QingLuan-256` | sign | 72% | 0.0% | drng 2, sm3hash 2.28e+04 |
| `QingLuan-256` | verify | 77% | 0.0% | sm3hash 9.73e+03 |
| `QingLuan-384` | keygen | 82% | 0.2% | drng 1, sm3hash 1.07e+03 |
| `QingLuan-384` | sign | 72% | 0.0% | drng 2, sm3hash 5.01e+04 |
| `QingLuan-384` | verify | 79% | 0.0% | sm3hash 2.13e+04 |
| `QingLuan-512` | keygen | 88% | 0.1% | drng 1, sm3hash 1.87e+03 |
| `QingLuan-512` | sign | 77% | 0.0% | drng 2, sm3hash 8.85e+04 |
| `QingLuan-512` | verify | 83% | 0.0% | sm3hash 3.76e+04 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `QingLuan-128` | KAT log (sha256 `126aa127d1420623…`) | `kat/sign-20/QingLuan-128.log` |
| `QingLuan-128` | timing keygen | `records/sign-20/QingLuan-128__keygen.json` |
| `QingLuan-128` | timing sign | `records/sign-20/QingLuan-128__sign.json` |
| `QingLuan-128` | timing verify | `records/sign-20/QingLuan-128__verify.json` |
| `QingLuan-128` | hash profile keygen | `profile/sign-20/QingLuan-128__keygen.json` |
| `QingLuan-128` | hash profile sign | `profile/sign-20/QingLuan-128__sign.json` |
| `QingLuan-128` | hash profile verify | `profile/sign-20/QingLuan-128__verify.json` |
| `QingLuan-256` | KAT log (sha256 `654d932cf1d9afd7…`) | `kat/sign-20/QingLuan-256.log` |
| `QingLuan-256` | timing keygen | `records/sign-20/QingLuan-256__keygen.json` |
| `QingLuan-256` | timing sign | `records/sign-20/QingLuan-256__sign.json` |
| `QingLuan-256` | timing verify | `records/sign-20/QingLuan-256__verify.json` |
| `QingLuan-256` | hash profile keygen | `profile/sign-20/QingLuan-256__keygen.json` |
| `QingLuan-256` | hash profile sign | `profile/sign-20/QingLuan-256__sign.json` |
| `QingLuan-256` | hash profile verify | `profile/sign-20/QingLuan-256__verify.json` |
| `QingLuan-384` | KAT log (sha256 `b6444860391972e7…`) | `kat/sign-20/QingLuan-384.log` |
| `QingLuan-384` | timing keygen | `records/sign-20/QingLuan-384__keygen.json` |
| `QingLuan-384` | timing sign | `records/sign-20/QingLuan-384__sign.json` |
| `QingLuan-384` | timing verify | `records/sign-20/QingLuan-384__verify.json` |
| `QingLuan-384` | hash profile keygen | `profile/sign-20/QingLuan-384__keygen.json` |
| `QingLuan-384` | hash profile sign | `profile/sign-20/QingLuan-384__sign.json` |
| `QingLuan-384` | hash profile verify | `profile/sign-20/QingLuan-384__verify.json` |
| `QingLuan-512` | KAT log (sha256 `bc89409ddc7e6a7a…`) | `kat/sign-20/QingLuan-512.log` |
| `QingLuan-512` | timing keygen | `records/sign-20/QingLuan-512__keygen.json` |
| `QingLuan-512` | timing sign | `records/sign-20/QingLuan-512__sign.json` |
| `QingLuan-512` | timing verify | `records/sign-20/QingLuan-512__verify.json` |
| `QingLuan-512` | hash profile keygen | `profile/sign-20/QingLuan-512__keygen.json` |
| `QingLuan-512` | hash profile sign | `profile/sign-20/QingLuan-512__sign.json` |
| `QingLuan-512` | hash profile verify | `profile/sign-20/QingLuan-512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

