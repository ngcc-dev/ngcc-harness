# Provenance of the vendored reference code

`upstream/` holds unmodified files from the official CRYSTALS reference
implementations. Only `ref/*.c`, `ref/*.h`, `ref/test/test_vectors.c`, the
licence and the upstream test-vector digests were copied. The ICCS adapters
live in `adapter/`. Licence: public domain (CC0), Apache 2.0 or GPL 2.0, at the
user's choice (`upstream/*/LICENSE`); we use the CC0 option.

| tree | repository | commit | date | standard |
|---|---|---|---|---|
| `upstream/kyber` | https://github.com/pq-crystals/kyber (`main`) | `3edd5af5991927164edd4aacebfcbee00b8064e7` | 2026-08-02 | ML-KEM, FIPS 203 |
| `upstream/dilithium` | https://github.com/pq-crystals/dilithium (`master`) | `d35ba3fe5449bee3e6d43e1f296c3ca818bd36be` | 2026-06-03 | ML-DSA, FIPS 204 |

FIPS 203 features checked in the Kyber tree: encapsulation uses its 32 random
bytes directly (`kem.c`), key generation hashes `d || k` (`indcpa.c`), and the
shared secret is `K` without a further KDF. FIPS 204 features checked in the
Dilithium tree: the context-string API (`sign.h`) and hedged signing by default
(`config.h`, `DILITHIUM_RANDOMIZED_SIGNING`). The Kyber README (July 2026) notes
that the code is no longer maintained for production use; it remains the
designers' reference implementation, which is what the candidates' reference
builds are compared with.

Every build is checked by compiling the upstream `test/test_vectors.c` with the
same flags and comparing the SHA-256 of its output with `upstream/*/SHA256SUMS`
(`tvecs512`, `tvecs768`, `tvecs1024`; `tvecs2`, `tvecs3`, `tvecs5`).

## SHA-256 of the vendored files

```
kyber (3edd5af5991927164edd4aacebfcbee00b8064e7)
53681bce13fd98721b4cabcd0b6184c6a92d9de813a02536161cc706e4ea9f2e  LICENSE
d82afafc6edec201d866bca37d9dd27ef95620f26d087efc52a70d2d33e059b8  ref/api.h
95b6893fd3bf909e57c2522bef413dfdb5c9c677cc1b8c7a246001c64264a238  ref/cbd.c
4c7bebcb19ac24916d906689a50c9e264f99fbc7786793b34d50fd08303c0e76  ref/cbd.h
5df8461d9dc938d6a2960d72b0d331adbfa9beed4116f9d295eef6af238632ee  ref/fips202.c
5d93d468f18ecd9efbeb896a28a075e18ed2ff05c79621d75527f9f3f03a0ab4  ref/fips202.h
7707ba3c80d96862bd6261e55ce633a113156a86e0c524e040226e98a6f4f834  ref/indcpa.c
8cfb9453f7277a211b9f22e472b0e70d6d7895fd78a14125e14193f76727e475  ref/indcpa.h
f73a1329290bd7b722949c9a06a4a578c1962a4ae72d9c825f1fa47e8b7e1138  ref/kem.c
aa35466376918265ba4cd201ea61e9c738f9da591812ea205821fee11e101c3c  ref/kem.h
aeeee3cf363ce87b0617693cee82bed181f1bda5ad90cd355e97a5306bbf65d1  ref/ntt.c
7897ff95c99aa512a1e29b5414da61966756279e7df0e163c5488e83f426ddb0  ref/ntt.h
97175f8cc035ea7f9e487b8145fe97ba490893bae96d9d46658b73c6f7d21601  ref/params.h
4679701512b13f3ff92514531e5abaafa6dcb7544ba6b4025f9dea9c19d14678  ref/poly.c
b59c092caf9ea78b3764c59b1eb7364e51dd1e0a51f5642ab137041400b37c16  ref/poly.h
bc5d7c07cce321ed81eeff65f44c7f2f95d89ba5fb71a257935a4c73bded6702  ref/polyvec.c
650e12181c2459f12fe613bc5241442e4f2742378ef919cbb06083376e6e7ed3  ref/polyvec.h
7fe034fc25b9bf0742296677fe3ffe940cf99e005201e636d44f3cb09225a19c  ref/randombytes.c
5ece412ed498abfd892ecec23480910b76cb8ea1b605da10adc5584ede79f048  ref/randombytes.h
5cdb0497f5eaaab41709a35719596435c661a21620e9b42e8cb9deb44bfc510a  ref/reduce.c
f8a6066958bbf0e0c1937ca8af659e022ab42ea21aeb69e189e2d4ed171be4dd  ref/reduce.h
4b59936d9012ce29ca5326759581053560a91b4d68138cbf62759185a2b41b5c  ref/symmetric.h
91be00d2a7da7ccb6ab758fef838d200f54154169d3835214c94d3d83ca22ee2  ref/symmetric-shake.c
90467f7d29b56413e8962b5ab49339b3e55272188d3ff714540bebaaf9e30c1f  ref/test/test_vectors.c
63c206d0a97170ed9956df61369e00a17dcb23a1500687c6463ab1dc07b07986  ref/verify.c
da62699f86b6e8190bb773bb6c78e74f6dfd6b4ccd8e878261e8d7a02338c483  ref/verify.h
be8afddebad0d43c704bad33a80a8dc76f9e4e300b1550fe97cac7948984fcd8  SHA256SUMS

dilithium (d35ba3fe5449bee3e6d43e1f296c3ca818bd36be)
573f5a0f6eb40308667e3cefd101a3bc3d47a09c034514873bfdffe1fdeaec4a  LICENSE
eff2fe4cca86cb72543628656a40af6720bb66f1781a5fadd55454c695472598  ref/api.h
4f798c78a3b44669fc0febc116b9235692a555987b09d528df3e644415d4dea9  ref/config.h
e4e486d8c839805ae9ad0c2976c3ad4455eeb9a4126835cb1583d7c287ccd9f6  ref/fips202.c
e5e06085263e4b7726c398095fd2c6d65e8f1c714de49537198d937d9dae9bb6  ref/fips202.h
921b76ae7a183305893800e69c0b9ea561bffd2a6e85621767eb789c02a0f8d0  ref/ntt.c
32345efead2c237177f3c0e050c26caf6c9d1772a629664632d9ff32f2693a3e  ref/ntt.h
af51fe6b63164c17a765fcca14afa1ad2bfa11a1389a1964781951cdaf0f6016  ref/packing.c
73b2a0840e23e53edaf9a3509120864ec7ee82b1a2329f35816642a788c96509  ref/packing.h
1d6c116396e3d20f40d72c35a093af2be9f03177282762bdbdb282bb4c4b64af  ref/params.h
845f8991d166c9facbfc9646da77e9e9f6b75f0ea8fb9d629e8ac5f234707d6d  ref/poly.c
2b63cc2ec62c027dd16cf92bd120a4bc4f3a41a94a4a51b2005d366fc30a0330  ref/poly.h
2fa664c380a5bef7e2e9868e0b8f02f8353c4bd81afe0d6328b498baf6864eac  ref/polyvec.c
6523a95d74add7e36129831fd5a72370c6275703fafd1529b6af59ef9645368e  ref/polyvec.h
5058333ccae5c1903f9cfbcbfa19f41260fc97c07e0d39317bd750d51c1d4c86  ref/randombytes.c
5ece412ed498abfd892ecec23480910b76cb8ea1b605da10adc5584ede79f048  ref/randombytes.h
3088cac3d23355a3f9adaac9fd4ee864be9caf5fb0ad30bcb4a4da17dba0e4f4  ref/reduce.c
9abe0b725e0608948eaa74fabd01c4701de5480ce9494a5f6bfa35140eee0399  ref/reduce.h
7223dbbe56b1fba226d536d64eaf05b1ad3f249d33ec767ee655eee471478816  ref/rounding.c
7459644c21e7cbfe48361a1f6b6fda687ff8792f8e0577a0862b3de620558d25  ref/rounding.h
3abd138813b3f803f4f3cbabb706b93e319e19e27b8defa99dd065f8fc416bed  ref/sign.c
a22ea01f54c68888d75db42def615aa7ca6050679894aab3a048904d98830210  ref/sign.h
a272fe9473a968936121a1aa52ac928790bb653b3697fe58ffd3935f08aaaa14  ref/symmetric.h
5b23002163d0dd4a1030e51fed8994a06972644e923df0feca6d1d183f4bdd45  ref/symmetric-shake.c
0de027a30c9395511a658bbf91bbd14177ec5e31e9c50bafd9c1a6d2f02403a9  ref/test/test_vectors.c
990b04769b3f8f7d5cac0467e8bd303d379fbd903d004a56447f8e3ebbab981b  SHA256SUMS
```
