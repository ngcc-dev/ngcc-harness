#!/bin/sh
set -eu

COMMIT=4b296c380d6be61e35c824a9e489d6cdcf548651
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
TMP=$(mktemp -d /tmp/ngcc-weaver-bch.XXXXXX)
trap 'rm -rf -- "$TMP"' EXIT HUP INT TERM

git clone --quiet https://github.com/acprk/ngcc-round1-cryptanalysis.git "$TMP/audit"
git -C "$TMP/audit" checkout --quiet "$COMMIT"
test "$(git -C "$TMP/audit" rev-parse HEAD)" = "$COMMIT"

cd "$TMP/audit/weaver-bch-decoder"
REF="$HERE/Implementations/Reference_Implementation" ./run_all.sh
