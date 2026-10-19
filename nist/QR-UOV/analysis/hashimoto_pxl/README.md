# Hashimoto zero-guess and PXL analysis

`toy_hashimoto.sage` exercises Hashimoto centrifugation and extraction at the
zero-guess boundary on seeded random homogeneous quadratic maps over GF(31).
It checks the pseudo-oil block structure, residual dimension and Hilbert
series, then lifts a solved residual to a valid preimage of the original map.

Run from the repository root with SageMath:

```sh
sage QR-UOV/analysis/hashimoto_pxl/toy_hashimoto.sage
```

These are generic MQ toys. They do not construct the submitted quotient-ring
QR-UOV public map and do not execute PXL. Their role is limited to checking
that the Hashimoto construction remains coherent at `k_H=0`.

`structured_toy.py` supplies the missing quotient-ring check without external
packages. It uses the submitted `q=31`, `ell=3`, `f=X^3-X-1` field and block
encoding, at smaller dimensions `V=3`, `M=2`. It samples Algorithm 1's
`P1`, `P2`, and `S'`, derives `P3`, expands every block as `W Phi_f(g)`, and
checks the hidden oil space before applying the same zero-guess extraction.

```sh
python3 QR-UOV/analysis/hashimoto_pxl/structured_toy.py --trials 3
```

For each seed it checks the predicted 4-by-4 residual, computes the homogeneous
Hilbert function through degree 5, and brute-forces the residual to verify a
preimage under the original structured map. Seeds `20261018` through
`20261027` all produced `[1, 4, 6, 4, 1, 0]`, the coefficients of `(1+z)^4`
followed by zero, and a valid preimage. Two seeds needed one centrifugation
restart; the other eight needed none. This is finite-dimensional evidence at
toy size. It does not establish semi-regularity at the submitted dimensions or
implement PXL.

The dependency-free report reproducer is
`QR-UOV/reproduce_qruov_4.py`. It evaluates the full-size specification
formulas and their finite-exponent and memory sensitivities.
