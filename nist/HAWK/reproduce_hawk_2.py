#!/usr/bin/env python3
"""Pin and verify the submitted source trace underlying HAWK-2."""

import hashlib
import json
import pathlib
import sys


ROOT = pathlib.Path(__file__).resolve().parent
EXPECTED_SIGN = "da6206462b5d5545e5851c22eb97120a4d4f3bf7fdbfca569d6a3c71bf5f3e27"
EXPECTED_SPEC = "eaccae95bf5508ebdeb7502d2cc1256687dbcd910df2976ce3f8bbbed024638b"


def ordered(text, needles):
    at = -1
    positions = []
    for needle in needles:
        at = text.find(needle, at + 1)
        if at < 0:
            raise ValueError(f"missing source step: {needle}")
        positions.append(at)
    return positions


def main():
    failures = []
    hashes = {}
    traces = {}
    for parameter in ("hawk256", "hawk512", "hawk1024"):
        path = ROOT / f"submission/NIST-SUBMISSION/Reference_Implementation/{parameter}/hawk_sign.c"
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        hashes[parameter] = digest
        if digest != EXPECTED_SIGN:
            failures.append(f"{parameter}: unexpected hawk_sign.c hash {digest}")
        try:
            traces[parameter] = ordered(path.read_text(encoding="utf-8"), [
                "for (uint32_t attempt = 0;; attempt += 2)",
                "basis_m2_mul(logn, t0, t1, h0, h1, f2, g2, F2, G2, xx);",
                "xsn = sig_gauss(logn, rng, rng_context, &scd, x0, t0);",
                "if (xsn > max_xnorm)",
                "continue;",
                "if (y < -lim || y >= lim)",
                "if (encode_sig(logn, tmp, sig_len, salt, salt_len, s1))",
                "return 1;",
                "Hawk_regen_fg(logn, f, g, priv);",
            ])
        except ValueError as error:
            failures.append(f"{parameter}: {error}")
    spec_hash = hashlib.sha256((ROOT / "HAWK-spec.pdf").read_bytes()).hexdigest()
    hashes["HAWK-spec.pdf"] = spec_hash
    if spec_hash != EXPECTED_SPEC:
        failures.append(f"specification: unexpected hash {spec_hash}")

    result = {
        "id": "HAWK-2",
        "status": "FAIL" if failures else "PASS",
        "evidence": "pinned-source ordered-statement witness for the rejection/retry path",
        "hashes": hashes,
        "trace_offsets": traces,
        "limitations": [
            "does not measure timing or attempt-count distributions",
            "does not prove correlation with the long-term basis",
            "does not recover a key or forge a signature",
        ],
        "failures": failures,
    }
    print(json.dumps(result, indent=2, sort_keys=True))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())

