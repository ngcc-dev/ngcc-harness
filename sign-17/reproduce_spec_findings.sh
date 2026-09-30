#!/bin/sh
set -eu

COMMIT=4d242052e80adbe77745f15edb8f649491b9ad58
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
TMP=$(mktemp -d /tmp/ngcc-ops-spec.XXXXXX)
trap 'rm -rf -- "$TMP"' EXIT HUP INT TERM

curl -fsSL "https://github.com/acprk/ngcc-round1-cryptanalysis/archive/$COMMIT.tar.gz" |
    tar -xz -C "$TMP"
POC="$TMP/ngcc-round1-cryptanalysis-$COMMIT/ops-sig-spec-sampler-forgery"
REFROOT="$HERE/Implementations and Test_Vectors/Implementations/Reference_Implementation"
TOYTAU=${TOYTAU:-14}

for item in OPSsig-128:149 OPSsig-256:630 OPSsig-512:1339; do
    level=${item%%:*}
    failures=${item##*:}
    log="$TMP/$level.log"
    (
        cd "$POC"
        rm -rf work
        REF="$REFROOT/$level" CC="${CC:-gcc}" TOYTAU="$TOYTAU" \
            ./run_all.sh
    ) | tee "$log"

    grep -Fq '>>> FIXED SUPPORT:' "$log"
    grep -Fq 'RESULT: FORGERY CONFIRMED (public key only, no signing query)' "$log"
    grep -Fq 'control: must be nonzero' "$log"
    grep -Fq 'REJECT -- forgery is specific to the spec' "$log"
    grep -Fq "verify-FAILURES=$failures" "$log"
    grep -Fq 'verify-FAILURES=0  (0.00%)' "$log"
done

echo 'ATTACK sign-17-1 OPS-SIG CONFIRMED fixed-support challenge and scaled public-key-only forgery with reject controls'
echo 'ATTACK sign-17-2 OPS-SIG CONFIRMED specified rounding rejects honest signatures while the reference control accepts'
