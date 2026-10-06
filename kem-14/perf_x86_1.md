# kem-14 DTRU — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-14` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560844682350592.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: DTRU
- Implementation versions measured: reference
- Parameter sets: `DTRU-648`, `DTRU-648-PACK_PK`, `DTRU-768`, `DTRU-768-PACK_PK`, `DTRU-1024`, `DTRU-1024-PACK_PK`, `DTRU-1536`, `DTRU-1536-PACK_PK`, `DTRU-2048`, `DTRU-2048-PACK_PK`, `DTRU-Light`, `DTRU-Prime`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-14/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `DTRU-648` | guide | PASS |
| `DTRU-648-PACK_PK` | guide | PASS |
| `DTRU-768` | guide | PASS |
| `DTRU-768-PACK_PK` | guide | PASS |
| `DTRU-1024` | guide | PASS |
| `DTRU-1024-PACK_PK` | guide | PASS |
| `DTRU-1536` | guide | PASS |
| `DTRU-1536-PACK_PK` | guide | PASS |
| `DTRU-2048` | guide | PASS |
| `DTRU-2048-PACK_PK` | guide | PASS |
| `DTRU-Light` | guide | PASS |
| `DTRU-Prime` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `DTRU-648` | keygen | 334.5 k | 160 µs | 6.26e+03 | 160 µs | 29680 (5 × 5936) |
| `DTRU-648` | enc | 164.8 k | 78.7 µs | 1.27e+04 | 78.7 µs | 58130 (5 × 11626) |
| `DTRU-648` | dec | 323.2 k | 154 µs | 6.48e+03 | 154 µs | 31075 (5 × 6215) |
| `DTRU-648-PACK_PK` | keygen | 336.0 k | 161 µs | 6.23e+03 | 160 µs | 29015 (5 × 5803) |
| `DTRU-648-PACK_PK` | enc | 179.1 k | 85.6 µs | 1.17e+04 | 85.5 µs | 53795 (5 × 10759) |
| `DTRU-648-PACK_PK` | dec | 337.5 k | 161 µs | 6.2e+03 | 161 µs | 29515 (5 × 5903) |
| `DTRU-768` | keygen | 293.7 k | 140 µs | 7.13e+03 | 140 µs | 32985 (5 × 6597) |
| `DTRU-768` | enc | 196.9 k | 94.1 µs | 1.06e+04 | 94.1 µs | 49400 (5 × 9880) |
| `DTRU-768` | dec | 367.9 k | 176 µs | 5.69e+03 | 176 µs | 26860 (5 × 5372) |
| `DTRU-768-PACK_PK` | keygen | 298.4 k | 143 µs | 7.02e+03 | 143 µs | 31705 (5 × 6341) |
| `DTRU-768-PACK_PK` | enc | 213.9 k | 102 µs | 9.79e+03 | 102 µs | 45165 (5 × 9033) |
| `DTRU-768-PACK_PK` | dec | 385.9 k | 184 µs | 5.42e+03 | 184 µs | 24210 (5 × 4842) |
| `DTRU-1024` | keygen | 346.4 k | 165 µs | 6.04e+03 | 165 µs | 28130 (5 × 5626) |
| `DTRU-1024` | enc | 233.8 k | 112 µs | 8.95e+03 | 112 µs | 41405 (5 × 8281) |
| `DTRU-1024` | dec | 451.0 k | 215 µs | 4.64e+03 | 216 µs | 22000 (5 × 4400) |
| `DTRU-1024-PACK_PK` | keygen | 351.1 k | 168 µs | 5.96e+03 | 168 µs | 27485 (5 × 5497) |
| `DTRU-1024-PACK_PK` | enc | 254.5 k | 122 µs | 8.22e+03 | 122 µs | 37840 (5 × 7568) |
| `DTRU-1024-PACK_PK` | dec | 473.5 k | 226 µs | 4.42e+03 | 226 µs | 19945 (5 × 3989) |
| `DTRU-1536` | keygen | 460.0 k | 220 µs | 4.55e+03 | 220 µs | 21680 (5 × 4336) |
| `DTRU-1536` | enc | 385.8 k | 184 µs | 5.43e+03 | 184 µs | 25705 (5 × 5141) |
| `DTRU-1536` | dec | 731.8 k | 350 µs | 2.86e+03 | 350 µs | 13625 (5 × 2725) |
| `DTRU-1536-PACK_PK` | keygen | 479.3 k | 229 µs | 4.37e+03 | 230 µs | 20680 (5 × 4136) |
| `DTRU-1536-PACK_PK` | enc | 424.7 k | 203 µs | 4.93e+03 | 203 µs | 23630 (5 × 4726) |
| `DTRU-1536-PACK_PK` | dec | 776.0 k | 371 µs | 2.7e+03 | 371 µs | 12960 (5 × 2592) |
| `DTRU-2048` | keygen | 804.6 k | 384 µs | 2.6e+03 | 384 µs | 12475 (5 × 2495) |
| `DTRU-2048` | enc | 533.0 k | 255 µs | 3.93e+03 | 255 µs | 18935 (5 × 3787) |
| `DTRU-2048` | dec | 1.02 M | 489 µs | 2.05e+03 | 489 µs | 9785 (5 × 1957) |
| `DTRU-2048-PACK_PK` | keygen | 813.9 k | 389 µs | 2.57e+03 | 389 µs | 12080 (5 × 2416) |
| `DTRU-2048-PACK_PK` | enc | 579.0 k | 277 µs | 3.62e+03 | 277 µs | 17530 (5 × 3506) |
| `DTRU-2048-PACK_PK` | dec | 1.07 M | 511 µs | 1.96e+03 | 511 µs | 9500 (5 × 1900) |
| `DTRU-Light` | keygen | 119.3 k | 57 µs | 1.75e+04 | 57 µs | 75515 (5 × 15103) |
| `DTRU-Light` | enc | 97.8 k | 46.7 µs | 2.14e+04 | 46.7 µs | 93925 (5 × 18785) |
| `DTRU-Light` | dec | 192.0 k | 91.7 µs | 1.09e+04 | 91.7 µs | 50485 (5 × 10097) |
| `DTRU-Prime` | keygen | 113.74 M | 54.3 ms | 18.4 | 54.2 ms | 100 (5 × 20) |
| `DTRU-Prime` | enc | 335.8 k | 160 µs | 6.23e+03 | 161 µs | 29900 (5 × 5980) |
| `DTRU-Prime` | dec | 679.9 k | 325 µs | 3.08e+03 | 325 µs | 14870 (5 × 2974) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `DTRU-648` | keygen | 32149 | 1716 KiB | 1784 KiB |
| `DTRU-648` | enc | 32149 | 1684 KiB | 1808 KiB |
| `DTRU-648` | dec | 32149 | 1728 KiB | 1792 KiB |
| `DTRU-648-PACK_PK` | keygen | 33557 | 1704 KiB | 1808 KiB |
| `DTRU-648-PACK_PK` | enc | 33557 | 1728 KiB | 1816 KiB |
| `DTRU-648-PACK_PK` | dec | 33557 | 1712 KiB | 1824 KiB |
| `DTRU-768` | keygen | 30709 | 1692 KiB | 1760 KiB |
| `DTRU-768` | enc | 30709 | 1676 KiB | 1804 KiB |
| `DTRU-768` | dec | 30709 | 1688 KiB | 1812 KiB |
| `DTRU-768-PACK_PK` | keygen | 32125 | 1712 KiB | 1816 KiB |
| `DTRU-768-PACK_PK` | enc | 32125 | 1724 KiB | 1788 KiB |
| `DTRU-768-PACK_PK` | dec | 32125 | 1728 KiB | 1824 KiB |
| `DTRU-1024` | keygen | 32789 | 1704 KiB | 1776 KiB |
| `DTRU-1024` | enc | 32789 | 1716 KiB | 1784 KiB |
| `DTRU-1024` | dec | 32789 | 1720 KiB | 1812 KiB |
| `DTRU-1024-PACK_PK` | keygen | 34205 | 1716 KiB | 1812 KiB |
| `DTRU-1024-PACK_PK` | enc | 34205 | 1744 KiB | 1812 KiB |
| `DTRU-1024-PACK_PK` | dec | 34205 | 1724 KiB | 1836 KiB |
| `DTRU-1536` | keygen | 33597 | 1696 KiB | 1812 KiB |
| `DTRU-1536` | enc | 33597 | 1716 KiB | 1800 KiB |
| `DTRU-1536` | dec | 33597 | 1728 KiB | 1804 KiB |
| `DTRU-1536-PACK_PK` | keygen | 35013 | 1700 KiB | 1812 KiB |
| `DTRU-1536-PACK_PK` | enc | 35013 | 1756 KiB | 1820 KiB |
| `DTRU-1536-PACK_PK` | dec | 35013 | 1752 KiB | 1840 KiB |
| `DTRU-2048` | keygen | 40589 | 1724 KiB | 1804 KiB |
| `DTRU-2048` | enc | 40589 | 1692 KiB | 1820 KiB |
| `DTRU-2048` | dec | 40589 | 1744 KiB | 1812 KiB |
| `DTRU-2048-PACK_PK` | keygen | 42005 | 1700 KiB | 1800 KiB |
| `DTRU-2048-PACK_PK` | enc | 42005 | 1764 KiB | 1868 KiB |
| `DTRU-2048-PACK_PK` | dec | 42005 | 1776 KiB | 1844 KiB |
| `DTRU-Light` | keygen | 32317 | 1692 KiB | 1784 KiB |
| `DTRU-Light` | enc | 32317 | 1692 KiB | 1804 KiB |
| `DTRU-Light` | dec | 32317 | 1708 KiB | 1800 KiB |
| `DTRU-Prime` | keygen | 36021 | 1700 KiB | 1796 KiB |
| `DTRU-Prime` | enc | 36021 | 1752 KiB | 1844 KiB |
| `DTRU-Prime` | dec | 36021 | 1760 KiB | 1844 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `DTRU-648` | 972 | 1328 | 729 | 32 |
| `DTRU-648-PACK_PK` | 954 | 1310 | 729 | 32 |
| `DTRU-768` | 1152 | 1568 | 960 | 32 |
| `DTRU-768-PACK_PK` | 1130 | 1546 | 960 | 32 |
| `DTRU-1024` | 1536 | 2080 | 1280 | 32 |
| `DTRU-1024-PACK_PK` | 1506 | 2050 | 1280 | 32 |
| `DTRU-1536` | 2304 | 3136 | 1920 | 64 |
| `DTRU-1536-PACK_PK` | 2259 | 3091 | 1920 | 64 |
| `DTRU-2048` | 3072 | 3904 | 2560 | 64 |
| `DTRU-2048-PACK_PK` | 3012 | 3844 | 2560 | 64 |
| `DTRU-Light` | 640 | 864 | 512 | 32 |
| `DTRU-Prime` | 1495 | 1935 | 1359 | 32 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `DTRU-648` | keygen | 0.0% | 27% | drng 2 |
| `DTRU-648` | enc | 25% | 2.8% | drng 1, pseudoXOF 1, pseudohash 1 |
| `DTRU-648` | dec | 26% | 0.0% | pseudoXOF 1, pseudohash 2 |
| `DTRU-648-PACK_PK` | keygen | 0.0% | 27% | drng 2 |
| `DTRU-648-PACK_PK` | enc | 23% | 2.6% | drng 1, pseudoXOF 1, pseudohash 1 |
| `DTRU-648-PACK_PK` | dec | 25% | 0.0% | pseudoXOF 1, pseudohash 2 |
| `DTRU-768` | keygen | 0.0% | 24% | drng 2 |
| `DTRU-768` | enc | 33% | 2.3% | drng 1, pseudoXOF 1, pseudohash 1 |
| `DTRU-768` | dec | 32% | 0.0% | pseudoXOF 1, pseudohash 2 |
| `DTRU-768-PACK_PK` | keygen | 0.0% | 23% | drng 2 |
| `DTRU-768-PACK_PK` | enc | 31% | 2.2% | drng 1, pseudoXOF 1, pseudohash 1 |
| `DTRU-768-PACK_PK` | dec | 30% | 0.0% | pseudoXOF 1, pseudohash 2 |
| `DTRU-1024` | keygen | 0.0% | 26% | drng 2 |
| `DTRU-1024` | enc | 31% | 2.0% | drng 1, pseudoXOF 1, pseudohash 1 |
| `DTRU-1024` | dec | 31% | 0.0% | pseudoXOF 1, pseudohash 2 |
| `DTRU-1024-PACK_PK` | keygen | 0.0% | 26% | drng 2 |
| `DTRU-1024-PACK_PK` | enc | 28% | 1.8% | drng 1, pseudoXOF 1, pseudohash 1 |
| `DTRU-1024-PACK_PK` | dec | 29% | 0.0% | pseudoXOF 1, pseudohash 2 |
| `DTRU-1536` | keygen | 0.0% | 21% | drng 2 |
| `DTRU-1536` | enc | 33% | 1.6% | drng 1, pseudoXOF 1, pseudohash 2 |
| `DTRU-1536` | dec | 30% | 0.0% | pseudoXOF 1, pseudohash 3 |
| `DTRU-1536-PACK_PK` | keygen | 0.0% | 20% | drng 2 |
| `DTRU-1536-PACK_PK` | enc | 30% | 1.4% | drng 1, pseudoXOF 1, pseudohash 2 |
| `DTRU-1536-PACK_PK` | dec | 29% | 0.0% | pseudoXOF 1, pseudohash 3 |
| `DTRU-2048` | keygen | 0.0% | 19% | drng 2 |
| `DTRU-2048` | enc | 31% | 1.2% | drng 1, pseudoXOF 1, pseudohash 2 |
| `DTRU-2048` | dec | 28% | 0.0% | pseudoXOF 1, pseudohash 3 |
| `DTRU-2048-PACK_PK` | keygen | 0.0% | 18% | drng 2 |
| `DTRU-2048-PACK_PK` | enc | 28% | 1.1% | drng 1, pseudoXOF 1, pseudohash 2 |
| `DTRU-2048-PACK_PK` | dec | 27% | 0.0% | pseudoXOF 1, pseudohash 3 |
| `DTRU-Light` | keygen | 0.0% | 21% | drng 2 |
| `DTRU-Light` | enc | 28% | 4.7% | drng 1, pseudoXOF 1, pseudohash 1 |
| `DTRU-Light` | dec | 31% | 0.0% | pseudoXOF 1, pseudohash 2 |
| `DTRU-Prime` | keygen | 0.0% | 0.0% | drng 2 |
| `DTRU-Prime` | enc | 15% | 1.4% | drng 1, pseudoXOF 1, pseudohash 1 |
| `DTRU-Prime` | dec | 18% | 0.0% | pseudoXOF 1, pseudohash 2 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `DTRU-648` | KAT log (sha256 `552778746e9caca0…`) | `kat/kem-14/DTRU-648.log` |
| `DTRU-648` | timing dec | `records/kem-14/DTRU-648__dec.json` |
| `DTRU-648` | timing enc | `records/kem-14/DTRU-648__enc.json` |
| `DTRU-648` | timing keygen | `records/kem-14/DTRU-648__keygen.json` |
| `DTRU-648` | hash profile dec | `profile/kem-14/DTRU-648__dec.json` |
| `DTRU-648` | hash profile enc | `profile/kem-14/DTRU-648__enc.json` |
| `DTRU-648` | hash profile keygen | `profile/kem-14/DTRU-648__keygen.json` |
| `DTRU-648-PACK_PK` | KAT log (sha256 `582f2b72b630b41d…`) | `kat/kem-14/DTRU-648-PACK_PK.log` |
| `DTRU-648-PACK_PK` | timing dec | `records/kem-14/DTRU-648-PACK_PK__dec.json` |
| `DTRU-648-PACK_PK` | timing enc | `records/kem-14/DTRU-648-PACK_PK__enc.json` |
| `DTRU-648-PACK_PK` | timing keygen | `records/kem-14/DTRU-648-PACK_PK__keygen.json` |
| `DTRU-648-PACK_PK` | hash profile dec | `profile/kem-14/DTRU-648-PACK_PK__dec.json` |
| `DTRU-648-PACK_PK` | hash profile enc | `profile/kem-14/DTRU-648-PACK_PK__enc.json` |
| `DTRU-648-PACK_PK` | hash profile keygen | `profile/kem-14/DTRU-648-PACK_PK__keygen.json` |
| `DTRU-768` | KAT log (sha256 `33ae227e3d0058dc…`) | `kat/kem-14/DTRU-768.log` |
| `DTRU-768` | timing dec | `records/kem-14/DTRU-768__dec.json` |
| `DTRU-768` | timing enc | `records/kem-14/DTRU-768__enc.json` |
| `DTRU-768` | timing keygen | `records/kem-14/DTRU-768__keygen.json` |
| `DTRU-768` | hash profile dec | `profile/kem-14/DTRU-768__dec.json` |
| `DTRU-768` | hash profile enc | `profile/kem-14/DTRU-768__enc.json` |
| `DTRU-768` | hash profile keygen | `profile/kem-14/DTRU-768__keygen.json` |
| `DTRU-768-PACK_PK` | KAT log (sha256 `69718c3e56b6717f…`) | `kat/kem-14/DTRU-768-PACK_PK.log` |
| `DTRU-768-PACK_PK` | timing dec | `records/kem-14/DTRU-768-PACK_PK__dec.json` |
| `DTRU-768-PACK_PK` | timing enc | `records/kem-14/DTRU-768-PACK_PK__enc.json` |
| `DTRU-768-PACK_PK` | timing keygen | `records/kem-14/DTRU-768-PACK_PK__keygen.json` |
| `DTRU-768-PACK_PK` | hash profile dec | `profile/kem-14/DTRU-768-PACK_PK__dec.json` |
| `DTRU-768-PACK_PK` | hash profile enc | `profile/kem-14/DTRU-768-PACK_PK__enc.json` |
| `DTRU-768-PACK_PK` | hash profile keygen | `profile/kem-14/DTRU-768-PACK_PK__keygen.json` |
| `DTRU-1024` | KAT log (sha256 `eb9417bdfce79267…`) | `kat/kem-14/DTRU-1024.log` |
| `DTRU-1024` | timing dec | `records/kem-14/DTRU-1024__dec.json` |
| `DTRU-1024` | timing enc | `records/kem-14/DTRU-1024__enc.json` |
| `DTRU-1024` | timing keygen | `records/kem-14/DTRU-1024__keygen.json` |
| `DTRU-1024` | hash profile dec | `profile/kem-14/DTRU-1024__dec.json` |
| `DTRU-1024` | hash profile enc | `profile/kem-14/DTRU-1024__enc.json` |
| `DTRU-1024` | hash profile keygen | `profile/kem-14/DTRU-1024__keygen.json` |
| `DTRU-1024-PACK_PK` | KAT log (sha256 `32ee7a4c59ba2e95…`) | `kat/kem-14/DTRU-1024-PACK_PK.log` |
| `DTRU-1024-PACK_PK` | timing dec | `records/kem-14/DTRU-1024-PACK_PK__dec.json` |
| `DTRU-1024-PACK_PK` | timing enc | `records/kem-14/DTRU-1024-PACK_PK__enc.json` |
| `DTRU-1024-PACK_PK` | timing keygen | `records/kem-14/DTRU-1024-PACK_PK__keygen.json` |
| `DTRU-1024-PACK_PK` | hash profile dec | `profile/kem-14/DTRU-1024-PACK_PK__dec.json` |
| `DTRU-1024-PACK_PK` | hash profile enc | `profile/kem-14/DTRU-1024-PACK_PK__enc.json` |
| `DTRU-1024-PACK_PK` | hash profile keygen | `profile/kem-14/DTRU-1024-PACK_PK__keygen.json` |
| `DTRU-1536` | KAT log (sha256 `b77ff3ceb727427f…`) | `kat/kem-14/DTRU-1536.log` |
| `DTRU-1536` | timing dec | `records/kem-14/DTRU-1536__dec.json` |
| `DTRU-1536` | timing enc | `records/kem-14/DTRU-1536__enc.json` |
| `DTRU-1536` | timing keygen | `records/kem-14/DTRU-1536__keygen.json` |
| `DTRU-1536` | hash profile dec | `profile/kem-14/DTRU-1536__dec.json` |
| `DTRU-1536` | hash profile enc | `profile/kem-14/DTRU-1536__enc.json` |
| `DTRU-1536` | hash profile keygen | `profile/kem-14/DTRU-1536__keygen.json` |
| `DTRU-1536-PACK_PK` | KAT log (sha256 `e873a3b3d85ede4b…`) | `kat/kem-14/DTRU-1536-PACK_PK.log` |
| `DTRU-1536-PACK_PK` | timing dec | `records/kem-14/DTRU-1536-PACK_PK__dec.json` |
| `DTRU-1536-PACK_PK` | timing enc | `records/kem-14/DTRU-1536-PACK_PK__enc.json` |
| `DTRU-1536-PACK_PK` | timing keygen | `records/kem-14/DTRU-1536-PACK_PK__keygen.json` |
| `DTRU-1536-PACK_PK` | hash profile dec | `profile/kem-14/DTRU-1536-PACK_PK__dec.json` |
| `DTRU-1536-PACK_PK` | hash profile enc | `profile/kem-14/DTRU-1536-PACK_PK__enc.json` |
| `DTRU-1536-PACK_PK` | hash profile keygen | `profile/kem-14/DTRU-1536-PACK_PK__keygen.json` |
| `DTRU-2048` | KAT log (sha256 `2df070bcbe2646dc…`) | `kat/kem-14/DTRU-2048.log` |
| `DTRU-2048` | timing dec | `records/kem-14/DTRU-2048__dec.json` |
| `DTRU-2048` | timing enc | `records/kem-14/DTRU-2048__enc.json` |
| `DTRU-2048` | timing keygen | `records/kem-14/DTRU-2048__keygen.json` |
| `DTRU-2048` | hash profile dec | `profile/kem-14/DTRU-2048__dec.json` |
| `DTRU-2048` | hash profile enc | `profile/kem-14/DTRU-2048__enc.json` |
| `DTRU-2048` | hash profile keygen | `profile/kem-14/DTRU-2048__keygen.json` |
| `DTRU-2048-PACK_PK` | KAT log (sha256 `8ae19931b80f6eec…`) | `kat/kem-14/DTRU-2048-PACK_PK.log` |
| `DTRU-2048-PACK_PK` | timing dec | `records/kem-14/DTRU-2048-PACK_PK__dec.json` |
| `DTRU-2048-PACK_PK` | timing enc | `records/kem-14/DTRU-2048-PACK_PK__enc.json` |
| `DTRU-2048-PACK_PK` | timing keygen | `records/kem-14/DTRU-2048-PACK_PK__keygen.json` |
| `DTRU-2048-PACK_PK` | hash profile dec | `profile/kem-14/DTRU-2048-PACK_PK__dec.json` |
| `DTRU-2048-PACK_PK` | hash profile enc | `profile/kem-14/DTRU-2048-PACK_PK__enc.json` |
| `DTRU-2048-PACK_PK` | hash profile keygen | `profile/kem-14/DTRU-2048-PACK_PK__keygen.json` |
| `DTRU-Light` | KAT log (sha256 `8cade79c07b07816…`) | `kat/kem-14/DTRU-Light.log` |
| `DTRU-Light` | timing dec | `records/kem-14/DTRU-Light__dec.json` |
| `DTRU-Light` | timing enc | `records/kem-14/DTRU-Light__enc.json` |
| `DTRU-Light` | timing keygen | `records/kem-14/DTRU-Light__keygen.json` |
| `DTRU-Light` | hash profile dec | `profile/kem-14/DTRU-Light__dec.json` |
| `DTRU-Light` | hash profile enc | `profile/kem-14/DTRU-Light__enc.json` |
| `DTRU-Light` | hash profile keygen | `profile/kem-14/DTRU-Light__keygen.json` |
| `DTRU-Prime` | KAT log (sha256 `4f58eb558d878367…`) | `kat/kem-14/DTRU-Prime.log` |
| `DTRU-Prime` | timing dec | `records/kem-14/DTRU-Prime__dec.json` |
| `DTRU-Prime` | timing enc | `records/kem-14/DTRU-Prime__enc.json` |
| `DTRU-Prime` | timing keygen | `records/kem-14/DTRU-Prime__keygen.json` |
| `DTRU-Prime` | hash profile dec | `profile/kem-14/DTRU-Prime__dec.json` |
| `DTRU-Prime` | hash profile enc | `profile/kem-14/DTRU-Prime__enc.json` |
| `DTRU-Prime` | hash profile keygen | `profile/kem-14/DTRU-Prime__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

