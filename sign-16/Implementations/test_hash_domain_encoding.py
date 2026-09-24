#!/usr/bin/env python3
"""Check that Octarine hash/XOF calls use tagged domain wrappers."""

from pathlib import Path


ROOT = Path(__file__).resolve().parent
FAMILIES = [
    "Reference_Implementation",
    "Optimized_Implementation",
    "Additional_Implementation",
]
LEVELS = ["Octarine-128", "Octarine-256", "Octarine-512"]

SIGN_WRAPPERS = [
    "RRLWR_SIGN_HASH_KDF",
    "RRLWR_SIGN_HASH_TR",
    "RRLWR_SIGN_HASH_MU",
    "RRLWR_SIGN_HASH_RHOPP",
    "RRLWR_SIGN_HASH_CH",
]


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def main():
    for family in FAMILIES:
        for level in LEVELS:
            base = ROOT / family / level
            require((base / "hash_domain.h").exists(), f"{base}: missing hash_domain.h")
            hash_domain_h = (base / "hash_domain.h").read_text()
            require(
                "if(field_len != 0)" in hash_domain_h,
                f"{base}/hash_domain.h: zero-length fields are copied unconditionally",
            )
            require(
                "#define RRLWR_DOMAIN_XOF_INPUT_MAX(label, seed_len)" in hash_domain_h,
                f"{base}/hash_domain.h: XOF input bound must be label-aware",
            )
            require(
                "RRLWR_DOMAIN_XOF_INPUT_MAX((size_t)seed_len)" not in hash_domain_h,
                f"{base}/hash_domain.h: XOF single wrappers still use seed_len-sized VLAs",
            )
            require(
                "RRLWR_DOMAIN_XOF_SEED_MAX" in hash_domain_h,
                f"{base}/hash_domain.h: missing fixed XOF seed bound",
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
                "memset(out, 0, outlen);" in hash_domain_h,
                f"{base}/hash_domain.h: invalid XOF seed_len leaves output unchanged",
            )

            sign_c = (base / "sign.c").read_text()
            for wrapper in SIGN_WRAPPERS:
                require(wrapper in sign_c, f"{base}/sign.c: missing {wrapper}")
            require(
                "RRLWR_SIGN_HASH_H(" not in sign_c,
                f"{base}/sign.c: still uses raw RRLWR_SIGN_HASH_H",
            )
            require(
                "unsigned char *mu_w1 = mu;" not in sign_c,
                f"{base}/sign.c: vestigial mu_w1 alias remains in signing path",
            )

            sign_test_c = (base / "sign_test.c").read_text()
            for wrapper in SIGN_WRAPPERS:
                require(wrapper in sign_test_c, f"{base}/sign_test.c: missing {wrapper}")
            require(
                "RRLWR_SIGN_HASH_H(" not in sign_test_c,
                f"{base}/sign_test.c: still uses raw RRLWR_SIGN_HASH_H",
            )
            require(
                "unsigned char *mu_w1 = mu;" not in sign_test_c,
                f"{base}/sign_test.c: vestigial mu_w1 alias remains in signing path",
            )
            require(
                "rhopp[RRLWR_SIGN_RHOPRIMEPRIME_LEN] = kappa" not in sign_test_c,
                f"{base}/sign_test.c: still uses one-byte kappa encoding",
            )
            require(
                "RRLWR_SIGN_RHOPRIMEPRIME_LEN+1" not in sign_test_c,
                f"{base}/sign_test.c: still samples y from one-byte kappa seed",
            )

            uniform_c = (base / "arith" / "uniform.c").read_text()
            require(
                "RRLWR_MAX_SEED_LEN" not in uniform_c,
                f"{base}/arith/uniform.c: dead local RRLWR_MAX_SEED_LEN macro remains",
            )
            require(
                "RRLWR_MAX_DOMAIN_SEED_LEN" not in uniform_c,
                f"{base}/arith/uniform.c: duplicate local XOF seed bound remains",
            )
            require(
                "Support seed lengths up to 128 bytes" not in uniform_c and
                "Support seed lengths up to 129 bytes" not in uniform_c,
                f"{base}/arith/uniform.c: stale RRLWR_MAX_SEED_LEN comment remains",
            )
            if "RRLWR_DOMAIN_ENCODE_XOF_PUBLIC" in uniform_c:
                require(
                    "RRLWR_DOMAIN_XOF_INPUT_MAX(RRLWR_DOMAIN_LABEL_PUBLIC" in uniform_c,
                    f"{base}/arith/uniform.c: public x4 input bound is not label-aware",
                )
                require(
                    uniform_c.count("RRLWR_DOMAIN_ENCODE_XOF_PUBLIC(") == 1,
                    f"{base}/arith/uniform.c: public x4 path repeats full domain encoding",
                )
            if "RRLWR_DOMAIN_ENCODE_XOF_SECRET" in uniform_c:
                require(
                    "RRLWR_DOMAIN_XOF_INPUT_MAX(RRLWR_DOMAIN_LABEL_SECRET" in uniform_c,
                    f"{base}/arith/uniform.c: secret x4 input bound is not label-aware",
                )
                require(
                    uniform_c.count("RRLWR_DOMAIN_ENCODE_XOF_SECRET(") == 1,
                    f"{base}/arith/uniform.c: secret x4 path repeats full domain encoding",
                )
                require(
                    "seed_len < 0 || (size_t)seed_len > RRLWR_DOMAIN_XOF_SEED_MAX" in uniform_c,
                    f"{base}/arith/uniform.c: x4 encoder path lacks seed_len guard",
                )
            require(
                "RRLWR_XOF_PUBLIC_DOMAIN" in uniform_c or "RRLWR_DOMAIN_ENCODE_XOF_PUBLIC" in uniform_c,
                f"{base}/arith/uniform.c: missing public domain encoding",
            )
            require(
                "RRLWR_XOF_SECRET_DOMAIN" in uniform_c or "RRLWR_DOMAIN_ENCODE_XOF_SECRET" in uniform_c,
                f"{base}/arith/uniform.c: missing secret domain encoding",
            )

            sign_ring_c = (base / "sign_ring.c").read_text()
            require(
                "RRLWR_SIGN_HASH_SAMPLE_C_DOMAIN" in sign_ring_c,
                f"{base}/sign_ring.c: missing challenge sampler domain wrapper",
            )
            require(
                "RRLWR_SIGN_HASH_SAMPLE_C(" not in sign_ring_c,
                f"{base}/sign_ring.c: still uses raw challenge sampler hash",
            )
            require(
                "sample_challenge_10b" in sign_ring_c and
                "RRLWR_SIGN_SAMPLE_C_NUM_INDICES" in sign_ring_c,
                f"{base}/sign_ring.c: challenge sampler does not unpack 10-bit indices",
            )
            require(
                "hash_out_buffer[pos++]" not in sign_ring_c,
                f"{base}/sign_ring.c: challenge sampler still uses byte-sized indices",
            )

            unit_tests_c = (base / "test" / "unit_tests_SIGN.c").read_text()
            require(
                "test_uniform_invalid_seed_len" in unit_tests_c,
                f"{base}/test/unit_tests_SIGN.c: missing invalid seed_len coverage",
            )

    description = (ROOT.parents[2] / "Sign" / "description.tex").read_text()
    require("Need domain-separation labels" not in description, "description.tex keeps hash TODO")
    require("H_{\\mathsf{kdf}}" in description, "description.tex missing hash-domain notation")
    require("AOX1" in description, "description.tex missing compact XOF encoding prefix")
    require("compact length-delimited encoding" in description, "description.tex missing compact XOF encoding description")


if __name__ == "__main__":
    main()
