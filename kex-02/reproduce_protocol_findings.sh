#!/bin/sh
set -eu

COMMIT=4d242052e80adbe77745f15edb8f649491b9ad58
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
TMP=$(mktemp -d /tmp/ngcc-afs-protocol.XXXXXX)
trap 'rm -rf -- "$TMP"' EXIT HUP INT TERM

curl -fsSL "https://github.com/acprk/ngcc-round1-cryptanalysis/archive/$COMMIT.tar.gz" |
    tar -xz -C "$TMP"
POC="$TMP/ngcc-round1-cryptanalysis-$COMMIT/afs-kex-auth-binding"
REFROOT="$HERE/Implementations and Test_Vectors"
LOG="$TMP/run.log"
TRIALS=${TR:-200}

(
    cd "$POC"
    CC="${CC:-cc}" TR="$TRIALS" REFROOT="$REFROOT" ./run_all.sh
) | tee "$LOG"

test "$(grep -Fc '[REPLY3] RESULT: CONFIRMED' "$LOG")" -eq 3
test "$(grep -Fc "Bob accepts peer=alice (control,no leak) : 0/$TRIALS" "$LOG")" -eq 3
test "$(grep -Fc '[UKS] RESULT: CONFIRMED' "$LOG")" -eq 3
test "$(grep -Fc '[ORACLE] RESULT: CONFIRMED' "$LOG")" -eq 3
test "$(grep -Fc "honest=$TRIALS/$TRIALS  injected-failure=0/$TRIALS" "$LOG")" -eq 3

echo 'ATTACK kex-02-3 AFS-KEX CONFIRMED leaked in-session K_A permits initiator impersonation; no-leak control rejects'
echo 'ATTACK kex-02-4 AFS-KEX CONFIRMED copied certified public key gives an unknown-key-share'
echo 'ATTACK kex-02-5 AFS-KEX CONFIRMED unauthenticated responder observes the decapsulation-failure bit'
