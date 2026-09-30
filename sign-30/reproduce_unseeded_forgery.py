#!/usr/bin/env python3
"""Reproduce the predictable-key forgery in TRINE's normal SHA3 build."""

from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parent
SRC = ROOT / "Implementations" / "Reference_Implementation" / "TRINE-128-ShortSig"

DRIVER = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api.h"

void randombytes_init(unsigned char *, unsigned char *, int);

static int write_file(const char *path, const unsigned char *buf, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    int ok = fwrite(buf, 1, len, f) == len ? 0 : -1;
    fclose(f);
    return ok;
}

static int read_file(const char *path, unsigned char *buf, size_t len) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    int ok = fread(buf, 1, len, f) == len && fgetc(f) == EOF ? 0 : -1;
    fclose(f);
    return ok;
}

int main(int argc, char **argv) {
    unsigned char pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
    if (argc >= 4 && strcmp(argv[1], "keygen") == 0) {
        if (argc == 5) {
            unsigned char entropy[48];
            memset(entropy, (unsigned char)strtoul(argv[4], NULL, 0), sizeof entropy);
            randombytes_init(entropy, NULL, 256);
        }
        if (crypto_sign_keypair(pk, sk) != 0) return 2;
        return write_file(argv[2], pk, sizeof pk) || write_file(argv[3], sk, sizeof sk);
    }
    if (argc == 4 && strcmp(argv[1], "forge") == 0) {
        static const unsigned char message[] = "fresh-message forgery";
        unsigned char signed_message[CRYPTO_BYTES + sizeof message - 1];
        unsigned long long signed_len = 0;
        if (read_file(argv[2], pk, sizeof pk) || read_file(argv[3], sk, sizeof sk)) return 3;
        if (crypto_sign(signed_message, &signed_len, message, sizeof message - 1, sk) != 0) return 4;
        if (signed_len != CRYPTO_BYTES + sizeof message - 1) return 5;
        return crypto_sign_verify(signed_message, CRYPTO_BYTES,
                                  message, sizeof message - 1, pk) == 0 ? 0 : 6;
    }
    return 64;
}
'''


def main() -> int:
    sources = [
        "trine", "util", "field", "matrixmod", "matrixelim", "triform",
        "canonical", "corank1", "trine_expand", "trine_codec", "bitstream",
        "hashkdf", "fips202", "randombytes",
    ]
    with tempfile.TemporaryDirectory(prefix="trine-unseeded-") as directory:
        tmp = Path(directory)
        exe = tmp / "witness"
        command = [
            "gcc", "-x", "c", "-", "-x", "none", "-O3", "-DPARAMS=4",
            "-DUSE_SHA3", f"-I{SRC}",
            *(str(SRC / f"{name}.c") for name in sources),
            "-lssl", "-lcrypto", "-o", str(exe),
        ]
        subprocess.run(command, input=DRIVER, text=True, check=True)

        victim_pk, victim_sk = tmp / "victim.pk", tmp / "victim.sk"
        attacker_pk, attacker_sk = tmp / "attacker.pk", tmp / "attacker.sk"
        seeded_a_pk, seeded_a_sk = tmp / "seed-a.pk", tmp / "seed-a.sk"
        seeded_b_pk, seeded_b_sk = tmp / "seed-b.pk", tmp / "seed-b.sk"
        subprocess.run([exe, "keygen", victim_pk, victim_sk], check=True)
        subprocess.run([exe, "keygen", attacker_pk, attacker_sk], check=True)
        subprocess.run([exe, "keygen", seeded_a_pk, seeded_a_sk, "17"], check=True)
        subprocess.run([exe, "keygen", seeded_b_pk, seeded_b_sk, "34"], check=True)

        assert victim_pk.read_bytes() == attacker_pk.read_bytes()
        assert victim_sk.read_bytes() == attacker_sk.read_bytes()
        assert seeded_a_pk.read_bytes() != seeded_b_pk.read_bytes()
        assert seeded_a_sk.read_bytes() != seeded_b_sk.read_bytes()
        subprocess.run([exe, "forge", victim_pk, attacker_sk], check=True)

    print("fresh_process_public_keys=IDENTICAL")
    print("fresh_process_secret_keys=IDENTICAL")
    print("distinct_seed_control=DISTINCT")
    print("victim_key_fresh_message_forgery=ACCEPTED")
    print("TRINE_UNSEEDED_KEY_FORGERY=CONFIRMED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
