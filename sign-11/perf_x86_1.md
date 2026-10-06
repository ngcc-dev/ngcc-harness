# sign-11 FlexTree — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-11` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561077587857408.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: FlexTree
- Implementation versions measured: reference
- Parameter sets: `Flextree-160f`, `Flextree-160s`, `Flextree-256f`, `Flextree-256s`, `Flextree-384f`, `Flextree-384s`, `Flextree-512f`, `Flextree-512s`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-11/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Flextree-160f` | guide | PASS |
| `Flextree-160s` | guide | PASS |
| `Flextree-256f` | guide | PASS |
| `Flextree-256s` | guide | PASS |
| `Flextree-384f` | guide | PASS |
| `Flextree-384s` | guide | PASS |
| `Flextree-512f` | guide | PASS |
| `Flextree-512s` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Flextree-160f` | keygen | 14.52 M | 6.94 ms | 144 | 6.93 ms | 720 (5 × 144) |
| `Flextree-160f` | sign | 563.57 M | 271 ms | 3.7 | 269 ms | 100 (5 × 20) |
| `Flextree-160f` | verify | 15.46 M | 7.38 ms | 135 | 7.38 ms | 680 (5 × 136) |
| `Flextree-160s` | keygen | 593.29 M | 285 ms | 3.51 | 283 ms | 100 (5 × 20) |
| `Flextree-160s` | sign | 8.29 G | 3.98 s | 0.251 | 3.98 s | 100 (5 × 20) |
| `Flextree-160s` | verify | 21.34 M | 10.2 ms | 98.1 | 10.2 ms | 485 (5 × 97) |
| `Flextree-256f` | keygen | 46.80 M | 22.3 ms | 44.7 | 22.3 ms | 225 (5 × 45) |
| `Flextree-256f` | sign | 975.63 M | 467 ms | 2.14 | 466 ms | 100 (5 × 20) |
| `Flextree-256f` | verify | 25.49 M | 12.2 ms | 82.1 | 12.2 ms | 410 (5 × 82) |
| `Flextree-256s` | keygen | 488.82 M | 235 ms | 4.26 | 233 ms | 100 (5 × 20) |
| `Flextree-256s` | sign | 9.24 G | 4.43 s | 0.226 | 4.43 s | 100 (5 × 20) |
| `Flextree-256s` | verify | 39.41 M | 18.8 ms | 53.1 | 18.8 ms | 265 (5 × 53) |
| `Flextree-384f` | keygen | 652.08 M | 313 ms | 3.2 | 312 ms | 100 (5 × 20) |
| `Flextree-384f` | sign | 12.33 G | 5.91 s | 0.169 | 5.91 s | 100 (5 × 20) |
| `Flextree-384f` | verify | 125.64 M | 60 ms | 16.7 | 60 ms | 100 (5 × 20) |
| `Flextree-384s` | keygen | 3.27 G | 1.57 s | 0.638 | 1.57 s | 100 (5 × 20) |
| `Flextree-384s` | sign | 44.66 G | 21.4 s | 0.0467 | 21.4 s | 40 (5 × 8) |
| `Flextree-384s` | verify | 53.95 M | 25.8 ms | 38.8 | 25.8 ms | 195 (5 × 39) |
| `Flextree-512f` | keygen | 1.76 G | 845 ms | 1.18 | 847 ms | 100 (5 × 20) |
| `Flextree-512f` | sign | 24.80 G | 11.9 s | 0.0841 | 11.9 s | 75 (5 × 15) |
| `Flextree-512f` | verify | 158.24 M | 75.6 ms | 13.2 | 75.6 ms | 100 (5 × 20) |
| `Flextree-512s` | keygen | 6.99 G | 3.34 s | 0.299 | 3.34 s | 100 (5 × 20) |
| `Flextree-512s` | sign | 84.54 G | 40.5 s | 0.0247 | 40.6 s | 20 (5 × 4) |
| `Flextree-512s` | verify | 117.82 M | 56.3 ms | 17.8 | 56.3 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Flextree-160f` | keygen | 3183557 | 1776 KiB | 1924 KiB |
| `Flextree-160f` | sign | 3183557 | 1824 KiB | 5048 KiB |
| `Flextree-160f` | verify | 3183557 | 4804 KiB | 4880 KiB |
| `Flextree-160s` | keygen | 1824077 | 1764 KiB | 1892 KiB |
| `Flextree-160s` | sign | 1824077 | 1804 KiB | 5192 KiB |
| `Flextree-160s` | verify | 1824077 | 3468 KiB | 4948 KiB |
| `Flextree-256f` | keygen | 3125621 | 1828 KiB | 1960 KiB |
| `Flextree-256f` | sign | 3125621 | 1832 KiB | 5924 KiB |
| `Flextree-256f` | verify | 3125621 | 4776 KiB | 5560 KiB |
| `Flextree-256s` | keygen | 14750069 | 1804 KiB | 1880 KiB |
| `Flextree-256s` | sign | 14750069 | 1816 KiB | 17564 KiB |
| `Flextree-256s` | verify | 14750069 | 8820 KiB | 13708 KiB |
| `Flextree-384f` | keygen | 5295637 | 1832 KiB | 1968 KiB |
| `Flextree-384f` | sign | 5295637 | 1880 KiB | 13756 KiB |
| `Flextree-384f` | verify | 5295637 | 6904 KiB | 13464 KiB |
| `Flextree-384s` | keygen | 4422389 | 1816 KiB | 1968 KiB |
| `Flextree-384s` | sign | 4422389 | 1856 KiB | 29644 KiB |
| `Flextree-384s` | verify | 4422389 | 6008 KiB | 29364 KiB |
| `Flextree-512f` | keygen | 8129405 | 1904 KiB | 2000 KiB |
| `Flextree-512f` | sign | 8129405 | 1916 KiB | 32580 KiB |
| `Flextree-512f` | verify | 8129405 | 9700 KiB | 32312 KiB |
| `Flextree-512s` | keygen | 7445541 | 1864 KiB | 2036 KiB |
| `Flextree-512s` | sign | 7445541 | 1940 KiB | 62672 KiB |
| `Flextree-512s` | verify | 7445541 | 8940 KiB | 62456 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `Flextree-160f` | 40 | 80 | 18672 |
| `Flextree-160s` | 40 | 80 | 9580 |
| `Flextree-256f` | 64 | 128 | 46856 |
| `Flextree-256s` | 64 | 128 | 25420 |
| `Flextree-384f` | 96 | 192 | 73396 |
| `Flextree-384s` | 96 | 192 | 60180 |
| `Flextree-512f` | 128 | 256 | 117296 |
| `Flextree-512s` | 128 | 256 | 94948 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Flextree-160f` | keygen | 98% | 0.0% | drng 1, sm3hash 5.07e+03 |
| `Flextree-160f` | sign | 92% | 0.0% | drng 1, pseudoXOF 1.63e+03, sm3hash 1.84e+05 |
| `Flextree-160f` | verify | 98% | 0.0% | pseudoXOF 2, sm3hash 5.34e+03 |
| `Flextree-160s` | keygen | 98% | 0.0% | drng 1, sm3hash 2.09e+05 |
| `Flextree-160s` | sign | 98% | 0.0% | drng 1, pseudoXOF 2.33, sm3hash 2.94e+06 |
| `Flextree-160s` | verify | 98% | 0.0% | pseudoXOF 2, sm3hash 7.49e+03 |
| `Flextree-256f` | keygen | 98% | 0.0% | drng 1, sm3hash 1.63e+04 |
| `Flextree-256f` | sign | 98% | 0.0% | drng 1, pseudoXOF 13.7, sm3hash 3.3e+05 |
| `Flextree-256f` | verify | 98% | 0.0% | pseudoXOF 2, sm3hash 8.53e+03 |
| `Flextree-256s` | keygen | 98% | 0.0% | drng 1, sm3hash 1.72e+05 |
| `Flextree-256s` | sign | 98% | 0.0% | drng 1, pseudoXOF 2, sm3hash 3.21e+06 |
| `Flextree-256s` | verify | 98% | 0.0% | pseudoXOF 2, sm3hash 1.36e+04 |
| `Flextree-384f` | keygen | 99% | 0.0% | drng 1, pseudoXOF 7.74e+04 |
| `Flextree-384f` | sign | 99% | 0.0% | drng 1, pseudoXOF 1.46e+06 |
| `Flextree-384f` | verify | 99% | 0.0% | pseudoXOF 1.47e+04 |
| `Flextree-384s` | keygen | 99% | 0.0% | drng 1, pseudoXOF 3.86e+05 |
| `Flextree-384s` | sign | 99% | 0.0% | drng 1, pseudoXOF 5.34e+06 |
| `Flextree-384s` | verify | 98% | 0.0% | pseudoXOF 6.18e+03 |
| `Flextree-512f` | keygen | 99% | 0.0% | drng 1, pseudoXOF 2.08e+05 |
| `Flextree-512f` | sign | 99% | 0.0% | drng 1, pseudoXOF 2.85e+06 |
| `Flextree-512f` | verify | 98% | 0.0% | pseudoXOF 1.81e+04 |
| `Flextree-512s` | keygen | 99% | 0.0% | drng 1, pseudoXOF 8.28e+05 |
| `Flextree-512s` | sign | 99% | 0.0% | drng 1, pseudoXOF 9.81e+06 |
| `Flextree-512s` | verify | 97% | 0.0% | pseudoXOF 1.33e+04 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Flextree-160f` | KAT log (sha256 `74f0d60b6353c8f5…`) | `kat/sign-11/Flextree-160f.log` |
| `Flextree-160f` | timing keygen | `records/sign-11/Flextree-160f__keygen.json` |
| `Flextree-160f` | timing sign | `records/sign-11/Flextree-160f__sign.json` |
| `Flextree-160f` | timing verify | `records/sign-11/Flextree-160f__verify.json` |
| `Flextree-160f` | hash profile keygen | `profile/sign-11/Flextree-160f__keygen.json` |
| `Flextree-160f` | hash profile sign | `profile/sign-11/Flextree-160f__sign.json` |
| `Flextree-160f` | hash profile verify | `profile/sign-11/Flextree-160f__verify.json` |
| `Flextree-160s` | KAT log (sha256 `075d9be10b6ad44a…`) | `kat/sign-11/Flextree-160s.log` |
| `Flextree-160s` | timing keygen | `records/sign-11/Flextree-160s__keygen.json` |
| `Flextree-160s` | timing sign | `records/sign-11/Flextree-160s__sign.json` |
| `Flextree-160s` | timing verify | `records/sign-11/Flextree-160s__verify.json` |
| `Flextree-160s` | hash profile keygen | `profile/sign-11/Flextree-160s__keygen.json` |
| `Flextree-160s` | hash profile sign | `profile/sign-11/Flextree-160s__sign.json` |
| `Flextree-160s` | hash profile verify | `profile/sign-11/Flextree-160s__verify.json` |
| `Flextree-256f` | KAT log (sha256 `dd1de7295db23c34…`) | `kat/sign-11/Flextree-256f.log` |
| `Flextree-256f` | timing keygen | `records/sign-11/Flextree-256f__keygen.json` |
| `Flextree-256f` | timing sign | `records/sign-11/Flextree-256f__sign.json` |
| `Flextree-256f` | timing verify | `records/sign-11/Flextree-256f__verify.json` |
| `Flextree-256f` | hash profile keygen | `profile/sign-11/Flextree-256f__keygen.json` |
| `Flextree-256f` | hash profile sign | `profile/sign-11/Flextree-256f__sign.json` |
| `Flextree-256f` | hash profile verify | `profile/sign-11/Flextree-256f__verify.json` |
| `Flextree-256s` | KAT log (sha256 `4a18b4c686843e97…`) | `kat/sign-11/Flextree-256s.log` |
| `Flextree-256s` | timing keygen | `records/sign-11/Flextree-256s__keygen.json` |
| `Flextree-256s` | timing sign | `records/sign-11/Flextree-256s__sign.json` |
| `Flextree-256s` | timing verify | `records/sign-11/Flextree-256s__verify.json` |
| `Flextree-256s` | hash profile keygen | `profile/sign-11/Flextree-256s__keygen.json` |
| `Flextree-256s` | hash profile sign | `profile/sign-11/Flextree-256s__sign.json` |
| `Flextree-256s` | hash profile verify | `profile/sign-11/Flextree-256s__verify.json` |
| `Flextree-384f` | KAT log (sha256 `c9f93062f7987bc1…`) | `kat/sign-11/Flextree-384f.log` |
| `Flextree-384f` | timing keygen | `records/sign-11/Flextree-384f__keygen.json` |
| `Flextree-384f` | timing sign | `records/sign-11/Flextree-384f__sign.json` |
| `Flextree-384f` | timing verify | `records/sign-11/Flextree-384f__verify.json` |
| `Flextree-384f` | hash profile keygen | `profile/sign-11/Flextree-384f__keygen.json` |
| `Flextree-384f` | hash profile sign | `profile/sign-11/Flextree-384f__sign.json` |
| `Flextree-384f` | hash profile verify | `profile/sign-11/Flextree-384f__verify.json` |
| `Flextree-384s` | KAT log (sha256 `334b3de3f505f797…`) | `kat/sign-11/Flextree-384s.log` |
| `Flextree-384s` | timing keygen | `records/sign-11/Flextree-384s__keygen.json` |
| `Flextree-384s` | timing sign | `records/sign-11/Flextree-384s__sign.json` |
| `Flextree-384s` | timing verify | `records/sign-11/Flextree-384s__verify.json` |
| `Flextree-384s` | hash profile keygen | `profile/sign-11/Flextree-384s__keygen.json` |
| `Flextree-384s` | hash profile sign | `profile/sign-11/Flextree-384s__sign.json` |
| `Flextree-384s` | hash profile verify | `profile/sign-11/Flextree-384s__verify.json` |
| `Flextree-512f` | KAT log (sha256 `c53d3ae276ac4309…`) | `kat/sign-11/Flextree-512f.log` |
| `Flextree-512f` | timing keygen | `records/sign-11/Flextree-512f__keygen.json` |
| `Flextree-512f` | timing sign | `records/sign-11/Flextree-512f__sign.json` |
| `Flextree-512f` | timing verify | `records/sign-11/Flextree-512f__verify.json` |
| `Flextree-512f` | hash profile keygen | `profile/sign-11/Flextree-512f__keygen.json` |
| `Flextree-512f` | hash profile sign | `profile/sign-11/Flextree-512f__sign.json` |
| `Flextree-512f` | hash profile verify | `profile/sign-11/Flextree-512f__verify.json` |
| `Flextree-512s` | KAT log (sha256 `22debfad129b65e3…`) | `kat/sign-11/Flextree-512s.log` |
| `Flextree-512s` | timing keygen | `records/sign-11/Flextree-512s__keygen.json` |
| `Flextree-512s` | timing sign | `records/sign-11/Flextree-512s__sign.json` |
| `Flextree-512s` | timing verify | `records/sign-11/Flextree-512s__verify.json` |
| `Flextree-512s` | hash profile keygen | `profile/sign-11/Flextree-512s__keygen.json` |
| `Flextree-512s` | hash profile sign | `profile/sign-11/Flextree-512s__sign.json` |
| `Flextree-512s` | hash profile verify | `profile/sign-11/Flextree-512s__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

