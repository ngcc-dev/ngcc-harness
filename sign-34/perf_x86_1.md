# sign-34 YuanYang.DSA — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-34` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561114459983872.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: YuanYang.DSA
- Implementation versions measured: reference
- Parameter sets: `yuanyang-512`, `yuanyang-1024`, `yuanyang-2048`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-34/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `yuanyang-512` | guide | PASS |
| `yuanyang-1024` | guide | PASS |
| `yuanyang-2048` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `yuanyang-512` | keygen | 17.05 M | 8.16 ms | 122 | 8.16 ms | 890 (5 × 178) |
| `yuanyang-512` | sign | 9.00 M | 4.3 ms | 233 | 4.3 ms | 880 (5 × 176) |
| `yuanyang-512` | verify | 210.7 k | 101 µs | 9.93e+03 | 101 µs | 41315 (5 × 8263) |
| `yuanyang-1024` | keygen | 19.76 M | 9.46 ms | 106 | 9.46 ms | 530 (5 × 106) |
| `yuanyang-1024` | sign | 14.65 M | 7 ms | 143 | 7 ms | 755 (5 × 151) |
| `yuanyang-1024` | verify | 461.1 k | 220 µs | 4.54e+03 | 220 µs | 19735 (5 × 3947) |
| `yuanyang-2048` | keygen | 88.03 M | 42.6 ms | 23.5 | 42.6 ms | 130 (5 × 26) |
| `yuanyang-2048` | sign | 46.36 M | 22.2 ms | 45.1 | 22.2 ms | 145 (5 × 29) |
| `yuanyang-2048` | verify | 1.02 M | 487 µs | 2.05e+03 | 487 µs | 9460 (5 × 1892) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `yuanyang-512` | keygen | 170949 | 2128 KiB | 3744 KiB |
| `yuanyang-512` | sign | 170949 | 2804 KiB | 2828 KiB |
| `yuanyang-512` | verify | 170949 | 2752 KiB | 2752 KiB |
| `yuanyang-1024` | keygen | 225029 | 2224 KiB | 3996 KiB |
| `yuanyang-1024` | sign | 225029 | 3060 KiB | 3156 KiB |
| `yuanyang-1024` | verify | 225029 | 3164 KiB | 3164 KiB |
| `yuanyang-2048` | keygen | 346213 | 2252 KiB | 6496 KiB |
| `yuanyang-2048` | sign | 346213 | 3500 KiB | 5796 KiB |
| `yuanyang-2048` | verify | 346213 | 3824 KiB | 5632 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `yuanyang-512` | 738 | 15584 | 561 |
| `yuanyang-1024` | 1570 | 31264 | 1150 |
| `yuanyang-2048` | 3330 | 62720 | 2364 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `yuanyang-512` | keygen | 0.0% | 24% | drng 83.4 |
| `yuanyang-512` | sign | 1.9% | 38% | drng 67.9, pseudoXOF 1.44, sm3hash 1 |
| `yuanyang-512` | verify | 59% | 0.0% | pseudoXOF 1, sm3hash 1 |
| `yuanyang-1024` | keygen | 0.0% | 4.2% | drng 16.5 |
| `yuanyang-1024` | sign | 1.8% | 36% | drng 105, pseudoXOF 1.05, sm3hash 1 |
| `yuanyang-1024` | verify | 54% | 0.0% | pseudoXOF 1, sm3hash 1 |
| `yuanyang-2048` | keygen | 0.0% | 19% | drng 347 |
| `yuanyang-2048` | sign | 1.7% | 38% | drng 359, pseudoXOF 1.79, sm3hash 1 |
| `yuanyang-2048` | verify | 49% | 0.0% | pseudoXOF 1, sm3hash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `yuanyang-512` | KAT log (sha256 `15822bfdeb307ec0…`) | `kat/sign-34/yuanyang-512.log` |
| `yuanyang-512` | timing keygen | `records/sign-34/yuanyang-512__keygen.json` |
| `yuanyang-512` | timing sign | `records/sign-34/yuanyang-512__sign.json` |
| `yuanyang-512` | timing verify | `records/sign-34/yuanyang-512__verify.json` |
| `yuanyang-512` | hash profile keygen | `profile/sign-34/yuanyang-512__keygen.json` |
| `yuanyang-512` | hash profile sign | `profile/sign-34/yuanyang-512__sign.json` |
| `yuanyang-512` | hash profile verify | `profile/sign-34/yuanyang-512__verify.json` |
| `yuanyang-1024` | KAT log (sha256 `ea6fde6f2d953858…`) | `kat/sign-34/yuanyang-1024.log` |
| `yuanyang-1024` | timing keygen | `records/sign-34/yuanyang-1024__keygen.json` |
| `yuanyang-1024` | timing sign | `records/sign-34/yuanyang-1024__sign.json` |
| `yuanyang-1024` | timing verify | `records/sign-34/yuanyang-1024__verify.json` |
| `yuanyang-1024` | hash profile keygen | `profile/sign-34/yuanyang-1024__keygen.json` |
| `yuanyang-1024` | hash profile sign | `profile/sign-34/yuanyang-1024__sign.json` |
| `yuanyang-1024` | hash profile verify | `profile/sign-34/yuanyang-1024__verify.json` |
| `yuanyang-2048` | KAT log (sha256 `ca6fec2b6085bb9f…`) | `kat/sign-34/yuanyang-2048.log` |
| `yuanyang-2048` | timing keygen | `records/sign-34/yuanyang-2048__keygen.json` |
| `yuanyang-2048` | timing sign | `records/sign-34/yuanyang-2048__sign.json` |
| `yuanyang-2048` | timing verify | `records/sign-34/yuanyang-2048__verify.json` |
| `yuanyang-2048` | hash profile keygen | `profile/sign-34/yuanyang-2048__keygen.json` |
| `yuanyang-2048` | hash profile sign | `profile/sign-34/yuanyang-2048__sign.json` |
| `yuanyang-2048` | hash profile verify | `profile/sign-34/yuanyang-2048__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

