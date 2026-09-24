#!/usr/bin/env python3
"""Check that Mithril hash/XOF calls use tagged domain wrappers."""

from pathlib import Path


ROOT = Path(__file__).resolve().parent
FAMILIES = [
    "Reference_Implementation",
    "Optimized_Implementation",
    "Additional_Implementation",
]
LEVELS = ["Mithril-128", "Mithril-256", "Mithril-512"]


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def main():
    for family in FAMILIES:
        for level in LEVELS:
            base = ROOT / family / level
            require((base / "hash_domain.h").exists(), f"{base}: missing hash_domain.h")

            hash_domain_h = (base / "hash_domain.h").read_text()
            for label in ["H_F", "H_G", "H_H", "XOF_A", "XOF_S"]:
                require(label in hash_domain_h, f"{base}/hash_domain.h: missing {label} label")
            require(
                "ARCANE-Mithril-v1" in hash_domain_h,
                f"{base}/hash_domain.h: missing Mithril domain prefix",
            )
            require(
                "rrlwr_store64_le" in hash_domain_h and "rrlwr_domain_add_field" in hash_domain_h,
                f"{base}/hash_domain.h: missing length-delimited field encoding",
            )
            require(
                "RRLWR_DOMAIN_XOF_LEN_BYTES 1" in hash_domain_h,
                f"{base}/hash_domain.h: XOF fields still use wide length framing",
            )
            require(
                "rrlwr_domain_encode_xof" in hash_domain_h,
                f"{base}/hash_domain.h: missing compact XOF encoder",
            )
            require(
                "abort()" in hash_domain_h and "memset(out, 0" not in hash_domain_h,
                f"{base}/hash_domain.h: invalid XOF seed length must fail loudly",
            )

            parameters_h = (base / "parameters.h").read_text()
            require(
                'RRLWR_SM3_XOF((output), RRLWR_KEM_HPK_LEN, (input), (input_len))'
                not in parameters_h,
                f"{base}/parameters.h: HASH_F still maps directly to raw pseudoXOF",
            )
            require(
                "shake256(output, RRLWR_KEM_HPK_LEN, input, input_len)" not in parameters_h,
                f"{base}/parameters.h: HASH_F still maps directly to raw SHAKE",
            )

            kem_c = (base / "kem.c").read_text()
            require('#include "hash_domain.h"' in kem_c, f"{base}/kem.c: missing hash_domain.h")
            for wrapper in ["RRLWR_KEM_HASH_F", "RRLWR_KEM_HASH_G", "RRLWR_KEM_HASH_H"]:
                require(wrapper in kem_c, f"{base}/kem.c: missing {wrapper}")

            uniform_c = (base / "arith" / "uniform.c").read_text()
            require(
                '#include "hash_domain.h"' in uniform_c,
                f"{base}/arith/uniform.c: missing hash_domain.h",
            )
            require(
                "RRLWR_XOF_PUBLIC_DOMAIN" in uniform_c
                or "RRLWR_DOMAIN_ENCODE_XOF_PUBLIC" in uniform_c,
                f"{base}/arith/uniform.c: missing public XOF domain wrapper",
            )
            require(
                "RRLWR_XOF_SECRET_DOMAIN" in uniform_c
                or "RRLWR_DOMAIN_ENCODE_XOF_SECRET" in uniform_c,
                f"{base}/arith/uniform.c: missing secret XOF domain wrapper",
            )
            require(
                "memset(r[i], 0" not in uniform_c,
                f"{base}/arith/uniform.c: invalid XOF seed length must not emit zeros",
            )
            require(
                "RRLWR_XOF_SECRET_DOMAIN(buf, RRLWR_K*outlen, seed, seed_len, 0, 0)"
                in uniform_c,
                f"{base}/arith/uniform.c: secret sampler is not documented seed,0,0 stream",
            )

            ring_c = (base / "arith" / "ring.c").read_text()
            if family == "Reference_Implementation":
                require(
                    "for(int i = RRLWR_K - 1; i >= row_min; i--)" in ring_c
                    and "const poly *row = &a->x[RRLWR_K - 1 - i]" in ring_c,
                    f"{base}/arith/ring.c: selected product rows are not emitted high-degree first",
                )
            else:
                require(
                    "rows_rev" in ring_c
                    and "const poly *row = &a->x[ncoeffs - 1 - out]" in ring_c,
                    f"{base}/arith/ring.c: optimized selected product rows are not emitted high-degree first",
                )

    description = (ROOT.parents[2] / "KEM" / "description.tex").read_text()
    require(
        "ARCANE-Mithril-v1" in description,
        "KEM description missing concrete hash-domain encoding",
    )
    require(
        "AMX1" in description and "compact length-delimited encoding" in description,
        "KEM description missing compact XOF encoding",
    )
    for shared_xof_macro in ["\\hashXOFpublic", "\\hashXOFprivate"]:
        require(
            shared_xof_macro not in description,
            f"KEM description should use KEM-local XOF notation instead of {shared_xof_macro}",
        )
    for required_text in [
        "one byte each",
        "16-bit little-endian",
        "32-bit little-endian",
        "64-bit little-endian",
        "index byte \\(0\\)",
        "lane byte \\(0\\)",
        "\\kemXOFprivate(\\seedS, 0, 0)",
        "\\boldsymbol g_{k-1}(y)",
        "\\boldsymbol g_{k-\\ell}(y)",
    ]:
        require(
            required_text in description,
            f"KEM description missing hash-domain field width: {required_text}",
        )


if __name__ == "__main__":
    main()
