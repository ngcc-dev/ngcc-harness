# MAMBA-NIKE constant-time review

Scope: MAMBA-NIKE-128 representative reference implementation; other levels share a similar layout but were not exhaustively compiled. The local long-term NIKE secret/noise, private reconciliation vector and derived shared key are secret. Public keys, their fixed lengths, and public matrix seeds are public.

- `MAMBA-NIKE-128/KEX_AlgorithmInstance.c:312-352` derives shared material using the local secret and peer public key. `MAMBA-NIKE-128/error_correction.c:61-95,103-120` processes secret-derived reconciliation values with fixed loop bounds, shifts and masks. Its `f/g/LDDecode` arithmetic at lines 12-57 uses no division instruction or data-indexed table; divisions by powers of two are explicit shifts.
- `MAMBA-NIKE-128/poly.c:57-100` contains a rejection branch (`val < PARAM_Q`) for generating a polynomial from an XOF. Trace the seed before calling this a leak: the matrix seed is public in the NIKE protocol. In the power-of-two-Q path, the loop accepts all 13-bit words.
- `MAMBA-NIKE-128/toom.c:194,215,220` divides secret-derived multiplication intermediates by public compile-time constants 12, 18, and 15. This is a compiler/platform review point, not automatically a variable-time divisor: the divisor is fixed. No secret-dependent `idiv` or timing witness was established in this pass.

No new report was filed. This source review is limited to the cited reference paths and does not certify the whole implementation constant-time.
