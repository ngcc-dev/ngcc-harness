#!/bin/sh
# Exact public-quotient check on the frozen QCTM KAT keys.
set -eu
cd "$(dirname "$0")/.."

command -v unzip >/dev/null 2>&1 || exit 77
ngcc_qctm_dir=$(mktemp -d /tmp/ngcc-qctm-fold-XXXXXX) || exit 77
trap 'rm -rf -- "$ngcc_qctm_dir"' EXIT HUP INT TERM
ngcc_qctm_zip="$ngcc_qctm_dir/QCTM.zip"

if [ -n "${NGCC_QCTM_ARCHIVE:-}" ]; then
    cp -- "$NGCC_QCTM_ARCHIVE" "$ngcc_qctm_zip" || exit 77
else
    command -v curl >/dev/null 2>&1 || exit 77
    curl -fsSL --retry 2 --max-time 120 \
        'https://www.niccs.org.cn/niccs/Proposal/Public-Key%20Cryptographic%20Algorithms/Round%201%20candidates/QCTM.zip' \
        -o "$ngcc_qctm_zip" || exit 77
fi

printf '%s  %s\n' \
    '24a3986a4fbb852a677267a6443756328eae3642af770e767fe38f8291f294db' \
    "$ngcc_qctm_zip" | sha256sum -c - >/dev/null

mkdir -p "$ngcc_qctm_dir/kem-32/Test_Vectors"
for ngcc_qctm_set in QCTM128 QCTM256 QCTM512; do
    unzip -p "$ngcc_qctm_zip" "Test_Vectors/KAT_KEM_${ngcc_qctm_set}.txt" > \
        "$ngcc_qctm_dir/kem-32/Test_Vectors/KAT_KEM_${ngcc_qctm_set}.txt"
done

python3 security/qctm_fold_audit.py --root "$ngcc_qctm_dir"
echo 'CONFIRMED kem-32-4: public quotient and punctured Goppa row spaces match'
