#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
command -v cc >/dev/null 2>&1 || { echo 'SKIP kem-19-3: cc unavailable'; exit 77; }
ngcc_bch_tmp=$(mktemp -d "${TMPDIR:-/tmp}/ngcc-lore-bch.XXXXXX") || exit 77
trap 'rm -rf "$ngcc_bch_tmp"' EXIT
cc -O2 -std=c99 -Wall -Wextra -Wno-unused-function -DLORE_LEVEL=2 \
    kem-19/reproduce_bch_work.c -o "$ngcc_bch_tmp/reproduce_bch_work"
"$ngcc_bch_tmp/reproduce_bch_work"
