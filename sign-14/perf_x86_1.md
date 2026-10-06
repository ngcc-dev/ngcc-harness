# sign-14 Lynxer — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-14` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561077990510592.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: Lynxer
- Implementation versions measured: reference
- Parameter sets: `Lynxer-160f`, `Lynxer-160s`, `Lynxer-256f`, `Lynxer-256s`, `Lynxer-384f`, `Lynxer-384s`, `Lynxer-512f`, `Lynxer-512s`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-14/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Lynxer-160f` | guide | PASS |
| `Lynxer-160s` | guide | PASS |
| `Lynxer-256f` | guide | PASS |
| `Lynxer-256s` | guide | PASS |
| `Lynxer-384f` | guide | PASS |
| `Lynxer-384s` | guide | PASS |
| `Lynxer-512f` | guide | PASS |
| `Lynxer-512s` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Lynxer-160f` | keygen | 28.80 M | 13.8 ms | 72.7 | 13.8 ms | 365 (5 × 73) |
| `Lynxer-160f` | sign | 533.63 M | 256 ms | 3.9 | 255 ms | 100 (5 × 20) |
| `Lynxer-160f` | verify | 470.80 M | 226 ms | 4.42 | 225 ms | 100 (5 × 20) |
| `Lynxer-160s` | keygen | 28.78 M | 13.7 ms | 72.7 | 13.8 ms | 360 (5 × 72) |
| `Lynxer-160s` | sign | 3.82 G | 1.83 s | 0.546 | 1.83 s | 100 (5 × 20) |
| `Lynxer-160s` | verify | 3.73 G | 1.79 s | 0.558 | 1.79 s | 100 (5 × 20) |
| `Lynxer-256f` | keygen | 174.01 M | 83.1 ms | 12 | 83.1 ms | 100 (5 × 20) |
| `Lynxer-256f` | sign | 1.55 G | 742 ms | 1.35 | 740 ms | 100 (5 × 20) |
| `Lynxer-256f` | verify | 1.18 G | 566 ms | 1.77 | 566 ms | 100 (5 × 20) |
| `Lynxer-256s` | keygen | 174.00 M | 83.1 ms | 12 | 83.1 ms | 100 (5 × 20) |
| `Lynxer-256s` | sign | 10.78 G | 5.17 s | 0.193 | 5.17 s | 100 (5 × 20) |
| `Lynxer-256s` | verify | 10.37 G | 4.97 s | 0.201 | 4.97 s | 100 (5 × 20) |
| `Lynxer-384f` | keygen | 741.32 M | 356 ms | 2.81 | 354 ms | 100 (5 × 20) |
| `Lynxer-384f` | sign | 5.16 G | 2.48 s | 0.403 | 2.48 s | 100 (5 × 20) |
| `Lynxer-384f` | verify | 3.64 G | 1.74 s | 0.574 | 1.74 s | 100 (5 × 20) |
| `Lynxer-384s` | keygen | 740.93 M | 357 ms | 2.8 | 354 ms | 100 (5 × 20) |
| `Lynxer-384s` | sign | 30.11 G | 14.4 s | 0.0693 | 14.4 s | 60 (5 × 12) |
| `Lynxer-384s` | verify | 28.34 G | 13.6 s | 0.0736 | 13.6 s | 65 (5 × 13) |
| `Lynxer-512f` | keygen | 2.15 G | 1.03 s | 0.967 | 1.04 s | 100 (5 × 20) |
| `Lynxer-512f` | sign | 6.77 G | 3.24 s | 0.309 | 3.24 s | 100 (5 × 20) |
| `Lynxer-512f` | verify | 2.48 G | 1.19 s | 0.842 | 1.19 s | 100 (5 × 20) |
| `Lynxer-512s` | keygen | 2.15 G | 1.03 s | 0.967 | 1.04 s | 100 (5 × 20) |
| `Lynxer-512s` | sign | 9.33 G | 4.46 s | 0.224 | 4.46 s | 100 (5 × 20) |
| `Lynxer-512s` | verify | 4.74 G | 2.28 s | 0.439 | 2.28 s | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Lynxer-160f` | keygen | 487477 | 1840 KiB | 2188 KiB |
| `Lynxer-160f` | sign | 487477 | 2056 KiB | 2588 KiB |
| `Lynxer-160f` | verify | 487477 | 2320 KiB | 2504 KiB |
| `Lynxer-160s` | keygen | 487477 | 1848 KiB | 2196 KiB |
| `Lynxer-160s` | sign | 487477 | 2048 KiB | 5468 KiB |
| `Lynxer-160s` | verify | 487477 | 2544 KiB | 5096 KiB |
| `Lynxer-256f` | keygen | 487493 | 1852 KiB | 2224 KiB |
| `Lynxer-256f` | sign | 487493 | 2144 KiB | 3132 KiB |
| `Lynxer-256f` | verify | 487493 | 2336 KiB | 2812 KiB |
| `Lynxer-256s` | keygen | 487493 | 1844 KiB | 2264 KiB |
| `Lynxer-256s` | sign | 487493 | 2144 KiB | 11292 KiB |
| `Lynxer-256s` | verify | 487493 | 3196 KiB | 11136 KiB |
| `Lynxer-384f` | keygen | 487493 | 1872 KiB | 2360 KiB |
| `Lynxer-384f` | sign | 487493 | 2328 KiB | 4404 KiB |
| `Lynxer-384f` | verify | 487493 | 2544 KiB | 4104 KiB |
| `Lynxer-384s` | keygen | 487493 | 1808 KiB | 2348 KiB |
| `Lynxer-384s` | sign | 487493 | 2232 KiB | 19724 KiB |
| `Lynxer-384s` | verify | 487493 | 3324 KiB | 19436 KiB |
| `Lynxer-512f` | keygen | 487517 | 1904 KiB | 2392 KiB |
| `Lynxer-512f` | sign | 487517 | 2272 KiB | 5912 KiB |
| `Lynxer-512f` | verify | 487517 | 2508 KiB | 5628 KiB |
| `Lynxer-512s` | keygen | 487517 | 1876 KiB | 2380 KiB |
| `Lynxer-512s` | sign | 487517 | 2264 KiB | 33216 KiB |
| `Lynxer-512s` | verify | 487517 | 3684 KiB | 33032 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `Lynxer-160f` | 40 | 40 | 5801 |
| `Lynxer-160s` | 40 | 40 | 4607 |
| `Lynxer-256f` | 64 | 64 | 15097 |
| `Lynxer-256s` | 64 | 64 | 12191 |
| `Lynxer-384f` | 96 | 96 | 34109 |
| `Lynxer-384s` | 96 | 96 | 27495 |
| `Lynxer-512f` | 128 | 128 | 61091 |
| `Lynxer-512s` | 128 | 128 | 48879 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **mixed** — with -DXOF_PSEUDO (as the KATs need): oracles via pseudoXOF, PRG via AES/Rijndael/SHACAL-2; source default is own Keccak (bypass)

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Lynxer-160f` | keygen | 80% | 0.0% | drng 2, pseudoXOF 62 |
| `Lynxer-160f` | sign | 14% | 0.0% | drng 1, pseudoXOF 412 |
| `Lynxer-160f` | verify | 6.2% | 0.0% | pseudoXOF 88 |
| `Lynxer-160s` | keygen | 80% | 0.0% | drng 2, pseudoXOF 62 |
| `Lynxer-160s` | sign | 3.4% | 0.0% | drng 1, pseudoXOF 3.64e+03 |
| `Lynxer-160s` | verify | 1.9% | 0.0% | pseudoXOF 81 |
| `Lynxer-256f` | keygen | 99% | 0.0% | drng 2, pseudoXOF 123 |
| `Lynxer-256f` | sign | 35% | 0.0% | drng 1, pseudoXOF 525 |
| `Lynxer-256f` | verify | 16% | 0.0% | pseudoXOF 163 |
| `Lynxer-256s` | keygen | 99% | 0.0% | drng 2, pseudoXOF 123 |
| `Lynxer-256s` | sign | 6.3% | 0.0% | drng 1, pseudoXOF 6.39e+03 |
| `Lynxer-256s` | verify | 3.0% | 0.0% | pseudoXOF 150 |
| `Lynxer-384f` | keygen | 99% | 0.0% | drng 2, pseudoXOF 255 |
| `Lynxer-384f` | sign | 44% | 0.0% | drng 1, pseudoXOF 971 |
| `Lynxer-384f` | verify | 22% | 0.0% | pseudoXOF 313 |
| `Lynxer-384s` | keygen | 99% | 0.0% | drng 2, pseudoXOF 255 |
| `Lynxer-384s` | sign | 10% | 0.0% | drng 1, pseudoXOF 2.73e+04 |
| `Lynxer-384s` | verify | 4.1% | 0.0% | pseudoXOF 294 |
| `Lynxer-512f` | keygen | 99% | 0.0% | drng 2, pseudoXOF 435 |
| `Lynxer-512f` | sign | 96% | 0.0% | drng 1, pseudoXOF 1.55e+03 |
| `Lynxer-512f` | verify | 91% | 0.0% | pseudoXOF 512 |
| `Lynxer-512s` | keygen | 99% | 0.0% | drng 2, pseudoXOF 435 |
| `Lynxer-512s` | sign | 83% | 0.0% | drng 1, pseudoXOF 1.21e+04 |
| `Lynxer-512s` | verify | 67% | 0.0% | pseudoXOF 486 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Lynxer-160f` | KAT log (sha256 `155689537809cc7c…`) | `kat/sign-14/Lynxer-160f.log` |
| `Lynxer-160f` | timing keygen | `records/sign-14/Lynxer-160f__keygen.json` |
| `Lynxer-160f` | timing sign | `records/sign-14/Lynxer-160f__sign.json` |
| `Lynxer-160f` | timing verify | `records/sign-14/Lynxer-160f__verify.json` |
| `Lynxer-160f` | hash profile keygen | `profile/sign-14/Lynxer-160f__keygen.json` |
| `Lynxer-160f` | hash profile sign | `profile/sign-14/Lynxer-160f__sign.json` |
| `Lynxer-160f` | hash profile verify | `profile/sign-14/Lynxer-160f__verify.json` |
| `Lynxer-160s` | KAT log (sha256 `a0fa2aa9e4c49620…`) | `kat/sign-14/Lynxer-160s.log` |
| `Lynxer-160s` | timing keygen | `records/sign-14/Lynxer-160s__keygen.json` |
| `Lynxer-160s` | timing sign | `records/sign-14/Lynxer-160s__sign.json` |
| `Lynxer-160s` | timing verify | `records/sign-14/Lynxer-160s__verify.json` |
| `Lynxer-160s` | hash profile keygen | `profile/sign-14/Lynxer-160s__keygen.json` |
| `Lynxer-160s` | hash profile sign | `profile/sign-14/Lynxer-160s__sign.json` |
| `Lynxer-160s` | hash profile verify | `profile/sign-14/Lynxer-160s__verify.json` |
| `Lynxer-256f` | KAT log (sha256 `a160418c14a7444c…`) | `kat/sign-14/Lynxer-256f.log` |
| `Lynxer-256f` | timing keygen | `records/sign-14/Lynxer-256f__keygen.json` |
| `Lynxer-256f` | timing sign | `records/sign-14/Lynxer-256f__sign.json` |
| `Lynxer-256f` | timing verify | `records/sign-14/Lynxer-256f__verify.json` |
| `Lynxer-256f` | hash profile keygen | `profile/sign-14/Lynxer-256f__keygen.json` |
| `Lynxer-256f` | hash profile sign | `profile/sign-14/Lynxer-256f__sign.json` |
| `Lynxer-256f` | hash profile verify | `profile/sign-14/Lynxer-256f__verify.json` |
| `Lynxer-256s` | KAT log (sha256 `d654147d984c36cb…`) | `kat/sign-14/Lynxer-256s.log` |
| `Lynxer-256s` | timing keygen | `records/sign-14/Lynxer-256s__keygen.json` |
| `Lynxer-256s` | timing sign | `records/sign-14/Lynxer-256s__sign.json` |
| `Lynxer-256s` | timing verify | `records/sign-14/Lynxer-256s__verify.json` |
| `Lynxer-256s` | hash profile keygen | `profile/sign-14/Lynxer-256s__keygen.json` |
| `Lynxer-256s` | hash profile sign | `profile/sign-14/Lynxer-256s__sign.json` |
| `Lynxer-256s` | hash profile verify | `profile/sign-14/Lynxer-256s__verify.json` |
| `Lynxer-384f` | KAT log (sha256 `4eae1f88b4141b00…`) | `kat/sign-14/Lynxer-384f.log` |
| `Lynxer-384f` | timing keygen | `records/sign-14/Lynxer-384f__keygen.json` |
| `Lynxer-384f` | timing sign | `records/sign-14/Lynxer-384f__sign.json` |
| `Lynxer-384f` | timing verify | `records/sign-14/Lynxer-384f__verify.json` |
| `Lynxer-384f` | hash profile keygen | `profile/sign-14/Lynxer-384f__keygen.json` |
| `Lynxer-384f` | hash profile sign | `profile/sign-14/Lynxer-384f__sign.json` |
| `Lynxer-384f` | hash profile verify | `profile/sign-14/Lynxer-384f__verify.json` |
| `Lynxer-384s` | KAT log (sha256 `9cd82d176108f1cd…`) | `kat/sign-14/Lynxer-384s.log` |
| `Lynxer-384s` | timing keygen | `records/sign-14/Lynxer-384s__keygen.json` |
| `Lynxer-384s` | timing sign | `records/sign-14/Lynxer-384s__sign.json` |
| `Lynxer-384s` | timing verify | `records/sign-14/Lynxer-384s__verify.json` |
| `Lynxer-384s` | hash profile keygen | `profile/sign-14/Lynxer-384s__keygen.json` |
| `Lynxer-384s` | hash profile sign | `profile/sign-14/Lynxer-384s__sign.json` |
| `Lynxer-384s` | hash profile verify | `profile/sign-14/Lynxer-384s__verify.json` |
| `Lynxer-512f` | KAT log (sha256 `f6baa4b8fa22180b…`) | `kat/sign-14/Lynxer-512f.log` |
| `Lynxer-512f` | timing keygen | `records/sign-14/Lynxer-512f__keygen.json` |
| `Lynxer-512f` | timing sign | `records/sign-14/Lynxer-512f__sign.json` |
| `Lynxer-512f` | timing verify | `records/sign-14/Lynxer-512f__verify.json` |
| `Lynxer-512f` | hash profile keygen | `profile/sign-14/Lynxer-512f__keygen.json` |
| `Lynxer-512f` | hash profile sign | `profile/sign-14/Lynxer-512f__sign.json` |
| `Lynxer-512f` | hash profile verify | `profile/sign-14/Lynxer-512f__verify.json` |
| `Lynxer-512s` | KAT log (sha256 `32fd34fb1cb962d7…`) | `kat/sign-14/Lynxer-512s.log` |
| `Lynxer-512s` | timing keygen | `records/sign-14/Lynxer-512s__keygen.json` |
| `Lynxer-512s` | timing sign | `records/sign-14/Lynxer-512s__sign.json` |
| `Lynxer-512s` | timing verify | `records/sign-14/Lynxer-512s__verify.json` |
| `Lynxer-512s` | hash profile keygen | `profile/sign-14/Lynxer-512s__keygen.json` |
| `Lynxer-512s` | hash profile sign | `profile/sign-14/Lynxer-512s__sign.json` |
| `Lynxer-512s` | hash profile verify | `profile/sign-14/Lynxer-512s__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

