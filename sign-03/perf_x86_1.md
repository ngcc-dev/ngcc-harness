# sign-03 CEDRUS+C — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-03` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561076497338368.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: CEDRUS+C
- Implementation versions measured: reference
- Parameter sets: `CEDRUSC-160f`, `CEDRUSC-160s`, `CEDRUSC-256f`, `CEDRUSC-256s`, `CEDRUSC-384f`, `CEDRUSC-384s`, `CEDRUSC-512f`, `CEDRUSC-512s`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-03/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `CEDRUSC-160f` | guide | PASS |
| `CEDRUSC-160s` | guide | PASS |
| `CEDRUSC-256f` | guide | PASS |
| `CEDRUSC-256s` | guide | PASS |
| `CEDRUSC-384f` | guide | PASS |
| `CEDRUSC-384s` | guide | PASS |
| `CEDRUSC-512f` | guide | PASS |
| `CEDRUSC-512s` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `CEDRUSC-160f` | keygen | 14.36 M | 6.86 ms | 146 | 6.85 ms | 730 (5 × 146) |
| `CEDRUSC-160f` | sign | 513.49 M | 247 ms | 4.05 | 245 ms | 100 (5 × 20) |
| `CEDRUSC-160f` | verify | 15.34 M | 7.34 ms | 136 | 7.33 ms | 685 (5 × 137) |
| `CEDRUSC-160s` | keygen | 583.25 M | 280 ms | 3.57 | 279 ms | 100 (5 × 20) |
| `CEDRUSC-160s` | sign | 8.65 G | 4.15 s | 0.241 | 4.15 s | 100 (5 × 20) |
| `CEDRUSC-160s` | verify | 20.99 M | 10 ms | 99.8 | 9.98 ms | 495 (5 × 99) |
| `CEDRUSC-256f` | keygen | 46.43 M | 22.2 ms | 45.1 | 22.2 ms | 225 (5 × 45) |
| `CEDRUSC-256f` | sign | 1.24 G | 593 ms | 1.69 | 593 ms | 100 (5 × 20) |
| `CEDRUSC-256f` | verify | 21.30 M | 10.2 ms | 98.3 | 10.2 ms | 495 (5 × 99) |
| `CEDRUSC-256s` | keygen | 964.78 M | 464 ms | 2.16 | 461 ms | 100 (5 × 20) |
| `CEDRUSC-256s` | sign | 12.74 G | 6.11 s | 0.164 | 6.1 s | 100 (5 × 20) |
| `CEDRUSC-256s` | verify | 35.11 M | 16.8 ms | 59.6 | 16.8 ms | 300 (5 × 60) |
| `CEDRUSC-384f` | keygen | 656.21 M | 315 ms | 3.18 | 313 ms | 100 (5 × 20) |
| `CEDRUSC-384f` | sign | 12.48 G | 5.99 s | 0.167 | 5.99 s | 100 (5 × 20) |
| `CEDRUSC-384f` | verify | 125.86 M | 60.1 ms | 16.6 | 60.1 ms | 100 (5 × 20) |
| `CEDRUSC-384s` | keygen | 3.27 G | 1.57 s | 0.637 | 1.57 s | 100 (5 × 20) |
| `CEDRUSC-384s` | sign | 48.20 G | 23.1 s | 0.0433 | 23.1 s | 35 (5 × 7) |
| `CEDRUSC-384s` | verify | 53.33 M | 25.5 ms | 39.3 | 25.5 ms | 195 (5 × 39) |
| `CEDRUSC-512f` | keygen | 1.76 G | 845 ms | 1.18 | 847 ms | 100 (5 × 20) |
| `CEDRUSC-512f` | sign | 24.73 G | 11.9 s | 0.0843 | 11.9 s | 75 (5 × 15) |
| `CEDRUSC-512f` | verify | 157.20 M | 75.1 ms | 13.3 | 75.1 ms | 100 (5 × 20) |
| `CEDRUSC-512s` | keygen | 7.00 G | 3.34 s | 0.299 | 3.34 s | 100 (5 × 20) |
| `CEDRUSC-512s` | sign | 86.78 G | 41.4 s | 0.0241 | 41.4 s | 20 (5 × 4) |
| `CEDRUSC-512s` | verify | 115.31 M | 55.1 ms | 18.2 | 55.1 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `CEDRUSC-160f` | keygen | 29305 | 1728 KiB | 1796 KiB |
| `CEDRUSC-160f` | sign | 29305 | 1708 KiB | 1820 KiB |
| `CEDRUSC-160f` | verify | 29305 | 1680 KiB | 1808 KiB |
| `CEDRUSC-160s` | keygen | 29201 | 1684 KiB | 1808 KiB |
| `CEDRUSC-160s` | sign | 29201 | 1720 KiB | 1784 KiB |
| `CEDRUSC-160s` | verify | 29201 | 1708 KiB | 1808 KiB |
| `CEDRUSC-256f` | keygen | 29681 | 1740 KiB | 1836 KiB |
| `CEDRUSC-256f` | sign | 29681 | 1728 KiB | 1840 KiB |
| `CEDRUSC-256f` | verify | 29681 | 1700 KiB | 1828 KiB |
| `CEDRUSC-256s` | keygen | 29609 | 1736 KiB | 1812 KiB |
| `CEDRUSC-256s` | sign | 29609 | 1728 KiB | 1824 KiB |
| `CEDRUSC-256s` | verify | 29609 | 1732 KiB | 1816 KiB |
| `CEDRUSC-384f` | keygen | 30169 | 1784 KiB | 1888 KiB |
| `CEDRUSC-384f` | sign | 30169 | 1788 KiB | 1856 KiB |
| `CEDRUSC-384f` | verify | 30169 | 1796 KiB | 1872 KiB |
| `CEDRUSC-384s` | keygen | 30385 | 1748 KiB | 1884 KiB |
| `CEDRUSC-384s` | sign | 30385 | 1780 KiB | 1872 KiB |
| `CEDRUSC-384s` | verify | 30385 | 1772 KiB | 1860 KiB |
| `CEDRUSC-512f` | keygen | 30865 | 1820 KiB | 1912 KiB |
| `CEDRUSC-512f` | sign | 30865 | 1844 KiB | 1928 KiB |
| `CEDRUSC-512f` | verify | 30865 | 1840 KiB | 1948 KiB |
| `CEDRUSC-512s` | keygen | 30729 | 1788 KiB | 1908 KiB |
| `CEDRUSC-512s` | sign | 30729 | 1788 KiB | 1920 KiB |
| `CEDRUSC-512s` | verify | 30729 | 1812 KiB | 1916 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `CEDRUSC-160f` | 40 | 80 | 19812 |
| `CEDRUSC-160s` | 40 | 80 | 9460 |
| `CEDRUSC-256f` | 64 | 128 | 43548 |
| `CEDRUSC-256s` | 64 | 128 | 24104 |
| `CEDRUSC-384f` | 96 | 192 | 75988 |
| `CEDRUSC-384s` | 96 | 192 | 61572 |
| `CEDRUSC-512f` | 128 | 256 | 121520 |
| `CEDRUSC-512s` | 128 | 256 | 98212 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `CEDRUSC-160f` | keygen | 98% | 0.0% | drng 1, sm3hash 5.01e+03 |
| `CEDRUSC-160f` | sign | 98% | 0.0% | drng 1, pseudoXOF 456, sm3hash 1.79e+05 |
| `CEDRUSC-160f` | verify | 98% | 0.0% | pseudoXOF 1, sm3hash 5.3e+03 |
| `CEDRUSC-160s` | keygen | 98% | 0.0% | drng 1, sm3hash 2.05e+05 |
| `CEDRUSC-160s` | sign | 98% | 0.0% | drng 1, pseudoXOF 3.32e+04, sm3hash 2.99e+06 |
| `CEDRUSC-160s` | verify | 98% | 0.0% | pseudoXOF 1, sm3hash 7.34e+03 |
| `CEDRUSC-256f` | keygen | 98% | 0.0% | drng 1, sm3hash 1.62e+04 |
| `CEDRUSC-256f` | sign | 98% | 0.0% | drng 1, pseudoXOF 708, sm3hash 4.25e+05 |
| `CEDRUSC-256f` | verify | 98% | 0.0% | pseudoXOF 1, sm3hash 7.1e+03 |
| `CEDRUSC-256s` | keygen | 98% | 0.0% | drng 1, sm3hash 3.4e+05 |
| `CEDRUSC-256s` | sign | 98% | 0.0% | drng 1, pseudoXOF 3.11e+04, sm3hash 4.34e+06 |
| `CEDRUSC-256s` | verify | 98% | 0.0% | pseudoXOF 1, sm3hash 1.22e+04 |
| `CEDRUSC-384f` | keygen | 99% | 0.0% | drng 1, pseudoXOF 7.79e+04 |
| `CEDRUSC-384f` | sign | 99% | 0.0% | drng 1, pseudoXOF 1.48e+06 |
| `CEDRUSC-384f` | verify | 99% | 0.0% | pseudoXOF 1.48e+04 |
| `CEDRUSC-384s` | keygen | 99% | 0.0% | drng 1, pseudoXOF 3.86e+05 |
| `CEDRUSC-384s` | sign | 99% | 0.0% | drng 1, pseudoXOF 5.46e+06 |
| `CEDRUSC-384s` | verify | 99% | 0.0% | pseudoXOF 6.19e+03 |
| `CEDRUSC-512f` | keygen | 99% | 0.0% | drng 1, pseudoXOF 2.08e+05 |
| `CEDRUSC-512f` | sign | 99% | 0.0% | drng 1, pseudoXOF 2.88e+06 |
| `CEDRUSC-512f` | verify | 99% | 0.0% | pseudoXOF 1.81e+04 |
| `CEDRUSC-512s` | keygen | 99% | 0.0% | drng 1, pseudoXOF 8.28e+05 |
| `CEDRUSC-512s` | sign | 99% | 0.0% | drng 1, pseudoXOF 9.97e+06 |
| `CEDRUSC-512s` | verify | 99% | 0.0% | pseudoXOF 1.33e+04 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `CEDRUSC-160f` | KAT log (sha256 `ba942570c052899c…`) | `kat/sign-03/CEDRUSC-160f.log` |
| `CEDRUSC-160f` | timing keygen | `records/sign-03/CEDRUSC-160f__keygen.json` |
| `CEDRUSC-160f` | timing sign | `records/sign-03/CEDRUSC-160f__sign.json` |
| `CEDRUSC-160f` | timing verify | `records/sign-03/CEDRUSC-160f__verify.json` |
| `CEDRUSC-160f` | hash profile keygen | `profile/sign-03/CEDRUSC-160f__keygen.json` |
| `CEDRUSC-160f` | hash profile sign | `profile/sign-03/CEDRUSC-160f__sign.json` |
| `CEDRUSC-160f` | hash profile verify | `profile/sign-03/CEDRUSC-160f__verify.json` |
| `CEDRUSC-160s` | KAT log (sha256 `86580d87f29b4483…`) | `kat/sign-03/CEDRUSC-160s.log` |
| `CEDRUSC-160s` | timing keygen | `records/sign-03/CEDRUSC-160s__keygen.json` |
| `CEDRUSC-160s` | timing sign | `records/sign-03/CEDRUSC-160s__sign.json` |
| `CEDRUSC-160s` | timing verify | `records/sign-03/CEDRUSC-160s__verify.json` |
| `CEDRUSC-160s` | hash profile keygen | `profile/sign-03/CEDRUSC-160s__keygen.json` |
| `CEDRUSC-160s` | hash profile sign | `profile/sign-03/CEDRUSC-160s__sign.json` |
| `CEDRUSC-160s` | hash profile verify | `profile/sign-03/CEDRUSC-160s__verify.json` |
| `CEDRUSC-256f` | KAT log (sha256 `9edafcfc864d5524…`) | `kat/sign-03/CEDRUSC-256f.log` |
| `CEDRUSC-256f` | timing keygen | `records/sign-03/CEDRUSC-256f__keygen.json` |
| `CEDRUSC-256f` | timing sign | `records/sign-03/CEDRUSC-256f__sign.json` |
| `CEDRUSC-256f` | timing verify | `records/sign-03/CEDRUSC-256f__verify.json` |
| `CEDRUSC-256f` | hash profile keygen | `profile/sign-03/CEDRUSC-256f__keygen.json` |
| `CEDRUSC-256f` | hash profile sign | `profile/sign-03/CEDRUSC-256f__sign.json` |
| `CEDRUSC-256f` | hash profile verify | `profile/sign-03/CEDRUSC-256f__verify.json` |
| `CEDRUSC-256s` | KAT log (sha256 `3559b91fe71e81c9…`) | `kat/sign-03/CEDRUSC-256s.log` |
| `CEDRUSC-256s` | timing keygen | `records/sign-03/CEDRUSC-256s__keygen.json` |
| `CEDRUSC-256s` | timing sign | `records/sign-03/CEDRUSC-256s__sign.json` |
| `CEDRUSC-256s` | timing verify | `records/sign-03/CEDRUSC-256s__verify.json` |
| `CEDRUSC-256s` | hash profile keygen | `profile/sign-03/CEDRUSC-256s__keygen.json` |
| `CEDRUSC-256s` | hash profile sign | `profile/sign-03/CEDRUSC-256s__sign.json` |
| `CEDRUSC-256s` | hash profile verify | `profile/sign-03/CEDRUSC-256s__verify.json` |
| `CEDRUSC-384f` | KAT log (sha256 `8d39bd0b1ff6aa3a…`) | `kat/sign-03/CEDRUSC-384f.log` |
| `CEDRUSC-384f` | timing keygen | `records/sign-03/CEDRUSC-384f__keygen.json` |
| `CEDRUSC-384f` | timing sign | `records/sign-03/CEDRUSC-384f__sign.json` |
| `CEDRUSC-384f` | timing verify | `records/sign-03/CEDRUSC-384f__verify.json` |
| `CEDRUSC-384f` | hash profile keygen | `profile/sign-03/CEDRUSC-384f__keygen.json` |
| `CEDRUSC-384f` | hash profile sign | `profile/sign-03/CEDRUSC-384f__sign.json` |
| `CEDRUSC-384f` | hash profile verify | `profile/sign-03/CEDRUSC-384f__verify.json` |
| `CEDRUSC-384s` | KAT log (sha256 `41c2d712f187c7ab…`) | `kat/sign-03/CEDRUSC-384s.log` |
| `CEDRUSC-384s` | timing keygen | `records/sign-03/CEDRUSC-384s__keygen.json` |
| `CEDRUSC-384s` | timing sign | `records/sign-03/CEDRUSC-384s__sign.json` |
| `CEDRUSC-384s` | timing verify | `records/sign-03/CEDRUSC-384s__verify.json` |
| `CEDRUSC-384s` | hash profile keygen | `profile/sign-03/CEDRUSC-384s__keygen.json` |
| `CEDRUSC-384s` | hash profile sign | `profile/sign-03/CEDRUSC-384s__sign.json` |
| `CEDRUSC-384s` | hash profile verify | `profile/sign-03/CEDRUSC-384s__verify.json` |
| `CEDRUSC-512f` | KAT log (sha256 `784ab77dd8c07bbe…`) | `kat/sign-03/CEDRUSC-512f.log` |
| `CEDRUSC-512f` | timing keygen | `records/sign-03/CEDRUSC-512f__keygen.json` |
| `CEDRUSC-512f` | timing sign | `records/sign-03/CEDRUSC-512f__sign.json` |
| `CEDRUSC-512f` | timing verify | `records/sign-03/CEDRUSC-512f__verify.json` |
| `CEDRUSC-512f` | hash profile keygen | `profile/sign-03/CEDRUSC-512f__keygen.json` |
| `CEDRUSC-512f` | hash profile sign | `profile/sign-03/CEDRUSC-512f__sign.json` |
| `CEDRUSC-512f` | hash profile verify | `profile/sign-03/CEDRUSC-512f__verify.json` |
| `CEDRUSC-512s` | KAT log (sha256 `17c045bb905079a5…`) | `kat/sign-03/CEDRUSC-512s.log` |
| `CEDRUSC-512s` | timing keygen | `records/sign-03/CEDRUSC-512s__keygen.json` |
| `CEDRUSC-512s` | timing sign | `records/sign-03/CEDRUSC-512s__sign.json` |
| `CEDRUSC-512s` | timing verify | `records/sign-03/CEDRUSC-512s__verify.json` |
| `CEDRUSC-512s` | hash profile keygen | `profile/sign-03/CEDRUSC-512s__keygen.json` |
| `CEDRUSC-512s` | hash profile sign | `profile/sign-03/CEDRUSC-512s__sign.json` |
| `CEDRUSC-512s` | hash profile verify | `profile/sign-03/CEDRUSC-512s__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

