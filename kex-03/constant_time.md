# CreTAKE constant-time review

Scope: representative `CreTAKE128/CreTAKE-K2K-ZEN128` reference wrapper and its shared POLARLAC-128 KEM; the submission contains many K2K/K2S/S2K/S2S combinations and PLAC/ZEN/BiT variants that were not exhaustively traced. Secrets include long-term component keys, ephemeral decapsulation keys, recovered KEM messages, signing secrets where used, and session keys. Peer public keys, transmitted ciphertexts, signatures, identifiers and fixed lengths are public.

- `CreTAKE128/CreTAKE-K2K-ZEN128/twokem.c:107-137` invokes `kem_dec` and branches on its API return status. In the inspected POLARLAC-128 API, `Common/primitives/kem/POLARLAC-128/KEM_AlgorithmInstance.c:193-213` checks pointer/length and primitive return errors; the source trace did not establish that the `twokem` branch reveals a secret value beyond a publicly returned failure.
- `CreTAKE128/CreTAKE-K2K-ZEN128/KEX_AlgorithmInstance.c:189-205` handles peer-message and component decapsulation status; the message/length checks are public-input control flow. The shared KEM code has fixed-parameter loops; rejection sampling of public matrices must not be mistaken for a secret leak.

No new report was filed for this candidate on the inspected path. The multi-primitive, multi-variant source tree needs further full call-graph and compiled-code review; this note must not be read as a constant-time certification.
