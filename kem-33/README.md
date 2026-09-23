# QUBE reference KAT caveat

`make -C kem-33 test` is expected to report four `MISMATCH` results and one
`NOKAT`. This is a mismatch between the submitted reference source and the
submitted top-level `Test_Vectors/`, not a harness build failure. The reference
tree reproduces its own in-tree KAT files byte for byte, but the top-level
vectors have the optimized implementation's key/ciphertext sizes at the four
listed levels; no top-level qube-192 vector was supplied. See the comments in
`Makefile` and the comparison in `pseudocode.md` for details.

The `kem-33-1` constant-time witness is independent of these KAT mismatches:

```sh
make -C kem-33 exploit
```
