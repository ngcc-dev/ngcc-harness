# sign-21 ReSolveD-ɑ — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-21` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561087368974336.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: ReSolveD-ɑ
- Implementation versions measured: reference
- Parameter sets: `ReSolveD-alpha-160f`, `ReSolveD-alpha-160s`, `ReSolveD-alpha-256f`, `ReSolveD-alpha-256s`, `ReSolveD-alpha-384f`, `ReSolveD-alpha-384s`, `ReSolveD-alpha-512f`, `ReSolveD-alpha-512s`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-21/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `ReSolveD-alpha-160f` | guide | PASS |
| `ReSolveD-alpha-160s` | guide | PASS |
| `ReSolveD-alpha-256f` | guide | PASS |
| `ReSolveD-alpha-256s` | guide | PASS |
| `ReSolveD-alpha-384f` | guide | PASS |
| `ReSolveD-alpha-384s` | guide | PASS |
| `ReSolveD-alpha-512f` | guide | PASS |
| `ReSolveD-alpha-512s` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `ReSolveD-alpha-160f` | keygen | 14.25 M | 6.81 ms | 147 | 6.81 ms | 720 (5 × 144) |
| `ReSolveD-alpha-160f` | sign | 677.96 M | 324 ms | 3.09 | 324 ms | 100 (5 × 20) |
| `ReSolveD-alpha-160f` | verify | 639.43 M | 305 ms | 3.27 | 305 ms | 100 (5 × 20) |
| `ReSolveD-alpha-160s` | keygen | 14.26 M | 6.82 ms | 147 | 6.82 ms | 720 (5 × 144) |
| `ReSolveD-alpha-160s` | sign | 4.66 G | 2.24 s | 0.447 | 2.24 s | 100 (5 × 20) |
| `ReSolveD-alpha-160s` | verify | 4.61 G | 2.21 s | 0.452 | 2.22 s | 100 (5 × 20) |
| `ReSolveD-alpha-256f` | keygen | 32.73 M | 15.8 ms | 63.2 | 15.8 ms | 315 (5 × 63) |
| `ReSolveD-alpha-256f` | sign | 1.39 G | 667 ms | 1.5 | 667 ms | 100 (5 × 20) |
| `ReSolveD-alpha-256f` | verify | 1.33 G | 636 ms | 1.57 | 637 ms | 100 (5 × 20) |
| `ReSolveD-alpha-256s` | keygen | 32.74 M | 15.8 ms | 63.2 | 15.8 ms | 315 (5 × 63) |
| `ReSolveD-alpha-256s` | sign | 13.08 G | 6.28 s | 0.159 | 6.28 s | 100 (5 × 20) |
| `ReSolveD-alpha-256s` | verify | 12.98 G | 6.23 s | 0.161 | 6.23 s | 100 (5 × 20) |
| `ReSolveD-alpha-384f` | keygen | 80.53 M | 38.9 ms | 25.7 | 38.9 ms | 130 (5 × 26) |
| `ReSolveD-alpha-384f` | sign | 3.77 G | 1.81 s | 0.552 | 1.81 s | 100 (5 × 20) |
| `ReSolveD-alpha-384f` | verify | 3.61 G | 1.73 s | 0.577 | 1.73 s | 100 (5 × 20) |
| `ReSolveD-alpha-384s` | keygen | 80.50 M | 38.9 ms | 25.7 | 38.9 ms | 130 (5 × 26) |
| `ReSolveD-alpha-384s` | sign | 33.60 G | 16.1 s | 0.0621 | 16.1 s | 55 (5 × 11) |
| `ReSolveD-alpha-384s` | verify | 33.31 G | 16 s | 0.0627 | 16 s | 55 (5 × 11) |
| `ReSolveD-alpha-512f` | keygen | 133.32 M | 64.4 ms | 15.5 | 64.4 ms | 100 (5 × 20) |
| `ReSolveD-alpha-512f` | sign | 802.75 M | 388 ms | 2.58 | 386 ms | 100 (5 × 20) |
| `ReSolveD-alpha-512f` | verify | 640.49 M | 308 ms | 3.25 | 307 ms | 100 (5 × 20) |
| `ReSolveD-alpha-512s` | keygen | 133.37 M | 64.4 ms | 15.5 | 64.4 ms | 100 (5 × 20) |
| `ReSolveD-alpha-512s` | sign | 3.82 G | 1.84 s | 0.544 | 1.84 s | 100 (5 × 20) |
| `ReSolveD-alpha-512s` | verify | 3.28 G | 1.58 s | 0.633 | 1.58 s | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `ReSolveD-alpha-160f` | keygen | 156109 | 1776 KiB | 2216 KiB |
| `ReSolveD-alpha-160f` | sign | 156109 | 2120 KiB | 2644 KiB |
| `ReSolveD-alpha-160f` | verify | 156109 | 2164 KiB | 2468 KiB |
| `ReSolveD-alpha-160s` | keygen | 156109 | 1776 KiB | 2212 KiB |
| `ReSolveD-alpha-160s` | sign | 156109 | 2144 KiB | 5456 KiB |
| `ReSolveD-alpha-160s` | verify | 156109 | 2640 KiB | 5204 KiB |
| `ReSolveD-alpha-256f` | keygen | 156133 | 1780 KiB | 2620 KiB |
| `ReSolveD-alpha-256f` | sign | 156133 | 1832 KiB | 3592 KiB |
| `ReSolveD-alpha-256f` | verify | 156133 | 2820 KiB | 3252 KiB |
| `ReSolveD-alpha-256s` | keygen | 156133 | 1776 KiB | 2660 KiB |
| `ReSolveD-alpha-256s` | sign | 156133 | 1848 KiB | 11816 KiB |
| `ReSolveD-alpha-256s` | verify | 156133 | 3612 KiB | 11276 KiB |
| `ReSolveD-alpha-384f` | keygen | 156133 | 1832 KiB | 3696 KiB |
| `ReSolveD-alpha-384f` | sign | 156133 | 1824 KiB | 5748 KiB |
| `ReSolveD-alpha-384f` | verify | 156133 | 3348 KiB | 5516 KiB |
| `ReSolveD-alpha-384s` | keygen | 156133 | 1868 KiB | 3684 KiB |
| `ReSolveD-alpha-384s` | sign | 156133 | 1864 KiB | 20572 KiB |
| `ReSolveD-alpha-384s` | verify | 156133 | 4224 KiB | 20372 KiB |
| `ReSolveD-alpha-512f` | keygen | 156149 | 1904 KiB | 5080 KiB |
| `ReSolveD-alpha-512f` | sign | 156149 | 1900 KiB | 8516 KiB |
| `ReSolveD-alpha-512f` | verify | 156149 | 4464 KiB | 8336 KiB |
| `ReSolveD-alpha-512s` | keygen | 156149 | 1824 KiB | 4992 KiB |
| `ReSolveD-alpha-512s` | sign | 156149 | 1868 KiB | 35288 KiB |
| `ReSolveD-alpha-512s` | verify | 156149 | 5812 KiB | 35072 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `ReSolveD-alpha-160f` | 121 | 40 | 6851 |
| `ReSolveD-alpha-160s` | 121 | 40 | 5307 |
| `ReSolveD-alpha-256f` | 194 | 64 | 17932 |
| `ReSolveD-alpha-256s` | 194 | 64 | 13973 |
| `ReSolveD-alpha-384f` | 288 | 96 | 40469 |
| `ReSolveD-alpha-384s` | 288 | 96 | 31575 |
| `ReSolveD-alpha-512f` | 385 | 128 | 72611 |
| `ReSolveD-alpha-512s` | 385 | 128 | 56239 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **bypass** — Keccak oracles + AES/Rijndael/SHACAL-2 PRGs; pseudoXOF only with -DXOF_PSEUDO (undefined)

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `ReSolveD-alpha-160f` | keygen | 34% | 0.1% | drng 2, pseudoXOF 1 |
| `ReSolveD-alpha-160f` | sign | 2.5% | 0.0% | drng 1, pseudoXOF 353 |
| `ReSolveD-alpha-160f` | verify | 1.8% | 0.0% | pseudoXOF 27 |
| `ReSolveD-alpha-160s` | keygen | 34% | 0.1% | drng 2, pseudoXOF 1 |
| `ReSolveD-alpha-160s` | sign | 1.4% | 0.0% | drng 1, pseudoXOF 1.9e+03 |
| `ReSolveD-alpha-160s` | verify | 1.2% | 0.0% | pseudoXOF 20 |
| `ReSolveD-alpha-256f` | keygen | 39% | 0.0% | drng 2, pseudoXOF 1 |
| `ReSolveD-alpha-256f` | sign | 3.0% | 0.0% | drng 1, pseudoXOF 407 |
| `ReSolveD-alpha-256f` | verify | 2.1% | 0.0% | pseudoXOF 41 |
| `ReSolveD-alpha-256s` | keygen | 39% | 0.0% | drng 2, pseudoXOF 1 |
| `ReSolveD-alpha-256s` | sign | 1.4% | 0.0% | drng 1, pseudoXOF 4.71e+03 |
| `ReSolveD-alpha-256s` | verify | 1.1% | 0.0% | pseudoXOF 28 |
| `ReSolveD-alpha-384f` | keygen | 36% | 0.0% | drng 2, pseudoXOF 1 |
| `ReSolveD-alpha-384f` | sign | 3.0% | 0.0% | drng 1, pseudoXOF 490 |
| `ReSolveD-alpha-384f` | verify | 2.2% | 0.0% | pseudoXOF 59 |
| `ReSolveD-alpha-384s` | keygen | 36% | 0.0% | drng 2, pseudoXOF 1 |
| `ReSolveD-alpha-384s` | sign | 1.7% | 0.0% | drng 1, pseudoXOF 5.78e+03 |
| `ReSolveD-alpha-384s` | verify | 1.4% | 0.0% | pseudoXOF 40 |
| `ReSolveD-alpha-512f` | keygen | 72% | 0.0% | drng 2, pseudoXOF 1 |
| `ReSolveD-alpha-512f` | sign | 38% | 0.0% | drng 1, pseudoXOF 190 |
| `ReSolveD-alpha-512f` | verify | 33% | 0.0% | pseudoXOF 78 |
| `ReSolveD-alpha-512s` | keygen | 72% | 0.0% | drng 2, pseudoXOF 1 |
| `ReSolveD-alpha-512s` | sign | 39% | 0.0% | drng 1, pseudoXOF 1.3e+04 |
| `ReSolveD-alpha-512s` | verify | 35% | 0.0% | pseudoXOF 52 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `ReSolveD-alpha-160f` | KAT log (sha256 `55b94c7c9237d7b8…`) | `kat/sign-21/ReSolveD-alpha-160f.log` |
| `ReSolveD-alpha-160f` | timing keygen | `records/sign-21/ReSolveD-alpha-160f__keygen.json` |
| `ReSolveD-alpha-160f` | timing sign | `records/sign-21/ReSolveD-alpha-160f__sign.json` |
| `ReSolveD-alpha-160f` | timing verify | `records/sign-21/ReSolveD-alpha-160f__verify.json` |
| `ReSolveD-alpha-160f` | hash profile keygen | `profile/sign-21/ReSolveD-alpha-160f__keygen.json` |
| `ReSolveD-alpha-160f` | hash profile sign | `profile/sign-21/ReSolveD-alpha-160f__sign.json` |
| `ReSolveD-alpha-160f` | hash profile verify | `profile/sign-21/ReSolveD-alpha-160f__verify.json` |
| `ReSolveD-alpha-160s` | KAT log (sha256 `63629f9c598320bf…`) | `kat/sign-21/ReSolveD-alpha-160s.log` |
| `ReSolveD-alpha-160s` | timing keygen | `records/sign-21/ReSolveD-alpha-160s__keygen.json` |
| `ReSolveD-alpha-160s` | timing sign | `records/sign-21/ReSolveD-alpha-160s__sign.json` |
| `ReSolveD-alpha-160s` | timing verify | `records/sign-21/ReSolveD-alpha-160s__verify.json` |
| `ReSolveD-alpha-160s` | hash profile keygen | `profile/sign-21/ReSolveD-alpha-160s__keygen.json` |
| `ReSolveD-alpha-160s` | hash profile sign | `profile/sign-21/ReSolveD-alpha-160s__sign.json` |
| `ReSolveD-alpha-160s` | hash profile verify | `profile/sign-21/ReSolveD-alpha-160s__verify.json` |
| `ReSolveD-alpha-256f` | KAT log (sha256 `879a0f292adbb08d…`) | `kat/sign-21/ReSolveD-alpha-256f.log` |
| `ReSolveD-alpha-256f` | timing keygen | `records/sign-21/ReSolveD-alpha-256f__keygen.json` |
| `ReSolveD-alpha-256f` | timing sign | `records/sign-21/ReSolveD-alpha-256f__sign.json` |
| `ReSolveD-alpha-256f` | timing verify | `records/sign-21/ReSolveD-alpha-256f__verify.json` |
| `ReSolveD-alpha-256f` | hash profile keygen | `profile/sign-21/ReSolveD-alpha-256f__keygen.json` |
| `ReSolveD-alpha-256f` | hash profile sign | `profile/sign-21/ReSolveD-alpha-256f__sign.json` |
| `ReSolveD-alpha-256f` | hash profile verify | `profile/sign-21/ReSolveD-alpha-256f__verify.json` |
| `ReSolveD-alpha-256s` | KAT log (sha256 `1a46ca8bb75e7961…`) | `kat/sign-21/ReSolveD-alpha-256s.log` |
| `ReSolveD-alpha-256s` | timing keygen | `records/sign-21/ReSolveD-alpha-256s__keygen.json` |
| `ReSolveD-alpha-256s` | timing sign | `records/sign-21/ReSolveD-alpha-256s__sign.json` |
| `ReSolveD-alpha-256s` | timing verify | `records/sign-21/ReSolveD-alpha-256s__verify.json` |
| `ReSolveD-alpha-256s` | hash profile keygen | `profile/sign-21/ReSolveD-alpha-256s__keygen.json` |
| `ReSolveD-alpha-256s` | hash profile sign | `profile/sign-21/ReSolveD-alpha-256s__sign.json` |
| `ReSolveD-alpha-256s` | hash profile verify | `profile/sign-21/ReSolveD-alpha-256s__verify.json` |
| `ReSolveD-alpha-384f` | KAT log (sha256 `76bd4639d02ca834…`) | `kat/sign-21/ReSolveD-alpha-384f.log` |
| `ReSolveD-alpha-384f` | timing keygen | `records/sign-21/ReSolveD-alpha-384f__keygen.json` |
| `ReSolveD-alpha-384f` | timing sign | `records/sign-21/ReSolveD-alpha-384f__sign.json` |
| `ReSolveD-alpha-384f` | timing verify | `records/sign-21/ReSolveD-alpha-384f__verify.json` |
| `ReSolveD-alpha-384f` | hash profile keygen | `profile/sign-21/ReSolveD-alpha-384f__keygen.json` |
| `ReSolveD-alpha-384f` | hash profile sign | `profile/sign-21/ReSolveD-alpha-384f__sign.json` |
| `ReSolveD-alpha-384f` | hash profile verify | `profile/sign-21/ReSolveD-alpha-384f__verify.json` |
| `ReSolveD-alpha-384s` | KAT log (sha256 `478d27d642fab629…`) | `kat/sign-21/ReSolveD-alpha-384s.log` |
| `ReSolveD-alpha-384s` | timing keygen | `records/sign-21/ReSolveD-alpha-384s__keygen.json` |
| `ReSolveD-alpha-384s` | timing sign | `records/sign-21/ReSolveD-alpha-384s__sign.json` |
| `ReSolveD-alpha-384s` | timing verify | `records/sign-21/ReSolveD-alpha-384s__verify.json` |
| `ReSolveD-alpha-384s` | hash profile keygen | `profile/sign-21/ReSolveD-alpha-384s__keygen.json` |
| `ReSolveD-alpha-384s` | hash profile sign | `profile/sign-21/ReSolveD-alpha-384s__sign.json` |
| `ReSolveD-alpha-384s` | hash profile verify | `profile/sign-21/ReSolveD-alpha-384s__verify.json` |
| `ReSolveD-alpha-512f` | KAT log (sha256 `985ecb3dc88da9a9…`) | `kat/sign-21/ReSolveD-alpha-512f.log` |
| `ReSolveD-alpha-512f` | timing keygen | `records/sign-21/ReSolveD-alpha-512f__keygen.json` |
| `ReSolveD-alpha-512f` | timing sign | `records/sign-21/ReSolveD-alpha-512f__sign.json` |
| `ReSolveD-alpha-512f` | timing verify | `records/sign-21/ReSolveD-alpha-512f__verify.json` |
| `ReSolveD-alpha-512f` | hash profile keygen | `profile/sign-21/ReSolveD-alpha-512f__keygen.json` |
| `ReSolveD-alpha-512f` | hash profile sign | `profile/sign-21/ReSolveD-alpha-512f__sign.json` |
| `ReSolveD-alpha-512f` | hash profile verify | `profile/sign-21/ReSolveD-alpha-512f__verify.json` |
| `ReSolveD-alpha-512s` | KAT log (sha256 `cb37ed04e97a7f6b…`) | `kat/sign-21/ReSolveD-alpha-512s.log` |
| `ReSolveD-alpha-512s` | timing keygen | `records/sign-21/ReSolveD-alpha-512s__keygen.json` |
| `ReSolveD-alpha-512s` | timing sign | `records/sign-21/ReSolveD-alpha-512s__sign.json` |
| `ReSolveD-alpha-512s` | timing verify | `records/sign-21/ReSolveD-alpha-512s__verify.json` |
| `ReSolveD-alpha-512s` | hash profile keygen | `profile/sign-21/ReSolveD-alpha-512s__keygen.json` |
| `ReSolveD-alpha-512s` | hash profile sign | `profile/sign-21/ReSolveD-alpha-512s__sign.json` |
| `ReSolveD-alpha-512s` | hash profile verify | `profile/sign-21/ReSolveD-alpha-512s__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

