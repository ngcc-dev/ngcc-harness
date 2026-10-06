# sign-28 SYDO — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-28` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561105194766336.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: SYDO
- Implementation versions measured: reference
- Parameter sets: `sydo_160f`, `sydo_160s`, `sydo_256f`, `sydo_256s`, `sydo_512f`, `sydo_512s`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-28/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `sydo_160f` | guide | PASS |
| `sydo_160s` | guide | PASS |
| `sydo_256f` | guide | PASS |
| `sydo_256s` | guide | PASS |
| `sydo_512f` | guide | PASS |
| `sydo_512s` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `sydo_160f` | keygen | 901.2 k | 430 µs | 2.32e+03 | 430 µs | 11240 (5 × 2248) |
| `sydo_160f` | sign | 443.76 M | 212 ms | 4.71 | 212 ms | 100 (5 × 20) |
| `sydo_160f` | verify | 449.76 M | 217 ms | 4.62 | 215 ms | 100 (5 × 20) |
| `sydo_160s` | keygen | 901.6 k | 431 µs | 2.32e+03 | 431 µs | 11195 (5 × 2239) |
| `sydo_160s` | sign | 1.12 G | 537 ms | 1.86 | 537 ms | 100 (5 × 20) |
| `sydo_160s` | verify | 1.11 G | 529 ms | 1.89 | 529 ms | 100 (5 × 20) |
| `sydo_256f` | keygen | 1.60 M | 763 µs | 1.31e+03 | 763 µs | 6325 (5 × 1265) |
| `sydo_256f` | sign | 967.48 M | 466 ms | 2.14 | 464 ms | 100 (5 × 20) |
| `sydo_256f` | verify | 1.00 G | 481 ms | 2.08 | 478 ms | 100 (5 × 20) |
| `sydo_256s` | keygen | 1.60 M | 763 µs | 1.31e+03 | 763 µs | 6405 (5 × 1281) |
| `sydo_256s` | sign | 2.40 G | 1.16 s | 0.865 | 1.16 s | 100 (5 × 20) |
| `sydo_256s` | verify | 2.43 G | 1.17 s | 0.856 | 1.17 s | 100 (5 × 20) |
| `sydo_512f` | keygen | 577.0 k | 276 µs | 3.62e+03 | 277 µs | 17220 (5 × 3444) |
| `sydo_512f` | sign | 2.49 G | 1.2 s | 0.833 | 1.2 s | 100 (5 × 20) |
| `sydo_512f` | verify | 2.65 G | 1.27 s | 0.787 | 1.27 s | 100 (5 × 20) |
| `sydo_512s` | keygen | 570.3 k | 273 µs | 3.66e+03 | 273 µs | 16530 (5 × 3306) |
| `sydo_512s` | sign | 4.30 G | 2.07 s | 0.482 | 2.08 s | 100 (5 × 20) |
| `sydo_512s` | verify | 4.36 G | 2.1 s | 0.477 | 2.1 s | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `sydo_160f` | keygen | 269273 | 1804 KiB | 1872 KiB |
| `sydo_160f` | sign | 269273 | 1808 KiB | 4076 KiB |
| `sydo_160f` | verify | 269273 | 2156 KiB | 3668 KiB |
| `sydo_160s` | keygen | 269273 | 1764 KiB | 1896 KiB |
| `sydo_160s` | sign | 269273 | 1808 KiB | 6396 KiB |
| `sydo_160s` | verify | 269273 | 2188 KiB | 6024 KiB |
| `sydo_256f` | keygen | 269273 | 1816 KiB | 1916 KiB |
| `sydo_256f` | sign | 269273 | 1820 KiB | 6512 KiB |
| `sydo_256f` | verify | 269273 | 2272 KiB | 6300 KiB |
| `sydo_256s` | keygen | 269273 | 1772 KiB | 1896 KiB |
| `sydo_256s` | sign | 269273 | 1820 KiB | 12576 KiB |
| `sydo_256s` | verify | 269273 | 2328 KiB | 12404 KiB |
| `sydo_512f` | keygen | 269273 | 1880 KiB | 1944 KiB |
| `sydo_512f` | sign | 269273 | 1844 KiB | 18240 KiB |
| `sydo_512f` | verify | 269273 | 2252 KiB | 18004 KiB |
| `sydo_512s` | keygen | 269273 | 1852 KiB | 1916 KiB |
| `sydo_512s` | sign | 269273 | 1820 KiB | 42596 KiB |
| `sydo_512s` | verify | 269273 | 2404 KiB | 42404 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `sydo_160f` | 80 | 174 | 6724 |
| `sydo_160s` | 80 | 174 | 5428 |
| `sydo_256f` | 128 | 278 | 17604 |
| `sydo_256s` | 128 | 278 | 14444 |
| `sydo_512f` | 246 | 534 | 67716 |
| `sydo_512s` | 246 | 534 | 56672 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **mixed** — hashing via pseudoXOF; PRGs via own AES/Rijndael and a BLAKE2s-round cipher

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `sydo_160f` | keygen | 1.3% | 0.7% | drng 1, pseudoXOF 8 |
| `sydo_160f` | sign | 2.1% | 0.0% | drng 1, pseudoXOF 35.4 |
| `sydo_160f` | verify | 2.1% | 0.0% | pseudoXOF 25 |
| `sydo_160s` | keygen | 1.3% | 0.7% | drng 1, pseudoXOF 8 |
| `sydo_160s` | sign | 5.3% | 0.0% | drng 1, pseudoXOF 1.71e+03 |
| `sydo_160s` | verify | 4.5% | 0.0% | pseudoXOF 19 |
| `sydo_256f` | keygen | 1.1% | 0.4% | drng 1, pseudoXOF 12 |
| `sydo_256f` | sign | 2.5% | 0.0% | drng 1, pseudoXOF 50 |
| `sydo_256f` | verify | 2.5% | 0.0% | pseudoXOF 37 |
| `sydo_256s` | keygen | 1.1% | 0.4% | drng 1, pseudoXOF 12 |
| `sydo_256s` | sign | 5.5% | 0.0% | drng 1, pseudoXOF 145 |
| `sydo_256s` | verify | 5.4% | 0.0% | pseudoXOF 28 |
| `sydo_512f` | keygen | 12% | 1.6% | drng 1, pseudoXOF 23 |
| `sydo_512f` | sign | 7.8% | 0.0% | drng 1, pseudoXOF 97.3 |
| `sydo_512f` | verify | 7.4% | 0.0% | pseudoXOF 69 |
| `sydo_512s` | keygen | 11% | 1.6% | drng 1, pseudoXOF 23 |
| `sydo_512s` | sign | 26% | 0.0% | drng 1, pseudoXOF 2.98e+03 |
| `sydo_512s` | verify | 24% | 0.0% | pseudoXOF 51 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `sydo_160f` | KAT log (sha256 `2bf6097e2c78f39a…`) | `kat/sign-28/sydo_160f.log` |
| `sydo_160f` | timing keygen | `records/sign-28/sydo_160f__keygen.json` |
| `sydo_160f` | timing sign | `records/sign-28/sydo_160f__sign.json` |
| `sydo_160f` | timing verify | `records/sign-28/sydo_160f__verify.json` |
| `sydo_160f` | hash profile keygen | `profile/sign-28/sydo_160f__keygen.json` |
| `sydo_160f` | hash profile sign | `profile/sign-28/sydo_160f__sign.json` |
| `sydo_160f` | hash profile verify | `profile/sign-28/sydo_160f__verify.json` |
| `sydo_160s` | KAT log (sha256 `074c21af4137e488…`) | `kat/sign-28/sydo_160s.log` |
| `sydo_160s` | timing keygen | `records/sign-28/sydo_160s__keygen.json` |
| `sydo_160s` | timing sign | `records/sign-28/sydo_160s__sign.json` |
| `sydo_160s` | timing verify | `records/sign-28/sydo_160s__verify.json` |
| `sydo_160s` | hash profile keygen | `profile/sign-28/sydo_160s__keygen.json` |
| `sydo_160s` | hash profile sign | `profile/sign-28/sydo_160s__sign.json` |
| `sydo_160s` | hash profile verify | `profile/sign-28/sydo_160s__verify.json` |
| `sydo_256f` | KAT log (sha256 `b0647264ed92bf7c…`) | `kat/sign-28/sydo_256f.log` |
| `sydo_256f` | timing keygen | `records/sign-28/sydo_256f__keygen.json` |
| `sydo_256f` | timing sign | `records/sign-28/sydo_256f__sign.json` |
| `sydo_256f` | timing verify | `records/sign-28/sydo_256f__verify.json` |
| `sydo_256f` | hash profile keygen | `profile/sign-28/sydo_256f__keygen.json` |
| `sydo_256f` | hash profile sign | `profile/sign-28/sydo_256f__sign.json` |
| `sydo_256f` | hash profile verify | `profile/sign-28/sydo_256f__verify.json` |
| `sydo_256s` | KAT log (sha256 `4b89775e4fd29635…`) | `kat/sign-28/sydo_256s.log` |
| `sydo_256s` | timing keygen | `records/sign-28/sydo_256s__keygen.json` |
| `sydo_256s` | timing sign | `records/sign-28/sydo_256s__sign.json` |
| `sydo_256s` | timing verify | `records/sign-28/sydo_256s__verify.json` |
| `sydo_256s` | hash profile keygen | `profile/sign-28/sydo_256s__keygen.json` |
| `sydo_256s` | hash profile sign | `profile/sign-28/sydo_256s__sign.json` |
| `sydo_256s` | hash profile verify | `profile/sign-28/sydo_256s__verify.json` |
| `sydo_512f` | KAT log (sha256 `801ca661746d20c3…`) | `kat/sign-28/sydo_512f.log` |
| `sydo_512f` | timing keygen | `records/sign-28/sydo_512f__keygen.json` |
| `sydo_512f` | timing sign | `records/sign-28/sydo_512f__sign.json` |
| `sydo_512f` | timing verify | `records/sign-28/sydo_512f__verify.json` |
| `sydo_512f` | hash profile keygen | `profile/sign-28/sydo_512f__keygen.json` |
| `sydo_512f` | hash profile sign | `profile/sign-28/sydo_512f__sign.json` |
| `sydo_512f` | hash profile verify | `profile/sign-28/sydo_512f__verify.json` |
| `sydo_512s` | KAT log (sha256 `31c1983d7fa39ac0…`) | `kat/sign-28/sydo_512s.log` |
| `sydo_512s` | timing keygen | `records/sign-28/sydo_512s__keygen.json` |
| `sydo_512s` | timing sign | `records/sign-28/sydo_512s__sign.json` |
| `sydo_512s` | timing verify | `records/sign-28/sydo_512s__verify.json` |
| `sydo_512s` | hash profile keygen | `profile/sign-28/sydo_512s__keygen.json` |
| `sydo_512s` | hash profile sign | `profile/sign-28/sydo_512s__sign.json` |
| `sydo_512s` | hash profile verify | `profile/sign-28/sydo_512s__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

