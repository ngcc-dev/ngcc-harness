#!/bin/sh
# Native component witness for kem-26-5. It does not recover a secret key.
set -eu
cd "$(dirname "$0")/.."
ngcc_rs_bin=$(mktemp /tmp/ngcc-rs-XXXXXX)
trap 'rm -f "$ngcc_rs_bin"' EXIT
cc -O2 -std=c99 -Wall -Wextra -Wno-unused-function \
    kem-26/reproduce_rs_decoder_iterations.c -o "$ngcc_rs_bin"
"$ngcc_rs_bin"
