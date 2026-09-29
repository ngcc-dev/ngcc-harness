# sign-21 ReSolveD-ɑ — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-21` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561087368974336.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: ReSolveD-ɑ
- Implementation versions measured: reference
- Parameter sets: `ReSolveD-alpha-160f`, `ReSolveD-alpha-160s`, `ReSolveD-alpha-256f`, `ReSolveD-alpha-256s`, `ReSolveD-alpha-384f`, `ReSolveD-alpha-384s`, `ReSolveD-alpha-512f`, `ReSolveD-alpha-512s`

## 2. Assessment environment

| item | value |
|---|---|
| processor | Qualcomm Oryon (CPU 2, one core) |
| machine | ASUS Vivobook S 15 |
| clock | fixed 2.71 GHz (governor performance, minimum = maximum), boost off, SMT none |
| memory | 30562 MiB |
| OS / kernel | Ubuntu 26.04.1 LTS / 7.0.0-34-generic |
| compiler / build tool | gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0 / cmake version 4.2.3 |
| campaign start / end (UTC) | 2026-09-28T15:05:46 / 2026-09-29T02:47:54 |

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
| `ReSolveD-alpha-160f` | keygen | 11.70 M | 4.34 ms | 230 | 4.34 ms | 715 (5 × 143) |
| `ReSolveD-alpha-160f` | sign | 454.96 M | 169 ms | 5.92 | 168 ms | 100 (5 × 20) |
| `ReSolveD-alpha-160f` | verify | 429.40 M | 159 ms | 6.28 | 159 ms | 100 (5 × 20) |
| `ReSolveD-alpha-160s` | keygen | 11.75 M | 4.36 ms | 229 | 4.34 ms | 725 (5 × 145) |
| `ReSolveD-alpha-160s` | sign | 3.50 G | 1.3 s | 0.77 | 1.3 s | 100 (5 × 20) |
| `ReSolveD-alpha-160s` | verify | 3.47 G | 1.29 s | 0.778 | 1.29 s | 100 (5 × 20) |
| `ReSolveD-alpha-256f` | keygen | 27.35 M | 10.2 ms | 98.5 | 10.1 ms | 305 (5 × 61) |
| `ReSolveD-alpha-256f` | sign | 1.07 G | 397 ms | 2.52 | 397 ms | 100 (5 × 20) |
| `ReSolveD-alpha-256f` | verify | 1.02 G | 378 ms | 2.65 | 378 ms | 100 (5 × 20) |
| `ReSolveD-alpha-256s` | keygen | 27.42 M | 10.2 ms | 98.3 | 10.1 ms | 305 (5 × 61) |
| `ReSolveD-alpha-256s` | sign | 9.99 G | 3.71 s | 0.27 | 3.71 s | 100 (5 × 20) |
| `ReSolveD-alpha-256s` | verify | 9.90 G | 3.67 s | 0.272 | 3.67 s | 100 (5 × 20) |
| `ReSolveD-alpha-384f` | keygen | 68.50 M | 25.4 ms | 39.3 | 25.4 ms | 125 (5 × 25) |
| `ReSolveD-alpha-384f` | sign | 3.05 G | 1.13 s | 0.884 | 1.13 s | 100 (5 × 20) |
| `ReSolveD-alpha-384f` | verify | 2.91 G | 1.08 s | 0.928 | 1.08 s | 100 (5 × 20) |
| `ReSolveD-alpha-384s` | keygen | 68.54 M | 25.4 ms | 39.3 | 25.4 ms | 125 (5 × 25) |
| `ReSolveD-alpha-384s` | sign | 27.07 G | 10 s | 0.0996 | 10 s | 35 (5 × 7) |
| `ReSolveD-alpha-384s` | verify | 26.86 G | 9.97 s | 0.1 | 9.96 s | 35 (5 × 7) |
| `ReSolveD-alpha-512f` | keygen | 120.91 M | 44.9 ms | 22.3 | 44.9 ms | 100 (5 × 20) |
| `ReSolveD-alpha-512f` | sign | 721.69 M | 268 ms | 3.73 | 268 ms | 100 (5 × 20) |
| `ReSolveD-alpha-512f` | verify | 577.76 M | 214 ms | 4.66 | 214 ms | 100 (5 × 20) |
| `ReSolveD-alpha-512s` | keygen | 120.91 M | 44.9 ms | 22.3 | 44.9 ms | 100 (5 × 20) |
| `ReSolveD-alpha-512s` | sign | 3.67 G | 1.36 s | 0.733 | 1.36 s | 100 (5 × 20) |
| `ReSolveD-alpha-512s` | verify | 3.16 G | 1.17 s | 0.853 | 1.17 s | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `ReSolveD-alpha-160f` | keygen | 115832 | 1512 KiB | 1888 KiB |
| `ReSolveD-alpha-160f` | sign | 115832 | 3724 KiB | 4364 KiB |
| `ReSolveD-alpha-160f` | verify | 115832 | 2284 KiB | 2348 KiB |
| `ReSolveD-alpha-160s` | keygen | 115832 | 1512 KiB | 3916 KiB |
| `ReSolveD-alpha-160s` | sign | 115832 | 3644 KiB | 7092 KiB |
| `ReSolveD-alpha-160s` | verify | 115832 | 3644 KiB | 6916 KiB |
| `ReSolveD-alpha-256f` | keygen | 115836 | 1524 KiB | 4220 KiB |
| `ReSolveD-alpha-256f` | sign | 115836 | 2320 KiB | 5492 KiB |
| `ReSolveD-alpha-256f` | verify | 115836 | 3752 KiB | 4188 KiB |
| `ReSolveD-alpha-256s` | keygen | 115836 | 1520 KiB | 2388 KiB |
| `ReSolveD-alpha-256s` | sign | 115836 | 2316 KiB | 13340 KiB |
| `ReSolveD-alpha-256s` | verify | 115836 | 5228 KiB | 12684 KiB |
| `ReSolveD-alpha-384f` | keygen | 115872 | 1544 KiB | 5364 KiB |
| `ReSolveD-alpha-384f` | sign | 115872 | 3656 KiB | 7480 KiB |
| `ReSolveD-alpha-384f` | verify | 115872 | 3936 KiB | 7116 KiB |
| `ReSolveD-alpha-384s` | keygen | 115872 | 1536 KiB | 4480 KiB |
| `ReSolveD-alpha-384s` | sign | 115872 | 3312 KiB | 21944 KiB |
| `ReSolveD-alpha-384s` | verify | 115872 | 4460 KiB | 21472 KiB |
| `ReSolveD-alpha-512f` | keygen | 115872 | 1576 KiB | 6744 KiB |
| `ReSolveD-alpha-512f` | sign | 115872 | 3632 KiB | 10036 KiB |
| `ReSolveD-alpha-512f` | verify | 115872 | 4456 KiB | 9640 KiB |
| `ReSolveD-alpha-512s` | keygen | 115872 | 1560 KiB | 6472 KiB |
| `ReSolveD-alpha-512s` | sign | 115872 | 2612 KiB | 36840 KiB |
| `ReSolveD-alpha-512s` | verify | 115872 | 6020 KiB | 37076 KiB |

## 6. Transmission and storage overhead

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
| `ReSolveD-alpha-160f` | keygen | 39% | 0.1% | drng 2, pseudoXOF 1 |
| `ReSolveD-alpha-160f` | sign | 3.6% | 0.0% | drng 1, pseudoXOF 351 |
| `ReSolveD-alpha-160f` | verify | 2.5% | 0.0% | pseudoXOF 27 |
| `ReSolveD-alpha-160s` | keygen | 39% | 0.1% | drng 2, pseudoXOF 1 |
| `ReSolveD-alpha-160s` | sign | 1.8% | 0.0% | drng 1, pseudoXOF 1.9e+03 |
| `ReSolveD-alpha-160s` | verify | 1.5% | 0.0% | pseudoXOF 20 |
| `ReSolveD-alpha-256f` | keygen | 44% | 0.0% | drng 2, pseudoXOF 1 |
| `ReSolveD-alpha-256f` | sign | 3.7% | 0.0% | drng 1, pseudoXOF 346 |
| `ReSolveD-alpha-256f` | verify | 2.6% | 0.0% | pseudoXOF 41 |
| `ReSolveD-alpha-256s` | keygen | 44% | 0.0% | drng 2, pseudoXOF 1 |
| `ReSolveD-alpha-256s` | sign | 1.8% | 0.0% | drng 1, pseudoXOF 6.65e+03 |
| `ReSolveD-alpha-256s` | verify | 1.5% | 0.0% | pseudoXOF 28 |
| `ReSolveD-alpha-384f` | keygen | 39% | 0.0% | drng 2, pseudoXOF 1 |
| `ReSolveD-alpha-384f` | sign | 3.5% | 0.0% | drng 1, pseudoXOF 490 |
| `ReSolveD-alpha-384f` | verify | 2.6% | 0.0% | pseudoXOF 59 |
| `ReSolveD-alpha-384s` | keygen | 39% | 0.0% | drng 2, pseudoXOF 1 |
| `ReSolveD-alpha-384s` | sign | 2.0% | 0.0% | drng 1, pseudoXOF 5.78e+03 |
| `ReSolveD-alpha-384s` | verify | 1.7% | 0.0% | pseudoXOF 40 |
| `ReSolveD-alpha-512f` | keygen | 76% | 0.0% | drng 2, pseudoXOF 1 |
| `ReSolveD-alpha-512f` | sign | 41% | 0.0% | drng 1, pseudoXOF 310 |
| `ReSolveD-alpha-512f` | verify | 34% | 0.0% | pseudoXOF 78 |
| `ReSolveD-alpha-512s` | keygen | 76% | 0.0% | drng 2, pseudoXOF 1 |
| `ReSolveD-alpha-512s` | sign | 39% | 0.0% | drng 1, pseudoXOF 1.3e+04 |
| `ReSolveD-alpha-512s` | verify | 34% | 0.0% | pseudoXOF 52 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `ReSolveD-alpha-160f` | KAT log (sha256 `e4007e95a14a477b…`) | `kat/sign-21/ReSolveD-alpha-160f.log` |
| `ReSolveD-alpha-160f` | timing keygen | `records/sign-21/ReSolveD-alpha-160f__keygen.json` |
| `ReSolveD-alpha-160f` | timing sign | `records/sign-21/ReSolveD-alpha-160f__sign.json` |
| `ReSolveD-alpha-160f` | timing verify | `records/sign-21/ReSolveD-alpha-160f__verify.json` |
| `ReSolveD-alpha-160f` | hash profile keygen | `profile/sign-21/ReSolveD-alpha-160f__keygen.json` |
| `ReSolveD-alpha-160f` | hash profile sign | `profile/sign-21/ReSolveD-alpha-160f__sign.json` |
| `ReSolveD-alpha-160f` | hash profile verify | `profile/sign-21/ReSolveD-alpha-160f__verify.json` |
| `ReSolveD-alpha-160s` | KAT log (sha256 `ca47b2f0dd02c472…`) | `kat/sign-21/ReSolveD-alpha-160s.log` |
| `ReSolveD-alpha-160s` | timing keygen | `records/sign-21/ReSolveD-alpha-160s__keygen.json` |
| `ReSolveD-alpha-160s` | timing sign | `records/sign-21/ReSolveD-alpha-160s__sign.json` |
| `ReSolveD-alpha-160s` | timing verify | `records/sign-21/ReSolveD-alpha-160s__verify.json` |
| `ReSolveD-alpha-160s` | hash profile keygen | `profile/sign-21/ReSolveD-alpha-160s__keygen.json` |
| `ReSolveD-alpha-160s` | hash profile sign | `profile/sign-21/ReSolveD-alpha-160s__sign.json` |
| `ReSolveD-alpha-160s` | hash profile verify | `profile/sign-21/ReSolveD-alpha-160s__verify.json` |
| `ReSolveD-alpha-256f` | KAT log (sha256 `e34a2cf29f6508dc…`) | `kat/sign-21/ReSolveD-alpha-256f.log` |
| `ReSolveD-alpha-256f` | timing keygen | `records/sign-21/ReSolveD-alpha-256f__keygen.json` |
| `ReSolveD-alpha-256f` | timing sign | `records/sign-21/ReSolveD-alpha-256f__sign.json` |
| `ReSolveD-alpha-256f` | timing verify | `records/sign-21/ReSolveD-alpha-256f__verify.json` |
| `ReSolveD-alpha-256f` | hash profile keygen | `profile/sign-21/ReSolveD-alpha-256f__keygen.json` |
| `ReSolveD-alpha-256f` | hash profile sign | `profile/sign-21/ReSolveD-alpha-256f__sign.json` |
| `ReSolveD-alpha-256f` | hash profile verify | `profile/sign-21/ReSolveD-alpha-256f__verify.json` |
| `ReSolveD-alpha-256s` | KAT log (sha256 `ad37a415040b3779…`) | `kat/sign-21/ReSolveD-alpha-256s.log` |
| `ReSolveD-alpha-256s` | timing keygen | `records/sign-21/ReSolveD-alpha-256s__keygen.json` |
| `ReSolveD-alpha-256s` | timing sign | `records/sign-21/ReSolveD-alpha-256s__sign.json` |
| `ReSolveD-alpha-256s` | timing verify | `records/sign-21/ReSolveD-alpha-256s__verify.json` |
| `ReSolveD-alpha-256s` | hash profile keygen | `profile/sign-21/ReSolveD-alpha-256s__keygen.json` |
| `ReSolveD-alpha-256s` | hash profile sign | `profile/sign-21/ReSolveD-alpha-256s__sign.json` |
| `ReSolveD-alpha-256s` | hash profile verify | `profile/sign-21/ReSolveD-alpha-256s__verify.json` |
| `ReSolveD-alpha-384f` | KAT log (sha256 `b90b1fef8f7d7b33…`) | `kat/sign-21/ReSolveD-alpha-384f.log` |
| `ReSolveD-alpha-384f` | timing keygen | `records/sign-21/ReSolveD-alpha-384f__keygen.json` |
| `ReSolveD-alpha-384f` | timing sign | `records/sign-21/ReSolveD-alpha-384f__sign.json` |
| `ReSolveD-alpha-384f` | timing verify | `records/sign-21/ReSolveD-alpha-384f__verify.json` |
| `ReSolveD-alpha-384f` | hash profile keygen | `profile/sign-21/ReSolveD-alpha-384f__keygen.json` |
| `ReSolveD-alpha-384f` | hash profile sign | `profile/sign-21/ReSolveD-alpha-384f__sign.json` |
| `ReSolveD-alpha-384f` | hash profile verify | `profile/sign-21/ReSolveD-alpha-384f__verify.json` |
| `ReSolveD-alpha-384s` | KAT log (sha256 `5fe40ac34987e04c…`) | `kat/sign-21/ReSolveD-alpha-384s.log` |
| `ReSolveD-alpha-384s` | timing keygen | `records/sign-21/ReSolveD-alpha-384s__keygen.json` |
| `ReSolveD-alpha-384s` | timing sign | `records/sign-21/ReSolveD-alpha-384s__sign.json` |
| `ReSolveD-alpha-384s` | timing verify | `records/sign-21/ReSolveD-alpha-384s__verify.json` |
| `ReSolveD-alpha-384s` | hash profile keygen | `profile/sign-21/ReSolveD-alpha-384s__keygen.json` |
| `ReSolveD-alpha-384s` | hash profile sign | `profile/sign-21/ReSolveD-alpha-384s__sign.json` |
| `ReSolveD-alpha-384s` | hash profile verify | `profile/sign-21/ReSolveD-alpha-384s__verify.json` |
| `ReSolveD-alpha-512f` | KAT log (sha256 `2a4070d607ecf0f4…`) | `kat/sign-21/ReSolveD-alpha-512f.log` |
| `ReSolveD-alpha-512f` | timing keygen | `records/sign-21/ReSolveD-alpha-512f__keygen.json` |
| `ReSolveD-alpha-512f` | timing sign | `records/sign-21/ReSolveD-alpha-512f__sign.json` |
| `ReSolveD-alpha-512f` | timing verify | `records/sign-21/ReSolveD-alpha-512f__verify.json` |
| `ReSolveD-alpha-512f` | hash profile keygen | `profile/sign-21/ReSolveD-alpha-512f__keygen.json` |
| `ReSolveD-alpha-512f` | hash profile sign | `profile/sign-21/ReSolveD-alpha-512f__sign.json` |
| `ReSolveD-alpha-512f` | hash profile verify | `profile/sign-21/ReSolveD-alpha-512f__verify.json` |
| `ReSolveD-alpha-512s` | KAT log (sha256 `1dfe514199aae577…`) | `kat/sign-21/ReSolveD-alpha-512s.log` |
| `ReSolveD-alpha-512s` | timing keygen | `records/sign-21/ReSolveD-alpha-512s__keygen.json` |
| `ReSolveD-alpha-512s` | timing sign | `records/sign-21/ReSolveD-alpha-512s__sign.json` |
| `ReSolveD-alpha-512s` | timing verify | `records/sign-21/ReSolveD-alpha-512s__verify.json` |
| `ReSolveD-alpha-512s` | hash profile keygen | `profile/sign-21/ReSolveD-alpha-512s__keygen.json` |
| `ReSolveD-alpha-512s` | hash profile sign | `profile/sign-21/ReSolveD-alpha-512s__sign.json` |
| `ReSolveD-alpha-512s` | hash profile verify | `profile/sign-21/ReSolveD-alpha-512s__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

