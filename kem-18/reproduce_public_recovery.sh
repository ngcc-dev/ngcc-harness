#!/bin/sh
set -eu

URL='https://list.niccs.org.cn/archives/list/pkcforum@list.niccs.org.cn/message/VBOUFRS2CDGNLPEAR4ZBKZM4U5YHBTZY/attachment/4/poc2.zip'
SHA256='a358cef75d5ad1b801300fad4a70ab24dfc513ec9bacb2cb8c0e467e3d6ae951'
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
SRC="$ROOT/Implementations/Reference_Implementation/Loong128"
PYTHON=${NGCC_FPYLLL_PYTHON:-python3}
TMP=$(mktemp -d /tmp/ngcc-loong-public-recovery.XXXXXX)
trap 'rm -rf -- "$TMP"' EXIT HUP INT TERM

curl -fsSL -A 'Mozilla/5.0' -o "$TMP/poc2.zip" "$URL"
printf '%s  %s\n' "$SHA256" "$TMP/poc2.zip" | sha256sum -c -
unzip -q "$TMP/poc2.zip" -d "$TMP"

cc -O2 -I"$SRC" -o "$TMP/dump_loong128_pkct" \
    "$TMP/poc2/dump_loong128_pkct.c" \
    "$SRC/KEM_Loong.c" "$SRC/poly.c" "$SRC/auxfunc.c" "$SRC/drng.c"
"$TMP/dump_loong128_pkct" 66 > "$TMP/pkct.txt"

out=$(cd "$TMP/poc2" && "$PYTHON" attack_public_only.py "$TMP/pkct.txt" --m 90 --block 40)
printf '%s\n' "$out"
printf '%s\n' "$out" | grep -q '^mode: public-only (PK,CT -> SS; no SK / expected SS involved)$'
printf '%s\n' "$out" | grep -q '^r2_candidates=1$'
printf '%s\n' "$out" | grep -q '^message_candidates=4096$'
printf '%s\n' "$out" | grep -q '^FOUND checked='
printf '%s\n' "$out" | grep -Eq '^SS [0-9a-f]{32}$'
echo 'CONFIRMED kem-18-3 public-only Loong128 shared-secret recovery'
