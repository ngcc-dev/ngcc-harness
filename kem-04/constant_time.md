# Constant-time review: BAG-Piglet

The master seed in `sk` expands to the PKE secret, and the PKE plaintext, fallback value, and shared secret are secret. The ciphertext, public key, and salt are public. This review traces the bag_piglet128 reference implementation; the same source shape is present in the other submitted parameter directories. It is a source-level audit, not a measured remote attack.

`src/common/ccakem.c:125-132` expands `sk` and decrypts the public ciphertext. `src/scheme/bag_piglet.c:325` implements that secret-key decryption; the decoded word reaches `src/common/augabidulin.c:70-133`, which calls `gabidulin_code_decode_3`. In `src/common/gabidulin.c:273-294`, a data-dependent search tests field discrepancies and changes the pivot index `j`; `gabidulin.c:299-390` branches on discrepancy values and follows different polynomial updates. The decoder therefore has secret-dependent branches and memory access. The branchless final secret selection in `src/common/ccakem.c:136-142` does not remove earlier leakage. The `seed == NULL` key-generation branch and public encoding/length decisions are not vulnerabilities.

No complete key recovery or shared-secret extraction is demonstrated. A source-level witness is the call chain and the data-dependent pivot loop cited above; see `report.md` for the bounded finding.
