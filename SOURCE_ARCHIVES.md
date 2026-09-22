# Source archive provenance

The candidate reference source files in this repository were copied without
modification from the official NICCS Round 1 submission archives identified in
`downloads.csv`. Source and include files from the relevant reference
implementation trees are retained. Submitted build systems, binaries, object
files and bulky test-vector text files are excluded. The static parameter audit
also retains the exact source files that it cites. The canonical submitted
specification PDF and the extracted pseudocode/parameter review are retained for
all 119 candidates, including candidates without a reported vulnerability.

The `kat.sha256` file in each candidate directory records the expected digest
of every test vector used by the harness. Thus `make test` verifies freshly
generated vectors without storing the original multi-gigabyte vector corpus.

The following SHA-256 values, and sizes where recorded, identify the complete
original ZIPs. They are not required for a normal build.

| id | bytes | SHA-256 |
|---|---:|---|
| hash-04 | 8099370 | `8953f95618236d8d86191992b10e3580ae74520c28a36a715c2fc8bea2a9e8c1` |
| hash-09 | 21377056 | `5e2581d905b9a3d3a8c34c76ed73213c77da970a15cc6b3b26b2bb086b93e3f4` |
| hash-12 | 44195487 | `42ceda5aa3da2fc74c2b21abfaace3f5af208aab40ef00fcca6d18cdd7a0a844` |
| hash-14 | — | `9499a772e532577f85902c0f631a95af8e2fb4bc308da6466c076debd63d1415` |
| hash-17 | 32224403 | `1f9773b8ece90152a6a9adc632a7112c9afc670a5d28b7f1e9ac111e5eea8f13` |
| hash-18 | 19481324 | `70d0796942e60a1ddd25ad2332dbfe2755052825c301e9585167c90c1fa09d3d` |
| hash-19 | — | `dfdd7dd54361fb09e6d0bc47ae6245a9f992f284c56b04ff55d528e2339c9acc` |
| hash-20 | 34365493 | `f68c6b73ff4ac064e6bb8a9b74c4bd8a929594c3c48c89e1a65941e03676ea30` |
| hash-21 | — | `ffa76faaf6ae1c939ef6b1a726d6bcef19f0f0df63daddfe74a82e2831c4c6b3` |
| hash-24 | — | `d2b33441b42825ea10fd374adfdafea64525ad471159b4787c6dc2bd420f05d9` |
| hash-26 | — | `3368eff8f86744eefba82825c11990a3b4d914e014ff184324ab3cb9af88b3a4` |
| hash-31 | — | `b6da323d388d3411b882228152f25626999502b8c246cc0c9b64a0a9d72eaaac` |
| hash-32 | — | `235157f178c0ec885dad4062ee232cdc0d16e95474f05c6f93038fd951ca3712` |
| kem-01 | 13910031 | `1c053133175cd189fd9f0e058bde684168f55a6fc3871de2a755404a08990186` |
| kem-02 | — | `719c438a30cccd72c9d83da4b5150fee0c55351f2c13e7a66fd61bf653ca5c24` |
| kem-06 | — | `612a3fe69c7a28fae7ca87a226d29e67a9cffb6ce9e4cbe54ac9de37f7650d7a` |
| kem-09 | 8357751 | `fc321e46bac9c387535e2053bed560eac3d3cd88ba9bebc154f5bf68dd47dcd1` |
| kem-11 | — | `8fc838488ac0849d4c6afd161f8b9742c7a8b9a330bc3b79df70ced7e2d1d5e2` |
| kem-12 | — | `512caaf5fa04ea5ab51fb5eeee9e81f8c7c7c430b60727130067634bfb674d01` |
| kem-17 | 789966981 | `3780991127f49b6397b4182d32b35ab7a6359bf825a707e90df0f809beeb3790` |
| kem-18 | 8583603 | `a6e5070647a40e7de1bf6e5b085dc1d6423c003a409864fdb303d87f881cec0b` |
| kem-22 | 13023595 | `9fed68e7923c6bc9ede072183f7d3983058f88deec5534e779ed2200ee4f7f9f` |
| kem-23 | — | `ee11788a3e8bf8653d18ade6580c47a331b91e7eac3e31a5a6750a4b4cfb827b` |
| kem-24 | 16810898 | `fdb6749116bddbb8a86074b9ab6f5f55d37540b92e3964906dede0b09c7faf4b` |
| kem-29 | 1081142 | `3ae9d4f1a717fb473e76447014d585cd5d14fe38728f02b1a044cc6a2d03e16d` |
| kem-32 | 87169850 | `24a3986a4fbb852a677267a6443756328eae3642af770e767fe38f8291f294db` |
| kem-36 | — | `03956a13fde3d402513bfcf9942f2b04fd23e98b01a3dc52b48938b9c89fe4d6` |
| kem-38 | — | `f9a1b135ea16aca0c974861732e03cd3164f1288f33ff150ff66c98326b75bcb` |
| kem-39 | 5743165 | `9b0bad96e9bc836b0ff811492a70891066df5634ee47fda4d1518dccd4e09927` |
| kem-40 | — | `fe1bf3d78272bb79048e7d456819160324c6140206798cb77a22c2babcfdcf4a` |
| kex-02 | 9752914 | `7d902107b74870e512c84da82c6bfb386633ca864ddc7df2f7b0b183511f983a` |
| kex-03 | 17998433 | `0b356074741bc20fa82132719e1678e001083b9746f5c4c582425b727b511741` |
| kex-05 | 24191394 | `e9e7ff0fb453a371634797febeaeac7c1f081b5fd2438204787c763667482ec4` |
| kex-06 | — | `79b53ff726121ccb1ce970003c06b605e973c4e4f452dbe780e70cd4c40ca018` |
| kex-07 | 12680002 | `da75005b4060167f25125cbd1a769ed872c8b6fcb770fe0827a599e44536e8e6` |
| sign-01 | 17707823 | `88242576a3ae8f9d090b0c9045020f04ee9b5ae828e01b839f25267959e9c7ea` |
| sign-02 | — | `698fbe834279a4100a66c20f3e0b634c738e1150936acf74b55efc6db6975e8d` |
| sign-03 | 18695378 | `a31de849cf0a0703a4e57decbdf4d97b100d00dc74756feaded2c04a7593e110` |
| sign-04 | — | `b90559ca94bda0130420f91fa52eeb242063a57188c766037c1b234ec3af563b` |
| sign-06 | — | `ce88066506fe9b58c300b3ca51462c7a8350484d88ad5b8a9c9f17b5152ca820` |
| sign-07 | 13640373 | `c790d31cd4a288990f3d692381ed721a06641938a02dfc2e0435b7d323475cef` |
| sign-08 | — | `1846cfe63f0cef83e2e0ca21f5dcadce3c4b16da33713957be6d156e2a9e6e95` |
| sign-09 | — | `19169e545f6a6fd90b844b0deb72db81b5eb73c136873d1dae113c2c39d1c6e0` |
| sign-10 | — | `b3de38fcc2d92ce83290d17cb250dde61f95d3326a49ada36fad3c578f2fc509` |
| sign-11 | — | `f31d434ac297ef6a8c454bf9d219f125290749de157df3eb90d29ae3b8a0270a` |
| sign-12 | 41464713 | `98d57af868fa10d74b2c0656e565aa14a42ff902646d6fc440ffb7355b75342c` |
| sign-15 | 12010922 | `c796b106a7d43b2b3d6110ec2be426aa321cc7336f39a2cc027b2d6e8b4cc8c1` |
| sign-18 | — | `e34f18832e968681dd0c51ce0d4b29d80e01ad29daa83dfb805718b76fdffa80` |
| sign-22 | — | `7b509d21c6674bc23751b8743cb7c2a4a07ee295fafe04e2fa95c0613160bdfd` |
| sign-25 | 19195437 | `cf635de5eebbdeb7b2212e84f349da4ca878889e0a4a59b463d0c8cdfdb8eeab` |
| sign-29 | — | `84affc1f7cbdeec48a7cb6df3349b5708644672f2ac12140c529e74f6d57ca21` |
| sign-31 | — | `7ab1effc5fe911c6ab9483ca44a9d9f6d6d911b03eee9f2873b384d97f9aee1d` |
| sign-32 | 1326858513 | `bbfa8dad5ee57083578b50b9937e773e6158f72646e825da3d3265cc00cb1294` |
| sign-33 | 396850232 | `4b7bb0f15388b395b9308ae480f25622105a734f0ab4a6bd16398438c4b9752a` |
| sign-34 | — | `1eb45f24ad8f7ff25db923479c6500f273aee8b2470d76339030fed8d96b5b31` |
